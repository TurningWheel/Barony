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
#ifdef USE_FMOD
 #include "fmod.h"
#else
 #include "sound.hpp"
#endif


//Splitscreen support stuff.

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
	SDL_Haptic* sdl_haptic;
	int id;
	std::string name;
	static const int BUTTON_HELD_TICKS = TICKS_PER_SECOND / 4;
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
		SDL_HapticEffect hapticEffect;
		Uint32 hapticTick;
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
		UP,
		UPLEFT,
		LEFT,
		DOWNLEFT,
		DOWN,
		DOWNRIGHT,
		RIGHT,
		UPRIGHT
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
	//If c < 0 or c >= SDL_NumJoysticks() or c is not a game controller, then returns false.
	bool open(int c);

	void initBindings();
	const int getID() { return id; }
	const SDL_GameController* getControllerDevice() { return sdl_device; }
	SDL_Haptic* getHaptic() { return sdl_haptic; }
	const bool isActive();
	void addRumble(Haptic_t::RumblePattern pattern, Uint16 smallMagnitude, Uint16 largeMagnitude, Uint32 length, Uint32 srcEntityUid);
	void doRumble(Haptic_t::Rumble* r);
	void stopRumble();
	void handleRumble();

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

		bool draw_cursor = true; //True if the gamepad's d-pad has been used to navigate menus and such. //TODO: Off by default on consoles and the like.
		bool moved = false;
		bool lastMovementFromController = false;
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

		Item* selectedItem = nullptr;
		bool toggleclick = false;
		bool itemMenuOpen = false;
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
		playerUsingKeyboardControl = player;
	}
	const bool bPlayerUsingKeyboardControl(const int player) const
	{
		if ( !splitscreen )
		{
			return player == clientnum;
		}
		return player == playerUsingKeyboardControl;
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
		if ( !hasController(player) )
		{
			return;
		}
		getController(player)->addRumble(pattern, smallMagnitude, largeMagnitude, length, srcEntityUid);
	}
	void rumbleStop(const int player)
	{
		if ( !hasController(player) )
		{
			return;
		}
		getController(player)->stopRumble();
	}
	void addRumbleForPlayerHPLoss(const int player, Sint32 damageAmount);
	SDL_Rect getGlyphRectForInput(const int player, bool pressed, const unsigned keyboardImpulse, const unsigned controllerImpulse);
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

	Player(int playernum = 0, bool local_host = true);
	~Player();

	void init();
	void cleanUpOnEntityRemoval();

	view_t& camera() const { return *cam; }
	const int camera_x1() const { return cam->winx; }
	const int camera_x2() const { return cam->winx + cam->winw; }
	const int camera_y1() const { return cam->winy; }
	const int camera_y2() const { return cam->winy + cam->winh; }
	const int camera_width() const { return cam->winw; }
	const int camera_height() const { return cam->winh; }
	const int camera_midx() const { return camera_x1() + camera_width() / 2; }
	const int camera_midy() const { return camera_y1() + camera_height() / 2; }
	const bool isLocalPlayer() const;
	const bool isLocalPlayerAlive() const;

	//All the code that sets shootmode = false. Display chests, inventory, books, shopkeeper, identify, whatever.
	void openStatusScreen(int whichGUIMode, int whichInventoryMode); //TODO: Make all the everything use this. //TODO: Make an accompanying closeStatusScreen() function.
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
	public:
		int DEFAULT_INVENTORY_SIZEX = 12;
		int DEFAULT_INVENTORY_SIZEY = 3;
		Inventory_t(Player& p) : 
			player(p), 
			appraisal(p), 
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
		const int getStartX() const;
		const int getStartY() const;
		const int getSlotSize() const { return static_cast<int>(40 * uiscale_inventory); }
		void setSizeY(int size) { sizey = size; }
		void selectSlot(const int x, const int y) { selectedSlotX = x; selectedSlotY = y; }
		const int getSelectedSlotX() const { return selectedSlotX; }
		const int getSelectedSlotY() const { return selectedSlotY; }
		const bool selectedSlotInPaperDoll() const { return selectedSlotY < 0; }
		const int getSelectedSlotPositionX(Item* snapToItem) const;
		const int getSelectedSlotPositionY(Item* snapToItem) const;
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
			int getAppraisalTime(Item* item); // Return time in ticks needed to appraise an item
			void appraiseItem(Item* item); // start appraise process
		} appraisal;
		bool bNewInventoryLayout = true;
	} inventoryUI;

	class StatusBar_t
	{
		Player& player;
	public:
		StatusBar_t(Player& p) : player(p)
		{};
		~StatusBar_t() {};
		SDL_Rect messageStatusBarBox;
		const int getStartX() const 
		{ 
			return (player.camera_midx() - status_bmp->w * uiscale_chatlog / 2);
		}
		const int getStartY() const
		{
			return (player.camera_y2() - getOffsetY());
		}
		const int getOffsetY() const { return (status_bmp->h * uiscale_chatlog * (hide_statusbar ? 0 : 1)); }
	} statusBarUI;

	class BookGUI_t
	{
		Player& player;
		static const int BOOK_TITLE_PADDING = 2; //The amount of empty space above and below the book titlename.
		static const int FLIPMARGIN = 240;
		static const int DRAGHEIGHT_BOOK = 32;
	public:
		static const int BOOK_PAGE_WIDTH = 248;
		static const int BOOK_PAGE_HEIGHT = 256;
		BookGUI_t(Player& p) : player(p)
		{};
		~BookGUI_t() {};
		int offsetx = 0;
		int offsety = 0;
		bool draggingBookGUI = false;
		bool bBookOpen = false;
		node_t* bookPageNode = nullptr;
		Item* openBookItem = nullptr;
		book_t* book = nullptr;
		const int getStartX() const
		{
			return ((player.camera_midx() - (getBookWidth() / 2)) + offsetx);
		}
		const int getStartY() const
		{
			return ((player.camera_midy() - (getBookHeight() / 2)) + offsety);
		}
		const int getBookWidth() const { return bookgui_img->w; }
		const int getBookHeight() const { return bookgui_img->h; }
		void updateBookGUI();
		void closeBookGUI();
		void openBook(struct book_t* book, Item* item);
	} bookGUI;

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

		void setDefaultSkillsSheetBox();
		void setDefaultPartySheetBox();
		void setDefaultCharacterSheetBox();

		bool lock_right_sidebar = false;
		int proficienciesPage = 0;
		int attributespage = 0;
	} characterSheet;

	class HUD_t
	{
		Player& player;
	public:
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
		FMOD_CHANNEL* bowDrawingSoundChannel = NULL;
		FMOD_BOOL bowDrawingSoundPlaying = 0;
#elif defined USE_OPENAL
		OPENAL_SOUND* bowDrawingSoundChannel = NULL;
		ALboolean bowDrawingSoundPlaying = 0;
#endif
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
		enum ActionPrompts : int
		{
			ACTION_PROMPT_MAINHAND,
			ACTION_PROMPT_OFFHAND,
			ACTION_PROMPT_MAGIC
		};
		void drawActionGlyph(SDL_Rect& pos, ActionPrompts prompt) const;
		void drawActionIcon(SDL_Rect& pos, int skill) const;
		const int getActionIconForPlayer(ActionPrompts prompt) const;
	} hud;

	class Magic_t
	{
		Player& player;
		spell_t* selected_spell = nullptr; //The spell the player has currently selected.
	public:
		spell_t* selected_spell_alternate[NUM_HOTBAR_ALTERNATES] = { nullptr, nullptr, nullptr, nullptr, nullptr };
		int selected_spell_last_appearance = -1;
		list_t spellList; //All of the player's spells are stored here.

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
		}
		void equipSpell(spell_t* spell) 
		{ 
			selected_spell = spell; 
		}
		spell_t* selectedSpell() const { return selected_spell; }

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
		void handlePlayerCameraPosition(bool useRefreshRateDelta);
		void reset();
	} movement;

	class MessageZone_t
	{
		static const int MESSAGE_X_OFFSET = 5;
		//Time in seconds before the message starts fading.
		static const int MESSAGE_PREFADE_TIME = 3600;
		//How fast the alpha value de-increments
		static const int MESSAGE_FADE_RATE = 10;
		TTF_Font* font = ttf16;
		Uint32 old_sdl_ticks;
		Player& player;
	public:
		static const int ADD_MESSAGE_BUFFER_LENGTH = 256;
		MessageZone_t(Player& p) : player(p) {};
		~MessageZone_t() {};
		std::list<Message*> notification_messages;
		//Adds a message to the list of messages.
		void addMessage(Uint32 color, char* content, ...);
		//Updates all the messages; fades them & removes them.
		void updateMessages();
		//Draw all the messages.
		void drawMessages();
		//Used on program deinitialization.
		void deleteAllNotificationMessages();

		// leftmost x-anchored location
		int getMessageZoneStartX();
		// bottommost y-anchored location
		int getMessageZoneStartY();
		//Maximum number of messages displayed on screen at once before the oldest message is automatically deleted.
		int getMaxTotalLines();

		int fontSize() { return getHeightOfFont(font); }
	} messageZone;

	class WorldUI_t
	{
		Player& player;
		bool bEnabled = false;
		static const int UID_TOOLTIP_ACTIVE = -21;
		static const int UID_TOOLTIP_DISABLED = -20;
		enum TooltipView
		{
			TOOLTIP_VIEW_FREE,
			TOOLTIP_VIEW_LOCKED,
			TOOLTIP_VIEW_RESCAN
		};
	public:
		WorldUI_t(Player& p) : player(p)
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
			SDL_Rect pos { 0,0,0,0 };
			PaperDollSlot_t()
			{
				item = 0;
				slotType = SLOT_MAX;
			}
			bool bMouseInSlot = false;
		};
		std::array<PaperDollSlot_t, kNumPaperDollSlots> dollSlots;
		const int getSlotSize() const;
		void initSlots()
		{
			returningItemsToInventory.clear();
			for ( int i = 0; i < kNumPaperDollSlots; ++i )
			{
				dollSlots[i].item = 0;
				dollSlots[i].slotType = static_cast<PaperDollSlotType>(i);
				dollSlots[i].bMouseInSlot = false;
			}
		}
		void clear()
		{
			returningItemsToInventory.clear();
			for ( int i = 0; i < kNumPaperDollSlots; ++i )
			{
				dollSlots[i].item = 0;
				dollSlots[i].bMouseInSlot = false;
				SDL_Rect nullRect{ 0,0,0,0 };
				dollSlots[i].pos = nullRect;
			}
		}
		void drawSlots();
		void updateSlots();
		PaperDollSlotType getSlotForItem(const Item& item) const;
		bool isItemOnDoll(const Item& item) const { return getSlotForItem(item) != SLOT_MAX; }
		PaperDollSlotType paperDollSlotFromCoordinates(int x, int y) const;
		void selectPaperDollCoordinatesFromSlotType(PaperDollSlotType slot) const;
		void warpMouseToPaperDollSlot(PaperDollSlotType slot);
		std::vector<Uint32> returningItemsToInventory;
		void warpMouseToMostRecentReturnedInventoryItem();
	} paperDoll;

	class Hotbar_t {
		std::array<hotbar_slot_t, NUM_HOTBAR_SLOTS> hotbar;
		std::array<std::array<hotbar_slot_t, NUM_HOTBAR_SLOTS>, NUM_HOTBAR_ALTERNATES> hotbar_alternate;
		Player& player;
	public:
		int current_hotbar = 0;
		bool hotbarShapeshiftInit[NUM_HOTBAR_ALTERNATES] = { false, false, false, false, false };
		int swapHotbarOnShapeshift = 0;
		bool hotbarHasFocus = false;
		int magicBoomerangHotbarSlot = -1;
		Uint32 hotbarTooltipLastGameTick = 0;
		SDL_Rect hotbarBox;

		// temp stuff
		bool useHotbarRadialMenu = false;
		bool useHotbarFaceMenu = true;
		bool faceMenuInvertLayout = false;
		bool faceMenuQuickCastEnabled = true;
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
		// end temp stuff

		std::array<SDL_Rect, NUM_HOTBAR_SLOTS> faceButtonPositions;
		const int getStartX() const
		{
			return (player.camera_midx() - ((NUM_HOTBAR_SLOTS / 2) * getSlotSize()));
		}
		const int getSlotSize() const { return hotbar_img->w * uiscale_hotbar; }

		Hotbar_t(Player& p) : player(p)
		{
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
			hotbarHasFocus = false;
			magicBoomerangHotbarSlot = -1;
			hotbarTooltipLastGameTick = 0;
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
		auto& slotsAlternate() { return hotbar_alternate; }
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
		void initFaceButtonHotbar();
		void drawFaceButtonGlyph(Uint32 slot, SDL_Rect& slotPos);
		FaceMenuGroup getFaceMenuGroupForSlot(int hotbarSlot);
	} hotbar;
};

extern Player* players[MAXPLAYERS];
//In the process of switching from the old entity player array, all of the old uses of player need to be hunted down and then corrected to account for the new array.
//So, search for the comment:
//TODO: PLAYERSWAP
//and fix and verify that the information is correct.
//Then, once all have been fixed and verified, uncomment this declaration, and the accompanying definition in player.cpp; uncomment all of the TODO: PLAYERSWAP code segments, and attempt compilation and running.
