/*-------------------------------------------------------------------------------

	BARONY
	File: draw.cpp
	Desc: contains various definitions for player code

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "player.hpp"
#include "game.hpp"

Player **players = nullptr;

Entity *selectedEntity = nullptr;
int current_player = 0;
Sint32 mousex=0, mousey=0;
Sint32 omousex=0, omousey=0;
Sint32 mousexrel=0, mouseyrel=0;

bool splitscreen = false;

int gamepad_deadzone = 8000;
int gamepad_leftx_sensitivity = 350;
int gamepad_lefty_sensitivity = 350;
int gamepad_rightx_sensitivity = 500;
int gamepad_righty_sensitivity = 600;

bool gamepad_leftx_invert = false;
bool gamepad_lefty_invert = false;
bool gamepad_rightx_invert = false;
bool gamepad_righty_invert = false;
bool gamepad_menux_invert = false;
bool gamepad_menuy_invert = false;


GameController* game_controller = nullptr;

GameController::GameController()
{
	sdl_device = nullptr;
}

GameController::~GameController()
{
	if (sdl_device)
	{
		close();
	}
}

void GameController::close()
{
	if (sdl_device)
	{
		SDL_GameControllerClose(sdl_device);
		sdl_device = nullptr;
	}
}

bool GameController::open(int c)
{
	if (sdl_device)
		close();

	if (c < 0 || c >= SDL_NumJoysticks())
		return false;

	if (!SDL_IsGameController(c))
		return false;

	sdl_device = SDL_GameControllerOpen(c);

	if (sdl_device == nullptr)
	{
		printlog("Error: Failed to open game controller! SDL Error: %s\n", SDL_GetError());
	}
	else
	{
		printlog("Successfully initialized game controller!\n");
	}

	return (sdl_device != nullptr);
}

bool GameController::isActive()
{
	return (sdl_device != nullptr);
}

void GameController::handleLook()
{
	if (!isActive())
		return;

	if (!shootmode)
	{
		//Left analog stick = move the mouse around on the menus.
		int leftx = getLeftXMove();
		if (gamepad_menux_invert)
			leftx = -leftx;
		int lefty = getLeftYMove();
		if (gamepad_menuy_invert)
			lefty = -lefty;

		if (leftx || lefty)
		{
			SDL_Event e;

			e.type = SDL_MOUSEMOTION;
			e.motion.x = mousex + leftx;
			e.motion.y = mousey + lefty;
			e.motion.xrel = leftx;
			e.motion.yrel = lefty;
			SDL_PushEvent(&e);
		}
	}
	else
	{
		//Right analog stick = look.
		int rightx = getRightXMove();
		int righty = getRightYMove();

		if (rightx || righty)
		{
			//TODO: Use right stick for character head movement and left stick for mouse movement in the GUI?
			//The controller needs different sensitivity in the GUI vs moving the character's head around.
			SDL_Event e;

			e.type = SDL_MOUSEMOTION;
			e.motion.x = mousex + rightx;
			e.motion.y = mousey + righty;
			e.motion.xrel = rightx;
			e.motion.yrel = righty;
			SDL_PushEvent(&e);
		}
	}
}

int GameController::getLeftXMove()
{
	if (!isActive())
		return 0;

	int x = getRawLeftXMove();

	x /= gamepad_leftx_sensitivity;

	return x;
}

int GameController::getLeftYMove()
{
	if (!isActive())
		return 0;

	int y = getRawLeftYMove();

	y /= gamepad_lefty_sensitivity;

	return y;
}

int GameController::getRightXMove()
{
	if (!isActive())
		return 0;

	int x = getRawRightXMove();

	x /= gamepad_rightx_sensitivity;

	return x;
}

int GameController::getRightYMove()
{
	if (!isActive())
		return 0;

	int y = getRawRightYMove();

	y /= gamepad_righty_sensitivity;

	return y;
}

int GameController::getRawLeftXMove()
{
	if (!isActive())
		return 0;

	int x = SDL_GameControllerGetAxis(sdl_device, SDL_CONTROLLER_AXIS_LEFTX);

	if (x < gamepad_deadzone && x > -gamepad_deadzone)
		return 0;

	if (x < -gamepad_deadzone) //TODO: Give each controller a deadzone setting? Or maybe on a player by player basis? And/or each analog stick its own deadzone setting?
		x += gamepad_deadzone;
	else
		x -= gamepad_deadzone;

	return (!gamepad_leftx_invert) ? x : -x;
}

int GameController::getRawLeftYMove()
{
	if (!isActive())
		return 0;

	int y = SDL_GameControllerGetAxis(sdl_device, SDL_CONTROLLER_AXIS_LEFTY);

	if (y < gamepad_deadzone && y > -gamepad_deadzone)
		return 0;

	if (y < -gamepad_deadzone)
		y += gamepad_deadzone;
	else
		y -= gamepad_deadzone;

	return (!gamepad_lefty_invert) ? y : -y;
}

int GameController::getRawRightXMove()
{
	if (!isActive())
		return 0;

	int x = SDL_GameControllerGetAxis(sdl_device, SDL_CONTROLLER_AXIS_RIGHTX);

	if (x < gamepad_deadzone && x > -gamepad_deadzone)
		return 0;

	if (x < -gamepad_deadzone)
		x += gamepad_deadzone;
	else
		x -= gamepad_deadzone;

	return (!gamepad_rightx_invert) ? x : -x;
}

int GameController::getRawRightYMove()
{
	if (!isActive())
		return 0;

	int y = SDL_GameControllerGetAxis(sdl_device, SDL_CONTROLLER_AXIS_RIGHTY);

	if (y < gamepad_deadzone && y > -gamepad_deadzone)
		return 0;

	if (y < -gamepad_deadzone)
		y += gamepad_deadzone;
	else
		y -= gamepad_deadzone;

	return (!gamepad_righty_invert) ? y : -y;
}

float GameController::getLeftXPercent()
{
	return (float)game_controller->getRawLeftXMove() / (float)game_controller->maxLeftXMove();
}

float GameController::getLeftYPercent()
{
	return (float)game_controller->getRawLeftYMove() / (float)game_controller->maxLeftYMove();
}

float GameController::getRightXPercent()
{
	return (float)game_controller->getRawRightXMove() / (float)game_controller->maxRightXMove();
}

float GameController::getRightYPercent()
{
	return (float)game_controller->getRawRightYMove() / (float)game_controller->maxRightYMove();
}

int GameController::maxLeftXMove()
{
	return 32767 - gamepad_deadzone;
}

int GameController::maxLeftYMove()
{
	return 32767 - gamepad_deadzone;
}

int GameController::maxRightXMove()
{
	return 32767 - gamepad_deadzone;
}

int GameController::maxRightYMove()
{
	return 32767 - gamepad_deadzone; //Ya, it's pretty constant in SDL2.
}

void initGameControllers()
{
	int c = 0;
	bool found = false; //TODO: Bugger this and implement multi-controller support.
	game_controller = new GameController();
	for (c = 0; c < SDL_NumJoysticks() && !found; ++c) //TODO: Bugger this and implement multi-controller support on a player-by-player basis.
	{
		if (SDL_IsGameController(c) && game_controller->open(c))
		{
			printlog("(Device %d successfully initialized as game controller.)\n", c);
			found = true; //TODO: Bugger this and implement multi-controller support.
		}
		else
		{
			printlog("Info: device %d is not a game controller! Joysticks are not supported.\n", c);
		}
	}
	if (!found)
	{
		printlog("Info: No game controller detected!");
	}
}

Player::Player(int in_playernum, bool in_local_host)
{
	screen = nullptr;
	local_host = in_local_host;
	playernum = in_playernum;
	entity = nullptr;
}

Player::~Player()
{
	if (screen)
	{
		SDL_FreeSurface(screen);
	}

	if (entity)
	{
		delete entity;
	}
}