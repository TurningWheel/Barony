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
	int player = 0;

	//! set default bindings for all players
	static void defaultBindings();

	//! input mapping
	struct binding_t {
		std::string input = "";
		float analog = 0.f;
		bool binary = false;
		bool consumed = false;

		bool analogConsumed = false;
		bool analogHeld = false;
		Uint32 analogHeldTicks = 0; // tick when the binding was pressed

		Uint32 binaryHeldTicks = 0; // tick when the binding was pressed
		bool binaryHeld = false; // if the current binding is held
		bool binaryRelease = false; // to detect when a button is lifted
		bool binaryReleaseConsumed = false; // to detect when a button is lifted + consumed

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

		//! checks type is a gamepad-adjacent input (i.e not keyboard or mouse)
		bool isBindingUsingGamepad() const { return (type != KEYBOARD && type != MOUSE_BUTTON && type != INVALID); }
		bool isBindingUsingKeyboard() const { return (type == KEYBOARD || type == MOUSE_BUTTON); }
	};

	//! useful way to get direct access to bindings
	auto& getBindings() const { return bindings; }

	//! useful way to get direct access to more bindings
	auto& getKeyboardBindings() { return kb_bindings; }
	auto& getSystemKeyboardBindings() { return kb_system_bindings; }
	auto& getGamepadBindings() { return gamepad_bindings; }
	auto& getSystemGamepadBindings() { return gamepad_system_bindings; }
	auto& getJoystickBindings() { return joystick_bindings; }
	auto& getSystemJoystickBindings() { return joystick_system_bindings; }
	void setKeyboardBindings(std::unordered_map<std::string, std::string>& toSet) { kb_bindings = toSet; }
	void setGamepadBindings(std::unordered_map<std::string, std::string>& toSet) { gamepad_bindings = toSet; }
	void setJoystickBindings(std::unordered_map<std::string, std::string>& toSet) { joystick_bindings = toSet; }

	//! controller type
	enum playerControlType_t {
		PLAYER_CONTROLLED_BY_INVALID,
		PLAYER_CONTROLLED_BY_KEYBOARD,
		PLAYER_CONTROLLED_BY_CONTROLLER,
		PLAYER_CONTROLLED_BY_JOYSTICK,
		NUM
	};
	playerControlType_t getPlayerControlType();

	//! disable all bindings temporarily
	void setDisabled(bool _disabled) { disabled = _disabled; }

	//! get the status of disabled variable
	auto isDisabled() const { return disabled; }

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
	bool analogToggle(const char* binding) const;

	//! consume an input action no matter what
	//! @param binding the binding to be consumed
	//! @return true if the toggle was consumed (ie the button was pressed)
	bool consumeBinary(const char* binding);
	bool consumeAnalog(const char* binding);

	//! consume an input action, if it is being pressed
	//! @param binding the binding to be consumed
	//! @return true if the toggle was consumed (ie the button was pressed)
	bool consumeBinaryToggle(const char* binding);
	bool consumeAnalogToggle(const char* binding);

	//! gets the 'released' binary value of a particular input binding, if it's not been consumed
	//! pressing the input and rereleasing it "unconsumes"
	//! @param binding the binding to query
	//! @return the bool value (false = pressed/untouched, true = released)
	bool binaryReleaseToggle(const char* binding) const;

	//! consume an input release action
	//! @param binding the binding to be consumed
	//! @return true if the toggle was consumed (ie the button was released)
	bool consumeBinaryReleaseToggle(const char* binding);

	//! gets the 'held' button value of a particular input binding, if it's not been consumed
	//! requires the button to be held for 'BUTTON_HELD_TICKS' to return true
	//! @param binding the binding to query
	//! @return the bool value (false = not pressed, true = pressed for greater than tick time)
	bool binaryHeldToggle(const char* binding) const;
	bool analogHeldToggle(const char* binding) const;

	//! gets the input mapped to a particular input binding
	//! @param binding the binding to query
	//! @return the input mapped to the given binding
	const char* binding(const char* binding) const;

	//! bind the given action to the given input
	//! @param name the action to bind
	//! @param input the input to bind to the action
	void bind(const char* binding, const char* input);

	//! refresh bindings (eg after a new controller is detected)
	void refresh();

	//! updates the state of all current bindings from the physical devices
	void update();

	//! updates released button binding status after gameLogic() has been run
	void updateReleasedBindings();

	//! if true, Y axis for mouse/gamepads/joysticks is inverted
	bool inverted = false;

	//! return the binding_t struct for the input name
	binding_t input(const char* binding) const;

	static std::string getGlyphPathForInput(const char* input, bool pressed = false);
	std::string getGlyphPathForBinding(const binding_t& binding, bool pressed = false) const;
	std::string getGlyphPathForBinding(const char* binding, bool pressed = false) const;

	static float getJoystickRebindingDeadzone() { return rebinding_deadzone; }

	//! list of connected input devices
	static std::string lastInputOfAnyKind;
	static int waitingToBindControllerForPlayer;
	static std::unordered_map<int, SDL_GameController*> gameControllers;
	static std::unordered_map<int, SDL_Joystick*> joysticks;
	static bool keys[SDL_NUM_SCANCODES];
	static bool mouseButtons[18];
	static const int MOUSE_WHEEL_UP;
	static const int MOUSE_WHEEL_DOWN;
private:
	std::unordered_map<std::string, binding_t> bindings;

	//! bindings written by the config file
	std::unordered_map<std::string, std::string> kb_bindings;
	std::unordered_map<std::string, std::string> gamepad_bindings;
	std::unordered_map<std::string, std::string> joystick_bindings;

	//! default system bindings, likely not changeable
	std::unordered_map<std::string, std::string> kb_system_bindings;
	std::unordered_map<std::string, std::string> gamepad_system_bindings;
	std::unordered_map<std::string, std::string> joystick_system_bindings;

	bool disabled = false;

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

	//! joystick deadzone for rebinding
	static const float rebinding_deadzone;

	//! analog binding threshold
	static const float analogToggleThreshold;

	//! map of scancodes to input names
	static std::unordered_map<std::string, SDL_Scancode> scancodeNames;
	static SDL_Scancode getScancodeFromName(const char* name);

	//! number of game ticks to consider a button 'held' for long-press actions
	static const Uint32 BUTTON_HELD_TICKS;

	//! number of game ticks to for analog button to repeatedly send
	static const Uint32 BUTTON_ANALOG_REPEAT_TICKS;
};
