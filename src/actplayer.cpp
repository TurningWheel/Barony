/*-------------------------------------------------------------------------------

	BARONY
	File: actplayer.cpp
	Desc: behavior function(s) for player

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "messages.hpp"
#include "entity.hpp"
#include "interface/interface.hpp"
#include "engine/audio/sound.hpp"
#include "items.hpp"
#include "magic/magic.hpp"
#include "menu.hpp"
#include "scores.hpp"
#include "monster.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "player.hpp"
#include "colors.hpp"
#include "draw.hpp"
#include "mod_tools.hpp"
#include "classdescriptions.hpp"
#include "ui/MainMenu.hpp"
#include "interface/consolecommand.hpp"

bool settings_smoothmouse = false;
bool usecamerasmoothing = false;
bool disablemouserotationlimit = true;
bool settings_disablemouserotationlimit = false;
bool swimDebuffMessageHasPlayed = false;
bool partymode = false;

/*-------------------------------------------------------------------------------

	act*

	The following functions describe various entity behaviors. All functions
	take a pointer to the entities that use them as an argument.

-------------------------------------------------------------------------------*/

#define DEATHCAM_TIME my->skill[0]
#define DEATHCAM_PLAYERTARGET my->skill[1]
#define DEATHCAM_PLAYERNUM my->skill[2]
#define DEATHCAM_IDLETIME my->skill[3]
#define DEATHCAM_IDLEROTATEDIRYAW my->skill[4]
#define DEATHCAM_IDLEROTATEPITCHINIT my->skill[5]
#define DEATHCAM_ROTX my->fskill[0]
#define DEATHCAM_ROTY my->fskill[1]
#define DEATHCAM_IDLEPITCH my->fskill[2]
#define DEATHCAM_IDLEPITCH_START my->fskill[3]
#define DEATHCAM_IDLEPITCH_FLOAT_TO_ZERO my->fskill[4]
#define DEATHCAM_IDLEYAWSPEED my->fskill[5]

void actDeathCam(Entity* my)
{
	/*if ( keystatus[SDLK_F4] )
	{
		buttonStartSingleplayer(nullptr);
		keystatus[SDLK_F4] = 0;
	}*/
	DEATHCAM_TIME++;

	Uint32 deathcamGameoverPromptTicks = *MainMenu::cvar_fastRestart ? TICKS_PER_SECOND :
		(splitscreen ? TICKS_PER_SECOND * 3 : TICKS_PER_SECOND * 6);
	if ( gameModeManager.getMode() == GameModeManager_t::GAME_MODE_TUTORIAL )
	{
		deathcamGameoverPromptTicks = TICKS_PER_SECOND * 3;
	}

	real_t mousex_relative = mousexrel;
	real_t mousey_relative = mouseyrel;

	mousex_relative = inputs.getMouseFloat(DEATHCAM_PLAYERNUM, Inputs::ANALOGUE_XREL);
	mousey_relative = inputs.getMouseFloat(DEATHCAM_PLAYERNUM, Inputs::ANALOGUE_YREL);

    const bool smoothmouse = playerSettings[DEATHCAM_PLAYERNUM].smoothmouse;
    const bool reversemouse = playerSettings[DEATHCAM_PLAYERNUM].reversemouse;
	real_t mouse_speed = playerSettings[DEATHCAM_PLAYERNUM].mousespeed;
	if ( inputs.getVirtualMouse(DEATHCAM_PLAYERNUM)->lastMovementFromController )
	{
		mouse_speed = 32.0;
	}

	if ( DEATHCAM_TIME == 1 )
	{
		DEATHCAM_PLAYERTARGET = DEATHCAM_PLAYERNUM;
		DEATHCAM_IDLEROTATEDIRYAW = (local_rng.rand() % 2 == 0) ? 1 : -1;
	}
	else if ( DEATHCAM_TIME == deathcamGameoverPromptTicks )
	{
		if ( gameModeManager.getMode() == GameModeManager_t::GAME_MODE_TUTORIAL )
		{
			gameModeManager.Tutorial.openGameoverWindow();
		}
		else
		{
			MainMenu::openGameoverWindow(DEATHCAM_PLAYERNUM);
		}
		DEATHCAM_IDLETIME = TICKS_PER_SECOND * 3;
	}

	if ( DEATHCAM_TIME >= deathcamGameoverPromptTicks )
	{
		if ( !players[DEATHCAM_PLAYERNUM]->GUI.isGameoverActive() )
		{
			players[DEATHCAM_PLAYERNUM]->bControlEnabled = true;
		}
	}

	bool shootmode = players[DEATHCAM_PLAYERNUM]->shootmode;
	if ( shootmode && !gamePaused )
	{
		if ( !players[DEATHCAM_PLAYERNUM]->GUI.isGameoverActive() )
		{
			if ( smoothmouse )
			{
				DEATHCAM_ROTX += mousex_relative * .006 * (mouse_speed / 128.f);
				DEATHCAM_ROTX = fmin(fmax(-0.35, DEATHCAM_ROTX), 0.35);
			}
			else
			{
				DEATHCAM_ROTX = std::min<float>(std::max<float>(-0.35f, mousex_relative * .01f * (mouse_speed / 128.f)), 0.35f);
			}
			my->yaw += DEATHCAM_ROTX;
			if ( my->yaw >= PI * 2 )
			{
				my->yaw -= PI * 2;
			}
			else if ( my->yaw < 0 )
			{
				my->yaw += PI * 2;
			}

			if ( smoothmouse )
			{
				DEATHCAM_ROTY += mousey_relative * .006 * (mouse_speed / 128.f) * (reversemouse * 2 - 1);
				DEATHCAM_ROTY = fmin(fmax(-0.35, DEATHCAM_ROTY), 0.35);
			}
			else
			{
				DEATHCAM_ROTY = std::min<float>(std::max<float>(-0.35f, mousey_relative * .01f * (mouse_speed / 128.f) * (reversemouse * 2 - 1)), 0.35f);
			}
			my->pitch -= DEATHCAM_ROTY;
			if ( my->pitch > PI / 2 )
			{
				my->pitch = PI / 2;
			}
			else if ( my->pitch < -PI / 2 )
			{
				my->pitch = -PI / 2;
			}
		}

		if ( abs(DEATHCAM_ROTX) < 0.0001 && abs(DEATHCAM_ROTY) < 0.0001
			&& DEATHCAM_PLAYERTARGET == DEATHCAM_PLAYERNUM
			&& (DEATHCAM_TIME >= deathcamGameoverPromptTicks) )
		{
			++DEATHCAM_IDLETIME;
			if ( DEATHCAM_IDLETIME >= TICKS_PER_SECOND * 3 )
			{
				my->yaw += DEATHCAM_IDLEYAWSPEED * DEATHCAM_IDLEROTATEDIRYAW;
				if ( my->yaw >= PI * 2 )
				{
					my->yaw -= PI * 2;
				}
				else if ( my->yaw < 0 )
				{
					my->yaw += PI * 2;
				}

				if ( !DEATHCAM_IDLEROTATEPITCHINIT )
				{
					DEATHCAM_IDLEROTATEPITCHINIT = 1;
					DEATHCAM_IDLEPITCH_START = my->pitch;
					DEATHCAM_IDLEPITCH = 0.0;
					DEATHCAM_IDLEPITCH_FLOAT_TO_ZERO = 0.0;
					DEATHCAM_IDLEYAWSPEED = 0.0;
				}
				DEATHCAM_IDLEPITCH += PI / 1024;
				if ( DEATHCAM_IDLEPITCH >= PI * 2 )
				{
					DEATHCAM_IDLEPITCH -= PI * 2;
				}
				else if ( DEATHCAM_IDLEPITCH < 0 )
				{
					DEATHCAM_IDLEPITCH += PI * 2;
				}
				if ( (DEATHCAM_IDLEPITCH_START - DEATHCAM_IDLEPITCH_FLOAT_TO_ZERO) > .001 )
				{
					DEATHCAM_IDLEPITCH_FLOAT_TO_ZERO += .0001;
				}
				else if ( (DEATHCAM_IDLEPITCH_START - DEATHCAM_IDLEPITCH_FLOAT_TO_ZERO) < .001 )
				{
					DEATHCAM_IDLEPITCH_FLOAT_TO_ZERO -= .0001;
				}
				my->pitch = -DEATHCAM_IDLEPITCH_FLOAT_TO_ZERO + DEATHCAM_IDLEPITCH_START + (PI / 64) * sin(DEATHCAM_IDLEPITCH);
				if ( my->pitch > PI / 2 )
				{
					my->pitch = PI / 2;
				}
				else if ( my->pitch < -PI / 2 )
				{
					my->pitch = -PI / 2;
				}

				if ( DEATHCAM_IDLEYAWSPEED < .001 )
				{
					DEATHCAM_IDLEYAWSPEED += .00001;
				}
			}
		}
		else
		{
			DEATHCAM_IDLETIME = 0;
			DEATHCAM_IDLEROTATEPITCHINIT = 0;
			DEATHCAM_IDLEPITCH_FLOAT_TO_ZERO = 0.0;
			DEATHCAM_IDLEYAWSPEED = 0.0;
		}
	}
	if ( smoothmouse )
	{
		DEATHCAM_ROTX *= .5;
		DEATHCAM_ROTY *= .5;
	}
	else
	{
		DEATHCAM_ROTX = 0;
		DEATHCAM_ROTY = 0;
	}

	if ( players[DEATHCAM_PLAYERNUM] && players[DEATHCAM_PLAYERNUM]->entity )
	{
		// do nothing if still alive
	}
	else if ( shootmode
		&& !players[DEATHCAM_PLAYERNUM]->GUI.isGameoverActive()
		&& players[DEATHCAM_PLAYERNUM]->bControlEnabled
		&& !players[DEATHCAM_PLAYERNUM]->usingCommand()
		&& !gamePaused
		&& (Input::inputs[DEATHCAM_PLAYERNUM].consumeBinaryToggle("Attack")
			|| Input::inputs[DEATHCAM_PLAYERNUM].consumeBinaryToggle("MenuConfirm")) )
	{
		DEATHCAM_PLAYERTARGET++;
		if (DEATHCAM_PLAYERTARGET >= MAXPLAYERS)
		{
			DEATHCAM_PLAYERTARGET = 0;
		}
		int c = 0;
		while (!players[DEATHCAM_PLAYERTARGET] || !players[DEATHCAM_PLAYERTARGET]->entity)
		{
			if (c > MAXPLAYERS)
			{
				break;
			}
			DEATHCAM_PLAYERTARGET++;
			if (DEATHCAM_PLAYERTARGET >= MAXPLAYERS)
			{
				DEATHCAM_PLAYERTARGET = 0;
			}
			c++;
		}
	}

	if (DEATHCAM_PLAYERTARGET >= 0)
	{
		if (players[DEATHCAM_PLAYERTARGET] && players[DEATHCAM_PLAYERTARGET]->entity)
		{
			my->x = players[DEATHCAM_PLAYERTARGET]->entity->x;
			my->y = players[DEATHCAM_PLAYERTARGET]->entity->y;
		}
		else
		{
			DEATHCAM_PLAYERTARGET = DEATHCAM_PLAYERNUM;
		}
	}

	my->removeLightField();
	my->light = addLight(my->x / 16, my->y / 16, "deathcam");

	real_t camx, camy, camz, camang, camvang;
	camx = my->x / 16.f;
	camy = my->y / 16.f;
	camz = my->z * 2.f;
	camang = my->yaw;
	camvang = my->pitch;

	camx -= cos(my->yaw) * cos(my->pitch) * 1.5;
	camy -= sin(my->yaw) * cos(my->pitch) * 1.5;
	camz -= sin(my->pitch) * 16;

	if ( !TimerExperiments::bUseTimerInterpolation )
	{
		cameras[DEATHCAM_PLAYERNUM].x = camx;
		cameras[DEATHCAM_PLAYERNUM].y = camy;
		cameras[DEATHCAM_PLAYERNUM].z = camz;
		cameras[DEATHCAM_PLAYERNUM].ang = camang;
		cameras[DEATHCAM_PLAYERNUM].vang = camvang;
		return;
	}
	else
	{
		TimerExperiments::cameraCurrentState[DEATHCAM_PLAYERNUM].x.velocity = 
			TimerExperiments::lerpFactor * (camx - TimerExperiments::cameraCurrentState[DEATHCAM_PLAYERNUM].x.position);
		TimerExperiments::cameraCurrentState[DEATHCAM_PLAYERNUM].y.velocity = 
			TimerExperiments::lerpFactor * (camy - TimerExperiments::cameraCurrentState[DEATHCAM_PLAYERNUM].y.position);
		TimerExperiments::cameraCurrentState[DEATHCAM_PLAYERNUM].z.velocity = 
			TimerExperiments::lerpFactor * (camz - TimerExperiments::cameraCurrentState[DEATHCAM_PLAYERNUM].z.position);

		real_t diff = camang - TimerExperiments::cameraCurrentState[DEATHCAM_PLAYERNUM].yaw.position;
		if ( diff > PI )
		{
			diff -= 2 * PI;
		}
		else if ( diff < -PI )
		{
			diff += 2 * PI;
		}
		TimerExperiments::cameraCurrentState[DEATHCAM_PLAYERNUM].yaw.velocity = diff * TimerExperiments::lerpFactor;
		diff = camvang - TimerExperiments::cameraCurrentState[DEATHCAM_PLAYERNUM].pitch.position;
		if ( diff >= PI )
		{
			diff -= 2 * PI;
		}
		else if ( diff < -PI )
		{
			diff += 2 * PI;
		}
		TimerExperiments::cameraCurrentState[DEATHCAM_PLAYERNUM].pitch.velocity = diff * TimerExperiments::lerpFactor;
	}
}

#define PLAYER_INIT my->skill[0]
#define PLAYER_TORCH my->skill[1]
#define PLAYER_NUM my->skill[2]
#define PLAYER_DEBUGCAM my->skill[3]
#define PLAYER_BOBMODE my->skill[4]
#define PLAYER_ATTACK my->skill[9]
#define PLAYER_ATTACKTIME my->skill[10]
#define PLAYER_ARMBENDED my->skill[11]
#define PLAYER_ALIVETIME my->skill[12]
#define PLAYER_INWATER my->skill[13]
#define PLAYER_CLICKED my->skill[14]
#define PLAYER_DEATH_AUTOMATON my->skill[15]
#define PLAYER_VELX my->vel_x
#define PLAYER_VELY my->vel_y
#define PLAYER_VELZ my->vel_z
#define PLAYER_BOB my->fskill[0]
#define PLAYER_BOBMOVE my->fskill[1]
#define PLAYER_WEAPONYAW my->fskill[2]
#define PLAYER_DX my->fskill[3]
#define PLAYER_DY my->fskill[4]
#define PLAYER_DYAW my->fskill[5]
#define PLAYER_ROTX my->fskill[6]
#define PLAYER_ROTY my->fskill[7]
#define PLAYER_SHIELDYAW my->fskill[8]
#define PLAYER_SIDEBOB my->fskill[10]
#define PLAYER_CAMERAZ_ACCEL my->fskill[14]
#define PLAYERWALKSPEED .12

bool Player::PlayerMovement_t::isPlayerSwimming()
{
	if ( !players[player.playernum]->entity  || !stats[player.playernum])
	{
		return false;
	}

	Entity* my = players[player.playernum]->entity;

	// swimming
	bool waterwalkingboots = false;
	if ( stats[PLAYER_NUM]->shoes != NULL )
	{
		if ( stats[PLAYER_NUM]->shoes->type == IRON_BOOTS_WATERWALKING )
		{
			waterwalkingboots = true;
		}
	}
	bool swimming = false;
	bool levitating = isLevitating(stats[PLAYER_NUM]);
	if ( !levitating && !waterwalkingboots && !noclip && !skillCapstoneUnlocked(PLAYER_NUM, PRO_SWIMMING) )
	{
		int x = std::min(std::max<unsigned int>(0, floor(my->x / 16)), map.width - 1);
		int y = std::min(std::max<unsigned int>(0, floor(my->y / 16)), map.height - 1);
		if ( swimmingtiles[map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height]]
			|| lavatiles[map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height]] )
		{
			// can swim in lavatiles or swimmingtiles only.
			swimming = true;
		}
	}
	return swimming;
}

bool Player::PlayerMovement_t::handleQuickTurn(bool useRefreshRateDelta)
{
	if ( !players[player.playernum]->entity || !bDoingQuickTurn )
	{
		quickTurnRotation = 0.0;
		bDoingQuickTurn = false;
		quickTurnStartTicks = 0;
		return false;
	}

	Entity* my = players[player.playernum]->entity;
	double refreshRateDelta = 1.0;
	if ( useRefreshRateDelta && fps > 0.0 )
	{
		refreshRateDelta *= TICKS_PER_SECOND / (real_t)fpsLimit;
	}

	if ( abs(quickTurnRotation) > 0.001 )
	{
		int dir = ((quickTurnRotation > 0) ? 1 : -1);
		if ( my->ticks - quickTurnStartTicks < 15 )
		{
			PLAYER_ROTX = dir * players[player.playernum]->settings.quickTurnSpeed;
		}
		else
		{
			PLAYER_ROTX = std::max(0.01, (dir * PI / 15) * pow(0.99, my->ticks - quickTurnStartTicks));
		}

		if ( dir == 1 )
		{
			quickTurnRotation = std::max(0.0, quickTurnRotation - PLAYER_ROTX * refreshRateDelta);
		}
		else
		{
			quickTurnRotation = std::min(0.0, quickTurnRotation - PLAYER_ROTX * refreshRateDelta);
		}
		return true;
	}
	else
	{
		bDoingQuickTurn = false;
		return false;
	}
}

void Player::PlayerMovement_t::startQuickTurn()
{
	if ( !players[player.playernum]->entity || bDoingQuickTurn )
	{
		return;
	}

	Entity* my = players[player.playernum]->entity;

	if ( gamePaused || !stats[PLAYER_NUM] || !my->isMobile() )
	{
		return;
	}

	quickTurnRotation = PI * players[PLAYER_NUM]->settings.quickTurnDirection;
	if ( stats[PLAYER_NUM]->EFFECTS[EFF_CONFUSED] )
	{
		quickTurnRotation = -quickTurnRotation;
	}

	quickTurnStartTicks = players[PLAYER_NUM]->entity->ticks;
	bDoingQuickTurn = true;
	// code for moving left -> turn left, moving right -> turn right
	// maybe better for 1 direction only.
	/*if ( abs(PLAYER_VELY) > 0.001 || abs(PLAYER_VELX) > 0.001 )
	{
	real_t tangent = atan2(PLAYER_VELY, PLAYER_VELX);
	real_t playerFacingDir = my->yaw;
	while ( playerFacingDir >= 2 * PI )
	{
	playerFacingDir -= PI * 2;
	}
	while ( tangent < -PI )
	{
	playerFacingDir += PI * 2;
	}
	real_t difference = tangent - playerFacingDir;
	if ( difference < -PI )
	{
	difference += 2 * PI;
	}
	if ( difference > PI )
	{
	difference -= 2 * PI;
	}

	if ( difference >= 0.0f )
	{
	quickTurnRotation[playernum] = PI;
	}
	if ( difference < 0.0f )
	{
	quickTurnRotation[playernum] = -PI;
	}
	//messagePlayer(0, "%.2f, %.2f, %.2f", tangent, playerFacingDir, (PI - abs(abs(playerFacingDir - tangent) - PI)) * 2);
	}*/
}

void Player::PlayerMovement_t::handlePlayerCameraUpdate(bool useRefreshRateDelta)
{
	if ( !players[player.playernum]->entity )
	{
		return;
	}

	Entity* my = players[player.playernum]->entity;
	int playernum = player.playernum;

	real_t mousex_relative = mousexrel;
	real_t mousey_relative = mouseyrel;

	mousex_relative = inputs.getMouseFloat(playernum, Inputs::ANALOGUE_XREL);
	mousey_relative = inputs.getMouseFloat(playernum, Inputs::ANALOGUE_YREL);

    const bool smoothmouse = playerSettings[PLAYER_NUM].smoothmouse;
    const bool reversemouse = playerSettings[PLAYER_NUM].reversemouse;
	real_t mouse_speed = playerSettings[PLAYER_NUM].mousespeed;
	if ( inputs.getVirtualMouse(PLAYER_NUM)->lastMovementFromController )
	{
		mouse_speed = 32.0;
	}

	double refreshRateDelta = 1.0;
	if ( useRefreshRateDelta && fps > 0.0 )
	{
		refreshRateDelta *= TICKS_PER_SECOND / (real_t)fpsLimit;
	}
	if ( player.shootmode && !player.usingCommand()
		&& !gamePaused
		&& player.bControlEnabled
		&& player.hotbar.faceMenuButtonHeld == Hotbar_t::FaceMenuGroup::GROUP_NONE )
	{
		if ( Input::inputs[playernum].consumeBinaryToggle("Quick Turn") )
		{
			startQuickTurn();
		}
	}

	// rotate
	if ( !player.usingCommand()
		&& player.bControlEnabled && !gamePaused && my->isMobile() && !inputs.hasController(PLAYER_NUM) )
	{
		if ( !stats[playernum]->EFFECTS[EFF_CONFUSED] )
		{
			if ( noclip )
			{
				my->z -= (Input::inputs[playernum].analog("Turn Right") - Input::inputs[playernum].analog("Turn Left")) * .25 * refreshRateDelta;
			}
			else
			{
				my->yaw += (Input::inputs[playernum].analog("Turn Right") - Input::inputs[playernum].analog("Turn Left")) * .05 * refreshRateDelta;
			}
		}
		else
		{
			my->yaw += (Input::inputs[playernum].analog("Turn Left") - Input::inputs[playernum].analog("Turn Right")) * .05 * refreshRateDelta;
		}
	}
	bool shootmode = players[PLAYER_NUM]->shootmode;

	if ( handleQuickTurn(useRefreshRateDelta) )
	{
		// do nothing, override rotations.
	}
	else if ( shootmode && !gamePaused )
	{
		if ( !stats[PLAYER_NUM]->EFFECTS[EFF_CONFUSED] )
		{
			if ( smoothmouse )
			{
				if ( my->isMobile() )
				{
					PLAYER_ROTX += mousex_relative * .006 * (mouse_speed / 128.f);
				}
				if ( !disablemouserotationlimit )
				{
					PLAYER_ROTX = fmin(fmax(-0.35, PLAYER_ROTX), 0.35);
				}
				PLAYER_ROTX *= pow(0.5, refreshRateDelta);
			}
			else
			{
				if ( my->isMobile() )
				{
					if ( disablemouserotationlimit )
					{
						PLAYER_ROTX = mousex_relative * .01f * (mouse_speed / 128.f);
					}
					else
					{
						PLAYER_ROTX = std::min<float>(std::max<float>(-0.35f, mousex_relative * .01f * (mouse_speed / 128.f)), 0.35f);
					}
				}
				else
				{
					PLAYER_ROTX = 0;
				}
			}
		}
		else
		{
			if ( smoothmouse )
			{
				if ( my->isMobile() )
				{
					PLAYER_ROTX -= mousex_relative * .006f * (mouse_speed / 128.f);
				}
				if ( !disablemouserotationlimit )
				{
					PLAYER_ROTX = fmin(fmax(-0.35f, PLAYER_ROTX), 0.35f);
				}
				PLAYER_ROTX *= pow(0.5, refreshRateDelta);
			}
			else
			{
				if ( my->isMobile() )
				{
					if ( disablemouserotationlimit )
					{
						PLAYER_ROTX = -mousex_relative * .01f * (mouse_speed / 128.f);
					}
					else
					{
						PLAYER_ROTX = -std::min<float>(std::max<float>(-0.35f, mousex_relative * .01f * (mouse_speed / 128.f)), 0.35f);
					}
				}
				else
				{
					PLAYER_ROTX = 0;
				}
			}
		}
	}

	my->yaw += PLAYER_ROTX * refreshRateDelta;
	while ( my->yaw >= PI * 2 )
	{
		my->yaw -= PI * 2;
	}
	while ( my->yaw < 0 )
	{
		my->yaw += PI * 2;
	}

	if ( smoothmouse )
	{
		PLAYER_ROTX *= pow(0.5, refreshRateDelta);
	}
	else
	{
		PLAYER_ROTX = 0;
	}

	// look up and down
	if ( !player.usingCommand()
		&& player.bControlEnabled && !gamePaused && my->isMobile() && !inputs.hasController(PLAYER_NUM) )
	{
		if ( !stats[PLAYER_NUM]->EFFECTS[EFF_CONFUSED] )
		{
			my->pitch += (Input::inputs[playernum].analog("Look Down") - Input::inputs[playernum].analog("Look Up")) * .05 * refreshRateDelta;
		}
		else
		{
			my->pitch += (Input::inputs[playernum].analog("Look Up") - Input::inputs[playernum].analog("Look Down")) * .05 * refreshRateDelta;
		}
	}
	if ( shootmode && !gamePaused )
	{
		if ( !stats[PLAYER_NUM]->EFFECTS[EFF_CONFUSED] )
		{
			if ( smoothmouse )
			{
				if ( my->isMobile() )
				{
					PLAYER_ROTY += mousey_relative * .006 * (mouse_speed / 128.f) * (reversemouse * 2 - 1);
				}
				PLAYER_ROTY = fmin(fmax(-0.35, PLAYER_ROTY), 0.35);
				PLAYER_ROTY *= pow(0.5, refreshRateDelta);
			}
			else
			{
				if ( my->isMobile() )
				{
					PLAYER_ROTY = std::min<float>(std::max<float>(-0.35f, 
						mousey_relative * .01f * (mouse_speed / 128.f) * (reversemouse * 2 - 1)), 0.35f);
				}
				else
				{
					PLAYER_ROTY = 0;
				}
			}
		}
		else
		{
			if ( smoothmouse )
			{
				if ( my->isMobile() )
				{
					PLAYER_ROTY -= mousey_relative * .006f * (mouse_speed / 128.f) * (reversemouse * 2 - 1);
				}
				PLAYER_ROTY = fmin(fmax(-0.35f, PLAYER_ROTY), 0.35f);
				PLAYER_ROTY *= pow(0.5, refreshRateDelta);
			}
			else
			{
				if ( my->isMobile() )
				{
					PLAYER_ROTY = std::min<float>(std::max<float>(-0.35f, 
						mousey_relative * .01f * (mouse_speed / 128.f) * (reversemouse * 2 - 1)), 0.35f);
				}
				else
				{
					PLAYER_ROTY = 0;
				}
			}
		}
	}
	my->pitch -= PLAYER_ROTY * refreshRateDelta;

	if ( my->pitch > PI / 3 )
	{
		my->pitch = PI / 3;
	}
	if ( my->pitch < -PI / 3 )
	{
		my->pitch = -PI / 3;
	}

	if ( smoothmouse )
	{
		PLAYER_ROTY *= pow(0.5, refreshRateDelta);
	}
	else
	{
		PLAYER_ROTY = 0;
	}

	if ( TimerExperiments::bUseTimerInterpolation )
	{
		while ( TimerExperiments::cameraCurrentState[PLAYER_NUM].yaw.position >= PI * 2 )
		{
			TimerExperiments::cameraCurrentState[PLAYER_NUM].yaw.position -= PI * 2;
		}
		while ( TimerExperiments::cameraCurrentState[PLAYER_NUM].yaw.position < 0 )
		{
			TimerExperiments::cameraCurrentState[PLAYER_NUM].yaw.position += PI * 2;
		}
		real_t diff = my->yaw - TimerExperiments::cameraCurrentState[PLAYER_NUM].yaw.position;
		if ( diff > PI )
		{
			diff -= 2 * PI;
		}
		else if ( diff < -PI )
		{
			diff += 2 * PI;
		}
		TimerExperiments::cameraCurrentState[PLAYER_NUM].yaw.velocity = diff * TimerExperiments::lerpFactor;
		while ( TimerExperiments::cameraCurrentState[PLAYER_NUM].pitch.position >= PI )
		{
			TimerExperiments::cameraCurrentState[PLAYER_NUM].pitch.position -= PI * 2;
		}
		while ( TimerExperiments::cameraCurrentState[PLAYER_NUM].pitch.position < -PI )
		{
			TimerExperiments::cameraCurrentState[PLAYER_NUM].pitch.position += PI * 2;
		}
		diff = my->pitch - TimerExperiments::cameraCurrentState[PLAYER_NUM].pitch.position;
		if ( diff >= PI )
		{
			diff -= 2 * PI;
		}
		else if ( diff < -PI )
		{
			diff += 2 * PI;
		}
		TimerExperiments::cameraCurrentState[PLAYER_NUM].pitch.velocity = diff * TimerExperiments::lerpFactor;
	}
}

void Player::PlayerMovement_t::handlePlayerCameraBobbing(bool useRefreshRateDelta)
{
	if ( !players[player.playernum]->entity )
	{
		return;
	}

	Entity* my = players[player.playernum]->entity;
	int playernum = player.playernum;

	bool swimming = isPlayerSwimming();

	double refreshRateDelta = 1.0;
	if ( useRefreshRateDelta && fps > 0.0 )
	{
		refreshRateDelta *= TICKS_PER_SECOND / (real_t)fpsLimit;
	}

	Input& input = Input::inputs[playernum];

	// camera bobbing
	if ( bobbing )
	{
		if ( swimming )
		{
			if ( PLAYER_BOBMODE )
			{
				PLAYER_BOBMOVE += .03 * refreshRateDelta;
			}
			else
			{
				PLAYER_BOBMOVE -= .03 * refreshRateDelta;
			}
		}
		else if ( !gamePaused
			&& ((!inputs.hasController(PLAYER_NUM) 
				&& ((input.binary("Move Forward") || input.binary("Move Backward"))
					|| (input.binary("Move Left") - input.binary("Move Right"))))
			|| (inputs.hasController(PLAYER_NUM) 
				&& (inputs.getController(PLAYER_NUM)->getLeftXPercentForPlayerMovement() 
					|| inputs.getController(PLAYER_NUM)->getLeftYPercentForPlayerMovement()))) )
		{
			if ( !player.usingCommand()
				&& player.bControlEnabled && !swimming )
			{
				if ( !(stats[PLAYER_NUM]->defending || stats[PLAYER_NUM]->sneaking == 0) )
				{
					if ( PLAYER_BOBMODE )
					{
						PLAYER_BOBMOVE += .0125 * refreshRateDelta;
					}
					else
					{
						PLAYER_BOBMOVE -= .0125 * refreshRateDelta;
					}
				}
				else
				{
					if ( PLAYER_BOBMODE )
					{
						PLAYER_BOBMOVE += .025 * refreshRateDelta;
					}
					else
					{
						PLAYER_BOBMOVE -= .025 * refreshRateDelta;
					}
				}
			}
			else if ( !swimming )
			{
				PLAYER_BOBMOVE = 0;
				PLAYER_BOB = 0;
				PLAYER_BOBMODE = 0;
			}
		}
		else if ( !swimming )
		{
			PLAYER_BOBMOVE = 0;
			PLAYER_BOB = 0;
			PLAYER_BOBMODE = 0;
		}

		if ( !player.usingCommand()
			&& player.bControlEnabled
			&& !gamePaused
			&& !swimming && !inputs.hasController(PLAYER_NUM) && (input.binary("Move Left") - input.binary("Move Right")) )
		{
			if ( (input.binary("Move Right") && !input.binary("Move Backward")) ||
				(input.binary("Move Left") && input.binary("Move Backward")) )
			{
				PLAYER_SIDEBOB += 0.01 * refreshRateDelta;
				real_t angle = PI / 32;
				if ( input.binary("Move Backward") )
				{
					angle = PI / 64;
				}
				if ( PLAYER_SIDEBOB > angle )
				{
					PLAYER_SIDEBOB = angle;
				}
			}
			else if ( (input.binary("Move Left") && !input.binary("Move Backward")) ||
				(input.binary("Move Right") && input.binary("Move Backward")) )
			{
				PLAYER_SIDEBOB -= 0.01 * refreshRateDelta;
				real_t angle = -PI / 32;
				if ( input.binary("Move Backward") )
				{
					angle = -PI / 64;
				}
				if ( PLAYER_SIDEBOB < angle )
				{
					PLAYER_SIDEBOB = angle;
				}
			}
		}
		else if ( !player.usingCommand()
			&& player.bControlEnabled
			&& !gamePaused
			&& !swimming && inputs.hasController(PLAYER_NUM) && abs(inputs.getController(PLAYER_NUM)->getLeftXPercentForPlayerMovement()) > 0.001 )
		{
			auto controller = inputs.getController(PLAYER_NUM);
			if ( (controller->getLeftXPercentForPlayerMovement() > 0.001 && controller->getLeftYPercentForPlayerMovement() >= 0.0)
				|| (controller->getLeftXPercentForPlayerMovement() < -0.001 && controller->getLeftYPercentForPlayerMovement() < -0.001 ) )
			{
				PLAYER_SIDEBOB += 0.01 * refreshRateDelta;
				real_t angle = PI / 32;
				if ( controller->getLeftYPercentForPlayerMovement() < 0.001 )
				{
					angle = PI / 64;
				}
				if ( PLAYER_SIDEBOB > angle )
				{
					PLAYER_SIDEBOB = angle;
				}
			}
			else if ( (controller->getLeftXPercentForPlayerMovement() < -0.001 && controller->getLeftYPercentForPlayerMovement() >= 0.0)
				|| (controller->getLeftXPercentForPlayerMovement() > 0.001 && controller->getLeftYPercentForPlayerMovement() < -0.001) )
			{
				PLAYER_SIDEBOB -= 0.01 * refreshRateDelta;
				real_t angle = -PI / 32;
				if ( controller->getLeftYPercentForPlayerMovement() < 0.001 )
				{
					angle = -PI / 64;
				}
				if ( PLAYER_SIDEBOB < angle )
				{
					PLAYER_SIDEBOB = angle;
				}
			}
		}
		else
		{
			if ( PLAYER_SIDEBOB > 0.001 )
			{
				PLAYER_SIDEBOB -= 0.02 * refreshRateDelta;
				if ( PLAYER_SIDEBOB < 0.f )
				{
					PLAYER_SIDEBOB = 0.f;
				}
			}
			else
			{
				PLAYER_SIDEBOB += 0.02 * refreshRateDelta;
				if ( PLAYER_SIDEBOB > 0.f )
				{
					PLAYER_SIDEBOB = 0.f;
				}
			}
		}

		if ( !swimming && !(stats[PLAYER_NUM]->defending || stats[PLAYER_NUM]->sneaking == 0) )
		{
			if ( PLAYER_BOBMOVE > .1 )
			{
				PLAYER_BOBMOVE = .1;
				PLAYER_BOBMODE = 0;
			}
			else if ( PLAYER_BOBMOVE < -.1 )
			{
				PLAYER_BOBMOVE = -.1;
				PLAYER_BOBMODE = 1;
			}
		}
		else if ( swimming )
		{
			if ( PLAYER_BOBMOVE > .3 )
			{
				PLAYER_BOBMOVE = .3;
				PLAYER_BOBMODE = 0;
			}
			else if ( PLAYER_BOBMOVE < -.3 )
			{
				PLAYER_BOBMOVE = -.3;
				PLAYER_BOBMODE = 1;
			}
		}
		else
		{
			if ( PLAYER_BOBMOVE > .1 )
			{
				PLAYER_BOBMOVE = .1;
				PLAYER_BOBMODE = 0;
			}
			else if ( PLAYER_BOBMOVE < -.1 )
			{
				PLAYER_BOBMOVE = -.1;
				PLAYER_BOBMODE = 1;
			}
		}
		PLAYER_BOB += PLAYER_BOBMOVE * refreshRateDelta;
		if ( static_cast<Monster>(my->effectShapeshift) == SPIDER || static_cast<Monster>(my->effectShapeshift) == RAT )
		{
			PLAYER_BOB = std::min(static_cast<real_t>(1), PLAYER_BOB);
		}
	}
	else
	{
		PLAYER_BOBMOVE = 0;
		PLAYER_BOB = 0;
		PLAYER_BOBMODE = 0;
	}
}

real_t Player::PlayerMovement_t::getMaximumSpeed()
{
	real_t maxSpeed = 12.5;
	if ( gameplayCustomManager.inUse() )
	{
		maxSpeed = gameplayCustomManager.playerSpeedMax;
	}
	return maxSpeed;
}

int Player::PlayerMovement_t::getCharacterEquippedWeight()
{
	int weight = 0;
	for ( node_t* node = stats[player.playernum]->inventory.first; node != NULL; node = node->next )
	{
		Item* item = (Item*)node->element;
		if ( item != NULL && player.paperDoll.isItemOnDoll(*item) )
		{
			if ( item->type >= 0 && item->type < NUMITEMS )
			{
				weight += item->getWeight();
			}
		}
	}
	//weight += stats[player.playernum]->GOLD / 100;
	return weight;
}

int Player::PlayerMovement_t::getCharacterWeight()
{
	int weight = 0;
	for ( node_t* node = stats[player.playernum]->inventory.first; node != NULL; node = node->next )
	{
		Item* item = (Item*)node->element;
		if ( item != NULL )
		{
			if ( item->type >= 0 && item->type < NUMITEMS )
			{
				weight += item->getWeight();
			}
		}
	}
	weight += stats[player.playernum]->GOLD / 100;
	return weight;
}

int Player::PlayerMovement_t::getCharacterModifiedWeight(int* customWeight)
{
	int weight = getCharacterWeight();
	if ( customWeight )
	{
		weight = *customWeight;
	}

	if ( gameplayCustomManager.inUse() )
	{
		weight = weight * (gameplayCustomManager.playerWeightPercent / 100.f);
	}
	if ( stats[player.playernum]->EFFECTS[EFF_FAST] && !stats[player.playernum]->EFFECTS[EFF_SLOW] )
	{
		weight = weight * 0.5;
	}
	return weight;
}

real_t Player::PlayerMovement_t::getWeightRatio(int weight, Sint32 STR)
{
	real_t weightratio_zero = (1000 - weight) / (double)(1000);
	weightratio_zero = fmin(fmax(0, weightratio_zero), 1);
	real_t curveExponentFactor = 2.0;
	int curveYoffset = 1;
	weightratio_zero = -pow(1.0 - weightratio_zero, curveExponentFactor) + curveYoffset;

	real_t weightratio = (1000 + STR * 100 - weight) / (double)(1000 + STR * 100);
	weightratio = fmin(fmax(0, weightratio), 1);

	if ( weight <= 1000 )
	{
		weightratio = std::max(weightratio, weightratio_zero);
	}

	return weightratio;
}

real_t Player::PlayerMovement_t::getSpeedFactor(real_t weightratio, Sint32 DEX)
{
	real_t slowSpeedPenalty = 0.0;
	real_t maxSpeed = getMaximumSpeed();
	if ( !stats[player.playernum]->EFFECTS[EFF_FAST] && stats[player.playernum]->EFFECTS[EFF_SLOW] )
	{
		DEX = std::min(DEX - 3, -2);
		slowSpeedPenalty = 2.0;
	}
	else if ( stats[player.playernum]->EFFECTS[EFF_FAST] && !stats[player.playernum]->EFFECTS[EFF_SLOW] )
	{
		maxSpeed += 1.0;
	}
	real_t speedFactor = std::min((((DEX) * .4) + 8.5 - slowSpeedPenalty) * weightratio, maxSpeed);
	/*if ( DEX <= 5 )
	{
		speedFactor = std::min(((DEX * 0.5) + 8.5) * weightratio, maxSpeed);
	}*/

	if ( !stats[player.playernum]->EFFECTS[EFF_DASH] && stats[player.playernum]->EFFECTS[EFF_KNOCKBACK] )
	{
		speedFactor = std::min(speedFactor, 5.0);
	}


	for ( node_t* node = stats[player.playernum]->inventory.first; node != NULL; node = node->next )
	{
		Item* item = (Item*)node->element;
		if ( item != NULL )
		{
			if ( item->type == TOOL_PLAYER_LOOT_BAG )
			{
				if ( items[item->type].hasAttribute("EFF_WEIGHT_BURDEN") )
				{
					speedFactor *= (items[item->type].attributes["EFF_WEIGHT_BURDEN"]) / 100.0;
				}
			}
		}
	}
	return speedFactor;
}

real_t Player::PlayerMovement_t::getCurrentMovementSpeed()
{
	if ( players[player.playernum] && players[player.playernum]->entity )
	{
		return sqrt(pow(players[player.playernum]->entity->vel_x, 2) + pow(players[player.playernum]->entity->vel_y, 2));
	}
	return 0.0;
}

void Player::PlayerMovement_t::handlePlayerMovement(bool useRefreshRateDelta)
{
	if ( !players[player.playernum]->entity )
	{
		return;
	}

	Entity* my = players[player.playernum]->entity;

	Input& input = Input::inputs[player.playernum];

	double refreshRateDelta = 1.0;
	if ( useRefreshRateDelta && fps > 0.0 )
	{
		refreshRateDelta *= TICKS_PER_SECOND / (real_t)fpsLimit;
	}

	// calculate weight
	int weight = getCharacterModifiedWeight();
	real_t weightratio = getWeightRatio(weight, statGetSTR(stats[PLAYER_NUM], players[PLAYER_NUM]->entity));

	// calculate movement forces

	bool allowMovement = my->isMobile();
	bool pacified = stats[PLAYER_NUM]->EFFECTS[EFF_PACIFY];
	if ( !allowMovement && pacified )
	{
		if ( !stats[PLAYER_NUM]->EFFECTS[EFF_PARALYZED] && !stats[PLAYER_NUM]->EFFECTS[EFF_STUNNED]
			&& !stats[PLAYER_NUM]->EFFECTS[EFF_ASLEEP] )
		{
			allowMovement = true;
		}
	}

	if ( ((!player.usingCommand() && player.bControlEnabled && !gamePaused) || pacified) 
		&& allowMovement )
	{
		//x_force and y_force represent the amount of percentage pushed on that respective axis. Given a keyboard, it's binary; either you're pushing "move left" or you aren't. On an analog stick, it can range from whatever value to whatever.
		float x_force = 0;
		float y_force = 0;

		if ( pacified )
		{
			x_force = 0.f;
			y_force = -0.1;
			if ( stats[PLAYER_NUM]->EFFECTS[EFF_CONFUSED] )
			{
				y_force *= -1;
			}
		}
		else
		{
			double backpedalMultiplier = 0.25;
			if ( stats[PLAYER_NUM]->EFFECTS[EFF_DASH] )
			{
				backpedalMultiplier = 1.25;
			}

			if ( !inputs.hasController(PLAYER_NUM) )
			{
				if ( !stats[PLAYER_NUM]->EFFECTS[EFF_CONFUSED] )
				{
					//Normal controls.
					x_force = (input.binary("Move Right") - input.binary("Move Left"));
					y_force = input.binary("Move Forward") - (double)input.binary("Move Backward") * backpedalMultiplier;
					if ( noclip )
					{
						if ( keystatus[SDLK_LSHIFT] )
						{
							x_force = x_force * 0.5;
							y_force = y_force * 0.5;
						}
					}
				}
				else
				{
					//Confused controls.
					x_force = input.binary("Move Left") - input.binary("Move Right");
					y_force = input.binary("Move Backward") - (double)input.binary("Move Forward") * backpedalMultiplier;
				}
			}

			if ( inputs.hasController(PLAYER_NUM) /*&& !input.binary("Move Left") && !input.binary("Move Right")*/ )
			{
				x_force = inputs.getController(PLAYER_NUM)->getLeftXPercentForPlayerMovement();

				if ( stats[PLAYER_NUM]->EFFECTS[EFF_CONFUSED] )
				{
					x_force *= -1;
				}
			}
			if ( inputs.hasController(PLAYER_NUM) /*&& !input.binary("Move Forward") && !input.binary("Move Backward")*/ )
			{
				y_force = inputs.getController(PLAYER_NUM)->getLeftYPercentForPlayerMovement();

				if ( stats[PLAYER_NUM]->EFFECTS[EFF_CONFUSED] )
				{
					y_force *= -1;
				}

				if ( y_force < 0 )
				{
					y_force *= backpedalMultiplier;    //Move backwards more slowly.
				}
			}
		}

		real_t speedFactor = getSpeedFactor(weightratio, statGetDEX(stats[PLAYER_NUM], players[PLAYER_NUM]->entity));
		static ConsoleVariable<bool> cvar_debugspeedfactor("/player_showspeedfactor", false);
		if ( *cvar_debugspeedfactor && ticks % 50 == 0 )
		{
			Sint32 STR = statGetSTR(stats[PLAYER_NUM], players[PLAYER_NUM]->entity);
			real_t weightratioOld = (1000 + STR * 100 - weight) / (double)(1000 + STR * 100);
			weightratioOld = fmin(fmax(0, weightratioOld), 1);
			real_t maxSpeed = getMaximumSpeed();
			Sint32 DEX = statGetDEX(stats[PLAYER_NUM], players[PLAYER_NUM]->entity);
			real_t speedFactorOld = std::min((DEX * 0.1 + 15.5) * weightratioOld, maxSpeed);
			if ( DEX <= 5 )
			{
				speedFactorOld = std::min((DEX + 10) * weightratioOld, maxSpeed);
			}
			else if ( DEX <= 15 )
			{
				speedFactorOld = std::min((DEX * 0.2 + 14) * weightratioOld, maxSpeed);
			}
			messagePlayer(0, MESSAGE_DEBUG, "New: %.3f | Old: %.3f | (%+.3f)", speedFactor, speedFactorOld, 100.0 * ((speedFactor / speedFactorOld) - 1.0));
		}
		if ( stats[PLAYER_NUM]->EFFECTS[EFF_DASH] )
		{
			PLAYER_VELX += my->monsterKnockbackVelocity * cos(my->monsterKnockbackTangentDir) * refreshRateDelta;
			PLAYER_VELY += my->monsterKnockbackVelocity * sin(my->monsterKnockbackTangentDir) * refreshRateDelta;
			my->monsterKnockbackVelocity *= pow(0.95, refreshRateDelta);
		}
		else if ( stats[PLAYER_NUM]->EFFECTS[EFF_KNOCKBACK] )
		{
			PLAYER_VELX += my->monsterKnockbackVelocity * cos(my->monsterKnockbackTangentDir) * refreshRateDelta;
			PLAYER_VELY += my->monsterKnockbackVelocity * sin(my->monsterKnockbackTangentDir) * refreshRateDelta;
			my->monsterKnockbackVelocity *= pow(0.95, refreshRateDelta);
		}
		else
		{
			my->monsterKnockbackVelocity = 0.f;
			my->monsterKnockbackTangentDir = 0.f;
		}

		if ( fabs(my->playerStrafeVelocity) > 0.1 )
		{
			PLAYER_VELX += my->playerStrafeVelocity * cos(my->playerStrafeDir) * refreshRateDelta;
			PLAYER_VELY += my->playerStrafeVelocity * sin(my->playerStrafeDir) * refreshRateDelta;
			my->playerStrafeVelocity *= pow(0.95, refreshRateDelta);
		}
		else
		{
			my->playerStrafeDir = 0.0f;
			my->playerStrafeVelocity = 0.0f;
		}

		speedFactor *= refreshRateDelta;
		PLAYER_VELX += y_force * cos(my->yaw) * .045 * speedFactor / (1 + (stats[PLAYER_NUM]->defending || stats[PLAYER_NUM]->sneaking == 1));
		PLAYER_VELY += y_force * sin(my->yaw) * .045 * speedFactor / (1 + (stats[PLAYER_NUM]->defending || stats[PLAYER_NUM]->sneaking == 1));
		PLAYER_VELX += x_force * cos(my->yaw + PI / 2) * .0225 * speedFactor / (1 + (stats[PLAYER_NUM]->defending || stats[PLAYER_NUM]->sneaking == 1));
		PLAYER_VELY += x_force * sin(my->yaw + PI / 2) * .0225 * speedFactor / (1 + (stats[PLAYER_NUM]->defending || stats[PLAYER_NUM]->sneaking == 1));

	}
	PLAYER_VELX *= pow(0.75, refreshRateDelta);
	PLAYER_VELY *= pow(0.75, refreshRateDelta);

	//if ( keystatus[SDLK_G] )
	//{
		//messagePlayer(0, MESSAGE_DEBUG, "X: %5.5f, Y: %5.5f, Total: %5.5f", PLAYER_VELX, PLAYER_VELY, sqrt(pow(PLAYER_VELX, 2) + pow(PLAYER_VELY, 2)));
		//messagePlayer(0, MESSAGE_DEBUG, "Vel: %5.5f", getCurrentMovementSpeed());
	//}

	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		if ( players[i] && players[i]->entity )
		{
			if ( players[i]->entity == my )
			{
				continue;
			}
			if ( entityInsideEntity(my, players[i]->entity) )
			{
				double tangent = atan2(my->y - players[i]->entity->y, my->x - players[i]->entity->x);
				PLAYER_VELX += cos(tangent) * 0.075 * refreshRateDelta;
				PLAYER_VELY += sin(tangent) * 0.075 * refreshRateDelta;
			}
		}
	}

	// swimming slows you down
	bool amuletwaterbreathing = false;
	if ( stats[PLAYER_NUM]->amulet != NULL )
	{
		if ( stats[PLAYER_NUM]->amulet->type == AMULET_WATERBREATHING )
		{
			amuletwaterbreathing = true;
		}
	}
	bool swimming = isPlayerSwimming();
	if ( swimming && !amuletwaterbreathing )
	{
		PLAYER_VELX *= (((stats[PLAYER_NUM]->PROFICIENCIES[PRO_SWIMMING] / 100.f) * 50.f) + 50) / 100.f;
		PLAYER_VELY *= (((stats[PLAYER_NUM]->PROFICIENCIES[PRO_SWIMMING] / 100.f) * 50.f) + 50) / 100.f;

		if ( stats[PLAYER_NUM]->type == SKELETON )
		{
			if ( !swimDebuffMessageHasPlayed )
			{
				messagePlayer(PLAYER_NUM, MESSAGE_HINT, Language::get(3182));
				swimDebuffMessageHasPlayed = true;
			}
			// no swim good
			PLAYER_VELX *= 0.5;
			PLAYER_VELY *= 0.5;
		}
	}
}

void Player::PlayerMovement_t::handlePlayerCameraPosition(bool useRefreshRateDelta)
{
	if ( !players[player.playernum]->entity )
	{
		return;
	}

	Entity* my = players[player.playernum]->entity;

	int playerRace = my->getMonsterTypeFromSprite();
	bool swimming = isPlayerSwimming();

	double refreshRateDelta = 1.0;
	if ( useRefreshRateDelta && fps > 0.0 )
	{
		refreshRateDelta *= TICKS_PER_SECOND / (real_t)fpsLimit;
	}

	// camera
	if ( !PLAYER_DEBUGCAM && stats[PLAYER_NUM] && stats[PLAYER_NUM]->HP > 0 )
	{
		if ( !TimerExperiments::bUseTimerInterpolation )
		{
			cameras[PLAYER_NUM].x = my->x / 16.0;
			cameras[PLAYER_NUM].y = my->y / 16.0;
		}
		else
		{
			TimerExperiments::cameraCurrentState[PLAYER_NUM].x.velocity = TimerExperiments::lerpFactor * (my->x / 16.0 - TimerExperiments::cameraCurrentState[PLAYER_NUM].x.position);
			TimerExperiments::cameraCurrentState[PLAYER_NUM].y.velocity = TimerExperiments::lerpFactor * (my->y / 16.0 - TimerExperiments::cameraCurrentState[PLAYER_NUM].y.position);
		}

		real_t cameraSetpointZ = (my->z * 2) - 2.5 + (swimming ? 1 : 0);
		if ( swimming && (playerRace == RAT || playerRace == SPIDER) )
		{
			cameraSetpointZ -= 0.5; // float a little higher.
		}

		if ( playerRace == CREATURE_IMP && my->z == -4.5 )
		{
			cameraSetpointZ += 1;
		}
		else if ( playerRace == TROLL && my->z <= -1.5 )
		{
			cameraSetpointZ -= 2;
		}

		real_t diff = abs(PLAYER_CAMERAZ_ACCEL - cameraSetpointZ);
		if ( diff > 0.01 )
		{
			real_t rateChange = std::min(2.0, std::max(0.3, diff * 0.5)) * refreshRateDelta;

			if ( cameraSetpointZ >= 0.f )
			{
				PLAYER_CAMERAZ_ACCEL += rateChange;
				PLAYER_CAMERAZ_ACCEL = std::min(cameraSetpointZ, PLAYER_CAMERAZ_ACCEL);
			}
			else if ( cameraSetpointZ < 0.f )
			{
				PLAYER_CAMERAZ_ACCEL += -rateChange;
				PLAYER_CAMERAZ_ACCEL = std::max(cameraSetpointZ, PLAYER_CAMERAZ_ACCEL);
			}

			if ( !TimerExperiments::bUseTimerInterpolation )
			{
				cameras[PLAYER_NUM].z = PLAYER_CAMERAZ_ACCEL;
			}
			else
			{
				TimerExperiments::cameraCurrentState[PLAYER_NUM].z.velocity =
					((PLAYER_CAMERAZ_ACCEL) - TimerExperiments::cameraCurrentState[PLAYER_NUM].z.position)
					* TimerExperiments::lerpFactor;
			}

			// check updated value.
			if ( abs(PLAYER_CAMERAZ_ACCEL - cameraSetpointZ) <= 0.01 )
			{
				PLAYER_BOBMOVE = 0;
				PLAYER_BOB = 0;
				PLAYER_BOBMODE = 0;
			}
		}
		else
		{
			PLAYER_CAMERAZ_ACCEL = cameraSetpointZ;
			if ( !TimerExperiments::bUseTimerInterpolation )
			{
				cameras[PLAYER_NUM].z = PLAYER_CAMERAZ_ACCEL + PLAYER_BOB;
			}
			else
			{
				TimerExperiments::cameraCurrentState[PLAYER_NUM].z.velocity = 
					((PLAYER_CAMERAZ_ACCEL + PLAYER_BOB) - TimerExperiments::cameraCurrentState[PLAYER_NUM].z.position) 
					* TimerExperiments::lerpFactor;
			}
		}

		//messagePlayer(0, "Z: %.2f | %.2f | %.2f", my->z, PLAYER_CAMERAZ_ACCEL, cameraSetpointZ);

		if ( !TimerExperiments::bUseTimerInterpolation )
		{
			cameras[PLAYER_NUM].ang = my->yaw;
			cameras[PLAYER_NUM].vang = my->pitch;
		}
	}
}

void statueCycleItem(Item& item, bool dirForward)
{
	int cat = items[item.type].item_slot;
	item.appearance = local_rng.rand();
	if ( dirForward )
	{
		for ( int i = item.type + 1; i <= NUMITEMS && i != item.type; ++i )
		{
			if ( i == NUMITEMS )
			{
				i = 0;
			}
			if ( items[i].item_slot == cat )
			{
				item.type = ItemType(i);
				break;
			}
		}
	}
	else
	{
		for ( int i = item.type - 1; i < NUMITEMS && i != item.type; --i )
		{
			if ( i < 0 )
			{
				i = NUMITEMS - 1;
			}
			if ( items[i].item_slot == cat )
			{
				item.type = ItemType(i);
				break;
			}
		}
	}
}

void doStatueEditor(int player)
{
	if ( !StatueManager.activeEditing ) { return; }
	if ( player != clientnum ) { return; }

	Sint32 mouseX = inputs.getMouse(player, Inputs::OX);
	Sint32 mouseY = inputs.getMouse(player, Inputs::OY);
	bool shootmode = players[player]->shootmode;

	if ( ticks % 5 == 0 )
	{
		Entity* underMouse = nullptr;
		Uint32 uidnum = 0;
		if ( !shootmode )
		{
			uidnum = GO_GetPixelU32(mouseX, yres - mouseY, cameras[player]);
			if ( uidnum > 0 )
			{
				underMouse = uidToEntity(uidnum);
			}
		}
		else
		{
			uidnum = GO_GetPixelU32(cameras[player].winw / 2, yres - cameras[player].winh / 2, cameras[player]);
			if ( uidnum > 0 )
			{
				underMouse = uidToEntity(uidnum);
			}
		}
		if ( underMouse )
		{
			underMouse->highlightForUI = 1.0;
			if ( underMouse->behavior == &actPlayerLimb )
			{
				underMouse = players[underMouse->skill[2]]->entity;
			}
			if ( underMouse->behavior == &actPlayer )
			{
				StatueManager.editingPlayerUid = underMouse->getUID();
				StatueManager.lastEntityUnderMouse = uidnum;
			}
		}
	}


	if ( Entity* playerEntity = uidToEntity(StatueManager.editingPlayerUid) )
	{
		playerEntity->highlightForUI = 0.0;
		if ( StatueManager.drawGreyscale )
		{
			playerEntity->grayscaleGLRender = 1.0;
		}
		else
		{
			playerEntity->grayscaleGLRender = 0.0;
		}
		for ( auto& bodypart : playerEntity->bodyparts )
		{
			bodypart->highlightForUI = 0.0;
			if ( StatueManager.drawGreyscale )
			{
				bodypart->grayscaleGLRender = 1.0;
			}
			else
			{
				bodypart->grayscaleGLRender = 0.0;
			}
		}
	}

	if ( !players[player]->usingCommand() )
	{
		if ( Entity* limb = uidToEntity(StatueManager.lastEntityUnderMouse) )
		{
			limb->highlightForUI = 1.0;
			if ( keystatus[SDLK_o] )
			{
				keystatus[SDLK_o] = 0;
				Input::inputs[player].refresh();
				limb->pitch += PI / 32;
			}
			if ( keystatus[SDLK_p] )
			{
				keystatus[SDLK_p] = 0;
				Input::inputs[player].refresh();
				limb->pitch -= PI / 32;
			}
			if ( keystatus[SDLK_k] )
			{
				keystatus[SDLK_k] = 0;
				Input::inputs[player].refresh();
				limb->roll += PI / 32;
			}
			if ( keystatus[SDLK_l] )
			{
				keystatus[SDLK_l] = 0;
				Input::inputs[player].refresh();
				limb->roll -= PI / 32;
			}
			if ( keystatus[SDLK_COMMA] )
			{
				keystatus[SDLK_COMMA] = 0;
				Input::inputs[player].refresh();
				limb->yaw += PI / 32;
			}
			if ( keystatus[SDLK_PERIOD] )
			{
				keystatus[SDLK_PERIOD] = 0;
				Input::inputs[player].refresh();
				limb->yaw -= PI / 32;
			}
		}

		if ( Entity* playerEntity = uidToEntity(StatueManager.editingPlayerUid) )
		{
			Stat* stats = playerEntity->getStats();

			if ( keystatus[SDLK_LEFTBRACKET] )
			{
				keystatus[SDLK_LEFTBRACKET] = 0;
				Input::inputs[player].refresh();
				StatueManager.statueEditorHeightOffset -= .25;
			}
			if ( keystatus[SDLK_RIGHTBRACKET] )
			{
				keystatus[SDLK_RIGHTBRACKET] = 0;
				Input::inputs[player].refresh();
				StatueManager.statueEditorHeightOffset += .25;
			}

			if ( keystatus[SDLK_F1] )
			{
				keystatus[SDLK_F1] = 0;

				++stats->playerRace;
				if ( playerEntity->getMonsterFromPlayerRace(stats->playerRace) == HUMAN && stats->playerRace > 0 )
				{
					stats->playerRace = RACE_HUMAN;
				}
				if ( playerEntity->getMonsterFromPlayerRace(stats->playerRace) != HUMAN )
				{
					stats->appearance = 0;
				}
			}
			if ( keystatus[SDLK_F2] )
			{
				keystatus[SDLK_F2] = 0;

				++stats->appearance;
				if ( stats->appearance >= NUMAPPEARANCES )
				{
					stats->appearance = 0;
				}
			}
			if ( keystatus[SDLK_F3] )
			{
				keystatus[SDLK_F3] = 0;
				if ( stats->sex == MALE )
				{
					stats->sex = FEMALE;
				}
				else
				{
					stats->sex = MALE;
				}
			}
			if ( keystatus[SDLK_F4] )
			{
				keystatus[SDLK_F4] = 0;
				StatueManager.drawGreyscale = !StatueManager.drawGreyscale;
			}

			if ( keystatus[SDLK_1] )
			{
				Input::inputs[player].refresh();
				keystatus[SDLK_1] = 0;
				if ( keystatus[SDLK_LALT] || keystatus[SDLK_RALT] )
				{
					if ( stats->helmet )
					{
						list_RemoveNode(stats->helmet->node);
						stats->helmet = nullptr;
					}
				}
				else
				{
					if ( !stats->helmet )
					{
						stats->helmet = newItem(LEATHER_HELM, EXCELLENT, 0, 1, local_rng.rand(), true, &stats->inventory);
					}
					if ( keystatus[SDLK_LSHIFT] || keystatus[SDLK_RSHIFT] )
					{
						statueCycleItem(*stats->helmet, false);
					}
					else
					{
						statueCycleItem(*stats->helmet, true);
					}
				}
			}
			if ( keystatus[SDLK_2] )
			{
				Input::inputs[player].refresh();
				keystatus[SDLK_2] = 0;
				if ( keystatus[SDLK_LALT] || keystatus[SDLK_RALT] )
				{
					if ( stats->breastplate )
					{
						list_RemoveNode(stats->breastplate->node);
						stats->breastplate = nullptr;
					}
				}
				else
				{
					if ( !stats->breastplate )
					{
						stats->breastplate = newItem(LEATHER_BREASTPIECE, EXCELLENT, 0, 1, local_rng.rand(), true, &stats->inventory);
					}
					if ( keystatus[SDLK_LSHIFT] || keystatus[SDLK_RSHIFT] )
					{
						statueCycleItem(*stats->breastplate, false);
					}
					else
					{
						statueCycleItem(*stats->breastplate, true);
					}
				}
			}
			if ( keystatus[SDLK_3] )
			{
				Input::inputs[player].refresh();
				keystatus[SDLK_3] = 0;
				if ( keystatus[SDLK_LALT] || keystatus[SDLK_RALT] )
				{
					if ( stats->gloves )
					{
						list_RemoveNode(stats->gloves->node);
						stats->gloves = nullptr;
					}
				}
				else
				{
					if ( !stats->gloves )
					{
						stats->gloves = newItem(GLOVES, EXCELLENT, 0, 1, local_rng.rand(), true, &stats->inventory);
					}
					if ( keystatus[SDLK_LSHIFT] || keystatus[SDLK_RSHIFT] )
					{
						statueCycleItem(*stats->gloves, false);
					}
					else
					{
						statueCycleItem(*stats->gloves, true);
					}
				}
			}
			if ( keystatus[SDLK_4] )
			{
				Input::inputs[player].refresh();
				keystatus[SDLK_4] = 0;
				if ( keystatus[SDLK_LALT] || keystatus[SDLK_RALT] )
				{
					if ( stats->shoes )
					{
						list_RemoveNode(stats->shoes->node);
						stats->shoes = nullptr;
					}
				}
				else
				{
					if ( !stats->shoes )
					{
						stats->shoes = newItem(LEATHER_BOOTS, EXCELLENT, 0, 1, local_rng.rand(), true, &stats->inventory);
					}
					if ( keystatus[SDLK_LSHIFT] || keystatus[SDLK_RSHIFT] )
					{
						statueCycleItem(*stats->shoes, false);
					}
					else
					{
						statueCycleItem(*stats->shoes, true);
					}
				}
			}
			if ( keystatus[SDLK_5] )
			{
				Input::inputs[player].refresh();
				keystatus[SDLK_5] = 0;
				if ( keystatus[SDLK_LALT] || keystatus[SDLK_RALT] )
				{
					if ( stats->weapon )
					{
						list_RemoveNode(stats->weapon->node);
						stats->weapon = nullptr;
					}
				}
				else
				{
					if ( !stats->weapon )
					{
						stats->weapon = newItem(BRONZE_SWORD, EXCELLENT, 0, 1, local_rng.rand(), true, &stats->inventory);
					}
					if ( keystatus[SDLK_LSHIFT] || keystatus[SDLK_RSHIFT] )
					{
						statueCycleItem(*stats->weapon, false);
					}
					else
					{
						statueCycleItem(*stats->weapon, true);
					}
				}
			}
			if ( keystatus[SDLK_6] )
			{
				Input::inputs[player].refresh();
				keystatus[SDLK_6] = 0;
				if ( keystatus[SDLK_LALT] || keystatus[SDLK_RALT] )
				{
					if ( stats->shield )
					{
						list_RemoveNode(stats->shield->node);
						stats->shield = nullptr;
					}
				}
				else
				{
					if ( !stats->shield )
					{
						stats->shield = newItem(WOODEN_SHIELD, EXCELLENT, 0, 1, local_rng.rand(), true, &stats->inventory);
					}
					if ( keystatus[SDLK_LSHIFT] || keystatus[SDLK_RSHIFT] )
					{
						statueCycleItem(*stats->shield, false);
					}
					else
					{
						statueCycleItem(*stats->shield, true);
					}
				}
			}
			if ( keystatus[SDLK_7] )
			{
				Input::inputs[player].refresh();
				keystatus[SDLK_7] = 0;
				if ( keystatus[SDLK_LALT] || keystatus[SDLK_RALT] )
				{
					if ( stats->mask )
					{
						list_RemoveNode(stats->mask->node);
						stats->mask = nullptr;
					}
				}
				else
				{
					if ( !stats->mask )
					{
						stats->mask = newItem(TOOL_GLASSES, EXCELLENT, 0, 1, local_rng.rand(), true, &stats->inventory);
					}
					if ( keystatus[SDLK_LSHIFT] || keystatus[SDLK_RSHIFT] )
					{
						statueCycleItem(*stats->mask, false);
					}
					else
					{
						statueCycleItem(*stats->mask, true);
					}
				}
			}
			if ( keystatus[SDLK_8] )
			{
				Input::inputs[player].refresh();
				keystatus[SDLK_8] = 0;
				if ( keystatus[SDLK_LALT] || keystatus[SDLK_RALT] )
				{
					if ( stats->cloak )
					{
						list_RemoveNode(stats->cloak->node);
						stats->cloak = nullptr;
					}
				}
				else
				{
					if ( !stats->cloak )
					{
						stats->cloak = newItem(CLOAK, EXCELLENT, 0, 1, local_rng.rand(), true, &stats->inventory);
					}
					if ( keystatus[SDLK_LSHIFT] || keystatus[SDLK_RSHIFT] )
					{
						statueCycleItem(*stats->cloak, false);
					}
					else
					{
						statueCycleItem(*stats->cloak, true);
					}
				}
			}
		}
	}
}

int playerHeadSprite(Monster race, sex_t sex, int appearance, int frame) {
    if (race == HUMAN) {
        if (appearance < 5) {
            return 113 + 12 * sex + appearance;
        }
        else if (appearance == 5) {
            return 332 + sex;
        }
        else if (appearance >= 6 && appearance < 12) {
            return 341 + sex * 13 + appearance - 6;
        }
        else if (appearance >= 12) {
            return 367 + sex * 13 + appearance - 12;
        }
        else {
            return 113; // default human head
        }
    }
    else if (race == SKELETON) {
        return sex == FEMALE ? 1049 : 686;
    }
    else if (race == RAT) {
        return frame ?
            (frame < 5 ? 1043 :
            (frame < 9 ? 1044 : 814)) :
            814;
    }
    else if (race == TROLL) {
        return 817;
    }
    else if (race == SPIDER) {
        return arachnophobia_filter ? 1001 : 823;
    }
    else if (race == CREATURE_IMP) {
        return 827;
    }
    else if (race == GOBLIN) {
        return sex == FEMALE ? 752 : 694;
    }
    else if (race == INCUBUS) {
        return 702;
    }
    else if (race == SUCCUBUS) {
        return 710;
    }
    else if (race == VAMPIRE) {
        return sex == FEMALE ? 756 : 718;
    }
    else if (race == INSECTOID) {
        return sex == FEMALE ? 760 : 726;
    }
    else if (race == GOATMAN) {
        return sex == FEMALE ? 768 : 734;
    }
    else if (race == AUTOMATON) {
        return sex == FEMALE ? 770 : 742;
    }
    else {
        return 481; // shadow head due to unknown creature
    }
}

void actPlayer(Entity* my)
{
	if (!my)
	{
		return;
	}
	if ( logCheckObstacle )
	{
		if ( ticks % 50 == 0 )
		{
			messagePlayer(0, MESSAGE_DEBUG, "checkObstacle() calls/sec: %d", logCheckObstacleCount);
			logCheckObstacleCount = 0;
		}
	}
	if ( spamming && my->ticks % 2 == 0 )
	{
		for (int i = 0; i < 1; ++i)
		{
			char s[64] = "";
			char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

			for ( int j = 0; j < 63; ++j ) {
				s[j] = alphanum[local_rng.rand() % (sizeof(alphanum) - 1)];
			}
			Uint32 totalSize = 0;
			for ( size_t c = 0; c < HASH_SIZE; ++c ) {
				totalSize += list_Size(&ttfTextHash[c]);
			}
			messagePlayer(0, MESSAGE_DEBUG, "IMGREF: %d, total size: %d", imgref, totalSize);
			s[63] = '\0';
			messagePlayer(0, MESSAGE_DEBUG, "%s", s);
			//messagePlayer(0, "Lorem ipsum dolor sit amet, dico accusam reprehendunt ne mea, ea est illum tincidunt voluptatibus. Ne labore voluptua eos, nostro fierent mnesarchum an mei, cu mea dolor verear epicuri. Est id iriure principes, unum cotidieque qui te. An sit tractatos complectitur.");
		}
	}
	if ( autoLimbReload && ticks % 20 == 0 && (PLAYER_NUM == clientnum) )
	{
		consoleCommand("/reloadlimbs");
	}

	Entity* entity;
	Entity* entity2 = nullptr;
	Entity* rightbody = nullptr;
	Entity* weaponarm = nullptr;
	Entity* shieldarm = nullptr;
	Entity* additionalLimb = nullptr;
	Entity* torso = nullptr;
	node_t* node;
	Item* item;
	int i, bodypart;
	double dist = 0;
	bool wearingring = false;
	bool levitating = false;
	bool isHumanoid = true;
	bool showEquipment = true;
	if ( PLAYER_NUM < 0 || PLAYER_NUM >= MAXPLAYERS )
	{
		return;
	}

	Monster playerRace = HUMAN;
	int spriteTorso = 106 + 12 * stats[PLAYER_NUM]->sex;
	int spriteLegRight = 107 + 12 * stats[PLAYER_NUM]->sex;
	int spriteLegLeft = 108 + 12 * stats[PLAYER_NUM]->sex;
	int spriteArmRight = 109 + 12 * stats[PLAYER_NUM]->sex;
	int spriteArmLeft = 110 + 12 * stats[PLAYER_NUM]->sex;
	int playerAppearance = stats[PLAYER_NUM]->appearance;
	
	if ( my->effectShapeshift != NOTHING )
	{
		playerRace = static_cast<Monster>(my->effectShapeshift);
		stats[PLAYER_NUM]->type = playerRace;
	}
	else if ( stats[PLAYER_NUM]->playerRace > 0 || stats[PLAYER_NUM]->EFFECTS[EFF_POLYMORPH] || my->effectPolymorph != NOTHING )
	{
		playerRace = my->getMonsterFromPlayerRace(stats[PLAYER_NUM]->playerRace);
		if ( my->effectPolymorph != NOTHING )
		{
			if ( my->effectPolymorph > NUMMONSTERS )
			{
				playerRace = HUMAN;
				playerAppearance = my->effectPolymorph - 100;
			}
			else
			{
				playerRace = static_cast<Monster>(my->effectPolymorph);
			}
		}
		if ( stats[PLAYER_NUM]->appearance == 0 || my->effectPolymorph != NOTHING )
		{
			stats[PLAYER_NUM]->type = playerRace;
		}
		else
		{
			stats[PLAYER_NUM]->type = HUMAN; // appearance of 1 is aesthetic only
		}
	}
	else
	{
		stats[PLAYER_NUM]->type = HUMAN;
	}

	if ( stats[PLAYER_NUM]->type == RAT || stats[PLAYER_NUM]->type == SPIDER )
	{
		isHumanoid = false;
	}
	else if ( stats[PLAYER_NUM]->type == TROLL || stats[PLAYER_NUM]->type == CREATURE_IMP )
	{
		showEquipment = false;
	}

	if ( multiplayer != CLIENT )
	{
		if ( stats[PLAYER_NUM]->EFFECTS[EFF_SHAPESHIFT] )
		{
			stats[PLAYER_NUM]->playerShapeshiftStorage = my->effectShapeshift; // keep track of player shapeshift effects
		}
		else
		{
			if ( my->effectShapeshift != NOTHING ) // just in case this was cleared other than normal progression ticking down
			{
				my->effectShapeshift = NOTHING;
				serverUpdateEntitySkill(my, 53);
			}
		}

		if ( stats[PLAYER_NUM]->EFFECTS[EFF_POLYMORPH] )
		{
			stats[PLAYER_NUM]->playerPolymorphStorage = my->effectPolymorph; // keep track of player polymorph effects
		}
		else
		{
			if ( my->effectPolymorph != NOTHING ) // just in case this was cleared other than normal progression ticking down
			{
				my->effectPolymorph = NOTHING;
				serverUpdateEntitySkill(my, 50);
			}
		}
	}

	if ( players[PLAYER_NUM]->isLocalPlayer() ) // TODO: hotbar code splitscreen
	{
		if ( stats[PLAYER_NUM]->type != HUMAN && stats[PLAYER_NUM]->EFFECTS[EFF_SHAPESHIFT] )
		{
			if ( players[PLAYER_NUM]->hotbar.swapHotbarOnShapeshift == 0 )
			{
				initShapeshiftHotbar(PLAYER_NUM);
			}
			else if ( players[PLAYER_NUM]->hotbar.swapHotbarOnShapeshift != stats[PLAYER_NUM]->type )
			{
				// we likely transformed while still shapeshifted, fully init the hotbar code again.
				deinitShapeshiftHotbar(PLAYER_NUM);
				initShapeshiftHotbar(PLAYER_NUM);
			}
		}
		else if ( !stats[PLAYER_NUM]->EFFECTS[EFF_SHAPESHIFT] && players[PLAYER_NUM]->hotbar.swapHotbarOnShapeshift > 0 )
		{
			deinitShapeshiftHotbar(PLAYER_NUM);
		}
	}

	my->focalx = limbs[playerRace][0][0];
	my->focaly = limbs[playerRace][0][1];
	my->focalz = limbs[playerRace][0][2];

	if ( playerRace == GOATMAN && my->sprite == 768 )
	{
		my->focalz = limbs[playerRace][0][2] - 0.25; // minor head position fix to match male variant.
	}

	if ( playerRace == TROLL )
	{
		my->focalz += 1.25;
		my->scalex = 1.01;
		my->scaley = 1.01;
		my->scalez = 1.01;
	}
	else
	{
		my->scalex = 1.f;
		my->scaley = 1.f;
		my->scalez = 1.f;
	}

	if ( multiplayer == CLIENT )
	{
		if ( !players[PLAYER_NUM]->isLocalPlayer() )
		{
			my->flags[UPDATENEEDED] = true;
		}

		my->handleEffectsClient();

		// request entity update (check if I've been deleted)
		if ( ticks % (TICKS_PER_SECOND * 5) == my->getUID() % (TICKS_PER_SECOND * 5) )
		{
			strcpy((char*)net_packet->data, "ENTE");
			net_packet->data[4] = clientnum;
			SDLNet_Write32(my->getUID(), &net_packet->data[5]);
			net_packet->address.host = net_server.host;
			net_packet->address.port = net_server.port;
			net_packet->len = 9;
			sendPacketSafe(net_sock, -1, net_packet, 0);
		}
	}

	if ( !PLAYER_INIT )
	{
		PLAYER_INIT = 1;
		my->flags[BURNABLE] = true;

		Entity* nametag = newEntity(-1, 1, map.entities, nullptr);
		nametag->x = my->x;
		nametag->y = my->y;
		nametag->z = my->z - 6;
		nametag->sizex = 1;
		nametag->sizey = 1;
		nametag->flags[NOUPDATE] = true;
		nametag->flags[PASSABLE] = true;
		nametag->flags[SPRITE] = true;
		nametag->flags[UNCLICKABLE] = true;
        nametag->flags[BRIGHT] = true;
		nametag->behavior = &actSpriteNametag;
		nametag->parent = my->getUID();
		nametag->scalex = 0.2;
		nametag->scaley = 0.2;
		nametag->scalez = 0.2;
		nametag->skill[0] = PLAYER_NUM;
		nametag->skill[1] = playerColor(PLAYER_NUM, colorblind_lobby, false);

		// hud weapon
		if ( players[PLAYER_NUM]->isLocalPlayer() )
		{
			if ( multiplayer == CLIENT )
			{
				my->flags[UPDATENEEDED] = false;
			}

			entity = newEntity(-1, 1, map.entities, nullptr); //HUD entity.
			entity->flags[PASSABLE] = true;
			entity->flags[OVERDRAW] = true;
			entity->flags[NOUPDATE] = true;
			entity->flags[INVISIBLE] = true;
			entity->skill[11] = PLAYER_NUM;
			entity->behavior = &actHudWeapon;
			entity->focalz = -4;
			node = list_AddNodeLast(&my->children);
			node->element = entity;
			node->deconstructor = &emptyDeconstructor;
			node->size = sizeof(Entity*);
			my->bodyparts.push_back(entity);

			// magic hands

			//Left hand.
			entity = newEntity(-1, 1, map.entities, nullptr); //HUD entity.
			entity->flags[PASSABLE] = true;
			entity->flags[OVERDRAW] = true;
			entity->flags[NOUPDATE] = true;
			entity->flags[INVISIBLE] = true;
			entity->skill[2] = PLAYER_NUM;
			entity->behavior = &actLeftHandMagic;
			entity->focalz = -4;
			players[PLAYER_NUM]->hud.magicLeftHand = entity;
			//Right hand.
			entity = newEntity(-1, 1, map.entities, nullptr); //HUD entity.
			entity->flags[PASSABLE] = true;
			entity->flags[OVERDRAW] = true;
			entity->flags[NOUPDATE] = true;
			entity->flags[INVISIBLE] = true;
			entity->skill[2] = PLAYER_NUM;
			entity->behavior = &actRightHandMagic;
			entity->focalz = -4;
			players[PLAYER_NUM]->hud.magicRightHand = entity;
			my->bodyparts.push_back(entity);

			// hud shield
			entity = newEntity(-1, 1, map.entities, nullptr); //HUD entity.
			entity->flags[PASSABLE] = true;
			entity->flags[OVERDRAW] = true;
			entity->flags[NOUPDATE] = true;
			entity->flags[INVISIBLE] = true;
			entity->skill[2] = PLAYER_NUM;
			entity->behavior = &actHudShield;
			my->bodyparts.push_back(entity);

			// hud additional limb
			entity = newEntity(-1, 1, map.entities, nullptr); //HUD entity.
			entity->flags[PASSABLE] = true;
			entity->flags[OVERDRAW] = true;
			entity->flags[NOUPDATE] = true;
			entity->flags[INVISIBLE] = true;
			entity->skill[2] = PLAYER_NUM;
			entity->behavior = &actHudAdditional;
			my->bodyparts.push_back(entity);

			// hud additional limb 2
			entity = newEntity(-1, 1, map.entities, nullptr); //HUD entity.
			entity->flags[PASSABLE] = true;
			entity->flags[OVERDRAW] = true;
			entity->flags[NOUPDATE] = true;
			entity->flags[INVISIBLE] = true;
			entity->skill[2] = PLAYER_NUM;
			entity->behavior = &actHudArrowModel;
			my->bodyparts.push_back(entity);
		}
		else
		{
			node = list_AddNodeLast(&my->children);
			node->element = NULL;
			node->deconstructor = &emptyDeconstructor;
			node->size = 0;
		}

		// torso
		entity = newEntity(spriteTorso, 1, map.entities, nullptr); //Limb entity.
		entity->sizex = 4;
		entity->sizey = 4;
		entity->skill[2] = PLAYER_NUM;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[GENIUS] = true;
		entity->focalx = limbs[playerRace][1][0];
		entity->focaly = limbs[playerRace][1][1];
		entity->focalz = limbs[playerRace][1][2];
		entity->behavior = &actPlayerLimb;
		entity->parent = my->getUID();
		node = list_AddNodeLast(&my->children);
		node->element = entity;
		node->deconstructor = &emptyDeconstructor;
		node->size = sizeof(Entity*);
		my->bodyparts.push_back(entity);
		entity->setDefaultPlayerModel(PLAYER_NUM, playerRace, LIMB_HUMANOID_TORSO);

		// right leg
		entity = newEntity(spriteLegRight, 1, map.entities, nullptr); //Limb entity.
		entity->sizex = 4;
		entity->sizey = 4;
		entity->skill[2] = PLAYER_NUM;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[GENIUS] = true;
		entity->focalx = limbs[playerRace][2][0];
		entity->focaly = limbs[playerRace][2][1];
		entity->focalz = limbs[playerRace][2][2];
		entity->behavior = &actPlayerLimb;
		entity->parent = my->getUID();
		node = list_AddNodeLast(&my->children);
		node->element = entity;
		node->deconstructor = &emptyDeconstructor;
		node->size = sizeof(Entity*);
		my->bodyparts.push_back(entity);
		entity->setDefaultPlayerModel(PLAYER_NUM, playerRace, LIMB_HUMANOID_RIGHTLEG);

		// left leg
		entity = newEntity(spriteLegLeft, 1, map.entities, nullptr); //Limb entity.
		entity->sizex = 4;
		entity->sizey = 4;
		entity->skill[2] = PLAYER_NUM;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[GENIUS] = true;
		entity->focalx = limbs[playerRace][3][0];
		entity->focaly = limbs[playerRace][3][1];
		entity->focalz = limbs[playerRace][3][2];
		entity->behavior = &actPlayerLimb;
		entity->parent = my->getUID();
		node = list_AddNodeLast(&my->children);
		node->element = entity;
		node->deconstructor = &emptyDeconstructor;
		node->size = sizeof(Entity*);
		my->bodyparts.push_back(entity);
		entity->setDefaultPlayerModel(PLAYER_NUM, playerRace, LIMB_HUMANOID_LEFTLEG);

		// right arm
		entity = newEntity(spriteArmRight, 1, map.entities, nullptr); //Limb entity.
		entity->sizex = 4;
		entity->sizey = 4;
		entity->skill[2] = PLAYER_NUM;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[GENIUS] = true;
		entity->focalx = limbs[playerRace][4][0];
		entity->focaly = limbs[playerRace][4][1];
		entity->focalz = limbs[playerRace][4][2];
		entity->behavior = &actPlayerLimb;
		entity->parent = my->getUID();
		node = list_AddNodeLast(&my->children);
		node->element = entity;
		node->deconstructor = &emptyDeconstructor;
		node->size = sizeof(Entity*);
		my->bodyparts.push_back(entity);
		entity->setDefaultPlayerModel(PLAYER_NUM, playerRace, LIMB_HUMANOID_RIGHTARM);

		// left arm
		entity = newEntity(spriteArmLeft, 1, map.entities, nullptr); //Limb entity.
		entity->sizex = 4;
		entity->sizey = 4;
		entity->skill[2] = PLAYER_NUM;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[GENIUS] = true;
		entity->focalx = limbs[playerRace][5][0];
		entity->focaly = limbs[playerRace][5][1];
		entity->focalz = limbs[playerRace][5][2];
		entity->behavior = &actPlayerLimb;
		entity->parent = my->getUID();
		node = list_AddNodeLast(&my->children);
		node->element = entity;
		node->deconstructor = &emptyDeconstructor;
		node->size = sizeof(Entity*);
		my->bodyparts.push_back(entity);
		entity->setDefaultPlayerModel(PLAYER_NUM, playerRace, LIMB_HUMANOID_LEFTARM);

		// world weapon
		entity = newEntity(-1, 1, map.entities, nullptr); //Limb entity.
		entity->sizex = 4;
		entity->sizey = 4;
		entity->skill[2] = PLAYER_NUM;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[GENIUS] = true;
		entity->flags[INVISIBLE] = true;
		entity->focalx = limbs[playerRace][6][0];
		entity->focaly = limbs[playerRace][6][1];
		entity->focalz = limbs[playerRace][6][2];
		entity->behavior = &actPlayerLimb;
		entity->parent = my->getUID();
		node = list_AddNodeLast(&my->children);
		node->element = entity;
		node->deconstructor = &emptyDeconstructor;
		node->size = sizeof(Entity*);
		my->bodyparts.push_back(entity);

		// shield
		entity = newEntity(-1, 1, map.entities, nullptr); //Limb entity.
		entity->sizex = 4;
		entity->sizey = 4;
		entity->skill[2] = PLAYER_NUM;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[GENIUS] = true;
		entity->flags[INVISIBLE] = true;
		entity->focalx = limbs[playerRace][7][0];
		entity->focaly = limbs[playerRace][7][1];
		entity->focalz = limbs[playerRace][7][2];
		entity->behavior = &actPlayerLimb;
		entity->parent = my->getUID();
		entity->focalx = 2;
		node = list_AddNodeLast(&my->children);
		node->element = entity;
		node->deconstructor = &emptyDeconstructor;
		node->size = sizeof(Entity*);
		my->bodyparts.push_back(entity);

		// cloak
		entity = newEntity(-1, 1, map.entities, nullptr); //Limb entity.
		entity->sizex = 4;
		entity->sizey = 4;
		entity->scalex = 1.01;
		entity->scaley = 1.01;
		entity->scalez = 1.01;
		entity->skill[2] = PLAYER_NUM;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[GENIUS] = true;
		entity->flags[INVISIBLE] = true;
		entity->focalx = limbs[playerRace][8][0];
		entity->focaly = limbs[playerRace][8][1];
		entity->focalz = limbs[playerRace][8][2];
		entity->behavior = &actPlayerLimb;
		entity->parent = my->getUID();
		node = list_AddNodeLast(&my->children);
		node->element = entity;
		node->deconstructor = &emptyDeconstructor;
		node->size = sizeof(Entity*);
		my->bodyparts.push_back(entity);

		// helmet
		entity = newEntity(-1, 1, map.entities, nullptr); //Limb entity.
		entity->sizex = 4;
		entity->sizey = 4;
		entity->scalex = 1.01;
		entity->scaley = 1.01;
		entity->scalez = 1.01;
		entity->skill[2] = PLAYER_NUM;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[GENIUS] = true;
		entity->flags[INVISIBLE] = true;
		entity->focalx = limbs[playerRace][9][0];
		entity->focaly = limbs[playerRace][9][1];
		entity->focalz = limbs[playerRace][9][2];
		entity->behavior = &actPlayerLimb;
		entity->parent = my->getUID();
		node = list_AddNodeLast(&my->children);
		node->element = entity;
		node->deconstructor = &emptyDeconstructor;
		node->size = sizeof(Entity*);
		my->bodyparts.push_back(entity);

		// mask
		entity = newEntity(-1, 1, map.entities, nullptr); //Limb entity.
		entity->sizex = 4;
		entity->sizey = 4;
		entity->scalex = .99;
		entity->scaley = .99;
		entity->scalez = .99;
		entity->skill[2] = PLAYER_NUM;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[GENIUS] = true;
		entity->flags[INVISIBLE] = true;
		entity->focalx = limbs[playerRace][10][0];
		entity->focaly = limbs[playerRace][10][1];
		entity->focalz = limbs[playerRace][10][2];
		entity->behavior = &actPlayerLimb;
		entity->parent = my->getUID();
		node = list_AddNodeLast(&my->children);
		node->element = entity;
		node->deconstructor = &emptyDeconstructor;
		node->size = sizeof(Entity*);
		my->bodyparts.push_back(entity);

		// additional limb 1
		entity = newEntity(-1, 1, map.entities, nullptr); //Limb entity.
		entity->sizex = 4;
		entity->sizey = 4;
		entity->skill[2] = PLAYER_NUM;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[GENIUS] = true;
		entity->flags[INVISIBLE] = true;
		entity->focalx = limbs[playerRace][11][0];
		entity->focaly = limbs[playerRace][11][1];
		entity->focalz = limbs[playerRace][11][2];
		entity->behavior = &actPlayerLimb;
		entity->parent = my->getUID();
		node = list_AddNodeLast(&my->children);
		node->element = entity;
		node->deconstructor = &emptyDeconstructor;
		node->size = sizeof(Entity*);
		my->bodyparts.push_back(entity);

		// additional limb 2
		entity = newEntity(-1, 1, map.entities, nullptr); //Limb entity.
		entity->sizex = 4;
		entity->sizey = 4;
		entity->skill[2] = PLAYER_NUM;
		entity->flags[PASSABLE] = true;
		entity->flags[NOUPDATE] = true;
		entity->flags[GENIUS] = true;
		entity->flags[INVISIBLE] = true;
		entity->focalx = limbs[playerRace][12][0];
		entity->focaly = limbs[playerRace][12][1];
		entity->focalz = limbs[playerRace][12][2];
		entity->behavior = &actPlayerLimb;
		entity->parent = my->getUID();
		node = list_AddNodeLast(&my->children);
		node->element = entity;
		node->deconstructor = &emptyDeconstructor;
		node->size = sizeof(Entity*);
		my->bodyparts.push_back(entity);

		// additional limb 3 - 18.
		for ( int c = 0; c < 8; ++c )
		{
			entity = newEntity(-1, 1, map.entities, nullptr); //Limb entity.
			entity->sizex = 1;
			entity->sizey = 1;
			entity->skill[2] = PLAYER_NUM;
			entity->fskill[10] = (c / 8.f);
			entity->flags[PASSABLE] = true;
			entity->flags[NOUPDATE] = true;
			entity->flags[GENIUS] = true;
			entity->flags[INVISIBLE] = true;
			entity->focalx = limbs[playerRace][11][0];
			entity->focaly = limbs[playerRace][11][1];
			entity->focalz = limbs[playerRace][11][2];
			entity->behavior = &actPlayerLimb;
			entity->parent = my->getUID();
			node = list_AddNodeLast(&my->children);
			node->element = entity;
			node->deconstructor = &emptyDeconstructor;
			node->size = sizeof(Entity*);
			my->bodyparts.push_back(entity);

			entity = newEntity(-1, 1, map.entities, nullptr); //Limb entity.
			entity->sizex = 1;
			entity->sizey = 1;
			entity->skill[2] = PLAYER_NUM;
			entity->flags[PASSABLE] = true;
			entity->flags[NOUPDATE] = true;
			entity->flags[GENIUS] = true;
			entity->flags[INVISIBLE] = true;
			entity->focalx = limbs[playerRace][12][0];
			entity->focaly = limbs[playerRace][12][1];
			entity->focalz = limbs[playerRace][12][2];
			entity->behavior = &actPlayerLimb;
			entity->parent = my->getUID();
			node = list_AddNodeLast(&my->children);
			node->element = entity;
			node->deconstructor = &emptyDeconstructor;
			node->size = sizeof(Entity*);
			my->bodyparts.push_back(entity);
		}
	}
	Uint32 color = makeColorRGB(255, 0, 255);
	int blueSpeechVolume = 100;
	int orangeSpeechVolume = 128;

	if ( !intro )
	{
		PLAYER_ALIVETIME++;
		if ( PLAYER_NUM == clientnum ) // specifically the host - in splitscreen we only process this once for all players.
		{
			if ( PLAYER_ALIVETIME == 300 && gameModeManager.currentMode == GameModeManager_t::GameModes::GAME_MODE_DEFAULT )
			{
				// we no longer do the following:
				// take a screenshot to be associated with the current save game
				//auto screenshot_path = setSaveGameFileName(multiplayer == SINGLE, SaveFileType::SCREENSHOT);
				//takeScreenshot(screenshot_path.c_str());
			}
			clientplayer = my->getUID();
			if ( !strcmp(map.name, "Boss") && !my->skill[29] )
			{
				bool foundherx = false;
				for ( node = map.creatures->first; node != nullptr; node = node->next ) //Herx is in the creature list, so only search that.
				{
					Entity* entity = (Entity*)node->element;
					if ( entity->sprite == 274 )
					{
						foundherx = true;
						break;
					}
				}
				if ( !foundherx )
				{
					// ding, dong, the witch is dead
					my->skill[29] = PLAYER_ALIVETIME;
				}
				else
				{
					if ( PLAYER_ALIVETIME == 300 )
					{
						playSound(185, 128);
						messageLocalPlayersColor(color, MESSAGE_WORLD, Language::get(537));
						messageLocalPlayersColor(color, MESSAGE_WORLD, Language::get(89));
					}
				}
			}
			else
			{
				if ( PLAYER_ALIVETIME == 300 && !MFLAG_DISABLEMESSAGES )
				{
					// five seconds in, herx chimes in (maybe)
					// replicate the messagePlayer to all splitscreen clients so it's not randomly different between screens
					my->playerLevelEntrySpeech = 0;
					if ( currentlevel == 0 && !secretlevel )
					{
						int speech = local_rng.rand() % 3;
						playSound(126 + speech, 128);
						messageLocalPlayersColor(color, MESSAGE_WORLD, Language::get(537));
						messageLocalPlayersColor(color, MESSAGE_WORLD, Language::get(77 + speech));
					}
					else if ( currentlevel == 1 && !secretlevel )
					{
						int speech = local_rng.rand() % 3;
						playSound(117 + speech, 128);
						messageLocalPlayersColor(color, MESSAGE_WORLD, Language::get(537));
						messageLocalPlayersColor(color, MESSAGE_WORLD, Language::get(70 + speech));
					}
					else if ( currentlevel == 5 && !secretlevel )
					{
						int speech = local_rng.rand() % 2;
						playSound(156 + speech, 128);
						messageLocalPlayersColor(color, MESSAGE_WORLD, Language::get(537));
						messageLocalPlayersColor(color, MESSAGE_WORLD, Language::get(83 + speech));
					}
					else if ( currentlevel == 10 && !secretlevel )
					{
						int speech = local_rng.rand() % 2;
						playSound(158 + speech, 128);
						messageLocalPlayersColor(color, MESSAGE_WORLD, Language::get(537));
						messageLocalPlayersColor(color, MESSAGE_WORLD, Language::get(85 + speech));
					}
					else if ( currentlevel == 15 && !secretlevel )
					{
						int speech = local_rng.rand() % 2;
						playSound(160 + speech, 128);
						messageLocalPlayersColor(color, MESSAGE_WORLD, Language::get(537));
						messageLocalPlayersColor(color, MESSAGE_WORLD, Language::get(87 + speech));
					}
					else if ( currentlevel == 26 && !secretlevel )
					{
						int speech = 1 + local_rng.rand() % 3;
						switch ( speech )
						{
							case 1:
								playSound(341, blueSpeechVolume);
								messageLocalPlayersColor(uint32ColorBaronyBlue, MESSAGE_WORLD, Language::get(537));
								messageLocalPlayersColor(uint32ColorBaronyBlue, MESSAGE_WORLD, Language::get(2615));
								break;
							case 2:
								playSound(343, orangeSpeechVolume);
								messageLocalPlayersColor(uint32ColorOrange, MESSAGE_WORLD, Language::get(537));
								messageLocalPlayersColor(uint32ColorOrange, MESSAGE_WORLD, Language::get(2617));
								break;
							case 3:
								playSound(346, orangeSpeechVolume);
								messageLocalPlayersColor(uint32ColorOrange, MESSAGE_WORLD, Language::get(537));
								messageLocalPlayersColor(uint32ColorOrange, MESSAGE_WORLD, Language::get(2620));
								break;
						}
						my->playerLevelEntrySpeech = speech;
					}
					else if ( currentlevel == 28 && !secretlevel )
					{
						int speech = 1 + local_rng.rand() % 3;
						switch ( speech )
						{
							case 1:
								playSound(349, blueSpeechVolume);
								messageLocalPlayersColor(uint32ColorBaronyBlue, MESSAGE_WORLD, Language::get(537));
								messageLocalPlayersColor(uint32ColorBaronyBlue, MESSAGE_WORLD, Language::get(2629));
								break;
							case 2:
								playSound(352, orangeSpeechVolume);
								messageLocalPlayersColor(uint32ColorOrange, MESSAGE_WORLD, Language::get(537));
								messageLocalPlayersColor(uint32ColorOrange, MESSAGE_WORLD, Language::get(2632));
								break;
							case 3:
								playSound(354, blueSpeechVolume);
								messageLocalPlayersColor(uint32ColorBaronyBlue, MESSAGE_WORLD, Language::get(537));
								messageLocalPlayersColor(uint32ColorBaronyBlue, MESSAGE_WORLD, Language::get(2634));
								break;
						}
						my->playerLevelEntrySpeech = speech;
					}
					else if ( currentlevel == 30 && !secretlevel )
					{
						int speech = 1;
						switch ( speech )
						{
							case 1:
								playSound(356, blueSpeechVolume - 16);
								messageLocalPlayersColor(uint32ColorBaronyBlue, MESSAGE_WORLD, Language::get(537));
								messageLocalPlayersColor(uint32ColorBaronyBlue, MESSAGE_WORLD, Language::get(2636));
								break;
						}
						my->playerLevelEntrySpeech = speech;
					}
					else if ( currentlevel == 31 && !secretlevel )
					{
						int speech = 1;
						switch ( speech )
						{
							case 1:
								playSound(358, blueSpeechVolume);
								messageLocalPlayersColor(uint32ColorBaronyBlue, MESSAGE_WORLD, Language::get(537));
								messageLocalPlayersColor(uint32ColorBaronyBlue, MESSAGE_WORLD, Language::get(2638));
								break;
						}
						my->playerLevelEntrySpeech = speech;
					}
					else if ( currentlevel == 33 && !secretlevel )
					{
						int speech = 1 + local_rng.rand() % 2;
						switch ( speech )
						{
							case 1:
								playSound(360, orangeSpeechVolume);
								messageLocalPlayersColor(uint32ColorOrange, MESSAGE_WORLD, Language::get(537));
								messageLocalPlayersColor(uint32ColorOrange, MESSAGE_WORLD, Language::get(2640));
								break;
							case 2:
								playSound(362, blueSpeechVolume);
								messageLocalPlayersColor(uint32ColorBaronyBlue, MESSAGE_WORLD, Language::get(537));
								messageLocalPlayersColor(uint32ColorBaronyBlue, MESSAGE_WORLD, Language::get(2642));
								break;
						}
						my->playerLevelEntrySpeech = speech;
					}
					else if ( currentlevel == 35 && !secretlevel )
					{
						int speech = 1;
						switch ( speech )
						{
							case 1:
								playSound(364, orangeSpeechVolume);
								messageLocalPlayersColor(uint32ColorOrange, MESSAGE_WORLD, Language::get(537));
								messageLocalPlayersColor(uint32ColorOrange, MESSAGE_WORLD, Language::get(2644));
								break;
						}
						my->playerLevelEntrySpeech = speech;
					}
					else if ( minotaurlevel )
					{
						if ( currentlevel < 25 )
						{
							int speech = local_rng.rand() % 3;
							playSound(123 + speech, 128);
							messageLocalPlayersColor(color, MESSAGE_WORLD, Language::get(537));
							messageLocalPlayersColor(color, MESSAGE_WORLD, Language::get(74 + speech));
						}
						else
						{
							int speech = 1 + local_rng.rand() % 3;
							switch ( speech )
							{
								case 1:
									playSound(366, blueSpeechVolume);
									messageLocalPlayersColor(uint32ColorBaronyBlue, MESSAGE_WORLD, Language::get(537));
									messageLocalPlayersColor(uint32ColorBaronyBlue, MESSAGE_WORLD, Language::get(2623));
									break;
								case 2:
									playSound(368, orangeSpeechVolume);
									messageLocalPlayersColor(uint32ColorOrange, MESSAGE_WORLD, Language::get(537));
									messageLocalPlayersColor(uint32ColorOrange, MESSAGE_WORLD, Language::get(2625));
									break;
								case 3:
									playSound(370, blueSpeechVolume);
									messageLocalPlayersColor(uint32ColorBaronyBlue, MESSAGE_WORLD, Language::get(537));
									messageLocalPlayersColor(uint32ColorBaronyBlue, MESSAGE_WORLD, Language::get(2627));
									break;
							}
							my->playerLevelEntrySpeech = speech;
						}
					}
				}
				else if ( PLAYER_ALIVETIME == 480 && !MFLAG_DISABLEMESSAGES )
				{
					// 8 seconds in, herx chimes in again (maybe)
					if ( currentlevel == 1 && !secretlevel )
					{
						playSound(120 + local_rng.rand() % 3, 128);
						messageLocalPlayersColor(color, MESSAGE_WORLD, Language::get(73));
					}
					else if ( minotaurlevel && currentlevel < 25 )
					{
						int speech = local_rng.rand() % 3;
						playSound(129 + speech, 128);
						messageLocalPlayersColor(color, MESSAGE_WORLD, Language::get(80 + speech));
					}
				}
				else if ( my->playerLevelEntrySpeech > 0 )
				{
					my->playerLevelEntrySpeechSecond();
				}
			}
		}

		if ( players[PLAYER_NUM]->isLocalPlayer() )
		{
			// sharur the talking mace
			if ( stats[PLAYER_NUM]->weapon )
			{
				if ( stats[PLAYER_NUM]->weapon->type == ARTIFACT_MACE )
				{
					if ( PLAYER_ALIVETIME % 420 == 0 )
					{
						messagePlayerColor(PLAYER_NUM, MESSAGE_WORLD, color, Language::get(538 + local_rng.rand() % 32));
					}
				}
			}

			if ( players[PLAYER_NUM]->movement.monsterEmoteGimpTimer > 0 )
			{
				--players[PLAYER_NUM]->movement.monsterEmoteGimpTimer;
			}
		}
		if ( multiplayer == SERVER )
		{
			if ( my->getUID() % (TICKS_PER_SECOND * 3) == ticks % (TICKS_PER_SECOND * 3) )
			{
				serverUpdateBodypartIDs(my);

				int i;
				for ( i = 1; i < 11; i++ )
				{
					serverUpdateEntityBodypart(my, i);
				}
			}
		}
		if ( multiplayer != CLIENT )
		{
			if ( PLAYER_ALIVETIME == 50 && currentlevel == 0 )
			{
				int monsterSquad = 0;
				for ( int c = 0; c < MAXPLAYERS; ++c )
				{
					if ( players[c] && players[c]->entity && stats[c]->playerRace > 0 )
					{
						++monsterSquad;
					}
				}
				if ( monsterSquad >= 3 )
				{
					for ( int c = 0; c < MAXPLAYERS; ++c )
					{
						steamAchievementClient(c, "BARONY_ACH_MONSTER_SQUAD");
					}
				}
				if ( client_classes[PLAYER_NUM] == CLASS_ACCURSED )
				{
					my->setEffect(EFF_VAMPIRICAURA, true, -2, true);
					my->playerVampireCurse = 1;
					serverUpdateEntitySkill(my, 51);
					Uint32 color = makeColorRGB(0, 255, 0);
					messagePlayerColor(PLAYER_NUM, MESSAGE_STATUS, color, Language::get(2477));
					color = makeColorRGB(255, 255, 0);
					messagePlayerColor(PLAYER_NUM, MESSAGE_HINT, color, Language::get(3202));

					playSoundEntity(my, 167, 128);
					playSoundEntity(my, 403, 128);
					createParticleDropRising(my, 600, 0.7);
					serverSpawnMiscParticles(my, PARTICLE_EFFECT_VAMPIRIC_AURA, 600);
				}
			}
			if ( currentlevel == 0 && stats[PLAYER_NUM]->playerRace == RACE_GOATMAN && stats[PLAYER_NUM]->appearance == 0 )
			{
				if ( PLAYER_ALIVETIME == 1 )
				{
					my->setEffect(EFF_WITHDRAWAL, true, -2, true);
				}
				if ( PLAYER_ALIVETIME == 330 )
				{
					my->setEffect(EFF_ASLEEP, false, 0, true);
					if ( svFlags & SV_FLAG_HUNGER )
					{
						if ( stats[PLAYER_NUM]->HUNGER <= 1000 ) // just in case you ate before scripted sequence
						{
							playSoundPlayer(PLAYER_NUM, 32, 128);
							stats[PLAYER_NUM]->HUNGER = 150;
							serverUpdateHunger(PLAYER_NUM);
						}
						else
						{
							stats[PLAYER_NUM]->HUNGER -= 850;
							serverUpdateHunger(PLAYER_NUM);
						}
					}
				}
				if ( stats[PLAYER_NUM]->EFFECTS[EFF_WITHDRAWAL] )
				{
					if ( PLAYER_ALIVETIME == 500 )
					{
						color = makeColorRGB(255, 255, 255);
						messagePlayerColor(PLAYER_NUM, MESSAGE_HINT, color, Language::get(3221));
					}
					else if ( PLAYER_ALIVETIME == 700 )
					{
						color = makeColorRGB(255, 255, 255);
						messagePlayerColor(PLAYER_NUM, MESSAGE_HINT, color, Language::get(3222));
					}
				}
			}
		}
		if ( multiplayer == CLIENT && client_classes[PLAYER_NUM] == CLASS_ACCURSED )
		{
			if ( players[PLAYER_NUM]->isLocalPlayer() && my->playerVampireCurse == 1 )
			{
				stats[PLAYER_NUM]->EFFECTS_TIMERS[EFF_VAMPIRICAURA] = -2;
			}
			else
			{
				stats[PLAYER_NUM]->EFFECTS_TIMERS[EFF_VAMPIRICAURA] = 0;
			}
		}
	}

	// debug stuff
	if (enableDebugKeys)
	{
		static ConsoleVariable<bool> cvar_levelgentest("/levelgentest", false);
		if ( *cvar_levelgentest && PLAYER_ALIVETIME == 15 )
		{
			consoleCommand("/jumplevel -1");
		}
		if ( keystatus[SDLK_LCTRL] && keystatus[SDLK_KP_1] )
	    {
		    Input::waitingToBindControllerForPlayer = 0;
		    keystatus[SDLK_KP_1] = 0;
		    messagePlayer(PLAYER_NUM, MESSAGE_DEBUG, "Waiting to bind controller for player: 0");
	    }
	    if ( keystatus[SDLK_LCTRL] && keystatus[SDLK_KP_2] )
	    {
		    Input::waitingToBindControllerForPlayer = 1;
		    keystatus[SDLK_KP_2] = 0;
		    messagePlayer(PLAYER_NUM, MESSAGE_DEBUG, "Waiting to bind controller for player: 1");
	    }
	    if ( keystatus[SDLK_LCTRL] && keystatus[SDLK_KP_3] )
	    {
		    Input::waitingToBindControllerForPlayer = 2;
		    keystatus[SDLK_KP_3] = 0;
		    messagePlayer(PLAYER_NUM, MESSAGE_DEBUG, "Waiting to bind controller for player: 2");
	    }
	    if ( keystatus[SDLK_LCTRL] && keystatus[SDLK_KP_4] )
	    {
		    Input::waitingToBindControllerForPlayer = 3;
		    keystatus[SDLK_KP_4] = 0;
		    messagePlayer(PLAYER_NUM, MESSAGE_DEBUG, "Waiting to bind controller for player: 3");
	    }
	    if ( keystatus[SDLK_LCTRL] && keystatus[SDLK_KP_5] )
	    {
		    keystatus[SDLK_KP_5] = 0;
		    consoleCommand("/cyclekeyboard");
	    }
	    if ( keystatus[SDLK_LCTRL] && keystatus[SDLK_KP_0] )
	    {
		    keystatus[SDLK_KP_0] = 0;
		    inputs.setPlayerIDAllowedKeyboard(-1);
	        for ( int i = 0; i < MAXPLAYERS; ++i )
	        {
		        Input::inputs[i].refresh();
	        }
		    messagePlayer(PLAYER_NUM, MESSAGE_DEBUG, "Removed keyboard for any player");
	    }
		if ( keystatus[SDLK_LCTRL] && keystatus[SDLK_KP_7] )
		{
			keystatus[SDLK_KP_7] = 0;
			consoleCommand("/disable_controller_reconnect true");
			consoleCommand("/splitscreen");
		}
		if ( keystatus[SDLK_LCTRL] && keystatus[SDLK_KP_8] )
		{
			keystatus[SDLK_KP_8] = 0;
			consoleCommand("/disable_controller_reconnect true");
			consoleCommand("/splitscreen 2");
		}
		if ( keystatus[SDLK_LCTRL] && keystatus[SDLK_KP_6] )
		{
			keystatus[SDLK_KP_6] = 0;
			if ( keystatus[SDLK_LALT] )
			{
				consoleCommand("/split_clipped true");
			}
			else
			{
				consoleCommand("/split_clipped false");
				*MainMenu::vertical_splitscreen = !*MainMenu::vertical_splitscreen;
			}
			consoleCommand("/split_refresh");
		}
	    if ( keystatus[SDLK_LCTRL] && keystatus[SDLK_KP_9] )
	    {
		    keystatus[SDLK_KP_9] = 0;
		    for ( int i = 0; i < MAXPLAYERS; ++i )
		    {
			    if ( inputs.hasController(i) )
			    {
				    inputs.removeControllerWithDeviceID(inputs.getControllerID(i));
			    }
		    }
		    for ( int i = 0; i < MAXPLAYERS; ++i )
		    {
			    Input::inputs[i].refresh();
		    }
		    messagePlayer(PLAYER_NUM, MESSAGE_DEBUG, "Removed gamepad for player: %d", PLAYER_NUM);
	    }
	}

	static ConsoleVariable<bool> cvar_player_light_radius_test("/player_light_radius_test", false);
	if ( *cvar_player_light_radius_test && (svFlags & SV_FLAG_CHEATS) )
	{
		if ( ticks % 4 == 0 )
		{
			int entityLight = my->entityLightAfterReductions(*stats[PLAYER_NUM], my);
			entityLight = std::max(TOUCHRANGE, entityLight);
			for ( int degree = 0; degree < 360; degree += 1 )
			{
				real_t rad = degree * PI / 180.0;
				Entity* particle = spawnMagicParticle(my);
				particle->sprite = 576;
				particle->x = my->x + entityLight * cos(rad);
				particle->y = my->y + entityLight * sin(rad);
				particle->z = 7;
				particle->ditheringDisabled = true;
			}
		}
	}

	if ( multiplayer != CLIENT )
	{
		if ( my->getUID() % TICKS_PER_SECOND == ticks % TICKS_PER_SECOND )
		{
			monsterAllyFormations.updateFormation(my->getUID());
		}
	}

	if ( players[PLAYER_NUM]->isLocalPlayer() 
		&& players[PLAYER_NUM]->inventoryUI.appraisal.timer > 0 )
	{
		Item* tempItem = uidToItem(players[PLAYER_NUM]->inventoryUI.appraisal.current_item);
		if ( tempItem )
		{
			if ( tempItem->identified )
			{
				players[PLAYER_NUM]->inventoryUI.appraisal.timer = 0;
				players[PLAYER_NUM]->inventoryUI.appraisal.current_item = 0;
			}
			else if ( tempItem->type == GEM_ROCK )
			{
				//Auto-succeed on rocks.
				tempItem->identified = true;
				tempItem->notifyIcon = true;
				messagePlayer(PLAYER_NUM, MESSAGE_INVENTORY, Language::get(570), tempItem->description());
				players[PLAYER_NUM]->inventoryUI.appraisal.current_item = 0;
				players[PLAYER_NUM]->inventoryUI.appraisal.timer = 0;

				// update inventory by trying to stack the newly identified item.
				std::unordered_set<Uint32> appearancesOfSimilarItems;
				for ( node = stats[PLAYER_NUM]->inventory.first; node != NULL; node = node->next )
				{
					Item* item2 = (Item*)node->element;
					if ( item2 == tempItem )
					{
						continue;
					}
					if ( !item2 )
					{
						continue;
					}
					if ( !itemCompare(tempItem, item2, false) )
					{
						// if items are the same, check to see if they should stack
						if ( item2->shouldItemStack(PLAYER_NUM) )
						{
							if ( itemIsEquipped(tempItem, PLAYER_NUM) && players[PLAYER_NUM]->paperDoll.isItemOnDoll(*tempItem) )
							{
								// don't try to move our equipped item - it's an edge case to crash
								if ( tempItem->appearance == item2->appearance )
								{
									// items are the same (incl. appearance!)
									// if they shouldn't stack, we need to change appearance of the new item.
									appearancesOfSimilarItems.insert(item2->appearance);
								}
								continue;
							}

							item2->count += tempItem->count;
							if ( multiplayer == CLIENT && itemIsEquipped(item2, PLAYER_NUM) )
							{
								// if incrementing qty and holding item, then send "equip" for server to update their count of your held item.
								clientSendEquipUpdateToServer(EQUIP_ITEM_SLOT_WEAPON, EQUIP_ITEM_SUCCESS_UPDATE_QTY, PLAYER_NUM,
									item2->type, item2->status, item2->beatitude, item2->count, item2->appearance, item2->identified);
							}
							if ( tempItem->node )
							{
								list_RemoveNode(tempItem->node);
								tempItem = nullptr;
							}
							else
							{
								free(tempItem);
								tempItem = nullptr;
							}
							break;
						}
						else if ( !itemCompare(tempItem, item2, true) )
						{
							// items are the same (incl. appearance!)
							// if they shouldn't stack, we need to change appearance of the new item.
							appearancesOfSimilarItems.insert(item2->appearance);
						}
					}
				}
				if ( tempItem  )
				{
					if ( !appearancesOfSimilarItems.empty() )
					{
						Uint32 originalAppearance = tempItem->appearance;
						int originalVariation = originalAppearance % items[tempItem->type].variations;

						int tries = 100;
						// we need to find a unique appearance within the list.
						tempItem->appearance = local_rng.rand();
						if ( tempItem->appearance % items[tempItem->type].variations != originalVariation )
						{
							// we need to match the variation for the new appearance, take the difference so new varation matches
							int change = (tempItem->appearance % items[tempItem->type].variations - originalVariation);
							if ( tempItem->appearance < change ) // underflow protection
							{
								tempItem->appearance += items[tempItem->type].variations;
							}
							tempItem->appearance -= change;
							int newVariation = tempItem->appearance % items[tempItem->type].variations;
							assert(newVariation == originalVariation);
						}
						auto it = appearancesOfSimilarItems.find(tempItem->appearance);
						while ( it != appearancesOfSimilarItems.end() && tries > 0 )
						{
							tempItem->appearance = local_rng.rand();
							if ( tempItem->appearance % items[tempItem->type].variations != originalVariation )
							{
								// we need to match the variation for the new appearance, take the difference so new varation matches
								int change = (tempItem->appearance % items[tempItem->type].variations - originalVariation);
								if ( tempItem->appearance < change ) // underflow protection
								{
									tempItem->appearance += items[tempItem->type].variations;
								}
								tempItem->appearance -= change;
								int newVariation = tempItem->appearance % items[tempItem->type].variations;
								assert(newVariation == originalVariation);
							}
							it = appearancesOfSimilarItems.find(tempItem->appearance);
							--tries;
						}
					}

					if ( multiplayer == CLIENT && itemIsEquipped(tempItem, PLAYER_NUM) && players[PLAYER_NUM]->paperDoll.isItemOnDoll(*tempItem) )
					{
						clientSendAppearanceUpdateToServer(PLAYER_NUM, tempItem, true);
					}
				}
			}
			else
			{
				players[PLAYER_NUM]->inventoryUI.appraisal.timer -= 1; //De-increment appraisal timer.
				if ( players[PLAYER_NUM]->inventoryUI.appraisal.timer <= 0)
				{
					players[PLAYER_NUM]->inventoryUI.appraisal.timer = 0;

					//Cool. Time to identify the item.
					bool success = false;
					if ( stats[PLAYER_NUM]->PROFICIENCIES[PRO_APPRAISAL] < 100 )
					{
						if ( tempItem->type != GEM_GLASS )
						{
							success = (stats[PLAYER_NUM]->PROFICIENCIES[PRO_APPRAISAL] + my->getPER() * 5 >= items[tempItem->type].value / 10);
						}
						else
						{
							success = (stats[PLAYER_NUM]->PROFICIENCIES[PRO_APPRAISAL] + my->getPER() * 5 >= 100);
						}
					}
					else
					{
						success = true; // always succeed when appraisal is maxed out
					}
					if ( success )
					{
						tempItem->identified = true;
						tempItem->notifyIcon = true;
						messagePlayer(PLAYER_NUM, MESSAGE_INVENTORY, Language::get(570), tempItem->description());
						if ( tempItem->type == GEM_GLASS )
						{
							steamStatisticUpdate(STEAM_STAT_RHINESTONE_COWBOY, STEAM_STAT_INT, 1);
						}
					}
					else
					{
						messagePlayer(PLAYER_NUM, MESSAGE_INVENTORY, Language::get(3240), tempItem->description());
					}

					//Attempt a level up.
					if ( tempItem && items[tempItem->type].value > 0 && stats[PLAYER_NUM] )
					{
						if ( tempItem->identified )
						{
							int appraisalEaseOfDifficulty = 0;
							if ( items[tempItem->type].value < 100 )
							{
								// easy junk items
								appraisalEaseOfDifficulty = 2;
							}
							else if ( items[tempItem->type].value < 200 )
							{
								// medium
								appraisalEaseOfDifficulty = 1;
							}
							else if ( items[tempItem->type].value < 300 )
							{
								// medium
								appraisalEaseOfDifficulty = 0;
							}
							else if ( items[tempItem->type].value < 400 )
							{
								// hardest
								appraisalEaseOfDifficulty = -1;
							}
							else
							{
								// hardest
								appraisalEaseOfDifficulty = -1;
							}
							appraisalEaseOfDifficulty += stats[PLAYER_NUM]->PROFICIENCIES[PRO_APPRAISAL] / 20;
							// difficulty ranges from 1-in-1 to 1-in-6
							appraisalEaseOfDifficulty = std::max(appraisalEaseOfDifficulty, 1);
							//messagePlayer(0, "Appraisal level up chance: 1 in %d", appraisalEaseOfDifficulty);
							if ( local_rng.rand() % appraisalEaseOfDifficulty == 0 )
							{
								my->increaseSkill(PRO_APPRAISAL);
							}
						}
						else if ( local_rng.rand() % 7 == 0 )
						{
							my->increaseSkill(PRO_APPRAISAL);
						}
					}

					if ( success )
					{
						// update inventory by trying to stack the newly identified item.
						std::unordered_set<Uint32> appearancesOfSimilarItems;
						for ( node = stats[PLAYER_NUM]->inventory.first; node != NULL; node = node->next )
						{
							Item* item2 = (Item*)node->element;
							if ( item2 && item2 != tempItem && !itemCompare(tempItem, item2, false) )
							{
								if ( (itemTypeIsQuiver(item2->type) && (tempItem->count + item2->count) >= QUIVER_MAX_AMMO_QTY)
									|| ((item2->type == TOOL_MAGIC_SCRAP || item2->type == TOOL_METAL_SCRAP)
										&& (tempItem->count + item2->count) >= SCRAP_MAX_STACK_QTY) )
								{
									int maxStack = QUIVER_MAX_AMMO_QTY;
									if ( item2->type == TOOL_MAGIC_SCRAP || item2->type == TOOL_METAL_SCRAP )
									{
										maxStack = SCRAP_MAX_STACK_QTY;
									}

									// too many, split off into a new stack with reduced qty.
									if ( tempItem->count >= (maxStack - 1) )
									{
										// identified item is at max count so don't stack, abort.
										if ( tempItem->appearance == item2->appearance )
										{
											// items are the same (incl. appearance!)
											// if they shouldn't stack, we need to change appearance of the new item.
											appearancesOfSimilarItems.insert(item2->appearance);
										}
										continue;
									}
									if ( itemIsEquipped(tempItem, PLAYER_NUM) && players[PLAYER_NUM]->paperDoll.isItemOnDoll(*tempItem) )
									{
										// don't try to move our equipped item - it's an edge case to crash
										if ( tempItem->appearance == item2->appearance )
										{
											// items are the same (incl. appearance!)
											// if they shouldn't stack, we need to change appearance of the new item.
											appearancesOfSimilarItems.insert(item2->appearance);
										}
										continue;
									}
									if ( item2->count >= (maxStack - 1) )
									{
										// if we're at max count then skip this check.

										if ( tempItem->appearance == item2->appearance )
										{
											// items are the same (incl. appearance!)
											// if they shouldn't stack, we need to change appearance of the new item.
											appearancesOfSimilarItems.insert(item2->appearance);
										}
										continue;
									}
									int total = tempItem->count + item2->count;
									item2->count = maxStack - 1;
									tempItem->count = total - item2->count;
									if ( multiplayer == CLIENT && itemIsEquipped(item2, PLAYER_NUM) )
									{
										// if incrementing qty and holding item, then send "equip" for server to update their count of your held item.
										strcpy((char*)net_packet->data, "EQUS");
										SDLNet_Write32((Uint32)item2->type, &net_packet->data[4]);
										SDLNet_Write32((Uint32)item2->status, &net_packet->data[8]);
										SDLNet_Write32((Uint32)item2->beatitude, &net_packet->data[12]);
										SDLNet_Write32((Uint32)item2->count, &net_packet->data[16]);
										SDLNet_Write32((Uint32)item2->appearance, &net_packet->data[20]);
										net_packet->data[24] = item2->identified;
										net_packet->data[25] = PLAYER_NUM;
										net_packet->address.host = net_server.host;
										net_packet->address.port = net_server.port;
										net_packet->len = 27;
										sendPacketSafe(net_sock, -1, net_packet, 0);
									}
									if ( tempItem->count <= 0 )
									{
										if ( tempItem->node )
										{
											list_RemoveNode(tempItem->node);
											tempItem = nullptr;
										}
										else
										{
											free(tempItem);
											tempItem = nullptr;
										}
										break;
									}
									else
									{
										// we have to search other items to stack with, otherwise this search ends after 1 full stack.
										if ( tempItem->appearance == item2->appearance )
										{
											// items are the same (incl. appearance!)
											// if they shouldn't stack, we need to change appearance of the new item.
											appearancesOfSimilarItems.insert(item2->appearance);
										}
										continue;
									}
								}
								// if items are the same, check to see if they should stack
								else if ( item2->shouldItemStack(PLAYER_NUM) )
								{
									if ( itemIsEquipped(tempItem, PLAYER_NUM) && players[PLAYER_NUM]->paperDoll.isItemOnDoll(*tempItem) )
									{
										// don't try to move our equipped item - it's an edge case to crash
										if ( tempItem->appearance == item2->appearance )
										{
											// items are the same (incl. appearance!)
											// if they shouldn't stack, we need to change appearance of the new item.
											appearancesOfSimilarItems.insert(item2->appearance);
										}
										continue;
									}
									item2->count += tempItem->count;
									if ( multiplayer == CLIENT && itemIsEquipped(item2, PLAYER_NUM) )
									{
										// if incrementing qty and holding item, then send "equip" for server to update their count of your held item.
										Item** slot = itemSlot(stats[PLAYER_NUM], item2);
										if ( slot )
										{
											if ( slot == &stats[PLAYER_NUM]->weapon )
											{
												clientSendEquipUpdateToServer(EQUIP_ITEM_SLOT_WEAPON, EQUIP_ITEM_SUCCESS_UPDATE_QTY, PLAYER_NUM,
													item2->type, item2->status, item2->beatitude, item2->count, item2->appearance, item2->identified);
											}
											else if ( slot == &stats[PLAYER_NUM]->shield )
											{
												clientSendEquipUpdateToServer(EQUIP_ITEM_SLOT_SHIELD, EQUIP_ITEM_SUCCESS_UPDATE_QTY, PLAYER_NUM,
													item2->type, item2->status, item2->beatitude, item2->count, item2->appearance, item2->identified);
											}
										}
									}
									if ( tempItem->node )
									{
										list_RemoveNode(tempItem->node);
										tempItem = nullptr;
									}
									else
									{
										free(tempItem);
										tempItem = nullptr;
									}
									break;
								}
								else if ( !itemCompare(tempItem, item2, true) )
								{
									// items are the same (incl. appearance!)
									// if they shouldn't stack, we need to change appearance of the new item.
									appearancesOfSimilarItems.insert(item2->appearance);
								}
							}
						}
						if ( tempItem )
						{
							if ( !appearancesOfSimilarItems.empty() )
							{
								Uint32 originalAppearance = tempItem->appearance;
								int originalVariation = originalAppearance % items[tempItem->type].variations;

								int tries = 100;
								bool robot = false;
								// we need to find a unique appearance within the list.
								if ( tempItem->type == TOOL_SENTRYBOT || tempItem->type == TOOL_SPELLBOT || tempItem->type == TOOL_GYROBOT
									|| tempItem->type == TOOL_DUMMYBOT )
								{
									robot = true;
									tempItem->appearance += (local_rng.rand() % 100000) * 10;
								}
								else
								{
									tempItem->appearance = local_rng.rand();
									if ( tempItem->appearance % items[tempItem->type].variations != originalVariation )
									{
										// we need to match the variation for the new appearance, take the difference so new varation matches
										int change = (tempItem->appearance % items[tempItem->type].variations - originalVariation);
										if ( tempItem->appearance < change ) // underflow protection
										{
											tempItem->appearance += items[tempItem->type].variations;
										}
										tempItem->appearance -= change;
										int newVariation = tempItem->appearance % items[tempItem->type].variations;
										assert(newVariation == originalVariation);
									}
								}
								auto it = appearancesOfSimilarItems.find(tempItem->appearance);
								while ( it != appearancesOfSimilarItems.end() && tries > 0 )
								{
									if ( robot )
									{
										tempItem->appearance += (local_rng.rand() % 100000) * 10;
									}
									else
									{
										tempItem->appearance = local_rng.rand();
										if ( tempItem->appearance % items[tempItem->type].variations != originalVariation )
										{
											// we need to match the variation for the new appearance, take the difference so new varation matches
											int change = (tempItem->appearance % items[tempItem->type].variations - originalVariation);
											if ( tempItem->appearance < change ) // underflow protection
											{
												tempItem->appearance += items[tempItem->type].variations;
											}
											tempItem->appearance -= change;
											int newVariation = tempItem->appearance % items[tempItem->type].variations;
											assert(newVariation == originalVariation);
										}
									}
									it = appearancesOfSimilarItems.find(tempItem->appearance);
									--tries;
								}

							}
							if ( multiplayer == CLIENT && itemIsEquipped(tempItem, PLAYER_NUM) && players[PLAYER_NUM]->paperDoll.isItemOnDoll(*tempItem) )
							{
								clientSendAppearanceUpdateToServer(PLAYER_NUM, tempItem, true);
							}
						}
					}
					players[PLAYER_NUM]->inventoryUI.appraisal.old_item = players[PLAYER_NUM]->inventoryUI.appraisal.current_item;
					players[PLAYER_NUM]->inventoryUI.appraisal.current_item = 0;
				}
			}
		}
		else
		{
			players[PLAYER_NUM]->inventoryUI.appraisal.timer = 0;
			players[PLAYER_NUM]->inventoryUI.appraisal.current_item = 0;
		}
	}

	// remove broken equipment
	if ( stats[PLAYER_NUM]->helmet != NULL )
		if ( stats[PLAYER_NUM]->helmet->status == BROKEN )
		{
			stats[PLAYER_NUM]->helmet = NULL;
		}
	if ( stats[PLAYER_NUM]->breastplate != NULL )
		if ( stats[PLAYER_NUM]->breastplate->status == BROKEN )
		{
			stats[PLAYER_NUM]->breastplate = NULL;
		}
	if ( stats[PLAYER_NUM]->gloves != NULL )
		if ( stats[PLAYER_NUM]->gloves->status == BROKEN )
		{
			stats[PLAYER_NUM]->gloves = NULL;
		}
	if ( stats[PLAYER_NUM]->shoes != NULL )
		if ( stats[PLAYER_NUM]->shoes->status == BROKEN )
		{
			stats[PLAYER_NUM]->shoes = NULL;
		}
	if ( stats[PLAYER_NUM]->shield != NULL )
		if ( stats[PLAYER_NUM]->shield->status == BROKEN )
		{
			stats[PLAYER_NUM]->shield = NULL;
		}
	if ( stats[PLAYER_NUM]->weapon != NULL )
		if ( stats[PLAYER_NUM]->weapon->status == BROKEN )
		{
			stats[PLAYER_NUM]->weapon = NULL;
		}
	if ( stats[PLAYER_NUM]->cloak != NULL )
		if ( stats[PLAYER_NUM]->cloak->status == BROKEN )
		{
			stats[PLAYER_NUM]->cloak = NULL;
		}
	if ( stats[PLAYER_NUM]->amulet != NULL )
		if ( stats[PLAYER_NUM]->amulet->status == BROKEN )
		{
			stats[PLAYER_NUM]->amulet = NULL;
		}
	if ( stats[PLAYER_NUM]->ring != NULL )
		if ( stats[PLAYER_NUM]->ring->status == BROKEN )
		{
			stats[PLAYER_NUM]->ring = NULL;
		}
	if ( stats[PLAYER_NUM]->mask != NULL )
		if ( stats[PLAYER_NUM]->mask->status == BROKEN )
		{
			stats[PLAYER_NUM]->mask = NULL;
		}

	if ( multiplayer != CLIENT )
	{
		my->effectTimes();
	}

	bool freezeLimbMovements = false;
	if ( StatueManager.editingPlayerUid > 0 
		&& uidToEntity(StatueManager.editingPlayerUid) == players[PLAYER_NUM]->entity )
	{
		freezeLimbMovements = true;
	}

	// invisibility
	if ( !intro )
	{
		if ( players[PLAYER_NUM]->isLocalPlayer() || multiplayer == SERVER || StatueManager.activeEditing )
		{
			if ( my->isInvisible() && !freezeLimbMovements )
			{
				if ( !my->flags[INVISIBLE] )
				{
					my->flags[INVISIBLE] = true;
					serverUpdateEntityFlag(my, INVISIBLE);
				}
				my->flags[BLOCKSIGHT] = false;
				if ( multiplayer != CLIENT )
				{
					for ( i = 0, node = my->children.first; node != NULL; node = node->next, ++i )
					{
						if ( i == 0 )
						{
							continue;
						}
						if ( i >= 6 )
						{
							break;
						}
						entity = (Entity*)node->element;
						if ( !entity->flags[INVISIBLE] )
						{
							entity->flags[INVISIBLE] = true;
							serverUpdateEntityBodypart(my, i);
						}
					}
				}
			}
			else
			{
				if ( my->flags[INVISIBLE] )
				{
					my->flags[INVISIBLE] = false;
					serverUpdateEntityFlag(my, INVISIBLE);
				}
				my->flags[BLOCKSIGHT] = true;
				if ( multiplayer != CLIENT )
				{
					for (i = 0, node = my->children.first; node != NULL; node = node->next, i++)
					{
						if ( i == 0 )
						{
							continue;
						}
						if ( i >= 6 )
						{
							break;
						}
						entity = (Entity*)node->element;
						if ( entity->flags[INVISIBLE] )
						{
							if ( stats[PLAYER_NUM]->type == RAT )
							{
								if ( i == 1 )
								{
									// only unhide the body.
									if ( entity->flags[INVISIBLE] )
									{
										entity->flags[INVISIBLE] = false;
										serverUpdateEntityBodypart(my, i);
									}
								}
							}
							else if ( stats[PLAYER_NUM]->type == SPIDER )
							{
								// do nothing, these limbs are invisible for the spider so don't unhide.
							}
							else
							{
								if ( entity->flags[INVISIBLE] )
								{
									entity->flags[INVISIBLE] = false;
									serverUpdateEntityBodypart(my, i);
								}
							}
						}
					}
				}
			}
		}
	}

	real_t zOffset = 0;
	bool oldInsectoidLevitate = players[PLAYER_NUM]->movement.insectoidLevitating;
	bool& insectoidLevitating = players[PLAYER_NUM]->movement.insectoidLevitating;
	insectoidLevitating = false;

	if ( players[PLAYER_NUM]->isLocalPlayer() || multiplayer == SERVER )
	{
		switch ( stats[PLAYER_NUM]->type )
		{
			case RAT:
				zOffset = 6;
				break;
			case TROLL:
				zOffset = -1.5;
				break;
			case SPIDER:
				zOffset = 4.5;
				break;
			case CREATURE_IMP:
				zOffset = -3.5;
				break;
			default:
				break;
		}
		bool prevlevitating = false;
		if ( multiplayer != CLIENT )
		{
			if ( abs(zOffset) <= 0.05 )
			{
				if ( (my->z >= -2.05 && my->z <= -1.95 ) || (my->z >= -1.55 && my->z <= -1.45) )
				{
					prevlevitating = true;
				}
			}
			else
			{
				if( (my->z >= (zOffset - 1.05)) && (my->z <= (zOffset - 0.95))
					|| (my->z >= (zOffset - .55)) && (my->z <= (zOffset - .45)) )
				{
					prevlevitating = true;
				}
			}
		}

		// sleeping
		if ( stats[PLAYER_NUM]->EFFECTS[EFF_ASLEEP] )
		{
			switch ( playerRace )
			{
				case GOBLIN:
				case GOATMAN:
				case INSECTOID:
					my->z = 2.5;
					break;
				case SKELETON:
				case AUTOMATON:
					my->z = 2.f;
					break;
				case HUMAN:
				case VAMPIRE:
				case INCUBUS:
				case SUCCUBUS:
				case TROLL:
					my->z = 1.5;
					break;
				default:
					my->z = 1.5;
					break;
			}
			my->pitch = PI / 4;
		}
		else if ( !noclip || (noclip && intro) )
		{
			my->z = -1;
			if ( abs(zOffset) >= 0.05 )
			{
				my->z = zOffset;
			}
			if ( intro )
			{
				my->pitch = 0;
			}
		}

		// levitation
		levitating = isLevitating(stats[PLAYER_NUM]);

		if ( levitating )
		{
			my->z -= 1; // floating
			insectoidLevitating = (playerRace == INSECTOID && stats[PLAYER_NUM]->EFFECTS[EFF_FLUTTER]);
		}

		if ( !levitating && prevlevitating )
		{
			int x, y, u, v;
			x = std::min(std::max<unsigned int>(1, my->x / 16), map.width - 2);
			y = std::min(std::max<unsigned int>(1, my->y / 16), map.height - 2);
			std::vector<std::pair<std::pair<int, int>, real_t>> safeTiles; // pair of coordinates, with a distance to player.
			std::vector<std::pair<int, int>> deathTiles;
			bool checkSafeTiles = true;
			for ( u = x - 1; u <= x + 1; u++ )
			{
				for ( v = y - 1; v <= y + 1; v++ )
				{
					if ( entityInsideTile(my, u, v, 0, !checkSafeTiles) )
					{
						// intersecting with no floor, we might die.
						deathTiles.push_back(std::make_pair(u, v));
					}
					else if ( entityInsideTile(my, u, v, 0, checkSafeTiles) )
					{
						// intersecting with a safe floor, we might live.
						if ( barony_clear(u * 16 + 8, v * 16 + 8, my) )
						{
							// tile is clear from obstacles.
							real_t dx = my->x - u * 16 + 8;
							real_t dy = my->y - v * 16 + 8;
							real_t dist = sqrt(dx * dx + dy * dy);
							safeTiles.push_back(std::make_pair(std::make_pair(u, v), dist));
						}
					}
				}
			}

			if ( !deathTiles.empty() )
			{
				if ( !safeTiles.empty() )
				{
					// we might be able to warp ourselves to a safe spot.
					real_t lowestDist = 1000.0;
					int lowestIndex = -1;
					for ( auto it = safeTiles.begin(); it != safeTiles.end(); ++it )
					{
						if ( (*it).second < lowestDist )
						{
							lowestIndex = it - safeTiles.begin();
						}
					}
					if ( lowestIndex >= 0 )
					{
						int newx = safeTiles.at(lowestIndex).first.first;
						int newy = safeTiles.at(lowestIndex).first.second;
						real_t velx = newx * 16 + 8 - my->x;
						real_t vely = newy * 16 + 8 - my->y;
						// try moving in one direction only.
						if ( newy == y ) // keep y the same
						{
							if ( newx != x )
							{
								if ( barony_clear(newx * 16 + 8, y, my) )
								{
									vely = 0.0;
								}
							}
						}
						else if ( newx == x ) // keep x the same
						{
							if ( newy != y )
							{
								if ( barony_clear(x, newy * 16 + 8, my) )
								{
									velx = 0.0;
								}
							}
						}
						clipMove(&my->x, &my->y, velx, vely, my);
						messagePlayer(PLAYER_NUM, MESSAGE_HINT, Language::get(3869));
						if ( players[PLAYER_NUM]->isLocalPlayer() )
						{
							cameravars[PLAYER_NUM].shakex += .1;
							cameravars[PLAYER_NUM].shakey += 10;
						}
						else if ( !players[PLAYER_NUM]->isLocalPlayer() && multiplayer == SERVER )
						{
							if ( PLAYER_NUM > 0 )
							{
								strcpy((char*)net_packet->data, "SHAK");
								net_packet->data[4] = 10; // turns into .1
								net_packet->data[5] = 10;
								net_packet->address.host = net_clients[PLAYER_NUM - 1].host;
								net_packet->address.port = net_clients[PLAYER_NUM - 1].port;
								net_packet->len = 6;
								sendPacketSafe(net_sock, -1, net_packet, PLAYER_NUM - 1);
							}
						}
					}
					else
					{
						my->setObituary(Language::get(3010)); // fell to their death.
						stats[PLAYER_NUM]->killer = KilledBy::BOTTOMLESS_PIT;
						stats[PLAYER_NUM]->HP = 0; // kill me instantly
						if (stats[PLAYER_NUM]->type == AUTOMATON)
						{
							my->playerAutomatonDeathCounter = TICKS_PER_SECOND * 5; // set the death timer to immediately pop for players.
						}
					}
				}
				else if ( safeTiles.empty() )
				{
					my->setObituary(Language::get(3010)); // fell to their death.
					stats[PLAYER_NUM]->killer = KilledBy::BOTTOMLESS_PIT;
					stats[PLAYER_NUM]->HP = 0; // kill me instantly
					if (stats[PLAYER_NUM]->type == AUTOMATON)
					{
						my->playerAutomatonDeathCounter = TICKS_PER_SECOND * 5; // set the death timer to immediately pop for players.
					}
				}
			}

		}
	}
	else
	{
		if ( playerRace == INSECTOID && stats[PLAYER_NUM]->EFFECTS[EFF_FLUTTER] && (my->z >= -2.05 && my->z <= -1.95) )
		{
			insectoidLevitating = true;
		}
	}

	// swimming
	bool waterwalkingboots = false;
	if ( stats[PLAYER_NUM]->shoes != NULL )
	{
		if ( stats[PLAYER_NUM]->shoes->type == IRON_BOOTS_WATERWALKING )
		{
			waterwalkingboots = true;
		}
	}
	bool swimming = players[PLAYER_NUM]->movement.isPlayerSwimming();
	if ( players[PLAYER_NUM]->isLocalPlayer() || multiplayer == SERVER )
	{
		if ( swimming )
		{
			int x = std::min(std::max<unsigned int>(0, floor(my->x / 16)), map.width - 1);
			int y = std::min(std::max<unsigned int>(0, floor(my->y / 16)), map.height - 1);
			if ( local_rng.rand() % 400 == 0 && multiplayer != CLIENT )
			{
				my->increaseSkill(PRO_SWIMMING);
			}
			my->z = 7;
			if ( playerRace == SPIDER || playerRace == RAT )
			{
				my->z += 1;
			}
			if ( !PLAYER_INWATER && (players[PLAYER_NUM]->isLocalPlayer()) )
			{
				PLAYER_INWATER = 1;
				if ( lavatiles[map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height]] )
				{
					messagePlayer(PLAYER_NUM, MESSAGE_STATUS, Language::get(573));
					if ( stats[PLAYER_NUM]->type == AUTOMATON )
					{
						messagePlayer(PLAYER_NUM, MESSAGE_STATUS, Language::get(3703));
					}
					cameravars[PLAYER_NUM].shakex += .1;
					cameravars[PLAYER_NUM].shakey += 10;
				}
				else if ( swimmingtiles[map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height]] && stats[PLAYER_NUM]->type == VAMPIRE )
				{
					messagePlayerColor(PLAYER_NUM, MESSAGE_STATUS, makeColorRGB(255, 0, 0), Language::get(3183));
					playSoundPlayer(PLAYER_NUM, 28, 128);
					playSoundPlayer(PLAYER_NUM, 249, 128);
					cameravars[PLAYER_NUM].shakex += .1;
					cameravars[PLAYER_NUM].shakey += 10;
				}
				else if ( swimmingtiles[map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height]] && stats[PLAYER_NUM]->type == AUTOMATON )
				{
					messagePlayer(PLAYER_NUM, MESSAGE_STATUS, Language::get(3702));
					playSound(136, 128);
				}
				else if ( swimmingtiles[map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height]] )
				{
					playSound(136, 128);
				}
			}
			if ( multiplayer != CLIENT )
			{
				// Check if the Player is in Water or Lava
				if ( swimmingtiles[map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height]] )
				{
					if ( my->flags[BURNING] )
					{
						my->flags[BURNING] = false;
						messagePlayer(PLAYER_NUM, MESSAGE_STATUS, Language::get(574)); // "The water extinguishes the flames!"
						serverUpdateEntityFlag(my, BURNING);
					}
					if ( stats[PLAYER_NUM]->EFFECTS[EFF_POLYMORPH] )
					{
						if ( stats[PLAYER_NUM]->EFFECTS[EFF_POLYMORPH] )
						{
							my->setEffect(EFF_POLYMORPH, false, 0, true);
							my->effectPolymorph = 0;
							serverUpdateEntitySkill(my, 50);

							messagePlayer(PLAYER_NUM, MESSAGE_STATUS, Language::get(3192));
							if ( !stats[PLAYER_NUM]->EFFECTS[EFF_SHAPESHIFT] )
							{
								messagePlayer(PLAYER_NUM, MESSAGE_STATUS, Language::get(3185));
							}
							else
							{
								messagePlayer(PLAYER_NUM, MESSAGE_STATUS, Language::get(4303));  // wears out, no mention of 'normal' form
							}
						}
						playSoundEntity(my, 400, 92);
						createParticleDropRising(my, 593, 1.f);
						serverSpawnMiscParticles(my, PARTICLE_EFFECT_RISING_DROP, 593);
					}
					if ( stats[PLAYER_NUM]->type == VAMPIRE )
					{
						if ( ticks % 10 == 0 ) // Water deals damage every 10 ticks
						{
							my->modHP(-2 - local_rng.rand() % 2);
							if ( ticks % 20 == 0 )
							{
								playSoundPlayer(PLAYER_NUM, 28, 92);
							}
							my->setObituary(Language::get(3254)); // "goes for a swim in some water."
						    stats[PLAYER_NUM]->killer = KilledBy::WATER;
							steamAchievementClient(PLAYER_NUM, "BARONY_ACH_BLOOD_BOIL");
						}
					}
					else if ( stats[PLAYER_NUM]->type == AUTOMATON )
					{
						if ( ticks % 10 == 0 ) // Water deals heat damage every 10 ticks
						{
							my->safeConsumeMP(2);
						}
						if ( ticks % 50 == 0 )
						{
							messagePlayer(PLAYER_NUM, MESSAGE_STATUS, Language::get(3702));
							stats[PLAYER_NUM]->HUNGER -= 25;
							serverUpdateHunger(PLAYER_NUM);
						}
					}
				}
				else if ( ticks % 10 == 0 ) // Lava deals damage every 10 ticks
				{
					my->modHP(-2 - local_rng.rand() % 2);
					if ( stats[PLAYER_NUM]->type == AUTOMATON )
					{
						my->modMP(2);
						if ( ticks % 50 == 0 )
						{
							messagePlayer(PLAYER_NUM, MESSAGE_STATUS, Language::get(3703));
							stats[PLAYER_NUM]->HUNGER += 50;
							serverUpdateHunger(PLAYER_NUM);
						}
					}
					my->setObituary(Language::get(1506)); // "goes for a swim in some lava."
					stats[PLAYER_NUM]->killer = KilledBy::LAVA;
					if ( !my->flags[BURNING] )
					{
						// Attempt to set the Entity on fire
						my->SetEntityOnFire();
					}
					if ( stats[PLAYER_NUM]->type == AUTOMATON || stats[PLAYER_NUM]->type == SKELETON )
					{
						if ( ticks % 20 == 0 )
						{
							playSoundPlayer(PLAYER_NUM, 28, 92);
						}
					}
				}
			}
		}
	}

	if ( !swimming )
	{
		if ( PLAYER_INWATER != 0 )
		{
			PLAYER_INWATER = 0;
			PLAYER_BOBMOVE = 0;
			PLAYER_BOB = 0;
			PLAYER_BOBMODE = 0;
		}
	}

	if ( players[PLAYER_NUM]->isLocalPlayer() )
	{
		players[PLAYER_NUM]->entity = my;

		if ( !usecamerasmoothing )
		{
			players[PLAYER_NUM]->movement.handlePlayerCameraBobbing(false);
		}

		Sint32 mouseX = inputs.getMouse(PLAYER_NUM, Inputs::OX);
		Sint32 mouseY = inputs.getMouse(PLAYER_NUM, Inputs::OY);

		bool shootmode = players[PLAYER_NUM]->shootmode;
		FollowerRadialMenu& followerMenu = FollowerMenu[PLAYER_NUM];

		// object interaction
		if ( intro == false )
		{
			clickDescription(PLAYER_NUM, NULL); // inspecting objects
			doStatueEditor(PLAYER_NUM);

			if ( followerMenu.optionSelected == ALLY_CMD_ATTACK_SELECT )
			{
				Entity* underMouse = nullptr;
				if ( followerMenu.optionSelected == ALLY_CMD_ATTACK_SELECT && ticks % 10 == 0 )
				{
					if ( !players[PLAYER_NUM]->worldUI.isEnabled() )
					{
						if ( !shootmode )
						{
							Uint32 uidnum = GO_GetPixelU32(mouseX, yres - mouseY, cameras[PLAYER_NUM]);
							if ( uidnum > 0 )
							{
								underMouse = uidToEntity(uidnum);
							}
						}
						else
						{
							Uint32 uidnum = GO_GetPixelU32(cameras[PLAYER_NUM].winw / 2, yres - cameras[PLAYER_NUM].winh / 2, cameras[PLAYER_NUM]);
							if ( uidnum > 0 )
							{
								underMouse = uidToEntity(uidnum);
							}
						}
						if ( underMouse && followerMenu.followerToCommand )
						{
							Entity* parent = uidToEntity(underMouse->skill[2]);
							if ( underMouse->behavior == &actMonster || (parent && parent->behavior == &actMonster) )
							{
								// see if we selected a limb
								if ( parent )
								{
									underMouse = parent;
								}
							}
							followerMenu.allowedInteractEntity(*underMouse);
						}
						else
						{
							strcpy(followerMenu.interactText, "");
						}
					}
					else
					{
						if ( !players[PLAYER_NUM]->worldUI.bTooltipInView )
						{
							strcpy(followerMenu.interactText, "");
						}
					}
				}
			}

			Input& input = Input::inputs[PLAYER_NUM];

			if ( followerMenu.followerToCommand == nullptr && followerMenu.selectMoveTo == false )
			{
				bool clickedOnGUI = false;

				EntityClickType clickType = ENTITY_CLICK_USE;
				bool tempDisableWorldUI = false;
				bool skipUse = false;
				if ( players[PLAYER_NUM]->worldUI.isEnabled() )
				{
					if ( !players[PLAYER_NUM]->shootmode && input.input("Use").isBindingUsingGamepad() )
					{
						skipUse = true;
					}
					else if ( !players[PLAYER_NUM]->shootmode && inputs.bPlayerUsingKeyboardControl(PLAYER_NUM) )
					{
						tempDisableWorldUI = true;
					}
					else if ( (!players[PLAYER_NUM]->shootmode)
						|| (players[PLAYER_NUM]->hotbar.useHotbarFaceMenu
							&& (players[PLAYER_NUM]->hotbar.faceMenuButtonHeld != Player::Hotbar_t::GROUP_NONE)) )
					{
						skipUse = true;
					}
				}
				else if ( !players[PLAYER_NUM]->worldUI.isEnabled() && input.input("Use").isBindingUsingGamepad()
					&& !players[PLAYER_NUM]->shootmode )
				{
					skipUse = true;
				}

				if ( !skipUse )
				{
					if ( tempDisableWorldUI )
					{
						players[PLAYER_NUM]->worldUI.disable();
					}
					if ( players[PLAYER_NUM]->worldUI.isEnabled() )
					{
						clickType = ENTITY_CLICK_USE_TOOLTIPS_ONLY;
						Entity* activeTooltipEntity = uidToEntity(players[PLAYER_NUM]->worldUI.uidForActiveTooltip);
						if ( activeTooltipEntity && activeTooltipEntity->bEntityTooltipRequiresButtonHeld() )
						{
							clickType = ENTITY_CLICK_HELD_USE_TOOLTIPS_ONLY;
						}
					}

					selectedEntity[PLAYER_NUM] = entityClicked(&clickedOnGUI, false, PLAYER_NUM, clickType); // using objects
					if ( !selectedEntity[PLAYER_NUM] && !clickedOnGUI )
					{
						if ( clickType == ENTITY_CLICK_USE )
						{
							// otherwise if we hold right click we'll keep trying this function, FPS will drop.
							if ( input.binary("Use") )
							{
								++players[PLAYER_NUM]->movement.selectedEntityGimpTimer;
							}
						}
					}

					if ( tempDisableWorldUI )
					{
						players[PLAYER_NUM]->worldUI.enable();
					}
				}
				else
				{
					input.consumeBinaryToggle("Use");
					//input.consumeBindingsSharedWithBinding("Use");
					selectedEntity[PLAYER_NUM] = nullptr;
				}
			}
			else
			{
				selectedEntity[PLAYER_NUM] = NULL;

				if ( !players[PLAYER_NUM]->usingCommand() && players[PLAYER_NUM]->bControlEnabled && !gamePaused && input.binaryToggle("Use") )
				{
					if ( !followerMenu.menuToggleClick && followerMenu.selectMoveTo )
					{
						if ( followerMenu.optionSelected == ALLY_CMD_MOVETO_SELECT )
						{
							// we're selecting a point for the ally to move to.
							input.consumeBinaryToggle("Use");
							//input.consumeBindingsSharedWithBinding("Use");

							// we're selecting a point for the ally to move to.
							if ( players[PLAYER_NUM] && players[PLAYER_NUM]->entity )
							{
								real_t startx = cameras[PLAYER_NUM].x * 16.0;
								real_t starty = cameras[PLAYER_NUM].y * 16.0;
								static ConsoleVariable<float> cvar_followerStartZ("/follower_start_z", -2.5);
								real_t startz = cameras[PLAYER_NUM].z + (4.5 - cameras[PLAYER_NUM].z) / 2.0 + *cvar_followerStartZ;
								real_t pitch = cameras[PLAYER_NUM].vang;
								if ( pitch < 0 || pitch > PI )
								{
									pitch = 0;
								}

								static ConsoleVariable<float> cvar_followerMoveTo("/follower_moveto_z", 0.1);
								static ConsoleVariable<float> cvar_followerStartZLimit("/follower_start_z_limit", 7.5);
								// draw line from the players height and direction until we hit the ground.
								real_t previousx = startx;
								real_t previousy = starty;
								int index = 0;
								const real_t yaw = cameras[PLAYER_NUM].ang;
								for ( ; startz < *cvar_followerStartZLimit; startz += abs((*cvar_followerMoveTo) * tan(pitch)) )
								{
									startx += 0.1 * cos(yaw);
									starty += 0.1 * sin(yaw);
									const int index_x = static_cast<int>(startx) >> 4;
									const int index_y = static_cast<int>(starty) >> 4;
									index = (index_y)* MAPLAYERS + (index_x)* MAPLAYERS * map.height;
									if ( map.tiles[index] && !map.tiles[OBSTACLELAYER + index] )
									{
										// store the last known good coordinate
										previousx = startx;// + 16 * cos(yaw);
										previousy = starty;// + 16 * sin(yaw);
									}
									if ( map.tiles[OBSTACLELAYER + index] )
									{
										break;
									}
								}

								createParticleFollowerCommand(previousx, previousy, 0, FOLLOWER_TARGET_PARTICLE, 0);
								followerMenu.optionSelected = ALLY_CMD_MOVETO_CONFIRM;
								followerMenu.selectMoveTo = false;
								followerMenu.moveToX = static_cast<int>(previousx) / 16;
								followerMenu.moveToY = static_cast<int>(previousy) / 16;
								//messagePlayer(PLAYER_NUM, "%d, %d, pitch: %f", followerMoveToX, followerMoveToY, players[PLAYER_NUM]->entity->pitch);
							}
						}
						else if ( followerMenu.optionSelected == ALLY_CMD_ATTACK_SELECT )
						{
							// we're selecting a target for the ally.
							Entity* target = entityClicked(nullptr, false, PLAYER_NUM, EntityClickType::ENTITY_CLICK_FOLLOWER_INTERACT);
							input.consumeBinaryToggle("Use");
							//input.consumeBindingsSharedWithBinding("Use");
							if ( target && followerMenu.followerToCommand )
							{
								Entity* parent = uidToEntity(target->skill[2]);
								if ( target->behavior == &actMonster || (parent && parent->behavior == &actMonster) )
								{
									// see if we selected a limb
									if ( parent )
									{
										target = parent;
									}
								}
								else if ( target->sprite == 184 || target->sprite == 585 ) // switch base.
								{
									parent = uidToEntity(target->parent);
									if ( parent )
									{
										target = parent;
									}
								}
								if ( followerMenu.allowedInteractEntity(*target) )
								{
									createParticleFollowerCommand(target->x, target->y, 0, FOLLOWER_TARGET_PARTICLE,
										target->getUID());
									followerMenu.optionSelected = ALLY_CMD_ATTACK_CONFIRM;
									followerMenu.followerToCommand->monsterAllyInteractTarget = target->getUID();
								}
								else
								{
									messagePlayer(PLAYER_NUM, MESSAGE_HINT, Language::get(3094));
									followerMenu.optionSelected = ALLY_CMD_CANCEL;
									followerMenu.optionPrevious = ALLY_CMD_ATTACK_CONFIRM;
									followerMenu.followerToCommand->monsterAllyInteractTarget = 0;
								}
							}
							else
							{
								followerMenu.optionSelected = ALLY_CMD_CANCEL;
								followerMenu.optionPrevious = ALLY_CMD_ATTACK_CONFIRM;
								followerMenu.followerToCommand->monsterAllyInteractTarget = 0;
							}

							if ( players[PLAYER_NUM]->worldUI.isEnabled() )
							{
								players[PLAYER_NUM]->worldUI.reset();
							}

							followerMenu.selectMoveTo = false;
							strcpy(followerMenu.interactText, "");
						}
					}
				}
			}

			if ( !players[PLAYER_NUM]->usingCommand() && players[PLAYER_NUM]->bControlEnabled
				&& !gamePaused
				&& !followerMenu.followerToCommand && followerMenu.recentEntity )
			{
				auto& b = (multiplayer != SINGLE && PLAYER_NUM != 0) ? Input::inputs[0].getBindings() : input.getBindings();
				bool showNPCCommandsOnGamepad = false;
				auto showNPCCommandsFind = b.find("Show NPC Commands");
				std::string showNPCCommandsInputStr = "";
				std::string lastNPCCommandInputStr = "";
				if ( showNPCCommandsFind != b.end() )
				{
					showNPCCommandsOnGamepad = (*showNPCCommandsFind).second.isBindingUsingGamepad();
					showNPCCommandsInputStr = (*showNPCCommandsFind).second.input;
				}
				bool lastNPCCommandOnGamepad = false;
				auto lastNPCCommandFind = b.find("Command NPC");
				if ( lastNPCCommandFind != b.end() )
				{
					lastNPCCommandOnGamepad = (*lastNPCCommandFind).second.isBindingUsingGamepad();
					lastNPCCommandInputStr = (*lastNPCCommandFind).second.input;
				}
				
				if ( players[PLAYER_NUM]->worldUI.bTooltipInView && players[PLAYER_NUM]->worldUI.tooltipsInRange.size() > 1 )
				{
					if ( showNPCCommandsOnGamepad &&
						(showNPCCommandsInputStr == input.binding("Interact Tooltip Next")
							|| showNPCCommandsInputStr == input.binding("Interact Tooltip Prev")) )
					{
						input.consumeBinaryToggle("Show NPC Commands");
						players[PLAYER_NUM]->hud.followerDisplay.bOpenFollowerMenuDisabled = true;
					}
					if ( lastNPCCommandOnGamepad &&
						(lastNPCCommandInputStr == input.binding("Interact Tooltip Next")
							|| lastNPCCommandInputStr == input.binding("Interact Tooltip Prev")) )
					{
						input.consumeBinaryToggle("Command NPC");
						players[PLAYER_NUM]->hud.followerDisplay.bCommandNPCDisabled = true;
					}
				}

				if ( (input.binaryToggle("Show NPC Commands") && !showNPCCommandsOnGamepad)
						|| (input.binaryToggle("Show NPC Commands") && showNPCCommandsOnGamepad 
							&& players[PLAYER_NUM]->shootmode /*&& !players[PLAYER_NUM]->worldUI.bTooltipInView*/) )
				{
					if ( players[PLAYER_NUM] && players[PLAYER_NUM]->entity
						&& followerMenu.recentEntity->monsterTarget == players[PLAYER_NUM]->entity->getUID() )
					{
						// your ally is angry at you!
					}
					else
					{
						selectedEntity[PLAYER_NUM] = followerMenu.recentEntity;
						followerMenu.holdWheel = true;
						if ( showNPCCommandsOnGamepad )
						{
							followerMenu.holdWheel = false;
						}
					}
				}
				else if ( (input.binaryToggle("Command NPC") && !lastNPCCommandOnGamepad)
					|| (input.binaryToggle("Command NPC") && lastNPCCommandOnGamepad && players[PLAYER_NUM]->shootmode/*&& !players[PLAYER_NUM]->worldUI.bTooltipInView*/) )
				{
					if ( players[PLAYER_NUM] && players[PLAYER_NUM]->entity
						&& followerMenu.recentEntity->monsterTarget == players[PLAYER_NUM]->entity->getUID() )
					{
						// your ally is angry at you!
						input.consumeBinaryToggle("Command NPC");
					}
					else if ( followerMenu.optionPrevious != -1 )
					{
						followerMenu.followerToCommand = followerMenu.recentEntity;
					}
					else
					{
						input.consumeBinaryToggle("Command NPC");
					}
				}
			}
			if ( selectedEntity[PLAYER_NUM] != NULL )
			{
				followerMenu.followerToCommand = nullptr;
				Entity* parent = uidToEntity(selectedEntity[PLAYER_NUM]->skill[2]);
				if ( selectedEntity[PLAYER_NUM]->behavior == &actMonster || (parent && parent->behavior == &actMonster) )
				{
					// see if we selected a follower to process right click menu.
					if ( parent && parent->monsterAllyIndex == PLAYER_NUM )
					{
						followerMenu.followerToCommand = parent;
						//messagePlayer(0, "limb");
					}
					else if ( selectedEntity[PLAYER_NUM]->monsterAllyIndex == PLAYER_NUM )
					{
						followerMenu.followerToCommand = selectedEntity[PLAYER_NUM];
						//messagePlayer(0, "head");
					}

					if ( followerMenu.followerToCommand )
					{
						if ( players[PLAYER_NUM] && players[PLAYER_NUM]->entity
							&& followerMenu.followerToCommand->monsterTarget == players[PLAYER_NUM]->entity->getUID() )
						{
							// your ally is angry at you!
							followerMenu.followerToCommand = nullptr;
							followerMenu.optionPrevious = -1;
						}
						else
						{
							followerMenu.recentEntity = followerMenu.followerToCommand;
							followerMenu.initfollowerMenuGUICursor(true);
							followerMenu.updateScrollPartySheet();
							selectedEntity[PLAYER_NUM] = NULL;
							Player::soundActivate();
						}
					}
				}
				if ( selectedEntity[PLAYER_NUM] )
				{
					input.consumeBinaryToggle("Use");
					//input.consumeBindingsSharedWithBinding("Use");
					bool foundTinkeringKit = false;
					if ( entityDist(my, selectedEntity[PLAYER_NUM]) <= TOUCHRANGE )
					{
						inrange[PLAYER_NUM] = true;

						if ( (selectedEntity[PLAYER_NUM]->behavior == &actItem
							|| selectedEntity[PLAYER_NUM]->behavior == &actTorch
							|| selectedEntity[PLAYER_NUM]->behavior == &actCrystalShard)
							&& stats[PLAYER_NUM] && stats[PLAYER_NUM]->shield && stats[PLAYER_NUM]->defending
							&& stats[PLAYER_NUM]->shield->type == TOOL_TINKERING_KIT )
						{
							foundTinkeringKit = true;
						}
						if ( foundTinkeringKit && (players[PLAYER_NUM]->isLocalPlayer()) )
						{
							selectedEntity[PLAYER_NUM]->itemAutoSalvageByPlayer = static_cast<Sint32>(players[PLAYER_NUM]->entity->getUID());
						}
					}
					else
					{
						inrange[PLAYER_NUM] = false;
					}
					if ( multiplayer == CLIENT )
					{
						if ( inrange[PLAYER_NUM] )
						{
							if ( foundTinkeringKit )
							{
								strcpy((char*)net_packet->data, "SALV");
							}
							else
							{
								strcpy((char*)net_packet->data, "CKIR");
								if ( stats[PLAYER_NUM]->type == RAT
									&& selectedEntity[PLAYER_NUM]->behavior == &actItem
									&& selectedEntity[PLAYER_NUM]->itemShowOnMap == 1 )
								{
									strcpy((char*)net_packet->data, "RATF");
								}
							}
						}
						else
						{
							strcpy((char*)net_packet->data, "CKOR");
						}
						net_packet->data[4] = PLAYER_NUM;
						if ( selectedEntity[PLAYER_NUM]->behavior == &actPlayerLimb)
						{
							SDLNet_Write32((Uint32)players[selectedEntity[PLAYER_NUM]->skill[2]]->entity->getUID(), &net_packet->data[5]);
						}
						else
						{
							Entity* tempEntity = uidToEntity(selectedEntity[PLAYER_NUM]->skill[2]);
							if (tempEntity)
							{
								if (tempEntity->behavior == &actMonster)
								{
									SDLNet_Write32((Uint32)tempEntity->getUID(), &net_packet->data[5]);
								}
								else
								{
									SDLNet_Write32((Uint32)selectedEntity[PLAYER_NUM]->getUID(), &net_packet->data[5]);
								}
							}
							else
							{
								SDLNet_Write32((Uint32)selectedEntity[PLAYER_NUM]->getUID(), &net_packet->data[5]);
							}
						}
						net_packet->address.host = net_server.host;
						net_packet->address.port = net_server.port;
						net_packet->len = 9;
						sendPacketSafe(net_sock, -1, net_packet, 0);
					}
				}
			}
		}
	}

	if (multiplayer != CLIENT)
	{
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if ( (i == 0 && selectedEntity[0] == my) 
				|| (client_selected[i] == my)
				|| (i == PLAYER_CLICKED - 1) 
				|| (i > 0 && splitscreen && selectedEntity[i] == my) )
			{
				PLAYER_CLICKED = 0;
				if (inrange[i] && i != PLAYER_NUM)
				{
					messagePlayer(i, MESSAGE_INTERACTION, Language::get(575), stats[PLAYER_NUM]->name);
					messagePlayer(PLAYER_NUM, MESSAGE_INTERACTION, Language::get(576), stats[i]->name);
					if ( players[PLAYER_NUM]->isLocalPlayer() && players[i] && players[i]->entity)
					{
						double tangent = atan2(my->y - players[i]->entity->y, my->x - players[i]->entity->x);
						PLAYER_VELX += cos(tangent);
						PLAYER_VELY += sin(tangent);
					}
				}
			}
		}
	}

	// lights
    my->removeLightField();
    bool ambientLight = false;
    const char* light_type = nullptr;
    static ConsoleVariable<bool> cvar_playerLight("/player_light_enabled", true);
	int range_bonus = std::min(std::max(0, statGetPER(stats[PLAYER_NUM], my) / 5), 2);
	if (!intro && *cvar_playerLight) {
        if (!players[PLAYER_NUM]->isLocalPlayer() && multiplayer == CLIENT) {
            switch (PLAYER_TORCH) {
            default: break;
            case 1:
                light_type = "player_torch";
                if (stats[PLAYER_NUM]->defending) {
                    ++range_bonus;
                }
                break;
            case 2:
                light_type = "player_lantern";
                if (stats[PLAYER_NUM]->defending) {
                    ++range_bonus;
                }
                break;
            case 3:
                light_type = "player_shard";
                if (stats[PLAYER_NUM]->defending) {
                    ++range_bonus;
                }
                break;
            }
        } else { // multiplayer != CLIENT
            if (stats[PLAYER_NUM]->shield && showEquipment && isHumanoid) {
                if (stats[PLAYER_NUM]->shield->type == TOOL_TORCH) {
                    light_type = "player_torch";
                    if (stats[PLAYER_NUM]->defending) {
                        ++range_bonus;
                    }
                }
                else if (stats[PLAYER_NUM]->shield->type == TOOL_LANTERN) {
                    light_type = "player_lantern";
                    if (stats[PLAYER_NUM]->defending) {
                        ++range_bonus;
                    }
                }
                else if (stats[PLAYER_NUM]->shield->type == TOOL_CRYSTALSHARD) {
                    light_type = "player_shard";
                    if (stats[PLAYER_NUM]->defending) {
                        ++range_bonus;
                    }
                }
                else if (players[PLAYER_NUM]->isLocalPlayer()) {
                    ambientLight = true;
					if ( stats[PLAYER_NUM]->sneaking ) {
						light_type = "player_sneaking";
					}
					else
					{
						light_type = "player_ambient";
					}

                }
            }
            else if (players[PLAYER_NUM]->isLocalPlayer()) {
                ambientLight = true;
                
                // carrying no light source
                if (playerRace == RAT) {
					if ( stats[PLAYER_NUM]->sneaking )
					{
						light_type = "player_ambient_rat_sneaking";
					}
					else
					{
						light_type = "player_ambient_rat";
					}
                }
                else if (playerRace == SPIDER) {
					if ( stats[PLAYER_NUM]->sneaking )
					{
						light_type = "player_ambient_spider_sneaking";
					}
					else
					{
						light_type = "player_ambient_spider";
					}
                }
				else if ( playerRace == TROLL ) {
					if ( stats[PLAYER_NUM]->sneaking )
					{
						light_type = "player_ambient_troll_sneaking";
					}
					else
					{
						light_type = "player_ambient_troll";
					}
				}
				else if ( playerRace == CREATURE_IMP ) {
					if ( stats[PLAYER_NUM]->sneaking )
					{
						light_type = "player_ambient_imp_sneaking";
					}
					else
					{
						light_type = "player_ambient_imp";
					}
				}
                else if (stats[PLAYER_NUM]->sneaking) {
                    light_type = "player_sneaking";
                }
                else {
                    light_type = "player_ambient";
                }
            }
        }
	}
    if (*cvar_playerLight) {
        if (my->flags[BURNING]) {
            my->light = addLight(my->x / 16, my->y / 16, "player_burning");
        }
        else if (!my->light) {
            my->light = addLight(my->x / 16, my->y / 16, light_type, range_bonus, ambientLight ? PLAYER_NUM + 1 : 0);
        }
    }

	// server controls players primarily
	if ( players[PLAYER_NUM]->isLocalPlayer() || multiplayer == SERVER || StatueManager.activeEditing )
	{
		// set head model
        my->sprite = playerHeadSprite(playerRace, stats[PLAYER_NUM]->sex,
            playerAppearance, PLAYER_ATTACK ? PLAYER_ATTACKTIME : 0);
	}
	if ( multiplayer != CLIENT )
	{
		// remove client entities that should no longer exist
		if ( !intro )
		{
			my->handleEffects(stats[PLAYER_NUM]); // hunger, regaining hp/mp, poison, etc.
			
			if ( client_disconnected[PLAYER_NUM] || stats[PLAYER_NUM]->HP <= 0 )
			{
				bool doDeathProcedure = true;
				if ( client_disconnected[PLAYER_NUM] )
				{
					doDeathProcedure = true;
				}
				else if ( stats[PLAYER_NUM]->type == AUTOMATON && !client_disconnected[PLAYER_NUM] && stats[PLAYER_NUM]->HP <= 0 )
				{
					// delay death. (not via disconnection).
					if ( PLAYER_DEATH_AUTOMATON == 0 )
					{
						my->flags[PASSABLE] = true;
						serverUpdateEntityFlag(my, PASSABLE);
						if ( players[PLAYER_NUM]->isLocalPlayer() )
						{
							// deathcam
							entity = newEntity(-1, 1, map.entities, nullptr); //Deathcam entity.
							entity->x = my->x;
							entity->y = my->y;
							entity->z = -2;
							entity->flags[NOUPDATE] = true;
							entity->flags[PASSABLE] = true;
							entity->flags[INVISIBLE] = true;
							entity->behavior = &actDeathCam;
							entity->skill[2] = PLAYER_NUM;
							entity->yaw = my->yaw;
							entity->pitch = PI / 8;
							my->playerCreatedDeathCam = 1;
						}
						createParticleExplosionCharge(my, 174, 100, 0.25);
						serverSpawnMiscParticles(my, PARTICLE_EFFECT_PLAYER_AUTOMATON_DEATH, 174);
						playSoundEntity(my, 263, 128);
						playSoundEntity(my, 321, 128);
					}
					++PLAYER_DEATH_AUTOMATON;
					if ( PLAYER_DEATH_AUTOMATON >= TICKS_PER_SECOND * 2 )
					{
						doDeathProcedure = true;
						spawnExplosion(my->x, my->y, my->z);
						playSoundEntity(my, 260 + local_rng.rand() % 2, 128);
						my->attack(MONSTER_POSE_AUTOMATON_MALFUNCTION, 0, my);
					}
					else
					{
						doDeathProcedure = false;
					}
				}
				if ( doDeathProcedure )
				{
					// remove body parts
					node_t* nextnode;
					for ( node = my->children.first, i = 0; node != NULL; node = nextnode, i++ )
					{
						nextnode = node->next;
						if ( i == 0 )
						{
							continue;
						}
						if ( node->element )
						{
							Entity* tempEntity = (Entity*)node->element;
							if ( tempEntity )
							{
								list_RemoveNode(tempEntity->mynode);
							}
							if ( i > 28 )
							{
								break;
							}
						}
					}
					if ( stats[PLAYER_NUM]->HP <= 0 )
					{
						// die //TODO: Refactor.
						playSoundEntity(my, 28, 128);
						Entity* gib = spawnGib(my);
						if (gib) {
							gib->skill[5] = 1; // poof
							gib->sprite = my->sprite;
						}
						serverSpawnGibForClient(gib);
						for ( int c = 0; c < 8; c++ )
						{
							Entity* gib = spawnGib(my);
							serverSpawnGibForClient(gib);
						}
						switch (stats[PLAYER_NUM]->type)
						{
						default:
						case HUMAN:
							my->spawnBlood();
							break;
						case SKELETON:
							break;
						case SPIDER:
						case INSECTOID:
							my->spawnBlood(212);
							break;
						}
						node_t* spellnode;
						spellnode = stats[PLAYER_NUM]->magic_effects.first;
						while ( spellnode )
						{
							node_t* oldnode = spellnode;
							spellnode = spellnode->next;
							spell_t* spell = (spell_t*)oldnode->element;
							spell->magic_effects_node = NULL;
							list_RemoveNode(oldnode);
						}
						int c;
						for ( c = 0; c < MAXPLAYERS; c++ )
						{
							if ( client_disconnected[c] )
							{
								continue;
							}
							char whatever[256];
							snprintf(whatever, sizeof(whatever), "%s %s", stats[PLAYER_NUM]->name, stats[PLAYER_NUM]->obituary);
							whatever[255] = '\0';
							messagePlayer(c, MESSAGE_OBITUARY, whatever);
						}
						Uint32 color = makeColorRGB(255, 0, 0);
						messagePlayerColor(PLAYER_NUM, MESSAGE_STATUS, color, Language::get(577));

                        // send "you have died" signal to client
			            if (multiplayer == SERVER) {
				            if (PLAYER_NUM != clientnum && !client_disconnected[PLAYER_NUM]) {
			                    memcpy(net_packet->data, "UDIE", 4);
			                    SDLNet_Write32((Uint32)stats[PLAYER_NUM]->killer, &net_packet->data[4]);
			                    if (stats[PLAYER_NUM]->killer == KilledBy::MONSTER) {
			                        net_packet->data[8] = (Uint8)stats[PLAYER_NUM]->killer_name.size();
			                        if (net_packet->data[8]) {
			                            memcpy(&net_packet->data[9], stats[PLAYER_NUM]->killer_name.c_str(), net_packet->data[8]);
			                            net_packet->len = 9 + net_packet->data[8];
			                        } else {
			                            SDLNet_Write32((Uint32)stats[PLAYER_NUM]->killer_monster, &net_packet->data[9]);
			                            net_packet->len = 13;
			                        }
			                    }
			                    else if (stats[PLAYER_NUM]->killer == KilledBy::ITEM) {
			                        SDLNet_Write32((Uint32)stats[PLAYER_NUM]->killer_item, &net_packet->data[8]);
			                        net_packet->len = 12;
			                    }
				                net_packet->address.host = net_clients[PLAYER_NUM - 1].host;
				                net_packet->address.port = net_clients[PLAYER_NUM - 1].port;
				                sendPacketSafe(net_sock, -1, net_packet, PLAYER_NUM - 1);
				            }
			            }

						if ( (multiplayer == SERVER || (multiplayer == SINGLE && splitscreen))
							&& keepInventoryGlobal )
						{
							sendMinimapPing(PLAYER_NUM, my->x / 16.0, my->y / 16.0, MinimapPing::PING_DEATH_MARKER);
						}

						for ( node_t* node = stats[PLAYER_NUM]->FOLLOWERS.first; node != nullptr; node = nextnode )
						{
							nextnode = node->next;
							Uint32* c = (Uint32*)node->element;
							Entity* myFollower = nullptr;
							if ( c )
							{
								myFollower = uidToEntity(*c);
							}
							if ( myFollower )
							{
								if ( myFollower->monsterAllySummonRank != 0 )
								{
									myFollower->setMP(0);
									myFollower->setHP(0); // rip
								}
								else if ( myFollower->flags[USERFLAG2] )
								{
									// our leader died, let's undo the color change since we're now rabid.
									myFollower->flags[USERFLAG2] = false;
									serverUpdateEntityFlag(myFollower, USERFLAG2);

									int bodypart = 0;
									for ( node_t* node = myFollower->children.first; node != nullptr; node = node->next )
									{
										if ( bodypart >= LIMB_HUMANOID_TORSO )
										{
											Entity* tmp = (Entity*)node->element;
											if ( tmp )
											{
												tmp->flags[USERFLAG2] = false;
											}
										}
										++bodypart;
									}

									Stat* followerStats = myFollower->getStats();
									if ( followerStats )
									{
										followerStats->leader_uid = 0;
									}
									list_RemoveNode(node);
									if ( !players[PLAYER_NUM]->isLocalPlayer() )
									{
										serverRemoveClientFollower(PLAYER_NUM, myFollower->getUID());
									}
								}
							}
						}

						/* //TODO: Eventually.
						{
							strcpy((char *)net_packet->data,"UDIE");
							net_packet->address.host = net_clients[player-1].host;
							net_packet->address.port = net_clients[player-1].port;
							net_packet->len = 4;
							sendPacketSafe(net_sock, -1, net_packet, player-1);
						}
						*/
						if ( players[PLAYER_NUM]->isLocalPlayer() )
						{
							if ( (stats[PLAYER_NUM]->type != AUTOMATON) 
								|| (stats[PLAYER_NUM]->type == AUTOMATON && my->playerCreatedDeathCam == 0) )
							{
								// deathcam
								entity = newEntity(-1, 1, map.entities, nullptr); //Deathcam entity.
								entity->x = my->x;
								entity->y = my->y;
								entity->z = -2;
								entity->flags[NOUPDATE] = true;
								entity->flags[PASSABLE] = true;
								entity->flags[INVISIBLE] = true;
								entity->behavior = &actDeathCam;
								entity->skill[2] = PLAYER_NUM;
								entity->yaw = my->yaw;
								entity->pitch = PI / 8;
							}
							node_t* nextnode;

							if ( gameModeManager.getMode() == GameModeManager_t::GAME_MODE_DEFAULT )
							{
								if ( multiplayer == SINGLE )
								{
									// stops save scumming c:
									if ( !splitscreen )
									{
										deleteSaveGame(multiplayer);
									}
									else
									{
										bool allDead = true;
										for (int c = 0; c < MAXPLAYERS; ++c)
										{
											if (!client_disconnected[c] && stats[c]->HP > 0)
											{
												allDead = false;
												break;
											}
										}
										if (allDead)
										{
											deleteSaveGame(multiplayer);
										}
									}
								}
								else
								{
									// Will only delete save games if was last player alive.
									deleteMultiplayerSaveGames();
								}
							}

							players[PLAYER_NUM]->closeAllGUIs(CloseGUIShootmode::CLOSEGUI_ENABLE_SHOOTMODE, CloseGUIIgnore::CLOSEGUI_CLOSE_ALL);
							players[PLAYER_NUM]->bControlEnabled = false;
#ifdef SOUND
							levelmusicplaying = true;
							combatmusicplaying = false;
							fadein_increment = default_fadein_increment * 4;
							fadeout_increment = default_fadeout_increment * 4;
							if ( gameModeManager.getMode() == GameModeManager_t::GAME_MODE_TUTORIAL )
							{
								playMusic(tutorialmusic, true, true, true);
							}

							bool allDead = true;
							if (splitscreen)
							{
								for (int c = 0; c < MAXPLAYERS; ++c)
								{
									if (!client_disconnected[c] && stats[c]->HP > 0)
									{
										allDead = false;
										break;
									}
								}
							}

							if (allDead)
							{
								playMusic(gameovermusic, false, true, false);
							}
#endif
							combat = false;

							if ( players[PLAYER_NUM]->isLocalPlayer()
								&& gameModeManager.getMode() == GameModeManager_t::GAME_MODE_TUTORIAL )
							{
								if ( !strncmp(map.name, "Tutorial Hub", 12) )
								{
									steamAchievement("BARONY_ACH_EXPELLED");
								}
								else
								{
									steamAchievement("BARONY_ACH_TEACHABLE_MOMENT");
								}
							}

							if ( (multiplayer == SINGLE && !splitscreen) || !keepInventoryGlobal )
							{
								for ( node = stats[PLAYER_NUM]->inventory.first; node != nullptr; node = nextnode )
								{
									nextnode = node->next;
									Item* item = (Item*)node->element;
									if ( itemCategory(item) == SPELL_CAT )
									{
										continue;    // don't drop spells on death, stupid!
									}
									if ( item->type >= ARTIFACT_SWORD && item->type <= ARTIFACT_GLOVES )
									{
										if ( itemIsEquipped(item, PLAYER_NUM) )
										{
											steamAchievement("BARONY_ACH_CHOSEN_ONE");
										}
									}
									if ( (multiplayer == SINGLE && !splitscreen) )
									{
										int c = item->count;
										for ( c = item->count; c > 0; c-- )
										{
											entity = newEntity(-1, 1, map.entities, nullptr); //Item entity.
											entity->flags[INVISIBLE] = true;
											entity->flags[UPDATENEEDED] = true;
											entity->x = my->x;
											entity->y = my->y;
											entity->sizex = 4;
											entity->sizey = 4;
											entity->yaw = (local_rng.rand() % 360) * (PI / 180.f);
											entity->vel_x = (local_rng.rand() % 20 - 10) / 10.0;
											entity->vel_y = (local_rng.rand() % 20 - 10) / 10.0;
											entity->vel_z = -.5;
											entity->flags[PASSABLE] = true;
											entity->flags[USERFLAG1] = true;
											entity->behavior = &actItem;
											entity->skill[10] = item->type;
											entity->skill[11] = item->status;
											entity->skill[12] = item->beatitude;
											int qtyToDrop = 1;
											if ( c >= 10 && (item->type == TOOL_METAL_SCRAP || item->type == TOOL_MAGIC_SCRAP) )
											{
												qtyToDrop = 10;
												c -= 9;
											}
											else if ( itemTypeIsQuiver(item->type) )
											{
												qtyToDrop = item->count;
												c -= item->count;
											}
											entity->skill[13] = qtyToDrop;
											entity->skill[14] = item->appearance;
											entity->skill[15] = item->identified;
										}
									}
									else
									{
										stats[0]->addItemToLootingBag(PLAYER_NUM, my->x, my->y, *item);
									}
								}
							}
							else
							{
								// to not soft lock at Herx
								for ( node = stats[PLAYER_NUM]->inventory.first; node != nullptr; node = nextnode )
								{
									nextnode = node->next;
									Item* item = (Item*)node->element;
									if ( item->type == ARTIFACT_ORB_PURPLE )
									{
										int c = item->count;
										for ( c = item->count; c > 0; c-- )
										{
											entity = newEntity(-1, 1, map.entities, nullptr); //Item entity.
											entity->flags[INVISIBLE] = true;
											entity->flags[UPDATENEEDED] = true;
											entity->x = my->x;
											entity->y = my->y;
											entity->sizex = 4;
											entity->sizey = 4;
											entity->yaw = (local_rng.rand() % 360) * (PI / 180.f);
											entity->vel_x = (local_rng.rand() % 20 - 10) / 10.0;
											entity->vel_y = (local_rng.rand() % 20 - 10) / 10.0;
											entity->vel_z = -.5;
											entity->flags[PASSABLE] = true;
											entity->flags[USERFLAG1] = true;
											entity->behavior = &actItem;
											entity->skill[10] = item->type;
											entity->skill[11] = item->status;
											entity->skill[12] = item->beatitude;
											entity->skill[13] = 1;
											entity->skill[14] = item->appearance;
											entity->skill[15] = item->identified;
										}
										break;
									}
								}
							}
							for ( node_t* mapNode = map.creatures->first; mapNode != nullptr; mapNode = mapNode->next )
							{
								Entity* mapCreature = (Entity*)mapNode->element;
								if ( mapCreature )
								{
									mapCreature->monsterEntityRenderAsTelepath = 0; // do a final pass to undo any telepath rendering.
								}
							}
						}
						else
						{
							if ( !keepInventoryGlobal )
							{
								my->x = ((int)(my->x / 16)) * 16 + 8;
								my->y = ((int)(my->y / 16)) * 16 + 8;
                                
                                Item* items[] = {
                                    stats[PLAYER_NUM]->helmet,
                                    stats[PLAYER_NUM]->breastplate,
                                    stats[PLAYER_NUM]->gloves,
                                    stats[PLAYER_NUM]->shoes,
                                    stats[PLAYER_NUM]->shield,
                                    stats[PLAYER_NUM]->weapon,
                                    stats[PLAYER_NUM]->cloak,
                                    stats[PLAYER_NUM]->amulet,
                                    stats[PLAYER_NUM]->ring,
                                    stats[PLAYER_NUM]->mask,
                                };
                                constexpr int num_slots = sizeof(items) / sizeof(items[0]);
                                for (int c = 0; c < num_slots; ++c) {
                                    Item* slot = items[c];
                                    if (slot) {
                                        stats[0]->addItemToLootingBag(PLAYER_NUM, my->x, my->y, *slot);
                                    }
                                }
							}

							deleteMultiplayerSaveGames(); //Will only delete save games if was last player alive.
						}

						assailant[PLAYER_NUM] = false;
						assailantTimer[PLAYER_NUM] = 0;

						if ( multiplayer != SINGLE )
						{
							messagePlayer(PLAYER_NUM, MESSAGE_HINT, Language::get(578));
						}
					}
					my->removeLightField();
					list_RemoveNode(my->mynode);
					return;
				}
			}
			else
			{
				my->playerCreatedDeathCam = 0;
				PLAYER_DEATH_AUTOMATON = 0;
			}
		}
	}

	if ( (players[PLAYER_NUM]->isLocalPlayer()) && intro == false )
	{
		// effects of drunkenness
		if ( (stats[PLAYER_NUM]->EFFECTS[EFF_DRUNK] && (stats[PLAYER_NUM]->type != GOATMAN))
			|| stats[PLAYER_NUM]->EFFECTS[EFF_WITHDRAWAL] )
		{
			CHAR_DRUNK++;
			int drunkInterval = TICKS_PER_SECOND * 6;
			if ( stats[PLAYER_NUM]->EFFECTS[EFF_WITHDRAWAL] )
			{
				if ( PLAYER_ALIVETIME < TICKS_PER_SECOND * 16 )
				{
					drunkInterval = TICKS_PER_SECOND * 6;
				}
				else
				{
					drunkInterval = TICKS_PER_SECOND * 30;
				}
			}

			if ( CHAR_DRUNK >= drunkInterval )
			{
				CHAR_DRUNK = 0;
				messagePlayer(PLAYER_NUM, MESSAGE_WORLD, Language::get(579));
				cameravars[PLAYER_NUM].shakex -= .04;
				cameravars[PLAYER_NUM].shakey -= 5;
			}
		}

		if ( !my->isMobile() )
		{
			if ( (players[PLAYER_NUM]->isLocalPlayer()) && openedChest[PLAYER_NUM] )
			{
				openedChest[PLAYER_NUM]->closeChest();
			}
		}

		if ( PLAYER_DEATH_AUTOMATON > 0 )
		{
			if ( my->pitch < PI / 3 || my->pitch > 5 * PI / 3 )
			{
				limbAnimateToLimit(my, ANIMATE_PITCH, 0.01, PI / 3, true, 0.005);
			}
		}

		if ( !usecamerasmoothing )
		{
			players[PLAYER_NUM]->movement.handlePlayerMovement(false);
			players[PLAYER_NUM]->movement.handlePlayerCameraUpdate(false);
		}

		// send movement updates to server
		if ( multiplayer == CLIENT )
		{
			strcpy((char*)net_packet->data, "PMOV");
			net_packet->data[4] = PLAYER_NUM;
			net_packet->data[5] = currentlevel;
			SDLNet_Write16((Sint16)(my->x * 32), &net_packet->data[6]);
			SDLNet_Write16((Sint16)(my->y * 32), &net_packet->data[8]);
			SDLNet_Write16((Sint16)(PLAYER_VELX * 128), &net_packet->data[10]);
			SDLNet_Write16((Sint16)(PLAYER_VELY * 128), &net_packet->data[12]);
			SDLNet_Write16((Sint16)(my->yaw * 128), &net_packet->data[14]);
			SDLNet_Write16((Sint16)(my->pitch * 128), &net_packet->data[16]);
			net_packet->data[18] = secretlevel;
			net_packet->address.host = net_server.host;
			net_packet->address.port = net_server.port;
			net_packet->len = 19;
			sendPacket(net_sock, -1, net_packet, 0);
		}

		// move
		if ( noclip == false )
		{
			// perform collision detection
			dist = clipMove(&my->x, &my->y, PLAYER_VELX, PLAYER_VELY, my);

			// bumping into monsters disturbs them
			if ( hit.entity && !intro && multiplayer != CLIENT )
			{
				if ( !everybodyfriendly && hit.entity->behavior == &actMonster )
				{
					bool enemy = my->checkEnemy(hit.entity);
					if ( enemy )
					{
						if ( hit.entity->monsterState == MONSTER_STATE_WAIT || (hit.entity->monsterState == MONSTER_STATE_HUNT && hit.entity->monsterTarget == 0) )
						{
							double tangent = atan2( my->y - hit.entity->y, my->x - hit.entity->x );
							hit.entity->skill[4] = 1;
							hit.entity->skill[6] = local_rng.rand() % 10 + 1;
							hit.entity->fskill[4] = tangent;
						}
					}
				}
				else if ( stats[PLAYER_NUM]->EFFECTS[EFF_DASH] && hit.entity->behavior == &actDoor )
				{
					hit.entity->doorHealth = 0;
				}
			}
		}
		else
		{
			// no collision detection
			my->x += PLAYER_VELX;
			my->y += PLAYER_VELY;
			dist = sqrt(PLAYER_VELX * PLAYER_VELX + PLAYER_VELY * PLAYER_VELY);
		}
	}

	if ( !players[PLAYER_NUM]->isLocalPlayer() && multiplayer == SERVER )
	{
		// PLAYER_VEL* skills updated by messages sent to server from client

		// move (dead reckoning)
		if ( noclip == false )
		{
			// from PMOV in serverHandlePacket - new_x and new_y are accumulated positions
			if ( my->new_x > 0.001 )
			{
				my->x = my->new_x;
			}
			if ( my->new_y > 0.001 )
			{
				my->y = my->new_y;
			}

			dist = clipMove(&my->x, &my->y, PLAYER_VELX, PLAYER_VELY, my);

			// bumping into monsters disturbs them
			if ( hit.entity && !intro )
			{
				if ( !everybodyfriendly && hit.entity->behavior == &actMonster )
				{
					bool enemy = my->checkEnemy(hit.entity);
					if ( enemy )
					{
						if ( hit.entity->monsterState == MONSTER_STATE_WAIT || (hit.entity->monsterState == MONSTER_STATE_HUNT && hit.entity->monsterTarget == 0) )
						{
							double tangent = atan2( my->y - hit.entity->y, my->x - hit.entity->x );
							hit.entity->skill[4] = 1;
							hit.entity->skill[6] = local_rng.rand() % 10 + 1;
							hit.entity->fskill[4] = tangent;
						}
					}
				}
				else if ( stats[PLAYER_NUM]->EFFECTS[EFF_DASH] && hit.entity->behavior == &actDoor )
				{
					hit.entity->doorHealth = 0;
				}
			}
		}
		else
		{
			my->x += PLAYER_VELX;
			my->y += PLAYER_VELY;
			dist = sqrt(PLAYER_VELX * PLAYER_VELX + PLAYER_VELY * PLAYER_VELY);
		}
	}

	if ( !players[PLAYER_NUM]->isLocalPlayer() && multiplayer == CLIENT )
	{
		dist = sqrt(PLAYER_VELX * PLAYER_VELX + PLAYER_VELY * PLAYER_VELY);
	}

	if ( (players[PLAYER_NUM]->isLocalPlayer()) && ticks % 65 == 0 && stats[PLAYER_NUM]->EFFECTS[EFF_TELEPATH] )
	{
		for ( node_t* mapNode = map.creatures->first; mapNode != nullptr; mapNode = mapNode->next )
		{
			Entity* mapCreature = (Entity*)mapNode->element;
			if ( mapCreature )
			{
				if ( stats[PLAYER_NUM]->EFFECTS[EFF_TELEPATH] && !intro )
				{
					// periodically set the telepath rendering flag.
					mapCreature->monsterEntityRenderAsTelepath = 1;
				}
				else
				{
					mapCreature->monsterEntityRenderAsTelepath = 0;
				}
			}
		}
	}

	Entity* helmet = nullptr;

	// move bodyparts
	if ( isHumanoid )
	{
		for ( bodypart = 0, node = my->children.first; node != NULL; node = node->next, bodypart++ )
		{
			if ( bodypart == 0 )
			{
				// hudweapon case
				continue;
			}
			entity = (Entity*)node->element;
			entity->x = my->x;
			entity->y = my->y;
			entity->z = my->z;

			if ( bodypart < 9 ) // don't shift helm/mask. 
			{
				// these monsters are shorter than humans so extend the limbs down to floor, gives longer neck.
				if ( playerRace == GOBLIN || playerRace == INSECTOID || playerRace == GOATMAN )
				{
					entity->z += 0.5;
				}
				else if ( playerRace == SKELETON || playerRace == AUTOMATON )
				{
					entity->z += 0.25;
				}
			}

			if ( bodypart > 12 )
			{
				entity->flags[INVISIBLE] = true;
				continue;
			}

			entity->yaw = my->yaw;
			if ( bodypart == 2 || bodypart == 5 ) // right leg, left arm
			{
				if ( bodypart == 2 )
				{
					rightbody = (Entity*)node->next->element;
				}
				if ( bodypart == 5 )
				{
					shieldarm = entity;
				}
				double limbSpeed = dist;
				double pitchLimit = PI / 4.f;
				if ( playerRace == CREATURE_IMP )
				{
					limbSpeed = 1 / 12.f;
					pitchLimit = PI / 8.f;
				}
				if ( stats[PLAYER_NUM]->EFFECTS[EFF_DASH] )
				{
					limbSpeed = 1 / 12.f;
				}
				node_t* shieldNode = list_Node(&my->children, 7);
				if ( shieldNode )
				{
					Entity* shield = (Entity*)shieldNode->element;
					bool bendArm = true;
					if ( shield->flags[INVISIBLE] )
					{
						bendArm = false;
					}
					else if ( shield->sprite >= items[SPELLBOOK_LIGHT].index
						&& shield->sprite < (items[SPELLBOOK_LIGHT].index + items[SPELLBOOK_LIGHT].variations) )
					{
						bendArm = false;
					}

					if ( insectoidLevitating )
					{
						// hands stationary, legs pitched back and little swing.
						limbSpeed = 0.03;
						if ( bodypart == 5 ) // left arm
						{
							if ( entity->pitch < 0 )
							{
								entity->pitch += 1 / fmax(limbSpeed * .1, 10.0);
								if ( entity->pitch > 0 )
								{
									entity->pitch = 0;
								}
							}
							else if ( entity->pitch > 0 )
							{
								entity->pitch -= 1 / fmax(limbSpeed * .1, 10.0);
								if ( entity->pitch < 0 )
								{
									entity->pitch = 0;
								}
							}
						}
						else if ( bodypart == 2 )
						{
							if ( entity->pitch < 0 )
							{
								entity->pitch += 5 * limbSpeed * PLAYERWALKSPEED; // speed up to reach target.
							}
							if ( !rightbody->skill[3] )
							{
								entity->pitch -= limbSpeed * PLAYERWALKSPEED;
								if ( entity->pitch < PI / 6.f )
								{
									entity->pitch = PI / 6.f;
								}
							}
							else
							{
								entity->pitch += limbSpeed * PLAYERWALKSPEED;
								if ( entity->pitch > PI / 3.f )
								{
									entity->pitch = PI / 3.f;
								}
							}
						}
					}
					else if ( (fabs(PLAYER_VELX) > 0.1 || fabs(PLAYER_VELY) > 0.1) && (bodypart != 5 || !bendArm)
						|| playerRace == CREATURE_IMP )
					{
						if ( oldInsectoidLevitate )
						{
							entity->pitch = 0;
						}

						if ( !rightbody->skill[0] )
						{
							entity->pitch -= limbSpeed * PLAYERWALKSPEED;
							if ( entity->pitch < -pitchLimit )
							{
								entity->pitch = -pitchLimit;
								if ( bodypart == 2 && dist > .4 && !levitating && !swimming )
								{
									node_t* tempNode = list_Node(&my->children, 2);
									if ( tempNode )
									{
										Entity* foot = (Entity*)tempNode->element;
										if ( playerRace == TROLL )
										{
											playSoundEntityLocal(my, my->getMonsterFootstepSound(MONSTER_FOOTSTEP_STOMP, foot->sprite), 32);
										}
										else if ( playerRace == SPIDER || playerRace == RAT || playerRace == CREATURE_IMP )
										{
											// no sound.
										}
										else
										{
											playSoundEntityLocal(my, my->getMonsterFootstepSound(MONSTER_FOOTSTEP_USE_BOOTS, foot->sprite), 32);
										}
									}
								}
							}
						}
						else
						{
							entity->pitch += limbSpeed * PLAYERWALKSPEED;
							if ( entity->pitch > pitchLimit )
							{
								entity->pitch = pitchLimit;
								if ( bodypart == 2 && dist > .4 && !levitating && !swimming )
								{
									node_t* tempNode = list_Node(&my->children, 2);
									if ( tempNode )
									{
										Entity* foot = (Entity*)tempNode->element;
										if ( playerRace == TROLL )
										{
											playSoundEntityLocal(my, my->getMonsterFootstepSound(MONSTER_FOOTSTEP_STOMP, foot->sprite), 32);
										}
										else if ( playerRace == SPIDER || playerRace == RAT || playerRace == CREATURE_IMP )
										{
											// no sound.
										}
										else
										{
											playSoundEntityLocal(my, my->getMonsterFootstepSound(MONSTER_FOOTSTEP_USE_BOOTS, foot->sprite), 32);
										}
									}
								}
							}
						}
					}
					else
					{
						if ( !freezeLimbMovements )
						{
							if ( entity->pitch < 0 )
							{
								entity->pitch += 1 / fmax(limbSpeed * .1, 10.0);
								if ( entity->pitch > 0 )
								{
									entity->pitch = 0;
								}
							}
							else if ( entity->pitch > 0 )
							{
								entity->pitch -= 1 / fmax(limbSpeed * .1, 10.0);
								if ( entity->pitch < 0 )
								{
									entity->pitch = 0;
								}
							}
						}
					}
				}
			}
			else if ( bodypart == 3 || bodypart == 4 || bodypart == 8 ) // left leg, right arm, cloak
			{
				if ( bodypart == 4 )
				{
					weaponarm = entity;
					if ( PLAYER_ATTACK == 1 || PLAYER_ATTACK == PLAYER_POSE_GOLEM_SMASH )
					{
						// vertical chop
						if ( PLAYER_ATTACKTIME == 0 )
						{
							PLAYER_ARMBENDED = 0;
							PLAYER_WEAPONYAW = 0;
							entity->pitch = 0;
							entity->roll = 0;
							entity->skill[1] = 0;
						}
						else
						{
							if ( entity->skill[1] == 0 )
							{
								// upswing
								if ( limbAnimateToLimit(entity, ANIMATE_PITCH, -0.5, 5 * PI / 4, false, 0.0) )
								{
									entity->skill[1] = 1;
								}
							}
							else
							{
								if ( entity->pitch >= 3 * PI / 2 )
								{
									PLAYER_ARMBENDED = 1;
								}
								if ( limbAnimateToLimit(entity, ANIMATE_PITCH, 0.3, PI / 4, false, 0.0) )
								{
									entity->skill[0] = rightbody->skill[0];
									entity->skill[1] = 0;
									PLAYER_WEAPONYAW = 0;
									entity->pitch = rightbody->pitch;
									entity->roll = 0;
									PLAYER_ARMBENDED = 0;
									PLAYER_ATTACK = 0;
								}
							}
						}

						if ( PLAYER_ATTACK == PLAYER_POSE_GOLEM_SMASH && (players[PLAYER_NUM]->isLocalPlayer()) )
						{
							if ( my->pitch < PI / 32 )
							{
								// rotate head upwards
								if ( limbAngleWithinRange(my->pitch, 0.1, PI / 32) )
								{
									my->pitch = PI / 32;
								}
								else
								{
									my->pitch += 0.1;
								}
							}
							else
							{
								// rotate head downwards
								if ( limbAngleWithinRange(my->pitch, -0.1, PI / 32) )
								{
									my->pitch = PI / 32;
								}
								else
								{
									my->pitch -= 0.1;
								}
							}
						}
					}
					else if ( PLAYER_ATTACK == 2 )
					{
						// horizontal chop
						if ( PLAYER_ATTACKTIME == 0 )
						{
							PLAYER_ARMBENDED = 1;
							PLAYER_WEAPONYAW = -3 * PI / 4;
							entity->pitch = 0;
							entity->roll = -PI / 2;
						}
						else
						{
							if ( PLAYER_WEAPONYAW >= PI / 8 )
							{
								entity->skill[0] = rightbody->skill[0];
								PLAYER_WEAPONYAW = 0;
								entity->pitch = rightbody->pitch;
								entity->roll = 0;
								PLAYER_ARMBENDED = 0;
								PLAYER_ATTACK = 0;
							}
							else
							{
								PLAYER_WEAPONYAW += .25;
							}
						}
					}
					else if ( PLAYER_ATTACK == 3 )
					{
						// stab
						if ( PLAYER_ATTACKTIME == 0 )
						{
							PLAYER_ARMBENDED = 0;
							PLAYER_WEAPONYAW = 0;
							entity->pitch = 0;
							entity->roll = 0;
						}
						else
						{
							if ( PLAYER_ATTACKTIME >= 5 )
							{
								if ( playerRace != CREATURE_IMP )
								{
									PLAYER_ARMBENDED = 1;
								}
								limbAnimateToLimit(entity, ANIMATE_PITCH, -0.5, 11 * PI / 6, false, 0.0);
							}
							else
							{
								limbAnimateToLimit(entity, ANIMATE_PITCH, 0.4, 2 * PI / 3, false, 0.0);
							}
							if ( PLAYER_ATTACKTIME >= 10 )
							{
								entity->skill[0] = rightbody->skill[0];
								PLAYER_WEAPONYAW = 0;
								entity->pitch = rightbody->pitch;
								entity->roll = 0;
								PLAYER_ARMBENDED = 0;
								PLAYER_ATTACK = 0;
							}
						}
					}
					// special double vertical chop
					else if ( PLAYER_ATTACK == MONSTER_POSE_SPECIAL_WINDUP1
						|| PLAYER_ATTACK == MONSTER_POSE_SPECIAL_WINDUP2 )
					{
						if ( PLAYER_ATTACKTIME == 0 )
						{
							// init rotations
							PLAYER_ARMBENDED = 0;
							PLAYER_WEAPONYAW = 0;
							entity->pitch = 0;
							entity->roll = 0;
							entity->skill[1] = 0;
							if ( PLAYER_ATTACK == MONSTER_POSE_SPECIAL_WINDUP1 )
							{
								createParticleDot(my);
							}
						}
						else
						{
							// move the head.
							//limbAnimateToLimit(my, ANIMATE_PITCH, -0.1, 11 * PI / 6, true, 0.1);
							if ( (players[PLAYER_NUM]->isLocalPlayer()) && PLAYER_ATTACK == MONSTER_POSE_SPECIAL_WINDUP1 )
							{
								if ( my->pitch > -PI / 12 )
								{
									// rotate head upwards
									if ( limbAngleWithinRange(my->pitch, -0.03, -PI / 12) )
									{
										my->pitch = -PI / 12;
									}
									else
									{
										my->pitch += -0.02;
									}
								}
								else
								{
									// slowly rotate head downwards
									if ( limbAngleWithinRange(my->pitch, 0.03, -PI / 12) )
									{
										my->pitch = -PI / 12;
									}
									else
									{
										my->pitch -= -0.01;
									}
								}
							}

							if ( PLAYER_ATTACK == MONSTER_POSE_SPECIAL_WINDUP2 )
							{
								// lower right arm.
								if ( entity->skill[1] == 0 )
								{
									// upswing
									if ( limbAnimateToLimit(entity, ANIMATE_PITCH, -0.5, 5 * PI / 4, false, 0.0) )
									{
										entity->skill[1] = 1;
									}
								}
								else
								{
									if ( entity->pitch >= 3 * PI / 2 )
									{
										PLAYER_ARMBENDED = 1;
									}
									if ( limbAnimateToLimit(entity, ANIMATE_PITCH, 0.3, PI / 4, false, 0.1) )
									{
										entity->skill[0] = rightbody->skill[0];
										entity->skill[1] = 0;
										PLAYER_WEAPONYAW = 0;
										entity->pitch = rightbody->pitch;
										entity->roll = 0;
										PLAYER_ARMBENDED = 0;
										PLAYER_ATTACK = 0;
									}
								}
							}
							else
							{
								// raise right arm and tilt.
								limbAnimateToLimit(entity, ANIMATE_PITCH, -0.1, 9 * PI / 8, true, 0.1);
								limbAnimateToLimit(entity, ANIMATE_ROLL, -0.2, PI / 32, false, 0);
								if ( PLAYER_ATTACKTIME == 5 )
								{
									if ( playerRace == TROLL && local_rng.rand() % 4 == 0 )
									{
										playSoundEntityLocal(players[PLAYER_NUM]->entity, 79, 128);
									}
								}
								else if ( PLAYER_ATTACKTIME == 35 )
								{
									playSoundEntityLocal(players[PLAYER_NUM]->entity, 164, 128);
								}
							}
						}
					}
					else if ( PLAYER_ATTACK == MONSTER_POSE_RANGED_SHOOT1 || PLAYER_ATTACK == MONSTER_POSE_RANGED_SHOOT2 )
					{
						// init rotations
						PLAYER_ARMBENDED = 0;
						PLAYER_WEAPONYAW = 0;
						weaponarm->roll = 0;
						weaponarm->skill[1] = 0;
						weaponarm->pitch = 0;
						PLAYER_ATTACK = MONSTER_POSE_RANGED_SHOOT3;
					}
					else if ( PLAYER_ATTACK == MONSTER_POSE_RANGED_SHOOT3 )
					{
						// recoil upwards
						if ( weaponarm->skill[1] == 0 )
						{
							real_t targetPitch = 14 * PI / 8;
							if ( weaponarm->sprite == items[CROSSBOW].index || weaponarm->sprite == items[HEAVY_CROSSBOW].index )
							{
								targetPitch = 15 * PI / 8;
							}
							if ( limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.2, 14 * PI / 8, false, 0.0) )
							{
								weaponarm->skill[1] = 1;
							}
						}
						// recoil downwards
						else if ( weaponarm->skill[1] == 1 )
						{
							if ( limbAnimateToLimit(weaponarm, ANIMATE_PITCH, 0.1, 1 * PI / 3, false, 0.0) )
							{
								weaponarm->skill[1] = 2;
							}
						}
						else if ( weaponarm->skill[1] >= 2 )
						{
							// limbAngleWithinRange cuts off animation early so it doesn't snap too far back to position.
							real_t targetPitch = rightbody->pitch;
							while ( targetPitch < 0 )
							{
								targetPitch += 2 * PI;
							}
							while ( targetPitch >= 2 * PI )
							{
								targetPitch -= 2 * PI;
							}
							if ( limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.2, targetPitch, false, 0.0) 
								|| limbAngleWithinRange(weaponarm->pitch, -0.4, targetPitch)
								|| weaponarm->pitch < -PI / 8)
							{
								weaponarm->skill[0] = rightbody->skill[0];
								PLAYER_WEAPONYAW = 0;
								weaponarm->pitch = rightbody->pitch;
								weaponarm->roll = 0;
								PLAYER_ARMBENDED = 0;
								PLAYER_ATTACK = 0;
							}
						}
					}
				}
				else if ( bodypart == 8 )
				{
					entity->pitch = entity->fskill[0];
				}

				if ( insectoidLevitating )
				{
					// hands stationary, legs pitched back and little swing.
					double limbSpeed = 0.03;
					if ( bodypart == 4 && (PLAYER_ATTACK == 0 && PLAYER_ATTACKTIME == 0) ) // right arm relaxed, not attacking.
					{
						entity->skill[0] = rightbody->skill[0];
						if ( entity->pitch < 0 )
						{
							entity->pitch += 1 / fmax(limbSpeed * .1, 10.0);
							if ( entity->pitch > 0 )
							{
								entity->pitch = 0;
							}
						}
						else if ( entity->pitch > 0 )
						{
							entity->pitch -= 1 / fmax(limbSpeed * .1, 10.0);
							if ( entity->pitch < 0 )
							{
								entity->pitch = 0;
							}
						}
					}
					else if ( bodypart == 3 ) // leftleg
					{
						if ( entity->pitch < 0 )
						{
							entity->pitch += 5 * limbSpeed * PLAYERWALKSPEED; // speed up to reach target.
						}
						entity->skill[0] = 1;
						if ( entity->skill[3] == 1 ) // throwaway skill.
						{
							entity->pitch -= limbSpeed * PLAYERWALKSPEED;
							if ( entity->pitch < PI / 6.f )
							{
								entity->skill[3] = 0;
								entity->pitch = PI / 6.f;
							}
						}
						else
						{
							entity->pitch += limbSpeed * PLAYERWALKSPEED;
							if ( entity->pitch > PI / 3.f )
							{
								entity->skill[3] = 1;
								entity->pitch = PI / 3.f;
							}
						}
					}
				}
				else if ( bodypart != 4 || (PLAYER_ATTACK == 0 && PLAYER_ATTACKTIME == 0) )
				{
					if ( bodypart != 8 )
					{
						if ( oldInsectoidLevitate )
						{
							entity->pitch = 0;
						}
					}

					double limbSpeed = dist;
					double pitchLimit = PI / 4.f;
					if ( playerRace == CREATURE_IMP )
					{
						limbSpeed = 1 / 12.f;
						pitchLimit = PI / 8.f;
					}
					if ( stats[PLAYER_NUM]->EFFECTS[EFF_DASH] )
					{
						limbSpeed = 1 / 12.f;
					}
					if ( (fabs(PLAYER_VELX) > 0.1 || fabs(PLAYER_VELY) > 0.1 || playerRace == CREATURE_IMP) )
					{
						if ( entity->skill[0] )
						{
							entity->pitch -= limbSpeed * PLAYERWALKSPEED;
							if ( entity->pitch < -pitchLimit )
							{
								entity->skill[0] = 0;
								entity->pitch = -pitchLimit;
							}
						}
						else
						{
							entity->pitch += limbSpeed * PLAYERWALKSPEED;
							if ( entity->pitch > pitchLimit )
							{
								entity->skill[0] = 1;
								entity->pitch = pitchLimit;
							}
						}
					}
					else
					{
						if ( !freezeLimbMovements )
						{
							if ( entity->pitch < 0 )
							{
								entity->pitch += 1 / fmax(limbSpeed * .1, 10.0);
								if ( entity->pitch > 0 )
								{
									entity->pitch = 0;
								}
							}
							else if ( entity->pitch > 0 )
							{
								entity->pitch -= 1 / fmax(limbSpeed * .1, 10.0);
								if ( entity->pitch < 0 )
								{
									entity->pitch = 0;
								}
							}
						}
					}
				}
				if ( bodypart == 8 )
				{
					entity->fskill[0] = entity->pitch;
					entity->roll = my->roll - fabs(entity->pitch) / 2;
					entity->pitch = 0;
				}
			}

			switch ( bodypart )
			{
				// torso
				case 1:
					torso = entity;
					entity->focalx = limbs[playerRace][1][0];
					entity->focaly = limbs[playerRace][1][1];
					entity->focalz = limbs[playerRace][1][2];
					if ( multiplayer != CLIENT )
					{
						if ( stats[PLAYER_NUM]->breastplate == NULL || !showEquipment )
						{
							entity->setDefaultPlayerModel(PLAYER_NUM, playerRace, LIMB_HUMANOID_TORSO);
						}
						else
						{
							entity->sprite = itemModel(stats[PLAYER_NUM]->breastplate);
						}
						if ( multiplayer == SERVER )
						{
							// update sprites for clients
							if ( entity->skill[10] != entity->sprite )
							{
								entity->skill[10] = entity->sprite;
								serverUpdateEntityBodypart(my, bodypart);
							}
							if ( PLAYER_ALIVETIME == TICKS_PER_SECOND + bodypart )
							{
								serverUpdateEntityBodypart(my, bodypart);
							}
						}
					}
					my->setHumanoidLimbOffset(entity, playerRace, LIMB_HUMANOID_TORSO);
					break;
					// right leg
				case 2:
					entity->focalx = limbs[playerRace][2][0];
					entity->focaly = limbs[playerRace][2][1];
					entity->focalz = limbs[playerRace][2][2];
					if ( multiplayer != CLIENT )
					{
						if ( stats[PLAYER_NUM]->shoes == NULL || !showEquipment )
						{
							entity->setDefaultPlayerModel(PLAYER_NUM, playerRace, LIMB_HUMANOID_RIGHTLEG);
						}
						else
						{
							my->setBootSprite(entity, SPRITE_BOOT_RIGHT_OFFSET);
						}
						if ( multiplayer == SERVER )
						{
							// update sprites for clients
							if ( entity->skill[10] != entity->sprite )
							{
								entity->skill[10] = entity->sprite;
								serverUpdateEntityBodypart(my, bodypart);
							}
							if ( PLAYER_ALIVETIME == TICKS_PER_SECOND + bodypart )
							{
								serverUpdateEntityBodypart(my, bodypart);
							}
						}
					}
					my->setHumanoidLimbOffset(entity, playerRace, LIMB_HUMANOID_RIGHTLEG);
					break;
					// left leg
				case 3:
					entity->focalx = limbs[playerRace][3][0];
					entity->focaly = limbs[playerRace][3][1];
					entity->focalz = limbs[playerRace][3][2];
					if ( multiplayer != CLIENT )
					{
						if ( stats[PLAYER_NUM]->shoes == NULL || !showEquipment )
						{
							entity->setDefaultPlayerModel(PLAYER_NUM, playerRace, LIMB_HUMANOID_LEFTLEG);
						}
						else
						{
							my->setBootSprite(entity, SPRITE_BOOT_LEFT_OFFSET);
						}
						if ( multiplayer == SERVER )
						{
							// update sprites for clients
							if ( entity->skill[10] != entity->sprite )
							{
								entity->skill[10] = entity->sprite;
								serverUpdateEntityBodypart(my, bodypart);
							}
							if ( PLAYER_ALIVETIME == TICKS_PER_SECOND + bodypart )
							{
								serverUpdateEntityBodypart(my, bodypart);
							}
						}
					}
					my->setHumanoidLimbOffset(entity, playerRace, LIMB_HUMANOID_LEFTLEG);
					break;
					// right arm
				case 4:
				{
					if ( multiplayer != CLIENT )
					{
						if ( stats[PLAYER_NUM]->gloves == NULL || !showEquipment )
						{
							entity->setDefaultPlayerModel(PLAYER_NUM, playerRace, LIMB_HUMANOID_RIGHTARM);
						}
						else
						{
							if ( setGloveSprite(stats[PLAYER_NUM], entity, SPRITE_GLOVE_RIGHT_OFFSET) != 0 )
							{
								// successfully set sprite for the human model
							}
						}
						if ( (!PLAYER_ARMBENDED && showEquipment) || (insectoidLevitating && PLAYER_ATTACK == 0 && PLAYER_ATTACKTIME == 0) )
						{
							entity->sprite += 2 * (stats[PLAYER_NUM]->weapon != NULL);

							if ( stats[PLAYER_NUM]->weapon == nullptr
								&& insectoidLevitating )
							{
								entity->sprite += 2;
							}
						}
						if ( multiplayer == SERVER )
						{
							// update sprites for clients
							if ( entity->skill[10] != entity->sprite )
							{
								entity->skill[10] = entity->sprite;
								serverUpdateEntityBodypart(my, bodypart);
							}
							if ( PLAYER_ALIVETIME == TICKS_PER_SECOND + bodypart )
							{
								serverUpdateEntityBodypart(my, bodypart);
							}
						}
					}
					my->setHumanoidLimbOffset(entity, playerRace, LIMB_HUMANOID_RIGHTARM);
					node_t* tempNode = list_Node(&my->children, 6);
					if ( tempNode )
					{
						Entity* weapon = (Entity*)tempNode->element;
						if ( weapon->flags[INVISIBLE] || PLAYER_ARMBENDED || playerRace == CREATURE_IMP )
						{
							if ( playerRace == INCUBUS || playerRace == SUCCUBUS )
							{
								entity->focalx = limbs[playerRace][4][0] - 0.25;
								entity->focaly = limbs[playerRace][4][1] - 0.25;
								entity->focalz = limbs[playerRace][4][2];
							}
							else
							{
								entity->focalx = limbs[playerRace][4][0]; // 0
								entity->focaly = limbs[playerRace][4][1]; // 0
								entity->focalz = limbs[playerRace][4][2]; // 1.5
							}
						}
						else
						{
							if ( playerRace == INCUBUS || playerRace == SUCCUBUS )
							{
								entity->focalx = limbs[playerRace][4][0];
								entity->focaly = limbs[playerRace][4][1];
								entity->focalz = limbs[playerRace][4][2];
							}
							else if ( playerRace == AUTOMATON )
							{
								entity->focalx = limbs[playerRace][4][0] + 1.5; // 1
								entity->focaly = limbs[playerRace][4][1] + 0.25; // 0
								entity->focalz = limbs[playerRace][4][2] - 1; // 1
							}
							else
							{
								entity->focalx = limbs[playerRace][4][0] + 0.75;
								entity->focaly = limbs[playerRace][4][1];
								entity->focalz = limbs[playerRace][4][2] - 0.75;
							}
						}
					}
					entity->yaw += PLAYER_WEAPONYAW;
					break;
				}
				// left arm
				case 5:
				{
					if ( multiplayer != CLIENT )
					{
						if ( stats[PLAYER_NUM]->gloves == NULL || !showEquipment )
						{
							entity->setDefaultPlayerModel(PLAYER_NUM, playerRace, LIMB_HUMANOID_LEFTARM);
						}
						else
						{
							if ( setGloveSprite(stats[PLAYER_NUM], entity, SPRITE_GLOVE_LEFT_OFFSET) != 0 )
							{
								// successfully set sprite for the human model
							}
						}
						if ( showEquipment )
						{
							bool bendArm = false;
							if ( insectoidLevitating )
							{
								bendArm = true;
							}
							if ( stats[PLAYER_NUM]->shield != NULL )
							{
								if ( itemCategory(stats[PLAYER_NUM]->shield) == SPELLBOOK )
								{
									bendArm = false;
								}
								else
								{
									bendArm = true;
								}
							}
							entity->sprite += 2 * (bendArm);
						}
						if ( multiplayer == SERVER )
						{
							// update sprites for clients
							if ( entity->skill[10] != entity->sprite )
							{
								entity->skill[10] = entity->sprite;
								serverUpdateEntityBodypart(my, bodypart);
							}
							if ( PLAYER_ALIVETIME == TICKS_PER_SECOND + bodypart )
							{
								serverUpdateEntityBodypart(my, bodypart);
							}
						}
					}
					my->setHumanoidLimbOffset(entity, playerRace, LIMB_HUMANOID_LEFTARM);

					if ( weaponarm && !showEquipment &&
						(PLAYER_ATTACK == MONSTER_POSE_SPECIAL_WINDUP1 
							|| PLAYER_ATTACK == MONSTER_POSE_SPECIAL_WINDUP2
							|| PLAYER_ATTACK == PLAYER_POSE_GOLEM_SMASH))
					{
						// special swing - copy the right arm movements.
						entity->pitch = weaponarm->pitch;
						entity->roll = -weaponarm->roll;
					}
					else
					{
						entity->roll = 0.f;
					}

					node_t* tempNode = list_Node(&my->children, 7);
					if ( tempNode )
					{
						Entity* shield = (Entity*)tempNode->element;
						bool bendArm = true;
						if ( shield->flags[INVISIBLE] )
						{
							bendArm = false;
							if ( insectoidLevitating )
							{
								bendArm = true;
							}
						}
						else if ( shield->sprite >= items[SPELLBOOK_LIGHT].index
							&& shield->sprite < (items[SPELLBOOK_LIGHT].index + items[SPELLBOOK_LIGHT].variations) )
						{
							bendArm = false;
						}
						else if ( playerRace == CREATURE_IMP )
						{
							bendArm = false;
						}
						
						if ( !bendArm )
						{
							if ( playerRace == INCUBUS || playerRace == SUCCUBUS )
							{
								entity->focalx = limbs[playerRace][5][0] - 0.25;
								entity->focaly = limbs[playerRace][5][1] + 0.25;
								entity->focalz = limbs[playerRace][5][2];
							}
							else
							{
								entity->focalx = limbs[playerRace][5][0]; // 0
								entity->focaly = limbs[playerRace][5][1]; // 0
								entity->focalz = limbs[playerRace][5][2]; // 1.5
							}
						}
						else
						{
							if ( playerRace == INCUBUS || playerRace == SUCCUBUS )
							{
								entity->focalx = limbs[playerRace][5][0];
								entity->focaly = limbs[playerRace][5][1];
								entity->focalz = limbs[playerRace][5][2];
							}
							else if ( playerRace == AUTOMATON )
							{
								entity->focalx = limbs[playerRace][5][0] + 1.5; // 1
								entity->focaly = limbs[playerRace][5][1] - 0.25; // 0
								entity->focalz = limbs[playerRace][5][2] - 1; // 1
							}
							else
							{
								entity->focalx = limbs[playerRace][5][0] + 0.75;
								entity->focaly = limbs[playerRace][5][1];
								entity->focalz = limbs[playerRace][5][2] - 0.75;
							}
						}
					}
					if ( multiplayer != CLIENT )
					{
						real_t prevYaw = PLAYER_SHIELDYAW;
						if ( stats[PLAYER_NUM]->defending )
						{
							PLAYER_SHIELDYAW = PI / 5;
						}
						else
						{
							PLAYER_SHIELDYAW = 0;
						}
						if ( prevYaw != PLAYER_SHIELDYAW || ticks % 200 == 0 )
						{
							serverUpdateEntityFSkill(players[PLAYER_NUM]->entity, 8);
						}
					}
					entity->yaw += PLAYER_SHIELDYAW;
					break;
				}
				// weapon
				case 6:
					if ( multiplayer != CLIENT )
					{
						if ( swimming )
						{
							entity->flags[INVISIBLE] = true;
						}
						else
						{
							if ( stats[PLAYER_NUM]->weapon == NULL || my->isInvisible() )
							{
								entity->flags[INVISIBLE] = true;
							}
							else
							{
								entity->sprite = itemModel(stats[PLAYER_NUM]->weapon);
								if ( itemCategory(stats[PLAYER_NUM]->weapon) == SPELLBOOK )
								{
									entity->flags[INVISIBLE] = true;
								}
								else
								{
									entity->flags[INVISIBLE] = false;
								}
							}
						}
						if ( multiplayer == SERVER )
						{
							// update sprites for clients
							if ( entity->skill[10] != entity->sprite )
							{
								entity->skill[10] = entity->sprite;
								serverUpdateEntityBodypart(my, bodypart);
							}
							if ( entity->skill[11] != entity->flags[INVISIBLE] )
							{
								entity->skill[11] = entity->flags[INVISIBLE];
								serverUpdateEntityBodypart(my, bodypart);
							}
							if ( PLAYER_ALIVETIME == TICKS_PER_SECOND + bodypart )
							{
								serverUpdateEntityBodypart(my, bodypart);
							}
						}
					}
					else
					{
						if ( entity->sprite <= 0 )
						{
							entity->flags[INVISIBLE] = true;
						}
					}
					my->handleHumanoidWeaponLimb(entity, weaponarm);
					break;
				// shield
				case 7:
					if ( multiplayer != CLIENT )
					{
						if ( swimming )
						{
							entity->flags[INVISIBLE] = true;
						}
						else
						{
							if ( stats[PLAYER_NUM]->shield == NULL )
							{
								entity->flags[INVISIBLE] = true;
								entity->sprite = 0;
							}
							else
							{
								entity->flags[INVISIBLE] = false;
								entity->sprite = itemModel(stats[PLAYER_NUM]->shield);
								if ( itemTypeIsQuiver(stats[PLAYER_NUM]->shield->type) )
								{
									if ( itemTypeIsQuiver(stats[PLAYER_NUM]->shield->type) )
									{
										entity->handleQuiverThirdPersonModel(*stats[PLAYER_NUM]);
									}
								}
							}
							if ( my->isInvisible() )
							{
								entity->flags[INVISIBLE] = true;
							}
						}
						if ( multiplayer == SERVER )
						{
							// update sprites for clients
							if ( entity->skill[10] != entity->sprite )
							{
								entity->skill[10] = entity->sprite;
								serverUpdateEntityBodypart(my, bodypart);
							}
							if ( entity->skill[11] != entity->flags[INVISIBLE] )
							{
								entity->skill[11] = entity->flags[INVISIBLE];
								serverUpdateEntityBodypart(my, bodypart);
							}
							if ( PLAYER_ALIVETIME == TICKS_PER_SECOND + bodypart )
							{
								serverUpdateEntityBodypart(my, bodypart);
							}
						}
					}
					else
					{
						if ( entity->sprite <= 0 )
						{
							entity->flags[INVISIBLE] = true;
						}
					}
					my->handleHumanoidShieldLimb(entity, shieldarm);
					break;
					// cloak
				case 8:
					entity->focalx = limbs[playerRace][8][0];
					entity->focaly = limbs[playerRace][8][1];
					entity->focalz = limbs[playerRace][8][2];
					entity->scalex = 1.01;
					entity->scaley = 1.01;
					if ( multiplayer != CLIENT )
					{
						if ( stats[PLAYER_NUM]->cloak == NULL || my->isInvisible() )
						{
							entity->flags[INVISIBLE] = true;
						}
						else
						{
							entity->flags[INVISIBLE] = false;
							entity->sprite = itemModel(stats[PLAYER_NUM]->cloak);
						}
						if ( multiplayer == SERVER )
						{
							// update sprites for clients
							if ( entity->skill[10] != entity->sprite )
							{
								entity->skill[10] = entity->sprite;
								serverUpdateEntityBodypart(my, bodypart);
							}
							if ( entity->skill[11] != entity->flags[INVISIBLE] )
							{
								entity->skill[11] = entity->flags[INVISIBLE];
								serverUpdateEntityBodypart(my, bodypart);
							}
							if ( PLAYER_ALIVETIME == TICKS_PER_SECOND + bodypart )
							{
								serverUpdateEntityBodypart(my, bodypart);
							}
						}
					}
					else
					{
						if ( entity->sprite <= 0 )
						{
							entity->flags[INVISIBLE] = true;
						}
					}

					if ( entity->sprite == items[CLOAK_BACKPACK].index )
					{
						// human
						if ( playerRace == HUMAN || playerRace == VAMPIRE )
						{
							entity->focaly = limbs[playerRace][8][1] + 0.25;
							entity->focalz = limbs[playerRace][8][2] - 0.3;
						}
						else if ( playerRace == SUCCUBUS || playerRace == INCUBUS )
						{
							// succubus/incubus
							entity->focaly = limbs[playerRace][8][1] + 0.25;
							entity->focalz = limbs[playerRace][8][2] - 0.7;
						}
						else if ( playerRace == SKELETON )
						{
							entity->focaly = limbs[playerRace][8][1] + 0.25;
							entity->focalz = limbs[playerRace][8][2] - 0.5;
						}
						else if ( playerRace == AUTOMATON )
						{
							entity->focaly = limbs[playerRace][8][1] - 0.25;
							entity->focalz = limbs[playerRace][8][2] - 0.5;
						}
						else if ( playerRace == GOATMAN || playerRace == INSECTOID || playerRace == GOBLIN )
						{
							entity->focaly = limbs[playerRace][8][1] - 0.25;
							entity->focalz = limbs[playerRace][8][2] - 0.5;
						}

						entity->scalex = 0.99;
						entity->scaley = 0.99;
					}
					entity->x -= cos(my->yaw);
					entity->y -= sin(my->yaw);
					entity->yaw += PI / 2;
					break;
					// helm
				case 9:
					helmet = entity;
					entity->focalx = limbs[playerRace][9][0]; // 0
					entity->focaly = limbs[playerRace][9][1]; // 0
					entity->focalz = limbs[playerRace][9][2]; // -1.75
					entity->pitch = my->pitch;
					entity->roll = 0;
					if ( multiplayer != CLIENT )
					{
						entity->sprite = itemModel(stats[PLAYER_NUM]->helmet);
						if ( stats[PLAYER_NUM]->helmet == NULL || my->isInvisible() )
						{
							entity->flags[INVISIBLE] = true;
						}
						else
						{
							entity->flags[INVISIBLE] = false;
						}
						if ( multiplayer == SERVER )
						{
							// update sprites for clients
							if ( entity->skill[10] != entity->sprite )
							{
								entity->skill[10] = entity->sprite;
								serverUpdateEntityBodypart(my, bodypart);
							}
							if ( entity->skill[11] != entity->flags[INVISIBLE] )
							{
								entity->skill[11] = entity->flags[INVISIBLE];
								serverUpdateEntityBodypart(my, bodypart);
							}
							if ( PLAYER_ALIVETIME == TICKS_PER_SECOND + bodypart )
							{
								serverUpdateEntityBodypart(my, bodypart);
							}
						}
					}
					else
					{
						if ( entity->sprite <= 0 )
						{
							entity->flags[INVISIBLE] = true;
						}
					}
					my->setHelmetLimbOffset(entity);
					break;
					// mask
				case 10:
					entity->focalx = limbs[playerRace][10][0]; // 0
					entity->focaly = limbs[playerRace][10][1]; // 0
					entity->focalz = limbs[playerRace][10][2]; // .5
					entity->pitch = my->pitch;
					entity->roll = PI / 2;
					if ( multiplayer != CLIENT )
					{
						bool hasSteelHelm = false;
						if ( stats[PLAYER_NUM]->helmet )
						{
							if ( stats[PLAYER_NUM]->helmet->type == STEEL_HELM
								|| stats[PLAYER_NUM]->helmet->type == CRYSTAL_HELM 
								|| stats[PLAYER_NUM]->helmet->type == ARTIFACT_HELM )
							{
								hasSteelHelm = true;
							}
						}
						if ( stats[PLAYER_NUM]->mask == NULL || my->isInvisible() || hasSteelHelm )
						{
							entity->flags[INVISIBLE] = true;
						}
						else
						{
							entity->flags[INVISIBLE] = false;
						}
						if ( stats[PLAYER_NUM]->mask != NULL )
						{
							if ( stats[PLAYER_NUM]->mask->type == TOOL_GLASSES )
							{
								entity->sprite = 165; // GlassesWorn.vox
							}
							else if ( stats[PLAYER_NUM]->mask->type == MONOCLE )
							{
								entity->sprite = 1196; // MonocleWorn.vox
							}
							else
							{
								entity->sprite = itemModel(stats[PLAYER_NUM]->mask);
							}
						}
						if ( multiplayer == SERVER )
						{
							// update sprites for clients
							if ( entity->skill[10] != entity->sprite )
							{
								entity->skill[10] = entity->sprite;
								serverUpdateEntityBodypart(my, bodypart);
							}
							if ( entity->skill[11] != entity->flags[INVISIBLE] )
							{
								entity->skill[11] = entity->flags[INVISIBLE];
								serverUpdateEntityBodypart(my, bodypart);
							}
							if ( PLAYER_ALIVETIME == TICKS_PER_SECOND + bodypart )
							{
								serverUpdateEntityBodypart(my, bodypart);
							}
						}
					}
					else
					{
						if ( entity->sprite <= 0 )
						{
							entity->flags[INVISIBLE] = true;
						}
					}
					entity->scalex = 0.99;
					entity->scaley = 0.99;
					entity->scalez = 0.99;
					if ( entity->sprite == 165 || entity->sprite == 1196 )
					{
						entity->focalx = limbs[playerRace][10][0] + .25; // .25
						entity->focaly = limbs[playerRace][10][1] - 2.25; // -2.25
						entity->focalz = limbs[playerRace][10][2]; // .5
						if ( entity->sprite == 1196 ) // MonocleWorn.vox
						{
							switch ( playerRace )
							{
								case INCUBUS:
									entity->focalx -= .4;
									break;
								case SUCCUBUS:
									entity->focalx -= .25;
									break;
								case GOBLIN:
									entity->focalx -= .5;
									entity->focalz -= .05;
									break;
								default:
									break;
							}
						}
						if ( helmet && !helmet->flags[INVISIBLE] && helmet->sprite == items[PUNISHER_HOOD].index )
						{
							switch ( playerRace )
							{
								case HUMAN:
								case VAMPIRE:
								case SHOPKEEPER:
								case INSECTOID:
									entity->focaly += 0.25; // lower glasses a bit.
									break;
								case INCUBUS:
								case SUCCUBUS:
								case AUTOMATON:
								case GOBLIN:
								case GOATMAN:
								case SKELETON:
									// no change.
									break;
								default:
									break;
							}
						}
					}
					else if ( entity->sprite == items[MASK_SHAMAN].index )
					{
						entity->roll = 0;
						my->setHelmetLimbOffset(entity);
						my->setHelmetLimbOffsetWithMask(helmet, entity);
					}
					else
					{
						entity->focalx = limbs[playerRace][10][0] + .35; // .35
						entity->focaly = limbs[playerRace][10][1] - 2; // -2
						entity->focalz = limbs[playerRace][10][2]; // .5
					}

					break;
				case 11:
					additionalLimb = entity;
					entity->focalx = limbs[playerRace][11][0];
					entity->focaly = limbs[playerRace][11][1];
					entity->focalz = limbs[playerRace][11][2];
					entity->flags[INVISIBLE] = true;
					if ( playerRace == INSECTOID || playerRace == CREATURE_IMP )
					{
						entity->flags[INVISIBLE] = my->flags[INVISIBLE];
						if ( playerRace == INSECTOID )
						{
							if ( stats[PLAYER_NUM]->sex == FEMALE )
							{
								entity->sprite = 771;
							}
							else
							{
								entity->sprite = 750;
							}
							if ( torso && torso->sprite != 727 && torso->sprite != 761 && torso->sprite != 458 )
							{
								// wearing armor, offset more.
								entity->x -= 2.25 * cos(my->yaw);
								entity->y -= 2.25 * sin(my->yaw);
							}
							else
							{
								entity->x -= 1.5 * cos(my->yaw);
								entity->y -= 1.5 * sin(my->yaw);
							}
						}
						else if ( playerRace == CREATURE_IMP )
						{
							entity->sprite = 833;
							entity->focalx = limbs[playerRace][7][0];
							entity->focaly = limbs[playerRace][7][1];
							entity->focalz = limbs[playerRace][7][2];
							entity->x -= 1 * cos(my->yaw + PI / 2) + 2.5 * cos(my->yaw);
							entity->y -= 1 * sin(my->yaw + PI / 2) + 2.5 * sin(my->yaw);
							entity->z += 1;
						}
						bool moving = false;
						if ( fabs(PLAYER_VELX) > 0.1 || fabs(PLAYER_VELY) > 0.1 || insectoidLevitating )
						{
							moving = true;
						}

						if ( entity->skill[0] == 0 )
						{
							if ( moving )
							{
								if ( insectoidLevitating )
								{
									entity->fskill[0] += std::min(std::max(0.2, dist * PLAYERWALKSPEED), 2.f * PLAYERWALKSPEED); // move proportional to move speed
								}
								else
								{
									entity->fskill[0] += std::min(dist * PLAYERWALKSPEED, 2.f * PLAYERWALKSPEED); // move proportional to move speed
								}
							}
							else if ( PLAYER_ATTACK != 0 )
							{
								entity->fskill[0] += PLAYERWALKSPEED; // move fixed speed when attacking if stationary
							}
							else
							{
								entity->fskill[0] += 0.01; // otherwise move slow idle
							}

							if ( entity->fskill[0] > PI / 3 
								|| ((!moving || PLAYER_ATTACK != 0) && entity->fskill[0] > PI / 5)
								|| (playerRace == CREATURE_IMP && entity->fskill[0] > PI / 8) )
							{
								// switch direction if angle too great, angle is shorter if attacking or stationary
								entity->skill[0] = 1;
							}
						}
						else // reverse of the above
						{
							if ( moving )
							{
								if ( insectoidLevitating )
								{
									entity->fskill[0] -= std::min(std::max(0.15, dist * PLAYERWALKSPEED), 2.f * PLAYERWALKSPEED);
								}
								else
								{
									entity->fskill[0] -= std::min(dist * PLAYERWALKSPEED, 2.f * PLAYERWALKSPEED);
								}
							}
							else if ( PLAYER_ATTACK != 0 )
							{
								entity->fskill[0] -= PLAYERWALKSPEED;
							}
							else
							{
								entity->fskill[0] -= 0.007;
							}

							if ( playerRace == INSECTOID && entity->fskill[0] < -PI / 32 )
							{
								entity->skill[0] = 0;
							}
							else if ( playerRace == CREATURE_IMP && entity->fskill[0] < -PI / 4 )
							{
								entity->skill[0] = 0;
							}
						}
						entity->yaw += entity->fskill[0];
					}
					break;
				case 12:
					entity->focalx = limbs[playerRace][12][0];
					entity->focaly = limbs[playerRace][12][1];
					entity->focalz = limbs[playerRace][12][2];
					entity->flags[INVISIBLE] = true;
					if ( playerRace == INSECTOID || playerRace == CREATURE_IMP )
					{
						entity->flags[INVISIBLE] = my->flags[INVISIBLE];
						if ( playerRace == INSECTOID )
						{
							if ( stats[PLAYER_NUM]->sex == FEMALE )
							{
								entity->sprite = 772;
							}
							else
							{
								entity->sprite = 751;
							}
							if ( torso && torso->sprite != 727 && torso->sprite != 761 && torso->sprite != 458 )
							{
								// wearing armor, offset more.
								entity->x -= 2.25 * cos(my->yaw);
								entity->y -= 2.25 * sin(my->yaw);
							}
							else
							{
								entity->x -= 1.5 * cos(my->yaw);
								entity->y -= 1.5 * sin(my->yaw);
							}
						}
						else if ( playerRace == CREATURE_IMP )
						{
							entity->sprite = 834;
							entity->focalx = limbs[playerRace][6][0];
							entity->focaly = limbs[playerRace][6][1];
							entity->focalz = limbs[playerRace][6][2];
							entity->x += 1 * cos(my->yaw + PI / 2) - 2.5 * cos(my->yaw);
							entity->y += 1 * sin(my->yaw + PI / 2) - 2.5 * sin(my->yaw);
							entity->z += 1;
						}
						if ( additionalLimb ) // follow the yaw of the previous limb.
						{
							entity->yaw -= additionalLimb->fskill[0];
						}
					}
					break;
				default:
					break;
			}


			if ( !showEquipment )
			{
				if ( bodypart >= 6 && bodypart <= 10 )
				{
					entity->flags[INVISIBLE] = true;
					if ( playerRace == CREATURE_IMP && bodypart == 7 )
					{
						if ( entity->sprite >= items[SPELLBOOK_LIGHT].index
							&& entity->sprite < (items[SPELLBOOK_LIGHT].index + items[SPELLBOOK_LIGHT].variations) )
						{
							entity->flags[INVISIBLE] = false; // show spellbooks
						}
					}
					else if ( playerRace == CREATURE_IMP && bodypart == 6 )
					{
						if ( entity->sprite >= 59 && entity->sprite < 64 )
						{
							entity->flags[INVISIBLE] = false; // show magicstaffs
						}
						if ( multiplayer != CLIENT && !stats[PLAYER_NUM]->weapon )
						{
							entity->flags[INVISIBLE] = true; // show magicstaffs
						}
					}
				}
			}
		}
		// rotate shield a bit
		node_t* shieldNode = list_Node(&my->children, 7);
		if ( shieldNode )
		{
			Entity* shieldEntity = (Entity*)shieldNode->element;
			if ( shieldEntity->sprite != items[TOOL_TORCH].index && shieldEntity->sprite != items[TOOL_LANTERN].index && shieldEntity->sprite != items[TOOL_CRYSTALSHARD].index )
			{
				shieldEntity->yaw -= PI / 6;
			}
		}
	}
	else
	{
		if ( stats[PLAYER_NUM]->type == RAT )
		{
			playerAnimateRat(my);
		}
		else if ( stats[PLAYER_NUM]->type == SPIDER )
		{
			playerAnimateSpider(my);
		}
	}
	if ( PLAYER_ATTACK != 0 )
	{
		PLAYER_ATTACKTIME++;
	}
	else
	{
		PLAYER_ATTACKTIME = 0;
	}

	if ( !usecamerasmoothing )
	{
		players[PLAYER_NUM]->movement.handlePlayerCameraPosition(false);
	}
}

// client function
void actPlayerLimb(Entity* my)
{
	int i;

	Entity* parent = uidToEntity(my->parent);

	if ( multiplayer == CLIENT )
	{
		if ( stats[PLAYER_NUM]->HP <= 0 )
		{
			if ( parent && parent->getMonsterTypeFromSprite() == AUTOMATON )
			{
				if ( parent->playerAutomatonDeathCounter != 0 )
				{
					my->flags[INVISIBLE] = false;
				}
				else
				{
					my->flags[INVISIBLE] = true;
				}
			}
			else
			{
				my->flags[INVISIBLE] = true;
			}
			return;
		}
	}

	if ( parent && multiplayer != CLIENT )
	{
		for ( i = 0; i < MAXPLAYERS; i++ )
		{
			if ( inrange[i] )
			{
				if ( i == 0 && selectedEntity[0] == my )
				{
					parent->skill[14] = i + 1;
				}
				else if ( client_selected[i] == my )
				{
					parent->skill[14] = i + 1;
				}
				else if ( i > 0 && splitscreen && selectedEntity[i] == my )
				{
					parent->skill[14] = i + 1;
				}
			}
		}
	}

	if ( parent && parent->monsterEntityRenderAsTelepath == 1 )
	{
		my->monsterEntityRenderAsTelepath = 1;
	}
	else
	{
		my->monsterEntityRenderAsTelepath = 0;
	}

	if (multiplayer != CLIENT)
	{
		return;
	}

	if (my->skill[2] < 0 || my->skill[2] >= MAXPLAYERS )
	{
		return;
	}
	if (players[my->skill[2]] == nullptr || players[my->skill[2]]->entity == nullptr)
	{
		list_RemoveNode(my->mynode);
		return;
	}

	//TODO: These three are _NOT_ PLAYERSWAP
	//my->vel_x = players[my->skill[2]]->vel_x;
	//my->vel_y = players[my->skill[2]]->vel_y;
	//my->vel_z = players[my->skill[2]]->vel_z;

	if ( stats[PLAYER_NUM] )
	{
		if ( stats[PLAYER_NUM]->type == RAT
			|| stats[PLAYER_NUM]->type == SPIDER
			|| stats[PLAYER_NUM]->type == TROLL
			|| stats[PLAYER_NUM]->type == CREATURE_IMP )
		{
			players[PLAYER_NUM]->entity->skill[1] = 0;
			return;
		}
	}

	// player lights for clients
    if (my->sprite == 93) {
        // PLAYER_TORCH = 1 aka torch
		players[my->skill[2]]->entity->skill[1] = 1;
        my->skill[4] = 1; // lets us know that this is indeed the off-hand item (shield)
	}
    else if (my->sprite == 94) {
        // PLAYER_TORCH = 2 aka lantern
		players[my->skill[2]]->entity->skill[1] = 2;
        my->skill[4] = 1; // lets us know that this is indeed the off-hand item (shield)
	}
    else if ( my->sprite == 529 ) {
        // PLAYER_TORCH = 3 aka crystal shard
		players[my->skill[2]]->entity->skill[1] = 3;
        my->skill[4] = 1; // lets us know that this is indeed the off-hand item (shield)
	}
	else {
        // PLAYER_TORCH = 4 aka none
        if (my->skill[4]) {
            // we have to check skill[4] because if this logic runs for every limb,
            // the others will override the PLAYER_TORCH value set by the shield.
            // skill[4] is only ever set by the shield. so this is a convenient workaround
            players[my->skill[2]]->entity->skill[1] = 0;
        }
	}
}

void Entity::playerLevelEntrySpeechSecond()
{
	int timeDiff = playerAliveTime - 300;
	int orangeSpeechVolume = 128;
	int blueSpeechVolume = 112;
	if ( timeDiff > 0 && playerLevelEntrySpeech > 0 && !secretlevel )
	{
		switch ( currentlevel )
		{
			case 26:
				switch ( playerLevelEntrySpeech )
				{
					case 1:
						if ( timeDiff == 200 )
						{
							playSound(342, orangeSpeechVolume);
							messageLocalPlayersColor(uint32ColorOrange, MESSAGE_WORLD, Language::get(2616));
							playerLevelEntrySpeech = 0;
						}
						break;
					case 2:
						if ( timeDiff == 200 )
						{
							playSound(344, blueSpeechVolume);
							messageLocalPlayersColor(uint32ColorBaronyBlue, MESSAGE_WORLD, Language::get(2618));
						}
						else if ( timeDiff == 350 )
						{
							playSound(345, orangeSpeechVolume);
							messageLocalPlayersColor(uint32ColorOrange, MESSAGE_WORLD, Language::get(2619));
							playerLevelEntrySpeech = 0;
						}
						break;
					case 3:
						if ( timeDiff == 200 )
						{
							playSound(347, blueSpeechVolume);
							messageLocalPlayersColor(uint32ColorBaronyBlue, MESSAGE_WORLD, Language::get(2621));
						}
						else if ( timeDiff == 350 )
						{
							playSound(348, orangeSpeechVolume);
							messageLocalPlayersColor(uint32ColorOrange, MESSAGE_WORLD, Language::get(2622));
							playerLevelEntrySpeech = 0;
						}
						break;
					default:
						break;
				}
				break;
			case 28:
				switch ( playerLevelEntrySpeech )
				{
					case 1:
						if ( timeDiff == 200 )
						{
							playSound(350, orangeSpeechVolume);
							messageLocalPlayersColor(uint32ColorOrange, MESSAGE_WORLD, Language::get(2630));
						}
						else if ( timeDiff == 350 )
						{
							playSound(351, blueSpeechVolume);
							messageLocalPlayersColor(uint32ColorBaronyBlue, MESSAGE_WORLD, Language::get(2631));
							playerLevelEntrySpeech = 0;
						}
						break;
					case 2:
						if ( timeDiff == 350 )
						{
							playSound(353, blueSpeechVolume);
							messageLocalPlayersColor(uint32ColorBaronyBlue, MESSAGE_WORLD, Language::get(2633));
							playerLevelEntrySpeech = 0;
						}
						break;
					case 3:
						if ( timeDiff == 200 )
						{
							playSound(355, orangeSpeechVolume);
							messageLocalPlayersColor(uint32ColorOrange, MESSAGE_WORLD, Language::get(2635));
							playerLevelEntrySpeech = 0;
						}
						break;
					default:
						break;
				}
				break;
			case 30:
				switch ( playerLevelEntrySpeech )
				{
					case 1:
						if ( timeDiff == 350 )
						{
							playSound(357, orangeSpeechVolume);
							messageLocalPlayersColor(uint32ColorOrange, MESSAGE_WORLD, Language::get(2637));
						}
						else if ( timeDiff == 500 )
						{
							messageLocalPlayersColor(uint32ColorOrange, MESSAGE_WORLD, Language::get(2652));
							playerLevelEntrySpeech = 0;
						}
						break;
					default:
						break;
				}
				break;
			case 31:
				switch ( playerLevelEntrySpeech )
				{
					case 1:
						if ( timeDiff == 350 )
						{
							playSound(359, orangeSpeechVolume);
							messageLocalPlayersColor(uint32ColorOrange, MESSAGE_WORLD, Language::get(2639));
						}
						else if ( timeDiff == 510 )
						{
							messageLocalPlayersColor(uint32ColorOrange, MESSAGE_WORLD, Language::get(2653));
							playerLevelEntrySpeech = 0;
						}
						break;
					default:
						break;
				}
				break;
			case 33:
				switch ( playerLevelEntrySpeech )
				{
					case 1:
						if ( timeDiff == 200 )
						{
							playSound(361, blueSpeechVolume);
							messageLocalPlayersColor(uint32ColorBaronyBlue, MESSAGE_WORLD, Language::get(2641));
							playerLevelEntrySpeech = 0;
						}
						break;
					case 2:
						if ( timeDiff == 350 )
						{
							playSound(363, orangeSpeechVolume);
							messageLocalPlayersColor(uint32ColorOrange, MESSAGE_WORLD, Language::get(2643));
							playerLevelEntrySpeech = 0;
						}
						break;
					default:
						break;
				}
				break;
			case 35:
				switch ( playerLevelEntrySpeech )
				{
					case 1:
						if ( timeDiff == 310 )
						{
							playSound(365, blueSpeechVolume);
							messageLocalPlayersColor(uint32ColorBaronyBlue, MESSAGE_WORLD, Language::get(2645));
							playerLevelEntrySpeech = 0;
						}
						break;
					default:
						break;
				}
				break;
			case 27:
			case 29:
			case 32:
			case 34:
				if ( minotaurlevel )
				{
					switch ( playerLevelEntrySpeech )
					{
						case 1:
							if ( timeDiff == 200 )
							{
								playSound(367, orangeSpeechVolume);
								messageLocalPlayersColor(uint32ColorOrange, MESSAGE_WORLD, Language::get(2624));
								playerLevelEntrySpeech = 0;
							}
							break;
						case 2:
							if ( timeDiff == 200 )
							{
								playSound(369, blueSpeechVolume);
								messageLocalPlayersColor(uint32ColorBaronyBlue, MESSAGE_WORLD, Language::get(2626));
								playerLevelEntrySpeech = 0;
							}
							break;
						case 3:
							if ( timeDiff == 200 )
							{
								playSound(371, orangeSpeechVolume);
								messageLocalPlayersColor(uint32ColorOrange, MESSAGE_WORLD, Language::get(2628));
								playerLevelEntrySpeech = 0;
							}
							break;
						default:
							break;
					}
					break;
				}
				break;
			default:
				break;
		}

	}
}

bool Entity::isPlayerHeadSprite(const int sprite)
{
	switch ( sprite )
	{
		case 113:
		case 114:
		case 115:
		case 116:
		case 117:
		case 125:
		case 126:
		case 127:
		case 128:
		case 129:
		case 332:
		case 333:
		case 341:
		case 342:
		case 343:
		case 344:
		case 345:
		case 346:
		case 354:
		case 355:
		case 356:
		case 357:
		case 358:
		case 359:
		case 367:
		case 368:
		case 369:
		case 370:
		case 371:
		case 372:
		case 380:
		case 381:
		case 382:
		case 383:
		case 384:
		case 385:
		case 686:
		case 694:
		case 702:
		case 710:
		case 718:
		case 726:
		case 734:
		case 742:
		case 752:
		case 756:
		case 760:
		case 768:
		case 770:
		case 814:
		case 817:
		case 823:
		case 827:
		case 1001:
		case 1043:
		case 1044:
		case 1049:
			return true;
			break;
		default:
			break;
	}
	return false;
}

bool Entity::isPlayerHeadSprite() const
{
	return Entity::isPlayerHeadSprite(sprite);
}

Monster getMonsterFromPlayerRace(int playerRace)
{
	switch ( playerRace )
	{
		case RACE_HUMAN:
			return HUMAN;
			break;
		case RACE_SKELETON:
			return SKELETON;
			break;
		case RACE_INCUBUS:
			return INCUBUS;
			break;
		case RACE_GOBLIN:
			return GOBLIN;
			break;
		case RACE_AUTOMATON:
			return AUTOMATON;
			break;
		case RACE_INSECTOID:
			return INSECTOID;
			break;
		case RACE_GOATMAN:
			return GOATMAN;
			break;
		case RACE_VAMPIRE:
			return VAMPIRE;
			break;
		case RACE_SUCCUBUS:
			return SUCCUBUS;
			break;
		case RACE_RAT:
			return RAT;
			break;
		case RACE_TROLL:
			return TROLL;
			break;
		case RACE_SPIDER:
			return SPIDER;
			break;
		case RACE_IMP:
			return CREATURE_IMP;
			break;
		default:
			return HUMAN;
			break;
	}
	return HUMAN;
}

Monster Entity::getMonsterFromPlayerRace(int playerRace)
{
    return ::getMonsterFromPlayerRace(playerRace);
}

void Entity::setDefaultPlayerModel(int playernum, Monster playerRace, int limbType)
{
	if ( !players[playernum] || !players[playernum]->entity )
	{
		return;
	}

	int playerAppearance = stats[playernum]->appearance;
	if ( players[playernum] && players[playernum]->entity && players[playernum]->entity->effectPolymorph > NUMMONSTERS )
	{
		playerAppearance = players[playernum]->entity->effectPolymorph - 100;
	}

	if ( limbType == LIMB_HUMANOID_TORSO )
	{
		if ( playerRace == HUMAN )
		{
			switch ( playerAppearance / 6 )
			{
				case 1:
					this->sprite = 334 + 13 * stats[playernum]->sex;
					break;
				case 2:
					this->sprite = 360 + 13 * stats[playernum]->sex;
					break;
				default:
					this->sprite = 106 + 12 * stats[playernum]->sex;
					break;
			}
		}
		else
		{
			switch ( playerRace )
			{
				case SKELETON:
					this->sprite = stats[playernum]->sex == FEMALE ? 1052 : 687;
					break;
				case GOBLIN:
					if ( stats[playernum]->sex == FEMALE )
					{
						this->sprite = 753;
					}
					else
					{
						this->sprite = 695;
					}
					break;
				case CREATURE_IMP:
					this->sprite = 828;
					break;
				case INCUBUS:
					this->sprite = 703;
					break;
				case SUCCUBUS:
					this->sprite = 711;
					break;
				case VAMPIRE:
					if ( stats[playernum]->sex == FEMALE )
					{
						this->sprite = 757;
					}
					else
					{
						this->sprite = 719;
					}
					break;
				case INSECTOID:
					if ( stats[playernum]->sex == FEMALE )
					{
						this->sprite = 761;
					}
					else
					{
						this->sprite = 727;
					}
					break;
				case GOATMAN:
					if ( stats[playernum]->sex == FEMALE )
					{
						this->sprite = 769;
					}
					else
					{
						this->sprite = 735;
					}
					break;
				case AUTOMATON:
					this->sprite = 743;
					break;
				case TROLL:
					this->sprite = 818;
					break;
				default:
					break;
			}
		}
	}
	else if ( limbType == LIMB_HUMANOID_RIGHTLEG )
	{
		if ( playerRace == HUMAN )
		{
			switch ( playerAppearance / 6 )
			{
				case 1:
					this->sprite = 335 + 13 * stats[playernum]->sex;
					break;
				case 2:
					this->sprite = 361 + 13 * stats[playernum]->sex;
					break;
				default:
					this->sprite = 107 + 12 * stats[playernum]->sex;
					break;
			}
		}
		else
		{
			switch ( playerRace )
			{
				case SKELETON:
					this->sprite = stats[playernum]->sex == FEMALE ? 1051 : 693;
					break;
				case GOBLIN:
					if ( stats[playernum]->sex == FEMALE )
					{
						this->sprite = 755;
					}
					else
					{
						this->sprite = 701;
					}
					break;
				case CREATURE_IMP:
					this->sprite = 830;
					break;
				case INCUBUS:
					this->sprite = 709;
					break;
				case SUCCUBUS:
					this->sprite = 717;
					break;
				case VAMPIRE:
					if ( stats[playernum]->sex == FEMALE )
					{
						this->sprite = 759;
					}
					else
					{
						this->sprite = 725;
					}
					break;
				case INSECTOID:
					if ( stats[playernum]->sex == FEMALE )
					{
						this->sprite = 767;
					}
					else
					{
						this->sprite = 733;
					}
					break;
				case GOATMAN:
					this->sprite = 741;
					break;
				case AUTOMATON:
					this->sprite = 749;
					break;
				case TROLL:
					this->sprite = 822;
					break;
				default:
					break;
			}
		}
	}
	else if ( limbType == LIMB_HUMANOID_LEFTLEG )
	{
		if ( playerRace == HUMAN )
		{
			switch ( playerAppearance / 6 )
			{
				case 1:
					this->sprite = 336 + 13 * stats[playernum]->sex;
					break;
				case 2:
					this->sprite = 362 + 13 * stats[playernum]->sex;
					break;
				default:
					this->sprite = 108 + 12 * stats[playernum]->sex;
					break;
			}
		}
		else
		{
			switch ( playerRace )
			{
				case SKELETON:
					this->sprite = stats[playernum]->sex == FEMALE ? 1050 : 692;
					break;
				case GOBLIN:
					if ( stats[playernum]->sex == FEMALE )
					{
						this->sprite = 754;
					}
					else
					{
						this->sprite = 700;
					}
					break;
				case CREATURE_IMP:
					this->sprite = 829;
					break;
				case INCUBUS:
					this->sprite = 708;
					break;
				case SUCCUBUS:
					this->sprite = 716;
					break;
				case VAMPIRE:
					if ( stats[playernum]->sex == FEMALE )
					{
						this->sprite = 758;
					}
					else
					{
						this->sprite = 724;
					}
					break;
				case INSECTOID:
					if ( stats[playernum]->sex == FEMALE )
					{
						this->sprite = 766;
					}
					else
					{
						this->sprite = 732;
					}
					break;
				case GOATMAN:
					this->sprite = 740;
					break;
				case AUTOMATON:
					this->sprite = 748;
					break;
				case TROLL:
					this->sprite = 821;
					break;
				default:
					break;
			}
		}
	}
	else if ( limbType == LIMB_HUMANOID_RIGHTARM )
	{
		if ( playerRace == HUMAN )
		{
			switch ( playerAppearance / 6 )
			{
				case 1:
					this->sprite = 337 + 13 * stats[playernum]->sex;
					break;
				case 2:
					this->sprite = 363 + 13 * stats[playernum]->sex;
					break;
				default:
					this->sprite = 109 + 12 * stats[playernum]->sex;
					break;
			}
		}
		else
		{
			switch ( playerRace )
			{
				case SKELETON:
					this->sprite = stats[playernum]->sex == FEMALE ? 1046 : 689;
					break;
				case GOBLIN:
					this->sprite = 697;
					break;
				case CREATURE_IMP:
					this->sprite = 832;
					break;
				case INCUBUS:
					this->sprite = 705;
					break;
				case SUCCUBUS:
					this->sprite = 713;
					break;
				case VAMPIRE:
					this->sprite = 721;
					break;
				case INSECTOID:
					if ( stats[playernum]->sex == FEMALE )
					{
						this->sprite = 763;
					}
					else
					{
						this->sprite = 729;
					}
					break;
				case GOATMAN:
					this->sprite = 737;
					break;
				case AUTOMATON:
					this->sprite = 745;
					break;
				case TROLL:
					this->sprite = 820;
					break;
				default:
					break;
			}
		}
	}
	else if ( limbType == LIMB_HUMANOID_LEFTARM )
	{
		if ( playerRace == HUMAN )
		{
			switch ( playerAppearance / 6 )
			{
				case 1:
					this->sprite = 338 + 13 * stats[playernum]->sex;
					break;
				case 2:
					this->sprite = 364 + 13 * stats[playernum]->sex;
					break;
				default:
					this->sprite = 110 + 12 * stats[playernum]->sex;
					break;
			}
		}
		else
		{
			switch ( playerRace )
			{
				case SKELETON:
					this->sprite = stats[playernum]->sex == FEMALE ? 1045 : 688;
					break;
				case GOBLIN:
					this->sprite = 696;
					break;
				case CREATURE_IMP:
					this->sprite = 831;
					break;
				case INCUBUS:
					this->sprite = 704;
					break;
				case SUCCUBUS:
					this->sprite = 712;
					break;
				case VAMPIRE:
					this->sprite = 720;
					break;
				case INSECTOID:
					if ( stats[playernum]->sex == FEMALE )
					{
						this->sprite = 762;
					}
					else
					{
						this->sprite = 728;
					}
					break;
				case GOATMAN:
					this->sprite = 736;
					break;
				case AUTOMATON:
					this->sprite = 744;
					break;
				case TROLL:
					this->sprite = 819;
					break;
				default:
					break;
			}
		}
	}
}

bool playerRequiresBloodToSustain(int player)
{
	if ( player < 0 || player >= MAXPLAYERS )
	{
		return false;
	}
	if ( !stats[player] || client_disconnected[player] )
	{
		return false;
	}

	if ( stats[player]->type == VAMPIRE )
	{
		return true;
	}
	if ( stats[player]->EFFECTS[EFF_VAMPIRICAURA] || client_classes[player] == CLASS_ACCURSED )
	{
		return true;
	}
	if ( stats[player]->playerRace == VAMPIRE && stats[player]->appearance == 0 )
	{
		return true;
	}
	
	return false;
}

void playerAnimateRat(Entity* my)
{
	node_t* node = nullptr;
	int bodypart = 0;
	for ( bodypart = 0, node = my->children.first; node != NULL; node = node->next, bodypart++ )
	{
		if ( bodypart == 0 )
		{
		    // this is the head
			my->focalx = 3;
			my->scalex = 1.02;
			my->scaley = 1.02;
			my->scalez = 1.02;
			continue;
		}
		Entity* entity = (Entity*)node->element;
		entity->x = my->x;
		entity->y = my->y;
		entity->z = my->z;
		if ( bodypart == 1 )
		{
		    // this is the body
			if ( entity->sprite != 815 && entity->sprite != 816 )
			{
				entity->sprite = 815;
			}
			entity->focalx = -4;
			if ( fabs(PLAYER_VELX) > 0.1 || fabs(PLAYER_VELY) > 0.1 )
			{
				if ( (ticks % 10 == 0) )
				{
					if ( entity->sprite == 815 )
					{
						entity->sprite = 816;
					}
					else
					{
						entity->sprite = 815;
					}
				}
			}
		}
		else if ( bodypart > 1 )
		{
			entity->flags[INVISIBLE] = true;
		}
		if ( multiplayer == SERVER )
		{
			// update sprites for clients
			if ( entity->skill[10] != entity->sprite )
			{
				entity->skill[10] = entity->sprite;
				serverUpdateEntityBodypart(my, bodypart);
			}
			if ( entity->skill[11] != entity->flags[INVISIBLE] )
			{
				entity->skill[11] = entity->flags[INVISIBLE];
				serverUpdateEntityBodypart(my, bodypart);
			}
			if ( PLAYER_ALIVETIME == TICKS_PER_SECOND + bodypart )
			{
				serverUpdateEntityBodypart(my, bodypart);
			}
		}
		entity->yaw = my->yaw;
	}

	if ( PLAYER_ATTACKTIME >= 10 )
	{
		PLAYER_ATTACK = 0;
	}
}

void playerAnimateSpider(Entity* my)
{
	node_t* node = nullptr;
	int bodypart = 0;
	for ( bodypart = 0, node = my->children.first; node != NULL; node = node->next, bodypart++ )
	{
		Entity* entity = (Entity*)node->element;
		if ( bodypart == 0 )
		{
			// hudweapon case
			continue;
		}
		if ( bodypart < 11 )
		{
			entity->flags[INVISIBLE] = true;
			if ( multiplayer == SERVER )
			{
				// update sprites for clients
				if ( entity->skill[10] != entity->sprite )
				{
					entity->skill[10] = entity->sprite;
					serverUpdateEntityBodypart(my, bodypart);
				}
				if ( entity->skill[11] != entity->flags[INVISIBLE] )
				{
					entity->skill[11] = entity->flags[INVISIBLE];
					serverUpdateEntityBodypart(my, bodypart);
				}
				if ( PLAYER_ALIVETIME == TICKS_PER_SECOND + bodypart )
				{
					serverUpdateEntityBodypart(my, bodypart);
				}
			}
			continue;
		}
		Entity* previous = NULL; // previous part
		if ( bodypart > 11 )
		{
			previous = (Entity*)node->prev->element;
		}
		entity->x = my->x;
		entity->y = my->y;
		entity->z = my->z;
		entity->yaw = my->yaw;

		entity->roll = my->roll;

		if ( bodypart > 12 )
		{
			entity->pitch = -my->pitch;
			entity->pitch = std::max(-PI / 32, std::min(PI / 32, entity->pitch));
			if ( bodypart % 2 == 0 )
			{
			    if (arachnophobia_filter)
			    {
				    entity->sprite = 1004;
				    entity->focalx = limbs[CRAB][4][0];
				    entity->focaly = limbs[CRAB][4][1];
				    entity->focalz = limbs[CRAB][4][2];
			    }
			    else
			    {
				    entity->sprite = 826;
				    entity->focalx = limbs[SPIDER][4][0]; // 3
				    entity->focaly = limbs[SPIDER][4][1]; // 0
				    entity->focalz = limbs[SPIDER][4][2]; // 0
			    }
			}
			else
			{
			    if (arachnophobia_filter)
			    {
				    entity->sprite = 1003;
				    entity->focalx = limbs[CRAB][3][0];
				    entity->focaly = limbs[CRAB][3][1];
				    entity->focalz = limbs[CRAB][3][2];
			    }
			    else
			    {
				    entity->sprite = 825;
				    entity->focalx = limbs[SPIDER][3][0]; // 1
				    entity->focaly = limbs[SPIDER][3][1]; // 0
				    entity->focalz = limbs[SPIDER][3][2]; // -1
			    }
			}
		}

		if ( bodypart == 11 || bodypart == 12 )
		{
			if ( PLAYER_ATTACK >= 1 && PLAYER_ATTACK <= 3 )
			{
				// vertical chop
				if ( PLAYER_ATTACKTIME == 0 )
				{
					entity->pitch = 0;
					entity->roll = 0;
					entity->skill[1] = 0;
				}
				else
				{
					if ( entity->skill[1] == 0 )
					{
						// upswing
						if ( limbAnimateToLimit(entity, ANIMATE_PITCH, -0.5, 5 * PI / 4, false, 0.0) )
						{
							entity->skill[1] = 1;
						}
					}
					else
					{
						if ( entity->pitch >= 3 * PI / 2 )
						{
							PLAYER_ARMBENDED = 1;
						}
						if ( limbAnimateToLimit(entity, ANIMATE_PITCH, 0.3, 0, false, 0.0) )
						{
							entity->skill[1] = 0;
							entity->roll = 0;
							PLAYER_ARMBENDED = 0;
							PLAYER_ATTACK = 0;
						}
					}
				}
			}
		}

		entity->flags[INVISIBLE] = my->flags[INVISIBLE];

		switch ( bodypart )
		{
			// right pedipalp
			case 11:
				entity->sprite = arachnophobia_filter ? 1002 : 824;
				if (arachnophobia_filter)
				{
				    entity->focalx = limbs[CRAB][1][0];
				    entity->focaly = limbs[CRAB][1][1];
				    entity->focalz = limbs[CRAB][1][2];
				}
				else
				{
				    entity->focalx = limbs[SPIDER][1][0]; // 1
				    entity->focaly = limbs[SPIDER][1][1]; // 0
				    entity->focalz = limbs[SPIDER][1][2]; // 1
				}
				entity->x += cos(my->yaw) * 2 + cos(my->yaw + PI / 2) * 2;
				entity->y += sin(my->yaw) * 2 + sin(my->yaw + PI / 2) * 2;
				entity->yaw += PI / 10;
				if ( PLAYER_ATTACK == 0 )
				{
					entity->pitch = my->pitch;
					entity->pitch -= PI / 8;
				}
				break;
				// left pedipalp
			case 12:
				entity->sprite = arachnophobia_filter ? 1002 : 824;
				if (arachnophobia_filter)
				{
				    entity->focalx = limbs[CRAB][2][0];
				    entity->focaly = limbs[CRAB][2][1];
				    entity->focalz = limbs[CRAB][2][2];
				}
				else
				{
				    entity->focalx = limbs[SPIDER][2][0]; // 1
				    entity->focaly = limbs[SPIDER][2][1]; // 0
				    entity->focalz = limbs[SPIDER][2][2]; // 1
				}
				entity->x += cos(my->yaw) * 2 - cos(my->yaw + PI / 2) * 2;
				entity->y += sin(my->yaw) * 2 - sin(my->yaw + PI / 2) * 2;
				entity->yaw -= PI / 10;
				if ( PLAYER_ATTACK == 0 )
				{
					entity->pitch = my->pitch;
					entity->pitch -= PI / 8;
				}
				break;

				// 1st/5th leg:
				// thigh
			case 13:
			case 21:
				entity->x += cos(my->yaw) * 1 + cos(my->yaw + PI / 2) * 2.5 * (1 - 2 * (bodypart > 20));
				entity->y += sin(my->yaw) * 1 + sin(my->yaw + PI / 2) * 2.5 * (1 - 2 * (bodypart > 20));
				if ( (fabs(PLAYER_VELX) > 0.1 || fabs(PLAYER_VELY) > 0.1) )
				{
					if ( !entity->skill[0] )
					{
						entity->fskill[0] += .1;
						if ( entity->fskill[0] >= 1 )
						{
							entity->fskill[0] = 1;
							entity->skill[0] = 1;
						}
					}
					else
					{
						entity->fskill[0] -= .1;
						if ( entity->fskill[0] <= 0 )
						{
							entity->fskill[0] = 0;
							entity->skill[0] = 0;
						}
					}
				}
				entity->z += entity->fskill[0];
				entity->yaw += PI / 6 * (1 - 2 * (bodypart > 20));
				entity->pitch += PI / 4;
				break;
				// shin
			case 14:
			case 22:
				entity->x = previous->x;
				entity->y = previous->y;
				entity->z = previous->z;
				entity->yaw = previous->yaw;
				entity->pitch = previous->pitch;
				entity->x += cos(my->yaw) * 3 + cos(my->yaw + PI / 2) * 2 * (1 - 2 * (bodypart > 20));
				entity->y += sin(my->yaw) * 3 + sin(my->yaw + PI / 2) * 2 * (1 - 2 * (bodypart > 20));
				entity->z += .5;
				entity->pitch += PI / 6 - PI / 4;
				entity->pitch -= (PI / 10) * (previous->z - my->z);
				break;

				// 2nd/6th leg:
				// thigh
			case 15:
			case 23:
				entity->x += cos(my->yaw + PI / 2) * 3 * (1 - 2 * (bodypart > 20));
				entity->y += sin(my->yaw + PI / 2) * 3 * (1 - 2 * (bodypart > 20));
				if ( (fabs(PLAYER_VELX) > 0.1 || fabs(PLAYER_VELY) > 0.1) )
				{
					if ( !entity->skill[0] )
					{
						entity->fskill[0] += .1;
						if ( entity->fskill[0] >= 1 )
						{
							entity->fskill[0] = 1;
							entity->skill[0] = 1;
						}
					}
					else
					{
						entity->fskill[0] -= .1;
						if ( entity->fskill[0] <= 0 )
						{
							entity->fskill[0] = 0;
							entity->skill[0] = 0;
						}
					}
				}
				entity->z += entity->fskill[0];
				entity->yaw += PI / 3 * (1 - 2 * (bodypart > 20));
				entity->pitch += PI / 4;
				break;
				// shin
			case 16:
			case 24:
				entity->x = previous->x;
				entity->y = previous->y;
				entity->z = previous->z;
				entity->yaw = previous->yaw;
				entity->pitch = previous->pitch;
				entity->x += cos(my->yaw) * 1.75 + cos(my->yaw + PI / 2) * 3 * (1 - 2 * (bodypart > 20));
				entity->y += sin(my->yaw) * 1.75 + sin(my->yaw + PI / 2) * 3 * (1 - 2 * (bodypart > 20));
				entity->z += .5;
				entity->pitch += PI / 6 - PI / 4;
				entity->pitch -= (PI / 10) * (previous->z - my->z);
				break;

				// 3rd/7th leg:
				// thigh
			case 17:
			case 25:
				entity->x += cos(my->yaw) * -.5 + cos(my->yaw + PI / 2) * 2 * (1 - 2 * (bodypart > 20));
				entity->y += sin(my->yaw) * -.5 + sin(my->yaw + PI / 2) * 2 * (1 - 2 * (bodypart > 20));
				if ( (fabs(PLAYER_VELX) > 0.1 || fabs(PLAYER_VELY) > 0.1) )
				{
					if ( !entity->skill[0] )
					{
						entity->fskill[0] += .1;
						if ( entity->fskill[0] >= 1 )
						{
							entity->fskill[0] = 1;
							entity->skill[0] = 1;
						}
					}
					else
					{
						entity->fskill[0] -= .1;
						if ( entity->fskill[0] <= 0 )
						{
							entity->fskill[0] = 0;
							entity->skill[0] = 0;
						}
					}
				}
				entity->z += entity->fskill[0];
				entity->yaw += (PI / 2 + PI / 8) * (1 - 2 * (bodypart > 20));
				entity->pitch += PI / 4;
				break;
				// shin
			case 18:
			case 26:
				entity->x = previous->x;
				entity->y = previous->y;
				entity->z = previous->z;
				entity->yaw = previous->yaw;
				entity->pitch = previous->pitch;
				entity->x += cos(my->yaw) * -1.25 + cos(my->yaw + PI / 2) * 3.25 * (1 - 2 * (bodypart > 20));
				entity->y += sin(my->yaw) * -1.25 + sin(my->yaw + PI / 2) * 3.25 * (1 - 2 * (bodypart > 20));
				entity->z += .5;
				entity->pitch += PI / 6 - PI / 4;
				entity->pitch -= (PI / 10) * (previous->z - my->z);
				break;

				// 4th/8th leg:
				// thigh
			case 19:
			case 27:
				entity->x += cos(my->yaw) * -.5 + cos(my->yaw + PI / 2) * 2 * (1 - 2 * (bodypart > 20));
				entity->y += sin(my->yaw) * -.5 + sin(my->yaw + PI / 2) * 2 * (1 - 2 * (bodypart > 20));
				if ( (fabs(PLAYER_VELX) > 0.1 || fabs(PLAYER_VELY) > 0.1) )
				{
					if ( !entity->skill[0] )
					{
						entity->fskill[0] += .1;
						if ( entity->fskill[0] >= 1 )
						{
							entity->fskill[0] = 1;
							entity->skill[0] = 1;
						}
					}
					else
					{
						entity->fskill[0] -= .1;
						if ( entity->fskill[0] <= 0 )
						{
							entity->fskill[0] = 0;
							entity->skill[0] = 0;
						}
					}
				}
				entity->z += entity->fskill[0];
				entity->yaw += (PI / 2 + PI / 3) * (1 - 2 * (bodypart > 20));
				entity->pitch += PI / 4;
				break;
				// shin
			case 20:
			case 28:
				entity->x = previous->x;
				entity->y = previous->y;
				entity->z = previous->z;
				entity->yaw = previous->yaw;
				entity->pitch = previous->pitch;
				entity->x += cos(my->yaw) * -3 + cos(my->yaw + PI / 2) * 1.75 * (1 - 2 * (bodypart > 20));
				entity->y += sin(my->yaw) * -3 + sin(my->yaw + PI / 2) * 1.75 * (1 - 2 * (bodypart > 20));
				entity->z += .5;
				entity->pitch += PI / 6 - PI / 4;
				entity->pitch += (PI / 10) * (previous->z - my->z);
				break;
			default:
				//entity->flags[INVISIBLE] = true; // for debugging
				break;
		}
	}
}
