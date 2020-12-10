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
#include "menu.hpp"

#ifdef NINTENDO
#include "nintendo/baronynx.hpp"
#endif

Player* players[MAXPLAYERS] = { nullptr };
Entity* selectedEntity[MAXPLAYERS] = { nullptr };
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


std::array<GameController, MAX_GAME_CONTROLLERS> game_controllers;
Inputs inputs;

GameController::GameController()
{
	sdl_device = nullptr;
	id = -1;
	name = "";
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
	id = -1;

	initBindings(); // clear status of all values
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

void GameController::initBindings() 
{
	for ( int i = SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_A; i < SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_MAX; ++i )
	{
		this->buttons[i].type = Binding_t::Bindtype_t::CONTROLLER_BUTTON;
		this->buttons[i].padButton = static_cast<SDL_GameControllerButton>(i);
		this->buttons[i].analog = 0.f;
		this->buttons[i].binary = false;
		this->buttons[i].consumed = false;
	}
	for ( int i = SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTX; i < SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_MAX; ++i )
	{
		this->axis[i].type = Binding_t::Bindtype_t::CONTROLLER_AXIS;
		this->axis[i].padAxis = static_cast<SDL_GameControllerAxis>(i);
		this->axis[i].analog = 0.f;
		this->axis[i].binary = false;
		this->axis[i].consumed = false;
	}
}

const bool GameController::isActive()
{
	return (sdl_device != nullptr);
}

void GameController::handleAnalog(int player)
{
	if (!isActive())
	{
		return;
	}

	//Right analog stick = look.

	if (!players[player]->shootmode || gamePaused)
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

		if ( rightStickDeadzoneType != DEADZONE_PER_AXIS )
		{
			rightx = getRawRightXMove();
			righty = getRawRightYMove();

			const real_t maxInputVector = 32767 * sqrt(2);
			const real_t magnitude = sqrt(pow(rightx, 2) + pow(righty, 2));
			const real_t normalised = magnitude / (maxInputVector);
			real_t deadzone = rightStickDeadzone / maxInputVector;

			if ( normalised < deadzone )
			{
				rightx = 0;
				righty = 0;
			}
			else
			{
				const real_t angle = atan2(righty, rightx);
				real_t newMagnitude = 0.0;
				newMagnitude = magnitude * (normalised - deadzone) / (1 - deadzone); // linear gradient
				rightx = newMagnitude * cos(angle) / gamepad_menux_sensitivity;
				righty = newMagnitude * sin(angle) / gamepad_menuy_sensitivity;

				//rightx = oldFloatRightX;
				//righty = oldFloatRightY;
			}

			oldAxisRightX = rightx;
			oldAxisRightY = righty;
		}

		oldFloatRightX += rightx;
		oldFloatRightY += righty;

		if (rightx || righty)
		{
			const auto& mouse = inputs.getVirtualMouse(player);
			mouse->lastMovementFromController = true;
			if ( inputs.bPlayerUsingKeyboardControl(player) )
			{
				//SDL_WarpMouseInWindow(screen, std::max(0, std::min(xres, mousex + rightx)), std::max(0, std::min(yres, mousey + righty)));
				//mouse->warpMouseInScreen(screen, rightx, righty);
				// smoother to use virtual mouse than push mouse events
				if ( gamePaused )
				{
					mouse->warpMouseInScreen(screen, rightx, righty);
				}
				else
				{
					mouse->warpMouseInCamera(cameras[player], rightx, righty);
				}
			}
			else
			{
				if ( gamePaused )
				{
					mouse->warpMouseInScreen(screen, rightx, righty);
				}
				else
				{
					mouse->warpMouseInCamera(cameras[player], rightx, righty);
				}
			}
		}
	}
	else
	{
		bool debugMouse = true;
		if ( debugMouse )
		{
			if ( keystatus[SDL_SCANCODE_F1] && keystatus[SDL_SCANCODE_EQUALS] )
			{
				leftStickDeadzone = std::min(leftStickDeadzone + 100, 32767);
			}
			else if ( keystatus[SDL_SCANCODE_F1] && keystatus[SDL_SCANCODE_MINUS] )
			{
				leftStickDeadzone = std::max(leftStickDeadzone - 100, 0);
			}
			if ( keystatus[SDL_SCANCODE_F2] && keystatus[SDL_SCANCODE_EQUALS] )
			{
				rightStickDeadzone = std::min(rightStickDeadzone + 100, 32767);
			}
			else if ( keystatus[SDL_SCANCODE_F2] && keystatus[SDL_SCANCODE_MINUS] )
			{
				rightStickDeadzone = std::max(rightStickDeadzone - 100, 0);
			}
		}

		int rightx = 0;
		int righty = 0;
		real_t floatx = 0;
		real_t floaty = 0;

		if ( rightStickDeadzoneType == DEADZONE_PER_AXIS )
		{
			rightx = getRightXMove();
			righty = getRightYMove();
		}
		else if ( rightStickDeadzoneType == DEADZONE_MAGNITUDE_LINEAR || rightStickDeadzoneType == DEADZONE_MAGNITUDE_HALFPIPE )
		{
			floatx = getRawRightXMove();
			floaty = getRawRightYMove();

			const real_t maxInputVector = 32767 * sqrt(2);
			const real_t magnitude = sqrt(pow(floatx, 2) + pow(floaty, 2));
			const real_t normalised = magnitude / (maxInputVector);
			real_t deadzone = rightStickDeadzone / maxInputVector;
			if ( rightStickDeadzoneType == DEADZONE_MAGNITUDE_HALFPIPE )
			{
				deadzone = rightStickDeadzone / maxInputVector;
			}

			if ( normalised < deadzone )
			{
				floatx = 0.0;
				floaty = 0.0;
			}
			else
			{
				const real_t angle = atan2(floaty, floatx);
				real_t newMagnitude = 0.0;
				if ( rightStickDeadzoneType == DEADZONE_MAGNITUDE_HALFPIPE )
				{
					newMagnitude = magnitude * pow((normalised - deadzone), 2); // half pipe gradient
				}
				else if ( rightStickDeadzoneType == DEADZONE_MAGNITUDE_LINEAR )
				{
					newMagnitude = magnitude * (normalised - deadzone) / (1 - deadzone); // linear gradient
				}
				floatx = newMagnitude * cos(angle) / gamepad_rightx_sensitivity;
				floaty = newMagnitude * sin(angle) / gamepad_righty_sensitivity;

				//rightx = oldFloatRightX;
				//righty = oldFloatRightY;
			}

			oldFloatRightX = floatx;
			oldFloatRightY = floaty;
			oldAxisRightX = getRawRightXMove();
			oldAxisRightY = getRawRightYMove();
		}

		//real_t x = SDL_GameControllerGetAxis(sdl_device, SDL_CONTROLLER_AXIS_RIGHTX);
		//real_t y = SDL_GameControllerGetAxis(sdl_device, SDL_CONTROLLER_AXIS_RIGHTY);
	
		if ( rightx || righty )
		{
			const auto& mouse = inputs.getVirtualMouse(player);
			mouse->lastMovementFromController = true;
			if ( inputs.bPlayerUsingKeyboardControl(player) )
			{
				SDL_Event e;
				e.type = SDL_MOUSEMOTION;
				e.motion.x = mousex + rightx;
				e.motion.y = mousey + righty;
				e.motion.xrel = rightx;
				e.motion.yrel = righty;
				e.user.code = 1;
				SDL_PushEvent(&e);
			}
			else
			{
				mouse->floatx += rightx;
				mouse->floaty += righty;
				mouse->floatxrel += rightx;
				mouse->floatyrel += righty;

				if ( !mouse->draw_cursor )
				{
					mouse->draw_cursor = true;
				}
				mouse->moved = true;
			}
		}
		if ( abs(floatx) > 0.0001 || abs(floaty) > 0.0001 )
		{
			const auto& mouse = inputs.getVirtualMouse(player);
			mouse->lastMovementFromController = true;

			mouse->floatx += floatx;
			mouse->floaty += floaty;
			mouse->floatxrel += floatx;
			mouse->floatyrel += floaty;

			if ( !mouse->draw_cursor )
			{
				mouse->draw_cursor = true;
			}
			mouse->moved = true;
		}
	}
}

int GameController::getLeftXMove() // with sensitivity
{
	if (!isActive())
	{
		return 0;
	}
	int x = getRawLeftXMove();
	x /= gamepad_leftx_sensitivity;
	return x;
}

int GameController::getLeftYMove() // with sensitivity
{
	if (!isActive())
	{
		return 0;
	}
	int y = -getRawLeftYMove();
	y /= gamepad_lefty_sensitivity;
	return y;
}

int GameController::getRightXMove() // with sensitivity
{
	if (!isActive())
	{
		return 0;
	}
	int x = getRawRightXMove();
	x /= gamepad_rightx_sensitivity;
	return x;
}

int GameController::getRightYMove() // with sensitivity
{
	if (!isActive())
	{
		return 0;
	}
	int y = getRawRightYMove();
	y /= gamepad_righty_sensitivity;
	return y;
}

int GameController::getLeftTrigger() { return getRawLeftTrigger(); } //No sensitivity taken into account (yet)
int GameController::getRightTrigger() { return getRawRightTrigger(); } //No sensitivity taken into account (yet)

int GameController::getRawLeftXMove() // no sensitivity
{
	if (!isActive())
	{
		return 0;
	}
	int x = SDL_GameControllerGetAxis(sdl_device, SDL_CONTROLLER_AXIS_LEFTX);
	if ( leftStickDeadzoneType == DEADZONE_PER_AXIS )
	{
		if (x < leftStickDeadzone && x > -leftStickDeadzone )
		{
			return 0;
		}
		if (x < -leftStickDeadzone )
		{
			x += leftStickDeadzone;
		}
		else
		{
			x -= leftStickDeadzone;
		}
	}
	return (!gamepad_leftx_invert) ? x : -x;
}

int GameController::getRawLeftYMove() // no sensitivity
{
	if (!isActive())
	{
		return 0;
	}
	int y = SDL_GameControllerGetAxis(sdl_device, SDL_CONTROLLER_AXIS_LEFTY);
	if ( leftStickDeadzoneType == DEADZONE_PER_AXIS )
	{
		if (y < leftStickDeadzone && y > -leftStickDeadzone )
		{
			return 0;
		}
		if (y < -leftStickDeadzone )
		{
			y += leftStickDeadzone;
		}
		else
		{
			y -= leftStickDeadzone;
		}
	}
	return (!gamepad_lefty_invert) ? -y : y;
}

int GameController::getRawRightXMove() // no sensitivity
{
	if (!isActive())
	{
		return 0;
	}
	int x = SDL_GameControllerGetAxis(sdl_device, SDL_CONTROLLER_AXIS_RIGHTX);
	if ( rightStickDeadzoneType == DEADZONE_PER_AXIS )
	{
		if (x < rightStickDeadzone && x > -rightStickDeadzone )
		{
			return 0;
		}
		if (x < -rightStickDeadzone )
		{
			x += rightStickDeadzone;
		}
		else
		{
			x -= rightStickDeadzone;
		}
	}
	return (!gamepad_rightx_invert) ? x : -x;
}

int GameController::getRawRightYMove() // no sensitivity
{
	if (!isActive())
	{
		return 0;
	}
	int y = SDL_GameControllerGetAxis(sdl_device, SDL_CONTROLLER_AXIS_RIGHTY);
	if ( rightStickDeadzoneType == DEADZONE_PER_AXIS )
	{
		if (y < rightStickDeadzone && y > -rightStickDeadzone )
		{
			return 0;
		}
		if (y < -rightStickDeadzone )
		{
			y += rightStickDeadzone;
		}
		else
		{
			y -= rightStickDeadzone;
		}
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

float GameController::getLeftXPercent() { return (float)getRawLeftXMove() / (float)maxLeftXMove(); }
float GameController::getLeftYPercent() { return (float)getRawLeftYMove() / (float)maxLeftYMove(); }
float GameController::getRightXPercent() { return (float)getRawRightXMove() / (float)maxRightXMove(); }
float GameController::getRightYPercent() { return (float)getRawRightYMove() / (float)maxRightYMove(); }

float GameController::getLeftTriggerPercent() { return (float)getRawLeftTrigger() / (float)maxLeftTrigger(); }
float GameController::getRightTriggerPercent() { return (float)getRawRightTrigger() / (float)maxRightTrigger(); }

//Ya, it's pretty constant in SDL2.
int GameController::maxLeftXMove() { return 32767 - (leftStickDeadzoneType == DEADZONE_PER_AXIS ? leftStickDeadzone : 0); }
int GameController::maxLeftYMove() { return 32767 - (leftStickDeadzoneType == DEADZONE_PER_AXIS ? leftStickDeadzone : 0); }
int GameController::maxRightXMove() { return 32767 - (rightStickDeadzoneType == DEADZONE_PER_AXIS ? rightStickDeadzone : 0); }
int GameController::maxRightYMove() { return 32767 - (rightStickDeadzoneType == DEADZONE_PER_AXIS ? rightStickDeadzone : 0); }
int GameController::maxLeftTrigger() { return 32767 - gamepad_deadzone; }
int GameController::maxRightTrigger() {	return 32767 - gamepad_deadzone; }

const bool hotbarGamepadControlEnabled(const int player)
{
	return (!openedChest[player]
		&& players[player]->gui_mode != GUI_MODE_SHOP
		&& !identifygui_active[player]
		&& !GenericGUI[player].isGUIOpen());
}

bool GameController::handleInventoryMovement(const int player)
{
	bool dpad_moved = false;

	if ( inputs.getUIInteraction(player)->itemMenuOpen )
	{
		return false;
	}

	auto& hotbar_t = players[player]->hotbar;

	if ( hotbar_t->hotbarHasFocus && !hotbarGamepadControlEnabled(player) )
	{
		hotbar_t->hotbarHasFocus = false;
	}

	if ( inputs.bControllerInputPressed(player, INJOY_DPAD_LEFT) )
	{
		if ( hotbar_t->hotbarHasFocus && hotbarGamepadControlEnabled(player) )
		{
			//If hotbar is focused and chest, etc, not opened, navigate hotbar.
			hotbar_t->selectHotbarSlot(hotbar_t->current_hotbar - 1);
			warpMouseToSelectedHotbarSlot(player);
		}
		else
		{
			//Navigate inventory.
			select_inventory_slot(player, 
				players[player]->inventoryUI.getSelectedSlotX() - 1, 
				players[player]->inventoryUI.getSelectedSlotY());
		}
		inputs.controllerClearInput(player, INJOY_DPAD_LEFT);

		dpad_moved = true;
	}

	if ( inputs.bControllerInputPressed(player, INJOY_DPAD_RIGHT) )
	{
		if ( hotbar_t->hotbarHasFocus && hotbarGamepadControlEnabled(player) )
		{
			//If hotbar is focused and chest, etc, not opened, navigate hotbar.
			hotbar_t->selectHotbarSlot(hotbar_t->current_hotbar + 1);
			warpMouseToSelectedHotbarSlot(player);
		}
		else
		{
			//Navigate inventory.
			select_inventory_slot(player, 
				players[player]->inventoryUI.getSelectedSlotX() + 1, 
				players[player]->inventoryUI.getSelectedSlotY());
		}
		inputs.controllerClearInput(player, INJOY_DPAD_RIGHT);

		dpad_moved = true;
	}

	if ( inputs.bControllerInputPressed(player, INJOY_DPAD_UP) )
	{
		if ( hotbar_t->hotbarHasFocus && hotbarGamepadControlEnabled(player) )
		{
			//Warp back to top of inventory.
			hotbar_t->hotbarHasFocus = false;
			float percentage = static_cast<float>(hotbar_t->current_hotbar + 1) / static_cast<float>(NUM_HOTBAR_SLOTS);
			select_inventory_slot(player, (percentage) * players[player]->inventoryUI.getSizeX() - 1, players[player]->inventoryUI.getSizeY() - 1);
		}
		else
		{
			select_inventory_slot(player, 
				players[player]->inventoryUI.getSelectedSlotX(),
				players[player]->inventoryUI.getSelectedSlotY() - 1); //Will handle warping to hotbar.
		}
		inputs.controllerClearInput(player, INJOY_DPAD_UP);

		dpad_moved = true;
	}

	if ( inputs.bControllerInputPressed(player, INJOY_DPAD_DOWN) )
	{
		if ( hotbar_t->hotbarHasFocus && hotbarGamepadControlEnabled(player) )
		{
			//Warp back to bottom of inventory.
			hotbar_t->hotbarHasFocus = false;
			float percentage = static_cast<float>(hotbar_t->current_hotbar + 1) / static_cast<float>(NUM_HOTBAR_SLOTS);
			select_inventory_slot(player, (percentage) * players[player]->inventoryUI.getSizeX() - 1, 0);
		}
		else
		{
			select_inventory_slot(player, 
				players[player]->inventoryUI.getSelectedSlotX(),
				players[player]->inventoryUI.getSelectedSlotY() + 1);
		}
		inputs.controllerClearInput(player, INJOY_DPAD_DOWN);

		dpad_moved = true;
	}

	if (dpad_moved)
	{
		dpad_moved = false;
		//inputs.getVirtualMouse(player)->draw_cursor = false;

		return true;
	}

	return false;
}

bool GameController::handleChestMovement(const int player)
{
	bool dpad_moved = false;

	if ( inputs.getUIInteraction(player)->itemMenuOpen )
	{
		return false;
	}

	if ( inputs.bControllerInputPressed(player, INJOY_DPAD_UP) )
	{
		selectChestSlot(player, selectedChestSlot[player] - 1);
		inputs.controllerClearInput(player, INJOY_DPAD_UP);

		dpad_moved = true;
	}

	if ( inputs.bControllerInputPressed(player, INJOY_DPAD_DOWN) )
	{
		selectChestSlot(player, selectedChestSlot[player] + 1);
		inputs.controllerClearInput(player, INJOY_DPAD_DOWN);

		dpad_moved = true;
	}

	if (dpad_moved)
	{
		dpad_moved = false;
		//inputs.getVirtualMouse(player)->draw_cursor = false;

		return true;
	}

	return false;
}

bool GameController::handleShopMovement(const int player)
{
	bool dpad_moved = false;

	if ( inputs.getUIInteraction(player)->itemMenuOpen )
	{
		return false;
	}

	/*
	//I would love to just do these, but it just wouldn't work with the way the code is set up.
	if ( inputs.bControllerInputPressed(player, INJOY_DPAD_LEFT) )
	{
		cycleShopCategories(-1);
		inputs.controllerClearInput(player, INJOY_DPAD_LEFT);

		dpad_moved = true;
	}

	if ( inputs.bControllerInputPressed(player, INJOY_DPAD_RIGHT) )
	{
		cycleShopCategories(1);
		inputs.controllerClearInput(player, INJOY_DPAD_RIGHT);

		dpad_moved = true;
	}*/

	if ( inputs.bControllerInputPressed(player, INJOY_DPAD_UP) )
	{
		selectShopSlot(selectedShopSlot - 1);
		inputs.controllerClearInput(player, INJOY_DPAD_UP);

		dpad_moved = true;
	}

	if ( inputs.bControllerInputPressed(player, INJOY_DPAD_DOWN) )
	{
		selectShopSlot(selectedShopSlot + 1);
		inputs.controllerClearInput(player, INJOY_DPAD_DOWN);

		dpad_moved = true;
	}

	if (dpad_moved)
	{
		dpad_moved = false;
		//inputs.getVirtualMouse(player)->draw_cursor = false;

		return true;
	}

	return false;
}

bool GameController::handleIdentifyMovement(const int player)
{
	bool dpad_moved = false;

	if ( inputs.getUIInteraction(player)->itemMenuOpen )
	{
		return false;
	}

	if ( inputs.bControllerInputPressed(player, INJOY_DPAD_UP) )
	{
		selectIdentifySlot(player, selectedIdentifySlot[player] - 1);
		inputs.controllerClearInput(player, INJOY_DPAD_UP);

		dpad_moved = true;
	}

	if ( inputs.bControllerInputPressed(player, INJOY_DPAD_DOWN) )
	{
		selectIdentifySlot(player, selectedIdentifySlot[player] + 1);
		inputs.controllerClearInput(player, INJOY_DPAD_DOWN);

		dpad_moved = true;
	}

	if (dpad_moved)
	{
		dpad_moved = false;
		//inputs.getVirtualMouse(player)->draw_cursor = false;

		return true;
	}

	return false;
}

bool GameController::handleRepairGUIMovement(const int player)
{
	bool dpad_moved = false;

	if ( inputs.getUIInteraction(player)->itemMenuOpen )
	{
		return false;
	}

	if ( inputs.bControllerInputPressed(player, INJOY_DPAD_UP) )
	{
		GenericGUI[player].selectSlot(GenericGUI[player].selectedSlot - 1);
		inputs.controllerClearInput(player, INJOY_DPAD_UP);

		dpad_moved = true;
	}

	if ( inputs.bControllerInputPressed(player, INJOY_DPAD_DOWN) )
	{
		GenericGUI[player].selectSlot(GenericGUI[player].selectedSlot + 1);
		inputs.controllerClearInput(player, INJOY_DPAD_DOWN);

		dpad_moved = true;
	}

	if ( dpad_moved )
	{
		dpad_moved = false;
		//inputs.getVirtualMouse(player)->draw_cursor = false;

		return true;
	}

	return false;
}

bool GameController::handleItemContextMenu(const int player, const Item& item)
{
	bool dpad_moved = false;

	if (!inputs.getUIInteraction(player)->itemMenuOpen)
	{
		return false;
	}

	if ( inputs.bControllerInputPressed(player, INJOY_DPAD_UP) )
	{
		selectItemMenuSlot(player, item, inputs.getUIInteraction(player)->itemMenuSelected - 1);
		inputs.controllerClearInput(player, INJOY_DPAD_UP);

		dpad_moved = true;
	}

	if ( inputs.bControllerInputPressed(player, INJOY_DPAD_DOWN) )
	{
		selectItemMenuSlot(player, item, inputs.getUIInteraction(player)->itemMenuSelected + 1);
		inputs.controllerClearInput(player, INJOY_DPAD_DOWN);

		dpad_moved = true;
	}

	if (dpad_moved)
	{
		dpad_moved = false;
		//inputs.getVirtualMouse(player)->draw_cursor = false;

		return true;
	}

	return false;
}


void initGameControllers()
{
#ifdef NINTENDO
	SDL_GameControllerAddMappingsFromFile(GAME_CONTROLLER_DB_FILEPATH);
#else
	SDL_GameControllerAddMappingsFromFile("gamecontrollerdb.txt");
#endif
	for ( auto& controller : game_controllers )
	{
		controller.close();
		controller.initBindings();
	}

	int c = 0;
	bool found = false;

	if ( c == 0 && !inputs.hasController(c) )
	{
		for ( auto& controller : game_controllers )
		{
			if ( controller.isActive() )
			{
				inputs.setControllerID(c, controller.getID());
				break;
			}
		}
	}

	inputs.setPlayerIDAllowedKeyboard(0);

	auto controller_itr = game_controllers.begin();
	for ( c = 0; c < SDL_NumJoysticks(); ++c )
	{
		if (SDL_IsGameController(c) && controller_itr->open(c))
		{
			printlog("(Device %d successfully initialized as game controller.)\n", c);
			inputs.setControllerID(c, controller_itr->getID());
			found = true;

			controller_itr = std::next(controller_itr);
			if ( controller_itr == game_controllers.end() )
			{
				printlog("Info: Max controller limit reached.");
				break;
			}
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

SDL_GameControllerButton GameController::getSDLButtonFromImpulse(const unsigned controllerImpulse)
{
	if ( controllerImpulse >= NUM_JOY_IMPULSES )
	{
		return SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_INVALID;
	}

	if ( joyimpulses[controllerImpulse] < 0 || joyimpulses[controllerImpulse] == SCANCODE_UNASSIGNED_BINDING )
	{
		return SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_INVALID;
	}

	return static_cast<SDL_GameControllerButton>(joyimpulses[controllerImpulse] - 301);
}

SDL_GameControllerAxis GameController::getSDLTriggerFromImpulse(const unsigned controllerImpulse)
{
	if ( controllerImpulse >= NUM_JOY_IMPULSES )
	{
		return SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_INVALID;
	}

	if ( joyimpulses[controllerImpulse] == 299 || joyimpulses[controllerImpulse] == 300 )
	{
		return static_cast<SDL_GameControllerAxis>(joyimpulses[controllerImpulse] - 299 + SDL_CONTROLLER_AXIS_TRIGGERLEFT);
	}

	return SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_INVALID;
}

Player::Player(int in_playernum, bool in_local_host) : 
	inventoryUI(*this),
	statusBarUI(*this),
	hud(*this),
	magic(*this),
	characterSheet(*this)
{
	local_host = false;
	playernum = in_playernum;
	entity = nullptr;
	cam = &cameras[playernum];
	hotbar = new Hotbar_t(*this);
}

Player::~Player()
{
	if (entity)
	{
		delete entity;
	}
	if ( hotbar )
	{
		delete hotbar;
	}
}

const bool Player::isLocalPlayer() const
{
	return ((splitscreen && bSplitscreen) || playernum == clientnum);
}
const bool Player::isLocalPlayerAlive() const
{
	return (isLocalPlayer() && entity && !client_disconnected[playernum]);
}

void Inputs::setMouse(const int player, MouseInputs input, Sint32 value)
{
	if ( bPlayerUsingKeyboardControl(player) )
	{
		switch ( input )
		{
			case OX:
				omousex = value;
				return;
			case OY:
				omousey = value;
				return;
			case X:
				mousex = value;
				return;
			case Y:
				mousey = value;
				return;
			case XREL:
				mousexrel = value;
				return;
			case YREL:
				mouseyrel = value;
				return;
			default:
				return;
		}
	}
	else if ( hasController(player) )
	{
		switch ( input )
		{
			case OX:
				getVirtualMouse(player)->ox = value;
				return;
			case OY:
				getVirtualMouse(player)->oy = value;
				return;
			case X:
				getVirtualMouse(player)->x = value;
				return;
			case Y:
				getVirtualMouse(player)->y = value;
				return;
			case XREL:
				getVirtualMouse(player)->xrel = value;
				return;
			case YREL:
				getVirtualMouse(player)->yrel = value;
				return;
			default:
				return;
		}
	}
}

const Sint32 Inputs::getMouse(const int player, MouseInputs input)
{
	if ( bPlayerUsingKeyboardControl(player) && (!getVirtualMouse(player)->lastMovementFromController 
		|| (players[player]->shootmode && !gamePaused && !intro)) )
	{
		// add controller virtual mouse if applicable, only in shootmode
		// shootmode has no limits on rotation, but !shootmode is inventory

		bool combineMouseInputs = (players[player]->shootmode && hasController(player)) && !gamePaused && !intro;

		switch ( input )
		{
			case OX:
				return omousex + ((combineMouseInputs) ? getVirtualMouse(player)->ox : 0);
				//return omousex;
			case OY:
				return omousey + ((combineMouseInputs) ? getVirtualMouse(player)->oy : 0);
				//return omousey;
			case X:
				return mousex + ((combineMouseInputs) ? getVirtualMouse(player)->x : 0);
				//return mousex;
			case Y:
				return mousey + ((combineMouseInputs) ? getVirtualMouse(player)->y : 0);
				//return mousey;
			case XREL:
				return mousexrel + ((combineMouseInputs) ? getVirtualMouse(player)->xrel : 0);
				//return mousexrel;
			case YREL:
				return mouseyrel + ((combineMouseInputs) ? getVirtualMouse(player)->yrel : 0);
				//return mouseyrel;
			default:
				return 0;
		}
	}
	else if ( hasController(player) )
	{
		switch ( input )
		{
			case OX:
				return getVirtualMouse(player)->ox;
			case OY:
				return getVirtualMouse(player)->oy;
			case X:
				return getVirtualMouse(player)->x;
			case Y:
				return getVirtualMouse(player)->y;
			case XREL:
				return getVirtualMouse(player)->xrel;
			case YREL:
				return getVirtualMouse(player)->yrel;
			default:
				return 0;
		}
	}
	return 0;
}

const real_t Inputs::getMouseFloat(const int player, MouseInputs input)
{
	if ( bPlayerUsingKeyboardControl(player) && (!getVirtualMouse(player)->lastMovementFromController
		|| (players[player]->shootmode && !gamePaused && !intro)) )
	{
		// add controller virtual mouse if applicable, only in shootmode
		// shootmode has no limits on rotation, but !shootmode is inventory

		bool combineMouseInputs = (players[player]->shootmode && hasController(player)) && !gamePaused && !intro;

		switch ( input )
		{
			case OX:
				return omousex + ((combineMouseInputs) ? getVirtualMouse(player)->ox : 0);
				//return omousex;
			case OY:
				return omousey + ((combineMouseInputs) ? getVirtualMouse(player)->oy : 0);
				//return omousey;
			case X:
				return mousex + ((combineMouseInputs) ? getVirtualMouse(player)->x : 0);
				//return mousex;
			case Y:
				return mousey + ((combineMouseInputs) ? getVirtualMouse(player)->y : 0);
				//return mousey;
			case XREL:
				return mousexrel + ((combineMouseInputs) ? getVirtualMouse(player)->xrel : 0);
				//return mousexrel;
			case YREL:
				return mouseyrel + ((combineMouseInputs) ? getVirtualMouse(player)->yrel : 0);
				//return mouseyrel;
			case ANALOGUE_OX:
				return omousex + ((combineMouseInputs) ? getVirtualMouse(player)->floatox : 0);
				//return omousex;
			case ANALOGUE_OY:
				return omousey + ((combineMouseInputs) ? getVirtualMouse(player)->floatoy : 0);
				//return omousey;
			case ANALOGUE_X:
				return mousex + ((combineMouseInputs) ? getVirtualMouse(player)->floatx : 0);
				//return mousex;
			case ANALOGUE_Y:
				return mousey + ((combineMouseInputs) ? getVirtualMouse(player)->floaty : 0);
				//return mousey;
			case ANALOGUE_XREL:
				return mousexrel + ((combineMouseInputs) ? getVirtualMouse(player)->floatxrel : 0);
				//return mousexrel;
			case ANALOGUE_YREL:
				return mouseyrel + ((combineMouseInputs) ? getVirtualMouse(player)->floatyrel : 0);
				//return mouseyrel;
			default:
				return 0;
		}
	}
	else if ( hasController(player) )
	{
		switch ( input )
		{
			case OX:
				return getVirtualMouse(player)->ox;
			case OY:
				return getVirtualMouse(player)->oy;
			case X:
				return getVirtualMouse(player)->x;
			case Y:
				return getVirtualMouse(player)->y;
			case XREL:
				return getVirtualMouse(player)->xrel;
			case YREL:
				return getVirtualMouse(player)->yrel;
			case ANALOGUE_OX:
				return getVirtualMouse(player)->floatox;
			case ANALOGUE_OY:
				return getVirtualMouse(player)->floatoy;
			case ANALOGUE_X:
				return getVirtualMouse(player)->floatx;
			case ANALOGUE_Y:
				return getVirtualMouse(player)->floaty;
			case ANALOGUE_XREL:
				return getVirtualMouse(player)->floatxrel;
			case ANALOGUE_YREL:
				return getVirtualMouse(player)->floatyrel;
			default:
				return 0;
		}
	}
	return 0;
}

void Inputs::warpMouse(const int player, const Sint32 x, const Sint32 y, Uint32 flags)
{
	if ( inputs.bPlayerUsingKeyboardControl(player) && (flags & SET_MOUSE) )
	{
		if ( flags & UNSET_RELATIVE_MOUSE )
		{
			SDL_SetRelativeMouseMode(SDL_FALSE);
		}
		else if ( flags & SET_RELATIVE_MOUSE )
		{
			SDL_SetRelativeMouseMode(SDL_TRUE);
		}
		SDL_WarpMouseInWindow(screen, x, y); // this pushes to the SDL event queue
		
		// if we don't set mousex/y here, the mouse will flicker until the event is popped
		mousex = x;
		mousey = y;
		
		//// not sure about omousex/y here...
		omousex = x;
		omousey = y;
	}
	if ( inputs.hasController(player) && (flags & SET_CONTROLLER) )
	{
		const auto& mouse = inputs.getVirtualMouse(player);
		mouse->x = x;
		mouse->y = y;

		// not sure about omousex/y here...
		mouse->ox = x;
		mouse->oy = y;
	}
}

GameController* Inputs::getController(int player) const
{
	if ( player < 0 || player >= MAXPLAYERS )
	{
		printlog("[INPUTS]: Warning: player index %d out of range.", player);
		return nullptr;
	}
	if ( !hasController(player) )
	{
		return nullptr;
	}

	for ( auto& controller : game_controllers )
	{
		if ( controller.getID() == getControllerID(player) )
		{
			return &controller;
		}
	}
	return nullptr;
}

const bool Inputs::bControllerInputPressed(int player, const unsigned controllerImpulse) const
{
	if ( controllerImpulse >= NUM_JOY_IMPULSES )
	{
		return false;
	}
	if ( !bPlayerIsControllable(player) )
	{
		return false;
	}
	const GameController* controller = getController(player);
	if ( !controller )
	{
		return false;
	}

	if ( joyimpulses[controllerImpulse] == 299 || joyimpulses[controllerImpulse] == 300 ) // triggers
	{
		return controller->binaryToggle(GameController::getSDLTriggerFromImpulse(controllerImpulse));
	}
	else
	{
		return controller->binaryToggle(GameController::getSDLButtonFromImpulse(controllerImpulse));
	}
}

const bool Inputs::bMouseLeft(int player) const
{
	if ( !bPlayerIsControllable(player) )
	{
		return false;
	}
	if ( bPlayerUsingKeyboardControl(player) )
	{
		if ( mousestatus[SDL_BUTTON_LEFT] )
		{
			return true;
		}
	}

	bool hackFromPreviousCode = bControllerInputPressed(player, INJOY_MENU_LEFT_CLICK);
	if ( hackFromPreviousCode && ((!players[player]->shootmode && players[player]->gui_mode == GUI_MODE_NONE) || gamePaused) && rebindaction == -1 )
	{
		return true;
	}
	return false;
}

const bool Inputs::bMouseRight(int player) const
{
	if ( !bPlayerIsControllable(player) )
	{
		return false;
	}
	if ( bPlayerUsingKeyboardControl(player) && mousestatus[SDL_BUTTON_RIGHT] )
	{
		return true;
	}
	return false;
}

const void Inputs::mouseClearLeft(int player)
{
	if ( !bPlayerIsControllable(player) )
	{
		return;
	}
	if ( bPlayerUsingKeyboardControl(player) )
	{
		mousestatus[SDL_BUTTON_LEFT] = 0;
	}

	controllerClearInput(player, INJOY_MENU_LEFT_CLICK);
	//bool hackFromPreviousCode = bControllerInputPressed(player, INJOY_MENU_LEFT_CLICK);
	//if ( hackFromPreviousCode )
	//{
	//}
}

const void Inputs::mouseClearRight(int player)
{
	if ( !bPlayerIsControllable(player) )
	{
		return;
	}
	if ( bPlayerUsingKeyboardControl(player) )
	{
		mousestatus[SDL_BUTTON_RIGHT] = 0;
	}
}

void Inputs::controllerClearInput(int player, const unsigned controllerImpulse)
{
	if ( controllerImpulse >= NUM_JOY_IMPULSES )
	{
		return;
	}
	if ( !bPlayerIsControllable(player) )
	{
		return;
	}
	GameController* controller = getController(player);
	if ( !controller )
	{
		return;
	}

	if ( joyimpulses[controllerImpulse] == 299 || joyimpulses[controllerImpulse] == 300 ) // triggers
	{
		controller->consumeBinaryToggle(GameController::getSDLTriggerFromImpulse(controllerImpulse));
	}
	else
	{
		controller->consumeBinaryToggle(GameController::getSDLButtonFromImpulse(controllerImpulse));
	}
}

const bool Inputs::bPlayerIsControllable(int player) const
{
	if ( player < 0 || player >= MAXPLAYERS )
	{
		printlog("[INPUTS]: Warning: player index %d out of range.", player);
		return false;
	}
	if ( player == clientnum )
	{
		return true;
	}
	if ( player != clientnum )
	{
		if ( splitscreen )
		{
			// do some additional logic for 2/3 player here, otherwise all 4 players valid
			return true;
		}
	}

	//printlog("[INPUTS]: Warning: player index %d invalid.", player);
	return false;
}

void Inputs::controllerHandleMouse(int player)
{
	if ( !bPlayerIsControllable(player) )
	{
		return;
	}
	GameController* controller = getController(player);
	if ( !controller )
	{
		return;
	}

	controller->handleAnalog(player);
}

void initIdentifyGUIControllerCode(const int player)
{
	if ( identify_items[player][0] )
	{
		selectedIdentifySlot[player] = 0;
		warpMouseToSelectedIdentifySlot(player);
	}
	else
	{
		selectedIdentifySlot[player] = -1;
	}
}

bool GameController::binaryOf(Binding_t& binding) 
{
	if ( binding.type == Binding_t::CONTROLLER_AXIS || binding.type == Binding_t::CONTROLLER_BUTTON ) 
	{
		SDL_GameController* pad = sdl_device;
		if ( binding.type == Binding_t::CONTROLLER_BUTTON ) 
		{
			return SDL_GameControllerGetButton(pad, binding.padButton) == 1;
		}
		else 
		{
			if ( binding.padAxisNegative ) 
			{
				return SDL_GameControllerGetAxis(pad, binding.padAxis) < -16384;
			}
			else 
			{
				return SDL_GameControllerGetAxis(pad, binding.padAxis) > 16384;
			}
		}
	}

	return false;
}

float GameController::analogOf(Binding_t& binding) 
{
	if ( binding.type == Binding_t::CONTROLLER_AXIS || binding.type == Binding_t::CONTROLLER_BUTTON ) 
	{
		SDL_GameController* pad = sdl_device;
		if ( binding.type == Binding_t::CONTROLLER_BUTTON ) 
		{
			return SDL_GameControllerGetButton(pad, binding.padButton) ? 1.f : 0.f;
		}
		else 
		{
			if ( binding.padAxisNegative )
			{
				float result = std::min(SDL_GameControllerGetAxis(pad, binding.padAxis) / 32768.f, 0.f) * -1.f;
				return (fabs(result) > binding.deadzone) ? result : 0.f;
			}
			else 
			{
				float result = std::max(SDL_GameControllerGetAxis(pad, binding.padAxis) / 32767.f, 0.f);
				return (fabs(result) > binding.deadzone) ? result : 0.f;
			}
		}
	}

	return 0.f;
}

void GameController::updateButtons()
{
	if ( !isActive() )
	{
		return;
	}

	for ( int i = 0; i < NUM_JOY_STATUS; ++i )
	{
		buttons[i].analog = analogOf(buttons[i]);

		bool oldBinary = buttons[i].binary;
		buttons[i].binary = binaryOf(buttons[i]);
		if ( oldBinary != buttons[i].binary ) {
			// unconsume the input whenever it's released or pressed again.
			//messagePlayer(0, "%d: %d", i, buttons[i].binary ? 1 : 0);
			buttons[i].consumed = false;
		}
	}
}

void GameController::updateAxis()
{
	if ( !isActive() )
	{
		return;
	}

	for ( int i = 0; i < NUM_JOY_AXIS_STATUS; ++i )
	{
		axis[i].analog = analogOf(axis[i]);

		bool oldBinary = axis[i].binary;
		axis[i].binary = binaryOf(axis[i]);
		if ( oldBinary != axis[i].binary ) {
			// unconsume the input whenever it's released or pressed again.
			//messagePlayer(0, "%d: %d", i, axis[i].binary ? 1 : 0);
			axis[i].consumed = false;
		}
	}
}

float GameController::analog(SDL_GameControllerButton binding) const
{
	if ( binding == SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_INVALID || binding >= SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_MAX )
	{
		return 0.f;
	}
	return buttons[binding].analog;
}

bool GameController::binaryToggle(SDL_GameControllerButton binding) const
{
	if ( binding == SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_INVALID || binding >= SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_MAX )
	{
		return false;
	}
	return (buttons[binding].binary && !buttons[binding].consumed);
}

bool GameController::binary(SDL_GameControllerButton binding) const
{
	if ( binding == SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_INVALID || binding >= SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_MAX )
	{
		return false;
	}
	return buttons[binding].binary;
}

void GameController::consumeBinaryToggle(SDL_GameControllerButton binding)
{
	if ( binding == SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_INVALID || binding >= SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_MAX )
	{
		return;
	}
	if ( buttons[binding].binary )
	{
		buttons[binding].consumed = true;
	}
}

float GameController::analog(SDL_GameControllerAxis binding) const
{
	if ( binding == SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_INVALID || binding >= SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_MAX )
	{
		return 0.f;
	}
	return axis[binding].analog;
}

bool GameController::binaryToggle(SDL_GameControllerAxis binding) const
{
	if ( binding == SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_INVALID || binding >= SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_MAX )
	{
		return false;
	}
	return (axis[binding].binary && !axis[binding].consumed);
}

bool GameController::binary(SDL_GameControllerAxis binding) const
{
	if ( binding == SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_INVALID || binding >= SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_MAX )
	{
		return false;
	}
	return axis[binding].binary;
}

void GameController::consumeBinaryToggle(SDL_GameControllerAxis binding)
{
	if ( binding == SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_INVALID || binding >= SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_MAX )
	{
		return;
	}
	if ( axis[binding].binary )
	{
		axis[binding].consumed = true;
	}
}