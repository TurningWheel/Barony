/*-------------------------------------------------------------------------------

	BARONY
	File: draw.cpp
	Desc: contains various definitions for player code

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "player.hpp"
#include "game.hpp"
#include "main.hpp"
#include "interface/interface.hpp"
#include "items.hpp"
#include "shops.hpp"

Player** players = nullptr;

Entity* selectedEntity = nullptr;
int current_player = 0;
Sint32 mousex = 0, mousey = 0;
Sint32 omousex = 0, omousey = 0;
Sint32 mousexrel = 0, mouseyrel = 0;

bool splitscreen = false;

int gamepad_deadzone = 8000;
int gamepad_trigger_deadzone = 18000;
int gamepad_leftx_sensitivity = 1400;
int gamepad_lefty_sensitivity = 1400;
int gamepad_rightx_sensitivity = 500;
int gamepad_righty_sensitivity = 600;
int gamepad_menux_sensitivity = 1400;
int gamepad_menuy_sensitivity = 1400;

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
	id = -1;
	name = "";

	oldLeftTrigger = 0;
	oldRightTrigger = 0;
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
	{
		close();
	}

	if (c < 0 || c >= SDL_NumJoysticks())
	{
		return false;
	}

	if (!SDL_IsGameController(c))
	{
		return false;
	}

	sdl_device = SDL_GameControllerOpen(c);

	if (sdl_device == nullptr)
	{
		printlog("Error: Failed to open game controller! SDL Error: %s\n", SDL_GetError());
	}
	else
	{
		id = c;
		printlog("Successfully initialized game controller!\n");
		name = (SDL_GameControllerNameForIndex(c));
		printlog("Controller name is \"%s\"", name.c_str());
	}


	return (sdl_device != nullptr);
}

bool GameController::isActive()
{
	return (sdl_device != nullptr);
}

void GameController::handleAnalog()
{
	if (!isActive())
	{
		return;
	}

	//Right analog stick = look.

	if (!shootmode || gamePaused)
	{
		int rightx = getRawRightXMove() / gamepad_menux_sensitivity;
		int righty = getRawRightYMove() / gamepad_menuy_sensitivity;



		//The right stick's inversion and the menu's inversion should be independent of eachother. This just undoes any inversion.
		if (gamepad_rightx_invert)
		{
			rightx = -rightx;
		}
		if (gamepad_righty_invert)
		{
			righty = -righty;
		}

		if (gamepad_menux_invert)
		{
			rightx = -rightx;
		}
		if (gamepad_menuy_invert)
		{
			righty = -righty;
		}

		if (rightx || righty)
		{
			SDL_WarpMouseInWindow(screen, std::max(0, std::min(camera.winw, mousex + rightx)), std::max(0, std::min(camera.winh, mousey + righty)));
		}
	}
	else
	{
		int rightx = getRightXMove();
		int righty = getRightYMove();

		if (rightx || righty)
		{
			SDL_Event e;

			e.type = SDL_MOUSEMOTION;
			e.motion.x = mousex + rightx;
			e.motion.y = mousey + righty;
			e.motion.xrel = rightx;
			e.motion.yrel = righty;
			SDL_PushEvent(&e);
		}
	}

	if (getLeftTrigger())
	{
		if ( !oldLeftTrigger )
		{
			oldLeftTrigger = 1;
			joy_trigger_status[0] = 1;
			lastkeypressed = 299;
		}
	}
	else
	{
		oldLeftTrigger = 0;
		joy_trigger_status[0] = 0;
	}

	if (getRightTrigger())
	{
		if ( !oldRightTrigger )
		{
			oldRightTrigger = 1;
			joy_trigger_status[1] = 1;
			lastkeypressed = 300;
		}
	}
	else
	{
		oldRightTrigger = 0;
		joy_trigger_status[1] = 0;
	}
}

int GameController::getLeftXMove()
{
	if (!isActive())
	{
		return 0;
	}

	int x = getRawLeftXMove();

	x /= gamepad_leftx_sensitivity;

	return x;
}

int GameController::getLeftYMove()
{
	if (!isActive())
	{
		return 0;
	}

	int y = -getRawLeftYMove();

	y /= gamepad_lefty_sensitivity;

	return y;
}

int GameController::getRightXMove()
{
	if (!isActive())
	{
		return 0;
	}

	int x = getRawRightXMove();

	x /= gamepad_rightx_sensitivity;

	return x;
}

int GameController::getRightYMove()
{
	if (!isActive())
	{
		return 0;
	}

	int y = getRawRightYMove();

	y /= gamepad_righty_sensitivity;

	return y;
}



int GameController::getLeftTrigger()
{
	return getRawLeftTrigger(); //No sensitivity taken into account (yet)
}

int GameController::getRightTrigger()
{
	return getRawRightTrigger(); //No sensitivity taken into account (yet)
}


int GameController::getRawLeftXMove()
{
	if (!isActive())
	{
		return 0;
	}

	int x = SDL_GameControllerGetAxis(sdl_device, SDL_CONTROLLER_AXIS_LEFTX);

	if (x < gamepad_deadzone && x > -gamepad_deadzone)
	{
		return 0;
	}

	if (x < -gamepad_deadzone)   //TODO: Give each controller a deadzone setting? Or maybe on a player by player basis? And/or each analog stick its own deadzone setting?
	{
		x += gamepad_deadzone;
	}
	else
	{
		x -= gamepad_deadzone;
	}

	return (!gamepad_leftx_invert) ? x : -x;
}

int GameController::getRawLeftYMove()
{
	if (!isActive())
	{
		return 0;
	}

	int y = SDL_GameControllerGetAxis(sdl_device, SDL_CONTROLLER_AXIS_LEFTY);

	if (y < gamepad_deadzone && y > -gamepad_deadzone)
	{
		return 0;
	}

	if (y < -gamepad_deadzone)
	{
		y += gamepad_deadzone;
	}
	else
	{
		y -= gamepad_deadzone;
	}

	return (!gamepad_lefty_invert) ? -y : y;
}

int GameController::getRawRightXMove()
{
	if (!isActive())
	{
		return 0;
	}

	int x = SDL_GameControllerGetAxis(sdl_device, SDL_CONTROLLER_AXIS_RIGHTX);

	if (x < gamepad_deadzone && x > -gamepad_deadzone)
	{
		return 0;
	}

	if (x < -gamepad_deadzone)
	{
		x += gamepad_deadzone;
	}
	else
	{
		x -= gamepad_deadzone;
	}

	return (!gamepad_rightx_invert) ? x : -x;
}

int GameController::getRawRightYMove()
{
	if (!isActive())
	{
		return 0;
	}

	int y = SDL_GameControllerGetAxis(sdl_device, SDL_CONTROLLER_AXIS_RIGHTY);

	if (y < gamepad_deadzone && y > -gamepad_deadzone)
	{
		return 0;
	}

	if (y < -gamepad_deadzone)
	{
		y += gamepad_deadzone;
	}
	else
	{
		y -= gamepad_deadzone;
	}

	return (!gamepad_righty_invert) ? y : -y;
}



int GameController::getRawLeftTrigger()
{
	if (!isActive())
	{
		return 0;
	}

	int n = SDL_GameControllerGetAxis(sdl_device, SDL_CONTROLLER_AXIS_TRIGGERLEFT);

	if (n < gamepad_trigger_deadzone)
	{
		return 0;
	}

	n -= gamepad_trigger_deadzone;

	return n;
}

int GameController::getRawRightTrigger()
{
	if (!isActive())
	{
		return 0;
	}

	int n = SDL_GameControllerGetAxis(sdl_device, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);

	if (n < gamepad_trigger_deadzone)
	{
		return 0;
	}

	n -= gamepad_trigger_deadzone;

	return n;
}



float GameController::getLeftXPercent()
{
	return (float)getRawLeftXMove() / (float)maxLeftXMove();
}

float GameController::getLeftYPercent()
{
	return (float)getRawLeftYMove() / (float)maxLeftYMove();
}

float GameController::getRightXPercent()
{
	return (float)getRawRightXMove() / (float)maxRightXMove();
}

float GameController::getRightYPercent()
{
	return (float)getRawRightYMove() / (float)maxRightYMove();
}


float GameController::getLeftTriggerPercent()
{
	return (float)getRawLeftTrigger() / (float)maxLeftTrigger();
}

float GameController::getRightTriggerPercent()
{
	return (float)getRawRightTrigger() / (float)maxRightTrigger();
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


int GameController::maxLeftTrigger()
{
	return 32767 - gamepad_deadzone;
}

int GameController::maxRightTrigger()
{
	return 32767 - gamepad_deadzone;
}

bool GameController::handleInventoryMovement()
{
	bool dpad_moved = false;

	if (itemMenuOpen)
	{
		return false;
	}

	if ( hotbarHasFocus && !hotbarGamepadControlEnabled() )
	{
		hotbarHasFocus = false;
	}

	if (*inputPressed(joyimpulses[INJOY_DPAD_LEFT]))
	{
		if ( hotbarHasFocus && hotbarGamepadControlEnabled() )
		{
			//If hotbar is focused and chest, etc, not opened, navigate hotbar.
			selectHotbarSlot(current_hotbar - 1);
			warpMouseToSelectedHotbarSlot();
		}
		else
		{
			//Navigate inventory.
			select_inventory_slot(selected_inventory_slot_x - 1, selected_inventory_slot_y);
		}
		*inputPressed(joyimpulses[INJOY_DPAD_LEFT]) = 0;

		dpad_moved = true;
	}

	if (*inputPressed(joyimpulses[INJOY_DPAD_RIGHT]))
	{
		if ( hotbarHasFocus && hotbarGamepadControlEnabled() )
		{
			//If hotbar is focused and chest, etc, not opened, navigate hotbar.
			selectHotbarSlot(current_hotbar + 1);
			warpMouseToSelectedHotbarSlot();
		}
		else
		{
			//Navigate inventory.
			select_inventory_slot(selected_inventory_slot_x + 1, selected_inventory_slot_y);
		}
		*inputPressed(joyimpulses[INJOY_DPAD_RIGHT]) = 0;

		dpad_moved = true;
	}

	if (*inputPressed(joyimpulses[INJOY_DPAD_UP]))
	{
		if ( hotbarHasFocus && hotbarGamepadControlEnabled() )
		{
			//Warp back to top of inventory.
			hotbarHasFocus = false;
			select_inventory_slot(selected_inventory_slot_x, INVENTORY_SIZEY - 1);
		}
		else
		{
			select_inventory_slot(selected_inventory_slot_x, selected_inventory_slot_y - 1); //Will handle warping to hotbar.
		}
		*inputPressed(joyimpulses[INJOY_DPAD_UP]) = 0;

		dpad_moved = true;
	}

	if (*inputPressed(joyimpulses[INJOY_DPAD_DOWN]))
	{
		if ( hotbarHasFocus && hotbarGamepadControlEnabled() )
		{
			//Warp back to bottom of inventory.
			hotbarHasFocus = false;
			select_inventory_slot(selected_inventory_slot_x, 0);
		}
		else
		{
			select_inventory_slot(selected_inventory_slot_x, selected_inventory_slot_y + 1);
		}
		*inputPressed(joyimpulses[INJOY_DPAD_DOWN]) = 0;

		dpad_moved = true;
	}

	if (dpad_moved)
	{
		dpad_moved = false;
		draw_cursor = false;

		return true;
	}

	return false;
}

bool GameController::handleChestMovement()
{
	bool dpad_moved = false;

	if ( itemMenuOpen )
	{
		return false;
	}

	if (*inputPressed(joyimpulses[INJOY_DPAD_UP]))
	{
		selectChestSlot(selectedChestSlot - 1);
		*inputPressed(joyimpulses[INJOY_DPAD_UP]) = 0;

		dpad_moved = true;
	}

	if (*inputPressed(joyimpulses[INJOY_DPAD_DOWN]))
	{
		selectChestSlot(selectedChestSlot + 1);
		*inputPressed(joyimpulses[INJOY_DPAD_DOWN]) = 0;

		dpad_moved = true;
	}

	if (dpad_moved)
	{
		dpad_moved = false;
		draw_cursor = false;

		return true;
	}

	return false;
}

bool GameController::handleShopMovement()
{
	bool dpad_moved = false;

	if ( itemMenuOpen )
	{
		return false;
	}

	/*
	//I would love to just do these, but it just wouldn't work with the way the code is set up.
	if (*inputPressed(joyimpulses[INJOY_DPAD_LEFT]))
	{
		cycleShopCategories(-1);
		*inputPressed(joyimpulses[INJOY_DPAD_LEFT]) = 0;

		dpad_moved = true;
	}

	if (*inputPressed(joyimpulses[INJOY_DPAD_RIGHT]))
	{
		cycleShopCategories(1);
		*inputPressed(joyimpulses[INJOY_DPAD_RIGHT]) = 0;

		dpad_moved = true;
	}*/

	if (*inputPressed(joyimpulses[INJOY_DPAD_UP]))
	{
		selectShopSlot(selectedShopSlot - 1);
		*inputPressed(joyimpulses[INJOY_DPAD_UP]) = 0;

		dpad_moved = true;
	}

	if (*inputPressed(joyimpulses[INJOY_DPAD_DOWN]))
	{
		selectShopSlot(selectedShopSlot + 1);
		*inputPressed(joyimpulses[INJOY_DPAD_DOWN]) = 0;

		dpad_moved = true;
	}

	if (dpad_moved)
	{
		dpad_moved = false;
		draw_cursor = false;

		return true;
	}

	return false;
}

bool GameController::handleIdentifyMovement()
{
	bool dpad_moved = false;

	if ( itemMenuOpen )
	{
		return false;
	}

	if (*inputPressed(joyimpulses[INJOY_DPAD_UP]))
	{
		selectIdentifySlot(selectedIdentifySlot - 1);
		*inputPressed(joyimpulses[INJOY_DPAD_UP]) = 0;

		dpad_moved = true;
	}

	if (*inputPressed(joyimpulses[INJOY_DPAD_DOWN]))
	{
		selectIdentifySlot(selectedIdentifySlot + 1);
		*inputPressed(joyimpulses[INJOY_DPAD_DOWN]) = 0;

		dpad_moved = true;
	}

	if (dpad_moved)
	{
		dpad_moved = false;
		draw_cursor = false;

		return true;
	}

	return false;
}

bool GameController::handleRemoveCurseMovement()
{
	bool dpad_moved = false;

	if ( itemMenuOpen )
	{
		return false;
	}

	if (*inputPressed(joyimpulses[INJOY_DPAD_UP]))
	{
		selectRemoveCurseSlot(selectedRemoveCurseSlot - 1);
		*inputPressed(joyimpulses[INJOY_DPAD_UP]) = 0;

		dpad_moved = true;
	}

	if (*inputPressed(joyimpulses[INJOY_DPAD_DOWN]))
	{
		selectRemoveCurseSlot(selectedRemoveCurseSlot + 1);
		*inputPressed(joyimpulses[INJOY_DPAD_DOWN]) = 0;

		dpad_moved = true;
	}

	if (dpad_moved)
	{
		dpad_moved = false;
		draw_cursor = false;

		return true;
	}

	return false;
}

bool GameController::handleItemContextMenu(const Item& item)
{
	bool dpad_moved = false;

	if (!itemMenuOpen)
	{
		return false;
	}

	if (*inputPressed(joyimpulses[INJOY_DPAD_UP]))
	{
		selectItemMenuSlot(item, itemMenuSelected - 1);
		*inputPressed(joyimpulses[INJOY_DPAD_UP]) = 0;

		dpad_moved = true;
	}

	if (*inputPressed(joyimpulses[INJOY_DPAD_DOWN]))
	{
		selectItemMenuSlot(item, itemMenuSelected + 1);
		*inputPressed(joyimpulses[INJOY_DPAD_DOWN]) = 0;

		dpad_moved = true;
	}

	if (dpad_moved)
	{
		dpad_moved = false;
		draw_cursor = false;

		return true;
	}

	return false;
}


void initGameControllers()
{
	SDL_GameControllerAddMappingsFromFile("gamecontrollerdb.txt");

	int c = 0;
	bool found = false; //TODO: Bugger this and implement multi-controller support.
	game_controller = new GameController();
	for (c = 0; c < SDL_NumJoysticks() && !found; ++c)   //TODO: Bugger this and implement multi-controller support on a player-by-player basis.
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

void initIdentifyGUIControllerCode()
{
	if ( identify_items[0] )
	{
		selectedIdentifySlot = 0;
		warpMouseToSelectedIdentifySlot();
	}
	else
	{
		selectedIdentifySlot = -1;
	}
}

void initRemoveCurseGUIControllerCode()
{
	if ( removecurse_items[0] )
	{
		selectedRemoveCurseSlot = 0;
		warpMouseToSelectedRemoveCurseSlot();
	}
	else
	{
		selectedRemoveCurseSlot = -1;
	}
}

