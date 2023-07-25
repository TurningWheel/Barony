/*-------------------------------------------------------------------------------

	BARONY
	File: player.hpp
	Desc: contains various declarations for player code.

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#pragma once
#include "interface/interface.hpp"
#include "magic/magic.hpp"
#include "messages.hpp"
#include "engine/audio/sound.hpp"
#include "input.hpp"
#include "ui/Frame.hpp"
#include "ui/Field.hpp"


//Splitscreen support stuff.

//TODO: Move these into each and every individual player.
extern Entity* selectedEntity[MAXPLAYERS];
extern Entity* lastSelectedEntity[MAXPLAYERS];

/*
 * TODO: Will need to make messages work for each hotseat player.
 * This will probably involve taking the current notification_messages thing and instead including that in a wrapper or something that is owned by each player instance.
 * Basically, each player will need to keep track of its own messages.
 *
 * I believe one of the splitscreen layouts included a version where all of the messages were communal and were in the center of the screen or summat.
 */

extern int gamepad_deadzone;
extern int gamepad_trigger_deadzone;
extern real_t gamepad_leftx_sensitivity;
extern real_t gamepad_lefty_sensitivity;
extern real_t gamepad_rightx_sensitivity;
extern real_t gamepad_righty_sensitivity;
extern real_t gamepad_menux_sensitivity;
extern real_t gamepad_menuy_sensitivity;

extern bool gamepad_leftx_invert;
extern bool gamepad_lefty_invert;
extern bool gamepad_rightx_invert;
extern bool gamepad_righty_invert;
extern bool gamepad_menux_invert;
extern bool gamepad_menuy_invert;

struct PlayerSettings_t
{
	int player = -1;
	int shootmodeCrosshair = 0;
	int shootmodeCrosshairOpacity = 50;
    real_t mousespeed = 32.0;
    bool mkb_world_tooltips_enabled = true;
    bool gamepad_facehotbar = true;
    bool hotbar_numkey_quick_add = true;
    bool reversemouse = 0;
    bool smoothmouse = false;
    real_t gamepad_rightx_sensitivity = 1.0;
    real_t gamepad_righty_sensitivity = 1.0;
    bool gamepad_rightx_invert = false;
    bool gamepad_righty_invert = false;
	void init(const int _player)
	{
		player = _player;
	}
};
extern PlayerSettings_t playerSettings[MAXPLAYERS];

//Game Controller 1 handler
//TODO: Joystick support?
//extern SDL_GameController* game_controller;

#include <chrono>

class GameController
{
	friend class Input;
	SDL_GameController* sdl_device;
	SDL_Haptic* sdl_haptic;
	int id;
	std::string name;
	static const int BUTTON_HELD_TICKS = TICKS_PER_SECOND / 4;

#ifdef NINTENDO
	using time_unit = std::chrono::time_point<std::chrono::high_resolution_clock, std::chrono::milliseconds>;
	time_unit timeNow;
	time_unit timeStart = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now());
#endif

public:
	GameController();
	~GameController();

	class Haptic_t
	{
	public:
		enum RumblePattern
		{
			RUMBLE_NORMAL,
			RUMBLE_BOULDER,
			RUMBLE_BOULDER_BOUNCE,
			RUMBLE_BOULDER_ROLLING,
			RUMBLE_DEATH,
			RUMBLE_TMP
		};
		int hapticEffectId = -1;
		struct HapticEffect
		{
			Uint16 type = SDL_HAPTIC_LEFTRIGHT;
			Uint32 length = 0;
			Uint16 large_magnitude = 0;
			Uint16 small_magnitude = 0;
			Sint32 leftRightBalance = 0;
		};
		HapticEffect hapticEffect;
		Uint32 hapticTick;
		Uint32 oscillatorTick;
		Haptic_t();
		~Haptic_t() {};

		struct Rumble
		{
			Uint32 startTick = 0;
			Uint16 smallMagnitude = 0;
			Uint16 largeMagnitude = 0;
			Uint32 startTime = 0;
			Uint32 length = 0;
			real_t customEffect = 0.0;
			RumblePattern pattern = RUMBLE_NORMAL;
			Uint32 entityUid = 0;
			bool isPlaying = false;
			Rumble(int sm, int lg, int len, Uint32 tick, Uint32 uid) :
				smallMagnitude(sm),
				largeMagnitude(lg),
				length(len),
				entityUid(uid)
			{
				startTick = tick;
			};
		};
		std::vector<std::pair<Uint32, Rumble>> activeRumbles;
		bool vibrationEnabled = true;
	} haptics;

	enum DpadDirection : int
	{
		INVALID = -2,
		CENTERED = -1,
		DOWN,
		DOWNLEFT,
		LEFT,
		UPLEFT,
		UP,
		UPRIGHT,
		RIGHT,
		DOWNRIGHT
	};

	enum RadialSelection : int
	{
		RADIAL_INVALID = -2,
		RADIAL_CENTERED = -1,
		RADIAL_MAX = 16
	};

	struct Binding_t {
		float analog = 0.f;
		float deadzone = 0.f;
		bool binary = false;
		bool consumed = false;
		Uint32 buttonHeldTicks = 0;
		bool buttonHeld = false;
		bool binaryRelease = false;
		bool binaryReleaseConsumed = false;

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
			VIRTUAL_DPAD,
			RADIAL_SELECTION,
			//JOYSTICK_BALL,
			NUM
		};
		Bindtype_t type = INVALID;

		SDL_GameControllerAxis padAxis = SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_INVALID;
		SDL_GameControllerButton padButton = SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_INVALID;
		DpadDirection padVirtualDpad = DpadDirection::CENTERED;
		RadialSelection padRadialSelection = RadialSelection::RADIAL_CENTERED;
		bool padAxisNegative = false;
	};

	void updateButtons();
	void updateButtonsReleased();
	void updateAxis();
	static SDL_GameControllerButton getSDLButtonFromImpulse(const unsigned controllerImpulse);
	static SDL_GameControllerAxis getSDLTriggerFromImpulse(const unsigned controllerImpulse);

	Binding_t buttons[NUM_JOY_STATUS];
	Binding_t axis[NUM_JOY_AXIS_STATUS];
	Binding_t virtualDpad;
	Binding_t radialSelection;

	bool binary(SDL_GameControllerButton binding) const;
	bool binaryToggle(SDL_GameControllerButton binding) const;
	bool binaryReleaseToggle(SDL_GameControllerButton binding) const;
	void consumeBinaryToggle(SDL_GameControllerButton binding);
	void consumeBinaryReleaseToggle(SDL_GameControllerButton binding);
	bool buttonHeldToggle(SDL_GameControllerButton binding) const;
	float analog(SDL_GameControllerButton binding) const;
	bool binary(SDL_GameControllerAxis binding) const;
	bool binaryToggle(SDL_GameControllerAxis binding) const;
	void consumeBinaryToggle(SDL_GameControllerAxis binding);
	bool buttonHeldToggle(SDL_GameControllerAxis binding) const;
	float analog(SDL_GameControllerAxis binding) const;
	DpadDirection dpadDir() const;
	DpadDirection dpadDirToggle() const;
	void consumeDpadDirToggle();


	//! converts the given input to a boolean/analog value
	bool binaryOf(Binding_t& binding);
	float analogOf(Binding_t& binding);


	//Closes the SDL device.
	void close();

	//Opens the SDL device.
	//If sdl_which < 0 or sdl_which >= SDL_NumJoysticks() or sdl_which is not a game controller, then returns false.
	//index is the new id of the game controller.
	bool open(int sdl_which, int index);

	void initBindings();
	const int getID() { return id; }
	SDL_GameController* getControllerDevice() const { return sdl_device; }
	SDL_Haptic* getHaptic() { return sdl_haptic; }
	const bool isActive();
	void addRumble(Haptic_t::RumblePattern pattern, Uint16 smallMagnitude, Uint16 largeMagnitude, Uint32 length, Uint32 srcEntityUid);
	GameController::Haptic_t::HapticEffect* doRumble(Haptic_t::Rumble* r);
	GameController::Haptic_t::HapticEffect* handleRumble();
	void stopRumble();
	void reinitHaptic();

	/*
	 * Moves the player's head around.
	 * Handles the triggers.
	 */
	void handleAnalog(int player);

	//Right analog stick movement along the x axis.
	int getRightXMove(int player);
	//...along the y axis.
	int getRightYMove(int player);

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

	//Gets the percentage of the left stick for player movement, 100% input is multiplied by :
	// x_forceMaxForwardThreshold, x_forceMaxBackwardThreshold, y_forceMaxStrafeThreshold
	float getLeftXPercentForPlayerMovement();
	float getLeftYPercentForPlayerMovement();

	float getLeftTriggerPercent();
	float getRightTriggerPercent();

	//The maximum amount the given analog stick can move on its respective axis. After the gamepad deadzone is taken into account.
	int maxLeftXMove();
	int maxLeftYMove();
	int maxRightXMove();
	int maxRightYMove();

	int maxLeftTrigger();
	int maxRightTrigger();

	enum DeadZoneType
	{
		DEADZONE_PER_AXIS,
		DEADZONE_MAGNITUDE_LINEAR,
		DEADZONE_MAGNITUDE_HALFPIPE
	};
	DeadZoneType leftStickDeadzoneType = DEADZONE_PER_AXIS;
	DeadZoneType rightStickDeadzoneType = DEADZONE_MAGNITUDE_HALFPIPE;

	Sint32 leftStickDeadzone = 8000;
	Sint32 rightStickDeadzone = 8000;

	real_t oldFloatRightX = 0.0; // current delta per frame right-stick analogue value
	real_t oldFloatRightY = 0.0; // current delta per frame right-stick analogue value
	int oldAxisRightX = 0;  // current raw right-stick analogue value (0-32768)
	int oldAxisRightY = 0; // current raw right-stick analogue value (0-32768)

	float x_forceMaxForwardThreshold = 0.7;
	float x_forceMaxBackwardThreshold = 0.5;
	float y_forceMaxStrafeThreshold = 0.7;
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
		static const int MOUSE_HELD_TICKS = TICKS_PER_SECOND;
		Sint32 xrel = 0; //mousexrel
		Sint32 yrel = 0; //mouseyrel
		Sint32 ox = 0; //omousex
		Sint32 oy = 0; //omousey
		Sint32 x = 0; //mousex
		Sint32 y = 0; //mousey

		real_t floatxrel = 0.0;
		real_t floatyrel = 0.0;
		real_t floatx = 0.0;
		real_t floaty = 0.0;
		real_t floatox = 0.0;
		real_t floatoy = 0.0;

		Uint32 mouseLeftHeldTicks = 0;
		bool mouseLeftHeld = false;
		Uint32 mouseRightHeldTicks = 0;
		bool mouseRightHeld = false;

		bool draw_cursor = false; //True if the gamepad's d-pad has been used to navigate menus and such. //TODO: Off by default on consoles and the like.
		bool moved = false;
		bool lastMovementFromController = true;
		real_t mouseAnimationPercent = 0.0;
		VirtualMouse() {};
		~VirtualMouse() {};

		void warpMouseInCamera(const view_t& camera, const Sint32 newx, const Sint32 newy)
		{
			x = std::max(camera.winx, std::min(camera.winx + camera.winw, x + newx));
			y = std::max(camera.winy, std::min(camera.winy + camera.winh, y + newy));
			xrel += newx;
			yrel += newy;
			moved = true;
		}
		void warpMouseInScreen(SDL_Window*& window, const Sint32 newx, const Sint32 newy)
		{
			int w, h;
			SDL_GetWindowSize(window, &w, &h);
			x = std::max(0, std::min(w, x + newx));
			y = std::max(0, std::min(h, y + newy));
			xrel += newx;
			yrel += newy;
			moved = true;
		}
	};
	VirtualMouse vmouse[MAXPLAYERS];

	class UIStatus
	{
	public:
		/*
		* So that the cursor jumps back to the hotbar instead of the inventory if a picked up hotbar item is canceled.
		* Its value indicates which hotbar slot it's from.
		* -1 means it's not from the hotbar.
		*/
		int selectedItemFromHotbar = -1;
		Uint32 selectedItemFromChest = 0;

		Item* selectedItem = nullptr;
		bool toggleclick = false;
		bool itemMenuOpen = false;
		bool itemMenuFromHotbar = false;
		int itemMenuOffsetDetectionY = 0;
		int itemMenuX = 0;
		int itemMenuY = 0;
		int itemMenuSelected = 0;
		Uint32 itemMenuItem = 0;
	};
	UIStatus uiStatus[MAXPLAYERS];

public:
	Inputs() 
	{
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			playerControllerIds[i] = -1;
		}
	};
	~Inputs() {};
	const void setPlayerIDAllowedKeyboard(const int player)
	{
		if (multiplayer != SINGLE && player != 0) {
			setPlayerIDAllowedKeyboard(0);
			return;
		}
	    printlog("giving keyboard to player %d", player);
		playerUsingKeyboardControl = player;
	}
	const int getPlayerIDAllowedKeyboard()
	{
		if (multiplayer != SINGLE)
		{
			return 0;
		}
		else
		{
	   		return playerUsingKeyboardControl;
		}
	}
	const bool bPlayerUsingKeyboardControl(const int player) const
	{
		return player == playerUsingKeyboardControl || multiplayer != SINGLE;
	}
	void controllerHandleMouse(const int player);
	const bool bControllerInputPressed(const int player, const unsigned controllerImpulse) const;
	const bool bControllerInputHeld(int player, const unsigned controllerImpulse) const;
	const bool bControllerRawInputPressed(const int player, const unsigned button) const;
	const bool bControllerRawInputReleased(const int player, const unsigned button) const;
	void controllerClearInput(const int player, const unsigned controllerImpulse);
	void controllerClearRawInput(const int player, const unsigned button);
	void controllerClearRawInputRelease(const int player, const unsigned button);
	const bool bMouseLeft (const int player) const;
	const bool bMouseHeldLeft(const int player) const;
	const bool bMouseRight(const int player) const;
	const bool bMouseHeldRight(const int player) const;
	const void mouseClearLeft(int player);
	const void mouseClearRight(int player);
	void removeControllerWithDeviceID(const int id)
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
	VirtualMouse* getVirtualMouse(int player)
	{
		if (multiplayer != SINGLE && player != 0) {
			return getVirtualMouse(0);
		}
		if ( player < 0 || player >= MAXPLAYERS )
		{
			printlog("[INPUTS]: Warning: player index %d out of range.", player);
			return nullptr;
		}
		return &vmouse[player];
	}
	UIStatus* getUIInteraction(int player)
	{
		if ( player < 0 || player >= MAXPLAYERS )
		{
			printlog("[INPUTS]: Warning: player index %d out of range.", player);
			return nullptr;
		}
		return &uiStatus[player];
	}
	enum MouseInputs
	{
		OX,
		OY,
		X,
		Y,
		XREL,
		YREL,
		ANALOGUE_OX,
		ANALOGUE_OY,
		ANALOGUE_X,
		ANALOGUE_Y,
		ANALOGUE_XREL,
		ANALOGUE_YREL,
	};
	const Sint32 getMouse(const int player, MouseInputs input);
	const real_t getMouseFloat(const int player, MouseInputs input);
	void setMouse(const int player, MouseInputs input, Sint32 value);
	void hideMouseCursors()
	{
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			getVirtualMouse(i)->draw_cursor = false;
		}
	};
	enum MouseWarpFlags : Uint32
	{
		UNSET_RELATIVE_MOUSE = 1,
		SET_RELATIVE_MOUSE = 2,
		SET_MOUSE = 4,
		SET_CONTROLLER = 8
	};
	void warpMouse(const int player, const Sint32 x, const Sint32 y, Uint32 flags);
	const int getControllerID(int player) const
	{
		if (multiplayer != SINGLE && player != 0) {
			return getControllerID(0);
		}
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
		if (multiplayer != SINGLE && player != 0) {
			return hasController(0);
		}
		if ( player < 0 || player >= MAXPLAYERS )
		{
			printlog("[INPUTS]: Warning: player index %d out of range.", player);
			return false;
		}
		return playerControllerIds[player] != -1 && getController(player);
	}
	void setControllerID(int player, const int id) 
	{
		if (multiplayer != SINGLE && player != 0) {
			return setControllerID(0, id);
		}
		if ( player < 0 || player >= MAXPLAYERS )
		{
			printlog("[INPUTS]: Warning: player index %d out of range.", player);
		}
		playerControllerIds[player] = id;
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
			if ( !bMouseLeft(i) )
			{
				vmouse[i].ox = vmouse[i].x;
				vmouse[i].oy = vmouse[i].y;
				vmouse[i].floatox = vmouse[i].floatx;
				vmouse[i].floatoy = vmouse[i].floaty;
				vmouse[i].moved = false;
				vmouse[i].mouseLeftHeld = false;
				vmouse[i].mouseLeftHeldTicks = 0;
			}
			else
			{
				if ( vmouse[i].mouseLeftHeldTicks == 0 )
				{
					vmouse[i].mouseLeftHeldTicks = ticks;
				}
				else if ( ticks - vmouse[i].mouseLeftHeldTicks > Inputs::VirtualMouse::MOUSE_HELD_TICKS )
				{
					vmouse[i].mouseLeftHeld = true;
				}
			}
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
			vmouse[i].floatxrel = 0.0;
			vmouse[i].floatyrel = 0.0;
		}
	}
	void rumble(const int player, GameController::Haptic_t::RumblePattern pattern, Uint16 smallMagnitude, Uint16 largeMagnitude, Uint32 length, Uint32 srcEntityUid)
	{
		if (multiplayer != SINGLE && player != 0) {
			rumble(0, pattern, smallMagnitude, largeMagnitude, length, srcEntityUid);
			return;
		}
		if ( !hasController(player) )
		{
			return;
		}
		getController(player)->addRumble(pattern, smallMagnitude, largeMagnitude, length, srcEntityUid);
	}
	void rumbleStop(const int player)
	{
		if (multiplayer != SINGLE && player != 0) {
			rumbleStop(0);
			return;
		}
		if ( !hasController(player) )
		{
			return;
		}
		getController(player)->stopRumble();
	}
	void addRumbleForPlayerHPLoss(const int player, Sint32 damageAmount);
	SDL_Rect getGlyphRectForInput(const int player, bool pressed, const unsigned keyboardImpulse, const unsigned controllerImpulse);
	void addRumbleForHapticType(const int player, Uint32 hapticType, Uint32 uid);
	static const Uint32 HAPTIC_SFX_BOULDER_BOUNCE_VOL;
	static const Uint32 HAPTIC_SFX_BOULDER_ROLL_LOW_VOL;
	static const Uint32 HAPTIC_SFX_BOULDER_ROLL_HIGH_VOL;
	static const Uint32 HAPTIC_SFX_BOULDER_LAUNCH_VOL;
	void addRumbleRemotePlayer(const int player, Uint32 hapticType, Uint32 uid);
};
extern Inputs inputs;
void initGameControllers();

static const unsigned NUM_HOTBAR_SLOTS = 10; //NOTE: If you change this, you must dive into drawstatus.c and update the hotbar code. It expects 10.
static const unsigned NUM_HOTBAR_ALTERNATES = 5;

class Player
{
	//Splitscreen support. Every player gets their own screen.
	//Except in multiplayer. In that case, this is just a big old dummy class.
	//SDL_Surface* screen;
	
	//Is this a hotseat player? If so, draw splitscreen and stuff. (Host player is automatically a hotseat player). If not, then this is a dummy container for the multiplayer client.
	bool local_host;
	view_t* cam;

	int playernum;

public:
	Entity* entity;

	enum SplitScreenTypes : int
	{
		SPLITSCREEN_DEFAULT,
		SPLITSCREEN_VERTICAL
	};

	bool bSplitscreen = false;
	SplitScreenTypes splitScreenType = SPLITSCREEN_DEFAULT;
	bool bControlEnabled = true; // disabled if dead waiting for gameover prompt etc
	Player(int playernum = 0, bool local_host = true);
	~Player();

	void init();
	void cleanUpOnEntityRemoval();

	const char* getAccountName() const;

	view_t& camera() const { return *cam; }
	const int camera_x1() const { return cam->winx; }
	const int camera_x2() const { return cam->winx + cam->winw; }
	const int camera_y1() const { return cam->winy; }
	const int camera_y2() const { return cam->winy + cam->winh; }
	const int camera_width() const { return cam->winw; }
	const int camera_height() const { return cam->winh; }
	const int camera_virtualx1() const { return cam->winx * ((float)Frame::virtualScreenX / xres); }
	const int camera_virtualx2() const { return (cam->winx + cam->winw)* ((float)Frame::virtualScreenX / xres); }
	const int camera_virtualy1() const { return cam->winy * ((float)Frame::virtualScreenY / yres); }
	const int camera_virtualy2() const { return (cam->winy + cam->winh) * ((float)Frame::virtualScreenY / yres); }
	const int camera_virtualWidth() const { return cam->winw * ((float)Frame::virtualScreenX / xres); }
	const int camera_virtualHeight() const { return cam->winh * ((float)Frame::virtualScreenY / yres); }
	const int camera_midx() const { return camera_x1() + camera_width() / 2; }
	const int camera_midy() const { return camera_y1() + camera_height() / 2; }
	const bool isLocalPlayer() const;
	const bool isLocalPlayerAlive() const;
	const bool bUseCompactGUIWidth() const;
	const bool bUseCompactGUIHeight() const;
	const bool bAlignGUINextToInventoryCompact() const; // if chest/shop etc appears alongside inventory as opposed to opposite of viewport in compact view
	const bool usingCommand() const;
	void clearGUIPointers();

	enum PanelJustify_t
	{
		PANEL_JUSTIFY_LEFT,
		PANEL_JUSTIFY_RIGHT
	};

	struct GUIDropdown_t {
		Player& player;
		int dropDownX = 0;
		int dropDownY = 0;
		int dropDownOptionSelected = -1;
		Uint32 dropDownItem = 0;
		bool bOpen = false;
		std::string currentName = "";
		Frame* dropdownBlockClickFrame = nullptr;
		Frame* dropdownFrame = nullptr;
		bool dropDownToggleClick = false;
		int dropdownLinkToModule = 0;
		bool bClosedThisTick = false;
		struct DropdownOption_t {
			std::string text = "";
			std::string keyboardGlyph = "";
			std::string controllerGlyph = "";
			std::string action = "";
			DropdownOption_t(std::string _text, std::string _keyboardGlyph, std::string _controllerGlyph, std::string _action)
			{
				text = _text;
				action = _action;
				keyboardGlyph = _keyboardGlyph;
				controllerGlyph = _controllerGlyph;
			}
		};
		struct DropDown_t
		{
			std::string title = "Interact";
			std::string internalName = "";
			bool alignRight = true;
			int module = 0;
			int defaultOption = 0;
			std::vector<DropdownOption_t> options;
		};

		void open(const std::string name);
		void close();
		void create(const std::string name);
		bool set(const std::string name);
		void process();
		bool getDropDownAlignRight(const std::string& name);
		void activateSelection(const std::string& name, int option);
		static std::map<std::string, DropDown_t> allDropDowns;
		GUIDropdown_t(Player& p) :
			player(p) {}
	};

	class GUI_t
	{
		Player& player;
	public:
		GUI_t(Player& p) :
			player(p),
			dropdownMenu(p)
		{};
		~GUI_t() {};
		enum GUIModules
		{
			MODULE_NONE,
			MODULE_INVENTORY,
			MODULE_SHOP,
			MODULE_CHEST,
			MODULE_REMOVECURSE,
			MODULE_IDENTIFY,
			MODULE_TINKERING,
			MODULE_ALCHEMY,
			MODULE_FEATHER,
			MODULE_FOLLOWERMENU,
			MODULE_CHARACTERSHEET,
			MODULE_SKILLS_LIST,
			MODULE_BOOK_VIEW,
			MODULE_HOTBAR,
			MODULE_SPELLS,
			MODULE_STATUS_EFFECTS,
			MODULE_LOG,
			MODULE_MAP,
			MODULE_SIGN_VIEW,
			MODULE_ITEMEFFECTGUI,
			MODULE_PORTRAIT
		};
		GUIModules activeModule = MODULE_NONE;
		GUIModules previousModule = MODULE_NONE;
		GUIModules hoveringButtonModule = MODULE_NONE;
		void activateModule(GUIModules module);
		bool warpControllerToModule(bool moveCursorInstantly);
		bool bActiveModuleUsesInventory();
		bool bActiveModuleHasNoCursor();
		void setHoveringOverModuleButton(GUIModules moduleOfButton);
		void clearHoveringOverModuleButton();
		GUIModules hoveringOverModuleButton();
		bool handleCharacterSheetMovement(); // controller movement for misc GUIs not for inventory/hotbar
		bool handleInventoryMovement(); // controller movement for hotbar/inventory
		GUIModules handleModuleNavigation(bool checkDestinationOnly, bool checkLeftNavigation = true);
		bool bModuleAccessibleWithMouse(GUIModules moduleToAccess); // if no other full-screen modules taking precedence
		bool isGameoverActive();
		bool returnToPreviousActiveModule();
		GUIDropdown_t dropdownMenu;
		void closeDropdowns();
		bool isDropdownActive();
	} GUI;

	//All the code that sets shootmode = false. Display chests, inventory, books, shopkeeper, identify, whatever.
	void openStatusScreen(const int whichGUIMode, const int whichInventoryMode, const int whichModule = Player::GUI_t::MODULE_INVENTORY); //TODO: Make all the everything use this. //TODO: Make an accompanying closeStatusScreen() function.
	void closeAllGUIs(CloseGUIShootmode shootmodeAction, CloseGUIIgnore whatToClose);
	bool shootmode = false;
	int inventory_mode = INVENTORY_MODE_ITEM;
	int gui_mode = GUI_MODE_NONE;

	class Inventory_t
	{
		int sizex = 0;
		int sizey = 0;
		const int starty = 10;
		Player& player;

		int selectedSlotX = 0;
		int selectedSlotY = 0;

		int selectedSpellX = 0;
		int selectedSpellY = 0;
	public:
		Frame* frame = nullptr;
		Frame* tooltipContainerFrame = nullptr;
		Frame* tooltipFrame = nullptr;
		Frame* titleOnlyTooltipFrame = nullptr;
		Frame* interactFrame = nullptr;
		Frame* interactBlockClickFrame = nullptr;
		Frame* tooltipPromptFrame = nullptr;
		Frame* selectedItemCursorFrame = nullptr;
		Frame* spellFrame = nullptr;
		Frame* chestFrame = nullptr;
		std::unordered_map<int, Frame*> slotFrames;
		std::unordered_map<int, Frame*> spellSlotFrames;
		std::unordered_map<int, Frame*> chestSlotFrames;
		bool bCompactView = false;
		real_t slideOutPercent = 0.0;
		static int slideOutWidth;
		bool bFirstTimeSnapCursor = false;
		bool isInteractable = false;
		Uint32 tooltipDelayTick = 0;
		real_t animPaperDollHide = 0.0;
		bool bIsTooltipDelayed();
		void openInventory();
		void closeInventory();

		int miscTooltipOpacitySetpoint = 100;
		real_t miscTooltipOpacityAnimate = 1.0;
		Uint32 miscTooltipDeselectedTick = 0;
		Frame* miscTooltipFrame = nullptr;

		PanelJustify_t inventoryPanelJustify = PANEL_JUSTIFY_LEFT;
		PanelJustify_t paperDollPanelJustify = PANEL_JUSTIFY_LEFT;
		void setCompactView(bool bCompact);
		void resizeAndPositionInventoryElements();
		bool paperDollContextMenuActive();
		void updateInventoryMiscTooltip();
		enum GamepadDropdownTypes : int
		{
			GAMEPAD_DROPDOWN_DISABLE,
			GAMEPAD_DROPDOWN_FULL, // always open full context menu on 'A', 'B' to close inventory
			GAMEPAD_DROPDOWN_COMPACT // 'A' opens a context menu if item has 2+ options on 'A', 'Y'. 'Y' drop, 'B' close inv
		};
		GamepadDropdownTypes useItemDropdownOnGamepad = GAMEPAD_DROPDOWN_COMPACT;

		struct Cursor_t
		{
			real_t animateX = 0.0;
			real_t animateY = 0.0;
			int animateSetpointX = 0;
			int animateSetpointY = 0;
			int animateStartX = 0;
			int animateStartY = 0;
			Uint32 lastUpdateTick = 0;
			const int cursorToSlotOffset = 7;
			Player::GUI_t::GUIModules queuedModule = Player::GUI_t::GUIModules::MODULE_NONE;
			Frame* queuedFrameToWarpTo = nullptr;
		};
		Cursor_t cursor;

		struct SelectedItemAnimate_t
		{
			real_t animateX = 0.0;
			real_t animateY = 0.0;
		};
		SelectedItemAnimate_t selectedItemAnimate;

		struct SpellPanel_t
		{
			Player& player;
			PanelJustify_t panelJustify = PANEL_JUSTIFY_LEFT;
			real_t animx = 0.0;
			real_t scrollPercent = 0.0;
			real_t scrollInertia = 0.0;
			int scrollSetpoint = 0;
			real_t scrollAnimateX = 0.0;
			bool isInteractable = true;
			bool bOpen = false;
			bool bFirstTimeSnapCursor = false;
			int currentScrollRow = 0;
			const int kNumSpellsToDisplayVertical = 5;
			int getNumSpellsToDisplayVertical() const;
			void openSpellPanel();
			void closeSpellPanel();
			void updateSpellPanel();
			void scrollToSlot(int x, int y, bool instantly);
			bool isSlotVisible(int x, int y) const;
			bool isItemVisible(Item* item) const;
			static int heightOffsetWhenNotCompact;
			SpellPanel_t(Player& p) :
				player(p) {}
		};
		SpellPanel_t spellPanel;

		struct ChestGUI_t
		{
			Player& player;
			PanelJustify_t panelJustify = PANEL_JUSTIFY_LEFT;
			real_t animx = 0.0;
			real_t scrollPercent = 0.0;
			real_t scrollInertia = 0.0;
			int scrollSetpoint = 0;
			real_t scrollAnimateX = 0.0;
			bool isInteractable = true;
			bool bOpen = false;
			bool bFirstTimeSnapCursor = false;
			int currentScrollRow = 0;
			const int kNumItemsToDisplayVertical = 3;
			int getNumItemsToDisplayVertical() const;
			void openChest();
			void closeChest();
			void updateChest();
			void scrollToSlot(int x, int y, bool instantly);
			bool isSlotVisible(int x, int y) const;
			bool isItemVisible(Item* item) const;

			int selectedChestSlotX = -1;
			int selectedChestSlotY = -1;
			bool isChestSelected();

			static int heightOffsetWhenNotCompact;
			ChestGUI_t(Player& p) :
				player(p) {}
		};
		ChestGUI_t chestGUI;

		struct ItemTooltipDisplay_t
		{
			Uint32 type;
			int status;
			int beatitude;
			int count;
			Uint32 appearance;
			bool identified;
			Uint32 uid;
			bool wasAppraisalTarget = false;

			int playernum;
			Sint32 playerLVL;
			Sint32 playerEXP;
			Sint32 playerSTR;
			Sint32 playerDEX;
			Sint32 playerCON;
			Sint32 playerINT;
			Sint32 playerPER;
			Sint32 playerCHR;

			int opacitySetpoint = 100;
			real_t opacityAnimate = 1.0;
			int titleOnlyOpacitySetpoint = 100;
			real_t titleOnlyOpacityAnimate = 1.0;
			real_t expandAnimate = 1.0;
			int expandSetpoint = 1;
			real_t expandCurrent = 1.0;
			bool expanded = false;
			real_t frameTooltipScrollAnim = 0.0;
			real_t frameTooltipScrollSetpoint = 0.0;
			int scrolledToMax = 0;
			real_t frameTooltipScrollPrevSetpoint = 0.0;
			bool scrollable = false;

			int tooltipDescriptionHeight = 0;
			int tooltipAttributeHeight = 0;
			int tooltipWidth = 0;
			int tooltipHeight = 0;

			bool isItemSameAsCurrent(const int player, Item* newItem);
			void updateItem(const int player, Item* newItem);
			bool displayingShortFormTooltip = false;
			bool displayingTitleOnlyTooltip = false;
			ItemTooltipDisplay_t();
		};
		ItemTooltipDisplay_t itemTooltipDisplay;

		int DEFAULT_INVENTORY_SIZEX = 12;
		int DEFAULT_INVENTORY_SIZEY = 3;
		static const int MAX_SPELLS_X;
		static const int MAX_SPELLS_Y;
		static const int MAX_CHEST_X;
		static const int MAX_CHEST_Y;
		Inventory_t(Player& p) : 
			player(p), 
			appraisal(p), 
			spellPanel(p),
			chestGUI(p),
			DEFAULT_INVENTORY_SIZEX(12),
			DEFAULT_INVENTORY_SIZEY(3)
		{
			sizex = DEFAULT_INVENTORY_SIZEX;
			sizey = DEFAULT_INVENTORY_SIZEY;
		};
		~Inventory_t() {};
		const int getTotalSize() const { return sizex * sizey; }
		const int getSizeX() const { return sizex; }
		const int getSizeY() const { return sizey; }
		const int getSlotSize() const { return 40; }
		const int getItemSpriteSize() const { return 36; }
		void setSizeY(int size) { sizey = size; }
		void selectSlot(const int x, const int y) { selectedSlotX = x; selectedSlotY = y; }
		const int getSelectedSlotX() const { return selectedSlotX; }
		const int getSelectedSlotY() const { return selectedSlotY; }
		void selectSpell(const int x, const int y) { selectedSpellX = x; selectedSpellY = y; }
		const int getSelectedSpellX() const { return selectedSpellX; }
		const int getSelectedSpellY() const { return selectedSpellY; }
		void selectChestSlot(const int x, const int y);
		const int getSelectedChestX() const { return chestGUI.selectedChestSlotX; }
		const int getSelectedChestY() const { return chestGUI.selectedChestSlotY; }
		const bool isItemFromChest(Item* item) const;
		const bool selectedSlotInPaperDoll() const { return selectedSlotY < 0; }
		bool warpMouseToSelectedItem(Item* snapToItem, Uint32 flags);
		bool warpMouseToSelectedSpell(Item* snapToItem, Uint32 flags);
		bool warpMouseToSelectedChestSlot(Item* snapToItem, Uint32 flags);
		bool guiAllowDropItems(Item* itemToDrop) const;
		bool guiAllowDefaultRightClick() const;
		void processInventory();
		void updateInventory();
		void updateCursor();
		void updateItemContextMenuClickFrame();
		void updateInventoryItemTooltip();
		void updateSelectedItemAnimation();
		void updateItemContextMenu();
		void cycleInventoryTab();
		void activateItemContextMenuOption(Item* item, ItemContextMenuPrompts prompt);
		bool moveItemToFreeInventorySlot(Item* item);
		void resetInventory()
		{
			if ( bNewInventoryLayout )
			{
				DEFAULT_INVENTORY_SIZEX = 5;
				DEFAULT_INVENTORY_SIZEY = 6;
			}
			else
			{
				DEFAULT_INVENTORY_SIZEX = 12;
				DEFAULT_INVENTORY_SIZEY = 3;
			}
			sizex = DEFAULT_INVENTORY_SIZEX;
			sizey = DEFAULT_INVENTORY_SIZEY;
		}
		const int freeVisibleInventorySlots() const
		{
			int x = getPlayerItemInventoryX();
			int y = getPlayerItemInventoryY();
			return x * y;
		}
		const bool bItemInventoryHasFreeSlot() const;
		const int getPlayerItemInventoryX() const;
		const int getPlayerItemInventoryY() const;
		const int getPlayerBackpackBonusSizeY() const
		{
			if ( bNewInventoryLayout )
			{
				return 2;
			}
			return 1;
		}
		void updateSelectedSlotAnimation(int destx, int desty, int width, int height, bool usingMouse);
		Frame* getInventorySlotFrame(int x, int y) const;
		Frame* getSpellSlotFrame(int x, int y) const;
		Frame* getItemSlotFrame(Item* item, int x, int y) const;
		Frame* getChestSlotFrame(int x, int y) const;

		enum PaperDollRows : int
		{
			DOLL_ROW_1 = -5,
			DOLL_ROW_2,
			DOLL_ROW_3,
			DOLL_ROW_4,
			DOLL_ROW_5,
		};
		enum PaperDollColumns : int
		{
			DOLL_COLUMN_LEFT = 0,
			DOLL_COLUMN_RIGHT
		};

		class Appraisal_t
		{
			Player& player;
		public:
			Appraisal_t(Player& p) : player(p) {};
			~Appraisal_t() {};
			int timer = 0; //There is a delay after the appraisal skill is activated before the item is identified.
			int timermax = 0;
			Uint32 current_item = 0; //The item being appraised (or rather its uid)
			Uint32 old_item = 0;
			int getAppraisalTime(Item* item); // Return time in ticks needed to appraise an item
			void appraiseItem(Item* item); // start appraise process
			bool appraisalPossible(Item* item); // if possible with current skill and stats
			real_t animAppraisal = 0.0;
			Uint32 animStartTick = 0;
			Uint32 itemNotifyUpdatedThisTick = 0;
			int itemNotifyAnimState = 0;
			enum ItemNotifyHoverStates : int
			{
				NOTIFY_ITEM_WAITING_TO_HOVER,
				NOTIFY_ITEM_HOVERED,
				NOTIFY_ITEM_REMOVE
			};
			std::unordered_map<Uint32, ItemNotifyHoverStates> itemsToNotify;
			void updateAppraisalAnim();
		} appraisal;
		bool bNewInventoryLayout = true;
	} inventoryUI;

	struct ShopGUI_t
	{
		Player& player;
		Frame* shopFrame = nullptr;
		PanelJustify_t panelJustify = PANEL_JUSTIFY_LEFT;
		bool buybackView = false;
		real_t animx = 0.0;
		bool isInteractable = true;
		bool bOpen = false;
		bool bFirstTimeSnapCursor = false;
		void openShop();
		void closeShop();
		void updateShop();
		Uint32 chatTicks = 0;
		size_t chatStringLength = 0;
		std::string chatStrFull = "";
		Sint32 itemPrice = -1;
		bool itemUnknownPreventPurchase = false;
		std::string itemDesc = "";
		bool itemRequiresTitleReflow = true;
		Sint32 playerCurrentGold = 0;
		Sint32 playerChangeGold = 0;
		real_t animGold = 0.0;
		Uint32 animGoldStartTicks = 0;
		real_t animTooltip = 0.0;
		Uint32 animTooltipTicks = 0;
		real_t animNoDeal = 0.0;
		Uint32 animNoDealTicks = 0;

		int lastTooltipModule = Player::GUI_t::MODULE_NONE;
		int selectedShopSlotX = -1;
		int selectedShopSlotY = -1;
		static const int MAX_SHOP_X;
		static const int MAX_SHOP_Y;
		std::unordered_map<int, Frame*> shopSlotFrames;
		bool isShopSelected();
		void selectShopSlot(const int x, const int y);
		const int getSelectedShopX() const { return selectedShopSlotX; }
		const int getSelectedShopY() const { return selectedShopSlotY; }
		Frame* getShopSlotFrame(int x, int y) const;
		const bool isItemFromShop(Item* item) const;
		void setItemDisplayNameAndPrice(Item* item);
		const bool isItemSelectedFromShop(Item* item) const;
		const bool isItemSelectedToSellToShop(Item* item) const;
		bool warpMouseToSelectedShopItem(Item* snapToItem, Uint32 flags);
		void clearItemDisplayed();

		static int heightOffsetWhenNotCompact;
		ShopGUI_t(Player& p) :
			player(p) {}
	};
	ShopGUI_t shopGUI;

	class BookGUI_t
	{
		Player& player;
	public:
		static const int BOOK_PAGE_WIDTH = 220;
		static const int BOOK_PAGE_HEIGHT = 260;
		BookGUI_t(Player& p) : player(p)
		{};
		~BookGUI_t() {};

		real_t bookFadeInAnimationY = 0.0;

		Frame* bookFrame = nullptr;
		int offsetx = 0;
		int offsety = 0;
		bool bBookOpen = false;
		Item* openBookItem = nullptr;
		std::string openBookName = "";
		int currentBookPage = 0;
		void updateBookGUI();
		void closeBookGUI();
		void createBookGUI();
		void openBook(int index, Item* item);
	} bookGUI;

	class SignGUI_t
	{
		Player& player;
	public:
		static const int SIGN_WIDTH = 220;
		static const int SIGN_HEIGHT = 260;
		SignGUI_t(Player& p) : player(p)
		{};
		~SignGUI_t() {};

		real_t signFadeInAnimationY = 0.0;
		real_t signAnimVideo = 0.0;
		real_t signWorldCoordX = 0.0;
		real_t signWorldCoordY = 0.0;
		Frame* signFrame = nullptr;
		bool bSignOpen = false;
		std::string signName = "";
		int currentSignPage = 0;
		void updateSignGUI();
		void closeSignGUI();
		void createSignGUI();
		void openSign(std::string name, Uint32 uid);
		Uint32 signUID = 0;
	} signGUI;

	class CharacterSheet_t
	{
		Player& player;
	public:
		CharacterSheet_t(Player& p) : player(p)
		{};
		~CharacterSheet_t() {};
		SDL_Rect skillsSheetBox;
		SDL_Rect partySheetBox;
		SDL_Rect characterSheetBox;
		SDL_Rect statsSheetBox;

		Player::PanelJustify_t panelJustify = PANEL_JUSTIFY_RIGHT;

		void setDefaultSkillsSheetBox();
		void setDefaultPartySheetBox();
		void setDefaultCharacterSheetBox();

		bool lock_right_sidebar = false;
		int proficienciesPage = 0;
		int attributespage = 0;
		bool showGameTimerAlways = false;
		bool isInteractable = false;
		int tooltipOpacitySetpoint = 100;
		real_t tooltipOpacityAnimate = 1.0;
		Uint32 tooltipDeselectedTick = 0;

		static std::map<std::string, std::pair<std::string, std::string>> mapDisplayNamesDescriptions;
		static std::map<std::string, std::string> hoverTextStrings;
		enum SheetElements
		{
			SHEET_UNSELECTED,
			SHEET_OPEN_LOG,
			SHEET_OPEN_MAP,
			SHEET_TIMER,
			SHEET_GOLD,
			SHEET_DUNGEON_FLOOR,
			SHEET_CHAR_CLASS,
			SHEET_CHAR_RACE_SEX,
			SHEET_SKILL_LIST,
			SHEET_STR,
			SHEET_DEX,
			SHEET_CON,
			SHEET_INT,
			SHEET_PER,
			SHEET_CHR,
			SHEET_ATK,
			SHEET_AC,
			SHEET_POW,
			SHEET_RES,
			SHEET_RGN,
			SHEET_RGN_MP,
			SHEET_WGT,
			SHEET_ENUM_END
		};
		enum SheetDisplay
		{
			CHARSHEET_DISPLAY_NORMAL,
			CHARSHEET_DISPLAY_COMPACT
		};
		bool isSheetElementAllowedToNavigateTo(SheetElements element);
		SheetDisplay sheetDisplayType = CHARSHEET_DISPLAY_NORMAL;
		Frame* sheetFrame = nullptr;
		SheetElements selectedElement = SHEET_UNSELECTED;
		SheetElements queuedElement = SHEET_UNSELECTED;
		SheetElements cachedElementTooltip = SHEET_UNSELECTED;
		void selectElement(SheetElements element, bool usingMouse, bool moveCursor = false);
		void createCharacterSheet();
		void processCharacterSheet();
		void updateGameTimer();
		void updateStats();
		void updateAttributes();
		void updateCharacterInfo();
		static void loadCharacterSheetJSON();
		static std::string defaultString;
		static std::string& getHoverTextString(std::string key);
		void updateCharacterSheetTooltip(SheetElements element, SDL_Rect pos, Player::PanelJustify_t tooltipJustify = PANEL_JUSTIFY_RIGHT);
	} characterSheet;

	class SkillSheet_t
	{
		Player& player;
	public:
		SkillSheet_t(Player& p) : player(p)
		{};
		~SkillSheet_t() {};

		Frame* skillFrame = nullptr;
		int selectedSkill = 0;
		int highlightedSkill = 0;
		real_t skillsFadeInAnimationY = 0.0;
		bool bSkillSheetOpen = false;
		Uint32 openTick = 0;
		bool bSkillSheetEntryLoaded = false;
		real_t scrollPercent = 0.0;
		real_t scrollInertia = 0.0;
		int skillSlideDirection = 0;
		real_t skillSlideAmount = 0.0;
		bool bUseCompactSkillsView = false;
		bool bSlideWindowsOnly = false;
		static real_t windowHeightScaleX;
		static real_t windowHeightScaleY;
		static real_t windowCompactHeightScaleX;
		static real_t windowCompactHeightScaleY;
		static bool generateFollowerTableForSkillsheet;
		static struct SkillSheetData_t
		{
			Uint32 defaultTextColor = 0xFFFFFFFF;
			Uint32 noviceTextColor = 0xFFFFFFFF;
			Uint32 expertTextColor = 0xFFFFFFFF;
			Uint32 legendTextColor = 0xFFFFFFFF;
			struct SkillEntry_t
			{
				SkillEntry_t() {};
				~SkillEntry_t() {};
				std::string name;
				int skillId = -1;
				std::string skillIconPath;
				std::string skillIconPathLegend;
				std::string skillIconPath32px;
				std::string skillIconPathLegend32px;
				std::string statIconPath;
				std::string description;
				std::string legendaryDescription;
				int skillSfx = 552;
				int effectStartOffsetX = 72;
				int effectBackgroundOffsetX = 8;
				int effectBackgroundWidth = 80;
				struct SkillEffect_t
				{
					SkillEffect_t() {};
					~SkillEffect_t() {};
					std::string tag;
					std::string title;
					std::string rawValue;
					std::string value;
					int valueCustomWidthOffset = 0;
					bool bAllowAutoResizeValue = false;
					bool bAllowRealtimeUpdate = false;
					real_t marquee[MAXPLAYERS] = { 0.0 };
					Uint32 marqueeTicks[MAXPLAYERS] = { 0 };
					bool marqueeCompleted[MAXPLAYERS] = { false };
					int effectUpdatedAtSkillLevel = -1;
					int effectUpdatedAtMonsterType = -1;
					int cachedWidth = -1;
				};
				std::vector<SkillEffect_t> effects;
			};
			std::vector<SkillEntry_t> skillEntries;
			std::string iconBgPathDefault = "";
			std::string iconBgPathNovice = "";
			std::string iconBgPathExpert = "";
			std::string iconBgPathLegend = "";
			std::string iconBgSelectedPathDefault = "";
			std::string iconBgSelectedPathNovice = "";
			std::string iconBgSelectedPathExpert = "";
			std::string iconBgSelectedPathLegend = "";
			std::string highlightSkillImg = "";
			std::string selectSkillImg = "";
			std::string highlightSkillImg_Right = "";
			std::string selectSkillImg_Right = "";
			std::vector<std::string> potionNamesToFilter;
			std::map<Monster, std::vector<Monster>> leadershipAllyTableBase;
			std::map<Monster, std::vector<Monster>> leadershipAllyTableLegendary;
			std::map<Monster, std::vector<std::pair<Monster, std::string>>> leadershipAllyTableSpecialRecruitment;
		} skillSheetData;

		void selectSkill(int skill);
		void createSkillSheet();
		void processSkillSheet();
		void closeSkillSheet();
		void openSkillSheet();
		void resetSkillDisplay();
		static void loadSkillSheetJSON();
	} skillSheet;

	class HUD_t
	{
		Player& player;
	public:
	    Frame* controllerFrame = nullptr;
		Frame* hudFrame = nullptr;
		Frame* xpFrame = nullptr;
		Frame* levelupFrame = nullptr;
		Frame* skillupFrame = nullptr;
		Frame* hpFrame = nullptr;
		Frame* mpFrame = nullptr;
		Frame* minimapFrame = nullptr;
		Frame* gameTimerFrame = nullptr;
		Frame* allyStatusFrame = nullptr;
		Frame* minotaurFrame = nullptr;
		Frame* minotaurSharedDisplay = nullptr;
		Frame* minotaurDisplay = nullptr;
		Frame* mapPromptFrame = nullptr;
		Frame* allyFollowerFrame = nullptr;
		Frame* allyFollowerTitleFrame = nullptr;
		Frame* allyFollowerGlyphFrame = nullptr;
		Frame* allyPlayerFrame = nullptr;
		Frame* enemyBarFrame = nullptr;
		Frame* enemyBarFrameHUD = nullptr;
		Frame* actionPromptsFrame = nullptr;
		Frame* worldTooltipFrame = nullptr;
		Frame* statusEffectFocusedWindow = nullptr;
		Frame* uiNavFrame = nullptr;
		Frame* cursorFrame = nullptr;
		real_t hudDamageTextVelocityX = 0.0;
		real_t hudDamageTextVelocityY = 0.0;
		real_t animHideXP = 0.0;

		Entity* weapon = nullptr;
		Entity* arm = nullptr;
		Entity* magicLeftHand = nullptr;
		Entity* magicRightHand = nullptr;

		bool weaponSwitch = false;
		bool shieldSwitch = false;

		Sint32 throwGimpTimer = 0; // player cannot throw objects unless zero
		Sint32 pickaxeGimpTimer = 0; // player cannot swap weapons immediately after using pickaxe 
									 // due to multiplayer weapon degrade lag... equipping new weapon before degrade
									 // message hits can degrade the wrong weapon.
		Sint32 swapWeaponGimpTimer = 0; // player cannot swap weapons unless zero
		Sint32 bowGimpTimer = 0; // can't draw bow unless zero.

		bool bowFire = false;
		bool bowIsBeingDrawn = false;
		Uint32 bowStartDrawingTick = 0;
		Uint32 bowDrawBaseTicks = 50;
#ifdef USE_FMOD
		FMOD::Channel* bowDrawingSoundChannel = NULL;
		bool bowDrawingSoundPlaying = 0;
#elif defined USE_OPENAL
		OPENAL_SOUND* bowDrawingSoundChannel = NULL;
		ALboolean bowDrawingSoundPlaying = 0;
#endif
		struct Cursor_t
		{
			real_t animateX = 0.0;
			real_t animateY = 0.0;
			real_t animateW = 0.0;
			real_t animateH = 0.0;
			int animateSetpointX = 0;
			int animateSetpointY = 0;
			int animateSetpointW = 0;
			int animateSetpointH = 0;
			int animateStartX = 0;
			int animateStartY = 0;
			int animateStartW = 0;
			int animateStartH = 0;
			Uint32 lastUpdateTick = 0;
			const int cursorToSlotOffset = 7;
		};
		Cursor_t cursor;

		enum AnimateStates : int {
			ANIMATE_NONE,
			ANIMATE_MOVING,
			ANIMATE_LEVELUP_RISING,
			ANIMATE_LEVELUP_FALLING
		};

		enum AnimateFlashEffects_t : int {
			FLASH_ON_DAMAGE,
			FLASH_ON_RECOVERY
		};

		struct Bar_t
		{
			real_t animateValue = 0.0;
			real_t animateValue2 = 0.0;
			real_t animatePreviousSetpoint = 0.0;
			Sint32 animateSetpoint = 0;
			Uint32 animateTicks = 0;
			AnimateStates animateState = ANIMATE_NONE;
			Uint32 xpLevelups = 0;
			real_t maxValue = 0.0;
			real_t fadeIn = 0.0;
			real_t fadeOut = 0.0;
			real_t widthMultiplier = 1.0;

			Uint32 flashTicks = 0;
			Uint32 flashProcessedOnTick = 0;
			int flashAnimState = -1;
			AnimateFlashEffects_t flashType = FLASH_ON_DAMAGE;
		};
		struct XPInfo_t
		{
			enum XPCycleInfo : int
			{
				CYCLE_NONE,
				CYCLE_LVL,
				CYCLE_XP
			};
			XPCycleInfo cycleStatus = CYCLE_NONE;
			real_t fade = 1.0;
			Uint32 cycleTicks = 0;
			Uint32 cycleProcessedOnTick = 0;
			bool fadeIn = true;
		};
		XPInfo_t xpInfo;
		struct InteractPrompt_t
		{
			real_t promptAnim = 0.0;
			Uint32 activeTicks = 0;
			Uint32 processedOnTick = 0;
			real_t cycleAnim = 1.0;
		};
		InteractPrompt_t interactPrompt;
		Bar_t xpBar;
		Bar_t HPBar;
		Bar_t MPBar;
		Bar_t enemyBar;
		struct FollowerBar_t
		{
			Bar_t hpBar;
			Bar_t mpBar;
			real_t animx = 0.0;
			real_t animy = 0.0;
			bool expired = false;
			Uint32 expiredTicks = 0;
			real_t animFade = 0.0;
			real_t animFadeScroll = 0.0;
			real_t animFadeScrollDummy = 0.0;
			bool bInit = false;
			std::string name = "";
			std::string customPortraitPath = "";
			int level = 0;
			int model = 0;
			int monsterType = 0;
			bool selected = false;
			bool dummy = false;
		};
		struct FollowerDisplay_t
		{
			static bool infiniteScrolling;
			static int getNumEntriesToShow(const int playernum);
			bool getCompactMode(const int playernum);
			static int numFiniteBars;
			static int numInfiniteFullsizeBars;
			static int numInfiniteCompactBars;
			static int numInfiniteSplitscreenFullsizeBars;
			static int numInfiniteSplitscreenCompactBars;
			real_t scrollPercent = 0.0;
			real_t scrollInertia = 0.0;
			int scrollSetpoint = 0;
			int currentScrollRow = 0;
			real_t scrollAnimateX = 0.0;
			Uint32 lastUidSelected = 0;
			real_t animSelected = 0.0;
			Uint32 scrollTicks = 0;
			bool isInteractable = false;
			bool bCompact = false;
			bool bHalfWidthBars = false;
			bool bCycleNextDisabled = false;
			bool bCommandNPCDisabled = false;
			bool bOpenFollowerMenuDisabled = false;
		};
		FollowerDisplay_t followerDisplay;
		std::vector<std::pair<Uint32, FollowerBar_t>> followerBars;
		std::vector<std::pair<Uint32, FollowerBar_t>> playerBars;

		enum CompactLayoutModes : int {
			COMPACT_LAYOUT_INVENTORY,
			COMPACT_LAYOUT_CHARSHEET
		};
		CompactLayoutModes compactLayoutMode = COMPACT_LAYOUT_CHARSHEET;
		bool bShowUINavigation = false;
		void closeStatusFxWindow();
		bool statusFxFocusedWindowActive = false;

		HUD_t(Player& p) : player(p)
		{};
		~HUD_t() {};

		void reset()
		{
			swapWeaponGimpTimer = 0;
			bowGimpTimer = 0;
			throwGimpTimer = 0;
			pickaxeGimpTimer = 0;
			bowFire = false;
			bowIsBeingDrawn = false;
			bowStartDrawingTick = 0;
			bowDrawBaseTicks = 50;
			weaponSwitch = false;
			shieldSwitch = false;
		}
		bool bShowActionPrompts = true;
		bool bShortHPMPForActionBars = false;
		enum ActionPrompts : int
		{
			ACTION_PROMPT_MAINHAND,
			ACTION_PROMPT_OFFHAND,
			ACTION_PROMPT_MAGIC,
			ACTION_PROMPT_SNEAK
		};
		const int getActionIconForPlayer(ActionPrompts prompt, std::string& promptString) const;
		void processHUD();
		void updateGameTimer();
		void updateMinimapPrompts();
		int XP_FRAME_WIDTH = 650;
		int XP_FRAME_START_Y = 44;
		int XP_FRAME_HEIGHT = 34;
		void updateXPBar();
		int HPMP_FRAME_WIDTH = 276;
		int HPMP_FRAME_START_X = 14;
		int HPMP_FRAME_START_Y = 106;
		int HPMP_FRAME_HEIGHT = 34;
		void updateHPBar();
		void updateMPBar();
		const int ENEMYBAR_FRAME_WIDTH = 564;
		const int ENEMYBAR_BAR_WIDTH = 556;
		const int ENEMYBAR_FRAME_START_Y = 182;
		const int ENEMYBAR_FRAME_HEIGHT = 44;
		static int actionPromptOffsetX;
		static int actionPromptOffsetY;
		static int actionPromptBackingSize;
		static int actionPromptIconSize;
		static int actionPromptIconOpacity;
		static int actionPromptIconBackingOpacity;
		int offsetHUDAboveHotbarHeight = 0;
		void updateEnemyBar(Frame* whichFrame);
		void updateEnemyBar2(Frame* whichFrame, void* enemyHPDetails);
		void resetBars();
		void updateFrameTooltip(Item* item, const int x, const int y, int justify);
        void finalizeFrameTooltip(Item* item, const int x, const int y, int justify);
		void updateStatusEffectTooltip();
		void updateCursor();
		void updateActionPrompts();
		void updateWorldTooltipPrompts();
		void updateUINavigation();
		void updateMinotaurWarning();
		void updateStatusEffectFocusedWindow();
		void updateCursorAnimation(int destx, int desty, int width, int height, bool usingMouse);
		void setCursorDisabled(bool disabled) { if ( cursorFrame ) { cursorFrame->setDisabled(disabled); } };
		const char* getCrosshairPath();
	} hud;

	class Magic_t
	{
		Player& player;
		spell_t* selected_spell = nullptr; //The spell the player has currently selected.
		spell_t* quick_cast_spell = nullptr; //Spell ready for quick-casting
	public:
		spell_t* selected_spell_alternate[NUM_HOTBAR_ALTERNATES] = { nullptr, nullptr, nullptr, nullptr, nullptr };
		int selected_spell_last_appearance = -1;
		list_t spellList; //All of the player's spells are stored here.
		bool bHasUnreadNewSpell = false;
		Uint32 noManaFeedbackTicks = 0;
		Uint32 noManaProcessedOnTick = 0;
		Uint32 spellbookUidFromHotbarSlot = 0;
		void flashNoMana()
		{
			noManaFeedbackTicks = 0;
			noManaProcessedOnTick = ticks;
		}
		Magic_t(Player& p) : player(p)
		{
			spellList.first = nullptr;
			spellList.last = nullptr;
		};
		~Magic_t() {};
		void clearSelectedSpells()
		{
			selected_spell = nullptr;
			for ( int c = 0; c < NUM_HOTBAR_ALTERNATES; ++c )
			{
				selected_spell_alternate[c] = nullptr;
			}
			selected_spell_last_appearance = -1;
			quick_cast_spell = nullptr;
		}
		void equipSpell(spell_t* spell) 
		{ 
			selected_spell = spell; 
		}
		void setQuickCastSpellFromInventory(Item* item);
		bool doQuickCastSpell() { return quick_cast_spell != nullptr; }
		void resetQuickCastSpell() { quick_cast_spell = nullptr; }
		spell_t* selectedSpell() const { return selected_spell; }
		spell_t* quickCastSpell() const { return quick_cast_spell; }

	} magic;
	class PlayerSettings_t
	{
	public:
		PlayerSettings_t() {};
		~PlayerSettings_t() {};
		int quickTurnDirection = 1; // 1 == right, -1 == left
		real_t quickTurnSpeed = PI / 15;
	} settings;
	
	class PlayerMovement_t
	{
		real_t quickTurnRotation = 0.0;
		Uint32 quickTurnStartTicks = 0;
		bool bDoingQuickTurn = false;
		Player& player;
	public:
		PlayerMovement_t(Player& p) : player(p)
		{};
		~PlayerMovement_t() {};
		
		int monsterEmoteGimpTimer = 0;
		int selectedEntityGimpTimer = 0;
		bool insectoidLevitating = false;

		bool handleQuickTurn(bool useRefreshRateDelta);
		void startQuickTurn();
		bool isPlayerSwimming();
		void handlePlayerCameraUpdate(bool useRefreshRateDelta);
		void handlePlayerCameraBobbing(bool useRefreshRateDelta);
		void handlePlayerMovement(bool useRefreshRateDelta);
		real_t getMaximumSpeed();
		real_t getWeightRatio(int weight, Sint32 STR);
		int getCharacterWeight();
		int getCharacterEquippedWeight();
		int getCharacterModifiedWeight(int* customWeight = nullptr);
		real_t getSpeedFactor(real_t weightratio, Sint32 DEX);
		real_t getCurrentMovementSpeed();
		void handlePlayerCameraPosition(bool useRefreshRateDelta);
		void reset();
	} movement;

	class MessageZone_t
	{
		//Time in seconds before the message starts fading.
		static const int MESSAGE_PREFADE_TIME = 3600;
		//How fast the alpha value de-increments
		static const int MESSAGE_FADE_RATE = 10;
		TTF_Font* font = ttf16;
		Uint32 old_sdl_ticks = 0;
		Player& player;
	public:
		static const int ADD_MESSAGE_BUFFER_LENGTH = 256;
		MessageZone_t(Player& p) : player(p) {};
		~MessageZone_t() {};
		std::list<Message*> notification_messages;
		//Init old_sdl_ticks to determine when to fade messages
		static void startMessages();
		//Adds a message to the list of messages.
		void addMessage(Uint32 color, const char* content);
		//Updates all the messages; fades them & removes them.
		void updateMessages();
		//Draw all the messages.
		void drawMessages();
		//Used on program deinitialization.
		void deleteAllNotificationMessages();

		//Maximum number of messages displayed on screen at once before the oldest message is automatically deleted.
		int getMaxTotalLines();

		int MESSAGE_MAX_ENTRIES = 20;
		Frame* chatFrame = nullptr;
		real_t animFade = 1.0;
		bool bottomAlignedMessages = false;
		bool useBigFont = false;
		static const char* bigfont;
		static const char* smallfont;
		enum ChatAlignment_t : int
		{
			ALIGN_CENTER_BOTTOM,
			ALIGN_LEFT_BOTTOM,
			ALIGN_LEFT_TOP
		};
		ChatAlignment_t messageAlignment = ALIGN_CENTER_BOTTOM;
		ChatAlignment_t actualAlignment = ALIGN_CENTER_BOTTOM;
		void createChatbox();
		void processChatbox();

		Frame* logParentFrame = nullptr;
		Frame* logWindow = nullptr;
		void processLogFrame();
		int fontSize() { return getHeightOfFont(font); }
	} messageZone;

	class WorldUI_t
	{
		Player& player;
		bool bEnabled = true;
		static const int UID_TOOLTIP_ACTIVE = -21;
		static const int UID_TOOLTIP_DISABLED = -20;
		enum TooltipView
		{
			TOOLTIP_VIEW_FREE,
			TOOLTIP_VIEW_LOCKED,
			TOOLTIP_VIEW_RESCAN
		};
	public:
		struct WorldTooltipItem_t
		{
			Player& player;
			WorldTooltipItem_t(Player& p) : player(p)
			{};
			~WorldTooltipItem_t() 
			{
				if ( itemFrame )
				{
					delete itemFrame;
					itemFrame = nullptr;
				}
				if ( itemWorldTooltipSurface )
				{
					SDL_FreeSurface(itemWorldTooltipSurface);
					itemWorldTooltipSurface = nullptr;
				}
			};
			Uint32 type = WOODEN_SHIELD;
			int status = 0;
			int beatitude = -99;
			int count = 0;
			Uint32 appearance = 0;
			bool identified = false;
			bool isItemSameAsCurrent(Item* item);
			SDL_Surface* blitItemWorldTooltip(Item* item);
			SDL_Surface* itemWorldTooltipSurface = nullptr;
			Frame* itemFrame = nullptr;

			struct WorldItemSettings_t
			{
				static real_t scaleMod;
				static real_t opacity;
			};
		} worldTooltipItem;

		struct WorldTooltipDialogue_t
		{
			Player& player;
			enum DialogueType_t
			{
				DIALOGUE_NONE,
				DIALOGUE_NPC,
				DIALOGUE_GRAVE,
				DIALOGUE_SIGNPOST,
				DIALOGUE_FOLLOWER_CMD,
				DIALOGUE_BROADCAST,
				DIALOGUE_ATTACK
			};
			struct Dialogue_t
			{
				int player = -1;
				Uint32 parent = 0;
				real_t x = 0.0;
				real_t y = 0.0;
				real_t z = 0.0;
				bool active = false;
				bool draw = false;
				bool init = false;
				real_t animZ = 0.0;
				real_t alpha = 0.0;
				real_t drawScale = 0.0;
				Uint32 spawnTick = 0;
				Uint32 updatedThisTick = 0;
				Uint32 expiryTicks = 0;
				Field* dialogueField = nullptr;
				size_t dialogueStringLength = 0;
				std::string dialogueStrFull = "";
				std::string dialogueStrCurrent = "";
				void deactivate();
				void update();
				DialogueType_t dialogueType = DIALOGUE_NONE;
				SDL_Surface* blitDialogueTooltip();
				SDL_Surface* dialogueTooltipSurface = nullptr;
				Dialogue_t() {};
				Dialogue_t(int player)
				{
					this->player = player;
				};
				~Dialogue_t() 
				{
					if ( dialogueField )
					{
						delete dialogueField;
						dialogueField = nullptr;
					}
					if ( dialogueTooltipSurface )
					{
						SDL_FreeSurface(dialogueTooltipSurface);
						dialogueTooltipSurface = nullptr;
					}
				};
			};
			Dialogue_t playerDialogue;
			std::map<Uint32, Dialogue_t> sharedDialogues;
			int getPlayerNum() { return player.playernum; }
			WorldTooltipDialogue_t(Player& p) : player(p)
			{};
			~WorldTooltipDialogue_t() {};
			void update();
			void createDialogueTooltip(Uint32 uid, DialogueType_t type, char const * const message, ...);
			struct WorldDialogueSettings_t
			{
				struct Setting_t
				{
					real_t offsetZ = 0.0;
					int textDelay = 0;
					bool followEntity = false;
					real_t fadeDist = 32.0; // 2 tiles radius
					Uint32 baseTicksToDisplay = TICKS_PER_SECOND * 3;
					Uint32 extraTicksPerLine = TICKS_PER_SECOND * 2;
					int maxWidth = 300;
					int padx = 8;
					int pady = 8;
					int padAfterFirstLine = 0;
					real_t scaleMod = 0.0;
				};
				static std::map<Player::WorldUI_t::WorldTooltipDialogue_t::DialogueType_t, Setting_t> settings;
			};
		} worldTooltipDialogue;

		WorldUI_t(Player& p) : 
			player(p),
			worldTooltipItem(p),
			worldTooltipDialogue(p)
		{};
		~WorldUI_t() {};
		TooltipView tooltipView = TOOLTIP_VIEW_FREE;
		std::vector<std::pair<Entity*, real_t>> tooltipsInRange;
		static real_t tooltipHeightOffsetZ;
		real_t playerLastYaw = 0.0;
		int gimpDisplayTimer = 0;
		void reset();
		void setTooltipActive(Entity& tooltip);
		void setTooltipDisabled(Entity& tooltip);
		bool bTooltipActiveForPlayer(Entity& tooltip);
		bool bTooltipInView = false;
		Uint32 uidForActiveTooltip = 0;
		std::string interactText = "Interact";
		void enable() { bEnabled = true; }
		void disable() { 
			bEnabled = false; 
			reset();
		}
		bool isEnabled() const { return bEnabled; }
		static void handleTooltips();
		real_t tooltipInRange(Entity& tooltip); // returns distance of added tooltip, otherwise 0.
		void cycleToNextTooltip();
		void cycleToPreviousTooltip();

	} worldUI;

	class PaperDoll_t
	{
		Player& player;
		static const Uint32 kNumPaperDollSlots = 10;
	public:
		bool enabled = true;
		static const int ITEM_PAPERDOLL_COORDINATE = -9999;
		static const int ITEM_RETURN_TO_INVENTORY_COORDINATE = -99999;
		PaperDoll_t(Player& p) : player(p)
		{
			initSlots();
		};
		~PaperDoll_t() {};
		enum PaperDollSlotType : int
		{
			SLOT_GLASSES,
			SLOT_CLOAK,
			SLOT_AMULET,
			SLOT_RING,
			SLOT_OFFHAND,
			SLOT_HELM,
			SLOT_BREASTPLATE,
			SLOT_GLOVES,
			SLOT_BOOTS,
			SLOT_WEAPON,
			SLOT_MAX
		};
		struct PaperDollSlot_t
		{
			Uint32 item;
			PaperDollSlotType slotType;
			PaperDollSlot_t()
			{
				item = 0;
				slotType = SLOT_MAX;
			}
		};
		std::array<PaperDollSlot_t, kNumPaperDollSlots> dollSlots;
		//const int getSlotSize() const;
		void initSlots()
		{
			returningItemsToInventory.clear();
			for ( int i = 0; i < kNumPaperDollSlots; ++i )
			{
				dollSlots[i].item = 0;
				dollSlots[i].slotType = static_cast<PaperDollSlotType>(i);
			}
		}
		void clear()
		{
			returningItemsToInventory.clear();
			for ( int i = 0; i < kNumPaperDollSlots; ++i )
			{
				dollSlots[i].item = 0;
			}
		}
		void drawSlots();
		void updateSlots();
		PaperDollSlotType getSlotForItem(const Item& item) const;
		bool isItemOnDoll(const Item& item) const { return getSlotForItem(item) != SLOT_MAX; }
		PaperDollSlotType paperDollSlotFromCoordinates(int x, int y) const;
		void getCoordinatesFromSlotType(PaperDollSlotType slot, int& outx, int& outy) const;
		void selectPaperDollCoordinatesFromSlotType(PaperDollSlotType slot) const;
		std::vector<Uint32> returningItemsToInventory;
		void warpMouseToMostRecentReturnedInventoryItem();
		bool portraitActiveToEdit = false;
		real_t portraitRotationInertia = 0.0;
		real_t portraitRotationPercent = 0.0;
		real_t portraitYaw = (330) * PI / 180;
		void resetPortrait()
		{
			portraitRotationInertia = 0.0;
			portraitRotationPercent = 0.0;
			portraitYaw = (330) * PI / 180;
		}
	} paperDoll;

	class Hotbar_t {
		std::array<hotbar_slot_t, NUM_HOTBAR_SLOTS> hotbar;
		std::array<std::array<hotbar_slot_t, NUM_HOTBAR_SLOTS>, NUM_HOTBAR_ALTERNATES> hotbar_alternate;
		Player& player;
	public:
		std::array<Frame*, NUM_HOTBAR_SLOTS> hotbarSlotFrames;
		int current_hotbar = 0;
		bool hotbarShapeshiftInit[NUM_HOTBAR_ALTERNATES] = { false, false, false, false, false };
		int swapHotbarOnShapeshift = 0;
		bool hotbarHasFocus = false;
		int magicBoomerangHotbarSlot = -1;
		Uint32 hotbarTooltipLastGameTick = 0;
		SDL_Rect hotbarBox;
		Frame* hotbarFrame = nullptr;
		real_t selectedSlotAnimateCurrentValue = 0.0;
		bool isInteractable = false;

		struct Cursor_t
		{
			real_t animateX = 0.0;
			real_t animateY = 0.0;
			int animateSetpointX = 0;
			int animateSetpointY = 0;
			int animateStartX = 0;
			int animateStartY = 0;
			Uint32 lastUpdateTick = 0;
			const int cursorToSlotOffset = 7;
		};
		Cursor_t shootmodeCursor;

		// temp stuff
		bool useHotbarRadialMenu = false;
		bool useHotbarFaceMenu = true;
		bool faceMenuInvertLayout = false;
		bool faceMenuQuickCastEnabled = false;
		bool faceMenuQuickCast = true;
		bool faceMenuAlternateLayout = false;
		enum FaceMenuGroup : int
		{
			GROUP_NONE,
			GROUP_LEFT,
			GROUP_MIDDLE,
			GROUP_RIGHT
		};
		FaceMenuGroup faceMenuButtonHeld = GROUP_NONE;
		int faceButtonTopYPosition = yres;
		int radialHotbarSlots = NUM_HOTBAR_SLOTS;
		int radialHotbarProgress = 0;
		int oldSlotFrameTrackSlot = -1;
		
		real_t animHide = 0.0;

		std::array<SDL_Rect, NUM_HOTBAR_SLOTS> faceButtonPositions;
		const int getSlotSize() const { return 48; }
		const int getHotbarStartY1() const { return -106; }
		const int getHotbarStartY2() const { return -96; }

		Hotbar_t(Player& p) : player(p)
		{
			for ( int i = 0; i < NUM_HOTBAR_SLOTS; ++i )
			{
				hotbarSlotFrames[i] = nullptr;
			}
			clear();
		};
		~Hotbar_t() {};

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
			faceButtonTopYPosition = yres;
			swapHotbarOnShapeshift = 0;
			current_hotbar = 0;
			//hotbarHasFocus = false;
			magicBoomerangHotbarSlot = -1;
			hotbarTooltipLastGameTick = 0;
			for ( int j = 0; j < NUM_HOTBAR_ALTERNATES; ++j )
			{
				hotbarShapeshiftInit[j] = false;
			}
			for ( int i = 0; i < NUM_HOTBAR_SLOTS; ++i )
			{
				hotbar[i].item = 0;
				hotbar[i].resetLastItem();
				for ( int j = 0; j < NUM_HOTBAR_ALTERNATES; ++j )
				{
					hotbar_alternate[j][i].item = 0;
					hotbar_alternate[j][i].resetLastItem();
				}
			}
		}

		auto& slots() { return hotbar; };
		auto& slotsAlternate(int alternate) { return hotbar_alternate[alternate]; };
		auto& slotsAlternate() { return hotbar_alternate; }
		void selectHotbarSlot(int slot);
		void initFaceButtonHotbar();
		FaceMenuGroup getFaceMenuGroupForSlot(int hotbarSlot);
		void processHotbar();
		void updateHotbar();
		Frame* getHotbarSlotFrame(const int hotbarSlot);
		bool warpMouseToHotbar(const int hotbarSlot, Uint32 flags);
		void updateSelectedSlotAnimation(int destx, int desty, int width, int height, bool usingMouse);
		void updateCursor();
	} hotbar;

	class Minimap_t
	{
		Player& player;
	public:
		static std::vector<std::pair<std::string, std::string>> mapDetails;
		Minimap_t(Player& p) : player(p)
		{};
		~Minimap_t() {};

		bool big = false;           // "big" mode (centered)
		real_t real_scale = 0.0;    // canonical scale
		real_t scale = 0.0;         // momentary scale
		real_t scale_ang = 0.0;     // used to interpolate
		bool animating = false;		// if in the middle of scaling animation
		SDL_Rect minimapPos;
		static SDL_Rect sharedMinimapPos;
		Frame* mapParentFrame = nullptr;
		Frame* mapWindow = nullptr;
		bool bScalePromptEnabled = false;
		bool bExpandPromptEnabled = false;
		void processMapFrame();
		static int fullSize;
		static int compactSize;
		static bool bUpdateMainMenuSettingScale;
		static real_t mainMenuSettingScale;
		static int compact2pVerticalSize;
		static real_t fullBigScale;
		static real_t compactBigScale;
		static real_t compact2pVerticalBigScale;
	} minimap;

	static void soundMovement();
	static void soundActivate();
	static void soundCancel();
	static void soundModuleNavigation();
	static void soundStatusOpen();
	static void soundStatusClose();
	static void soundHotbarShootmodeMovement();
};

extern Player* players[MAXPLAYERS];
//In the process of switching from the old entity player array, all of the old uses of player need to be hunted down and then corrected to account for the new array.
//So, search for the comment:
//TODO: PLAYERSWAP
//and fix and verify that the information is correct.
//Then, once all have been fixed and verified, uncomment this declaration, and the accompanying definition in player.cpp; uncomment all of the TODO: PLAYERSWAP code segments, and attempt compilation and running.
