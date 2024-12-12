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
#include "ui/GameUI.hpp"
#include "ui/Frame.hpp"
#include "ui/Slider.hpp"
#include "lobbies.hpp"
#include "ui/MainMenu.hpp"

#ifdef NINTENDO
#include "nintendo/baronynx.hpp"
#endif

Player* players[MAXPLAYERS] = { nullptr };
PlayerSettings_t playerSettings[MAXPLAYERS];
Entity* selectedEntity[MAXPLAYERS] = { nullptr };
Sint32 mousex = 0, mousey = 0;
Sint32 omousex = 0, omousey = 0;
Sint32 mousexrel = 0, mouseyrel = 0;

int gamepad_deadzone = 8000;
int gamepad_trigger_deadzone = 18000;
real_t gamepad_leftx_sensitivity = 1.0;
real_t gamepad_lefty_sensitivity = 1.0;
real_t gamepad_rightx_sensitivity = 1.0;
real_t gamepad_righty_sensitivity = 1.0;
real_t gamepad_menux_sensitivity = 1.0;
real_t gamepad_menuy_sensitivity = 1.0;

bool gamepad_leftx_invert = false;
bool gamepad_lefty_invert = false;
bool gamepad_rightx_invert = false;
bool gamepad_righty_invert = false;
bool gamepad_menux_invert = false;
bool gamepad_menuy_invert = false;

const int Player::Inventory_t::MAX_SPELLS_X = 4;
const int Player::Inventory_t::MAX_SPELLS_Y = 20;
const int Player::Inventory_t::MAX_CHEST_X = 4;
const int Player::Inventory_t::MAX_CHEST_Y = 3;

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
    // this crashes because the array containing these controllers
    // doesn't get purged until program exit, after SDL has deinited.
    // When SDL closes, all devices are safely closed anyway, so
    // maybe this is redundant?
	/*if ( sdl_device )
	{
		close();
	}*/
}

void GameController::close()
{
	stopRumble();
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

void GameController::reinitHaptic()
{
	return; // using SDL_GameControllerRumble instead..
	if ( !sdl_device )
	{
		return;
	}
	if ( sdl_haptic )
	{
		stopRumble();
		printlog("[INPUTS]: Stopping haptics on device: %d...", getID());
		SDL_HapticClose(sdl_haptic);
		sdl_haptic = nullptr;
	}

	printlog("[INPUTS]: Reinit haptics on device: %d", getID());
	SDL_Joystick* joystick = SDL_GameControllerGetJoystick(sdl_device);
	sdl_haptic = SDL_HapticOpenFromJoystick(joystick);
	if ( sdl_haptic == nullptr )
	{
		printlog("Notice: Controller does not support haptics! SDL Error: %s\n", SDL_GetError());
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

bool GameController::open(int sdl_which, int index)
{
#ifdef NINTENDO
	id = index;
	return true;
#else
	if (sdl_device)
	{
		close();
	}

	if (sdl_which < 0 || sdl_which >= SDL_NumJoysticks())
	{
		return false;
	}

	if (!SDL_IsGameController(sdl_which))
	{
		return false;
	}

	sdl_device = SDL_GameControllerOpen(sdl_which);

	if (sdl_device == nullptr)
	{
		printlog("Error: Failed to open game controller! SDL Error: %s\n", SDL_GetError());
	}
	else
	{
		id = index;
		printlog("Successfully initialized game controller!\n");
		name = (SDL_GameControllerNameForIndex(sdl_which));

#ifdef DISABLE_RUMBLE
		haptics.vibrationEnabled = false;
#else
		if ( SDL_GameControllerHasRumble(sdl_device) == SDL_FALSE )
		{
			printlog("Notice: Controller does not support rumble!");
			haptics.vibrationEnabled = false;
		}
		else
		{
			printlog("Controller name is \"%s\", rumble enabled", name.c_str());
			haptics.vibrationEnabled = true;
		}
#endif
		/*SDL_Joystick* joystick = SDL_GameControllerGetJoystick(sdl_device);
		sdl_haptic = SDL_HapticOpenFromJoystick(joystick);
		if ( sdl_haptic == nullptr )
		{
			printlog("Notice: Controller does not support haptics! SDL Error: %s\n", SDL_GetError());
		}
		if ( sdl_haptic )
		{
			printlog("Controller name is \"%s\", haptics available: %d", name.c_str(), SDL_HapticQuery(sdl_haptic));
		}
		else
		{
			printlog("Controller name is \"%s\", haptics disabled", name.c_str());
		}*/
	}


	return (sdl_device != nullptr);
#endif
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
#ifdef NINTENDO
	return nxIsControllerConnected(id);
#else
	return (sdl_device != nullptr);
#endif
}

real_t getGamepadMenuXSensitivity(int player)
{
	if ( !TimerExperiments::bUseTimerInterpolation )
	{
		return gamepad_menux_sensitivity;
	}
	const real_t fpsScale = getFPSScale(60.0);
	return gamepad_menux_sensitivity * fpsScale;
}

real_t getGamepadMenuYSensitivity(int player)
{
	if ( !TimerExperiments::bUseTimerInterpolation )
	{
		return gamepad_menuy_sensitivity;
	}
	const real_t fpsScale = getFPSScale(60.0);
	return gamepad_menuy_sensitivity * fpsScale;
}

real_t getGamepadRightXSensitivity(int player)
{
    const auto value = gamepad_rightx_sensitivity * playerSettings[multiplayer ? 0 : player].gamepad_rightx_sensitivity;
	if ( !TimerExperiments::bUseTimerInterpolation )
	{
		return value;
	}
	const real_t fpsScale = getFPSScale(60.0);
	return value * fpsScale;
}

real_t getGamepadRightYSensitivity(int player)
{
    const auto value = gamepad_righty_sensitivity * playerSettings[multiplayer ? 0 : player].gamepad_righty_sensitivity;
	if ( !TimerExperiments::bUseTimerInterpolation )
	{
		return value;
	}
	const real_t fpsScale = getFPSScale(60.0);
	return value * fpsScale;
}

void GameController::handleAnalog(int player)
{
	if (!isActive())
	{
		return;
	}

	//Right analog stick = look.

	bool radialMenuOpen = FollowerMenu[player].followerMenuIsOpen() || CalloutMenu[player].calloutMenuIsOpen();
	if ( !radialMenuOpen )
	{
		consumeDpadDirToggle();
		virtualDpad.padVirtualDpad = DpadDirection::CENTERED;
	}

	if (!players[player]->shootmode || gamePaused)
	{
        const auto rawx = getRawRightXMove() * (playerSettings[multiplayer ? 0 : player].gamepad_rightx_invert * -2 + 1);
        const auto rawy = getRawRightYMove() * (playerSettings[multiplayer ? 0 : player].gamepad_righty_invert * -2 + 1);
		int rightx = rawx * getGamepadMenuXSensitivity(player);
		int righty = rawy * getGamepadMenuYSensitivity(player);

		//The right stick's inversion and the menu's inversion should be independent of eachother. This just undoes any inversion.
		if (gamepad_rightx_invert ^ playerSettings[multiplayer ? 0 : player].gamepad_rightx_invert)
		{
			rightx = -rightx;
		}
		if (gamepad_righty_invert ^ playerSettings[multiplayer ? 0 : player].gamepad_righty_invert)
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
            const auto rawx = getRawRightXMove();
            const auto rawy = getRawRightYMove();
			const real_t floatx = rawx;
			const real_t floaty = rawy;

			const real_t maxInputVector = 32767;
			const real_t magnitude = sqrt(pow(floatx, 2) + pow(floaty, 2));
			const real_t normalised = magnitude / (maxInputVector);
			real_t deadzone = 0.8;

			const int numoptions = 8;
			DpadDirection dir = DpadDirection::CENTERED;
			if ( normalised >= deadzone )
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
				inputs.getVirtualMouse(player)->lastMovementFromController = true;
			}
			rightx = 0;
			righty = 0;
		}
		else if ( rightStickDeadzoneType != DEADZONE_PER_AXIS )
		{
            const auto rawx = getRawRightXMove() * (playerSettings[multiplayer ? 0 : player].gamepad_rightx_invert * -2 + 1);
            const auto rawy = getRawRightYMove() * (playerSettings[multiplayer ? 0 : player].gamepad_righty_invert * -2 + 1);
			rightx = rawx;
			righty = rawy;

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
				rightx = newMagnitude * cos(angle) * gamepad_menux_sensitivity;
				righty = newMagnitude * sin(angle) * gamepad_menuy_sensitivity;

				//rightx = oldFloatRightX;
				//righty = oldFloatRightY;
			}

			oldAxisRightX = rightx;
			oldAxisRightY = righty;
		}

		if (rightx || righty)
		{
			const auto& mouse = inputs.getVirtualMouse(player);
			mouse->lastMovementFromController = true;
			return;
		}
	}
	else
	{
		if ( enableDebugKeys )
		{
			if ( keystatus[SDLK_F1] && keystatus[SDLK_EQUALS] )
			{
				leftStickDeadzone = std::min(leftStickDeadzone + 100, 32767);
			}
			else if ( keystatus[SDLK_F1] && keystatus[SDLK_MINUS] )
			{
				leftStickDeadzone = std::max(leftStickDeadzone - 100, 0);
			}
			if ( keystatus[SDLK_F2] && keystatus[SDLK_EQUALS] )
			{
				rightStickDeadzone = std::min(rightStickDeadzone + 100, 32767);
			}
			else if ( keystatus[SDLK_F2] && keystatus[SDLK_MINUS] )
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
			rightx = getRightXMove(player);
			righty = getRightYMove(player);
		}
		else if ( rightStickDeadzoneType == DEADZONE_MAGNITUDE_LINEAR || rightStickDeadzoneType == DEADZONE_MAGNITUDE_HALFPIPE )
		{
            const auto rawx = getRawRightXMove() * (playerSettings[multiplayer ? 0 : player].gamepad_rightx_invert * -2 + 1);
            const auto rawy = getRawRightYMove() * (playerSettings[multiplayer ? 0 : player].gamepad_righty_invert * -2 + 1);
			floatx = rawx;
			floaty = rawy;

			const real_t maxInputVector = 32767 * sqrt(2);
			const real_t magnitude = std::min(32767.0, sqrt(pow(floatx, 2) + pow(floaty, 2)));
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
				floatx = newMagnitude * cos(angle) * getGamepadRightXSensitivity(player);
				floaty = newMagnitude * sin(angle) * getGamepadRightYSensitivity(player);

				//rightx = oldFloatRightX;
				//righty = oldFloatRightY;
			}

			oldFloatRightX = floatx;
			oldFloatRightY = floaty;
			oldAxisRightX = rawx;
			oldAxisRightY = rawy;
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
				SDL_PushEvent(&e);
			}
			else
			{
				mouse->floatx += rightx;
				mouse->floaty += righty;
				mouse->floatxrel += rightx;
				mouse->floatyrel += righty;

				/*if ( !mouse->draw_cursor )
				{
					mouse->draw_cursor = true;
				}*/
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

			/*if ( !mouse->draw_cursor )
			{
				mouse->draw_cursor = true;
			}*/
			mouse->moved = true;
		}
	}
}

int GameController::getRightXMove(int player) // with sensitivity
{
	if (!isActive())
	{
		return 0;
	}
	int x = getRawRightXMove();
    x *= playerSettings[multiplayer ? 0 : player].gamepad_rightx_invert * -2 + 1;
	x *= getGamepadRightXSensitivity(player);
	return x;
}

int GameController::getRightYMove(int player) // with sensitivity
{
	if (!isActive())
	{
		return 0;
	}
	int y = getRawRightYMove();
    y *= playerSettings[multiplayer ? 0 : player].gamepad_righty_invert * -2 + 1;
	y *= getGamepadRightYSensitivity(player);
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
#ifdef NINTENDO
	int x = nxGetControllerState(id, nxInput::LeftStickX);
#else
	int x = SDL_GameControllerGetAxis(sdl_device, SDL_CONTROLLER_AXIS_LEFTX);
#endif
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
#ifdef NINTENDO
	int y = nxGetControllerState(id, nxInput::LeftStickY);
#else
	int y = SDL_GameControllerGetAxis(sdl_device, SDL_CONTROLLER_AXIS_LEFTY);
#endif
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
#ifdef NINTENDO
	int x = nxGetControllerState(id, nxInput::RightStickX);
#else
	int x = SDL_GameControllerGetAxis(sdl_device, SDL_CONTROLLER_AXIS_RIGHTX);
#endif
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
#ifdef NINTENDO
	int y = nxGetControllerState(id, nxInput::RightStickY);
#else
	int y = SDL_GameControllerGetAxis(sdl_device, SDL_CONTROLLER_AXIS_RIGHTY);
#endif
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
#ifdef NINTENDO
	int n = nxGetControllerState(id, nxInput::ButtonZL) * INT16_MAX;
#else
	int n = SDL_GameControllerGetAxis(sdl_device, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
#endif
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
#ifdef NINTENDO
	int n = nxGetControllerState(id, nxInput::ButtonZR) * INT16_MAX;
#else
	int n = SDL_GameControllerGetAxis(sdl_device, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
#endif
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

bool Player::CharacterSheet_t::isSheetElementAllowedToNavigateTo(Player::CharacterSheet_t::SheetElements element)
{
	if ( element == SHEET_ENUM_END )
	{
		return false;
	}

	if ( player.bUseCompactGUIHeight() || player.bUseCompactGUIWidth() )
	{
		switch ( element )
		{
			case SHEET_SKILL_LIST:
			case SHEET_TIMER:
				return false;
				break;
			default:
				return true;
				break;
		}
	}
	else
	{
		return true;
	}
}

bool Player::GUI_t::handleCharacterSheetMovement()
{
	if ( activeModule != MODULE_CHARACTERSHEET )
	{
		return false;
	}
	bool dpad_moved = false;
	int player = this->player.playernum;

	if ( !players[player]->bControlEnabled
		|| gamePaused
		|| players[player]->usingCommand()
		|| players[player]->GUI.isDropdownActive() )
	{
		return false;
	}
	if ( !Input::inputs[player].binaryToggle("InventoryMoveUp")
		&& !Input::inputs[player].binaryToggle("InventoryMoveLeft")
		&& !Input::inputs[player].binaryToggle("InventoryMoveRight")
		&& !Input::inputs[player].binaryToggle("InventoryMoveDown")
		&& !Input::inputs[player].binaryToggle("InventoryMoveUpAnalog")
		&& !Input::inputs[player].binaryToggle("InventoryMoveLeftAnalog")
		&& !Input::inputs[player].binaryToggle("InventoryMoveRightAnalog")
		&& !Input::inputs[player].binaryToggle("InventoryMoveDownAnalog") )
	{
		return false;
	}

	auto& characterSheet_t = players[player]->characterSheet;
	int currentElement = characterSheet_t.selectedElement;

	bool bCompactView = false;
	if ( this->player.bUseCompactGUIHeight() || this->player.bUseCompactGUIWidth() )
	{
		bCompactView = true;
	}

	std::vector<int> elementSelectableList;
	Player::CharacterSheet_t::SheetElements defaultElement = Player::CharacterSheet_t::SHEET_OPEN_MAP;

	if ( !bCompactView )
	{
		elementSelectableList.push_back(Player::CharacterSheet_t::SHEET_OPEN_MAP);
		elementSelectableList.push_back(Player::CharacterSheet_t::SHEET_OPEN_LOG);
		elementSelectableList.push_back(Player::CharacterSheet_t::SHEET_TIMER);
		elementSelectableList.push_back(Player::CharacterSheet_t::SHEET_DUNGEON_FLOOR);
		elementSelectableList.push_back(Player::CharacterSheet_t::SHEET_CHAR_RACE_SEX);
		elementSelectableList.push_back(Player::CharacterSheet_t::SHEET_CHAR_CLASS);
		elementSelectableList.push_back(Player::CharacterSheet_t::SHEET_GOLD);
		elementSelectableList.push_back(Player::CharacterSheet_t::SHEET_SKILL_LIST);
		elementSelectableList.push_back(Player::CharacterSheet_t::SHEET_STR);
		elementSelectableList.push_back(Player::CharacterSheet_t::SHEET_DEX);
		elementSelectableList.push_back(Player::CharacterSheet_t::SHEET_CON);
		elementSelectableList.push_back(Player::CharacterSheet_t::SHEET_INT);
		elementSelectableList.push_back(Player::CharacterSheet_t::SHEET_PER);
		elementSelectableList.push_back(Player::CharacterSheet_t::SHEET_CHR);
		elementSelectableList.push_back(Player::CharacterSheet_t::SHEET_ATK);
		elementSelectableList.push_back(Player::CharacterSheet_t::SHEET_AC);
		elementSelectableList.push_back(Player::CharacterSheet_t::SHEET_POW);
		elementSelectableList.push_back(Player::CharacterSheet_t::SHEET_RES);
		elementSelectableList.push_back(Player::CharacterSheet_t::SHEET_RGN);
		elementSelectableList.push_back(Player::CharacterSheet_t::SHEET_RGN_MP);
		elementSelectableList.push_back(Player::CharacterSheet_t::SHEET_WGT);
	}
	else
	{
		std::vector<int> elementSelectableList1;
		elementSelectableList1.push_back(Player::CharacterSheet_t::SHEET_DUNGEON_FLOOR);
		elementSelectableList1.push_back(Player::CharacterSheet_t::SHEET_CHAR_RACE_SEX);
		elementSelectableList1.push_back(Player::CharacterSheet_t::SHEET_CHAR_CLASS);
		elementSelectableList1.push_back(Player::CharacterSheet_t::SHEET_GOLD);
		if ( currentElement == Player::CharacterSheet_t::SHEET_OPEN_LOG )
		{
			elementSelectableList1.push_back(Player::CharacterSheet_t::SHEET_OPEN_LOG);
		}
		else
		{
			elementSelectableList1.push_back(Player::CharacterSheet_t::SHEET_OPEN_MAP);
		}

		std::vector<int> elementSelectableList2;
		elementSelectableList2.push_back(Player::CharacterSheet_t::SHEET_STR);
		elementSelectableList2.push_back(Player::CharacterSheet_t::SHEET_DEX);
		elementSelectableList2.push_back(Player::CharacterSheet_t::SHEET_CON);
		elementSelectableList2.push_back(Player::CharacterSheet_t::SHEET_INT);
		elementSelectableList2.push_back(Player::CharacterSheet_t::SHEET_PER);
		elementSelectableList2.push_back(Player::CharacterSheet_t::SHEET_CHR);
		elementSelectableList2.push_back(Player::CharacterSheet_t::SHEET_ATK);
		elementSelectableList2.push_back(Player::CharacterSheet_t::SHEET_AC);
		elementSelectableList2.push_back(Player::CharacterSheet_t::SHEET_POW);
		elementSelectableList2.push_back(Player::CharacterSheet_t::SHEET_RES);
		elementSelectableList2.push_back(Player::CharacterSheet_t::SHEET_RGN);
		elementSelectableList2.push_back(Player::CharacterSheet_t::SHEET_RGN_MP);
		elementSelectableList2.push_back(Player::CharacterSheet_t::SHEET_WGT);

		if ( std::find(elementSelectableList1.begin(), elementSelectableList1.end(), currentElement) != elementSelectableList1.end() )
		{
			for ( auto e : elementSelectableList1 )
			{
				elementSelectableList.push_back(e);
			}

			if ( Input::inputs[player].binaryToggle("InventoryMoveLeft")
				|| Input::inputs[player].binaryToggle("InventoryMoveLeftAnalog")
				|| Input::inputs[player].binaryToggle("InventoryMoveRight")
				|| Input::inputs[player].binaryToggle("InventoryMoveRightAnalog") )
			{
				defaultElement = Player::CharacterSheet_t::SHEET_STR;
			}
			else
			{
				defaultElement = Player::CharacterSheet_t::SHEET_OPEN_MAP;
			}
		}
		else if ( std::find(elementSelectableList2.begin(), elementSelectableList2.end(), currentElement) != elementSelectableList2.end() )
		{
			for ( auto e : elementSelectableList2 )
			{
				elementSelectableList.push_back(e);
			}
			if ( Input::inputs[player].binaryToggle("InventoryMoveLeft") 
				|| Input::inputs[player].binaryToggle("InventoryMoveLeftAnalog")
				|| Input::inputs[player].binaryToggle("InventoryMoveRight")
				|| Input::inputs[player].binaryToggle("InventoryMoveRightAnalog") )
			{
				defaultElement = Player::CharacterSheet_t::SHEET_OPEN_MAP;
			}
			else
			{
				defaultElement = Player::CharacterSheet_t::SHEET_STR;
			}
		}
		else
		{
			for ( auto e : elementSelectableList1 )
			{
				elementSelectableList.push_back(e);
			}
			defaultElement = Player::CharacterSheet_t::SHEET_OPEN_MAP;
		}
	}

	if ( elementSelectableList.empty() )
	{
		return false;
	}


	if ( Input::inputs[player].binaryToggle("InventoryMoveLeft")
		|| Input::inputs[player].binaryToggle("InventoryMoveLeftAnalog") )
	{
		Input::inputs[player].consumeBinaryToggle("InventoryMoveLeft");
		Input::inputs[player].consumeBinaryToggle("InventoryMoveLeftAnalog");
		if ( bCompactView )
		{
			if ( currentElement == Player::CharacterSheet_t::SHEET_OPEN_LOG )
			{
				currentElement = Player::CharacterSheet_t::SHEET_OPEN_MAP;
			}
			else
			{
				switch ( currentElement )
				{
					case Player::CharacterSheet_t::SHEET_DUNGEON_FLOOR:
						currentElement = Player::CharacterSheet_t::SHEET_STR;
						break;
					case Player::CharacterSheet_t::SHEET_CHAR_RACE_SEX:
						currentElement = Player::CharacterSheet_t::SHEET_INT;
						break;
					case Player::CharacterSheet_t::SHEET_CHAR_CLASS:
						currentElement = Player::CharacterSheet_t::SHEET_PER;
						break;
					case Player::CharacterSheet_t::SHEET_GOLD:
						currentElement = Player::CharacterSheet_t::SHEET_CHR;
						break;
					case Player::CharacterSheet_t::SHEET_OPEN_MAP:
						currentElement = Player::CharacterSheet_t::SHEET_ATK;
						break;

					case Player::CharacterSheet_t::SHEET_STR:
						currentElement = Player::CharacterSheet_t::SHEET_DUNGEON_FLOOR;
						break;
					case Player::CharacterSheet_t::SHEET_DEX:
						currentElement = Player::CharacterSheet_t::SHEET_DUNGEON_FLOOR;
						break;
					case Player::CharacterSheet_t::SHEET_CON:
						currentElement = Player::CharacterSheet_t::SHEET_DUNGEON_FLOOR;
						break;
					case Player::CharacterSheet_t::SHEET_INT:
						currentElement = Player::CharacterSheet_t::SHEET_CHAR_RACE_SEX;
						break;
					case Player::CharacterSheet_t::SHEET_PER:
						currentElement = Player::CharacterSheet_t::SHEET_CHAR_CLASS;
						break;
					case Player::CharacterSheet_t::SHEET_CHR:
						currentElement = Player::CharacterSheet_t::SHEET_GOLD;
						break;
					case Player::CharacterSheet_t::SHEET_ATK:
					case Player::CharacterSheet_t::SHEET_AC:
					case Player::CharacterSheet_t::SHEET_POW:
					case Player::CharacterSheet_t::SHEET_RES:
					case Player::CharacterSheet_t::SHEET_RGN:
					case Player::CharacterSheet_t::SHEET_RGN_MP:
					case Player::CharacterSheet_t::SHEET_WGT:
						currentElement = Player::CharacterSheet_t::SHEET_OPEN_LOG;
						break;
					default:
						currentElement = defaultElement;
						break;
				}
			}
		}
		characterSheet_t.selectElement((Player::CharacterSheet_t::SheetElements)currentElement, false, false);
		dpad_moved = true;
	}
	else if ( Input::inputs[player].binaryToggle("InventoryMoveRight")
		|| Input::inputs[player].binaryToggle("InventoryMoveRightAnalog") )
	{
		Input::inputs[player].consumeBinaryToggle("InventoryMoveRight");
		Input::inputs[player].consumeBinaryToggle("InventoryMoveRightAnalog");
		if ( bCompactView )
		{
			if ( currentElement == Player::CharacterSheet_t::SHEET_OPEN_MAP )
			{
				currentElement = Player::CharacterSheet_t::SHEET_OPEN_LOG;
			}
			else
			{
				switch ( currentElement )
				{
					case Player::CharacterSheet_t::SHEET_DUNGEON_FLOOR:
						currentElement = Player::CharacterSheet_t::SHEET_STR;
						break;
					case Player::CharacterSheet_t::SHEET_CHAR_RACE_SEX:
						currentElement = Player::CharacterSheet_t::SHEET_INT;
						break;
					case Player::CharacterSheet_t::SHEET_CHAR_CLASS:
						currentElement = Player::CharacterSheet_t::SHEET_PER;
						break;
					case Player::CharacterSheet_t::SHEET_GOLD:
						currentElement = Player::CharacterSheet_t::SHEET_CHR;
						break;
					case Player::CharacterSheet_t::SHEET_OPEN_LOG:
						currentElement = Player::CharacterSheet_t::SHEET_ATK;
						break;

					case Player::CharacterSheet_t::SHEET_STR:
						currentElement = Player::CharacterSheet_t::SHEET_DUNGEON_FLOOR;
						break;
					case Player::CharacterSheet_t::SHEET_DEX:
						currentElement = Player::CharacterSheet_t::SHEET_DUNGEON_FLOOR;
						break;
					case Player::CharacterSheet_t::SHEET_CON:
						currentElement = Player::CharacterSheet_t::SHEET_DUNGEON_FLOOR;
						break;
					case Player::CharacterSheet_t::SHEET_INT:
						currentElement = Player::CharacterSheet_t::SHEET_CHAR_RACE_SEX;
						break;
					case Player::CharacterSheet_t::SHEET_PER:
						currentElement = Player::CharacterSheet_t::SHEET_CHAR_CLASS;
						break;
					case Player::CharacterSheet_t::SHEET_CHR:
						currentElement = Player::CharacterSheet_t::SHEET_GOLD;
						break;
					case Player::CharacterSheet_t::SHEET_ATK:
					case Player::CharacterSheet_t::SHEET_AC:
					case Player::CharacterSheet_t::SHEET_POW:
					case Player::CharacterSheet_t::SHEET_RES:
					case Player::CharacterSheet_t::SHEET_RGN:
					case Player::CharacterSheet_t::SHEET_RGN_MP:
					case Player::CharacterSheet_t::SHEET_WGT:
						currentElement = Player::CharacterSheet_t::SHEET_OPEN_MAP;
						break;
					default:
						currentElement = defaultElement;
						break;
				}
			}
		}
		characterSheet_t.selectElement((Player::CharacterSheet_t::SheetElements)currentElement, false, false);
		dpad_moved = true;
	}
	else
	{
		if ( Input::inputs[player].binaryToggle("InventoryMoveUp")
			|| Input::inputs[player].binaryToggle("InventoryMoveUpAnalog") )
		{
			Input::inputs[player].consumeBinaryToggle("InventoryMoveUp");
			Input::inputs[player].consumeBinaryToggle("InventoryMoveUpAnalog");

			auto itr = std::find(elementSelectableList.begin(), elementSelectableList.end(), currentElement);
			if ( itr == elementSelectableList.end() )
			{
				currentElement = defaultElement;
			}
			else if ( itr == elementSelectableList.begin() )
			{
				currentElement = elementSelectableList[elementSelectableList.size() - 1];
			}
			else
			{
				currentElement = *(std::prev(itr));
			}
			/*currentElement = currentElement - 1;
			if ( currentElement <= Player::CharacterSheet_t::SHEET_UNSELECTED )
			{
				currentElement = Player::CharacterSheet_t::SHEET_ENUM_END - 1;
			}*/
			characterSheet_t.selectElement((Player::CharacterSheet_t::SheetElements)currentElement, false, false);
			dpad_moved = true;
		}
		if ( Input::inputs[player].binaryToggle("InventoryMoveDown")
			|| Input::inputs[player].binaryToggle("InventoryMoveDownAnalog") )
		{
			Input::inputs[player].consumeBinaryToggle("InventoryMoveDown");
			Input::inputs[player].consumeBinaryToggle("InventoryMoveDownAnalog");

			auto itr = std::find(elementSelectableList.begin(), elementSelectableList.end(), currentElement);
			if ( itr == elementSelectableList.end() )
			{
				currentElement = defaultElement;
			}
			else
			{
				if ( std::next(itr) == elementSelectableList.end() )
				{
					currentElement = elementSelectableList[0];
				}
				else
				{
					currentElement = *(std::next(itr));
				}
			}

			/*currentElement = currentElement + 1;
			if ( currentElement >= Player::CharacterSheet_t::SHEET_ENUM_END )
			{
				currentElement = Player::CharacterSheet_t::SHEET_UNSELECTED + 1;
			}*/
			characterSheet_t.selectElement((Player::CharacterSheet_t::SheetElements)currentElement, false, false);
			dpad_moved = true;
		}
	}

	if ( dpad_moved )
	{
		dpad_moved = false;
		warpControllerToModule(false);
		inputs.getVirtualMouse(player)->draw_cursor = false;
		Player::soundMovement();
		return true;
	}
	return false;
}

bool Player::GUI_t::isGameoverActive()
{
	if ( gameUIFrame[player.playernum] )
	{
		for ( auto f : gameUIFrame[player.playernum]->getFrames() )
		{
			if ( !strcmp(f->getName(), "gameover") )
			{
				return true;
			}
		}
	}
	return false;
}

bool Player::GUI_t::bModuleAccessibleWithMouse(GUIModules moduleToAccess)
{
	if ( isGameoverActive() )
	{
		return false;
	}
	if ( moduleToAccess == MODULE_INVENTORY || moduleToAccess == MODULE_SPELLS
		|| moduleToAccess == MODULE_HOTBAR || moduleToAccess == MODULE_CHEST
		|| moduleToAccess == MODULE_SHOP || moduleToAccess == MODULE_TINKERING
		|| moduleToAccess == MODULE_FEATHER
		|| moduleToAccess == MODULE_ALCHEMY
		|| moduleToAccess == MODULE_ASSISTSHRINE
		|| moduleToAccess == MODULE_ITEMEFFECTGUI )
	{
		if ( moduleToAccess == MODULE_HOTBAR )
		{
			if ( player.inventoryUI.bCompactView
				&& (player.shopGUI.bOpen
					|| GenericGUI[player.playernum].isGUIOpen()) )
			{
				return false;
			}
			if ( player.hotbar.animHide > 0.01 )
			{
				return false;
			}
		}
		if ( player.bookGUI.bBookOpen || player.skillSheet.bSkillSheetOpen
			|| player.signGUI.bSignOpen
			|| FollowerMenu[player.playernum].followerMenuIsOpen()
			|| CalloutMenu[player.playernum].calloutMenuIsOpen()
			|| player.minimap.mapWindow || player.messageZone.logWindow )
		{
			return false;
		}
		if ( player.hud.statusFxFocusedWindowActive )
		{
			return false;
		}
		if ( player.paperDoll.portraitActiveToEdit )
		{
			return false;
		}
		return true;
	}
	return true;
}

bool Player::GUI_t::returnToPreviousActiveModule()
{
	if ( previousModule == MODULE_NONE )
	{
		return false;
	}

	if ( player.shootmode )
	{
		// no action
	}
	else if ( previousModule == Player::GUI_t::MODULE_HOTBAR )
	{
		activateModule(MODULE_HOTBAR);
		player.hotbar.updateHotbar(); // simulate the slots rearranging before we try to move the mouse to it.
		warpControllerToModule(false);
	}
	else if ( previousModule == MODULE_INVENTORY || previousModule == MODULE_SPELLS )
	{
		if ( player.inventory_mode == INVENTORY_MODE_SPELL )
		{
			activateModule(MODULE_SPELLS);
			warpControllerToModule(false);
		}
		else if ( player.inventory_mode == INVENTORY_MODE_ITEM )
		{
			activateModule(MODULE_INVENTORY);
			warpControllerToModule(false);
		}
	}
	else if ( previousModule == MODULE_CHARACTERSHEET )
	{
		activateModule(MODULE_CHARACTERSHEET);
		//player.characterSheet.selectElement(Player::CharacterSheet_t::SHEET_OPEN_MAP, false, false);
		warpControllerToModule(false);
	}
	else
	{
		player.openStatusScreen(GUI_MODE_INVENTORY,
			INVENTORY_MODE_ITEM);
	}

	previousModule = MODULE_NONE;
	return true;
}

Player::GUI_t::GUIModules Player::GUI_t::handleModuleNavigation(bool checkDestinationOnly, bool checkLeftNavigation)
{
	auto& input = Input::inputs[player.playernum];
	if ( player.shootmode || gamePaused || !inputs.hasController(player.playernum)
		|| nohud || !player.isLocalPlayer() )
	{
		if ( !checkDestinationOnly )
		{
			input.consumeBinaryToggle("UINavLeftBumper");
			input.consumeBinaryToggle("UINavRightBumper");
		}
		return MODULE_NONE;
	}

	bool bCompactView = false;
	if ( player.bUseCompactGUIHeight() || player.bUseCompactGUIWidth() )
	{
		bCompactView = true;
	}

	if ( input.binaryToggle("UINavLeftBumper")
		|| (checkLeftNavigation && checkDestinationOnly) )
	{
		if ( activeModule == MODULE_INVENTORY 
			&& (player.inventoryUI.bFirstTimeSnapCursor || checkDestinationOnly ) )
		{
			if ( player.shopGUI.bOpen && player.gui_mode == GUI_MODE_SHOP )
			{
				if ( inputs.getUIInteraction(player.playernum)->selectedItem )
				{
					if ( !checkDestinationOnly ) // no action
					{
						activateModule(MODULE_INVENTORY);
						warpControllerToModule(false);
						input.consumeBinaryToggle("UINavLeftBumper");
						inputs.getVirtualMouse(player.playernum)->draw_cursor = false;
						soundModuleNavigation();
					}
					return MODULE_INVENTORY;
				}
				else
				{
					if ( !checkDestinationOnly )
					{
						player.inventory_mode = INVENTORY_MODE_ITEM;
						activateModule(MODULE_SHOP);
						warpControllerToModule(false);
						input.consumeBinaryToggle("UINavLeftBumper");
						inputs.getVirtualMouse(player.playernum)->draw_cursor = false;
						soundModuleNavigation();
					}
					return MODULE_SHOP;
				}
			}
			else if ( GenericGUI[player.playernum].tinkerGUI.bOpen )
			{
				return MODULE_NONE;
			}
			else if ( GenericGUI[player.playernum].featherGUI.bOpen )
			{
				return MODULE_NONE;
			}
			else if ( GenericGUI[player.playernum].assistShrineGUI.bOpen )
			{
				return MODULE_NONE;
			}
			else if ( GenericGUI[player.playernum].alchemyGUI.bOpen )
			{
				if ( inputs.getUIInteraction(player.playernum)->selectedItem )
				{
					if ( !checkDestinationOnly )
					{
						input.consumeBinaryToggle("UINavLeftBumper");
					}
					return MODULE_NONE;
				}
				else
				{
					if ( !checkDestinationOnly )
					{
						player.inventory_mode = INVENTORY_MODE_ITEM;
						activateModule(MODULE_ALCHEMY);
						warpControllerToModule(false);
						input.consumeBinaryToggle("UINavLeftBumper");
						inputs.getVirtualMouse(player.playernum)->draw_cursor = false;
						soundModuleNavigation();
					}
					return MODULE_ALCHEMY;
				}
			}
			else if ( player.inventoryUI.chestGUI.bOpen )
			{
				if ( !checkDestinationOnly )
				{
					player.inventory_mode = INVENTORY_MODE_ITEM;
					activateModule(MODULE_CHEST);
					warpControllerToModule(false);
					input.consumeBinaryToggle("UINavLeftBumper");
					inputs.getVirtualMouse(player.playernum)->draw_cursor = false;
					soundModuleNavigation();
				}
				return MODULE_CHEST;
			}
			else if ( inputs.getUIInteraction(player.playernum)->selectedItem || bCompactView )
			{
				if ( player.ghost.isActive() )
				{
					return MODULE_NONE;
				}
				if ( !checkDestinationOnly )
				{
					activateModule(MODULE_HOTBAR);
					player.hotbar.updateHotbar(); // simulate the slots rearranging before we try to move the mouse to it.
					warpControllerToModule(false);
					input.consumeBinaryToggle("UINavLeftBumper");
					inputs.getVirtualMouse(player.playernum)->draw_cursor = false;
					soundModuleNavigation();
				}
				return MODULE_HOTBAR;
			}
			else
			{
				if ( !checkDestinationOnly )
				{
					activateModule(MODULE_CHARACTERSHEET);
					if ( player.characterSheet.selectedElement == Player::CharacterSheet_t::SHEET_UNSELECTED
						|| !player.characterSheet.isSheetElementAllowedToNavigateTo(player.characterSheet.selectedElement) )
					{
						player.characterSheet.selectElement(Player::CharacterSheet_t::SHEET_OPEN_MAP, false, false);
					}
					warpControllerToModule(false);
					input.consumeBinaryToggle("UINavLeftBumper");
					inputs.getVirtualMouse(player.playernum)->draw_cursor = false;
					soundModuleNavigation();
				}
				return MODULE_CHARACTERSHEET;
			}
		}
		else if ( activeModule == MODULE_SPELLS 
			&& (player.inventoryUI.spellPanel.bFirstTimeSnapCursor || checkDestinationOnly) )
		{
			if ( player.shopGUI.bOpen || player.inventoryUI.chestGUI.bOpen || player.ghost.isActive() )
			{
				return MODULE_NONE;
			}
			if ( !checkDestinationOnly )
			{
				activateModule(MODULE_HOTBAR);
				player.hotbar.updateHotbar(); // simulate the slots rearranging before we try to move the mouse to it.
				warpControllerToModule(false);
				input.consumeBinaryToggle("UINavLeftBumper");
				inputs.getVirtualMouse(player.playernum)->draw_cursor = false;
				soundModuleNavigation();
			}
			return MODULE_HOTBAR;
		}
		else if ( activeModule == MODULE_CHEST
			&& (player.inventoryUI.chestGUI.bFirstTimeSnapCursor || checkDestinationOnly) )
		{
			if ( !checkDestinationOnly )
			{
				player.inventory_mode = INVENTORY_MODE_ITEM;
				activateModule(MODULE_INVENTORY);
				warpControllerToModule(false);
				input.consumeBinaryToggle("UINavLeftBumper");
				inputs.getVirtualMouse(player.playernum)->draw_cursor = false;
				soundModuleNavigation();
			}
			return MODULE_INVENTORY;
		}
		else if ( activeModule == MODULE_SHOP
			&& (player.shopGUI.bFirstTimeSnapCursor || checkDestinationOnly) )
		{
			if ( !checkDestinationOnly )
			{
				player.inventory_mode = INVENTORY_MODE_ITEM;
				activateModule(MODULE_INVENTORY);
				warpControllerToModule(false);
				input.consumeBinaryToggle("UINavLeftBumper");
				inputs.getVirtualMouse(player.playernum)->draw_cursor = false;
				soundModuleNavigation();
			}
			return MODULE_INVENTORY;
		}
		else if ( activeModule == MODULE_ALCHEMY
			&& (GenericGUI[player.playernum].alchemyGUI.bFirstTimeSnapCursor || checkDestinationOnly) )
		{
			if ( inputs.getUIInteraction(player.playernum)->selectedItem )
			{
				if ( !checkDestinationOnly )
				{
					input.consumeBinaryToggle("UINavLeftBumper");
				}
				return MODULE_NONE;
			}
			else
			{
				if ( !checkDestinationOnly )
				{
					player.inventory_mode = INVENTORY_MODE_ITEM;
					activateModule(MODULE_INVENTORY);
					warpControllerToModule(false);
					input.consumeBinaryToggle("UINavLeftBumper");
					inputs.getVirtualMouse(player.playernum)->draw_cursor = false;
					soundModuleNavigation();
				}
				return MODULE_INVENTORY;
			}
		}
		else if ( activeModule == MODULE_ASSISTSHRINE )
		{
			return MODULE_NONE;
		}
		else if ( activeModule == MODULE_TINKERING )
		{
			return MODULE_NONE;
		}
		else if ( activeModule == MODULE_FEATHER )
		{
			return MODULE_NONE;
		}
		else if ( activeModule == MODULE_HOTBAR )
		{
			if ( bCompactView && player.hud.compactLayoutMode == Player::HUD_t::COMPACT_LAYOUT_CHARSHEET )
			{
				if ( !checkDestinationOnly )
				{
					activateModule(MODULE_CHARACTERSHEET);
					if ( player.characterSheet.selectedElement == Player::CharacterSheet_t::SHEET_UNSELECTED
						|| !player.characterSheet.isSheetElementAllowedToNavigateTo(player.characterSheet.selectedElement) )
					{
						player.characterSheet.selectElement(Player::CharacterSheet_t::SHEET_OPEN_MAP, false, false);
					}
					warpControllerToModule(false);
					input.consumeBinaryToggle("UINavLeftBumper");
					inputs.getVirtualMouse(player.playernum)->draw_cursor = false;
					soundModuleNavigation();
				}
				return MODULE_CHARACTERSHEET;
			}
			else if ( player.inventory_mode == INVENTORY_MODE_SPELL 
				&& (player.inventoryUI.spellPanel.bFirstTimeSnapCursor || checkDestinationOnly ) )
			{
				if ( !checkDestinationOnly )
				{
					activateModule(MODULE_SPELLS);
					warpControllerToModule(false);
					input.consumeBinaryToggle("UINavLeftBumper");
					inputs.getVirtualMouse(player.playernum)->draw_cursor = false;
					soundModuleNavigation();
				}
				return MODULE_SPELLS;
			}
			else if ( player.inventory_mode == INVENTORY_MODE_ITEM 
				&& (player.inventoryUI.bFirstTimeSnapCursor || checkDestinationOnly ) )
			{
				if ( !checkDestinationOnly )
				{
					if ( !inputs.getUIInteraction(player.playernum)->selectedItem && bCompactView )
					{
						// code to bounce to the paper doll/inventory based on visual position
						//if ( player.inventoryUI.getSelectedSlotY() <= Player::Inventory_t::PaperDollRows::DOLL_ROW_5
						//	&& player.inventoryUI.paperDollPanelJustify == Player::PANEL_JUSTIFY_RIGHT )
						//{
						//	player.inventoryUI.selectSlot(player.inventoryUI.DEFAULT_INVENTORY_SIZEX - 1, player.inventoryUI.DEFAULT_INVENTORY_SIZEY - 1);
						//}
						//else if ( player.inventoryUI.getSelectedSlotY() >= 0 && player.inventoryUI.paperDollPanelJustify == Player::PANEL_JUSTIFY_LEFT )
						//{
						//	std::pair<int, int> goodSpot{ 10000, -10000 }; // dummy coordinates
						//	if ( player.inventoryUI.paperDollPanelJustify == Player::PANEL_JUSTIFY_LEFT )
						//	{
						//		goodSpot.first = -10000;
						//	}
						//	for ( auto& slot : player.paperDoll.dollSlots )
						//	{
						//		if ( slot.item != 0 )
						//		{
						//			int coordX = 0;
						//			int coordY = 0;
						//			player.paperDoll.getCoordinatesFromSlotType(slot.slotType, coordX, coordY);
						//			if ( coordY > goodSpot.second )
						//			{
						//				if ( player.inventoryUI.paperDollPanelJustify == Player::PANEL_JUSTIFY_RIGHT
						//					&& coordX <= goodSpot.first )
						//				{
						//					goodSpot.first = coordX;
						//					goodSpot.second = coordY;
						//				}
						//				else if ( player.inventoryUI.paperDollPanelJustify == Player::PANEL_JUSTIFY_LEFT
						//					&& coordX >= goodSpot.second )
						//				{
						//					goodSpot.first = coordX;
						//					goodSpot.second = coordY;
						//				}
						//			}
						//		}
						//	}
						//	if ( goodSpot.first >= Player::Inventory_t::PaperDollColumns::DOLL_COLUMN_LEFT
						//		&& goodSpot.first <= Player::Inventory_t::PaperDollColumns::DOLL_COLUMN_RIGHT
						//		&& goodSpot.second >= Player::Inventory_t::PaperDollRows::DOLL_ROW_1
						//		&& goodSpot.second <= Player::Inventory_t::PaperDollRows::DOLL_ROW_5 )
						//	{
						//		player.inventoryUI.selectSlot(goodSpot.first, goodSpot.second);
						//	}
						//}
					}
					activateModule(MODULE_INVENTORY);
					warpControllerToModule(false);
					input.consumeBinaryToggle("UINavLeftBumper");
					inputs.getVirtualMouse(player.playernum)->draw_cursor = false;
					soundModuleNavigation();
				}
				return MODULE_INVENTORY;
			}
		}
		else if ( activeModule == MODULE_CHARACTERSHEET 
			&& (player.characterSheet.isInteractable || checkDestinationOnly) )
		{
			if ( bCompactView && player.hud.compactLayoutMode == Player::HUD_t::COMPACT_LAYOUT_CHARSHEET )
			{
				// no action here
				if ( !checkDestinationOnly )
				{
					if ( player.characterSheet.selectedElement == Player::CharacterSheet_t::SHEET_UNSELECTED
						|| !player.characterSheet.isSheetElementAllowedToNavigateTo(player.characterSheet.selectedElement) )
					{
						player.characterSheet.selectElement(Player::CharacterSheet_t::SHEET_OPEN_MAP, false, false);
					}
					warpControllerToModule(false);
					input.consumeBinaryToggle("UINavLeftBumper");
					inputs.getVirtualMouse(player.playernum)->draw_cursor = false;
					if ( !player.ghost.isActive() )
					{
						soundModuleNavigation();
					}
				}
				return MODULE_CHARACTERSHEET;
			}
			else if ( player.ghost.isActive() )
			{
				if ( player.inventory_mode == INVENTORY_MODE_SPELL
					&& (player.inventoryUI.spellPanel.bFirstTimeSnapCursor || checkDestinationOnly) )
				{
					if ( !checkDestinationOnly )
					{
						activateModule(MODULE_SPELLS);
						warpControllerToModule(false);
						input.consumeBinaryToggle("UINavLeftBumper");
						inputs.getVirtualMouse(player.playernum)->draw_cursor = false;
						soundModuleNavigation();
					}
					return MODULE_SPELLS;
				}
				else if ( player.inventory_mode == INVENTORY_MODE_ITEM
					&& (player.inventoryUI.bFirstTimeSnapCursor || checkDestinationOnly) )
				{
					if ( !checkDestinationOnly )
					{
						activateModule(MODULE_INVENTORY);
						warpControllerToModule(false);
						input.consumeBinaryToggle("UINavLeftBumper");
						inputs.getVirtualMouse(player.playernum)->draw_cursor = false;
						soundModuleNavigation();
					}
					return MODULE_INVENTORY;
				}
			}
			else if ( !checkDestinationOnly )
			{
				activateModule(MODULE_HOTBAR);
				player.hotbar.updateHotbar(); // simulate the slots rearranging before we try to move the mouse to it.
				warpControllerToModule(false);
				input.consumeBinaryToggle("UINavLeftBumper");
				inputs.getVirtualMouse(player.playernum)->draw_cursor = false;
				soundModuleNavigation();
			}
			return MODULE_HOTBAR;
		}

		if ( !checkDestinationOnly )
		{
			input.consumeBinaryToggle("UINavLeftBumper");
			inputs.getVirtualMouse(player.playernum)->draw_cursor = false;
		}
	}
	if ( input.binaryToggle("UINavRightBumper")
		|| (!checkLeftNavigation && checkDestinationOnly) )
	{
		if ( activeModule == MODULE_INVENTORY 
			&& (player.inventoryUI.bFirstTimeSnapCursor || checkDestinationOnly) )
		{
			if ( player.shopGUI.bOpen && player.gui_mode == GUI_MODE_SHOP )
			{
				if ( inputs.getUIInteraction(player.playernum)->selectedItem )
				{
					if ( !checkDestinationOnly ) // no action
					{
						activateModule(MODULE_INVENTORY);
						warpControllerToModule(false);
						input.consumeBinaryToggle("UINavRightBumper");
						inputs.getVirtualMouse(player.playernum)->draw_cursor = false;
						soundModuleNavigation();
					}
					return MODULE_INVENTORY;
				}
				else
				{
					if ( !checkDestinationOnly )
					{
						player.inventory_mode = INVENTORY_MODE_ITEM;
						activateModule(MODULE_SHOP);
						warpControllerToModule(false);
						input.consumeBinaryToggle("UINavRightBumper");
						inputs.getVirtualMouse(player.playernum)->draw_cursor = false;
						soundModuleNavigation();
					}
					return MODULE_SHOP;
				}
			}
			else if ( GenericGUI[player.playernum].tinkerGUI.bOpen )
			{
				return MODULE_NONE;
			}
			else if ( GenericGUI[player.playernum].featherGUI.bOpen )
			{
				return MODULE_NONE;
			}
			else if ( GenericGUI[player.playernum].assistShrineGUI.bOpen )
			{
				return MODULE_NONE;
			}
			else if ( player.ghost.isActive() )
			{
				if ( bCompactView )
				{
					return MODULE_NONE;
				}
				if ( !checkDestinationOnly )
				{
					activateModule(MODULE_CHARACTERSHEET);
					if ( player.characterSheet.selectedElement == Player::CharacterSheet_t::SHEET_UNSELECTED
						|| !player.characterSheet.isSheetElementAllowedToNavigateTo(player.characterSheet.selectedElement) )
					{
						player.characterSheet.selectElement(Player::CharacterSheet_t::SHEET_OPEN_MAP, false, false);
					}
					warpControllerToModule(false);
					input.consumeBinaryToggle("UINavRightBumper");
					inputs.getVirtualMouse(player.playernum)->draw_cursor = false;
					soundModuleNavigation();
				}
				return MODULE_CHARACTERSHEET;
			}
			else if ( GenericGUI[player.playernum].alchemyGUI.bOpen )
			{
				if ( inputs.getUIInteraction(player.playernum)->selectedItem )
				{
					if ( !checkDestinationOnly )
					{
						input.consumeBinaryToggle("UINavRightBumper");
					}
					return MODULE_NONE;
				}
				else
				{
					if ( !checkDestinationOnly )
					{
						player.inventory_mode = INVENTORY_MODE_ITEM;
						activateModule(MODULE_ALCHEMY);
						warpControllerToModule(false);
						input.consumeBinaryToggle("UINavRightBumper");
						inputs.getVirtualMouse(player.playernum)->draw_cursor = false;
						soundModuleNavigation();
					}
					return MODULE_ALCHEMY;
				}
			}
			else if ( player.inventoryUI.chestGUI.bOpen )
			{
				if ( !checkDestinationOnly )
				{
					player.inventory_mode = INVENTORY_MODE_ITEM;
					activateModule(MODULE_CHEST);
					warpControllerToModule(false);
					input.consumeBinaryToggle("UINavRightBumper");
					inputs.getVirtualMouse(player.playernum)->draw_cursor = false;
					soundModuleNavigation();
				}
				return MODULE_CHEST;
			}
			else
			{
				if ( !checkDestinationOnly )
				{
					activateModule(MODULE_HOTBAR);
					player.hotbar.updateHotbar(); // simulate the slots rearranging before we try to move the mouse to it.
					warpControllerToModule(false);
					input.consumeBinaryToggle("UINavRightBumper");
					inputs.getVirtualMouse(player.playernum)->draw_cursor = false;
					soundModuleNavigation();
				}
				return MODULE_HOTBAR;
			}
		}
		else if ( activeModule == MODULE_SPELLS 
			&& (player.inventoryUI.spellPanel.bFirstTimeSnapCursor || checkDestinationOnly ) )
		{
			if ( player.shopGUI.bOpen || player.inventoryUI.chestGUI.bOpen || player.ghost.isActive() )
			{
				return MODULE_NONE;
			}
			if ( !checkDestinationOnly )
			{
				activateModule(MODULE_HOTBAR);
				player.hotbar.updateHotbar(); // simulate the slots rearranging before we try to move the mouse to it.
				warpControllerToModule(false);
				input.consumeBinaryToggle("UINavRightBumper");
				inputs.getVirtualMouse(player.playernum)->draw_cursor = false;
				soundModuleNavigation();
			}
			return MODULE_HOTBAR;
		}
		else if ( activeModule == MODULE_CHEST
			&& (player.inventoryUI.chestGUI.bFirstTimeSnapCursor || checkDestinationOnly) )
		{
			if ( !checkDestinationOnly )
			{
				player.inventory_mode = INVENTORY_MODE_ITEM;
				activateModule(MODULE_INVENTORY);
				warpControllerToModule(false);
				input.consumeBinaryToggle("UINavRightBumper");
				inputs.getVirtualMouse(player.playernum)->draw_cursor = false;
				soundModuleNavigation();
			}
			return MODULE_INVENTORY;
		}
		else if ( activeModule == MODULE_SHOP
			&& (player.shopGUI.bFirstTimeSnapCursor || checkDestinationOnly) )
		{
			if ( !checkDestinationOnly )
			{
				player.inventory_mode = INVENTORY_MODE_ITEM;
				activateModule(MODULE_INVENTORY);
				warpControllerToModule(false);
				input.consumeBinaryToggle("UINavRightBumper");
				inputs.getVirtualMouse(player.playernum)->draw_cursor = false;
				soundModuleNavigation();
			}
			return MODULE_INVENTORY;
		}
		else if ( activeModule == MODULE_ALCHEMY
			&& (GenericGUI[player.playernum].alchemyGUI.bFirstTimeSnapCursor || checkDestinationOnly) )
		{
			if ( inputs.getUIInteraction(player.playernum)->selectedItem )
			{
				if ( !checkDestinationOnly )
				{
					input.consumeBinaryToggle("UINavRightBumper");
				}
				return MODULE_NONE;
			}
			else
			{
				if ( !checkDestinationOnly )
				{
					player.inventory_mode = INVENTORY_MODE_ITEM;
					activateModule(MODULE_INVENTORY);
					warpControllerToModule(false);
					input.consumeBinaryToggle("UINavRightBumper");
					inputs.getVirtualMouse(player.playernum)->draw_cursor = false;
					soundModuleNavigation();
				}
				return MODULE_INVENTORY;
			}
		}
		else if ( activeModule == MODULE_TINKERING )
		{
			return MODULE_NONE;
		}
		else if ( activeModule == MODULE_FEATHER )
		{
			return MODULE_NONE;
		}
		else if ( activeModule == MODULE_ASSISTSHRINE )
		{
			return MODULE_NONE;
		}
		else if ( activeModule == MODULE_HOTBAR )
		{
			if ( bCompactView && player.hud.compactLayoutMode == Player::HUD_t::COMPACT_LAYOUT_CHARSHEET )
			{
				if ( !checkDestinationOnly )
				{
					activateModule(MODULE_CHARACTERSHEET);
					if ( player.characterSheet.selectedElement == Player::CharacterSheet_t::SHEET_UNSELECTED
						|| !player.characterSheet.isSheetElementAllowedToNavigateTo(player.characterSheet.selectedElement) )
					{
						player.characterSheet.selectElement(Player::CharacterSheet_t::SHEET_OPEN_MAP, false, false);
					}
					warpControllerToModule(false);
					input.consumeBinaryToggle("UINavRightBumper");
					inputs.getVirtualMouse(player.playernum)->draw_cursor = false;
					soundModuleNavigation();
				}
				return MODULE_CHARACTERSHEET;
			}
			else if ( inputs.getUIInteraction(player.playernum)->selectedItem || bCompactView )
			{
				if ( player.inventory_mode == INVENTORY_MODE_SPELL 
					&& (player.inventoryUI.spellPanel.bFirstTimeSnapCursor || checkDestinationOnly ) )
				{
					if ( !checkDestinationOnly )
					{
						activateModule(MODULE_SPELLS);
						warpControllerToModule(false);
						input.consumeBinaryToggle("UINavRightBumper");
						inputs.getVirtualMouse(player.playernum)->draw_cursor = false;
						soundModuleNavigation();
					}
					return MODULE_SPELLS;
				}
				else if ( player.inventory_mode == INVENTORY_MODE_ITEM 
					&& (player.inventoryUI.bFirstTimeSnapCursor || checkDestinationOnly) )
				{
					if ( !checkDestinationOnly )
					{
						// code to bounce to the paper doll/inventory based on visual position
						//if ( !inputs.getUIInteraction(player.playernum)->selectedItem && bCompactView )
						//{
						//	if ( player.inventoryUI.getSelectedSlotY() <= Player::Inventory_t::PaperDollRows::DOLL_ROW_5
						//		&& player.inventoryUI.paperDollPanelJustify == Player::PANEL_JUSTIFY_LEFT )
						//	{
						//		player.inventoryUI.selectSlot(0, player.inventoryUI.DEFAULT_INVENTORY_SIZEY - 1);
						//	}
						//	else if ( player.inventoryUI.getSelectedSlotY() >= 0 && player.inventoryUI.paperDollPanelJustify == Player::PANEL_JUSTIFY_RIGHT )
						//	{
						//		std::pair<int, int> goodSpot{10000, -10000}; // dummy coordinates
						//		if ( player.inventoryUI.paperDollPanelJustify == Player::PANEL_JUSTIFY_LEFT )
						//		{
						//			goodSpot.first = -10000;
						//		}
						//		for ( auto& slot : player.paperDoll.dollSlots )
						//		{
						//			if ( slot.item != 0 )
						//			{
						//				int coordX = 0;
						//				int coordY = 0;
						//				player.paperDoll.getCoordinatesFromSlotType(slot.slotType, coordX, coordY);
						//				if ( coordY > goodSpot.second )
						//				{
						//					if ( player.inventoryUI.paperDollPanelJustify == Player::PANEL_JUSTIFY_RIGHT
						//						&& coordX <= goodSpot.first )
						//					{
						//						goodSpot.first = coordX;
						//						goodSpot.second = coordY;
						//					}
						//					else if ( player.inventoryUI.paperDollPanelJustify == Player::PANEL_JUSTIFY_LEFT
						//						&& coordX >= goodSpot.second )
						//					{
						//						goodSpot.first = coordX;
						//						goodSpot.second = coordY;
						//					}
						//				}
						//			}
						//		}
						//		if ( goodSpot.first >= Player::Inventory_t::PaperDollColumns::DOLL_COLUMN_LEFT
						//			&& goodSpot.first <= Player::Inventory_t::PaperDollColumns::DOLL_COLUMN_RIGHT
						//			&& goodSpot.second >= Player::Inventory_t::PaperDollRows::DOLL_ROW_1
						//			&& goodSpot.second <= Player::Inventory_t::PaperDollRows::DOLL_ROW_5 )
						//		{
						//			player.inventoryUI.selectSlot(goodSpot.first, goodSpot.second);
						//		}
						//	}
						//}
						activateModule(MODULE_INVENTORY);
						warpControllerToModule(false);
						input.consumeBinaryToggle("UINavRightBumper");
						inputs.getVirtualMouse(player.playernum)->draw_cursor = false;
						soundModuleNavigation();
					}
					return MODULE_INVENTORY;
				}
			}
			else if ( player.inventory_mode == INVENTORY_MODE_SPELL )
			{
				if ( player.inventoryUI.spellPanel.bFirstTimeSnapCursor || checkDestinationOnly )
				{
					if ( !checkDestinationOnly )
					{
						activateModule(MODULE_SPELLS);
						warpControllerToModule(false);
						input.consumeBinaryToggle("UINavRightBumper");
						inputs.getVirtualMouse(player.playernum)->draw_cursor = false;
						soundModuleNavigation();
					}
					return MODULE_SPELLS;
				}
			}
			else
			{
				if ( !checkDestinationOnly )
				{
					activateModule(MODULE_CHARACTERSHEET);
					if ( player.characterSheet.selectedElement == Player::CharacterSheet_t::SHEET_UNSELECTED
						|| !player.characterSheet.isSheetElementAllowedToNavigateTo(player.characterSheet.selectedElement) )
					{
						player.characterSheet.selectElement(Player::CharacterSheet_t::SHEET_OPEN_MAP, false, false);
					}
					warpControllerToModule(false);
					input.consumeBinaryToggle("UINavRightBumper");
					inputs.getVirtualMouse(player.playernum)->draw_cursor = false;
					soundModuleNavigation();
				}
				return MODULE_CHARACTERSHEET;
			}
		}
		else if ( activeModule == MODULE_CHARACTERSHEET 
			&& (player.characterSheet.isInteractable || checkDestinationOnly) )
		{
			if ( bCompactView && player.hud.compactLayoutMode == Player::HUD_t::COMPACT_LAYOUT_CHARSHEET )
			{
				// no action here
				if ( !checkDestinationOnly )
				{
					if ( player.characterSheet.selectedElement == Player::CharacterSheet_t::SHEET_UNSELECTED
						|| !player.characterSheet.isSheetElementAllowedToNavigateTo(player.characterSheet.selectedElement) )
					{
						player.characterSheet.selectElement(Player::CharacterSheet_t::SHEET_OPEN_MAP, false, false);
					}
					warpControllerToModule(false);
					input.consumeBinaryToggle("UINavRightBumper");
					inputs.getVirtualMouse(player.playernum)->draw_cursor = false;
					soundModuleNavigation();
				}
				return MODULE_CHARACTERSHEET;
			}
			else if ( player.inventory_mode == INVENTORY_MODE_SPELL 
				&& (player.inventoryUI.spellPanel.bFirstTimeSnapCursor || checkDestinationOnly) )
			{
				if ( !checkDestinationOnly )
				{
					activateModule(MODULE_SPELLS);
					warpControllerToModule(false);
					input.consumeBinaryToggle("UINavRightBumper");
					inputs.getVirtualMouse(player.playernum)->draw_cursor = false;
					soundModuleNavigation();
				}
				return MODULE_SPELLS;
			}
			else if ( player.inventory_mode == INVENTORY_MODE_ITEM 
				&& (player.inventoryUI.bFirstTimeSnapCursor || checkDestinationOnly) )
			{
				if ( !checkDestinationOnly )
				{
					activateModule(MODULE_INVENTORY);
					warpControllerToModule(false);
					input.consumeBinaryToggle("UINavRightBumper");
					inputs.getVirtualMouse(player.playernum)->draw_cursor = false;
					soundModuleNavigation();
				}
				return MODULE_INVENTORY;
			}
		}

		if ( !checkDestinationOnly )
		{
			input.consumeBinaryToggle("UINavRightBumper");
			inputs.getVirtualMouse(player.playernum)->draw_cursor = false;
		}
	}
	return MODULE_NONE;
}

static ConsoleVariable<int> cvar_game_ui_sfx_volume("/game_ui_sfx_volume", 32);
void Player::soundMovement()
{
	playSound(604, *cvar_game_ui_sfx_volume);
}

void Player::soundHotbarShootmodeMovement()
{
	playSound(604, *cvar_game_ui_sfx_volume);
}

void Player::soundModuleNavigation()
{
	playSound(494, *cvar_game_ui_sfx_volume);
}

void Player::soundActivate() 
{
	playSound(493, *cvar_game_ui_sfx_volume);
}

void Player::soundCancel() 
{
	playSound(499, *cvar_game_ui_sfx_volume);
}

void Player::soundStatusOpen()
{
	//playSound(494, *cvar_game_ui_sfx_volume);
}

void Player::soundStatusClose()
{
	playSound(494, *cvar_game_ui_sfx_volume);
}

bool Player::GUI_t::handleInventoryMovement()
{
	bool dpad_moved = false;

	if ( inputs.getUIInteraction(player.playernum)->itemMenuOpen )
	{
		return false;
	}
	int player = this->player.playernum;
	auto& hotbar_t = players[player]->hotbar;

	if ( !bActiveModuleUsesInventory() )
	{
		return false;
	}

	bool rightStickMovement = 
		!(this->player.inventoryUI.itemTooltipDisplay.expanded 
			&& (players[player]->GUI.activeModule != Player::GUI_t::MODULE_INVENTORY
				|| players[player]->GUI.activeModule != Player::GUI_t::MODULE_HOTBAR))
		&& players[player]->GUI.activeModule != Player::GUI_t::MODULE_FEATHER
		&& players[player]->GUI.activeModule != Player::GUI_t::MODULE_ASSISTSHRINE
		&& players[player]->GUI.activeModule != Player::GUI_t::MODULE_SPELLS;

	if ( Input::inputs[player].binaryToggle("InventoryMoveLeft")
		|| (Input::inputs[player].binaryToggle("InventoryMoveLeftAnalog") && rightStickMovement) )
	{
		Input::inputs[player].consumeBinaryToggle("InventoryMoveLeft");
		Input::inputs[player].consumeBinaryToggle("InventoryMoveLeftAnalog");
		if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_HOTBAR
			&& hotbarGamepadControlEnabled(player) )
		{
			//If hotbar is focused and chest, etc, not opened, navigate hotbar.
			hotbar_t.selectHotbarSlot(players[player]->hotbar.current_hotbar - 1);
			auto slotFrame = hotbar_t.getHotbarSlotFrame(hotbar_t.current_hotbar);
			if ( slotFrame && slotFrame->isDisabled() )
			{
				// skip this disabled one, move twice. e.g using facebar and 10th slot disabled
				hotbar_t.selectHotbarSlot(hotbar_t.current_hotbar - 1);
			}
			warpMouseToSelectedHotbarSlot(player);
		}
		else if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_CHEST )
		{
			select_chest_slot(player,
				players[player]->inventoryUI.getSelectedChestX(),
				players[player]->inventoryUI.getSelectedChestY(),
				-1, 0);
		}
		else if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_SHOP )
		{
			select_shop_slot(player,
				players[player]->shopGUI.getSelectedShopX(),
				players[player]->shopGUI.getSelectedShopY(),
				-1, 0);
		}
		else if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_TINKERING )
		{
			select_tinkering_slot(player,
				GenericGUI[player].tinkerGUI.getSelectedTinkerSlotX(),
				GenericGUI[player].tinkerGUI.getSelectedTinkerSlotY(),
				-1, 0);
		}
		else if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_ALCHEMY )
		{
			select_alchemy_slot(player,
				GenericGUI[player].alchemyGUI.getSelectedAlchemySlotX(),
				GenericGUI[player].alchemyGUI.getSelectedAlchemySlotY(),
				-1, 0);
		}
		else if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_ASSISTSHRINE )
		{
			select_assistshrine_slot(player,
				GenericGUI[player].assistShrineGUI.getSelectedAssistShrineX(),
				GenericGUI[player].assistShrineGUI.getSelectedAssistShrineY(),
				-1, 0);
		}
		else if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_FEATHER )
		{
			if ( !GenericGUI[player].featherGUI.bFirstTimeSnapCursor
				|| !GenericGUI[player].featherGUI.isInteractable
				|| (abs(Input::inputs[player].analog("MenuScrollDown")) > (Input::inputs[player].getAnalogToggleThreshold()))
				|| (abs(Input::inputs[player].analog("MenuScrollUp")) > (Input::inputs[player].getAnalogToggleThreshold())) )
			{
				inputs.getVirtualMouse(player)->draw_cursor = false;
				return false;
			}
			select_feather_slot(player,
				GenericGUI[player].featherGUI.getSelectedFeatherSlotX(),
				GenericGUI[player].featherGUI.getSelectedFeatherSlotY(),
				-1, 0);
		}
		else if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_SPELLS )
		{
			Slider* slider = nullptr;
			bool disableDpad = false;
			if ( players[player]->inventoryUI.spellFrame )
			{
				auto baseFrame = players[player]->inventoryUI.spellFrame->findFrame("spell base");
				slider = baseFrame->findSlider("spell slider");
				if ( slider && !slider->isDisabled() &&
					((abs(Input::inputs[player].analog("MenuScrollDown")) > (Input::inputs[player].getAnalogToggleThreshold()))
						|| (abs(Input::inputs[player].analog("MenuScrollUp")) > (Input::inputs[player].getAnalogToggleThreshold()))) )
				{
					disableDpad = true;
				}
			}
			if ( !players[player]->inventoryUI.spellPanel.bFirstTimeSnapCursor
				|| !players[player]->inventoryUI.spellPanel.isInteractable
				|| disableDpad )
			{
				inputs.getVirtualMouse(player)->draw_cursor = false;
				return false;
			}
			select_spell_slot(player,
				players[player]->inventoryUI.getSelectedSpellX(), 
				players[player]->inventoryUI.getSelectedSpellY(),
				-1, 0);
		}
		else
		{
			//Navigate inventory.
			select_inventory_slot(player, 
				players[player]->inventoryUI.getSelectedSlotX(), 
				players[player]->inventoryUI.getSelectedSlotY(),
				-1, 0);
		}

		dpad_moved = true;
	}

	if ( Input::inputs[player].binaryToggle("InventoryMoveRight")
		|| (Input::inputs[player].binaryToggle("InventoryMoveRightAnalog") && rightStickMovement) )
	{
		Input::inputs[player].consumeBinaryToggle("InventoryMoveRight");
		Input::inputs[player].consumeBinaryToggle("InventoryMoveRightAnalog");
		if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_HOTBAR
			&& hotbarGamepadControlEnabled(player) )
		{
			//If hotbar is focused and chest, etc, not opened, navigate hotbar.
			hotbar_t.selectHotbarSlot(players[player]->hotbar.current_hotbar + 1);
			auto slotFrame = hotbar_t.getHotbarSlotFrame(hotbar_t.current_hotbar);
			if ( slotFrame && slotFrame->isDisabled() )
			{
				// skip this disabled one, move twice. e.g using facebar and 10th slot disabled
				hotbar_t.selectHotbarSlot(hotbar_t.current_hotbar + 1);
			}
			warpMouseToSelectedHotbarSlot(player);
		}
		else if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_CHEST )
		{
			select_chest_slot(player,
				players[player]->inventoryUI.getSelectedChestX(),
				players[player]->inventoryUI.getSelectedChestY(),
				1, 0);
		}
		else if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_SHOP )
		{
			select_shop_slot(player,
				players[player]->shopGUI.getSelectedShopX(),
				players[player]->shopGUI.getSelectedShopY(),
				1, 0);
		}
		else if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_TINKERING )
		{
			select_tinkering_slot(player,
				GenericGUI[player].tinkerGUI.getSelectedTinkerSlotX(),
				GenericGUI[player].tinkerGUI.getSelectedTinkerSlotY(),
				1, 0);
		}
		else if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_ALCHEMY )
		{
			select_alchemy_slot(player,
				GenericGUI[player].alchemyGUI.getSelectedAlchemySlotX(),
				GenericGUI[player].alchemyGUI.getSelectedAlchemySlotY(),
				1, 0);
		}
		else if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_ASSISTSHRINE )
		{
			select_assistshrine_slot(player,
				GenericGUI[player].assistShrineGUI.getSelectedAssistShrineX(),
				GenericGUI[player].assistShrineGUI.getSelectedAssistShrineY(),
				1, 0);
		}
		else if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_FEATHER )
		{
			if ( !GenericGUI[player].featherGUI.bFirstTimeSnapCursor
				|| !GenericGUI[player].featherGUI.isInteractable
				|| (abs(Input::inputs[player].analog("MenuScrollDown")) > (Input::inputs[player].getAnalogToggleThreshold()))
				|| (abs(Input::inputs[player].analog("MenuScrollUp")) > (Input::inputs[player].getAnalogToggleThreshold())) )
			{
				inputs.getVirtualMouse(player)->draw_cursor = false;
				return false;
			}
			select_feather_slot(player,
				GenericGUI[player].featherGUI.getSelectedFeatherSlotX(),
				GenericGUI[player].featherGUI.getSelectedFeatherSlotY(),
				1, 0);
		}
		else if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_SPELLS )
		{
			Slider* slider = nullptr;
			bool disableDpad = false;
			if ( players[player]->inventoryUI.spellFrame )
			{
				auto baseFrame = players[player]->inventoryUI.spellFrame->findFrame("spell base");
				slider = baseFrame->findSlider("spell slider");
				if ( slider && !slider->isDisabled() &&
					((abs(Input::inputs[player].analog("MenuScrollDown")) > (Input::inputs[player].getAnalogToggleThreshold()))
						|| (abs(Input::inputs[player].analog("MenuScrollUp")) > (Input::inputs[player].getAnalogToggleThreshold()))) )
				{
					disableDpad = true;
				}
			}
			if ( !players[player]->inventoryUI.spellPanel.bFirstTimeSnapCursor
				|| !players[player]->inventoryUI.spellPanel.isInteractable
				|| disableDpad )
			{
				inputs.getVirtualMouse(player)->draw_cursor = false;
				return false;
			}
			select_spell_slot(player,
				players[player]->inventoryUI.getSelectedSpellX(),
				players[player]->inventoryUI.getSelectedSpellY(),
				1, 0);
		}
		else
		{
			//Navigate inventory.
			select_inventory_slot(player,
				players[player]->inventoryUI.getSelectedSlotX(),
				players[player]->inventoryUI.getSelectedSlotY(),
				1, 0);
		}

		dpad_moved = true;
	}

	if ( Input::inputs[player].binaryToggle("InventoryMoveUp")
		|| (Input::inputs[player].binaryToggle("InventoryMoveUpAnalog") && rightStickMovement) )
	{
		Input::inputs[player].consumeBinaryToggle("InventoryMoveUp");
		Input::inputs[player].consumeBinaryToggle("InventoryMoveUpAnalog");
		if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_HOTBAR
			&& hotbarGamepadControlEnabled(player) )
		{
			if ( players[player]->inventory_mode == INVENTORY_MODE_SPELL )
			{
				Item* itemToSnapTo = nullptr;
				if ( auto& selectedItem = inputs.getUIInteraction(player)->selectedItem )
				{
					if ( itemCategory(selectedItem) == SPELL_CAT )
					{
						itemToSnapTo = selectedItem;
					}
				}
				else
				{
					itemToSnapTo = uidToItem(hotbar_t.slots()[hotbar_t.current_hotbar].item);
					if ( itemToSnapTo && itemCategory(itemToSnapTo) != SPELL_CAT )
					{
						itemToSnapTo = nullptr;
					}
				}
				if ( itemToSnapTo )
				{
					players[player]->inventoryUI.selectSpell(itemToSnapTo->x, itemToSnapTo->y);
				}
				players[player]->inventoryUI.spellPanel.scrollToSlot(players[player]->inventoryUI.getSelectedSpellX(),
					players[player]->inventoryUI.getSelectedSpellY(), false);
				players[player]->GUI.activateModule(Player::GUI_t::MODULE_SPELLS);
			}
			else
			{
				Item* itemToSnapTo = nullptr;
				if ( auto& selectedItem = inputs.getUIInteraction(player)->selectedItem )
				{
					if ( itemCategory(selectedItem) != SPELL_CAT )
					{
						itemToSnapTo = selectedItem;
					}
				}
				else
				{
					// enable this to move from an item on the hotbar to it's inventory slot
					/*itemToSnapTo = uidToItem(hotbar_t.slots()[hotbar_t.current_hotbar].item);
					if ( itemToSnapTo && itemCategory(itemToSnapTo) == SPELL_CAT )
					{
						itemToSnapTo = nullptr;
					}*/
				}
				if ( itemToSnapTo )
				{
					auto slot = players[player]->paperDoll.getSlotForItem(*itemToSnapTo);
					if ( slot != Player::PaperDoll_t::PaperDollSlotType::SLOT_MAX )
					{
						int x, y;
						players[player]->paperDoll.getCoordinatesFromSlotType(slot, x, y);
						players[player]->inventoryUI.selectSlot(x, y);
					}
					else
					{
						players[player]->inventoryUI.selectSlot(itemToSnapTo->x, itemToSnapTo->y);
					}
				}
				players[player]->GUI.activateModule(Player::GUI_t::MODULE_INVENTORY);
			}
		}
		else if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_CHEST )
		{
			select_chest_slot(player,
				players[player]->inventoryUI.getSelectedChestX(),
				players[player]->inventoryUI.getSelectedChestY(),
				0, -1);
		}
		else if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_SHOP )
		{
			select_shop_slot(player,
				players[player]->shopGUI.getSelectedShopX(),
				players[player]->shopGUI.getSelectedShopY(),
				0, -1);
		}
		else if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_TINKERING )
		{
			select_tinkering_slot(player,
				GenericGUI[player].tinkerGUI.getSelectedTinkerSlotX(),
				GenericGUI[player].tinkerGUI.getSelectedTinkerSlotY(),
				0, -1);
		}
		else if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_ALCHEMY )
		{
			select_alchemy_slot(player,
				GenericGUI[player].alchemyGUI.getSelectedAlchemySlotX(),
				GenericGUI[player].alchemyGUI.getSelectedAlchemySlotY(),
				0, -1);
		}
		else if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_ASSISTSHRINE )
		{
			select_assistshrine_slot(player,
				GenericGUI[player].assistShrineGUI.getSelectedAssistShrineX(),
				GenericGUI[player].assistShrineGUI.getSelectedAssistShrineY(),
				0, -1);
		}
		else if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_FEATHER )
		{
			if ( !GenericGUI[player].featherGUI.bFirstTimeSnapCursor
				|| !GenericGUI[player].featherGUI.isInteractable
				|| (abs(Input::inputs[player].analog("MenuScrollDown")) > (Input::inputs[player].getAnalogToggleThreshold()))
				|| (abs(Input::inputs[player].analog("MenuScrollUp")) > (Input::inputs[player].getAnalogToggleThreshold())) )
			{
				inputs.getVirtualMouse(player)->draw_cursor = false;
				return false;
			}
			select_feather_slot(player,
				GenericGUI[player].featherGUI.getSelectedFeatherSlotX(),
				GenericGUI[player].featherGUI.getSelectedFeatherSlotY(),
				0, -1);
		}
		else if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_SPELLS )
		{
			Slider* slider = nullptr;
			bool disableDpad = false;
			if ( players[player]->inventoryUI.spellFrame )
			{
				auto baseFrame = players[player]->inventoryUI.spellFrame->findFrame("spell base");
				slider = baseFrame->findSlider("spell slider");
				if ( slider && !slider->isDisabled() &&
					((abs(Input::inputs[player].analog("MenuScrollDown")) > (Input::inputs[player].getAnalogToggleThreshold()))
						|| (abs(Input::inputs[player].analog("MenuScrollUp")) > (Input::inputs[player].getAnalogToggleThreshold()))) )
				{
					disableDpad = true;
				}
			}
			if ( !players[player]->inventoryUI.spellPanel.bFirstTimeSnapCursor
				|| !players[player]->inventoryUI.spellPanel.isInteractable
				|| disableDpad )
			{
				inputs.getVirtualMouse(player)->draw_cursor = false;
				return false;
			}
			select_spell_slot(player,
				players[player]->inventoryUI.getSelectedSpellX(),
				players[player]->inventoryUI.getSelectedSpellY(),
				0, -1);
		}
		else
		{
			select_inventory_slot(player,
				players[player]->inventoryUI.getSelectedSlotX(),
				players[player]->inventoryUI.getSelectedSlotY(),
				0, -1);
			//Will handle warping to hotbar.
		}

		dpad_moved = true;
	}

	if ( Input::inputs[player].binaryToggle("InventoryMoveDown")
		|| (Input::inputs[player].binaryToggle("InventoryMoveDownAnalog") && rightStickMovement) )
	{
		Input::inputs[player].consumeBinaryToggle("InventoryMoveDown");
		Input::inputs[player].consumeBinaryToggle("InventoryMoveDownAnalog");
		if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_HOTBAR
			&& hotbarGamepadControlEnabled(player) )
		{
			if ( players[player]->inventory_mode == INVENTORY_MODE_SPELL )
			{
				Item* itemToSnapTo = nullptr;
				if ( auto& selectedItem = inputs.getUIInteraction(player)->selectedItem )
				{
					if ( itemCategory(selectedItem) == SPELL_CAT )
					{
						itemToSnapTo = selectedItem;
					}
				}
				else
				{
					itemToSnapTo = uidToItem(hotbar_t.slots()[hotbar_t.current_hotbar].item);
					if ( itemToSnapTo && itemCategory(itemToSnapTo) != SPELL_CAT )
					{
						itemToSnapTo = nullptr;
					}
				}
				if ( itemToSnapTo )
				{
					players[player]->inventoryUI.selectSpell(itemToSnapTo->x, itemToSnapTo->y);
				}
				players[player]->inventoryUI.spellPanel.scrollToSlot(players[player]->inventoryUI.getSelectedSpellX(), 
					players[player]->inventoryUI.getSelectedSpellY(), false);
				players[player]->GUI.activateModule(Player::GUI_t::MODULE_SPELLS);
			}
			else
			{
				Item* itemToSnapTo = nullptr;
				if ( auto& selectedItem = inputs.getUIInteraction(player)->selectedItem )
				{
					if ( itemCategory(selectedItem) != SPELL_CAT )
					{
						itemToSnapTo = selectedItem;
					}
				}
				else
				{
					// enable this to move from an item on the hotbar to it's inventory slot
					/*itemToSnapTo = uidToItem(hotbar_t.slots()[hotbar_t.current_hotbar].item);
					if ( itemToSnapTo && itemCategory(itemToSnapTo) == SPELL_CAT )
					{
						itemToSnapTo = nullptr;
					}*/
				}
				if ( itemToSnapTo )
				{
					auto slot = players[player]->paperDoll.getSlotForItem(*itemToSnapTo);
					if ( slot != Player::PaperDoll_t::PaperDollSlotType::SLOT_MAX )
					{
						int x, y;
						players[player]->paperDoll.getCoordinatesFromSlotType(slot, x, y);
						players[player]->inventoryUI.selectSlot(x, y);
					}
					else
					{
						players[player]->inventoryUI.selectSlot(itemToSnapTo->x, itemToSnapTo->y);
					}
				}
				players[player]->GUI.activateModule(Player::GUI_t::MODULE_INVENTORY);
			}
		}
		else if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_CHEST )
		{
			select_chest_slot(player,
				players[player]->inventoryUI.getSelectedChestX(),
				players[player]->inventoryUI.getSelectedChestY(),
				0, 1);
		}
		else if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_SHOP )
		{
			select_shop_slot(player,
				players[player]->shopGUI.getSelectedShopX(),
				players[player]->shopGUI.getSelectedShopY(),
				0, 1);
		}
		else if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_TINKERING )
		{
			select_tinkering_slot(player,
				GenericGUI[player].tinkerGUI.getSelectedTinkerSlotX(),
				GenericGUI[player].tinkerGUI.getSelectedTinkerSlotY(),
				0, 1);
		}
		else if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_ALCHEMY )
		{
			select_alchemy_slot(player,
				GenericGUI[player].alchemyGUI.getSelectedAlchemySlotX(),
				GenericGUI[player].alchemyGUI.getSelectedAlchemySlotY(),
				0, 1);
		}
		else if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_ASSISTSHRINE )
		{
			select_assistshrine_slot(player,
				GenericGUI[player].assistShrineGUI.getSelectedAssistShrineX(),
				GenericGUI[player].assistShrineGUI.getSelectedAssistShrineY(),
				0, 1);
		}
		else if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_FEATHER )
		{
			if ( !GenericGUI[player].featherGUI.bFirstTimeSnapCursor
				|| !GenericGUI[player].featherGUI.isInteractable
				|| (abs(Input::inputs[player].analog("MenuScrollDown")) > (Input::inputs[player].getAnalogToggleThreshold()))
				|| (abs(Input::inputs[player].analog("MenuScrollUp")) > (Input::inputs[player].getAnalogToggleThreshold())) )
			{
				inputs.getVirtualMouse(player)->draw_cursor = false;
				return false;
			}
			select_feather_slot(player,
				GenericGUI[player].featherGUI.getSelectedFeatherSlotX(),
				GenericGUI[player].featherGUI.getSelectedFeatherSlotY(),
				0, 1);
		}
		else if ( players[player]->GUI.activeModule == Player::GUI_t::MODULE_SPELLS )
		{
			Slider* slider = nullptr;
			bool disableDpad = false;
			if ( players[player]->inventoryUI.spellFrame )
			{
				auto baseFrame = players[player]->inventoryUI.spellFrame->findFrame("spell base");
				slider = baseFrame->findSlider("spell slider");
				if ( slider && !slider->isDisabled() &&
					((abs(Input::inputs[player].analog("MenuScrollDown")) > (Input::inputs[player].getAnalogToggleThreshold()))
					|| (abs(Input::inputs[player].analog("MenuScrollUp")) > (Input::inputs[player].getAnalogToggleThreshold()))) )
				{
					disableDpad = true;
				}
			}
			if ( !players[player]->inventoryUI.spellPanel.bFirstTimeSnapCursor
				|| !players[player]->inventoryUI.spellPanel.isInteractable
				|| disableDpad )
			{
				inputs.getVirtualMouse(player)->draw_cursor = false;
				return false;
			}
			select_spell_slot(player,
				players[player]->inventoryUI.getSelectedSpellX(),
				players[player]->inventoryUI.getSelectedSpellY(),
				0, 1);
		}
		else
		{
			select_inventory_slot(player,
				players[player]->inventoryUI.getSelectedSlotX(),
				players[player]->inventoryUI.getSelectedSlotY(),
				0, 1);
		}

		dpad_moved = true;
	}

	if (dpad_moved)
	{
		dpad_moved = false;
		inputs.getVirtualMouse(player)->draw_cursor = false;
		Player::soundMovement();
		return true;
	}

	return false;
}

void initGameControllers()
{
	inputs.setPlayerIDAllowedKeyboard(0);

	bool found = false;
	for ( auto controller : game_controllers )
	{
		if ( controller.isActive() )
		{
			found = true;
			break;
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
	oscillatorTick = 0;
	memset(&hapticEffect, 0, sizeof(HapticEffect));
}

GameController::Haptic_t::HapticEffect* GameController::handleRumble()
{
#ifdef DISABLE_RUMBLE
	return nullptr;
#endif
	if ( haptics.hapticTick == std::numeric_limits<Uint32>::max() )
	{
		haptics.hapticTick = 0;
	}
	else
	{
#ifdef NINTENDO
		timeNow = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now());
		auto elapsed = (timeNow - timeStart).count();
		if (elapsed > 500 / TICKS_PER_SECOND) {
			timeStart = timeNow;
			++haptics.oscillatorTick;
			if ( haptics.oscillatorTick % 2 == 0 )
			{
				++haptics.hapticTick;
			}
		}
#else
		++haptics.hapticTick;
#endif
	}
	size_t size = haptics.activeRumbles.size();
	if ( size == 0 )
	{
		return nullptr;
	}

#ifdef NINTENDO
	// unbound controllers are rumbling on nintendo. stop that
	int player = -1;
	for (int c = 0; c < MAXPLAYERS; ++c) {
		if (inputs.getController(c) == this) {
			player = c;
			break;
		}
	}
	if (multiplayer == SINGLE) {
		if (player >= 0 && player < MAXPLAYERS) {
			if (client_disconnected[player]) {
				return nullptr;
			}
		}
	}
	else {
		if (player != clientnum) {
			return nullptr;
		}
	}
#endif

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
#ifdef NINTENDO
		/*constantly update for all patterns*/ )
#else
		&& (!rumbleToPlay->second.isPlaying 
			|| rumbleToPlay->second.pattern == Haptic_t::RUMBLE_BOULDER 
			|| rumbleToPlay->second.pattern == Haptic_t::RUMBLE_BOULDER_BOUNCE
			|| rumbleToPlay->second.pattern == Haptic_t::RUMBLE_DEATH
			|| rumbleToPlay->second.pattern == Haptic_t::RUMBLE_TMP))
#endif
	{
		Uint32 newStartTime = (haptics.hapticTick - rumbleToPlay->second.startTick);
		rumbleToPlay->second.startTime = newStartTime; // move the playhead forward.
		rumbleToPlay->second.isPlaying = true;
		return doRumble(&rumbleToPlay->second);
	}
	return nullptr;
}

void GameController::addRumble(Haptic_t::RumblePattern pattern, Uint16 smallMagnitude, Uint16 largeMagnitude, Uint32 length, Uint32 srcEntityUid)
{
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

		real_t durationMult = 1.0;
#ifdef NINTENDO
		durationMult = 0.5;
#endif // NINTENDO


		real_t percentHPLost = std::min(1.0, (stats[player]->OLDHP - stats[player]->HP) / static_cast<real_t>(std::max(1, stats[player]->MAXHP)));
		if ( stats[player]->HP <= 0 )
		{
			rumble(player, GameController::Haptic_t::RUMBLE_DEATH, 32000, 32000, durationMult * 2 * TICKS_PER_SECOND, 0);
		}
		else if ( stats[player]->OLDHP == stats[player]->HP )
		{
			rumble(player, GameController::Haptic_t::RUMBLE_NORMAL, 8000, 8000, durationMult * 6, 0);
		}
		else if ( percentHPLost < .05 )
		{
			rumble(player, GameController::Haptic_t::RUMBLE_NORMAL, 16000, 16000, durationMult * 6, 0);
		}
		else if ( percentHPLost < .25 )
		{
			rumble(player, GameController::Haptic_t::RUMBLE_NORMAL, 24000, 24000, durationMult * 11, 0);
		}
		else
		{
			rumble(player, GameController::Haptic_t::RUMBLE_NORMAL, 32000, 32000, durationMult * 11, 0);
		}
	}
}

const Uint32 Inputs::HAPTIC_SFX_BOULDER_BOUNCE_VOL = 1;
const Uint32 Inputs::HAPTIC_SFX_BOULDER_ROLL_LOW_VOL = 2;
const Uint32 Inputs::HAPTIC_SFX_BOULDER_ROLL_HIGH_VOL = 3;
const Uint32 Inputs::HAPTIC_SFX_BOULDER_LAUNCH_VOL = 4;

void Inputs::addRumbleRemotePlayer(const int player, Uint32 hapticType, Uint32 uid)
{
	if ( multiplayer != SERVER ) { return; }
	if ( player <= 0 || player >= MAXPLAYERS ) { return; }
	if ( players[player]->isLocalPlayer() ) 
	{
		return;
	}
	if ( !client_disconnected[player] )
	{
		strcpy((char*)net_packet->data, "BRRR");
		SDLNet_Write32(hapticType, &net_packet->data[4]);
		SDLNet_Write32(uid, &net_packet->data[8]);
		net_packet->address.host = net_clients[player - 1].host;
		net_packet->address.port = net_clients[player - 1].port;
		net_packet->len = 12;
		sendPacketSafe(net_sock, -1, net_packet, player - 1);
	}
}

void Inputs::addRumbleForHapticType(const int player, Uint32 hapticType, Uint32 uid)
{
	if ( hapticType == HAPTIC_SFX_BOULDER_BOUNCE_VOL )
	{
		inputs.rumble(player, GameController::Haptic_t::RUMBLE_BOULDER_BOUNCE, 32000, 32000, 15, uid);
	}
	else if ( hapticType == HAPTIC_SFX_BOULDER_ROLL_LOW_VOL )
	{
		inputs.rumble(player, GameController::Haptic_t::RUMBLE_BOULDER_ROLLING, 0, 8000, TICKS_PER_SECOND / 2, uid);
	}
	else if ( hapticType == HAPTIC_SFX_BOULDER_ROLL_HIGH_VOL )
	{
		inputs.rumble(player, GameController::Haptic_t::RUMBLE_BOULDER_ROLLING, 0, 16000, TICKS_PER_SECOND / 2, uid);
	}
	else if ( hapticType == HAPTIC_SFX_BOULDER_LAUNCH_VOL )
	{
		rumble(player, GameController::Haptic_t::RUMBLE_BOULDER, 0, 32000, TICKS_PER_SECOND, uid);
	}
}

GameController::Haptic_t::HapticEffect* GameController::doRumble(Haptic_t::Rumble* r)
{
#ifdef DISABLE_RUMBLE
	return nullptr;
#endif
	if ( !r )
	{
		return nullptr;
	}

	// init an effect.
	haptics.hapticEffect.type = SDL_HAPTIC_LEFTRIGHT;
	haptics.hapticEffect.leftRightBalance = 0;
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

				//if we want l/r vibration channels
				//real_t tangent = atan2(ent->y - players[i]->entity->y, ent->x - players[i]->entity->x);
				//real_t leftright = sin(tangent - players[i]->entity->yaw);

				//haptics.hapticEffect.leftRightBalance = leftright * 100;
			}
		}
	}

	if ( r->pattern == Haptic_t::RUMBLE_BOULDER_BOUNCE )
	{
		haptics.hapticEffect.large_magnitude = r->largeMagnitude * dampening;
		haptics.hapticEffect.small_magnitude = r->smallMagnitude * dampening;
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
		haptics.hapticEffect.large_magnitude = r->largeMagnitude * r->customEffect * dampening;
		haptics.hapticEffect.small_magnitude = r->smallMagnitude * r->customEffect * dampening;
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
		haptics.hapticEffect.large_magnitude = r->largeMagnitude * dampening;
		haptics.hapticEffect.small_magnitude = r->smallMagnitude * dampening;
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
		haptics.hapticEffect.large_magnitude = r->largeMagnitude * r->customEffect;
		haptics.hapticEffect.small_magnitude = r->smallMagnitude * r->customEffect;
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
		haptics.hapticEffect.large_magnitude = r->largeMagnitude * r->customEffect;
		haptics.hapticEffect.small_magnitude = r->smallMagnitude * r->customEffect;
	}
	else
	{
		haptics.hapticEffect.large_magnitude = r->largeMagnitude;
		haptics.hapticEffect.small_magnitude = r->smallMagnitude;
	}
	haptics.hapticEffect.length = ((r->length - r->startTime) * 1000 / TICKS_PER_SECOND); // convert to ms
	haptics.hapticEffect.type = r->pattern;
#ifdef NINTENDO
	return &haptics.hapticEffect;
#else
	if ( sdl_device )
	{
		SDL_GameControllerRumble(sdl_device, haptics.hapticEffect.large_magnitude * 2, haptics.hapticEffect.small_magnitude * 2,
			haptics.hapticEffect.length);
	}
	else if ( sdl_haptic )
	{
		// not in use
		/*if ( haptics.hapticEffectId == -1 )
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
			return nullptr;
		}
		if ( SDL_HapticRunEffect(sdl_haptic, haptics.hapticEffectId, 1) < 0 )
		{
			printlog("SDL_HapticUpdateEffect error: %s", SDL_GetError());
		}*/
	}
#endif
	return nullptr;
}
void GameController::stopRumble()
{
	if (!sdl_haptic) {
		return;
	}
	SDL_HapticStopEffect(sdl_haptic, haptics.hapticEffectId);
	haptics.hapticEffectId = -1;
}

Player::Player(int in_playernum, bool in_local_host) :
	GUI(*this),
	inventoryUI(*this),
	hud(*this),
	magic(*this),
	characterSheet(*this),
	skillSheet(*this),
	movement(*this),
	ghost(*this),
	messageZone(*this),
	worldUI(*this),
	hotbar(*this),
	bookGUI(*this),
	signGUI(*this),
	paperDoll(*this),
	minimap(*this),
	shopGUI(*this),
	compendiumProgress(*this),
	mechanics(*this)
{
	local_host = false;
	playernum = in_playernum;
	entity = nullptr;
	cam = &cameras[playernum];
}

Player::~Player()
{
	clearGUIPointers();
	if (entity)
	{
		delete entity;
	}
}

void Player::init() // for use on new/restart game, UI related
{
	hud.resetBars();
	hud.compactLayoutMode = HUD_t::COMPACT_LAYOUT_INVENTORY;
	inventoryUI.resetInventory();
	paperDoll.clear();
	minotaurWarning[playernum].deinit();
	levelUpAnimation[playernum].lvlUps.clear();
	skillUpAnimation[playernum].skillUps.clear();
	mechanics.itemDegradeRng.clear();
	mechanics.sustainedSpellMPUsed = 0;
}

void Player::cleanUpOnEntityRemoval()
{
	if ( isLocalPlayer() )
	{
		hud.reset();
		movement.reset();
		worldUI.reset();
	}
	mechanics.enemyRaisedBlockingAgainst.clear();
	selectedEntity[playernum] = nullptr;
	client_selected[playernum] = nullptr;
}

const bool Player::isLocalPlayer() const
{
	return ((splitscreen && bSplitscreen) || playernum == clientnum || (intro && multiplayer == SINGLE));
}
const bool Player::isLocalPlayerAlive() const
{
	return (isLocalPlayer() && entity && !client_disconnected[playernum]);
}

Entity* Player::getPlayerInteractEntity(const int playernum) 
{
	if ( playernum < 0 || playernum >= MAXPLAYERS )
	{
		return nullptr;
	}
	if ( !players[playernum] )
	{
		return nullptr;
	}
	return players[playernum]->ghost.isActive() ? players[playernum]->ghost.my : players[playernum]->entity;
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

bool monsterIsFriendlyForTooltip(const int player, Entity& entity)
{
	if ( !players[player]->entity )
	{
		return false;
	}
	if ( multiplayer != CLIENT )
	{
		if ( !entity.checkEnemy(players[player]->entity) )
		{
			return true;
		}
	}
	if ( entity.flags[USERFLAG2] )
	{
		return true;
	}
	if ( entity.monsterAllyGetPlayerLeader() == players[player]->entity )
	{
		return true; // this is my follower
	}

	Monster playerRace = stats[player]->type;
	Monster targetEntityType = entity.getMonsterTypeFromSprite();
	if ( targetEntityType == SHOPKEEPER )
	{
		if ( shopIsMysteriousShopkeeper(&entity) || !ShopkeeperPlayerHostility.isPlayerEnemy(player) )
		{
			return true;
		}
	}
	else if ( targetEntityType == SUCCUBUS || targetEntityType == INCUBUS )
	{
		if ( stats[player]->mask && stats[player]->mask->type == MASK_MOUTH_ROSE
			&& !(playerRace == RAT || playerRace == SPIDER
				|| playerRace == TROLL || playerRace == CREATURE_IMP) )
		{
			return true;
		}
	}
	if ( targetEntityType != NOTHING )
	{
		std::map<Monster, std::vector<Monster>>* allyTable = &Player::SkillSheet_t::skillSheetData.leadershipAllyTableBase;
		int loops = 1;
		if ( skillCapstoneUnlocked(player, PRO_LEADERSHIP) )
		{
			loops = 2;
		}
		while ( loops > 0 )
		{
			if ( allyTable->find(playerRace) != allyTable->end() )
			{
				if ( !(*allyTable)[playerRace].empty() )
				{
					for ( auto& ally : (*allyTable)[playerRace] )
					{
						if ( ally < 0 || ally >= NUMMONSTERS ) { continue; }
						if ( ally == targetEntityType )
						{
							return true;
						}
					}
				}
			}
			--loops;
			if ( loops == 1 )
			{
				allyTable = &Player::SkillSheet_t::skillSheetData.leadershipAllyTableLegendary;
			}
		}
		// unique recruits
		{
			auto* allyTable = &Player::SkillSheet_t::skillSheetData.leadershipAllyTableSpecialRecruitment;
			if ( allyTable->find(playerRace) != allyTable->end() )
			{
				if ( !(*allyTable)[playerRace].empty() )
				{
					for ( auto& allyPair : (*allyTable)[playerRace] )
					{
						auto& ally = allyPair.first;
						if ( ally < 0 || ally >= NUMMONSTERS ) { continue; }
						if ( ally == targetEntityType )
						{
							return true;
						}
					}
				}
			}
		}
	}

	return false;
}

real_t Player::WorldUI_t::tooltipInRange(Entity& tooltip)
{
	if ( !players[player.playernum]->isLocalPlayerAlive() && !player.ghost.isActive() )
	{
		return 0.0;
	}

	Entity* playerEntity = Player::getPlayerInteractEntity(player.playernum);
	if ( !playerEntity )
	{
		return 0.0;
	}

	real_t dist = entityDist(&tooltip, playerEntity);
	Entity* parent = uidToEntity(tooltip.parent);

	real_t maxDist = 24.0;
	real_t minDist = 4.0;

	bool selectInteract = false;
	bool callout = false;
	if ( FollowerMenu[player.playernum].followerMenuIsOpen() && FollowerMenu[player.playernum].selectMoveTo )
	{
		selectInteract = (FollowerMenu[player.playernum].optionSelected == ALLY_CMD_ATTACK_SELECT);
		maxDist = 256;
	}
	else if ( CalloutMenu[player.playernum].calloutMenuIsOpen() && CalloutMenu[player.playernum].selectMoveTo )
	{
		selectInteract = (CalloutMenu[player.playernum].optionSelected == CalloutRadialMenu::CALLOUT_CMD_SELECT);
		callout = true;
		maxDist = 256;
	}
	else if ( parent 
		&& (parent->getMonsterTypeFromSprite() == SHOPKEEPER 
			|| (parent->behavior == &actFloorDecoration && parent->sprite == 991 /* sign */)
			|| (parent->behavior == &actMonster && (monsterIsFriendlyForTooltip(player.playernum, *parent)))
			|| (parent->monsterAllyGetPlayerLeader()
				&& parent->monsterAllyGetPlayerLeader() == players[player.playernum]->entity)) )
	{
		maxDist = TOUCHRANGE;
	}
	if ( parent && parent->behavior == &actDoor && parent->doorStatus == 0 ) // min dist 0.0 when door closed, just in case we're stuck inside.
	{
		minDist = 0.0;
	}

	if ( dist < maxDist && dist > minDist )
	{
		real_t tangent = atan2(tooltip.y - playerEntity->y, tooltip.x - playerEntity->x);
		while ( tangent >= 2 * PI )
		{
			tangent -= 2 * PI;
		}
		while ( tangent < 0 )
		{
			tangent += 2 * PI;
		}
		real_t playerYaw = playerEntity->yaw;
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
			if ( selectInteract )
			{
				if ( FollowerMenu[player.playernum].followerMenuIsOpen() && FollowerMenu[player.playernum].selectMoveTo )
				{
					if ( !FollowerMenu[player.playernum].allowedInteractEntity(*parent, false) )
					{
						return 0.0;
					}
				}
				else if ( CalloutMenu[player.playernum].calloutMenuIsOpen() && CalloutMenu[player.playernum].selectMoveTo )
				{
					if ( !CalloutMenu[player.playernum].allowedInteractEntity(*parent, false) )
					{
						return 0.0;
					}
				}

				if ( parent->behavior == &actPlayer && player.playernum == parent->skill[2] )
				{
					// can't select self
					return 0.0;
				}

				Entity* ohitentity = hit.entity;
				real_t tangent2 = atan2(playerEntity->y - parent->y, playerEntity->x - parent->x);
				bool oldPassable = playerEntity->flags[PASSABLE];
				if ( playerEntity->behavior == &actDeathGhost )
				{
					playerEntity->flags[PASSABLE] = false; // hack to make ghosts linetraceable
				}
				lineTraceTarget(parent, parent->x, parent->y, tangent2, maxDist, 0, false, playerEntity);
				playerEntity->flags[PASSABLE] = oldPassable;
				if ( hit.entity != playerEntity )
				{
					// no line of sight through walls
					hit.entity = ohitentity;
					return 0.0;
				}
				hit.entity = ohitentity;
			}
			else if ( parent->behavior == &actGoldBag && achievementPenniless )
			{
				return 0.0;
			}
			else if ( parent->behavior == &actBoulderTrapHole )
			{
				return 0.0;
			}
			else if ( parent->behavior == &actBell && parent->skill[7] > 0 )
			{
				return 0.0;
			}
			else if ( parent->behavior == &actTeleportShrine && parent->shrineActivateDelay > 0 )
			{
				return 0.0;
			}
			else if ( parent->behavior == &actDaedalusShrine && parent->shrineActivateDelay > 0 )
			{
				return 0.0;
			}
			else if ( parent->behavior == &actPlayer )
			{
				return 0.0;
			}
			else if ( player.ghost.isActive() && !player.ghost.allowedInteractEntity(*parent) )
			{
				return 0.0;
			}

			if ( parent->behavior == &actColliderDecoration && !callout )
			{
				return 0.0;
			}

			if ( !selectInteract && stats[player.playernum] && stats[player.playernum]->defending && player.entity )
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
			if ( parent->getMonsterTypeFromSprite() == SHOPKEEPER && !selectInteract )
			{
				if ( !shopIsMysteriousShopkeeper(parent) && ShopkeeperPlayerHostility.isPlayerEnemy(player.playernum) )
				{
					return 0.0;
				}
			}
			if ( parent->getMonsterTypeFromSprite() == BAT_SMALL && !selectInteract )
			{
				return 0.0;
			}
			if ( parent->behavior == &actPlayer 
				|| (parent->behavior == &actMonster && !(parent->isInertMimic())) )
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
			else if ( parent->behavior == &actItem && parent->sprite == items[GEM_ROCK].index )
			{
				dist += 8.0; // distance penalty for rocks from digging etc
			}
			else if ( parent->behavior == &actBell )
			{
				dist += 1.0; // distance penalty
			}
			else if ( parent->behavior == &actGoldBag )
			{
				dist = std::max(0.02, dist - 4.0); // bonus priority for goldbag
			}
			else if ( (parent->behavior == &actTorch || parent->behavior == &actCrystalShard) )
			{
				if ( callout )
				{
					dist += 8.0; // distance penalty when calling out
				}
			}

			if ( selectInteract )
			{
				if ( parent->behavior == &actMonster 
					&& !(parent->isInertMimic())
					&& ((multiplayer != CLIENT && parent->checkEnemy(player.entity)) 
						|| (multiplayer == CLIENT 
							&& !parent->monsterAllyGetPlayerLeader() && !monsterally[parent->getMonsterTypeFromSprite()][stats[player.playernum]->type])) )
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
			if ( !selectInteract )
			{
				Entity* ohitentity = hit.entity;
				real_t tangent2 = atan2(playerEntity->y - tooltip.y, playerEntity->x - tooltip.x);
				bool oldPassable = playerEntity->flags[PASSABLE];
				if ( playerEntity->behavior == &actDeathGhost )
				{
					playerEntity->flags[PASSABLE] = false; // hack to make ghosts linetraceable
				}
				lineTraceTarget(&tooltip, tooltip.x, tooltip.y, tangent2, maxDist, 0, false, playerEntity);
				playerEntity->flags[PASSABLE] = oldPassable;
				if ( hit.entity != playerEntity )
				{
					// no line of sight through walls
					hit.entity = ohitentity;
					return 0.0;
				}
				hit.entity = ohitentity;
				return dist;
			}

			if ( selectInteract )
			{
				// perform head pitch check
				if ( false )
				{
					// old method, not entirely accurate
					real_t startx = playerEntity->x;
					real_t starty = playerEntity->y;
					real_t startz = -4;
					real_t pitch = playerEntity->pitch;
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
						startx += 0.5 * cos(playerEntity->yaw);
						starty += 0.5 * sin(playerEntity->yaw);
						index = (static_cast<int>(starty + 16 * sin(playerEntity->yaw)) >> 4) * MAPLAYERS + (static_cast<int>(startx + 16 * cos(playerEntity->yaw)) >> 4) * MAPLAYERS * map.height;
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
					real_t lookDist = sqrt(pow(previousx - playerEntity->x, 2) + pow(previousy - playerEntity->y, 2));
					if ( lookDist < dist )
					{
						if ( abs(dist - lookDist) > 8.0 )
						{
							return 0.0; // looking at a tile on the ground more than x units away from the tooltip
						}
					}

					/*Entity* particle = spawnMagicParticle(playerEntity);
					particle->sprite = 576;
					particle->x = previousx;
					particle->y = previousy;
					particle->z = 7.5;*/
				}
				else if ( parent && (parent->behavior == &actBoulderTrapHole || parent->isUntargetableBat()) )
				{
					// more accurate line of sight, look up
					real_t startx = cameras[player.playernum].x * 16.0;
					real_t starty = cameras[player.playernum].y * 16.0;
					real_t startz = cameras[player.playernum].z + (4.5 - cameras[player.playernum].z) / 2.0 + -2.5;
					real_t pitch = cameras[player.playernum].vang;
					if ( pitch < PI ) // looking down
					{
						return 0.0;
					}

					// draw line from the players height and direction until we hit the ground.
					real_t previousx = startx;
					real_t previousy = starty;
					int index = 0;
					const real_t yaw = cameras[player.playernum].ang;

					bool onCeilingLayer = parent->z > -11.0 && parent->z < -10;
					if ( parent->isUntargetableBat() )
					{
						onCeilingLayer = false;
					}
					real_t endz = onCeilingLayer ? -8.0 : -8.0 - 16.0;
					for ( ; startz > endz; startz -= abs((0.1) * tan(pitch)) )
					{
						startx += 0.1 * cos(yaw);
						starty += 0.1 * sin(yaw);
						const int index_x = static_cast<int>(startx) >> 4;
						const int index_y = static_cast<int>(starty) >> 4;
						index = (index_y)*MAPLAYERS + (index_x)*MAPLAYERS * map.height;
						if ( !map.tiles[(OBSTACLELAYER) + index] )
						{
							// store the last known good coordinate
							previousx = startx;// + 16 * cos(yaw);
							previousy = starty;// + 16 * sin(yaw);
						}
						if ( map.tiles[OBSTACLELAYER + index] )
						{
							break;
						}
						if ( map.tiles[(OBSTACLELAYER + 1) + index] && !onCeilingLayer )
						{
							break;
						}
					}

					static ConsoleVariable<bool> cvar_calloutboulderdebug("/calloutboulderdebug", false);
					if ( *cvar_calloutboulderdebug )
					{
						Entity* particle = spawnMagicParticle(playerEntity);
						particle->sprite = 942;
						particle->x = previousx;
						particle->y = previousy;
						particle->z = startz;
					}

					real_t lookDist = sqrt(pow(previousx - playerEntity->x, 2) + pow(previousy - playerEntity->y, 2));
					if ( abs(dist - lookDist) > 8.25 )
					{
						return 0.0; // looking at a tile on the ground more than x units away from the tooltip
					}
					else if ( abs(startz - endz) > 0.25 )
					{
						return 0.0; // if we're not fixated on the tile of the object return
					}
				}
				else
				{
					// more accurate line of sight
					real_t startx = cameras[player.playernum].x * 16.0;
					real_t starty = cameras[player.playernum].y * 16.0;
					real_t startz = cameras[player.playernum].z + (4.5 - cameras[player.playernum].z) / 2.0 + -2.5;
					real_t pitch = cameras[player.playernum].vang;
					if ( pitch < 0 || pitch > PI )
					{
						pitch = 0;
					}

					// draw line from the players height and direction until we hit the ground.
					real_t previousx = startx;
					real_t previousy = starty;
					int index = 0;
					const real_t yaw = cameras[player.playernum].ang;
					for ( ; startz < 7.5; startz += abs((0.1) * tan(pitch)) )
					{
						startx += 0.1 * cos(yaw);
						starty += 0.1 * sin(yaw);
						const int index_x = static_cast<int>(startx) >> 4;
						const int index_y = static_cast<int>(starty) >> 4;
						index = (index_y)* MAPLAYERS + (index_x)* MAPLAYERS * map.height;
						if ( !map.tiles[OBSTACLELAYER + index] )
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

					real_t lookDist = sqrt(pow(previousx - playerEntity->x, 2) + pow(previousy - playerEntity->y, 2));
					if ( callout )
					{
						if ( parent )
						{
							if ( cameras[player.playernum].vang > PI ) // looking above horizon
							{
								if ( parent->behavior == &actItem || parent->behavior == &actGoldBag
									|| parent->behavior == &actLadder
									|| parent->behavior == &actBeartrap
									|| parent->behavior == &actBomb
									|| parent->behavior == &actSwitch || parent->behavior == &actSwitchWithTimer )
								{
									return 0.0;
								}
							}
						}
						if ( (lookDist < dist) )
						{
							if ( abs(dist - lookDist) > 16.0 )
							{
								return 0.0; // looking at a tile on the ground more than x units away from the tooltip
							}
						}
						else if ( parent && ((parent->behavior == &actItem && parent->z > 4) || parent->behavior == &actGoldBag
							|| parent->behavior == &actLadder
							|| parent->behavior == &actBeartrap
							|| parent->behavior == &actBomb
							|| parent->behavior == &actSwitch || parent->behavior == &actSwitchWithTimer) )
						{
							if ( dist < 32.0 && abs(dist - lookDist) > 16.0
								|| dist >= 32.0 && abs(dist - lookDist) > 24.0 )
							{
								return 0.0; // looking at a tile on the ground more than x units away from the tooltip
							}
						}
					}
					else if ( lookDist < dist )
					{
						if ( abs(dist - lookDist) > 24.0 )
						{
							return 0.0; // looking at a tile on the ground more than x units away from the tooltip
						}
					}

					/*Entity* particle = spawnMagicParticle(playerEntity);
					particle->sprite = 942;
					particle->x = previousx;
					particle->y = previousy;
					particle->z = 7.5;*/
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
		else if ( parent->behavior == &actColliderDecoration )
		{
			if ( CalloutMenu[player.playernum].calloutMenuIsOpen()
				&& CalloutMenu[player.playernum].selectMoveTo
				&& CalloutMenu[player.playernum].optionSelected == CalloutRadialMenu::CALLOUT_CMD_SELECT )
			{
				// allowed in callout mode, otherwise no
			}
			else
			{
				setTooltipDisabled(tooltip);
				return;
			}
		}
		else if ( player.ghost.isActive() && !CalloutMenu[player.playernum].calloutMenuIsOpen() )
		{
			if ( !player.ghost.allowedInteractEntity(*parent) )
			{
				setTooltipDisabled(tooltip);
				return;
			}
		}
		parent->highlightForUI = 1.0;

		if ( tooltip.worldTooltipRequiresButtonHeld == 1 && *MainMenu::cvar_hold_to_activate )
		{
			interactText = Language::get(3998); // "(Hold) ";
		}
		else
		{
			interactText = "";
		}

		bool foundTinkeringKit = false;
		if ( stats[player.playernum] && stats[player.playernum]->defending && player.entity )
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
		else if ( CalloutMenu[player.playernum].calloutMenuIsOpen()
			&& CalloutMenu[player.playernum].selectMoveTo
			&& CalloutMenu[player.playernum].optionSelected == CalloutRadialMenu::CALLOUT_CMD_SELECT )
		{
			CalloutMenu[player.playernum].allowedInteractEntity(*parent, true);
		}
		else if ( parent->behavior == &actItem )
		{
			if ( foundTinkeringKit )
			{
				interactText = Language::get(3999); // "Salvage item";
			}
			else
			{
				if ( player.ghost.isActive() )
				{
					interactText = Language::get(6045); // "Nudge item";
				}
				else
				{
					interactText = Language::get(4000); // "Pick up item";
				}
			}
		}
		else if ( parent->behavior == &actGoldBag )
		{
			interactText = Language::get(4001); // "Take gold";
		}
		else if ( parent->behavior == &actFountain )
		{
			interactText = Language::get(4002); // "Drink from fountain" 
		}
		else if ( parent->behavior == &actSink )
		{
			interactText = Language::get(4003); // "Drink from sink" 
		}
		else if ( parent->behavior == &actChestLid || parent->behavior == &actChest )
		{
			if ( parent->skill[1] == 1 )
			{
				interactText = Language::get(4004); // "Close chest" 
			}
			else if ( parent->skill[1] == 0 )
			{
				interactText = Language::get(4005); // "Open chest" 
			}
		}
		else if ( parent->behavior == &actTorch )
		{
			if ( foundTinkeringKit )
			{
				interactText = Language::get(4006); // "Salvage torch" 
			}
			else
			{
				interactText = Language::get(4007); // "Take torch" 
			}
		}
		else if ( parent->behavior == &actCrystalShard )
		{
			if ( foundTinkeringKit )
			{
				interactText = Language::get(4008); // "Salvage shard" 
			}
			else
			{
				interactText = Language::get(4009); // "Take shard" 
			}
		}
		else if ( parent->behavior == &actHeadstone )
		{
			interactText = Language::get(4010); // "Inspect gravestone" 
		}
		else if ( parent->behavior == &actMonster )
		{
			int monsterType = parent->getMonsterTypeFromSprite();
			if ( parent->isInertMimic() )
			{
				interactText = Language::get(4005); // "Open chest" 
			}
			else
			{
				std::string name = Language::get(4011); // "follower" 
				if ( parent->getStats() && strcmp(parent->getStats()->name, "") )
				{
					name = parent->getStats()->name;
				}
				else
				{
					name = getMonsterLocalizedName((Monster)monsterType).c_str();
				}

				if ( parent->monsterAllyGetPlayerLeader() 
					&& parent->monsterAllyGetPlayerLeader() == players[player.playernum]->entity )
				{
					if ( parent->monsterIsTinkeringCreation() )
					{
						interactText = Language::get(4012) + name; // "Command "
					}
					else
					{
						interactText = Language::get(4012) + name; // "Command "
					}
				}
				else if ( parent->getMonsterTypeFromSprite() == SHOPKEEPER )
				{
					interactText = Language::get(4013) + name; // "Trade with "
				}
				else
				{
					interactText = Language::get(4014) + name; // "Interact with "
				}
			}
		}
		else if ( parent->behavior == &actDoor )
		{
			if ( parent->doorStatus != 0 )
			{
				interactText = Language::get(4015); // "Close door" 
			}
			else
			{
				interactText = Language::get(4016); // "Open door" 
			}
		}
		else if ( parent->behavior == &actGate )
		{
			interactText = Language::get(4017); // "Inspect gate" 
		}
		else if ( parent->behavior == &actSwitch || parent->behavior == &actSwitchWithTimer )
		{
			if ( parent->skill[0] == 1 )
			{
				interactText = Language::get(4018); // "Deactivate switch" 
			}
			else
			{
				interactText = Language::get(4019); // "Activate switch" 
			}
		}
		else if ( parent->behavior == &actPowerCrystal )
		{
			interactText = Language::get(4020); // "Turn crystal" 
		}
		else if ( parent->behavior == &actBoulder )
		{
			interactText = Language::get(4021); // "Push boulder" 
		}
		else if ( parent->behavior == &actPedestalBase )
		{
			if ( parent->pedestalHasOrb > 0 )
			{
				interactText = Language::get(4022); // "Take orb" 
			}
			else
			{
				interactText = Language::get(4023); // "Inspect" 
			}
		}
		else if ( parent->behavior == &actCampfire )
		{
			interactText = Language::get(4024); // "Pull torch" 
		}
		else if ( parent->behavior == &actFurniture || parent->behavior == &actMCaxe )
		{
			interactText = Language::get(4023); // "Inspect" 
		}
		else if ( parent->behavior == &actFloorDecoration )
		{
			if ( parent->sprite == 991 ) // sign
			{
				interactText = Language::get(4025); // "Read sign" 
			}
			else
			{
				interactText = Language::get(4023); // "Inspect" 
			}
		}
		else if ( parent->behavior == &actBeartrap )
		{
			interactText = Language::get(4026); // "Disarm beartrap" 
		}
		else if ( parent->behavior == &actLadderUp )
		{
			interactText = Language::get(4027); // "Inspect trapdoor" 
		}
		else if ( parent->behavior == &actBell )
		{
			interactText = Language::get(6271);

		}
		else if ( parent->behavior == &actLadder )
		{
			if ( secretlevel && parent->skill[3] == 1 ) // secret ladder
			{
				interactText += Language::get(4028); // "Exit secret level" 
			}
			else if ( !secretlevel && parent->skill[3] == 1 ) // secret ladder
			{
				interactText += Language::get(4029); // "Enter secret level" 
			}
			else
			{
				interactText += Language::get(4030); // "Exit dungeon floor" 
			}
		}
		else if ( parent->behavior == &actPortal )
		{
			if ( parent->skill[3] == 0 ) // secret entrance portal
			{
				if ( secretlevel )
				{
					interactText += Language::get(4028); // "Exit secret level" 
				}
				else
				{
					interactText += Language::get(4029); // "Enter secret level" 
				}
			}
			else
			{
				if ( !strcmp(map.name, "Hell") )
				{
					interactText += Language::get(4030); // hell uses portals instead "Exit dungeon floor"
				}
				else if ( !strcmp(map.name, "Mages Guild") )
				{
					interactText += Language::get(4031); // mages guild exit to castle "Exit Hamlet"
				}
				else
				{
					interactText += Language::get(4030); // "Exit dungeon floor";
				}
			}
		}
		else if ( parent->behavior == &::actMidGamePortal )
		{
			interactText += Language::get(4032); // "Step through portal";
		}
		else if ( parent->behavior == &actCustomPortal )
		{
			if ( gameModeManager.getMode() == GameModeManager_t::GAME_MODE_TUTORIAL )
			{
				if ( !strcmp(map.name, "Tutorial Hub") )
				{
					interactText += Language::get(4033); // "Enter trial";
				}
				else
				{
					interactText += Language::get(4034); // "Exit trial";
				}
			}
			else
			{
				if ( parent->portalCustomSpriteAnimationFrames > 0 )
				{
					interactText += Language::get(4035); // "Enter portal";
				}
				else
				{
					interactText += Language::get(4036); // "Enter trapdoor";
				}
			}
		}
		else if ( parent->behavior == &::actExpansionEndGamePortal
			|| parent->behavior == &actWinningPortal )
		{
			interactText += Language::get(4032); // "Step through portal";
		}
		else if ( parent->behavior == &actTeleporter )
		{
			if ( parent->teleporterType == 2 ) // portal
			{
				interactText += Language::get(4035); // "Enter portal";
			}
			else if ( parent->teleporterType == 1 ) // down ladder
			{
				interactText += Language::get(4037); // "Descend ladder";
			}
			else if ( parent->teleporterType == 0 ) // up ladder
			{
				interactText += Language::get(4038); // "Climb ladder";
			}
		}
		else if ( parent->behavior == &::actTeleportShrine /*|| parent->behavior == &::actSpellShrine*/ )
		{
			interactText += Language::get(4299); // "Touch shrine";
		}
		else if ( parent->behavior == &::actDaedalusShrine )
		{
			interactText += Language::get(6262); // "Touch shrine";
		}
		else if ( parent->behavior == &::actAssistShrine )
		{
			interactText += Language::get(6354); // "use shrine";
		}
		else if ( parent->behavior == &actBomb && parent->skill[21] != 0 ) //skill[21] item type
		{
			interactText = Language::get(4039); // "Disarm ";
			if ( parent->skill[21] >= WOODEN_SHIELD && parent->skill[21] < NUMITEMS )
			{
				interactText += items[parent->skill[21]].getIdentifiedName();
			}
		}
		else
		{
			interactText = Language::get(4040); // "Interact";
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
	else if ( entity.behavior == &actStatue )
	{
		return false;
	}
	else if ( entity.behavior == &actTeleportShrine /*|| entity.behavior == &::actSpellShrine*/ )
	{
		return false;
	}
	else if ( entity.behavior == &::actDaedalusShrine )
	{
		return false;
	}
	else if ( entity.behavior == &::actAssistShrine )
	{
		return false;
	}
	else if ( entity.behavior == &actDoor || entity.behavior == &actFountain || entity.behavior == &actSink
		|| entity.behavior == &actHeadstone || entity.behavior == &actChest || entity.behavior == &actChestLid
		|| entity.behavior == &actBoulder || entity.behavior == &actPlayer || entity.behavior == &actPedestalOrb || entity.behavior == &actPowerCrystalBase
		|| entity.behavior == &actPowerCrystal
		|| entity.behavior == &actColliderDecoration )
	{
		return false;
	}
	else if ( entity.behavior == &actMonster )
	{
		if ( players[player]->ghost.isActive() )
		{
			return false;
		}
		if ( entity.isInertMimic() )
		{
			return false;
		}
		if ( monsterIsFriendlyForTooltip(player, entity) )
		{
			return false;
		}
	}

	return true;
}

void Player::WorldUI_t::handleTooltips()
{
	//DebugTimers.addTimePoint("tooltip", "tooltip start");
	for ( int player = 0; player < MAXPLAYERS && !gamePaused; ++player )
	{
		players[player]->worldUI.worldTooltipDialogue.update();
		if ( !players[player]->isLocalPlayer()
			|| (!players[player]->isLocalPlayerAlive() && !players[player]->ghost.isActive()) )
		{
			players[player]->worldUI.reset();
			players[player]->worldUI.tooltipView = Player::WorldUI_t::TooltipView::TOOLTIP_VIEW_RESCAN;
			continue;
		}
		if ( players[player]->entity && players[player]->entity->ticks < TICKS_PER_SECOND / 2 )
		{
			players[player]->worldUI.reset();
			Input::inputs[player].consumeBinaryToggle("Use");
			continue;
		}

		Entity* playerEntity = Player::getPlayerInteractEntity(player);

#ifdef NINTENDO
		players[player]->worldUI.bEnabled = true;
#else
		if ( inputs.hasController(player) )
		{
			players[player]->worldUI.bEnabled = true;
		}
		else if ( inputs.bPlayerUsingKeyboardControl(player) )
		{
			if ( playerSettings[multiplayer ? 0 : player].mkb_world_tooltips_enabled )
			{
				if ( !players[player]->worldUI.bEnabled )
				{
					players[player]->worldUI.enable();
				}
			}
			else
			{
				if ( players[player]->worldUI.bEnabled )
				{
					players[player]->worldUI.disable();
				}
			}
		}
#endif // NINTENDO

		if ( !players[player]->worldUI.bEnabled )
		{
			continue;
		}

		bool foundTinkeringKit = false;
		bool radialMenuOpen = FollowerMenu[player].followerMenuIsOpen();
		bool selectInteract = false;
		if ( radialMenuOpen )
		{
			// follower menu can be "open" but selectMoveTo == true means the GUI is closed and selecting move or interact.
			if ( FollowerMenu[player].selectMoveTo == false )
			{
				radialMenuOpen = true;
			}
			radialMenuOpen = false;
			selectInteract = (FollowerMenu[player].optionSelected == ALLY_CMD_ATTACK_SELECT);
		}
		else
		{
			radialMenuOpen = CalloutMenu[player].calloutMenuIsOpen();
			if ( radialMenuOpen )
			{
				// callout menu can be "open" but selectMoveTo == true means the GUI is closed and selecting interact.
				if ( CalloutMenu[player].selectMoveTo == false )
				{
					radialMenuOpen = true;
				}
				radialMenuOpen = false;
				selectInteract = (CalloutMenu[player].optionSelected == CalloutRadialMenu::CALLOUT_CMD_SELECT);
			}
		}

		bool bDoingActionHideTooltips = false;
		if ( players[player]->hotbar.useHotbarFaceMenu
			&& players[player]->hotbar.faceMenuButtonHeld != Player::Hotbar_t::GROUP_NONE && players[player]->entity )
		{
			bDoingActionHideTooltips = true;
		}
		else if ( !players[player]->shootmode || radialMenuOpen )
		{
			bDoingActionHideTooltips = true;
		}
		else if ( FollowerMenu[player].selectMoveTo && FollowerMenu[player].optionSelected == ALLY_CMD_MOVETO_SELECT && players[player]->entity )
		{
			bDoingActionHideTooltips = true;
		}
		else if ( (players[player]->hud.weapon && players[player]->hud.weapon->skill[0] != 0) && !selectInteract && players[player]->entity )
		{
			// hudweapon chop
			bDoingActionHideTooltips = true;
		}
		else if ( (players[player]->hud.bowFire || players[player]->hud.bowIsBeingDrawn) && !selectInteract && players[player]->entity )
		{
			bDoingActionHideTooltips = true;
		}
		else if ( (cast_animation[player].active || cast_animation[player].active_spellbook) && !selectInteract && players[player]->entity )
		{
			// spells
			bDoingActionHideTooltips = true;
		}
		else if ( stats[player] && stats[player]->defending && players[player]->entity )
		{
			if ( stats[player]->shield && stats[player]->shield->type == TOOL_TINKERING_KIT )
			{
				// don't ignore
				foundTinkeringKit = true;
			}
			else
			{
				bDoingActionHideTooltips = true;
				if ( selectInteract )
				{
					bDoingActionHideTooltips = false; // selecting follower target is OK if defending
				}
			}
		}
		
		if ( !bDoingActionHideTooltips )
		{
			Entity* ohitentity = hit.entity;
			lineTrace(playerEntity, playerEntity->x, playerEntity->y,
				playerEntity->yaw, STRIKERANGE, 0, true);
			if ( hit.entity )
			{
				if ( hit.entity->behavior == &actMonster && selectInteract
					&& !(hit.entity->isInertMimic()) )
				{
					// don't let hostile monsters get in the way of selection
				}
				else if ( entityBlocksTooltipInteraction(player, *hit.entity) )
				{
					bDoingActionHideTooltips = true;
				}
			}
			hit.entity = ohitentity;
		}

		bool cycleNext = Input::inputs[player].consumeBinaryToggle("Interact Tooltip Next");
		bool cyclePrev = Input::inputs[player].consumeBinaryToggle("Interact Tooltip Prev");
		if ( !bDoingActionHideTooltips && players[player]->worldUI.tooltipsInRange.size() > 1 )
		{
			if ( cyclePrev )
			{
				players[player]->worldUI.tooltipView = TOOLTIP_VIEW_LOCKED;
				players[player]->worldUI.cycleToPreviousTooltip();
				players[player]->hud.interactPrompt.cycleAnim = 0.0;
			}
			if ( cycleNext )
			{
				players[player]->worldUI.tooltipView = TOOLTIP_VIEW_LOCKED;
				players[player]->worldUI.cycleToNextTooltip();
				players[player]->hud.interactPrompt.cycleAnim = 0.0;
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
				if ( parent && parent->flags[INVISIBLE] 
					&& !(parent->behavior == &actMonster && 
						(parent->getMonsterTypeFromSprite() == DUMMYBOT || parent->getMonsterTypeFromSprite() == MIMIC || parent->getMonsterTypeFromSprite() == BAT_SMALL)) )
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
					if ( selectInteract && parent && closestTooltip && parent->behavior != &actMonster )
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
				players[player]->worldUI.playerLastYaw = playerEntity->yaw;
				players[player]->worldUI.playerLastPitch = players[player]->camera().vang;
				while ( players[player]->worldUI.playerLastYaw >= 4 * PI )
				{
					players[player]->worldUI.playerLastYaw -= 2 * PI;
				}
				while ( players[player]->worldUI.playerLastYaw < 2 * PI )
				{
					players[player]->worldUI.playerLastYaw += 2 * PI;
				}
				while ( players[player]->worldUI.playerLastPitch >= 4 * PI )
				{
					players[player]->worldUI.playerLastPitch -= 2 * PI;
				}
				while ( players[player]->worldUI.playerLastPitch < 2 * PI )
				{
					players[player]->worldUI.playerLastPitch += 2 * PI;
				}
				players[player]->worldUI.setTooltipActive(*closestTooltip);
			}
			std::sort(players[player]->worldUI.tooltipsInRange.begin(), players[player]->worldUI.tooltipsInRange.end(),
				[](const std::pair<Entity*, real_t>& lhs, const std::pair<Entity*, real_t>& rhs)
			{
				return lhs.second < rhs.second;
			}
			);
			if ( players[player]->worldUI.tooltipView == TOOLTIP_VIEW_RESCAN || players[player]->worldUI.tooltipView == TOOLTIP_VIEW_FREE )
			{
				players[player]->worldUI.tooltipView = TOOLTIP_VIEW_LOCKED;
			}
		}
		else if ( players[player]->worldUI.tooltipView == TOOLTIP_VIEW_LOCKED )
		{
			if ( players[player]->worldUI.tooltipsInRange.empty() )
			{
				players[player]->worldUI.tooltipView = TOOLTIP_VIEW_RESCAN;
				continue;
			}
			if ( bDoingActionHideTooltips )
			{
				players[player]->worldUI.tooltipView = TOOLTIP_VIEW_RESCAN;
				continue;
			}

			real_t currentYaw = playerEntity->yaw;
			while ( currentYaw >= 4 * PI )
			{
				currentYaw -= 2 * PI;
			}
			while ( currentYaw < 2 * PI )
			{
				currentYaw += 2 * PI;
			}
			real_t yawDiff = players[player]->worldUI.playerLastYaw - currentYaw;
			real_t currentPitch = players[player]->camera().vang;
			while ( currentPitch >= 4 * PI )
			{
				currentPitch -= 2 * PI;
			}
			while ( currentPitch < 2 * PI )
			{
				currentPitch += 2 * PI;
			}
			real_t pitchDiff = players[player]->worldUI.playerLastPitch - currentPitch;
			if ( inputs.hasController(player) )
			{
				real_t floatx = inputs.getController(player)->getLeftXPercent();
				real_t floaty = inputs.getController(player)->getLeftYPercent();
				real_t magnitude = sqrt(pow(floaty, 2) + pow(floatx, 2));
				if ( magnitude > 0.0 )
				{
					players[player]->worldUI.tooltipView = TOOLTIP_VIEW_FREE;
					continue;
				}
			}
			else
			{
				if ( Input::inputs[player].binary("Move Left")
					|| Input::inputs[player].binary("Move Right")
					|| Input::inputs[player].binary("Move Forward")
					|| Input::inputs[player].binary("Move Backward") )
				{
					players[player]->worldUI.tooltipView = TOOLTIP_VIEW_FREE;
					continue;
				}
			}
			if ( abs(yawDiff) > PI / 16 || abs(pitchDiff) > PI / 64 )
			{
				players[player]->worldUI.tooltipView = TOOLTIP_VIEW_RESCAN;
				continue;
			}
			if ( FollowerMenu[player].selectMoveTo && FollowerMenu[player].optionSelected == ALLY_CMD_MOVETO_SELECT )
			{
				// rescan constantly
				players[player]->worldUI.tooltipView = TOOLTIP_VIEW_RESCAN;
				continue;
			}

			std::array<const char*, 3> salvageStrings = { Language::get(3999), Language::get(4006), Language::get(4008) };
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
						continue;
					}
				}
			}
			if ( foundTinkeringKit && !foundSalvageString )
			{
				// rescan, out of date string.
				players[player]->worldUI.tooltipView = TOOLTIP_VIEW_RESCAN;
				continue;
			}

			std::array<const char*, 2> switchStrings = { Language::get(4018), Language::get(4019) };
			bool foundSwitchString = false;
			for ( auto s : switchStrings )
			{
				if ( players[player]->worldUI.interactText.find(s) != std::string::npos )
				{
					foundSwitchString = true;
					break;
				}
			}

			for ( auto& tooltip : players[player]->worldUI.tooltipsInRange )
			{
				if ( players[player]->worldUI.bTooltipActiveForPlayer(*tooltip.first) && foundSwitchString )
				{
					if ( Entity* parent = uidToEntity(tooltip.first->parent) )
					{
						if ( parent->behavior == &actSwitch || parent->behavior == &actSwitchWithTimer )
						{
							if ( parent->skill[0] == 1 )
							{
								if ( players[player]->worldUI.interactText.find(Language::get(4019)) != std::string::npos )
								{
									// rescan, out of date string.
									players[player]->worldUI.tooltipView = TOOLTIP_VIEW_RESCAN;
									break;
								}
							}
							else
							{
								if ( players[player]->worldUI.interactText.find(Language::get(4018)) != std::string::npos )
								{
									// rescan, out of date string.
									players[player]->worldUI.tooltipView = TOOLTIP_VIEW_RESCAN;
									break;
								}
							}
						}
					}
				}
				if ( players[player]->worldUI.tooltipInRange(*tooltip.first) < 0.01 )
				{
					players[player]->worldUI.tooltipView = TOOLTIP_VIEW_RESCAN;
					break;
				}
			}
		}
	}
	//DebugTimers.addTimePoint("tooltip", "tooltip end");
}

void Player::Hotbar_t::selectHotbarSlot(int slot)
{
	if ( slot < 0 )
	{
		slot = NUM_HOTBAR_SLOTS - 1;
	}
	if ( slot >= NUM_HOTBAR_SLOTS )
	{
		slot = 0;
	}
	current_hotbar = slot;
	player.GUI.activateModule(GUI_t::MODULE_HOTBAR);
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

const int Player::HUD_t::getActionIconForPlayer(ActionPrompts prompt, std::string& promptString) const
{
	if ( player.ghost.isActive() )
	{
		switch ( prompt )
		{
			case ACTION_PROMPT_MAGIC:
				promptString = Language::get(6047); // Haunt
				break;
			case ACTION_PROMPT_MAINHAND:
				promptString = Language::get(6046); // Chill
				break;
			case ACTION_PROMPT_OFFHAND:
				promptString = Language::get(6048); // Push
				break;
			case ACTION_PROMPT_SNEAK:
				promptString = Language::get(4077); // Sneak
				break;
			default:
				break;
		}
		return -1;
	}

	if ( prompt == ACTION_PROMPT_MAGIC ) 
	{ 
		promptString = Language::get(4078);
		return PRO_SPELLCASTING;
	}

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

	if ( prompt == ACTION_PROMPT_SNEAK )
	{
		promptString = Language::get(4077);
		return PRO_STEALTH;
	}
	else if ( prompt == ACTION_PROMPT_OFFHAND )
	{
		int skill = PRO_SHIELD;
		promptString = Language::get(4076);
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
					if ( player.bUseCompactGUIHeight() )
					{
						promptString = Language::get(4318);
					}
					else
					{
						promptString = Language::get(4079);
					}
					return PRO_MAGIC;
				}

				if ( shapeshifted || itemTypeIsQuiver(stats[player.playernum]->shield->type) )
				{
					allowDefending = false;
				}

				if ( allowDefending )
				{ 
					promptString = Language::get(4076);
					return PRO_SHIELD; 
				}
				promptString = Language::get(4077);
				return PRO_STEALTH;
			}
			else
			{
				skill = PRO_STEALTH;
				promptString = Language::get(4077);
			}
		}
		return skill;
	}
	else // prompt == ACTION_PROMPT_MAINHAND
	{
		int skill = PRO_UNARMED;
		promptString = Language::get(4075);
		if ( stats[player.playernum] )
		{
			if ( stats[player.playernum]->shield && stats[player.playernum]->shield->type == TOOL_TINKERING_KIT )
			{
				if ( !shapeshifted && stats[player.playernum]->defending )
				{
					promptString = Language::get(4081);
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
						promptString = Language::get(4083);
					}
				}
				else if ( !shapeshifted )
				{
					if ( itemCategory(stats[player.playernum]->weapon) == POTION )
					{
						skill = PRO_ALCHEMY;
						promptString = Language::get(4082);
					}
					else if ( itemCategory(stats[player.playernum]->weapon) == THROWN
						|| itemCategory(stats[player.playernum]->weapon) == GEM )
					{
						skill = PRO_RANGED;
						promptString = Language::get(4082);
					}
					else if ( itemCategory(stats[player.playernum]->weapon) == TOOL )
					{
						skill = PRO_LOCKPICKING;
						promptString = Language::get(4080);
						if ( stats[player.playernum]->weapon )
						{
							if ( stats[player.playernum]->weapon->type == TOOL_PICKAXE )
							{
								promptString = Language::get(4084);
							}
							else if ( stats[player.playernum]->weapon->type == TOOL_BEARTRAP )
							{
								promptString = Language::get(337);
							}
						}
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
		&& stats[player.playernum]->cloak->type == CLOAK_BACKPACK && stats[player.playernum]->cloak->status != BROKEN
		&& (shouldInvertEquipmentBeatitude(stats[player.playernum]) ? abs(stats[player.playernum]->cloak->beatitude) >= 0 : stats[player.playernum]->cloak->beatitude >= 0) )
	{
		y = DEFAULT_INVENTORY_SIZEY + 2;
	}
	return y;
}

bool Player::Inventory_t::warpMouseToSelectedItem(Item* snapToItem, Uint32 flags)
{
	if ( frame )
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
			player.paperDoll.getCoordinatesFromSlotType(slot, x, y);
		}

		if ( auto slot = getInventorySlotFrame(x, y) )
		{
			if ( !isInteractable )
			{
				//messagePlayer(0, "[Debug]: select item queued");
				cursor.queuedModule = Player::GUI_t::MODULE_INVENTORY;
				cursor.queuedFrameToWarpTo = slot;
				return false;
			}
			else
			{
				//messagePlayer(0, "[Debug]: select item warped");
				cursor.queuedModule = Player::GUI_t::MODULE_NONE;
				cursor.queuedFrameToWarpTo = nullptr;
				slot->warpMouseToFrame(player.playernum, flags);
			}
			return true;
		}
	}
	return false;
}

bool Player::Inventory_t::warpMouseToSelectedSpell(Item* snapToItem, Uint32 flags)
{
	if ( spellFrame )
	{
		int x = getSelectedSpellX();
		int y = getSelectedSpellY();
		if ( snapToItem )
		{
			x = snapToItem->x;
			y = snapToItem->y;
		}

		if ( spellPanel.isInteractable )
		{
			if ( abs(spellPanel.scrollAnimateX - spellPanel.scrollSetpoint) > 0.00001 )
			{
				int diff = (spellPanel.scrollAnimateX - spellPanel.scrollSetpoint) / getSlotSize();
				y += diff; // if we have a scroll in the works, then manipulate y to pretend where we'd be ahead of time.
			}
		}

		if ( auto slot = getSpellSlotFrame(x, y) )
		{
			if ( !spellPanel.isInteractable )
			{
				//messagePlayer(0, "[Debug]: select spell queued");
				cursor.queuedModule = Player::GUI_t::MODULE_SPELLS;
				cursor.queuedFrameToWarpTo = slot;
				return false;
			}
			else
			{
				//messagePlayer(0, "[Debug]: select spell warped");
				slot->warpMouseToFrame(player.playernum, flags);
				cursor.queuedModule = Player::GUI_t::MODULE_NONE;
				cursor.queuedFrameToWarpTo = nullptr;
			}
			return true;
		}
	}
	return false;
}

bool Player::Inventory_t::warpMouseToSelectedChestSlot(Item* snapToItem, Uint32 flags)
{
	if ( chestFrame )
	{
		int x = getSelectedChestX();
		int y = getSelectedChestY();
		if ( snapToItem )
		{
			x = snapToItem->x;
			y = snapToItem->y;
		}

		if ( chestGUI.isInteractable )
		{
			if ( abs(chestGUI.scrollAnimateX - chestGUI.scrollSetpoint) > 0.00001 )
			{
				int diff = (chestGUI.scrollAnimateX - chestGUI.scrollSetpoint) / getSlotSize();
				y += diff; // if we have a scroll in the works, then manipulate y to pretend where we'd be ahead of time.
			}
		}

		if ( auto slot = getChestSlotFrame(x, y) )
		{
			if ( !chestGUI.isInteractable )
			{
				//messagePlayer(0, "[Debug]: select spell queued");
				cursor.queuedModule = Player::GUI_t::MODULE_CHEST;
				cursor.queuedFrameToWarpTo = slot;
				return false;
			}
			else
			{
				//messagePlayer(0, "[Debug]: select spell warped");
				slot->warpMouseToFrame(player.playernum, flags);
				cursor.queuedModule = Player::GUI_t::MODULE_NONE;
				cursor.queuedFrameToWarpTo = nullptr;
			}
			return true;
		}
	}
	return false;
}

bool Player::ShopGUI_t::warpMouseToSelectedShopItem(Item* snapToItem, Uint32 flags)
{
	if ( shopFrame )
	{
		int x = getSelectedShopX();
		int y = getSelectedShopY();
		if ( snapToItem )
		{
			x = snapToItem->x;
			y = snapToItem->y;
		}

		if ( auto slot = getShopSlotFrame(x, y) )
		{
			if ( !isInteractable )
			{
				//messagePlayer(0, "[Debug]: select item queued");
				player.inventoryUI.cursor.queuedModule = Player::GUI_t::MODULE_SHOP;
				player.inventoryUI.cursor.queuedFrameToWarpTo = slot;
				return false;
			}
			else
			{
				//messagePlayer(0, "[Debug]: select item warped");
				player.inventoryUI.cursor.queuedModule = Player::GUI_t::MODULE_NONE;
				player.inventoryUI.cursor.queuedFrameToWarpTo = nullptr;
				slot->warpMouseToFrame(player.playernum, flags);
			}
			return true;
		}
	}
	return false;
}

Frame* Player::Inventory_t::getInventorySlotFrame(int x, int y) const
{
	//printlog("%d %d inv", x, y);
	if ( frame )
	{
		if ( (x >= 0 && y >= 0 && x < getSizeX() && y < getSizeY()) )
		{
			//assert(slotFrames.find(x + y * 100) != slotFrames.end());
			return slotFrames.at(x + y * 100);
		}
		else if ( x >= PaperDollColumns::DOLL_COLUMN_LEFT && x <= PaperDollColumns::DOLL_COLUMN_RIGHT
			&& y >= PaperDollRows::DOLL_ROW_1 && y <= PaperDollRows::DOLL_ROW_5 )
		{
			//assert(slotFrames.find(x + y * 100) != slotFrames.end());
			return slotFrames.at(x + y * 100);
		}
	}
	return nullptr;
}

Frame* Player::Inventory_t::getSpellSlotFrame(int x, int y) const
{
	//printlog("%d %d spell", x, y);
	if ( spellFrame )
	{
		if ( x >= 0 && y >= 0 && x < MAX_SPELLS_X && y < MAX_SPELLS_Y )
		{
			return spellSlotFrames.at(x + y * 100);
		}
	}
	return nullptr;
}

Frame* Player::Inventory_t::getChestSlotFrame(int x, int y) const
{
	if ( chestFrame )
	{
		//return chestSlotFrames.at(x + y * 100);

		int key = x + y * 100;
		auto find = chestSlotFrames.find(key);
		return find != chestSlotFrames.end() ? find->second : nullptr;
		//assert(chestSlotFrames.find(key) == chestSlotFrames.end());
	}
	return nullptr;
}

Frame* Player::ShopGUI_t::getShopSlotFrame(int x, int y) const
{
	if ( shopFrame )
	{
		//return shopSlotFrames.at(x + y * 100);

		int key = x + y * 100;
		auto find = shopSlotFrames.find(key);
		return find != shopSlotFrames.end() ? find->second : nullptr;
		//assert(shopSlotFrames.find(key) == shopSlotFrames.end());
	}
	return nullptr;
}


Frame* Player::Inventory_t::getItemSlotFrame(Item* item, int x, int y) const
{
	if ( item && GenericGUI[player.playernum].isNodeTinkeringCraftableItem(item->node) )
	{
		return GenericGUI[player.playernum].tinkerGUI.getTinkerSlotFrame(x, y);
	}
	else if ( item && player.shopGUI.isItemFromShop(item) )
	{
		return player.shopGUI.getShopSlotFrame(x, y);
	}
	else if ( item && isItemFromChest(item) )
	{
		return getChestSlotFrame(x, y);
	}
	else if ( item && itemCategory(item) == SPELL_CAT )
	{
		return getSpellSlotFrame(x, y);
	}
	else
	{
		return getInventorySlotFrame(x, y);
	}
	return nullptr;
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
	return;
}

void Player::Magic_t::setQuickCastSpellFromInventory(Item* item)
{
	if ( quick_cast_spell ) // spell already queued, ignore.
	{
		return;
	}
	quick_cast_spell = nullptr;
	if ( !item || itemCategory(item) != SPELL_CAT )
	{
		return;
	}
	quick_cast_spell = getSpellFromItem(player.playernum, item, true);
}

const bool Player::bUseCompactGUIWidth() const
{
	if ( splitscreen )
	{
		if ( camera_virtualWidth() < Frame::virtualScreenX * .75 )
		{
			return true;
		}
	}
	return false;
}
const bool Player::bUseCompactGUIHeight() const
{
	if ( splitscreen )
	{
		if ( camera_virtualHeight() < Frame::virtualScreenY * .75 )
		{
			return true;
		}
	}
	return false;
}

const bool Player::usingCommand() const
{
	if ( command )
	{
		return inputs.bPlayerUsingKeyboardControl(playernum);
	}
	return false;
}

const bool Player::bAlignGUINextToInventoryCompact() const
{
	if ( shopGUI.bOpen )
	{
		return false;
	}
	if ( bUseCompactGUIHeight() && !bUseCompactGUIWidth() )
	{
		return true;
	}
	return false;
}

std::vector<std::pair<std::string, std::string>> Player::Minimap_t::mapDetails;

void Inputs::setMouse(const int player, MouseInputs input, Sint32 value)
{
#ifdef NINTENDO
	if (fingerdown) {
		switch (input) {
		case X: fingerx = value; break;
		case Y: fingery = value; break;
		case OX: ofingerx = value; break;
		case OY: ofingery = value; break;
		}
	}
	if (hasController(player)) {
		switch (input) {
		case OX: getVirtualMouse(player)->ox = value; return;
		case OY: getVirtualMouse(player)->oy = value; return;
		case X: getVirtualMouse(player)->x = value; return;
		case Y: getVirtualMouse(player)->y = value; return;
		case XREL: getVirtualMouse(player)->xrel = value; return;
		case YREL: getVirtualMouse(player)->yrel = value; return;
		default: return;
		}
	}
#else
	// todo: add condition like getMouse()? && (!getVirtualMouse(player)->lastMovementFromController 
	// || (players[player]->shootmode && !gamePaused && !intro))
	if ( bPlayerUsingKeyboardControl(player) && (!getVirtualMouse(player)->lastMovementFromController
		|| (players[player]->shootmode && !gamePaused && !intro)) || intro )
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
#endif
}

const Sint32 Inputs::getMouse(const int player, MouseInputs input)
{
#ifdef NINTENDO
	if (fingerdown) {
		switch (input) {
		case OX: return fingerx;
		case OY: return fingery;
		case X: return ofingerx;
		case Y: return ofingery;
		}
	}
	else if (hasController(player)) {
		switch (input) {
		case OX: return getVirtualMouse(player)->ox;
		case OY: return getVirtualMouse(player)->oy;
		case X: return getVirtualMouse(player)->x;
		case Y: return getVirtualMouse(player)->y;
		case XREL: return getVirtualMouse(player)->xrel;
		case YREL: return getVirtualMouse(player)->yrel;
		default: return 0;
		}
	}
	return 0;
#else
	if ( bPlayerUsingKeyboardControl(player) &&
		(!getVirtualMouse(player)->lastMovementFromController || (players[player]->shootmode && !gamePaused && !intro) || gamePaused || intro) )
	{
		// add controller virtual mouse if applicable, only in shootmode
		// shootmode has no limits on rotation, but !shootmode is inventory

		bool combineMouseInputs = (players[player]->shootmode && hasController(player)) && !gamePaused && !intro;

		switch ( input )
		{
			case OX:
				return omousex + (combineMouseInputs ? getVirtualMouse(player)->ox : 0);
				//return omousex;
			case OY:
				return omousey + (combineMouseInputs ? getVirtualMouse(player)->oy : 0);
				//return omousey;
			case X:
				return mousex + (combineMouseInputs ? getVirtualMouse(player)->x : 0);
				//return mousex;
			case Y:
				return mousey + (combineMouseInputs ? getVirtualMouse(player)->y : 0);
				//return mousey;
			case XREL:
				return mousexrel + (combineMouseInputs ? getVirtualMouse(player)->xrel : 0);
				//return mousexrel;
			case YREL:
				return mouseyrel + (combineMouseInputs ? getVirtualMouse(player)->yrel : 0);
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
#endif
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
        int w, h, gw, gh;
        SDL_GetWindowSize(screen, &w, &h);
        SDL_GL_GetDrawableSize(screen, &gw, &gh);
		if ( !inputs.hasController(player) )
		{
			SDL_WarpMouseInWindow(screen, (x * w) / gw, (y * h) / gh); // this pushes to the SDL event queue
		}
		
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
	/*if ( !hasController(player) )
	{
		return nullptr;
	}*/

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

	if (fingerdown)
	{
		return true;
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
		if ( binding.type == Binding_t::CONTROLLER_BUTTON ) 
		{
#ifdef NINTENDO
			switch (binding.padButton) {
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_A: return nxGetControllerState(id, nxInput::ButtonB) == 1;
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_B: return nxGetControllerState(id, nxInput::ButtonA) == 1;
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_X: return nxGetControllerState(id, nxInput::ButtonY) == 1;
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_Y: return nxGetControllerState(id, nxInput::ButtonX) == 1;
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_UP: return nxGetControllerState(id, nxInput::ButtonUp) == 1;
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return nxGetControllerState(id, nxInput::ButtonRight) == 1;
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_DOWN: return nxGetControllerState(id, nxInput::ButtonDown) == 1;
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_LEFT: return nxGetControllerState(id, nxInput::ButtonLeft) == 1;
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_START: return nxGetControllerState(id, nxInput::ButtonPlus) == 1;
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_BACK: return nxGetControllerState(id, nxInput::ButtonMinus) == 1;
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_LEFTSTICK: return nxGetControllerState(id, nxInput::LeftStickClick) == 1;
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_RIGHTSTICK: return nxGetControllerState(id, nxInput::RightStickClick) == 1;
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return nxGetControllerState(id, nxInput::ButtonL) == 1;
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return nxGetControllerState(id, nxInput::ButtonR) == 1;
			default: return false;
			}
#else
			SDL_GameController* pad = sdl_device;
			return SDL_GameControllerGetButton(pad, binding.padButton) == 1;
#endif
		}
		else if ( binding.type == Binding_t::VIRTUAL_DPAD )
		{
			return binding.padVirtualDpad != DpadDirection::CENTERED;
		}
		else 
		{
#ifdef NINTENDO
			if (binding.padAxisNegative) {
				switch (binding.padAxis) {
				case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTX:
					return nxGetControllerState(id, nxInput::LeftStickX) < (INT16_MIN / 2);
				case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTY:
					return nxGetControllerState(id, nxInput::LeftStickY) < (INT16_MIN / 2);
				case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_RIGHTX:
					return nxGetControllerState(id, nxInput::RightStickX) < (INT16_MIN / 2);
				case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_RIGHTY:
					return nxGetControllerState(id, nxInput::RightStickY) < (INT16_MIN / 2);
				default:
					return false;
				}
			}
			else {
				switch (binding.padAxis) {
				case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTX:
					return nxGetControllerState(id, nxInput::LeftStickX) > (INT16_MAX / 2);
				case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTY:
					return nxGetControllerState(id, nxInput::LeftStickY) > (INT16_MAX / 2);
				case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_RIGHTX:
					return nxGetControllerState(id, nxInput::RightStickX) > (INT16_MAX / 2);
				case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_RIGHTY:
					return nxGetControllerState(id, nxInput::RightStickY) > (INT16_MAX / 2);
				case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_TRIGGERLEFT:
					return nxGetControllerState(id, nxInput::ButtonZL) == 1;
				case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
					return nxGetControllerState(id, nxInput::ButtonZR) == 1;
				default:
					return false;
				}
			}
#else
			SDL_GameController* pad = sdl_device;
			if ( binding.padAxisNegative ) 
			{
				return SDL_GameControllerGetAxis(pad, binding.padAxis) < -16384;
			}
			else 
			{
				return SDL_GameControllerGetAxis(pad, binding.padAxis) > 16384;
			}
#endif
		}
	}

	return false;
}

float GameController::analogOf(Binding_t& binding) 
{
	if ( binding.type == Binding_t::CONTROLLER_AXIS || binding.type == Binding_t::CONTROLLER_BUTTON ) 
	{
		if ( binding.type == Binding_t::CONTROLLER_BUTTON ) 
		{
#ifdef NINTENDO
			switch (binding.padButton) {
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_A: return (float)nxGetControllerState(id, nxInput::ButtonB);
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_B: return (float)nxGetControllerState(id, nxInput::ButtonA);
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_X: return (float)nxGetControllerState(id, nxInput::ButtonY);
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_Y: return (float)nxGetControllerState(id, nxInput::ButtonX);
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_UP: return (float)nxGetControllerState(id, nxInput::ButtonUp);
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return (float)nxGetControllerState(id, nxInput::ButtonRight);
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_DOWN: return (float)nxGetControllerState(id, nxInput::ButtonDown);
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_LEFT: return (float)nxGetControllerState(id, nxInput::ButtonLeft);
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_START: return (float)nxGetControllerState(id, nxInput::ButtonPlus);
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_BACK: return (float)nxGetControllerState(id, nxInput::ButtonMinus);
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_LEFTSTICK: return (float)nxGetControllerState(id, nxInput::LeftStickClick);
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_RIGHTSTICK: return (float)nxGetControllerState(id, nxInput::RightStickClick);
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return (float)nxGetControllerState(id, nxInput::ButtonL);
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return (float)nxGetControllerState(id, nxInput::ButtonR);
			default: return 0.f;
			}
#else
			SDL_GameController* pad = sdl_device;
			return SDL_GameControllerGetButton(pad, binding.padButton) ? 1.f : 0.f;
#endif
		}
		else 
		{
#ifdef NINTENDO
			float result = 0.f;
			if (binding.padAxisNegative) {
				switch (binding.padAxis) {
				case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTX:
					result = std::max((float)nxGetControllerState(id, nxInput::LeftStickX) / INT16_MIN, 0.f); break;
				case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTY:
					result = std::max((float)nxGetControllerState(id, nxInput::LeftStickY) / INT16_MIN, 0.f); break;
				case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_RIGHTX:
					result = std::max((float)nxGetControllerState(id, nxInput::RightStickX) / INT16_MIN, 0.f); break;
				case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_RIGHTY:
					result = std::max((float)nxGetControllerState(id, nxInput::RightStickY) / INT16_MIN, 0.f); break;
				}
			}
			else {
				switch (binding.padAxis) {
				case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTX:
					result = std::max((float)nxGetControllerState(id, nxInput::LeftStickX) / INT16_MAX, 0.f); break;
				case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTY:
					result = std::max((float)nxGetControllerState(id, nxInput::LeftStickY) / INT16_MAX, 0.f); break;
				case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_RIGHTX:
					result = std::max((float)nxGetControllerState(id, nxInput::RightStickX) / INT16_MAX, 0.f); break;
				case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_RIGHTY:
					result = std::max((float)nxGetControllerState(id, nxInput::RightStickY) / INT16_MAX, 0.f); break;
				case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_TRIGGERLEFT:
					result = nxGetControllerState(id, nxInput::ButtonZL); break;
				case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
					result = nxGetControllerState(id, nxInput::ButtonZR); break;
				}
			}
			return (fabs(result) > binding.deadzone) ? result : 0.f;
#else
			SDL_GameController* pad = sdl_device;
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
#endif
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

void Player::clearGUIPointers()
{
	closeAllGUIs(CloseGUIShootmode::CLOSEGUI_ENABLE_SHOOTMODE, CloseGUIIgnore::CLOSEGUI_CLOSE_ALL);

	GUI.dropdownMenu.dropdownBlockClickFrame = nullptr;
	GUI.dropdownMenu.dropdownFrame = nullptr;

	inventoryUI.tooltipContainerFrame = nullptr;
	inventoryUI.frame = nullptr;
	inventoryUI.tooltipFrame = nullptr;
	inventoryUI.titleOnlyTooltipFrame = nullptr;
	inventoryUI.interactFrame = nullptr;
	inventoryUI.interactBlockClickFrame = nullptr;
	inventoryUI.tooltipPromptFrame = nullptr;
	inventoryUI.selectedItemCursorFrame = nullptr;
	inventoryUI.spellFrame = nullptr;
	inventoryUI.chestFrame = nullptr;
	inventoryUI.miscTooltipFrame = nullptr;
	inventoryUI.itemTooltipDisplay.uid = 0;

	inventoryUI.slotFrames.clear();
	inventoryUI.spellSlotFrames.clear();
	inventoryUI.chestSlotFrames.clear();

	inventoryUI.cursor.queuedFrameToWarpTo = nullptr;

	shopGUI.shopFrame = nullptr;
	shopGUI.shopSlotFrames.clear();
	shopGUI.itemRequiresTitleReflow = true;

	bookGUI.bookFrame = nullptr;
	signGUI.signFrame = nullptr;

	characterSheet.sheetFrame = nullptr;

	skillSheet.skillFrame = nullptr;

    hud.controllerFrame = nullptr;
	hud.hudFrame = nullptr;
	hud.xpFrame = nullptr;
	hud.levelupFrame = nullptr;
	hud.skillupFrame = nullptr;
	hud.hpFrame = nullptr;
	hud.mpFrame = nullptr;
	hud.minimapFrame = nullptr;
	hud.gameTimerFrame = nullptr;
	hud.allyStatusFrame = nullptr;
	hud.minotaurFrame = nullptr;
	hud.minotaurSharedDisplay = nullptr;
	hud.minotaurDisplay = nullptr;
	hud.mapPromptFrame = nullptr;
	hud.allyFollowerFrame = nullptr;
	hud.allyFollowerTitleFrame = nullptr;
	hud.allyFollowerGlyphFrame = nullptr;
	hud.allyPlayerFrame = nullptr;
	hud.calloutPromptFrame = nullptr;
	hud.enemyBarFrame = nullptr;
	hud.enemyBarFrameHUD = nullptr;
	hud.actionPromptsFrame = nullptr;
	hud.worldTooltipFrame = nullptr;
	hud.statusEffectFocusedWindow = nullptr;
	hud.uiNavFrame = nullptr;
	hud.cursorFrame = nullptr;

	messageZone.chatFrame = nullptr;
	messageZone.logParentFrame = nullptr;
	messageZone.logWindow = nullptr;

	minimap.mapWindow = nullptr;
	minimap.mapParentFrame = nullptr;

	std::fill(hotbar.hotbarSlotFrames.begin(), hotbar.hotbarSlotFrames.end(), nullptr);
	hotbar.hotbarFrame = nullptr;

	auto& genericGUI = GenericGUI[playernum];
	genericGUI.tinkerGUI.tinkerFrame = nullptr;
	genericGUI.tinkerGUI.tinkerSlotFrames.clear();
	genericGUI.tinkerGUI.itemRequiresTitleReflow = true;
	genericGUI.alchemyGUI.alchFrame = nullptr;
	genericGUI.alchemyGUI.alchemySlotFrames.clear();
	genericGUI.alchemyGUI.itemRequiresTitleReflow = true;
	genericGUI.assistShrineGUI.assistShrineFrame = nullptr;
	genericGUI.assistShrineGUI.assistShrineSlotFrames.clear();
	genericGUI.featherGUI.featherFrame = nullptr;
	genericGUI.featherGUI.featherSlotFrames.clear();
	genericGUI.featherGUI.itemRequiresTitleReflow = true;
	genericGUI.itemfxGUI.itemEffectFrame = nullptr;
	genericGUI.itemfxGUI.itemRequiresTitleReflow = true;

	StatusEffectQueue[playernum].statusEffectFrame = nullptr;
	StatusEffectQueue[playernum].statusEffectTooltipFrame = nullptr;

	skillSheetEntryFrames[playernum].skillsFrame = nullptr;
	skillSheetEntryFrames[playernum].entryFrameLeft = nullptr;
	skillSheetEntryFrames[playernum].entryFrameRight = nullptr;
	skillSheetEntryFrames[playernum].skillDescFrame = nullptr;
	skillSheetEntryFrames[playernum].skillBgImgsFrame = nullptr;
	skillSheetEntryFrames[playernum].scrollAreaOuterFrame = nullptr;
	skillSheetEntryFrames[playernum].scrollArea = nullptr;
	for ( int i = 0; i < NUMPROFICIENCIES; ++i )
	{
		skillSheetEntryFrames[playernum].entryFrames[i] = nullptr;
	}
	for ( int i = 0; i < 10; ++i )
	{
		skillSheetEntryFrames[playernum].effectFrames[i] = nullptr;
	}
	skillSheetEntryFrames[playernum].legendFrame = nullptr;

	playerInventoryFrames[playernum].inventoryBgFrame = nullptr;
	playerInventoryFrames[playernum].selectedSlotFrame = nullptr;
	playerInventoryFrames[playernum].oldSelectedSlotFrame = nullptr;
	playerInventoryFrames[playernum].chestFrameSlots = nullptr;
	playerInventoryFrames[playernum].dollSlotsFrame = nullptr;
	playerInventoryFrames[playernum].invSlotsFrame = nullptr;
	playerInventoryFrames[playernum].backpackFrame = nullptr;
	playerInventoryFrames[playernum].flourishFrame = nullptr;
	playerInventoryFrames[playernum].characterPreview = nullptr;
	playerInventoryFrames[playernum].inventoryBaseImagesFrame = nullptr;
	playerInventoryFrames[playernum].backpackSlotsFrame = nullptr;
	playerInventoryFrames[playernum].chestBgFrame = nullptr;
	playerInventoryFrames[playernum].defaultInvImg = nullptr;
	playerInventoryFrames[playernum].compactInvImg = nullptr;
	playerInventoryFrames[playernum].compactCharImg = nullptr;
	playerInventoryFrames[playernum].oldSelectedSlotItemImg = nullptr;
	playerInventoryFrames[playernum].chestBaseImg = nullptr;
	playerInventoryFrames[playernum].spellBaseImg = nullptr;
	playerInventoryFrames[playernum].autosortFrame = nullptr;

	FollowerMenu[playernum].followerFrame = nullptr;
	CalloutMenu[playernum].calloutFrame = nullptr;
	CalloutMenu[playernum].calloutPingFrame = nullptr;

	players[playernum]->inventoryUI.itemTooltipDisplay.reset();
	players[playernum]->inventoryUI.compendiumItemTooltipDisplay.reset();
}

const char* Player::getAccountName() const {
    const char* unknown = "...";
    if (directConnect) {
	    switch (playernum) {
	    case 0: return "Player 1";
	    case 1: return "Player 2";
	    case 2: return "Player 3";
	    case 3: return "Player 4";
	    default: return unknown;
	    }
    } else {
		if (LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM) {
#ifdef STEAMWORKS
			if (isLocalPlayer()) {
				return SteamFriends()->GetPersonaName();
			} else {
				for (int remoteIDIndex = 0; remoteIDIndex < MAXPLAYERS; ++remoteIDIndex) {
					if (steamIDRemote[remoteIDIndex]) {
						const char* memberNumChar = SteamMatchmaking()->GetLobbyMemberData(
						    *static_cast<CSteamID*>(currentLobby),
						    *static_cast<CSteamID*>(steamIDRemote[remoteIDIndex]),
						    "clientnum");
						if (memberNumChar) {
							std::string str = memberNumChar;
							if (!str.empty()) {
								int memberNum = std::stoi(str);
								if (memberNum >= 0 && memberNum < MAXPLAYERS && memberNum == playernum) {
									return SteamFriends()->GetFriendPersonaName(
									    *static_cast<CSteamID*>(steamIDRemote[remoteIDIndex]));
								}
							}
						}
					}
				}
			}
#endif
		}
		else if (LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY) {
#if defined USE_EOS
			if (isLocalPlayer()) {
				return EOS.CurrentUserInfo.Name.c_str();
			} else {
				for (auto& player : EOS.CurrentLobbyData.playersInLobby) {
					if (player.clientNumber == playernum) {
						return player.name.c_str();
					}
				}
			}
#endif
		}
	}
    return unknown;
}

void Player::PlayerMechanics_t::onItemDegrade(Item* item)
{
	if ( !item )
	{
		return;
	}
	if ( item->type < 0 || item->type >= NUMITEMS )
	{
		return;
	}
	if ( itemCategory(item) == SPELLBOOK )
	{
		itemDegradeRng[item->type] = 0;
	}
}

bool Player::PlayerMechanics_t::itemDegradeRoll(Item* item, int* checkInterval)
{
	if ( !item )
	{
		return true;
	}
	// assuming just shields/spellbooks for now
	if ( item->type < 0 || item->type >= NUMITEMS )
	{
		return true;
	}

	auto& counter = itemDegradeRng[item->type];
	int interval = 0;
	if ( itemCategory(item) == SPELLBOOK )
	{
		// 10 max base interval
		interval = (1 + item->status) + stats[player.playernum]->getModifiedProficiency(PRO_SPELLCASTING) / 20;
		if ( item->beatitude < 0
			&& !intro && !shouldInvertEquipmentBeatitude(stats[player.playernum]) )
		{
			interval = 0;
		}
		else if ( item->beatitude > 0
			|| (item->beatitude < 0 
				&& !intro && shouldInvertEquipmentBeatitude(stats[player.playernum])) )
		{
			interval += std::min(abs(item->beatitude), 2);
		}
	}
	else
	{
		switch ( item->type )
		{
		case WOODEN_SHIELD:
			interval = 10;
			break;
		case BRONZE_SHIELD:
			interval = 20;
			break;
		case IRON_SHIELD:
			interval = 20;
			break;
		case STEEL_SHIELD:
			interval = 30;
			break;
		case STEEL_SHIELD_RESISTANCE:
			interval = 30;
			break;
		case CRYSTAL_SHIELD:
			interval = 20;
			break;
		default:
			break;
		}
		if ( item->beatitude < 0
			&& !intro && !shouldInvertEquipmentBeatitude(stats[player.playernum]) )
		{
			interval = 0;
		}
		else if ( item->beatitude > 0
			|| (item->beatitude < 0
				&& !intro && shouldInvertEquipmentBeatitude(stats[player.playernum])) )
		{
			interval += std::min(abs(item->beatitude), 5);
		}
	}

	if ( checkInterval )
	{
		*checkInterval = interval;
		return false;
	}

	//messagePlayer(0, MESSAGE_DEBUG, "counter: %d | interval: %d", counter, interval);

	if ( counter >= interval )
	{
		if ( itemCategory(item) == SPELLBOOK )
		{
			// dont decrement until degraded
		}
		else
		{
			counter = 0;
		}
		return true;
	}
	++counter;
	return false;
}

void Player::PlayerMechanics_t::sustainedSpellIncrementMP(int mpChange)
{
	sustainedSpellMPUsed += std::max(0, mpChange);
}

bool Player::PlayerMechanics_t::sustainedSpellLevelChance()
{
	int threshold = 10;
	if ( stats[player.playernum]->getProficiency(PRO_SPELLCASTING) < SKILL_LEVEL_BASIC )
	{
		threshold = 5;
	}
	if ( sustainedSpellMPUsed >= threshold )
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool Player::PlayerMechanics_t::allowedRaiseBlockingAgainstEntity(Entity& attacker)
{
	if ( attacker.behavior != &actMonster )
	{
		return false;
	}
	return enemyRaisedBlockingAgainst[attacker.getUID()] < 1;
}