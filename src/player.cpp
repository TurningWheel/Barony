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

#ifdef NINTENDO
#include "nintendo/baronynx.hpp"
#endif

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


std::array<GameController, MAX_GAME_CONTROLLERS> game_controllers;
Inputs inputs;

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
	id = -1;

	initBindings(); // clear status of all values

	oldLeftTrigger = 0;
	oldRightTrigger = 0;
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
		this->buttons[i].binary = 0.f;
		this->buttons[i].consumed = 0.f;
	}
	for ( int i = SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTX; i < SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_MAX; ++i )
	{
		this->axis[i].type = Binding_t::Bindtype_t::CONTROLLER_AXIS;
		this->axis[i].padAxis = static_cast<SDL_GameControllerAxis>(i);
		this->axis[i].analog = 0.f;
		this->axis[i].binary = 0.f;
		this->axis[i].consumed = 0.f;
	}
}

const bool GameController::isActive()
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
			SDL_WarpMouseInWindow(screen, std::max(0, std::min(xres, mousex + rightx)), std::max(0, std::min(yres, mousey + righty)));
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
			//axis[SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_TRIGGERLEFT].binary = 1;
			lastkeypressed = 299;
		}
	}
	else
	{
		oldLeftTrigger = 0;
		//axis[SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_TRIGGERLEFT].binary = 0;
	}

	if (getRightTrigger())
	{
		if ( !oldRightTrigger )
		{
			oldRightTrigger = 1;
			//axis[SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_TRIGGERRIGHT].binary = 1;
			lastkeypressed = 300;
		}
	}
	else
	{
		oldRightTrigger = 0;
		//axis[SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_TRIGGERRIGHT].binary = 0;
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
			float percentage = static_cast<float>(current_hotbar + 1) / static_cast<float>(NUM_HOTBAR_SLOTS);
			select_inventory_slot((percentage) * INVENTORY_SIZEX - 1, INVENTORY_SIZEY - 1);
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
			float percentage = static_cast<float>(current_hotbar + 1) / static_cast<float>(NUM_HOTBAR_SLOTS);
			select_inventory_slot((percentage) * INVENTORY_SIZEX - 1, 0);
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

bool GameController::handleRepairGUIMovement()
{
	bool dpad_moved = false;

	if ( itemMenuOpen )
	{
		return false;
	}

	if ( *inputPressed(joyimpulses[INJOY_DPAD_UP]) )
	{
		GenericGUI.selectSlot(GenericGUI.selectedSlot - 1);
		*inputPressed(joyimpulses[INJOY_DPAD_UP]) = 0;

		dpad_moved = true;
	}

	if ( *inputPressed(joyimpulses[INJOY_DPAD_DOWN]) )
	{
		GenericGUI.selectSlot(GenericGUI.selectedSlot + 1);
		*inputPressed(joyimpulses[INJOY_DPAD_DOWN]) = 0;

		dpad_moved = true;
	}

	if ( dpad_moved )
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

	if ( joyimpulses[controllerImpulse] == 299 || joyimpulses[controllerImpulse] == 300 )
	{
		return controller->binaryToggle(GameController::getSDLTriggerFromImpulse(controllerImpulse));
	}
	else
	{
		return controller->binaryToggle(GameController::getSDLButtonFromImpulse(controllerImpulse));
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

	if ( joyimpulses[controllerImpulse] == 299 || joyimpulses[controllerImpulse] == 300 )
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

	printlog("[INPUTS]: Warning: player index %d invalid.", player);
	return false;
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
			messagePlayer(0, "%d: %d", i, axis[i].binary ? 1 : 0);
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