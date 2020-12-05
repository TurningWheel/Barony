/*-------------------------------------------------------------------------------

	BARONY
	File: player.hpp
	Desc: contains various declarations for player code.

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "interface/interface.hpp"

#pragma once

//Splitscreen support stuff.
extern int current_player; //This may not be necessary. Consider this: Each Player instance keeps track of whether it is a network player or a localhost player.

//TODO: Move these into each and every individual player.
extern Entity* selectedEntity[MAXPLAYERS];
extern Entity* lastSelectedEntity[MAXPLAYERS];
extern Sint32 mousex, mousey;
extern Sint32 omousex, omousey;
extern Sint32 mousexrel, mouseyrel;

/*
 * TODO: Will need to make messages work for each hotseat player.
 * This will probably involve taking the current notification_messages thing and instead including that in a wrapper or something that is owned by each player instance.
 * Basically, each player will need to keep track of its own messages.
 *
 * I believe one of the splitscreen layouts included a version where all of the messages were communal and were in the center of the screen or summat.
 */

extern bool splitscreen;

extern int gamepad_deadzone;
extern int gamepad_trigger_deadzone;
extern int gamepad_leftx_sensitivity;
extern int gamepad_lefty_sensitivity;
extern int gamepad_rightx_sensitivity;
extern int gamepad_righty_sensitivity;
extern int gamepad_menux_sensitivity;
extern int gamepad_menuy_sensitivity;

extern bool gamepad_leftx_invert;
extern bool gamepad_lefty_invert;
extern bool gamepad_rightx_invert;
extern bool gamepad_righty_invert;
extern bool gamepad_menux_invert;
extern bool gamepad_menuy_invert;

//Game Controller 1 handler
//TODO: Joystick support?
//extern SDL_GameController* game_controller;

class GameController
{
	SDL_GameController* sdl_device;
	int id;

	int oldLeftTrigger;
	int oldRightTrigger;

	std::string name;
public:
	GameController();
	~GameController();

	struct Binding_t {
		float analog = 0.f;
		float deadzone = 0.f;
		bool binary = false;
		bool consumed = false;

		enum Bindtype_t 
		{
			INVALID,
			KEYBOARD,
			CONTROLLER_AXIS,
			CONTROLLER_BUTTON,
			MOUSE_BUTTON,
			JOYSTICK_AXIS,
			JOYSTICK_BUTTON,
			JOYSTICK_HAT,
			//JOYSTICK_BALL,
			NUM
		};
		Bindtype_t type = INVALID;

		SDL_GameControllerAxis padAxis = SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_INVALID;
		SDL_GameControllerButton padButton = SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_INVALID;
		bool padAxisNegative = false;
	};

	void updateButtons();
	void updateAxis();
	static SDL_GameControllerButton getSDLButtonFromImpulse(const unsigned controllerImpulse);
	static SDL_GameControllerAxis getSDLTriggerFromImpulse(const unsigned controllerImpulse);

	Binding_t buttons[NUM_JOY_STATUS];
	Binding_t axis[NUM_JOY_AXIS_STATUS];

	bool binary(SDL_GameControllerButton binding) const;
	bool binaryToggle(SDL_GameControllerButton binding) const;
	void consumeBinaryToggle(SDL_GameControllerButton binding);
	float analog(SDL_GameControllerButton binding) const;
	bool binary(SDL_GameControllerAxis binding) const;
	bool binaryToggle(SDL_GameControllerAxis binding) const;
	void consumeBinaryToggle(SDL_GameControllerAxis binding);
	float analog(SDL_GameControllerAxis binding) const;


	//! converts the given input to a boolean/analog value
	bool binaryOf(Binding_t& binding);
	float analogOf(Binding_t& binding);


	//Closes the SDL device.
	void close();

	//Opens the SDL device.
	//If c < 0 or c >= SDL_NumJoysticks() or c is not a game controller, then returns false.
	bool open(int c);

	void initBindings();
	const int getID() { return id; }
	const SDL_GameController* getControllerDevice() { return sdl_device; }
	const bool isActive();

	/*
	 * Moves the player's head around.
	 * Handles the triggers.
	 */
	void handleAnalog(int player);

	//Left analog stick movement along the x axis.
	int getLeftXMove();
	//...along the y axis.
	int getLeftYMove();
	//Right analog stick movement along the x axis.
	int getRightXMove();
	//...along the y axis.
	int getRightYMove();

	int getLeftTrigger();
	int getRightTrigger();

	//The amount of movement of the given analog stick along its respective axis, with no gamepad sensitivity application. Deadzone is taken into account.
	int getRawLeftXMove();
	int getRawLeftYMove();
	int getRawRightXMove();
	int getRawRightYMove();

	int getRawLeftTrigger();
	int getRawRightTrigger();

	//Gets the percentage the given stick is pressed along its current axis. From 0% after the deadzone to 100% all the way to the edge of the analog stick.
	float getLeftXPercent();
	float getLeftYPercent();
	float getRightXPercent();
	float getRightYPercent();

	float getLeftTriggerPercent();
	float getRightTriggerPercent();

	//The maximum amount the given analog stick can move on its respective axis. After the gamepad deadzone is taken into account.
	int maxLeftXMove();
	int maxLeftYMove();
	int maxRightXMove();
	int maxRightYMove();

	int maxLeftTrigger();
	int maxRightTrigger();

	/*
	 * Uses dpad to move the cursor around the inventory and select items.
	 * Returns true if moved.
	 */
	bool handleInventoryMovement(const int player);

	/*
	 * Uses dpad to move the cursor around a chest's inventory and select items.
	 * Returns true if moved.
	 */
	bool handleChestMovement(const int player);

	/*
	 * Uses dpad to move the cursor around a shop's inventory and select items.
	 * Returns true if moved.
	 */
	bool handleShopMovement(const int player);

	/*
	 * Uses dpad to move the cursor around Identify GUI's inventory and select items.
	 * Returns true if moved.
	 */
	bool handleIdentifyMovement(const int player);

	/*
	 * Uses dpad to move the cursor around Remove Curse GUI's inventory and select items.
	 * Returns true if moved.
	 */
	bool handleRemoveCurseMovement(const int player);

	/*
	 * Uses dpad to move the cursor through the item context menu and select entries.
	 * Returns true if moved.
	 */
	bool handleItemContextMenu(const int player, const Item& item);

	/*
	* Uses dpad to move the cursor through the item context menu and select entries.
	* Returns true if moved.
	*/
	bool handleRepairGUIMovement(const int player);
};
const int MAX_GAME_CONTROLLERS = 16;
extern std::array<GameController, MAX_GAME_CONTROLLERS> game_controllers;

class Inputs
{
	int playerControllerIds[MAXPLAYERS];
	int playerUsingKeyboardControl = 0;

	class VirtualMouse
	{
	public:
		int xrel = 0; //mousexrel
		int yrel = 0; //mouseyrel
		int ox = 0; //omousex
		int oy = 0; //omousey
		int x = 0; //mousex
		int y = 0; //mousey
		bool draw_cursor = true;
		bool moved = false;
		VirtualMouse() {};
		~VirtualMouse() {};

		void warpMouseInCamera(const view_t& camera, const Sint32 newx, const Sint32 newy)
		{
			x = std::max(camera.winx, std::min(camera.winx + camera.winw, x + newx));
			y = std::max(camera.winy, std::min(camera.winy + camera.winh, y + newy));
			moved = true;
		}
	};
	VirtualMouse vmouse[MAXPLAYERS];
public:
	Inputs() 
	{
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			playerControllerIds[i] = -1;
		}
	};
	~Inputs() {};
	const void setPlayerIDAllowedKeyboard(int player)
	{
		playerUsingKeyboardControl = player;
	}
	const bool bPlayerUsingKeyboardControl(int player) const
	{
		if ( !splitscreen )
		{
			return player == clientnum;
		}
		return player == playerUsingKeyboardControl;
	}
	void controllerHandleMouse(int player);
	const bool bControllerInputPressed(int player, const unsigned controllerImpulse) const;
	void controllerClearInput(int player, const unsigned controllerImpulse);
	void removeControllerWithDeviceID(int id)
	{
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			if ( playerControllerIds[i] == id )
			{
				playerControllerIds[i] = -1;
				printlog("[INPUTS]: Removed controller id %d from player index %d.", id, i);
			}
		}
	}
	VirtualMouse* getMouse(int player)
	{
		if ( player < 0 || player >= MAXPLAYERS )
		{
			printlog("[INPUTS]: Warning: player index %d out of range.", player);
			return nullptr;
		}
		return &vmouse[player];
	}
	const int getControllerID(int player) const
	{
		if ( player < 0 || player >= MAXPLAYERS )
		{
			printlog("[INPUTS]: Warning: player index %d out of range.", player);
			return -1;
		}
		return playerControllerIds[player];
	}
	GameController* getController(int player) const;

	const bool hasController(int player) const 
	{
		if ( player < 0 || player >= MAXPLAYERS )
		{
			printlog("[INPUTS]: Warning: player index %d out of range.", player);
			return false;
		}
		return playerControllerIds[player] != -1;
	}
	void setControllerID(int player, const int id) 
	{
		if ( player < 0 || player >= MAXPLAYERS )
		{
			printlog("[INPUTS]: Warning: player index %d out of range.", player);
		}
		playerControllerIds[player] = id;
	}
	void addControllerIDToNextAvailableInput(const int id)
	{
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			if ( playerControllerIds[i] == -1 )
			{
				playerControllerIds[i] = id;
				printlog("[INPUTS]: Automatically assigned controller id %d to player index %d.", id, i);
				break;
			}
		}
	}
	const bool bPlayerIsControllable(int player) const;
	void updateAllMouse()
	{
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			controllerHandleMouse(i);
		}
	}
	void updateAllOMouse()
	{
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			vmouse[i].ox = vmouse[i].x;
			vmouse[i].oy = vmouse[i].y;
			vmouse[i].moved = false;
		}
		/*messagePlayer(0, "x: %d | y: %d / x: %d | y: %d / x: %d | y: %d / x: %d | y: %d ", 
			vmouse[0].ox, vmouse[0].oy,
			vmouse[1].ox, vmouse[1].oy,
			vmouse[2].ox, vmouse[2].oy,
			vmouse[3].ox, vmouse[3].oy);*/
	}
	void updateAllRelMouse()
	{
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			vmouse[i].xrel = 0;
			vmouse[i].yrel = 0;
		}
	}
};
extern Inputs inputs;
void initGameControllers();

static const unsigned NUM_HOTBAR_SLOTS = 10; //NOTE: If you change this, you must dive into drawstatus.c and update the hotbar code. It expects 10.
static const unsigned NUM_HOTBAR_ALTERNATES = 5;

class Player
{
	//Splitscreen support. Every player gets their own screen.
	//Except in multiplayer. In that case, this is just a big old dummy class.
	SDL_Surface* screen;

	//Is this a hotseat player? If so, draw splitscreen and stuff. (Host player is automatically a hotseat player). If not, then this is a dummy container for the multiplayer client.
	bool local_host;
	view_t* cam;

	int playernum;

public:
	Entity* entity;
	bool bSplitscreen = false;
	Player(int playernum = 0, bool local_host = true);
	~Player();

	class Hotbar_t;
	Hotbar_t* hotbar;
	view_t& camera() const { return *cam; }
	const int camera_x1() const { return cam->winx; }
	const int camera_x2() const { return cam->winx + cam->winw; }
	const int camera_y1() const { return cam->winy; }
	const int camera_y2() const { return cam->winy + cam->winh; }
	const int camera_width() const { return cam->winw; }
	const int camera_height() const { return cam->winh; }
	const bool isLocalPlayer() const;
	const bool isLocalPlayerAlive() const;
};

class Player::Hotbar_t {
	std::array<hotbar_slot_t, NUM_HOTBAR_SLOTS> hotbar;
	std::array<std::array<hotbar_slot_t, NUM_HOTBAR_SLOTS>, NUM_HOTBAR_ALTERNATES> hotbar_alternate;
public:
	int current_hotbar = 0;
	bool hotbarShapeshiftInit[NUM_HOTBAR_ALTERNATES] = { false, false, false, false, false };
	int swapHotbarOnShapeshift = 0;
	bool hotbarHasFocus = false;
	int magicBoomerangHotbarSlot = -1;
	Uint32 hotbarTooltipLastGameTick = 0;

	Player::Hotbar_t()
	{
		clear();
	}

	enum HotbarLoadouts : int
	{
		HOTBAR_DEFAULT,
		HOTBAR_RAT,
		HOTBAR_SPIDER,
		HOTBAR_TROLL,
		HOTBAR_IMP
	};

	void clear()
	{
		swapHotbarOnShapeshift = 0;
		current_hotbar = 0;
		hotbarHasFocus = false;
		for ( int j = 0; j < NUM_HOTBAR_ALTERNATES; ++j )
		{
			hotbarShapeshiftInit[j] = false;
		}
		for ( int i = 0; i < NUM_HOTBAR_SLOTS; ++i )
		{
			hotbar[i].item = 0;
			for ( int j = 0; j < NUM_HOTBAR_ALTERNATES; ++j )
			{
				hotbar_alternate[j][i].item = 0;
			}
		}
	}

	auto& slots() { return hotbar; };
	auto& slotsAlternate(int alternate) { return hotbar_alternate[alternate]; };
	auto& slotsAlternate() { return hotbar_alternate;  }
	void selectHotbarSlot(int slot)
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
		hotbarHasFocus = true;
	}
};

void initIdentifyGUIControllerCode();
void initRemoveCurseGUIControllerCode();

extern Player** players;
//In the process of switching from the old entity player array, all of the old uses of player need to be hunted down and then corrected to account for the new array.
//So, search for the comment:
//TODO: PLAYERSWAP
//and fix and verify that the information is correct.
//Then, once all have been fixed and verified, uncomment this declaration, and the accompanying definition in player.cpp; uncomment all of the TODO: PLAYERSWAP code segments, and attempt compilation and running.
