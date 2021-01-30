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
#include "collision.hpp"
#include "mod_tools.hpp"
#include "draw.hpp"
#include "colors.hpp"

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
	sdl_haptic = nullptr;
	id = -1;
	name = "";
}

GameController::~GameController()
{
	if ( sdl_device )
	{
		close();
	}
}

void GameController::close()
{
	if ( sdl_haptic )
	{
		SDL_HapticClose(sdl_haptic);
		sdl_haptic = nullptr;
	}

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
		sdl_haptic = SDL_HapticOpen(c);
		if ( sdl_haptic == nullptr )
		{
			printf("Notice: Controller does not support haptics! SDL Error: %s\n", SDL_GetError());
		}
		else
		{
			//Get initialize rumble
			/*if ( SDL_HapticRumbleInit(sdl_haptic) < 0 )
			{
				printf("Warning: Unable to initialize rumble! SDL Error: %s\n", SDL_GetError());
				SDL_HapticClose(sdl_haptic);
				sdl_haptic = nullptr;
			}*/
		}
		if ( sdl_haptic )
		{
			printlog("Controller name is \"%s\", haptics available: %d", name.c_str(), SDL_HapticQuery(sdl_haptic));
		}
		else
		{
			printlog("Controller name is \"%s\", haptics disabled", name.c_str());
		}
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
		this->buttons[i].binaryRelease = false;
		this->buttons[i].binaryReleaseConsumed = false;
		this->buttons[i].consumed = false;
	}
	for ( int i = SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTX; i < SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_MAX; ++i )
	{
		this->axis[i].type = Binding_t::Bindtype_t::CONTROLLER_AXIS;
		this->axis[i].padAxis = static_cast<SDL_GameControllerAxis>(i);
		this->axis[i].analog = 0.f;
		this->axis[i].binary = false;
		this->axis[i].binaryRelease = false;
		this->axis[i].binaryReleaseConsumed = false;
		this->axis[i].consumed = false;
	}
	this->virtualDpad.padVirtualDpad = DpadDirection::CENTERED;
	this->virtualDpad.type = Binding_t::Bindtype_t::VIRTUAL_DPAD;
	this->virtualDpad.analog = 0.f;
	this->virtualDpad.binary = false;
	this->virtualDpad.binaryRelease = false;
	this->virtualDpad.binaryReleaseConsumed = false;
	this->virtualDpad.consumed = false;

	this->radialSelection.padRadialSelection = RadialSelection::RADIAL_CENTERED;
	this->radialSelection.type = Binding_t::Bindtype_t::RADIAL_SELECTION;
	this->radialSelection.analog = 0.f;
	this->radialSelection.binary = false;
	this->radialSelection.binaryRelease = false;
	this->radialSelection.binaryReleaseConsumed = false;
	this->radialSelection.consumed = false;
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

	bool radialMenuOpen = FollowerMenu[player].followerMenuIsOpen();
	if ( !radialMenuOpen )
	{
		consumeDpadDirToggle();
		virtualDpad.padVirtualDpad = DpadDirection::CENTERED;
	}

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

		if ( radialMenuOpen )
		{
			real_t floatx = getRightXPercent();
			real_t floaty = getRightYPercent();
			const int numoptions = 8;
			real_t magnitude = sqrt(pow(floaty, 2) + pow(floatx, 2));
			DpadDirection dir = DpadDirection::CENTERED;
			if ( magnitude > 1 )
			{
				real_t stickAngle = atan2(floaty, floatx);
				while ( stickAngle >= (2 * PI + (PI / 2 - (PI / numoptions))) )
				{
					stickAngle -= PI * 2;
				}
				while ( stickAngle < (PI / 2 - (PI / numoptions)) )
				{
					stickAngle += PI * 2;
				}

				real_t angleStart = PI / 2 - (PI / numoptions);
				real_t angleMiddle = angleStart + PI / numoptions;
				real_t angleEnd = angleMiddle + PI / numoptions;
				for ( int i = 0; i < numoptions; ++i )
				{
					if ( (stickAngle >= angleStart && stickAngle < angleEnd) )
					{
						dir = static_cast<DpadDirection>(i);
					}
					angleStart += 2 * PI / numoptions;
					angleMiddle = angleStart + PI / numoptions;
					angleEnd = angleMiddle + PI / numoptions;
				}
				DpadDirection oldDpad = virtualDpad.padVirtualDpad;
				virtualDpad.padVirtualDpad = dir;
				if ( oldDpad != virtualDpad.padVirtualDpad ) 
				{
					// unconsume the input whenever it's released or pressed again.
					virtualDpad.consumed = false;
					//messagePlayer(0, "%d", virtualDpad.padVirtualDpad);
				}
			}
			rightx = 0;
			righty = 0;
		}
		else if ( rightStickDeadzoneType != DEADZONE_PER_AXIS )
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

			if ( !mouse->draw_cursor )
			{
				mouse->draw_cursor = true;
			}

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

		if ( false && players[player]->hotbar.useHotbarRadialMenu
			&& buttonHeldToggle(SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_LEFTSHOULDER) )
		{
			real_t floatx = getRightXPercent();
			real_t floaty = getRightYPercent();
			const int numoptions = players[player]->hotbar.radialHotbarSlots;
			real_t magnitude = sqrt(pow(floaty, 2) + pow(floatx, 2));
			RadialSelection dir = RadialSelection::RADIAL_INVALID;
			if ( magnitude > 1 )
			{
				real_t stickAngle = atan2(floaty, floatx);
				while ( stickAngle >= (2 * PI + (2 * PI /*- (PI / numoptions)*/)) )
				{
					stickAngle -= PI * 2;
				}
				while ( stickAngle < (2 * PI /*- (PI / numoptions)*/) )
				{
					stickAngle += PI * 2;
				}

				real_t angleStart = 2 * PI /*- (PI / (2 * numoptions))*/;
				real_t angleMiddle = angleStart + PI / (2 * numoptions);
				real_t angleEnd = angleMiddle + PI / (2 * numoptions);
				for ( int i = 0; i < numoptions * 2; ++i )
				{
					if ( (stickAngle >= angleStart && stickAngle < angleEnd) )
					{
						dir = static_cast<RadialSelection>(i);
					}
					angleStart += 2 * PI / (2 * numoptions);
					angleMiddle = angleStart + PI / (2 * numoptions);
					angleEnd = angleMiddle + PI / (2 * numoptions);
				}
				RadialSelection oldRadial = radialSelection.padRadialSelection;
				radialSelection.padRadialSelection = dir;
				if ( oldRadial != radialSelection.padRadialSelection )
				{
					// unconsume the input whenever it's released or pressed again.
					radialSelection.consumed = false;

					if ( radialSelection.padRadialSelection > RADIAL_CENTERED )
					{
						int hotbarSlot = -1;
						if ( radialSelection.padRadialSelection < numoptions )
						{
							hotbarSlot = numoptions - radialSelection.padRadialSelection - 1;
						}
						else
						{
							hotbarSlot = radialSelection.padRadialSelection - numoptions;
						}
						if ( hotbarSlot >= 0 )
						{
							players[player]->hotbar.selectHotbarSlot(hotbarSlot);
						}
						messagePlayer(0, "%d", hotbarSlot);
					}
				}
			}
			else
			{
				radialSelection.consumed = false;
			}
			rightx = 0;
			righty = 0;
		}
		else if ( rightStickDeadzoneType == DEADZONE_PER_AXIS )
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

		if ( players[player]->hotbar.useHotbarRadialMenu
			&& binaryToggle(SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_LEFTSHOULDER) )
		{
			if ( floatx > 0.0001 )
			{
				players[player]->hotbar.radialHotbarProgress += 5;
			}
			else if ( floatx < -0.0001 )
			{
				players[player]->hotbar.radialHotbarProgress -= 5;
			}
			players[player]->hotbar.radialHotbarProgress = std::max(0, players[player]->hotbar.radialHotbarProgress);
			players[player]->hotbar.radialHotbarProgress = std::min(1000, players[player]->hotbar.radialHotbarProgress);
			int slot = players[player]->hotbar.radialHotbarProgress / 100;
			int oldSlot = players[player]->hotbar.current_hotbar;
			players[player]->hotbar.selectHotbarSlot(slot);
			inputs.getController(player)->radialSelection.consumed = false;
			if ( oldSlot != slot )
			{
			}
		}
		else if ( abs(floatx) > 0.0001 || abs(floaty) > 0.0001 )
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

float GameController::getLeftXPercentForPlayerMovement()
{
	float x_force = getLeftXPercent();
	if ( x_force > 0 )
	{
		x_force = std::min(x_force / x_forceMaxForwardThreshold, 1.f);
	}
	else if ( x_force < 0 )
	{
		x_force = std::max(x_force / x_forceMaxBackwardThreshold, -1.f);
	}
	return x_force;
}
float GameController::getLeftYPercentForPlayerMovement()
{
	float y_force = getLeftYPercent();
	if ( y_force > 0 )
	{
		y_force = std::min(y_force / y_forceMaxStrafeThreshold, 1.f);
	}
	else if ( y_force < 0 )
	{
		y_force = std::max(y_force / y_forceMaxStrafeThreshold, -1.f);
	}
	return y_force;
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

	if ( inputs.bControllerRawInputPressed(player, 301 + SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) )
	{
		hotbar_t.hotbarHasFocus = !hotbar_t.hotbarHasFocus;
		if ( hotbar_t.hotbarHasFocus )
		{
			warpMouseToSelectedHotbarSlot(player);
		}
		else
		{
			warpMouseToSelectedInventorySlot(player);
		}
		inputs.controllerClearRawInput(player, 301 + SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
		inputs.getVirtualMouse(player)->draw_cursor = false;
	}

	if ( hotbar_t.hotbarHasFocus && !hotbarGamepadControlEnabled(player) )
	{
		hotbar_t.hotbarHasFocus = false;
	}

	if ( inputs.bControllerInputPressed(player, INJOY_DPAD_LEFT) )
	{
		if ( hotbar_t.hotbarHasFocus && hotbarGamepadControlEnabled(player) )
		{
			//If hotbar is focused and chest, etc, not opened, navigate hotbar.
			int newSlot = hotbar_t.current_hotbar - 1;
			/*if ( hotbar_t.useHotbarFaceMenu )
			{
				if ( hotbar_t.current_hotbar == 2 || hotbar_t.current_hotbar == 6 
					|| hotbar_t.current_hotbar == 3 || hotbar_t.current_hotbar == 7 )
				{
					newSlot = hotbar_t.current_hotbar - 2;
				}
			}*/

			hotbar_t.selectHotbarSlot(newSlot);
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
		if ( hotbar_t.hotbarHasFocus && hotbarGamepadControlEnabled(player) )
		{
			//If hotbar is focused and chest, etc, not opened, navigate hotbar.
			int newSlot = hotbar_t.current_hotbar + 1;
			/*if ( hotbar_t.useHotbarFaceMenu )
			{
				if ( hotbar_t.current_hotbar == 1 || hotbar_t.current_hotbar == 5 )
				{
					newSlot = hotbar_t.current_hotbar + 2;
				}
			}*/

			hotbar_t.selectHotbarSlot(newSlot);
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
		if ( hotbar_t.hotbarHasFocus && hotbarGamepadControlEnabled(player) )
		{
			/*if ( hotbar_t.useHotbarFaceMenu && (hotbar_t.current_hotbar == 2 || hotbar_t.current_hotbar == 6) )
			{
				int newSlot = hotbar_t.current_hotbar - 1;
				hotbar_t.selectHotbarSlot(newSlot);
				warpMouseToSelectedHotbarSlot(player);
			}
			else*/
			{
				//Warp back to top of inventory.
				hotbar_t.hotbarHasFocus = false;
				float percentage = static_cast<float>(hotbar_t.current_hotbar + 1) / static_cast<float>(NUM_HOTBAR_SLOTS);
				select_inventory_slot(player, (percentage) * players[player]->inventoryUI.getSizeX() - 1, players[player]->inventoryUI.getSizeY() - 1);
			}
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
		if ( hotbar_t.hotbarHasFocus && hotbarGamepadControlEnabled(player) )
		{
			/*if ( hotbar_t.useHotbarFaceMenu && (hotbar_t.current_hotbar == 1 || hotbar_t.current_hotbar == 5) )
			{
				int newSlot = hotbar_t.current_hotbar + 1;
				hotbar_t.selectHotbarSlot(newSlot);
				warpMouseToSelectedHotbarSlot(player);
			}
			else*/
			{
				//Warp back to bottom of inventory.
				hotbar_t.hotbarHasFocus = false;
				float percentage = static_cast<float>(hotbar_t.current_hotbar + 1) / static_cast<float>(NUM_HOTBAR_SLOTS);
				select_inventory_slot(player, (percentage) * players[player]->inventoryUI.getSizeX() - 1, 0);
			}
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
		inputs.getVirtualMouse(player)->draw_cursor = false;

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
		selectShopSlot(player, selectedShopSlot[player] - 1);
		inputs.controllerClearInput(player, INJOY_DPAD_UP);

		dpad_moved = true;
	}

	if ( inputs.bControllerInputPressed(player, INJOY_DPAD_DOWN) )
	{
		selectShopSlot(player, selectedShopSlot[player] + 1);
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

GameController::Haptic_t::Haptic_t()
{
	hapticTick = 0;
	memset(&hapticEffect, 0, sizeof(SDL_HapticEffect));
}

void GameController::handleRumble()
{
	if ( haptics.hapticTick == std::numeric_limits<Uint32>::max() )
	{
		haptics.hapticTick = 0;
	}
	else
	{
		++haptics.hapticTick;
	}
	size_t size = haptics.activeRumbles.size();
	if ( !sdl_haptic || size == 0 )
	{
		return;
	}

	Uint32 highestPriority = 0;
	Uint32 earliestTick = std::numeric_limits<Uint32>::max();
	for ( auto it = haptics.activeRumbles.begin(); it != haptics.activeRumbles.end(); /*blank*/ )
	{
		Uint32 priority = it->first;
		auto& rumble = it->second;
		if ( haptics.hapticTick - rumble.startTick >= rumble.length ) // expired length
		{
			it = haptics.activeRumbles.erase(it);
			continue;
		}
		++it;
	}

	std::vector<std::pair<Uint32, Haptic_t::Rumble>>::iterator rumbleToPlay = haptics.activeRumbles.end();
	for ( auto it = haptics.activeRumbles.begin(); it != haptics.activeRumbles.end(); ++it )
	{
		Uint32 priority = it->first;
		auto& rumble = it->second;
		if ( priority > highestPriority )
		{
			highestPriority = priority;
			earliestTick = rumble.startTick;
			rumbleToPlay = it;
		}
		else if ( highestPriority == priority )
		{
			if ( rumble.startTick < earliestTick )
			{
				earliestTick = rumble.startTick;
				rumbleToPlay = it;
			}
		}
	}

	for ( auto it = haptics.activeRumbles.begin(); it != haptics.activeRumbles.end(); ++it )
	{
		if ( it != rumbleToPlay )
		{
			it->second.isPlaying = false;
		}
	}

	if ( rumbleToPlay != haptics.activeRumbles.end() 
		&& (!rumbleToPlay->second.isPlaying 
			|| rumbleToPlay->second.pattern == Haptic_t::RUMBLE_BOULDER 
			|| rumbleToPlay->second.pattern == Haptic_t::RUMBLE_BOULDER_BOUNCE
			|| rumbleToPlay->second.pattern == Haptic_t::RUMBLE_DEATH
			|| rumbleToPlay->second.pattern == Haptic_t::RUMBLE_TMP))
	{
		Uint32 newStartTime = (haptics.hapticTick - rumbleToPlay->second.startTick);
		rumbleToPlay->second.startTime = newStartTime; // move the playhead forward.
		rumbleToPlay->second.isPlaying = true;
		doRumble(&rumbleToPlay->second);
	}
}

void GameController::addRumble(Haptic_t::RumblePattern pattern, Uint16 smallMagnitude, Uint16 largeMagnitude, Uint32 length, Uint32 srcEntityUid)
{
	if ( !sdl_haptic )
	{
		return;
	}
	if ( !haptics.vibrationEnabled )
	{
		return;
	}
	Uint32 priority = 1;
	if ( pattern == Haptic_t::RumblePattern::RUMBLE_NORMAL )
	{
		priority = 1;
	}
	else if ( pattern == Haptic_t::RumblePattern::RUMBLE_DEATH )
	{
		priority = 20;
	}
	else if ( pattern == Haptic_t::RumblePattern::RUMBLE_BOULDER )
	{
		priority = 9;
	}
	else if ( pattern == Haptic_t::RumblePattern::RUMBLE_BOULDER_BOUNCE )
	{
		priority = 10;
	}
	else if ( pattern == Haptic_t::RumblePattern::RUMBLE_BOULDER_ROLLING )
	{
		priority = 1;
	}
	else if ( pattern == Haptic_t::RumblePattern::RUMBLE_TMP )
	{
		priority = 5;
	}
	else
	{
		priority = 1;
	}
	haptics.activeRumbles.push_back(std::make_pair(priority, Haptic_t::Rumble(smallMagnitude, largeMagnitude, length, haptics.hapticTick, srcEntityUid)));
	haptics.activeRumbles.back().second.pattern = pattern;
}

void Inputs::addRumbleForPlayerHPLoss(const int player, Sint32 damageAmount)
{
	if ( player >= 0 && players[player]->isLocalPlayer() && stats[player] && stats[player]->OLDHP >= stats[player]->HP )
	{
		if ( stats[player]->HP <= 0 && stats[player]->OLDHP <= 0 )
		{
			// already ded.
			return;
		}

		real_t percentHPLost = std::min(1.0, (stats[player]->OLDHP - stats[player]->HP) / static_cast<real_t>(std::max(1, stats[player]->MAXHP)));
		if ( stats[player]->HP <= 0 )
		{
			rumble(player, GameController::Haptic_t::RUMBLE_DEATH, 32000, 32000, 2 * TICKS_PER_SECOND, 0);
		}
		else if ( stats[player]->OLDHP == stats[player]->HP )
		{
			rumble(player, GameController::Haptic_t::RUMBLE_NORMAL, 8000, 8000, 6, 0);
		}
		else if ( percentHPLost < .05 )
		{
			rumble(player, GameController::Haptic_t::RUMBLE_NORMAL, 16000, 16000, 6, 0);
		}
		else if ( percentHPLost < .25 )
		{
			rumble(player, GameController::Haptic_t::RUMBLE_NORMAL, 24000, 24000, 11, 0);
		}
		else
		{
			rumble(player, GameController::Haptic_t::RUMBLE_NORMAL, 32000, 32000, 11, 0);
		}
	}
}

void GameController::doRumble(Haptic_t::Rumble* r)
{
	if ( !sdl_haptic || !r )
	{
		return;
	}

	// init an effect.
	haptics.hapticEffect.type = SDL_HAPTIC_LEFTRIGHT;
	haptics.hapticEffect.leftright.type = SDL_HAPTIC_LEFTRIGHT;

	Entity* ent = uidToEntity(r->entityUid);
	real_t dampening = 1.0;
	if ( ent )
	{
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			if ( players[i]->isLocalPlayerAlive() && inputs.getController(i) == this )
			{
				int dist = static_cast<int>(entityDist(ent, players[i]->entity));
				dampening = 1.0 - std::min((dist / TOUCHRANGE) * .1, 1.0);
			}
		}
	}

	if ( r->pattern == Haptic_t::RUMBLE_BOULDER_BOUNCE )
	{
		haptics.hapticEffect.leftright.large_magnitude = r->largeMagnitude * dampening;
		haptics.hapticEffect.leftright.small_magnitude = r->smallMagnitude * dampening;
	}
	else if ( r->pattern == Haptic_t::RUMBLE_BOULDER )
	{
		real_t currentPlayheadPercent = r->startTime / static_cast<real_t>(r->length);
		r->customEffect = (currentPlayheadPercent);
		/*real_t currentPlayheadPercent = r->startTime / static_cast<real_t>(r->length);
		if ( currentPlayheadPercent < .33 )
		{
			r->customEffect = (currentPlayheadPercent) / .33;
		}
		else if ( currentPlayheadPercent < .66 )
		{
			r->customEffect = std::max(0.1, 1 - ((currentPlayheadPercent - .33) / .33));
		}
		else
		{
			r->customEffect = std::max(0.1, (currentPlayheadPercent - .66) / .33);
		}*/
		haptics.hapticEffect.leftright.large_magnitude = r->largeMagnitude * r->customEffect * dampening;
		haptics.hapticEffect.leftright.small_magnitude = r->smallMagnitude * r->customEffect * dampening;
	}
	else if ( r->pattern == Haptic_t::RUMBLE_BOULDER_ROLLING )
	{
		//real_t currentPlayheadPercent = r->startTime / static_cast<real_t>(r->length);
		//if ( currentPlayheadPercent > .5 )
		//{
		//	r->customEffect = std::max(0.1, (1 - ((currentPlayheadPercent - .5) / .5)));
		//}
		//else
		//{
		//	r->customEffect = 1.0;
		//}
		haptics.hapticEffect.leftright.large_magnitude = r->largeMagnitude * dampening;
		haptics.hapticEffect.leftright.small_magnitude = r->smallMagnitude * dampening;
	}
	else if ( r->pattern == Haptic_t::RUMBLE_TMP )
	{
		real_t currentPlayheadPercent = r->startTime / static_cast<real_t>(r->length);
		if ( currentPlayheadPercent < .165 )
		{
			r->customEffect = (currentPlayheadPercent) / .165;
		}
		else if ( currentPlayheadPercent < .33 )
		{
			r->customEffect = std::max(0.1, 1 - ((currentPlayheadPercent - .165) / .165));
		}
		else if ( currentPlayheadPercent < .495 )
		{
			r->customEffect = std::max(0.1, (currentPlayheadPercent - .33) / .165);
		}
		else if ( currentPlayheadPercent < .66 )
		{
			r->customEffect = std::max(0.1, 1 - ((currentPlayheadPercent - .495) / .165));
		}
		else
		{
			r->customEffect = std::max(0.1, (currentPlayheadPercent - .66) / .165);
		}
		haptics.hapticEffect.leftright.large_magnitude = r->largeMagnitude * r->customEffect;
		haptics.hapticEffect.leftright.small_magnitude = r->smallMagnitude * r->customEffect;
	}
	else if ( r->pattern == Haptic_t::RUMBLE_DEATH )
	{
		real_t currentPlayheadPercent = r->startTime / static_cast<real_t>(r->length);
		if ( currentPlayheadPercent > .5 )
		{
			r->customEffect = std::max(0.1, (1 - ((currentPlayheadPercent - .5) / .5)));
		}
		else
		{
			r->customEffect = 1.0;
		}
		haptics.hapticEffect.leftright.large_magnitude = r->largeMagnitude * r->customEffect;
		haptics.hapticEffect.leftright.small_magnitude = r->smallMagnitude * r->customEffect;
	}
	else
	{
		haptics.hapticEffect.leftright.large_magnitude = r->largeMagnitude;
		haptics.hapticEffect.leftright.small_magnitude = r->smallMagnitude;
	}
	haptics.hapticEffect.leftright.length = ((r->length - r->startTime) * 1000 / TICKS_PER_SECOND); // convert to ms
	if ( haptics.hapticEffectId == -1 )
	{
		haptics.hapticEffectId = SDL_HapticNewEffect(sdl_haptic, &haptics.hapticEffect);
		if ( haptics.hapticEffectId == -1 )
		{
			printlog("SDL_HapticNewEffect error: %s", SDL_GetError());
		}
	}
	if ( SDL_HapticUpdateEffect(sdl_haptic, haptics.hapticEffectId, &haptics.hapticEffect) < 0 )
	{
		printlog("SDL_HapticUpdateEffect error: %s", SDL_GetError());
		return;
	}
	if ( SDL_HapticRunEffect(sdl_haptic, haptics.hapticEffectId, 1) < 0 )
	{
		printlog("SDL_HapticUpdateEffect error: %s", SDL_GetError());
	}
}
void GameController::stopRumble()
{
	SDL_HapticStopEffect(sdl_haptic, haptics.hapticEffectId);
}

Player::Player(int in_playernum, bool in_local_host) : 
	inventoryUI(*this),
	statusBarUI(*this),
	hud(*this),
	magic(*this),
	characterSheet(*this),
	movement(*this),
	messageZone(*this),
	worldUI(*this),
	hotbar(*this),
	bookGUI(*this),
	paperDoll(*this)
{
	local_host = false;
	playernum = in_playernum;
	entity = nullptr;
	cam = &cameras[playernum];
}

Player::~Player()
{
	if (entity)
	{
		delete entity;
	}
}

void Player::init() // for use on new/restart game, UI related
{
	inventoryUI.resetInventory();
	selectedChestSlot[playernum] = -1;
	selectedShopSlot[playernum] = -1;
	shopinventorycategory[playernum] = -1;
	characterSheet.setDefaultSkillsSheetBox();
	characterSheet.setDefaultPartySheetBox();
	characterSheet.setDefaultCharacterSheetBox();
	paperDoll.clear();
}

void Player::cleanUpOnEntityRemoval()
{
	if ( isLocalPlayer() )
	{
		//selectedChestSlot[playernum] = -1;
		//selectedShopSlot[playernum] = -1;
		//shopinventorycategory[playernum] = -1;
		hud.reset();
		movement.reset();
		worldUI.reset();
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

void Player::PlayerMovement_t::reset()
{
	quickTurnRotation = 0.0;
	quickTurnStartTicks = 0;
	bDoingQuickTurn = false;
	monsterEmoteGimpTimer = 0;
	selectedEntityGimpTimer = 0;
	insectoidLevitating = false;
}

real_t Player::WorldUI_t::tooltipHeightOffsetZ = 0.0;
void Player::WorldUI_t::reset()
{
	for ( auto& tooltip : tooltipsInRange )
	{
		if ( bTooltipActiveForPlayer(*tooltip.first) )
		{
			setTooltipDisabled(*tooltip.first);
		}
	}
	tooltipsInRange.clear();
	bTooltipInView = false;
	uidForActiveTooltip = 0;
}

real_t Player::WorldUI_t::tooltipInRange(Entity& tooltip)
{
	if ( !players[player.playernum]->isLocalPlayerAlive() )
	{
		return 0.0;
	}

	real_t dist = entityDist(&tooltip, players[player.playernum]->entity);
	Entity* parent = uidToEntity(tooltip.parent);

	real_t maxDist = 24;
	real_t minDist = 4;

	bool followerSelectInteract = false;
	if ( FollowerMenu[player.playernum].followerMenuIsOpen() && FollowerMenu[player.playernum].selectMoveTo )
	{
		followerSelectInteract = (FollowerMenu[player.playernum].optionSelected == ALLY_CMD_ATTACK_SELECT);
		maxDist = 256;
	}

	if ( dist < maxDist && dist > minDist )
	{
		real_t tangent = atan2(tooltip.y - players[player.playernum]->entity->y, tooltip.x - players[player.playernum]->entity->x);
		while ( tangent >= 2 * PI )
		{
			tangent -= 2 * PI;
		}
		while ( tangent < 0 )
		{
			tangent += 2 * PI;
		}
		real_t playerYaw = players[player.playernum]->entity->yaw;
		while ( playerYaw >= 2 * PI )
		{
			playerYaw -= 2 * PI;
		}
		while ( playerYaw < 0 )
		{
			playerYaw += 2 * PI;
		}

		real_t interactAngle = (PI / 8);
		if ( parent )
		{
			if ( followerSelectInteract )
			{
				if ( !FollowerMenu[player.playernum].allowedInteractEntity(*parent, false) )
				{
					return 0.0;
				}

				Entity* ohitentity = hit.entity;
				real_t tangent2 = atan2(players[player.playernum]->entity->y - parent->y, players[player.playernum]->entity->x - parent->x);
				lineTraceTarget(parent, parent->x, parent->y, tangent2, maxDist, 0, false, players[player.playernum]->entity);
				if ( hit.entity != players[player.playernum]->entity )
				{
					// no line of sight through walls
					hit.entity = ohitentity;
					return 0.0;
				}
				hit.entity = ohitentity;
			}

			if ( stats[player.playernum] && stats[player.playernum]->defending )
			{
				if ( stats[player.playernum]->shield && stats[player.playernum]->shield->type == TOOL_TINKERING_KIT )
				{
					if ( !(parent->behavior == &actItem
						|| parent->behavior == &actTorch
						|| parent->behavior == &actCrystalShard) )
					{
						return 0.0; // we can't salvage the entity if not one of the above
					}
				}
			}

			if ( parent->behavior == &actGate && parent->flags[PASSABLE] )
			{
				return 0.0;
			}
			if ( parent->behavior == &actPlayer || parent->behavior == &actMonster )
			{
				interactAngle = PI / 16;
			}
			else if ( parent->behavior == &actBeartrap || parent->behavior == &actBomb )
			{
				interactAngle = PI / 16;
			}
			else if ( parent->behavior == &actBoulder )
			{
				dist += .5; // distance penalty
			}
			else if ( parent->behavior == &actFurniture )
			{
				if ( parent->furnitureType == FURNITURE_BED 
					|| parent->furnitureType == FURNITURE_BUNKBED
					|| parent->furnitureType == FURNITURE_TABLE )
				{
					// wide angle
				}
				else
				{
					interactAngle = PI / 16;
				}
				Entity* itemOnFurniture = uidToEntity(parent->parent);
				if ( itemOnFurniture )
				{
					dist += 2; // distance penalty
				}
			}

			if ( followerSelectInteract )
			{
				if ( parent->behavior == &actMonster && parent->checkEnemy(player.entity) )
				{
					// monsters have wider interact angle for aim assist
					interactAngle = (PI / 6);
					if ( dist > STRIKERANGE )
					{
						// for every x units, shrink the selection angle
						int units = static_cast<int>(dist / 16);
						interactAngle -= (units * PI / 64);
						interactAngle = std::max(interactAngle, PI / 32);
					}
				}
				else
				{
					if ( dist > STRIKERANGE )
					{
						// for every x units, shrink the selection angle
						int units = static_cast<int>(dist / 8);
						interactAngle -= (units * PI / 64);
						interactAngle = std::max(interactAngle, PI / 64);
					}
				}
			}
		}

		if ( (abs(tangent - playerYaw) < (interactAngle)) || (abs(tangent - playerYaw) > (2 * PI - interactAngle)) )
		{
			//messagePlayer(0, "%.2f", tangent - playerYaw);
			if ( !followerSelectInteract )
			{
				return dist;
			}

			if ( followerSelectInteract )
			{
				// perform head pitch check
				real_t startx = players[player.playernum]->entity->x;
				real_t starty = players[player.playernum]->entity->y;
				real_t startz = -4;
				real_t pitch = players[player.playernum]->entity->pitch;
				if ( pitch < 0 )
				{
					//pitch = 0; - unneeded - negative pitch looks in a cone upwards as well - good check
				}

				// draw line from the players height and direction until we hit the ground.
				real_t previousx = startx;
				real_t previousy = starty;
				int index = 0;
				for ( ; startz < 0.f; startz += abs(0.25 * tan(pitch)) )
				{
					startx += 0.5 * cos(players[player.playernum]->entity->yaw);
					starty += 0.5 * sin(players[player.playernum]->entity->yaw);
					index = (static_cast<int>(starty + 16 * sin(players[player.playernum]->entity->yaw)) >> 4) * MAPLAYERS + (static_cast<int>(startx + 16 * cos(players[player.playernum]->entity->yaw)) >> 4) * MAPLAYERS * map.height;
					if ( !map.tiles[OBSTACLELAYER + index] )
					{
						// store the last known good coordinate
						previousx = startx;
						previousy = starty;
					}
					if ( map.tiles[OBSTACLELAYER + index] )
					{
						break;
					}
				}
				real_t lookDist = sqrt(pow(previousx - players[player.playernum]->entity->x, 2) + pow(previousy - players[player.playernum]->entity->y, 2));
				if ( lookDist < dist )
				{
					if ( abs(dist - lookDist) > 24.0 )
					{
						return 0.0; // looking at a tile on the ground more than x units away from the tooltip
					}
				}
			}
			return dist;
		}
	}
	return 0.0;
}

void Player::WorldUI_t::setTooltipActive(Entity& tooltip)
{
	tooltip.setUID(UID_TOOLTIP_ACTIVE);
	tooltip.worldTooltipActive = 1;
	if ( uidToEntity(tooltip.parent) )
	{
		Entity* parent = uidToEntity(tooltip.parent);
		if ( !parent )
		{
			setTooltipDisabled(tooltip);
			return;
		}
		parent->highlightForUI = 1.0;

		if ( tooltip.worldTooltipRequiresButtonHeld == 1 )
		{
			interactText = language[3998]; // "(Hold) ";
		}
		else
		{
			interactText = "";
		}

		bool foundTinkeringKit = false;
		if ( stats[player.playernum] && stats[player.playernum]->defending )
		{
			if ( stats[player.playernum]->shield && stats[player.playernum]->shield->type == TOOL_TINKERING_KIT )
			{
				foundTinkeringKit = true;
			}
		}

		if ( FollowerMenu[player.playernum].followerMenuIsOpen()
			&& FollowerMenu[player.playernum].selectMoveTo
			&& FollowerMenu[player.playernum].optionSelected == ALLY_CMD_ATTACK_SELECT )
		{
			FollowerMenu[player.playernum].allowedInteractEntity(*parent, true);
		}
		else if ( parent->behavior == &actItem )
		{
			if ( foundTinkeringKit )
			{
				interactText = language[3999]; // "Salvage item";
			}
			else
			{
				interactText = language[4000]; // "Pick up item";
			}
		}
		else if ( parent->behavior == &actGoldBag )
		{
			interactText = language[4001]; // "Take gold";
		}
		else if ( parent->behavior == &actFountain )
		{
			interactText = language[4002]; // "Drink from fountain" 
		}
		else if ( parent->behavior == &actSink )
		{
			interactText = language[4003]; // "Drink from sink" 
		}
		else if ( parent->behavior == &actChestLid || parent->behavior == &actChest )
		{
			if ( parent->skill[1] == 1 )
			{
				interactText = language[4004]; // "Close chest" 
			}
			else if ( parent->skill[1] == 0 )
			{
				interactText = language[4005]; // "Open chest" 
			}
		}
		else if ( parent->behavior == &actTorch )
		{
			if ( foundTinkeringKit )
			{
				interactText = language[4006]; // "Salvage torch" 
			}
			else
			{
				interactText = language[4007]; // "Take torch" 
			}
		}
		else if ( parent->behavior == &actCrystalShard )
		{
			if ( foundTinkeringKit )
			{
				interactText = language[4008]; // "Salvage shard" 
			}
			else
			{
				interactText = language[4009]; // "Take shard" 
			}
		}
		else if ( parent->behavior == &actHeadstone )
		{
			interactText = language[4010]; // "Inspect gravestone" 
		}
		else if ( parent->behavior == &actMonster )
		{
			int monsterType = parent->getMonsterTypeFromSprite();
			std::string name = language[4011]; // "follower" 
			if ( parent->getStats() && strcmp(parent->getStats()->name, "") )
			{
				name = parent->getStats()->name;
			}
			else
			{
				if ( monsterType < KOBOLD ) //Original monster count
				{
					name = language[90 + monsterType];
				}
				else if ( monsterType >= KOBOLD ) //New monsters
				{
					name = language[2000 + monsterType - KOBOLD];
				}
			}

			if ( parent->monsterAllyGetPlayerLeader() 
				&& parent->monsterAllyGetPlayerLeader() == players[player.playernum]->entity )
			{
				if ( parent->monsterIsTinkeringCreation() )
				{
					interactText = language[4012] + name; // "Command "
				}
				else
				{
					interactText = language[4012] + name; // "Command "
				}
			}
			else if ( parent->getMonsterTypeFromSprite() == SHOPKEEPER )
			{
				interactText = language[4013] + name; // "Trade with "
			}
			else
			{
				interactText = language[4014] + name; // "Interact with "
			}
		}
		else if ( parent->behavior == &actDoor )
		{
			if ( parent->flags[PASSABLE] )
			{
				interactText = language[4015]; // "Close door" 
			}
			else
			{
				interactText = language[4016]; // "Open door" 
			}
		}
		else if ( parent->behavior == &actGate )
		{
			interactText = language[4017]; // "Inspect gate" 
		}
		else if ( parent->behavior == &actSwitch || parent->behavior == &actSwitchWithTimer )
		{
			if ( parent->skill[0] == 1 )
			{
				interactText = language[4018]; // "Deactivate switch" 
			}
			else
			{
				interactText = language[4019]; // "Activate switch" 
			}
		}
		else if ( parent->behavior == &actPowerCrystal )
		{
			interactText = language[4020]; // "Turn crystal" 
		}
		else if ( parent->behavior == &actBoulder )
		{
			interactText = language[4021]; // "Push boulder" 
		}
		else if ( parent->behavior == &actPedestalBase )
		{
			if ( parent->pedestalHasOrb > 0 )
			{
				interactText = language[4022]; // "Take orb" 
			}
			else
			{
				interactText = language[4023]; // "Inspect" 
			}
		}
		else if ( parent->behavior == &actCampfire )
		{
			interactText = language[4024]; // "Pull torch" 
		}
		else if ( parent->behavior == &actFurniture || parent->behavior == &actMCaxe )
		{
			interactText = language[4023]; // "Inspect" 
		}
		else if ( parent->behavior == &actFloorDecoration )
		{
			if ( parent->sprite == 991 ) // sign
			{
				interactText = language[4025]; // "Read sign" 
			}
			else
			{
				interactText = language[4023]; // "Inspect" 
			}
		}
		else if ( parent->behavior == &actBeartrap )
		{
			interactText = language[4026]; // "Disarm beartrap" 
		}
		else if ( parent->behavior == &actLadderUp )
		{
			interactText = language[4027]; // "Inspect trapdoor" 
		}
		else if ( parent->behavior == &actLadder )
		{
			if ( secretlevel && parent->skill[3] == 1 ) // secret ladder
			{
				interactText += language[4028]; // "Exit secret level" 
			}
			else if ( !secretlevel && parent->skill[3] == 1 ) // secret ladder
			{
				interactText += language[4029]; // "Enter secret level" 
			}
			else
			{
				interactText += language[4030]; // "Exit dungeon floor" 
			}
		}
		else if ( parent->behavior == &actPortal )
		{
			if ( parent->skill[3] == 0 ) // secret entrance portal
			{
				if ( secretlevel )
				{
					interactText += language[4028]; // "Exit secret level" 
				}
				else
				{
					interactText += language[4029]; // "Enter secret level" 
				}
			}
			else
			{
				if ( !strcmp(map.name, "Hell") )
				{
					interactText += language[4030]; // hell uses portals instead "Exit dungeon floor"
				}
				else if ( !strcmp(map.name, "Mages Guild") )
				{
					interactText += language[4031]; // mages guild exit to castle "Exit Hamlet"
				}
				else
				{
					interactText += language[4030]; // "Exit dungeon floor";
				}
			}
		}
		else if ( parent->behavior == &::actMidGamePortal )
		{
			interactText += language[4032]; // "Step through portal";
		}
		else if ( parent->behavior == &actCustomPortal )
		{
			if ( gameModeManager.getMode() == GameModeManager_t::GAME_MODE_TUTORIAL )
			{
				if ( !strcmp(map.name, "Tutorial Hub") )
				{
					interactText += language[4033]; // "Enter trial";
				}
				else
				{
					interactText += language[4034]; // "Exit trial";
				}
			}
			else
			{
				if ( parent->portalCustomSpriteAnimationFrames > 0 )
				{
					interactText += language[4035]; // "Enter portal";
				}
				else
				{
					interactText += language[4036]; // "Enter trapdoor";
				}
			}
		}
		else if ( parent->behavior == &::actExpansionEndGamePortal
			|| parent->behavior == &actWinningPortal )
		{
			interactText += language[4032]; // "Step through portal";
		}
		else if ( parent->behavior == &actTeleporter )
		{
			if ( parent->teleporterType == 2 ) // portal
			{
				interactText += language[4035]; // "Enter portal";
			}
			else if ( parent->teleporterType == 1 ) // down ladder
			{
				interactText += language[4037]; // "Descend ladder";
			}
			else if ( parent->teleporterType == 0 ) // up ladder
			{
				interactText += language[4038]; // "Climb ladder";
			}
		}
		else if ( parent->behavior == &actBomb && parent->skill[21] != 0 ) //skill[21] item type
		{
			char* itemName = items[parent->skill[21]].name_identified;
			interactText = language[4039]; // "Disarm ";
			interactText += itemName;
		}
		else
		{
			interactText = language[4040]; // "Interact";
		}
	}
	bTooltipInView = true;
	uidForActiveTooltip = tooltip.parent;
}
void Player::WorldUI_t::setTooltipDisabled(Entity& tooltip)
{
	tooltip.setUID(UID_TOOLTIP_DISABLED);
	tooltip.worldTooltipActive = 0;
	if ( tooltip.parent == uidForActiveTooltip )
	{
		uidForActiveTooltip = 0;
	}
}
bool Player::WorldUI_t::bTooltipActiveForPlayer(Entity& tooltip)
{
	return (tooltip.worldTooltipActive == 1 && tooltip.worldTooltipPlayer == player.playernum);
}

void Player::WorldUI_t::cycleToNextTooltip()
{
	if ( tooltipsInRange.empty() )
	{
		return;
	}
	int index = 0;
	int newIndex = 0;
	bool bFound = false;
	for ( auto& pair : tooltipsInRange )
	{
		if ( pair.first->getUID() == UID_TOOLTIP_ACTIVE )
		{
			if ( !bFound )
			{
				newIndex = index;
			}
			bFound = true;
		}
		setTooltipDisabled(*pair.first);
		++index;
	}

	if ( !bFound )
	{
		setTooltipActive(*tooltipsInRange.front().first);
	}
	else if ( newIndex >= tooltipsInRange.size() - 1 )
	{
		setTooltipActive(*tooltipsInRange.front().first);
	}
	else
	{
		setTooltipActive(*tooltipsInRange.at(newIndex + 1).first);
	}
}

void Player::WorldUI_t::cycleToPreviousTooltip()
{
	if ( tooltipsInRange.empty() )
	{
		return;
	}
	int index = 0;
	int newIndex = 0;
	bool bFound = false;
	for ( auto& pair : tooltipsInRange )
	{
		if ( pair.first->getUID() == UID_TOOLTIP_ACTIVE )
		{
			if ( !bFound )
			{
				newIndex = index;
			}
			bFound = true;
		}
		setTooltipDisabled(*pair.first);
		++index;
	}

	if ( !bFound )
	{
		setTooltipActive(*tooltipsInRange.front().first);
	}
	else if ( newIndex == 0 )
	{
		setTooltipActive(*tooltipsInRange.at(tooltipsInRange.size() - 1).first); // set as last
	}
	else
	{
		setTooltipActive(*tooltipsInRange.at(newIndex - 1).first);
	}
}

bool entityBlocksTooltipInteraction(const int player, Entity& entity)
{
	if ( entity.behavior == &actGate )
	{
		return false;
	}
	else if ( entity.behavior == &actFurniture )
	{
		return false;
	}
	else if ( entity.behavior == &actStalagCeiling )
	{
		return false;
	}
	else if ( entity.behavior == &actStalagFloor )
	{
		return false;
	}
	else if ( entity.behavior == &actStalagColumn )
	{
		return false;
	}
	else if ( entity.behavior == &actPedestalBase )
	{
		return false;
	}
	else if ( entity.behavior == &actPistonBase || entity.behavior == &actPistonCam )
	{
		return false;
	}
	else if ( entity.behavior == &actColumn )
	{
		return false;
	}
	else if ( entity.behavior == &actDoorFrame )
	{
		return false;
	}
	else if ( entity.behavior == &actDoor || entity.behavior == &actFountain || entity.behavior == &actSink
		|| entity.behavior == &actHeadstone || entity.behavior == &actChest || entity.behavior == &actChestLid
		|| entity.behavior == &actBoulder || (entity.behavior == &actMonster && !entity.checkEnemy(players[player]->entity) )
		|| entity.behavior == &actPlayer || entity.behavior == &actPedestalOrb || entity.behavior == &actPowerCrystalBase
		|| entity.behavior == &actPowerCrystal )
	{
		return false;
	}

	return true;
}

void Player::WorldUI_t::handleTooltips()
{
	for ( int player = 0; player < MAXPLAYERS && !gamePaused; ++player )
	{
		if ( !players[player]->isLocalPlayerAlive() )
		{
			players[player]->worldUI.reset();
			continue;
		}

		if ( inputs.bControllerRawInputPressed(player, 301 + SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_LEFTSTICK) )
		{
			if ( players[player]->worldUI.bEnabled )
			{
				players[player]->worldUI.disable();
			}
			else
			{
				players[player]->worldUI.enable();
			}
			inputs.controllerClearRawInput(player, 301 + SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_LEFTSTICK);
		}

		if ( !players[player]->worldUI.bEnabled )
		{
			continue;
		}

		bool foundTinkeringKit = false;
		bool radialMenuOpen = FollowerMenu[player].followerMenuIsOpen();
		bool followerSelectInteract = false;
		if ( radialMenuOpen )
		{
			// follower menu can be "open" but selectMoveTo == true means the GUI is closed and selecting move or interact.
			if ( FollowerMenu[player].selectMoveTo == false )
			{
				continue;
			}
			followerSelectInteract = (FollowerMenu[player].optionSelected == ALLY_CMD_ATTACK_SELECT);
		}

		bool bDoingActionHideTooltips = false;
		if ( FollowerMenu[player].selectMoveTo && FollowerMenu[player].optionSelected == ALLY_CMD_MOVETO_SELECT )
		{
			bDoingActionHideTooltips = true;
		}
		else if ( players[player]->hud.weapon && players[player]->hud.weapon->skill[0] != 0 )
		{
			// hudweapon chop
			bDoingActionHideTooltips = true;
		}
		else if ( players[player]->hud.bowFire || players[player]->hud.bowIsBeingDrawn )
		{
			bDoingActionHideTooltips = true;
		}
		else if ( cast_animation[player].active || cast_animation[player].active_spellbook )
		{
			// spells
			bDoingActionHideTooltips = true;
		}
		else if ( stats[player] && stats[player]->defending )
		{
			if ( stats[player]->shield && stats[player]->shield->type == TOOL_TINKERING_KIT )
			{
				// don't ignore
				foundTinkeringKit = true;
			}
			else
			{
				bDoingActionHideTooltips = true;
			}
		}
		
		if ( !bDoingActionHideTooltips )
		{
			Entity* ohitentity = hit.entity;
			lineTrace(players[player]->entity, players[player]->entity->x, players[player]->entity->y,
				players[player]->entity->yaw, STRIKERANGE, 0, true);
			if ( hit.entity && entityBlocksTooltipInteraction(player, *hit.entity) )
			{
				if ( hit.entity->behavior == &actMonster && followerSelectInteract )
				{
					// don't let hostile monsters get in the way of selection
				}
				else
				{
					bDoingActionHideTooltips = true;
				}
			}
			hit.entity = ohitentity;
		}

		if ( players[player]->worldUI.tooltipsInRange.size() > 1 )
		{
			if ( inputs.bControllerRawInputPressed(player, 301 + SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_LEFT) )
			{
				players[player]->worldUI.tooltipView = TOOLTIP_VIEW_LOCKED;
				inputs.controllerClearRawInput(player, 301 + SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_LEFT);
				players[player]->worldUI.cycleToPreviousTooltip();
			}
			if ( inputs.bControllerRawInputPressed(player, 301 + SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_RIGHT) )
			{
				players[player]->worldUI.tooltipView = TOOLTIP_VIEW_LOCKED;
				inputs.controllerClearRawInput(player, 301 + SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
				players[player]->worldUI.cycleToNextTooltip();
			}
		}

		if ( players[player]->worldUI.tooltipView == TOOLTIP_VIEW_FREE
			|| players[player]->worldUI.tooltipView == TOOLTIP_VIEW_RESCAN)
		{
			players[player]->worldUI.reset();
			for ( node_t* node = map.worldUI->first; node; node = node->next )
			{
				Entity* tooltip = (Entity*)node->element;
				if ( !tooltip || tooltip->behavior != &actSpriteWorldTooltip )
				{
					continue;
				}
				if ( tooltip->worldTooltipPlayer != player )
				{
					continue;
				}
				players[player]->worldUI.setTooltipDisabled(*tooltip);
			}

			if ( bDoingActionHideTooltips )
			{
				players[player]->worldUI.gimpDisplayTimer = 10;
				continue;
			}

			players[player]->worldUI.gimpDisplayTimer = std::max(0, players[player]->worldUI.gimpDisplayTimer - 1);
			if ( players[player]->worldUI.gimpDisplayTimer > 0 )
			{
				continue;
			}

			Entity* closestTooltip = nullptr;
			Entity* parent = nullptr;
			real_t dist = 10000.0;
			for ( node_t* node = map.worldUI->first; node; node = node->next )
			{
				Entity* tooltip = (Entity*)node->element;
				if ( !tooltip || tooltip->behavior != &actSpriteWorldTooltip )
				{
					continue;
				}
				if ( tooltip->worldTooltipPlayer != player )
				{
					continue;
				}
				parent = uidToEntity(tooltip->parent);
				if ( parent && parent->flags[INVISIBLE] )
				{
					continue;
				}
				if ( parent && parent->flags[PASSABLE] && parent->behavior == &actBoulder )
				{
					continue;
				}
				real_t newDist = players[player]->worldUI.tooltipInRange(*tooltip);
				if ( newDist > 0.01 )
				{
					players[player]->worldUI.tooltipsInRange.push_back(std::make_pair(tooltip, newDist));
					if ( followerSelectInteract && parent && closestTooltip && parent->behavior != &actMonster )
					{
						// follower interaction - monsters have higher priority than interactibles.
						Entity* closestParent = uidToEntity(closestTooltip->parent);
						if ( closestParent && closestParent->behavior == &actMonster )
						{
							continue;
						}
					}
					if ( newDist < dist )
					{
						dist = newDist;
						closestTooltip = tooltip;
					}
				}
			}
			if ( closestTooltip )
			{
				players[player]->worldUI.playerLastYaw = players[player]->entity->yaw;
				while ( players[player]->worldUI.playerLastYaw >= 4 * PI )
				{
					players[player]->worldUI.playerLastYaw -= 2 * PI;
				}
				while ( players[player]->worldUI.playerLastYaw < 2 * PI )
				{
					players[player]->worldUI.playerLastYaw += 2 * PI;
				}
				players[player]->worldUI.setTooltipActive(*closestTooltip);
			}
			std::sort(players[player]->worldUI.tooltipsInRange.begin(), players[player]->worldUI.tooltipsInRange.end(),
				[](const std::pair<Entity*, real_t>& lhs, const std::pair<Entity*, real_t>& rhs)
			{
				return lhs.second < rhs.second;
			}
			);
			if ( players[player]->worldUI.tooltipView == TOOLTIP_VIEW_RESCAN )
			{
				players[player]->worldUI.tooltipView = TOOLTIP_VIEW_LOCKED;
			}
		}
		else if ( players[player]->worldUI.tooltipView == TOOLTIP_VIEW_LOCKED )
		{
			if ( players[player]->worldUI.tooltipsInRange.empty() )
			{
				players[player]->worldUI.tooltipView = TOOLTIP_VIEW_RESCAN;
				return;
			}
			real_t currentYaw = players[player]->entity->yaw;
			while ( currentYaw >= 4 * PI )
			{
				currentYaw -= 2 * PI;
			}
			while ( currentYaw < 2 * PI )
			{
				currentYaw += 2 * PI;
			}
			real_t yawDiff = players[player]->worldUI.playerLastYaw - currentYaw;
			if ( inputs.hasController(player) )
			{
				real_t floatx = inputs.getController(player)->getLeftXPercent();
				real_t floaty = inputs.getController(player)->getLeftYPercent();
				real_t magnitude = sqrt(pow(floaty, 2) + pow(floatx, 2));
				if ( magnitude > 0.0 )
				{
					players[player]->worldUI.tooltipView = TOOLTIP_VIEW_FREE;
					return;
				}
			}
			if ( abs(yawDiff) > PI / 16 )
			{
				players[player]->worldUI.tooltipView = TOOLTIP_VIEW_RESCAN;
				return;
			}
			if ( FollowerMenu[player].selectMoveTo && FollowerMenu[player].optionSelected == ALLY_CMD_MOVETO_SELECT )
			{
				// rescan constantly
				players[player]->worldUI.tooltipView = TOOLTIP_VIEW_RESCAN;
				return;
			}

			std::array<char*, 3> salvageStrings = { language[3999], language[4006], language[4008] };
			bool foundSalvageString = false;
			for ( auto s : salvageStrings )
			{
				if ( players[player]->worldUI.interactText.find(s) != std::string::npos )
				{
					foundSalvageString = true;
					if ( !foundTinkeringKit )
					{
						// rescan, out of date string.
						players[player]->worldUI.tooltipView = TOOLTIP_VIEW_RESCAN;
						return;
					}
				}
			}
			for ( auto& tooltip : players[player]->worldUI.tooltipsInRange )
			{
				if ( players[player]->worldUI.tooltipInRange(*tooltip.first) < 0.01 )
				{
					players[player]->worldUI.tooltipView = TOOLTIP_VIEW_RESCAN;
					return;
				}
			}
		}
	}
}

void Player::Hotbar_t::initFaceButtonHotbar()
{
	faceButtonTopYPosition = yres;

	if ( faceMenuAlternateLayout )
	{
		for ( Uint32 num = 0; num < NUM_HOTBAR_SLOTS; ++num )
		{
			faceButtonPositions[num].w = getSlotSize();
			faceButtonPositions[num].h = getSlotSize();
			faceButtonPositions[num].x = hotbarBox.x + getSlotSize() / 6;
			faceButtonPositions[num].y = hotbarBox.y - getSlotSize() / 2;

			if ( getFaceMenuGroupForSlot(num) == FaceMenuGroup::GROUP_LEFT )
			{
				faceButtonPositions[num].x += getSlotSize() / 1;
				faceButtonPositions[num].y += getSlotSize() / 8;
			}
			else if ( getFaceMenuGroupForSlot(num) == FaceMenuGroup::GROUP_MIDDLE )
			{
				faceButtonPositions[num].y -= getSlotSize();
			}
			else if ( getFaceMenuGroupForSlot(num) == FaceMenuGroup::GROUP_RIGHT )
			{
				faceButtonPositions[num].x -= getSlotSize() / 1;
				faceButtonPositions[num].y += getSlotSize() / 8;
			}

			switch ( num )
			{
				case 0:
					faceButtonPositions[num].x += 0 * getSlotSize();
					if ( faceMenuButtonHeld != FaceMenuGroup::GROUP_LEFT )
					{
						faceButtonPositions[num].y += getSlotSize() / 8;
					}
					break;
				case 1:
					faceButtonPositions[num].x += 1 * getSlotSize();
					if ( faceMenuButtonHeld != FaceMenuGroup::GROUP_LEFT )
					{
						faceButtonPositions[num].y += getSlotSize() / 8;
					}
					break;
				case 2:
					faceButtonPositions[num].x += 2 * getSlotSize();
					break;
				case 3:
					faceButtonPositions[num].x += 3 * getSlotSize() + getSlotSize() / 3;
					faceButtonPositions[num].y -= getSlotSize() / 4;
					if ( faceMenuButtonHeld != FaceMenuGroup::GROUP_MIDDLE )
					{
						faceButtonPositions[num].y += getSlotSize() / 8;
					}
					break;
				case 4:
					faceButtonPositions[num].x += 4 * getSlotSize() + getSlotSize() / 3;
					faceButtonPositions[num].y -= getSlotSize() / 4;
					break;
				case 5:
					faceButtonPositions[num].x += 5 * getSlotSize() + getSlotSize() / 3;
					faceButtonPositions[num].y -= getSlotSize() / 4;
					if ( faceMenuButtonHeld != FaceMenuGroup::GROUP_MIDDLE )
					{
						faceButtonPositions[num].y += getSlotSize() / 8;
					}
					break;
				case 6:
					faceButtonPositions[num].x += 6 * getSlotSize() + 2 * getSlotSize() / 3;
					break;
				case 7:
					faceButtonPositions[num].x += 7 * getSlotSize() + 2 * getSlotSize() / 3;
					if ( faceMenuButtonHeld != FaceMenuGroup::GROUP_RIGHT )
					{
						faceButtonPositions[num].y += getSlotSize() / 8;
					}
					break;
				case 8:
					faceButtonPositions[num].x += 8 * getSlotSize() + 2 * getSlotSize() / 3;
					if ( faceMenuButtonHeld != FaceMenuGroup::GROUP_RIGHT )
					{
						faceButtonPositions[num].y += getSlotSize() / 8;
					}
					break;
				case 9:
					faceButtonPositions[num].x += 14 * getSlotSize();
					break;
				default:
					break;
			}
		}

		faceButtonTopYPosition = std::min(faceButtonPositions[4].y, faceButtonTopYPosition);
		return;
	}

	for ( Uint32 num = 0; num < NUM_HOTBAR_SLOTS; ++num )
	{
		faceButtonPositions[num].w = getSlotSize();
		faceButtonPositions[num].h = getSlotSize();
		faceButtonPositions[num].x = hotbarBox.x + getSlotSize() / 6;
		faceButtonPositions[num].y = hotbarBox.y - getSlotSize() / 2;

		/*if ( faceMenuButtonHeld != FaceMenuGroup::GROUP_NONE && num == current_hotbar)
		{
			faceButtonPositions[num].y -= getSlotSize() / 4;
		}*/
		if ( faceMenuButtonHeld == FaceMenuGroup::GROUP_LEFT && getFaceMenuGroupForSlot(num) == FaceMenuGroup::GROUP_LEFT )
		{
			faceButtonPositions[num].y -= getSlotSize() / 4;
		}
		else if ( faceMenuButtonHeld == FaceMenuGroup::GROUP_MIDDLE && getFaceMenuGroupForSlot(num) == FaceMenuGroup::GROUP_MIDDLE )
		{
			faceButtonPositions[num].y -= getSlotSize() / 4;
		}
		else if ( faceMenuButtonHeld == FaceMenuGroup::GROUP_RIGHT && getFaceMenuGroupForSlot(num) == FaceMenuGroup::GROUP_RIGHT )
		{
			faceButtonPositions[num].y -= getSlotSize() / 4;
		}

		switch ( num )
		{
			case 0:
				faceButtonPositions[num].x += 0 * getSlotSize();
				if ( faceMenuButtonHeld != getFaceMenuGroupForSlot(num) )
				{
					faceButtonPositions[num].y += getSlotSize() / 8;
				}
				break;
			case 1:
				faceButtonPositions[num].x += 1 * getSlotSize();
				break;
			case 2:
				faceButtonPositions[num].x += 2 * getSlotSize();
				if ( faceMenuButtonHeld != getFaceMenuGroupForSlot(num) )
				{
					faceButtonPositions[num].y += getSlotSize() / 8;
				}
				break;
			case 3:
				faceButtonPositions[num].x += 3 * getSlotSize() + getSlotSize() / 3;
				faceButtonPositions[num].y -= getSlotSize() / 4;
				if ( faceMenuButtonHeld != getFaceMenuGroupForSlot(num) )
				{
					faceButtonPositions[num].y += getSlotSize() / 8;
				}
				break;
			case 4:
				faceButtonPositions[num].x += 4 * getSlotSize() + getSlotSize() / 3;
				faceButtonPositions[num].y -= getSlotSize() / 4;
				break;
			case 5:
				faceButtonPositions[num].x += 5 * getSlotSize() + getSlotSize() / 3;
				faceButtonPositions[num].y -= getSlotSize() / 4;
				if ( faceMenuButtonHeld != getFaceMenuGroupForSlot(num) )
				{
					faceButtonPositions[num].y += getSlotSize() / 8;
				}
				break;
			case 6:
				faceButtonPositions[num].x += 6 * getSlotSize() + 2 * getSlotSize() / 3;
				if ( faceMenuButtonHeld != getFaceMenuGroupForSlot(num) )
				{
					faceButtonPositions[num].y += getSlotSize() / 8;
				}
				break;
			case 7:
				faceButtonPositions[num].x += 7 * getSlotSize() + 2 * getSlotSize() / 3;
				break;
			case 8:
				faceButtonPositions[num].x += 8 * getSlotSize() + 2 * getSlotSize() / 3;
				if ( faceMenuButtonHeld != getFaceMenuGroupForSlot(num) )
				{
					faceButtonPositions[num].y += getSlotSize() / 8;
				}
				break;
			case 9:
				faceButtonPositions[num].x += 12 * getSlotSize();
				break;
			default:
				break;
		}
	}
	faceButtonTopYPosition = std::min(faceButtonPositions[4].y, faceButtonTopYPosition);
}

Player::Hotbar_t::FaceMenuGroup Player::Hotbar_t::getFaceMenuGroupForSlot(int hotbarSlot)
{
	if ( hotbarSlot < 3 )
	{
		return FaceMenuGroup::GROUP_LEFT;
	}
	else if ( hotbarSlot < 6 )
	{
		return FaceMenuGroup::GROUP_MIDDLE;
	}
	else if ( hotbarSlot < 9 )
	{
		return FaceMenuGroup::GROUP_RIGHT;
	}
	return FaceMenuGroup::GROUP_NONE;
}

void Player::Hotbar_t::drawFaceButtonGlyph(Uint32 slot, SDL_Rect& slotPos)
{
	int height = 2.25 * uiscale_hotbar;
	int width = 2.25 * uiscale_hotbar;
	int x = slotPos.x + slotPos.w / 2;
	int y = slotPos.y;
	SDL_Rect glyphsrc {0, 0, 0, 0};
	bool draw = true;

	switch ( slot )
	{
		case 0:
			if ( faceMenuButtonHeld == GROUP_LEFT )
			{
				glyphsrc = inputs.getGlyphRectForInput(player.playernum, true, 0,
					SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
			}
			else
			{
				draw = false;
			}
			break;
		case 1:
			glyphsrc = inputs.getGlyphRectForInput(player.playernum, faceMenuButtonHeld == GROUP_LEFT, 0,
				SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_X);
			break;
		case 2:
			if ( faceMenuButtonHeld == GROUP_LEFT )
			{
				glyphsrc = inputs.getGlyphRectForInput(player.playernum, true, 0,
					SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
			}
			else
			{
				draw = false;
			}
			break;
		case 3:
			// always grab this to determine the highest button height.
			glyphsrc = inputs.getGlyphRectForInput(player.playernum, true, 0,
				SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
			if ( faceMenuButtonHeld == GROUP_MIDDLE )
			{
				draw = true;
			}
			else
			{
				draw = false;
			}
			break;
		case 4:
			glyphsrc = inputs.getGlyphRectForInput(player.playernum, faceMenuButtonHeld == GROUP_MIDDLE, 0,
				SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_Y);
			break;
		case 5:
			// always grab this to determine the highest button height.
			glyphsrc = inputs.getGlyphRectForInput(player.playernum, true, 0,
				SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
			if ( faceMenuButtonHeld == GROUP_MIDDLE )
			{
				draw = true;
			}
			else
			{
				draw = false;
			}
			break;
		case 6:
			if ( faceMenuButtonHeld == GROUP_RIGHT )
			{
				glyphsrc = inputs.getGlyphRectForInput(player.playernum, true, 0,
					SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
			}
			else
			{
				draw = false;
			}
			break;
		case 7:
			glyphsrc = inputs.getGlyphRectForInput(player.playernum, faceMenuButtonHeld == GROUP_RIGHT, 0,
				SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_B);
			break;
		case 8:
			if ( faceMenuButtonHeld == GROUP_RIGHT )
			{
				glyphsrc = inputs.getGlyphRectForInput(player.playernum, true, 0,
					SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
			}
			else
			{
				draw = false;
			}
			break;
		case 9:
			return;
		default:
			return;
	}

	// temporary
	if ( faceMenuAlternateLayout )
	{
		if ( slot == 2 )
		{
			x -= slotPos.w;
		}
		else if ( slot == 6 )
		{
			x += slotPos.w;
		}
	}

	height *= glyphsrc.h;
	width *= glyphsrc.w;
	x -= width / 2;
	y -= height;

	if ( (faceMenuAlternateLayout && (slot == 3 || slot == 5) ) // highest slots
		|| (!faceMenuAlternateLayout && slot == 4) )
	{
		int offsetY = 0;
		int posY = y;
		// check if button not pressed and raised.
		if ( !faceMenuAlternateLayout && !(faceMenuButtonHeld == FaceMenuGroup::GROUP_MIDDLE) )
		{
			offsetY = getSlotSize() / 4;
		}
		else if ( faceMenuAlternateLayout )
		{
			posY = faceButtonPositions[4].y;
			posY -= height;
		}

		faceButtonTopYPosition = std::min(posY - offsetY, faceButtonTopYPosition);
	}

	if ( draw )
	{
		SDL_Rect glyphpos{ x, y, width, height };
		drawImageScaled(controllerglyphs1_bmp, &glyphsrc, &glyphpos);
	}
}

const int Player::HUD_t::getActionIconForPlayer(ActionPrompts prompt) const
{
	if ( prompt == ACTION_PROMPT_MAGIC ) { return PRO_SPELLCASTING; }

	bool shapeshifted = false;
	Monster playerRace = HUMAN;
	
	if ( players[player.playernum]->entity )
	{
		playerRace = players[player.playernum]->entity->getMonsterFromPlayerRace(stats[player.playernum]->playerRace);
		if ( players[player.playernum]->entity->effectShapeshift != NOTHING )
		{
			playerRace = static_cast<Monster>(players[player.playernum]->entity->effectShapeshift);
			shapeshifted = true;
		}
	}

	if ( prompt == ACTION_PROMPT_OFFHAND )
	{
		int skill = PRO_SHIELD;
		if ( stats[player.playernum] )
		{
			if ( stats[player.playernum]->shield )
			{
				bool hasSpellBook = itemCategory(stats[player.playernum]->shield) == SPELLBOOK;
				bool allowCasting = true;
				bool allowDefending = true;
				if ( shapeshifted && playerRace == CREATURE_IMP )
				{
					// imp allowed to cast via spellbook.
					allowCasting = false;
				}
				if ( hasSpellBook && allowCasting )
				{
					return PRO_MAGIC;
				}

				if ( shapeshifted || itemTypeIsQuiver(stats[player.playernum]->shield->type) )
				{
					allowDefending = false;
				}

				if ( allowDefending )
				{ 
					return PRO_SHIELD; 
				}
				return PRO_STEALTH;
			}
			else
			{
				skill = PRO_STEALTH;
			}
		}
		return skill;
	}
	else // prompt == ACTION_PROMPT_MAINHAND
	{
		int skill = PRO_UNARMED;
		if ( stats[player.playernum] )
		{
			if ( stats[player.playernum]->shield && stats[player.playernum]->shield->type == TOOL_TINKERING_KIT )
			{
				if ( !shapeshifted && stats[player.playernum]->defending )
				{
					return PRO_LOCKPICKING;
				}
			}
			if ( stats[player.playernum]->weapon )
			{
				if ( itemCategory(stats[player.playernum]->weapon) == MAGICSTAFF )
				{
					if ( !shapeshifted || (shapeshifted && playerRace == CREATURE_IMP) )
					{
						skill = PRO_SPELLCASTING;
					}
				}
				else if ( !shapeshifted )
				{
					if ( itemCategory(stats[player.playernum]->weapon) == POTION )
					{
						skill = PRO_ALCHEMY;
					}
					else if ( itemCategory(stats[player.playernum]->weapon) == TOOL )
					{
						skill = PRO_LOCKPICKING;
					}
					else
					{
						skill = getWeaponSkill(stats[player.playernum]->weapon);
						if ( skill == -1 )
						{
							skill =  PRO_UNARMED;
						}
					}
				}
			}
		}
		return skill;
	}
}

void Player::HUD_t::drawActionIcon(SDL_Rect& pos, int skill) const
{
	SDL_Rect skillIconSrc = getRectForSkillIcon(skill);
	SDL_Rect skillIconPos{ pos.x, pos.y, pos.w, pos.h };

	drawImageScaled(skillIcons_bmp, &skillIconSrc, &skillIconPos);
}

void Player::HUD_t::drawActionGlyph(SDL_Rect& pos, ActionPrompts prompt) const
{
	if ( !bShowActionPrompts )
	{
		return;
	}

	real_t scale = 2.25;
	int height = scale;
	int width = scale;
	int x = pos.x + pos.w / 2;
	int y = pos.y + pos.h;
	SDL_Rect glyphsrc;

	switch ( prompt )
	{
		case ACTION_PROMPT_MAINHAND:
			// replace these dirty hax
			glyphsrc = inputs.getGlyphRectForInput(player.playernum, true, 0,
				INJOY_GAME_ATTACK);
			break;
		case ACTION_PROMPT_OFFHAND:
			// replace these dirty hax
			glyphsrc = inputs.getGlyphRectForInput(player.playernum, true, 0,
				INJOY_GAME_DEFEND);
			break;
		case ACTION_PROMPT_MAGIC:
			// replace these dirty hax
			glyphsrc = inputs.getGlyphRectForInput(player.playernum, true, 0,
				std::min(joyimpulses[INJOY_GAME_CAST_SPELL] - 301U, SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_MAX - 1U));
			break;
		default:
			return;
	}

	height *= glyphsrc.h;
	width *= glyphsrc.w;
	x -= width / 2;
	y -= 8;
	SDL_Rect glyphpos{ x, y, width, height };
	drawImageScaled(controllerglyphs1_bmp, &glyphsrc, &glyphpos);
}

const int Player::Inventory_t::getPlayerItemInventoryX() const
{
	int x = DEFAULT_INVENTORY_SIZEX;
	if ( !stats[player.playernum] || !player.isLocalPlayer() )
	{
		return x;
	}
	return x;
}

const int Player::Inventory_t::getPlayerItemInventoryY() const
{
	int y = DEFAULT_INVENTORY_SIZEY;
	if ( !stats[player.playernum] || !player.isLocalPlayer() )
	{
		return y;
	}
	if ( stats[player.playernum]->cloak
		&& stats[player.playernum]->cloak->type == CLOAK_BACKPACK
		&& (shouldInvertEquipmentBeatitude(stats[player.playernum]) ? abs(stats[player.playernum]->cloak->beatitude) >= 0 : stats[player.playernum]->cloak->beatitude >= 0) )
	{
		y = DEFAULT_INVENTORY_SIZEY + 1;
	}
	return y;
}

const int Player::Inventory_t::getStartX() const 
{
	if ( bNewInventoryLayout )
	{
		return (player.characterSheet.characterSheetBox.x) + 8;
	}
	else
	{
		return (player.camera_midx() - (sizex) * (getSlotSize()) / 2 - inventory_mode_item_img->w / 2);
	}
}
const int Player::Inventory_t::getStartY() const
{
	if ( bNewInventoryLayout )
	{
		return player.characterSheet.characterSheetBox.y + player.characterSheet.characterSheetBox.h + 2;
	}
	else
	{
		return player.camera_y1() + starty;
	}
}

const int Player::Inventory_t::getSelectedSlotPositionX(Item* snapToItem) const
{
	int x = getSelectedSlotX();
	int y = getSelectedSlotY();
	if ( snapToItem )
	{
		x = snapToItem->x;
		y = snapToItem->y;
	}
	if ( player.paperDoll.enabled && (selectedSlotInPaperDoll() || snapToItem) )
	{
		auto slot = Player::PaperDoll_t::PaperDollSlotType::SLOT_MAX;
		if ( snapToItem )
		{
			slot = player.paperDoll.getSlotForItem(*snapToItem);
		}
		else
		{
			slot = player.paperDoll.paperDollSlotFromCoordinates(x, y);
		}
		if ( slot >= Player::PaperDoll_t::PaperDollSlotType::SLOT_MAX || slot < 0 )
		{
			return player.paperDoll.dollSlots[Player::PaperDoll_t::PaperDollSlotType::SLOT_GLASSES].pos.x
				+ player.paperDoll.dollSlots[Player::PaperDoll_t::PaperDollSlotType::SLOT_GLASSES].pos.w / 2;
		}
		return player.paperDoll.dollSlots[slot].pos.x + player.paperDoll.dollSlots[slot].pos.w / 2;
	}
	else
	{
		return getStartX()
			+ (x * getSlotSize())
			+ (getSlotSize() / 2);
	}
}

const int Player::Inventory_t::getSelectedSlotPositionY(Item* snapToItem) const
{
	int x = getSelectedSlotX();
	int y = getSelectedSlotY();
	if ( snapToItem )
	{
		x = snapToItem->x;
		y = snapToItem->y;
	}
	if ( player.paperDoll.enabled && (selectedSlotInPaperDoll() || snapToItem) )
	{
		auto slot = Player::PaperDoll_t::PaperDollSlotType::SLOT_MAX;
		if ( snapToItem )
		{
			slot = player.paperDoll.getSlotForItem(*snapToItem);
		}
		else
		{
			slot = player.paperDoll.paperDollSlotFromCoordinates(x, y);
		}
		if ( slot >= Player::PaperDoll_t::PaperDollSlotType::SLOT_MAX || slot < 0 )
		{
			return player.paperDoll.dollSlots[Player::PaperDoll_t::PaperDollSlotType::SLOT_GLASSES].pos.y
				+ player.paperDoll.dollSlots[Player::PaperDoll_t::PaperDollSlotType::SLOT_GLASSES].pos.h / 2;
		}
		return player.paperDoll.dollSlots[slot].pos.y + player.paperDoll.dollSlots[slot].pos.h / 2;
	}
	else
	{
		return getStartY()
			+ (y * getSlotSize())
			+ (getSlotSize() / 2);
	}
}

const bool Player::Inventory_t::bItemInventoryHasFreeSlot() const
{
	int numSlots = freeVisibleInventorySlots();
	int itemCount = 0;
	if ( !stats[player.playernum] || !player.isLocalPlayer() )
	{
		return false;
	}
	for ( node_t* node = stats[player.playernum]->inventory.first; node; node = node->next )
	{
		Item* item = (Item*)node->element;
		if ( !item )
		{
			continue;
		}
		if ( itemCategory(item) == SPELL_CAT
			|| (item->x < 0 || item->x >= getPlayerItemInventoryX())
			|| (item->y < 0 || item->y >= getPlayerItemInventoryY())
			)
		{
			continue; // ignore spells, or items not present in the grid.
		}
		if ( player.paperDoll.enabled
			&& player.paperDoll.isItemOnDoll(*item) )
		{
			continue;
		}
		++itemCount;
	}
	return itemCount < numSlots;
}

const int Player::PaperDoll_t::getSlotSize() const
{
	return 32;
}

Player::PaperDoll_t::PaperDollSlotType Player::PaperDoll_t::getSlotForItem(const Item& item) const
{
	for ( auto& slot : dollSlots )
	{
		if ( slot.item == item.uid )
		{
			return slot.slotType;
		}
	}
	return SLOT_MAX;
}

void Player::PaperDoll_t::drawSlots()
{
	updateSlots();
	if ( player.shootmode || !enabled || !player.isLocalPlayer() )
	{
		return;
	}
	auto& charSheetBox = player.characterSheet.characterSheetBox;

	int startx = charSheetBox.x + 2 + 8;
	int starty = charSheetBox.y + 2 + 8;

	SDL_Rect pos{ startx, starty, getSlotSize(), getSlotSize() };

	for ( auto& slot : dollSlots )
	{
		slot.bMouseInSlot = false;
		if ( slot.slotType != PaperDollSlotType::SLOT_MAX )
		{
			slot.pos = pos;

			// grey outline
			drawRect(&pos, SDL_MapRGB(mainsurface->format, 150, 150, 150), 255);
			if ( mouseInBounds(player.playernum, pos.x, pos.x + pos.w, pos.y, pos.y + pos.h) )
			{
				slot.bMouseInSlot = true;
				// yellow highlight

				selectPaperDollCoordinatesFromSlotType(slot.slotType);
				if ( !player.hotbar.hotbarHasFocus )
				{
					drawRect(&pos, SDL_MapRGB(mainsurface->format, 255, 255, 0), 127);
				}
			}

			SDL_Rect slotBackground = pos;
			slotBackground.x += 1;
			slotBackground.y += 1;
			slotBackground.w -= 2;
			slotBackground.h -= 2;
			// black background
			drawRect(&slotBackground, 0, 255);

			if ( slot.slotType == SLOT_OFFHAND )
			{
				pos.x = charSheetBox.x + charSheetBox.w - 2 - 8 - getSlotSize();
				pos.y = starty;
			}
			else
			{
				pos.y += 4 + getSlotSize();
			}
		}
	}
}

void Inputs::setMouse(const int player, MouseInputs input, Sint32 value)
{
	// todo: add condition like getMouse()? && (!getVirtualMouse(player)->lastMovementFromController 
	// || (players[player]->shootmode && !gamePaused && !intro))
	if ( bPlayerUsingKeyboardControl(player) && (!getVirtualMouse(player)->lastMovementFromController
		|| (players[player]->shootmode && !gamePaused && !intro)) )
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

const bool Inputs::bControllerRawInputPressed(int player, const unsigned button) const
{
	if ( button < 299 || button >= (301 + SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_MAX) )
	{
		return false;
	}
	const GameController* controller = getController(player);
	if ( !controller )
	{
		return false;
	}

	if ( button == 299 || button == 300 ) // triggers
	{
		return controller->binaryToggle(static_cast<SDL_GameControllerAxis>(button - 299 + SDL_CONTROLLER_AXIS_TRIGGERLEFT));
	}
	else
	{
		return controller->binaryToggle(static_cast<SDL_GameControllerButton>(button - 301));
	}
}

const bool Inputs::bControllerRawInputReleased(int player, const unsigned button) const
{
	if ( button < 299 || button >= (301 + SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_MAX) )
	{
		return false;
	}
	const GameController* controller = getController(player);
	if ( !controller )
	{
		return false;
	}

	if ( button == 299 || button == 300 ) // triggers
	{
		return false;
		//return controller->binaryReleaseToggle(static_cast<SDL_GameControllerAxis>(button - 299 + SDL_CONTROLLER_AXIS_TRIGGERLEFT));
	}
	else
	{
		return controller->binaryReleaseToggle(static_cast<SDL_GameControllerButton>(button - 301));
	}
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
		if ( players[player]->hotbar.faceMenuButtonHeld )
		{
			if ( GameController::getSDLButtonFromImpulse(controllerImpulse) == SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_LEFTSHOULDER
				|| GameController::getSDLButtonFromImpulse(controllerImpulse) == SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_RIGHTSHOULDER )
			{
				return false;
			}
		}
		return controller->binaryToggle(GameController::getSDLButtonFromImpulse(controllerImpulse));
	}
}

const bool Inputs::bControllerInputHeld(int player, const unsigned controllerImpulse) const
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
		return controller->buttonHeldToggle(GameController::getSDLTriggerFromImpulse(controllerImpulse));
	}
	else
	{
		return controller->buttonHeldToggle(GameController::getSDLButtonFromImpulse(controllerImpulse));
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

const bool Inputs::bMouseHeldLeft(int player) const
{
	if ( bMouseLeft(player) )
	{
		return vmouse[player].mouseLeftHeld;
	}
	return false;
}

const bool Inputs::bMouseHeldRight(int player) const
{
	if ( bMouseRight(player) )
	{
		return vmouse[player].mouseRightHeld;
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
	getVirtualMouse(player)->mouseLeftHeld = false;
	getVirtualMouse(player)->mouseLeftHeldTicks = 0;
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
	getVirtualMouse(player)->mouseRightHeld = false;
	getVirtualMouse(player)->mouseRightHeldTicks = 0;
}

void Inputs::controllerClearRawInput(int player, const unsigned button)
{
	if ( button < 299 || button >= (301 + SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_MAX) )
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

	if ( button == 299 || button == 300 ) // triggers
	{
		controller->consumeBinaryToggle(static_cast<SDL_GameControllerAxis>(button - 299 + SDL_CONTROLLER_AXIS_TRIGGERLEFT));
	}
	else
	{
		controller->consumeBinaryToggle(static_cast<SDL_GameControllerButton>(button - 301));
	}
}

void Inputs::controllerClearRawInputRelease(int player, const unsigned button)
{
	if ( button < 299 || button >= (301 + SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_MAX) )
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

	if ( button == 299 || button == 300 ) // triggers
	{
		//controller->consumeBinaryReleaseToggle(static_cast<SDL_GameControllerAxis>(button - 299 + SDL_CONTROLLER_AXIS_TRIGGERLEFT));
	}
	else
	{
		controller->consumeBinaryReleaseToggle(static_cast<SDL_GameControllerButton>(button - 301));
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

SDL_Rect Inputs::getGlyphRectForInput(const int player, bool pressed, const unsigned keyboardImpulse, const unsigned controllerImpulse)
{
	SDL_Rect defaultRect{ 0, 0, 0, 0 };

	if ( bPlayerUsingKeyboardControl(player) )
	{
		if ( !vmouse[player].lastMovementFromController )
		{
			// TODO - keyboard glyphs.
			// impulses[keyboardImpulse]
			return defaultRect;
		}
	}

	GameController* controller = getController(player);
	if ( !controller )
	{
		return defaultRect;
	}

	int glyphHeight = 16;
	int glyphWidth = 16;
	const int glyphSpacing = 16;

	if ( joyimpulses[controllerImpulse] == 299 || joyimpulses[controllerImpulse] == 300 ) // triggers
	{
		SDL_GameControllerAxis axis = GameController::getSDLTriggerFromImpulse(controllerImpulse);
		if ( axis == SDL_CONTROLLER_AXIS_INVALID && controllerImpulse >= 301 )
		{
			axis = static_cast<SDL_GameControllerAxis>(controllerImpulse - 299 + SDL_CONTROLLER_AXIS_TRIGGERLEFT);
		}

		switch ( axis )
		{
			case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_TRIGGERLEFT:
				glyphWidth = 13;
				glyphHeight = 11;
				return SDL_Rect{ 2 * glyphSpacing, 4 * glyphSpacing, glyphWidth, glyphHeight };
			case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
				glyphWidth = 13;
				glyphHeight = 11;
				return SDL_Rect{ 3 * glyphSpacing, 4 * glyphSpacing, glyphWidth, glyphHeight };
			default:
				return defaultRect;
		}
	}
	else
	{
		SDL_GameControllerButton but = static_cast<SDL_GameControllerButton>(controllerImpulse);
		/*if ( but == SDL_CONTROLLER_BUTTON_INVALID && controllerImpulse >= 299 )
		{
			but = static_cast<SDL_GameControllerButton>(controllerImpulse - 301);
		}*/

		switch ( but )
		{
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_A:
				glyphWidth = 11;
				glyphHeight = 12;
				return SDL_Rect{ 6 * glyphSpacing, (pressed ? 1 : 0) * glyphSpacing, glyphWidth, glyphHeight };
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_B:
				glyphWidth = 11;
				glyphHeight = 12;
				return SDL_Rect{ 7 * glyphSpacing, (pressed ? 1 : 0) * glyphSpacing, glyphWidth, glyphHeight };
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_X:
				glyphWidth = 11;
				glyphHeight = 12;
				return SDL_Rect{ 5 * glyphSpacing, (pressed ? 1 : 0) * glyphSpacing, glyphWidth, glyphHeight };
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_Y:
				glyphWidth = 11;
				glyphHeight = 12;
				return SDL_Rect{ 4 * glyphSpacing, (pressed ? 1 : 0) * glyphSpacing, glyphWidth, glyphHeight };
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
				glyphWidth = 15;
				glyphHeight = 7;
				return SDL_Rect{ 0 * glyphSpacing, (pressed ? 4 : 4) * glyphSpacing, glyphWidth, glyphHeight };
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
				glyphWidth = 15;
				glyphHeight = 7;
				return SDL_Rect{ 1 * glyphSpacing, (pressed ? 4 : 4) * glyphSpacing, glyphWidth, glyphHeight };
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_BACK:
				glyphWidth = 15;
				glyphHeight = 7;
				return SDL_Rect{ 2 * glyphSpacing, (pressed ? 1 : 1) * glyphSpacing, glyphWidth, glyphHeight };
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_START:
				glyphWidth = 15;
				glyphHeight = 7;
				return SDL_Rect{ 3 * glyphSpacing, (pressed ? 1 : 1) * glyphSpacing, glyphWidth, glyphHeight };
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_GUIDE:
				return defaultRect;
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_LEFTSTICK:
				glyphWidth = 19;
				glyphHeight = 19;
				return SDL_Rect{ 0 * glyphSpacing, (pressed ? 8 : 8) * glyphSpacing, glyphWidth, glyphHeight };
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_RIGHTSTICK:
				glyphWidth = 19;
				glyphHeight = 19;
				return SDL_Rect{ 0 * glyphSpacing, (pressed ? 8 : 8) * glyphSpacing, glyphWidth, glyphHeight };
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_LEFT:
				glyphWidth = 10;
				glyphHeight = 10;
				return SDL_Rect{ 0 * glyphSpacing, (pressed ? 0 : 0) * glyphSpacing, glyphWidth, glyphHeight };
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_UP:
				glyphWidth = 10;
				glyphHeight = 10;
				return SDL_Rect{ 1 * glyphSpacing, (pressed ? 0 : 0) * glyphSpacing, glyphWidth, glyphHeight };
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
				glyphWidth = 10;
				glyphHeight = 10;
				return SDL_Rect{ 2 * glyphSpacing, (pressed ? 0 : 0) * glyphSpacing, glyphWidth, glyphHeight };
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_DOWN:
				glyphWidth = 10;
				glyphHeight = 10;
				return SDL_Rect{ 3 * glyphSpacing, (pressed ? 0 : 0) * glyphSpacing, glyphWidth, glyphHeight };
			default:
				return defaultRect;
		}
	}

	return defaultRect;
}

bool GameController::binaryOf(Binding_t& binding) 
{
	if ( binding.type == Binding_t::CONTROLLER_AXIS || binding.type == Binding_t::CONTROLLER_BUTTON || binding.type == Binding_t::VIRTUAL_DPAD )
	{
		SDL_GameController* pad = sdl_device;
		if ( binding.type == Binding_t::CONTROLLER_BUTTON ) 
		{
			return SDL_GameControllerGetButton(pad, binding.padButton) == 1;
		}
		else if ( binding.type == Binding_t::VIRTUAL_DPAD )
		{
			return binding.padVirtualDpad != DpadDirection::CENTERED;
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

void GameController::updateButtonsReleased()
{
	if ( !isActive() )
	{
		return;
	}

	for ( int i = 0; i < NUM_JOY_STATUS; ++i )
	{
		buttons[i].binaryReleaseConsumed = true;
	}
}

void GameController::updateButtons()
{
	if ( !isActive() )
	{
		return;
	}

	bool pressed = false;

	for ( int i = 0; i < NUM_JOY_STATUS; ++i )
	{
		buttons[i].analog = analogOf(buttons[i]);

		bool oldBinary = buttons[i].binary;
		buttons[i].binary = binaryOf(buttons[i]);

		if ( buttons[i].binary )
		{
			pressed = true;
		}

		if ( oldBinary != buttons[i].binary ) 
		{
			// unconsume the input whenever it's released or pressed again.
			//messagePlayer(0, "%d: %d", i, buttons[i].binary ? 1 : 0);
			buttons[i].binaryReleaseConsumed = false;

			if ( oldBinary && !buttons[i].binary && !buttons[i].consumed )
			{
				buttons[i].binaryRelease = true;
			}
			else
			{
				buttons[i].binaryRelease = false;
			}

			buttons[i].consumed = false;
			if ( buttons[i].binary && buttons[i].buttonHeldTicks == 0 )
			{
				buttons[i].buttonHeldTicks = ticks;
			}
			else if ( !buttons[i].binary )
			{
				buttons[i].buttonHeldTicks = 0;
			}
		}
	}

	if ( pressed )
	{
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			if ( inputs.getController(i) == this )
			{
				inputs.getVirtualMouse(i)->lastMovementFromController = true;
				break;
			}
		}
	}
}

void GameController::updateAxis()
{
	if ( !isActive() )
	{
		return;
	}

	bool pressed = false;

	for ( int i = 0; i < NUM_JOY_AXIS_STATUS; ++i )
	{
		axis[i].analog = analogOf(axis[i]);

		bool oldBinary = axis[i].binary;
		axis[i].binary = binaryOf(axis[i]);

		if ( axis[i].binary )
		{
			pressed = true;
		}

		if ( oldBinary != axis[i].binary ) {
			// unconsume the input whenever it's released or pressed again.
			//messagePlayer(0, "%d: %d", i, axis[i].binary ? 1 : 0);
			axis[i].consumed = false;
		}
	}

	if ( pressed )
	{
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			if ( inputs.getController(i) == this )
			{
				inputs.getVirtualMouse(i)->lastMovementFromController = true;
				break;
			}
		}
	}
}

float GameController::analog(SDL_GameControllerButton binding) const
{
	if ( binding <= SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_INVALID || binding >= SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_MAX )
	{
		return 0.f;
	}
	return buttons[binding].analog;
}

bool GameController::binaryToggle(SDL_GameControllerButton binding) const
{
	if ( binding <= SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_INVALID || binding >= SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_MAX )
	{
		return false;
	}
	return (buttons[binding].binary && !buttons[binding].consumed);
}

bool GameController::binaryReleaseToggle(SDL_GameControllerButton binding) const
{
	if ( binding <= SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_INVALID || binding >= SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_MAX )
	{
		return false;
	}
	return (buttons[binding].binaryRelease && !buttons[binding].binaryReleaseConsumed);
}

bool GameController::buttonHeldToggle(SDL_GameControllerButton binding) const
{
	if ( binding <= SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_INVALID || binding >= SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_MAX )
	{
		return false;
	}
	return (buttons[binding].binary && !buttons[binding].consumed && (ticks - buttons[binding].buttonHeldTicks) > GameController::BUTTON_HELD_TICKS);
}

bool GameController::binary(SDL_GameControllerButton binding) const
{
	if ( binding <= SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_INVALID || binding >= SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_MAX )
	{
		return false;
	}
	return buttons[binding].binary;
}

void GameController::consumeBinaryReleaseToggle(SDL_GameControllerButton binding)
{
	if ( binding <= SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_INVALID || binding >= SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_MAX )
	{
		return;
	}
	if ( buttons[binding].binaryRelease )
	{
		buttons[binding].binaryReleaseConsumed = true;
	}
}

void GameController::consumeBinaryToggle(SDL_GameControllerButton binding)
{
	if ( binding <= SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_INVALID || binding >= SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_MAX )
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
	if ( binding <= SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_INVALID || binding >= SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_MAX )
	{
		return 0.f;
	}
	return axis[binding].analog;
}

bool GameController::binaryToggle(SDL_GameControllerAxis binding) const
{
	if ( binding <= SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_INVALID || binding >= SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_MAX )
	{
		return false;
	}
	return (axis[binding].binary && !axis[binding].consumed);
}

bool GameController::binary(SDL_GameControllerAxis binding) const
{
	if ( binding <= SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_INVALID || binding >= SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_MAX )
	{
		return false;
	}
	return axis[binding].binary;
}

bool GameController::buttonHeldToggle(SDL_GameControllerAxis binding) const
{
	if ( binding <= SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_INVALID || binding >= SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_MAX )
	{
		return false;
	}
	return (buttons[binding].binary && !buttons[binding].consumed && (ticks - buttons[binding].buttonHeldTicks) > GameController::BUTTON_HELD_TICKS);
}

void GameController::consumeBinaryToggle(SDL_GameControllerAxis binding)
{
	if ( binding <= SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_INVALID || binding >= SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_MAX )
	{
		return;
	}
	if ( axis[binding].binary )
	{
		axis[binding].consumed = true;
	}
}

GameController::DpadDirection GameController::dpadDirToggle() const
{
	if ( !virtualDpad.consumed )
	{
		return virtualDpad.padVirtualDpad;
	}
	return DpadDirection::INVALID;
}

GameController::DpadDirection GameController::dpadDir() const
{
	return virtualDpad.padVirtualDpad;
}

void GameController::consumeDpadDirToggle()
{
	virtualDpad.consumed = true;
}