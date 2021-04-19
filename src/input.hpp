//! @file input.hpp

#pragma once

#include "main.hpp"

#include <string>
#include <unordered_map>
#include <vector>

class GameController;

//! The Input class provides a way to bind physical keys to abstract names like "Move Forward",
//! collect the input data from the physical devices, and provide it back to you for any purpose.
class Input {
public:
	Input() = default;
	Input(const Input&) = delete;
	Input(Input&&) = delete;
	~Input() = default;

	Input& operator=(const Input&) = delete;
	Input& operator=(Input&&) = delete;

	//! one input for each player
	static Input inputs[MAXPLAYERS];

	//! set default bindings for all players
	static void defaultBindings();

	//! input mapping
	struct binding_t {
		std::string input = "";
		float analog = 0.f;
		bool binary = false;
		bool consumed = false;

		//! bind type
		enum bindtype_t {
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
		bindtype_t type = INVALID;

		//! keyboard binding info
		SDL_Scancode scancode = SDL_Scancode::SDL_SCANCODE_UNKNOWN;

		//! gamepad binding info
		SDL_GameController* pad = nullptr;
		SDL_GameControllerAxis padAxis = SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_INVALID;
		SDL_GameControllerButton padButton = SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_INVALID;
		bool padAxisNegative = false;

		//! joystick binding info
		SDL_Joystick* joystick = nullptr;
		int joystickAxis = 0;
		bool joystickAxisNegative = false;
		int joystickButton = 0;
		int joystickHat = 0;
		Uint8 joystickHatState = 0;

		//! mouse button info
		int mouseButton = 0;
	};

	//! gets the analog value of a particular input binding
	//! @param binding the binding to query
	//! @return the analog value (range = -1.f : +1.f)
	float analog(const char* binding) const;

	//! gets the binary value of a particular input binding
	//! @param binding the binding to query
	//! @return the bool value (false = not pressed, true = pressed)
	bool binary(const char* binding) const;

	//! gets the binary value of a particular input binding, if it's not been consumed
	//! releasing the input and retriggering it "unconsumes"
	//! @param binding the binding to query
	//! @return the bool value (false = not pressed, true = pressed)
	bool binaryToggle(const char* binding) const;

	//! consume an input action
	//! @param binding the binding to be consumed
	//! @return true if the toggle was consumed (ie the button was pressed)
	bool consumeBinaryToggle(const char* binding);

	//! gets the input mapped to a particular input binding
	//! @param binding the binding to query
	//! @return the input mapped to the given binding
	const char* binding(const char* binding) const;

	//! bind the given action to the given input
	//! @param name the action to bind
	//! @param input the input to bind to the action
	void bind(const char* binding, const char* input);

	//! add a game controller to our public map of controllers
	//! @param id the id of the controller
	//! @param controller the controller to add
	static void addGameController(int id, GameController& controller);

	//! refresh bindings (eg after a new controller is detected)
	void refresh();

	//! updates the state of all current bindings from the physical devices
	void update();

	//! if true, Y axis for mouse/gamepads/joysticks is inverted
	bool inverted = false;

	//! list of connected input devices
	static std::string lastInputOfAnyKind;
	static std::unordered_map<int, SDL_GameController*> gameControllers;
	static std::unordered_map<int, SDL_Joystick*> joysticks;
	static bool keys[SDL_NUM_SCANCODES];
	static bool mouseButtons[8];

private:
	std::unordered_map<std::string, binding_t> bindings;

	//! converts the given input to a boolean value
	//! @return the converted value
	static bool binaryOf(binding_t& binding);

	//! converts the given input to a float value
	//! @return the converted value
	static float analogOf(binding_t& binding);

	//! mouse sensitivity
	static const float sensitivity;

	//! joystick deadzone
	static const float deadzone;

	//! map of scancodes to input names
	static std::unordered_map<std::string, SDL_Scancode> scancodeNames;
	static SDL_Scancode getScancodeFromName(const char* name);
};
