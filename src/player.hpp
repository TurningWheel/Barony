/*-------------------------------------------------------------------------------

	BARONY
	File: player.hpp
	Desc: contains various declarations for player code.

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"

#pragma once

//Splitscreen support stuff.
extern int current_player; //This may not be necessary. Consider this: Each Player instance keeps track of whether it is a network player or a localhost player.

//TODO: Move these into each and every individual player.
extern Entity* selectedEntity;
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

	//Closes the SDL device.
	void close();

	//Opens the SDL device.
	//If c < 0 or c >= SDL_NumJoysticks() or c is not a game controller, then returns false.
	bool open(int c);

	bool isActive();

	/*
	 * Moves the player's head around.
	 * Handles the triggers.
	 */
	void handleAnalog();

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
	bool handleInventoryMovement();

	/*
	 * Uses dpad to move the cursor around a chest's inventory and select items.
	 * Returns true if moved.
	 */
	bool handleChestMovement();

	/*
	 * Uses dpad to move the cursor around a shop's inventory and select items.
	 * Returns true if moved.
	 */
	bool handleShopMovement();

	/*
	 * Uses dpad to move the cursor around a shop's inventory and select items.
	 * Returns true if moved.
	 */
	bool handleIdentifyMovement();

	/*
	 * Uses dpad to move the cursor through the item context menu and select entries.
	 * Returns true if moved.
	 */
	bool handleItemContextMenu(const Item& item);
};

extern GameController* game_controller;

void initGameControllers();

class Player
{
	//Splitscreen support. Every player gets their own screen.
	//Except in multiplayer. In that case, this is just a big old dummy class.
	SDL_Surface* screen;

	//Is this a hotseat player? If so, draw splitscreen and stuff. (Host player is automatically a hotseat player). If not, then this is a dummy container for the multiplayer client.
	bool local_host;

	int playernum;

public:
	Entity* entity;

	Player(int playernum = 0, bool local_host = true);
	~Player();
};

void initIdentifyGUIControllerCode();

extern Player** players;
//In the process of switching from the old entity player array, all of the old uses of player need to be hunted down and then corrected to account for the new array.
//So, search for the comment:
//TODO: PLAYERSWAP
//and fix and verify that the information is correct.
//Then, once all have been fixed and verified, uncomment this declaration, and the accompanying definition in player.cpp; uncomment all of the TODO: PLAYERSWAP code segments, and attempt compilation and running.
