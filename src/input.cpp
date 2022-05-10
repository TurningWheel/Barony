#include "main.hpp"
#include "input.hpp"
#ifdef EDITOR
#ifndef TICKS_PER_SECOND
#define TICKS_PER_SECOND 50
#endif
#else
#include "player.hpp"
#endif
#include "mod_tools.hpp"
#include "ui/MainMenu.hpp"

#include <algorithm>

Input Input::inputs[MAXPLAYERS];

const float Input::sensitivity = 1.f;
const float Input::deadzone = 0.2f;
const float Input::rebinding_deadzone = 0.5f;
const float Input::analogToggleThreshold = .5f;
const Uint32 Input::BUTTON_HELD_TICKS = TICKS_PER_SECOND / 4;
const Uint32 Input::BUTTON_ANALOG_REPEAT_TICKS = TICKS_PER_SECOND / 4;
std::unordered_map<std::string, SDL_Scancode> Input::scancodeNames;
std::unordered_map<int, SDL_GameController*> Input::gameControllers;
std::unordered_map<int, SDL_Joystick*> Input::joysticks;
bool Input::keys[SDL_NUM_SCANCODES] = { false };
bool Input::mouseButtons[18] = { false };
const int Input::MOUSE_WHEEL_UP = 16;
const int Input::MOUSE_WHEEL_DOWN = 17;
std::string Input::lastInputOfAnyKind;
int Input::waitingToBindControllerForPlayer = 0;

void Input::defaultBindings() {
	for (int i = 0; i < MAXPLAYERS; ++i) {
		inputs[i].player = i;
		inputs[i].kb_system_bindings.clear();
		inputs[i].gamepad_system_bindings.clear();
		inputs[i].joystick_system_bindings.clear();
	}

	// these bindings should probably not be accessible to the player to change.
	for (int c = 0; c < MAXPLAYERS; ++c) {
		inputs[c].gamepad_system_bindings.insert(std::make_pair("Console Command", "/"));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("MenuUp", (std::string("Pad") + std::to_string(c) + std::string("DpadY-")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("MenuLeft", (std::string("Pad") + std::to_string(c) + std::string("DpadX-")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("MenuRight", (std::string("Pad") + std::to_string(c) + std::string("DpadX+")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("MenuDown", (std::string("Pad") + std::to_string(c) + std::string("DpadY+")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("MenuConfirm", (std::string("Pad") + std::to_string(c) + std::string("ButtonA")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("MenuCancel", (std::string("Pad") + std::to_string(c) + std::string("ButtonB")).c_str()));
#ifdef NINTENDO
		inputs[c].gamepad_system_bindings.insert(std::make_pair"MenuAlt1", (std::string("Pad") + std::to_string(c) + std::string("ButtonY")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair"MenuAlt2", (std::string("Pad") + std::to_string(c) + std::string("ButtonX")).c_str()));
#else
		inputs[c].gamepad_system_bindings.insert(std::make_pair("MenuAlt1", (std::string("Pad") + std::to_string(c) + std::string("ButtonX")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("MenuAlt2", (std::string("Pad") + std::to_string(c) + std::string("ButtonY")).c_str()));
#endif
		inputs[c].gamepad_system_bindings.insert(std::make_pair("MenuStart", (std::string("Pad") + std::to_string(c) + std::string("ButtonStart")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("MenuSelect", (std::string("Pad") + std::to_string(c) + std::string("ButtonBack")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("MenuPageLeft", (std::string("Pad") + std::to_string(c) + std::string("ButtonLeftBumper")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("MenuPageRight", (std::string("Pad") + std::to_string(c) + std::string("ButtonRightBumper")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("MenuPageLeftAlt", (std::string("Pad") + std::to_string(c) + std::string("LeftTrigger")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("MenuPageRightAlt", (std::string("Pad") + std::to_string(c) + std::string("RightTrigger")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("AltMenuUp", (std::string("Pad") + std::to_string(c) + std::string("StickLeftY-")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("AltMenuLeft", (std::string("Pad") + std::to_string(c) + std::string("StickLeftX-")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("AltMenuRight", (std::string("Pad") + std::to_string(c) + std::string("StickLeftX+")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("AltMenuDown", (std::string("Pad") + std::to_string(c) + std::string("StickLeftY+")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("MenuScrollUp", (std::string("Pad") + std::to_string(c) + std::string("StickRightY-")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("MenuScrollLeft", (std::string("Pad") + std::to_string(c) + std::string("StickRightX-")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("MenuScrollRight", (std::string("Pad") + std::to_string(c) + std::string("StickRightX+")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("MenuScrollDown", (std::string("Pad") + std::to_string(c) + std::string("StickRightY+")).c_str()));

#ifdef NINTENDO
		inputs[c].gamepad_system_bindings.insert(std::make_pair("HotbarFacebarLeft", (std::string("Pad") + std::to_string(c) + std::string("ButtonY")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("HotbarFacebarUp", (std::string("Pad") + std::to_string(c) + std::string("ButtonX")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("HotbarFacebarRight", (std::string("Pad") + std::to_string(c) + std::string("ButtonA")).c_str()));
#else
		inputs[c].gamepad_system_bindings.insert(std::make_pair("HotbarFacebarLeft", (std::string("Pad") + std::to_string(c) + std::string("ButtonX")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("HotbarFacebarUp", (std::string("Pad") + std::to_string(c) + std::string("ButtonY")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("HotbarFacebarRight", (std::string("Pad") + std::to_string(c) + std::string("ButtonB")).c_str()));
#endif
		inputs[c].gamepad_system_bindings.insert(std::make_pair("HotbarFacebarModifierLeft", (std::string("Pad") + std::to_string(c) + std::string("ButtonLeftBumper")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("HotbarFacebarModifierRight", (std::string("Pad") + std::to_string(c) + std::string("ButtonRightBumper")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("HotbarFacebarCancel", (std::string("Pad") + std::to_string(c) + std::string("DpadY+")).c_str()));

		//inputs[c].bind("HotbarInventoryClearSlot", (std::string("Pad") + std::to_string(c) + std::string("ButtonY")).c_str()));

		inputs[c].gamepad_system_bindings.insert(std::make_pair("InventoryMoveUp", (std::string("Pad") + std::to_string(c) + std::string("DpadY-")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("InventoryMoveLeft", (std::string("Pad") + std::to_string(c) + std::string("DpadX-")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("InventoryMoveRight", (std::string("Pad") + std::to_string(c) + std::string("DpadX+")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("InventoryMoveDown", (std::string("Pad") + std::to_string(c) + std::string("DpadY+")).c_str()));

		inputs[c].gamepad_system_bindings.insert(std::make_pair("InventoryMoveUpAnalog", (std::string("Pad") + std::to_string(c) + std::string("StickRightY-")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("InventoryMoveLeftAnalog", (std::string("Pad") + std::to_string(c) + std::string("StickRightX-")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("InventoryMoveRightAnalog", (std::string("Pad") + std::to_string(c) + std::string("StickRightX+")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("InventoryMoveDownAnalog", (std::string("Pad") + std::to_string(c) + std::string("StickRightY+")).c_str()));

		inputs[c].gamepad_system_bindings.insert(std::make_pair("InventoryCharacterRotateLeft", (std::string("Pad") + std::to_string(c) + std::string("StickRightX-")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("InventoryCharacterRotateRight", (std::string("Pad") + std::to_string(c) + std::string("StickRightX+")).c_str()));

		inputs[c].gamepad_system_bindings.insert(std::make_pair("InventoryTooltipPromptAppraise", (std::string("Pad") + std::to_string(c) + std::string("ButtonLeftStick")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("Expand Inventory Tooltip", (std::string("Pad") + std::to_string(c) + std::string("ButtonRightStick")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("CycleWorldTooltipNext", (std::string("Pad") + std::to_string(c) + std::string("DpadX+")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("CycleWorldTooltipPrev", (std::string("Pad") + std::to_string(c) + std::string("DpadX-")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("UINavLeftBumper", (std::string("Pad") + std::to_string(c) + std::string("ButtonLeftBumper")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("UINavRightBumper", (std::string("Pad") + std::to_string(c) + std::string("ButtonRightBumper")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("UINavLeftTrigger", (std::string("Pad") + std::to_string(c) + std::string("LeftTrigger")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("UINavRightTrigger", (std::string("Pad") + std::to_string(c) + std::string("RightTrigger")).c_str()));

		inputs[c].gamepad_system_bindings.insert(std::make_pair("Move Forward", (std::string("Pad") + std::to_string(c) + std::string("StickLeftY-")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("Move Left", (std::string("Pad") + std::to_string(c) + std::string("StickLeftX-")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("Move Backward", (std::string("Pad") + std::to_string(c) + std::string("StickLeftY+")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("Move Right", (std::string("Pad") + std::to_string(c) + std::string("StickLeftX+")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("Turn Left", (std::string("Pad") + std::to_string(c) + std::string("StickRightX-")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("Turn Right", (std::string("Pad") + std::to_string(c) + std::string("StickRightX+")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("Look Up", (std::string("Pad") + std::to_string(c) + std::string("StickRightY-")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("Look Down", (std::string("Pad") + std::to_string(c) + std::string("StickRightY+")).c_str()));

		inputs[c].kb_system_bindings.insert(std::make_pair("HotbarSlot1", "1"));
		inputs[c].kb_system_bindings.insert(std::make_pair("HotbarSlot2", "2"));
		inputs[c].kb_system_bindings.insert(std::make_pair("HotbarSlot3", "3"));
		inputs[c].kb_system_bindings.insert(std::make_pair("HotbarSlot4", "4"));
		inputs[c].kb_system_bindings.insert(std::make_pair("HotbarSlot5", "5"));
		inputs[c].kb_system_bindings.insert(std::make_pair("HotbarSlot6", "6"));
		inputs[c].kb_system_bindings.insert(std::make_pair("HotbarSlot7", "7"));
		inputs[c].kb_system_bindings.insert(std::make_pair("HotbarSlot8", "8"));
		inputs[c].kb_system_bindings.insert(std::make_pair("HotbarSlot9", "9"));
		inputs[c].kb_system_bindings.insert(std::make_pair("HotbarSlot10", "0"));
		inputs[c].kb_system_bindings.insert(std::make_pair("MenuMouseWheelUp", "MouseWheelUp")); // consumed automatically by frame.cpp
		inputs[c].kb_system_bindings.insert(std::make_pair("MenuMouseWheelDown", "MouseWheelDown")); // consumed automatically by frame.cpp
		inputs[c].kb_system_bindings.insert(std::make_pair("MenuMouseWheelUpAlt", "MouseWheelUp"));
		inputs[c].kb_system_bindings.insert(std::make_pair("MenuMouseWheelDownAlt", "MouseWheelDown"));
		inputs[c].kb_system_bindings.insert(std::make_pair("MenuLeftClick", "Mouse1"));
		inputs[c].kb_system_bindings.insert(std::make_pair("MenuMiddleClick", "Mouse2"));
		inputs[c].kb_system_bindings.insert(std::make_pair("MenuRightClick", "Mouse3"));
		inputs[c].kb_system_bindings.insert(std::make_pair("InspectWithMouse", "Mouse1"));

		inputs[c].kb_system_bindings.insert(std::make_pair("MenuUp", "Up"));
		inputs[c].kb_system_bindings.insert(std::make_pair("MenuLeft", "Left"));
		inputs[c].kb_system_bindings.insert(std::make_pair("MenuRight", "Right"));
		inputs[c].kb_system_bindings.insert(std::make_pair("MenuDown", "Down"));
		inputs[c].kb_system_bindings.insert(std::make_pair("MenuConfirm", "Space"));
		inputs[c].kb_system_bindings.insert(std::make_pair("MenuCancel", "Escape"));
		//inputs[c].kb_system_bindings.insert(std::make_pair("MenuAlt1", "Left Shift"));
		//inputs[c].kb_system_bindings.insert(std::make_pair("MenuAlt2", "Left Ctrl"));
		inputs[c].kb_system_bindings.insert(std::make_pair("MenuStart", "Return"));
		inputs[c].kb_system_bindings.insert(std::make_pair("MenuSelect", "Backspace"));
		inputs[c].kb_system_bindings.insert(std::make_pair("MenuPageLeft", "["));
		inputs[c].kb_system_bindings.insert(std::make_pair("MenuPageRight", "]"));
		inputs[c].kb_system_bindings.insert(std::make_pair("Console Command", "/"));
	}
}

float Input::analog(const char* binding) const {
    if (disabled) { return 0.f; }
	auto b = bindings.find(binding);
	return b != bindings.end() ? (*b).second.analog : 0.f;
}

bool Input::binary(const char* binding) const {
    if (disabled) { return false; }
	auto b = bindings.find(binding);
	return b != bindings.end() ? (*b).second.binary : false;
}

bool Input::binaryToggle(const char* binding) const {
    if (disabled) { return false; }
	auto b = bindings.find(binding);
	return b != bindings.end() ? (*b).second.binary && !(*b).second.consumed : false;
}

bool Input::analogToggle(const char* binding) const {
    if (disabled) { return false; }
	auto b = bindings.find(binding);
	return b != bindings.end() ? (*b).second.analog > analogToggleThreshold && !(*b).second.analogConsumed : false;
}

bool Input::binaryReleaseToggle(const char* binding) const {
    if (disabled) { return false; }
	auto b = bindings.find(binding);
	return b != bindings.end() ? (*b).second.binaryRelease && !(*b).second.binaryReleaseConsumed : false;
}

bool Input::consumeAnalog(const char* binding) {
	auto b = bindings.find(binding);
	if ( b != bindings.end() && !(*b).second.analogConsumed ) {
		(*b).second.analogConsumed = true;
		return disabled == false;
	}
	else {
		return false;
	}
}

bool Input::consumeBinary(const char* binding) {
	auto b = bindings.find(binding);
	if (b != bindings.end() && !(*b).second.consumed) {
		(*b).second.consumed = true;
		if ( (*b).second.type == binding_t::bindtype_t::MOUSE_BUTTON
			&& ((*b).second.mouseButton == MOUSE_WHEEL_DOWN
				|| (*b).second.mouseButton == MOUSE_WHEEL_UP) )
		{
			mouseButtons[(*b).second.mouseButton] = false; // manually need to clear this
		}
		return disabled == false;
	} else {
		return false;
	}
}

bool Input::consumeAnalogToggle(const char* binding) {
	auto b = bindings.find(binding);
	if ( b != bindings.end() && (*b).second.analog > analogToggleThreshold && !(*b).second.analogConsumed ) {
		(*b).second.analogConsumed = true;
		return disabled == false;
	}
	else {
		return false;
	}
}

bool Input::consumeBinaryToggle(const char* binding) {
	auto b = bindings.find(binding);
	if (b != bindings.end() && (*b).second.binary && !(*b).second.consumed) {
		(*b).second.consumed = true;
		if ( (*b).second.type == binding_t::bindtype_t::MOUSE_BUTTON
			&& ((*b).second.mouseButton == MOUSE_WHEEL_DOWN
				|| (*b).second.mouseButton == MOUSE_WHEEL_UP) )
		{
			mouseButtons[(*b).second.mouseButton] = false; // manually need to clear this
		}
		return disabled == false;
	} else {
		return false;
	}
}

bool Input::consumeBinaryReleaseToggle(const char* binding) {
	auto b = bindings.find(binding);
	if ( b != bindings.end() && (*b).second.binaryRelease && !(*b).second.binaryReleaseConsumed ) {
		(*b).second.binaryReleaseConsumed = true;
		return disabled == false;
	}
	else {
		return false;
	}
}

bool Input::binaryHeldToggle(const char* binding) const {
    if (disabled) { return false; }
	auto b = bindings.find(binding);
	return b != bindings.end() 
		? ((*b).second.binary && !(*b).second.consumed && (ticks - (*b).second.binaryHeldTicks) > BUTTON_HELD_TICKS)
		: false;
}

bool Input::analogHeldToggle(const char* binding) const {
    if (disabled) { return false; }
	auto b = bindings.find(binding);
	return b != bindings.end()
		? ((*b).second.analog > analogToggleThreshold && !(*b).second.analogConsumed && (ticks - (*b).second.analogHeldTicks) > BUTTON_HELD_TICKS)
		: false;
}

const char* Input::binding(const char* binding) const {
	auto b = bindings.find(binding);
	return b != bindings.end() ? (*b).second.input.c_str() : "";
}

void Input::refresh() {
	bindings.clear();
	defaultBindings();
#ifndef EDITOR
	for ( auto& binding : kb_system_bindings )
	{
		bind(binding.first.c_str(), binding.second.c_str());
	}
	if ( ::inputs.bPlayerUsingKeyboardControl(player) )
	{
	    for (auto& binding : getKeyboardBindings() )
	    {
		    bind(binding.first.c_str(), binding.second.c_str());
	    }
	}
	if ( ::inputs.hasController(player) )
	{
		for ( auto& binding : gamepad_system_bindings )
		{
			bind(binding.first.c_str(), binding.second.c_str());
		}

		std::string prefix;
		prefix.append("Pad");
		prefix.append(std::to_string(player));
		for (auto& binding : getGamepadBindings()) {
			if ( binding.second == MainMenu::hiddenBinding )
			{
				auto b = bindings.find(binding.first);
				if ( b != bindings.end() && b->second.isBindingUsingGamepad() )
				{
					// hidden binding, don't override existing bind by the defaults.
					continue;
				}
			}
			bind(binding.first.c_str(), (prefix + binding.second).c_str());
		}
	}
	if ( false /*::inputs.hasJoystick(player)*/ )
	{
		for ( auto& binding : joystick_system_bindings )
		{
			bind(binding.first.c_str(), binding.second.c_str());
		}

		std::string prefix;
		prefix.append("Joy");
		prefix.append(std::to_string(player));
		for (auto& binding : getJoystickBindings()) {
		    bind(binding.first.c_str(), (prefix + binding.second).c_str());
		}
	}
#endif // !EDITOR
}

Input::binding_t Input::input(const char* binding) const {
	auto b = bindings.find(binding);
	return b != bindings.end() ? (*b).second : Input::binding_t();
}

std::string Input::getGlyphPathForInput(const char* input, bool pressed)
{
    std::string in = input;
	std::string rootPath = "images/ui/Glyphs/";
#ifdef NINTENDO
	if (in == "ButtonA")
	{
		return rootPath + "G_Switch_A00.png";
    }
	if (in == "ButtonB")
	{
		return rootPath + "G_Switch_B00.png";
    }
	if (in == "ButtonX")
	{
		return rootPath + "G_Switch_Y00.png";
    }
	if (in == "ButtonY")
	{
		return rootPath + "G_Switch_X00.png";
    }
	if (in == "ButtonLeftBumper")
	{
		return rootPath + "G_Switch_L00.png";
    }
	if (in == "ButtonRightBumper")
	{
		return rootPath + "G_Switch_R00.png";
    }
	if ( in == "ButtonLeftStick" )
	{
		if ( pressed )
		{
			return rootPath + "Stick_Switch_L_Pressed_00.png";
		}
		else
		{
			return rootPath + "Stick_Switch_L_00.png";
		}
	}
	if ( in == "ButtonRightStick" )
	{
		if ( pressed )
		{
			return rootPath + "Stick_Switch_R_Pressed_00.png";
		}
		else
		{
			return rootPath + "Stick_Switch_R_00.png";
		}
	}
	if (in == "ButtonStart")
	{
		return rootPath + "PlusMed00.png";
    }
	if (in == "ButtonBack")
	{
		return rootPath + "MinusMed00.png";
    }
	if ( in == "StickLeftX-" )
	{
		if ( pressed )
		{
			return rootPath + "Stick_Switch_L_Left_Pressed_00.png";
		}
		else
		{
			return rootPath + "Stick_Switch_L_Left_00.png";
		}
	}
	if ( in == "StickLeftX+" )
	{
		if ( pressed )
		{
			return rootPath + "Stick_Switch_L_Right_Pressed_00.png";
		}
		else
		{
			return rootPath + "Stick_Switch_L_Right_00.png";
		}
}
	if ( in == "StickLeftY-" )
	{
		if ( pressed )
		{
			return rootPath + "Stick_Switch_L_Up_Pressed_00.png";
		}
		else
		{
			return rootPath + "Stick_Switch_L_Up_00.png";
		}
	}
	if ( in == "StickLeftY+" )
	{
		if ( pressed )
		{
			return rootPath + "Stick_Switch_L_Down_Pressed_00.png";
		}
		else
		{
			return rootPath + "Stick_Switch_L_Down_00.png";
		}
	}
	if ( in == "StickRightX-" )
	{
		if ( pressed )
		{
			return rootPath + "Stick_Switch_R_Left_Pressed_00.png";
		}
		else
		{
			return rootPath + "Stick_Switch_R_Left_00.png";
		}
	}
	if ( in == "StickRightX+" )
	{
		if ( pressed )
		{
			return rootPath + "Stick_Switch_R_Right_Pressed_00.png";
		}
		else
		{
			return rootPath + "Stick_Switch_R_Right_00.png";
		}
	}
	if ( in == "StickRightY-" )
	{
		if ( pressed )
		{
			return rootPath + "Stick_Switch_R_Up_Pressed_00.png";
		}
		else
		{
			return rootPath + "Stick_Switch_R_Up_00.png";
		}
	}
	if ( in == "StickRightY+" )
	{
		if ( pressed )
		{
			return rootPath + "Stick_Switch_R_Down_Pressed_00.png";
		}
		else
		{
			return rootPath + "Stick_Switch_R_Down_00.png";
		}
	}
	if (in == "LeftTrigger")
	{
		return rootPath + "G_Switch_ZL00.png";
	}
	if (in == "RightTrigger")
	{
		return rootPath + "G_Switch_ZR00.png";
	}
	if (in == "DpadY-")
	{
		return rootPath + "G_Up00.png";
    }
	if (in == "DpadX-")
	{
		return rootPath + "G_Left00.png";
    }
	if (in == "DpadY+")
	{
		return rootPath + "G_Down00.png";
    }
	if (in == "DpadX+")
	{
		return rootPath + "G_Right00.png";
    }
#else
	if (in == "ButtonA")
	{
		return rootPath + "Button_Xbox_DarkA_00.png";
    }
	if (in == "ButtonB")
	{
		return rootPath + "Button_Xbox_DarkB_00.png";
    }
	if (in == "ButtonX")
	{
		return rootPath + "Button_Xbox_DarkX_00.png";
    }
	if (in == "ButtonY")
	{
		return rootPath + "Button_Xbox_DarkY_00.png";
    }
	if (in == "ButtonLeftBumper")
	{
		return rootPath + "Button_Xbox_LB_00.png";
    }
	if (in == "ButtonRightBumper")
	{
		return rootPath + "Button_Xbox_RB_00.png";
    }
	if (in == "ButtonLeftStick")
	{
		if ( pressed )
		{
			return rootPath + "Stick_Xbox_L_Pressed_00.png";
		}
		else
		{
			return rootPath + "Stick_Xbox_L_00.png";
		}
    }
	if (in == "ButtonRightStick")
	{
		if ( pressed )
		{
			return rootPath + "Stick_Xbox_R_Pressed_00.png";
		}
		else
		{
			return rootPath + "Stick_Xbox_R_00.png";
		}
    }
	if (in == "ButtonStart")
	{
		return rootPath + "Button_Xbox_Menu_00.png";
    }
	if (in == "ButtonBack")
	{
		return rootPath + "Button_Xbox_View_00.png";
    }
	if ( in == "StickLeftX-" )
	{
		if ( pressed )
		{
			return rootPath + "Stick_Xbox_L_Left_Pressed_00.png";
		}
		else
		{
			return rootPath + "Stick_Xbox_L_Left_00.png";
		}
	}
	if ( in == "StickLeftX+" )
	{
		if ( pressed )
		{
			return rootPath + "Stick_Xbox_L_Right_Pressed_00.png";
		}
		else
		{
			return rootPath + "Stick_Xbox_L_Right_00.png";
		}
    }
	if ( in == "StickLeftY-" )
	{
		if ( pressed )
		{
			return rootPath + "Stick_Xbox_L_Up_Pressed_00.png";
		}
		else
		{
			return rootPath + "Stick_Xbox_L_Up_00.png";
		}
	}
	if ( in == "StickLeftY+" )
	{
		if ( pressed )
		{
			return rootPath + "Stick_Xbox_L_Down_Pressed_00.png";
		}
		else
		{
			return rootPath + "Stick_Xbox_L_Down_00.png";
		}
	}
	if ( in == "StickRightX-" )
	{
		if ( pressed )
		{
			return rootPath + "Stick_Xbox_R_Left_Pressed_00.png";
		}
		else
		{
			return rootPath + "Stick_Xbox_R_Left_00.png";
		}
	}
	if ( in == "StickRightX+" )
	{
		if ( pressed )
		{
			return rootPath + "Stick_Xbox_R_Right_Pressed_00.png";
		}
		else
		{
			return rootPath + "Stick_Xbox_R_Right_00.png";
		}
	}
	if ( in == "StickRightY-" )
	{
		if ( pressed )
		{
			return rootPath + "Stick_Xbox_R_Up_Pressed_00.png";
		}
		else
		{
			return rootPath + "Stick_Xbox_R_Up_00.png";
		}
	}
	if ( in == "StickRightY+" )
	{
		if ( pressed )
		{
			return rootPath + "Stick_Xbox_R_Down_Pressed_00.png";
		}
		else
		{
			return rootPath + "Stick_Xbox_R_Down_00.png";
		}
	}
	if (in == "LeftTrigger")
	{
		return rootPath + "Button_Xbox_LT_00.png";
	}
	if (in == "RightTrigger")
	{
		return rootPath + "Button_Xbox_RT_00.png";
	}
	if (in == "DpadY-")
	{
		return rootPath + "G_Up00.png";
    }
	if (in == "DpadX-")
	{
		return rootPath + "G_Left00.png";
    }
	if (in == "DpadY+")
	{
		return rootPath + "G_Down00.png";
    }
	if (in == "DpadX+")
	{
		return rootPath + "G_Right00.png";
    }
#endif
	if (in == "Mouse1")
	{
		if ( pressed )
		{
			return rootPath + "Mouse/Mouse_LClick_Unpressed_00.png";
		}
		else
		{
			return rootPath + "Mouse/Mouse_LClick_Pressed_00.png";
		}
	}
	if (in == "Mouse2")
	{
		if ( pressed )
		{
			return rootPath + "Mouse/Mouse_MClick_Unpressed_00.png";
		}
		else
		{
			return rootPath + "Mouse/Mouse_MClick_Pressed_00.png";
		}
	}
	if (in == "Mouse3")
	{
		if ( pressed )
		{
			return rootPath + "Mouse/Mouse_RClick_Unpressed_00.png";
		}
		else
		{
			return rootPath + "Mouse/Mouse_RClick_Pressed_00.png";
		}
	}
	if (in == "MouseWheelDown")
	{
		if ( pressed )
		{
			return rootPath + "Mouse/Mouse_MWheelDown_Unpressed_00.png";
		}
		else
		{
			return rootPath + "Mouse/Mouse_MWheelDown_Pressed_00.png";
		}
	}
	if (in == "MouseWheelUp")
	{
		if ( pressed )
		{
			return rootPath + "Mouse/Mouse_MWheelUp_Unpressed_00.png";
		}
		else
		{
			return rootPath + "Mouse/Mouse_MWheelUp_Pressed_00.png";
		}
	}
	else
	{
	    auto scancode = getScancodeFromName(input);
	    return GlyphHelper.getGlyphPath(scancode, pressed);
	}
}

std::string Input::getGlyphPathForBinding(const char* binding, bool pressed) const
{
	return getGlyphPathForBinding(input(binding), pressed);
}

std::string Input::getGlyphPathForBinding(const binding_t& binding, bool pressed) const
{
	std::string rootPath = "images/ui/Glyphs/";
	if ( binding.type == binding_t::bindtype_t::CONTROLLER_BUTTON )
	{
#ifdef NINTENDO
		switch ( binding.padButton )
		{
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_A:
				return rootPath + "G_Switch_B00.png";
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_B:
				return rootPath + "G_Switch_A00.png";
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_X:
				return rootPath + "G_Switch_Y00.png";
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_Y:
				return rootPath + "G_Switch_X00.png";
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
				return rootPath + "G_Switch_L00.png";
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
				return rootPath + "G_Switch_R00.png";
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_LEFTSTICK:
				if ( pressed )
				{
					return rootPath + "Stick_Switch_L_Pressed_00.png";
				}
				else
				{
					return rootPath + "Stick_Switch_L_00.png";
				}
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_RIGHTSTICK:
				if ( pressed )
				{
					return rootPath + "Stick_Switch_R_Pressed_00.png";
				}
				else
				{
					return rootPath + "Stick_Switch_R_00.png";
				}
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_START:
				return rootPath + "PlusMed00.png";
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_BACK:
				return rootPath + "MinusMed00.png";
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_UP:
				return rootPath + "G_Up00.png";
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_LEFT:
				return rootPath + "G_Left00.png";
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_DOWN:
				return rootPath + "G_Down00.png";
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
				return rootPath + "G_Right00.png";
			default:
				return "";
		}
#else
		switch ( binding.padButton )
		{
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_A:
				return rootPath + "Button_Xbox_DarkA_00.png";
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_B:
				return rootPath + "Button_Xbox_DarkB_00.png";
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_X:
				return rootPath + "Button_Xbox_DarkX_00.png";
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_Y:
				return rootPath + "Button_Xbox_DarkY_00.png";
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
				return rootPath + "Button_Xbox_LB_00.png";
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
				return rootPath + "Button_Xbox_RB_00.png";
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_LEFTSTICK:
				if ( pressed )
				{
					return rootPath + "Stick_Xbox_L_Pressed_00.png";
				}
				else
				{
					return rootPath + "Stick_Xbox_L_00.png";
				}
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_RIGHTSTICK:
				if ( pressed )
				{
					return rootPath + "Stick_Xbox_R_Pressed_00.png";
				}
				else
				{
					return rootPath + "Stick_Xbox_R_00.png";
				}
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_START:
				return rootPath + "Button_Xbox_Menu_00.png";
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_BACK:
				return rootPath + "Button_Xbox_View_00.png";
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_UP:
				return rootPath + "G_Up00.png";
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_LEFT:
				return rootPath + "G_Left00.png";
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_DOWN:
				return rootPath + "G_Down00.png";
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
				return rootPath + "G_Right00.png";
			default:
				return "";
		}
#endif
	}
	else if ( binding.type == binding_t::bindtype_t::CONTROLLER_AXIS )
	{
#ifdef NINTENDO
		switch ( binding.padAxis )
		{
			case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTX:
				if ( binding.padAxisNegative )
				{
					return rootPath + "Stick_Switch_L_Left_00.png";
				}
				else
				{
					return rootPath + "Stick_Switch_L_Right_00.png";
				}
			case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTY:
				if ( binding.padAxisNegative )
				{
					return rootPath + "Stick_Switch_L_Up_00.png";
				}
				else
				{
					return rootPath + "Stick_Switch_L_Down_00.png";
				}
			case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_RIGHTX:
				if ( binding.padAxisNegative )
				{
					return rootPath + "Stick_Switch_R_Left_00.png";
				}
				else
				{
					return rootPath + "Stick_Switch_R_Right_00.png";
				}
			case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_RIGHTY:
				if ( binding.padAxisNegative )
				{
					return rootPath + "Stick_Switch_R_Up_00.png";
				}
				else
				{
					return rootPath + "Stick_Switch_R_Down_00.png";
				}
			case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_TRIGGERLEFT:
				return rootPath + "G_Switch_ZL00.png";
			case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
				return rootPath + "G_Switch_ZR00.png";
			default:
				return "";
		}
#else
		switch ( binding.padAxis )
		{
			case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTX:
				if ( binding.padAxisNegative )
				{
					return rootPath + "Stick_Xbox_L_Left_00.png";
				}
				else
				{
					return rootPath + "Stick_Xbox_L_Right_00.png";
				}
			case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTY:
				if ( binding.padAxisNegative )
				{
					return rootPath + "Stick_Xbox_L_Up_00.png";
				}
				else
				{
					return rootPath + "Stick_Xbox_L_Down_00.png";
				}
			case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_RIGHTX:
				if ( binding.padAxisNegative )
				{
					return rootPath + "Stick_Xbox_R_Left_00.png";
				}
				else
				{
					return rootPath + "Stick_Xbox_R_Right_00.png";
				}
			case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_RIGHTY:
				if ( binding.padAxisNegative )
				{
					return rootPath + "Stick_Xbox_R_Up_00.png";
				}
				else
				{
					return rootPath + "Stick_Xbox_R_Down_00.png";
				}
			case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_TRIGGERLEFT:
				return rootPath + "Button_Xbox_LT_00.png";
			case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
				return rootPath + "Button_Xbox_RT_00.png";
			default:
				return "";
		}
#endif
	}
	else if ( binding.type == binding_t::bindtype_t::KEYBOARD )
	{
		return GlyphHelper.getGlyphPath(binding.scancode, pressed);
	}
	else if ( binding.type == binding_t::bindtype_t::MOUSE_BUTTON )
	{
		if ( binding.mouseButton == 1 )
		{
			if ( pressed )
			{
				return rootPath + "Mouse/Mouse_LClick_Unpressed_00.png";
			}
			else
			{
				return rootPath + "Mouse/Mouse_LClick_Pressed_00.png";
			}
		}
		else if ( binding.mouseButton == 3 )
		{
			if ( pressed )
			{
				return rootPath + "Mouse/Mouse_RClick_Unpressed_00.png";
			}
			else
			{
				return rootPath + "Mouse/Mouse_RClick_Pressed_00.png";
			}
		}
		else if ( binding.mouseButton == 2 )
		{
			if ( pressed )
			{
				return rootPath + "Mouse/Mouse_MClick_Unpressed_00.png";
			}
			else
			{
				return rootPath + "Mouse/Mouse_MClick_Pressed_00.png";
			}
		}
		else if ( binding.mouseButton == MOUSE_WHEEL_DOWN )
		{
			if ( pressed )
			{
				return rootPath + "Mouse/Mouse_MWheelDown_Unpressed_00.png";
			}
			else
			{
				return rootPath + "Mouse/Mouse_MWheelDown_Pressed_00.png";
			}
		}
		else if ( binding.mouseButton == MOUSE_WHEEL_UP )
		{
			if ( pressed )
			{
				return rootPath + "Mouse/Mouse_MWheelUp_Unpressed_00.png";
			}
			else
			{
				return rootPath + "Mouse/Mouse_MWheelUp_Pressed_00.png";
			}
		}
		else
		{
			return "";
		}
	}
	return "";
}

void Input::bind(const char* binding, const char* input) {
	auto b = bindings.find(binding);
	if (b == bindings.end()) {
		auto result = bindings.emplace(binding, binding_t());
		b = result.first;
	}
	(*b).second.input.assign(input);
	if (input == nullptr) {
		(*b).second.type = binding_t::INVALID;
		return;
	}

	size_t len = strlen(input);
	if (len >= 3 && strncmp(input, "Pad", 3) == 0) {
		// game controller

		char* type = nullptr;
		Uint32 index = strtol((const char*)(input + 3), &type, 10);
		auto& list = gameControllers;
		auto find = list.find(index);

		bool foundControllerForPlayer = false;
		SDL_GameController* pad = nullptr;
#ifdef NINTENDO
		if ( find != list.end() )
		{
			foundControllerForPlayer = true;
			pad = (*find).second;
		}
#else
#ifndef EDITOR
		if ( auto controller = ::inputs.getController(player) )
		{
			foundControllerForPlayer = true;
			pad = controller->getControllerDevice();
		}
#endif
#endif // !NINTENDO
		if ( foundControllerForPlayer ) {
			(*b).second.pad = pad;
			if (strncmp(type, "Button", 6) == 0) {
				if (strcmp((const char*)(type + 6), "A") == 0) {
#ifdef NINTENDO
					(*b).second.padButton = SDL_CONTROLLER_BUTTON_B;
#else
					(*b).second.padButton = SDL_CONTROLLER_BUTTON_A;
#endif
					(*b).second.type = binding_t::CONTROLLER_BUTTON;
					return;
				} else if (strcmp((const char*)(type + 6), "B") == 0) {
#ifdef NINTENDO
					(*b).second.padButton = SDL_CONTROLLER_BUTTON_A;
#else
					(*b).second.padButton = SDL_CONTROLLER_BUTTON_B;
#endif
					(*b).second.type = binding_t::CONTROLLER_BUTTON;
					return;
				} else if (strcmp((const char*)(type + 6), "X") == 0) {
#ifdef NINTENDO
					(*b).second.padButton = SDL_CONTROLLER_BUTTON_Y;
#else
					(*b).second.padButton = SDL_CONTROLLER_BUTTON_X;
#endif
					(*b).second.type = binding_t::CONTROLLER_BUTTON;
					return;
				} else if (strcmp((const char*)(type + 6), "Y") == 0) {
#ifdef NINTENDO
					(*b).second.padButton = SDL_CONTROLLER_BUTTON_X;
#else
					(*b).second.padButton = SDL_CONTROLLER_BUTTON_Y;
#endif
					(*b).second.type = binding_t::CONTROLLER_BUTTON;
					return;
				} else if (strcmp((const char*)(type + 6), "Back") == 0) {
					(*b).second.padButton = SDL_CONTROLLER_BUTTON_BACK;
					(*b).second.type = binding_t::CONTROLLER_BUTTON;
					return;
				} else if (strcmp((const char*)(type + 6), "Start") == 0) {
					(*b).second.padButton = SDL_CONTROLLER_BUTTON_START;
					(*b).second.type = binding_t::CONTROLLER_BUTTON;
					return;
				} else if (strcmp((const char*)(type + 6), "LeftStick") == 0) {
					(*b).second.padButton = SDL_CONTROLLER_BUTTON_LEFTSTICK;
					(*b).second.type = binding_t::CONTROLLER_BUTTON;
					return;
				} else if (strcmp((const char*)(type + 6), "RightStick") == 0) {
					(*b).second.padButton = SDL_CONTROLLER_BUTTON_RIGHTSTICK;
					(*b).second.type = binding_t::CONTROLLER_BUTTON;
					return;
				} else if (strcmp((const char*)(type + 6), "LeftBumper") == 0) {
					(*b).second.padButton = SDL_CONTROLLER_BUTTON_LEFTSHOULDER;
					(*b).second.type = binding_t::CONTROLLER_BUTTON;
					return;
				} else if (strcmp((const char*)(type + 6), "RightBumper") == 0) {
					(*b).second.padButton = SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
					(*b).second.type = binding_t::CONTROLLER_BUTTON;
					return;
				} else {
					(*b).second.type = binding_t::INVALID;
					return;
				}
			} else if (strncmp(type, "StickLeft", 9) == 0) {
				if (strcmp((const char*)(type + 9), "X-") == 0) {
					(*b).second.padAxisNegative = true;
					(*b).second.padAxis = SDL_CONTROLLER_AXIS_LEFTX;
					(*b).second.type = binding_t::CONTROLLER_AXIS;
					return;
				} else if (strcmp((const char*)(type + 9), "X+") == 0) {
					(*b).second.padAxisNegative = false;
					(*b).second.padAxis = SDL_CONTROLLER_AXIS_LEFTX;
					(*b).second.type = binding_t::CONTROLLER_AXIS;
					return;
				} else if (strcmp((const char*)(type + 9), "Y-") == 0) {
					(*b).second.padAxisNegative = true;
					(*b).second.padAxis = SDL_CONTROLLER_AXIS_LEFTY;
					(*b).second.type = binding_t::CONTROLLER_AXIS;
					return;
				} else if (strcmp((const char*)(type + 9), "Y+") == 0) {
					(*b).second.padAxisNegative = false;
					(*b).second.padAxis = SDL_CONTROLLER_AXIS_LEFTY;
					(*b).second.type = binding_t::CONTROLLER_AXIS;
					return;
				} else {
					(*b).second.type = binding_t::INVALID;
					return;
				}
			} else if (strncmp(type, "StickRight", 10) == 0) {
				if (strcmp((const char*)(type + 10), "X-") == 0) {
					(*b).second.padAxisNegative = true;
					(*b).second.padAxis = SDL_CONTROLLER_AXIS_RIGHTX;
					(*b).second.type = binding_t::CONTROLLER_AXIS;
					return;
				} else if (strcmp((const char*)(type + 10), "X+") == 0) {
					(*b).second.padAxisNegative = false;
					(*b).second.padAxis = SDL_CONTROLLER_AXIS_RIGHTX;
					(*b).second.type = binding_t::CONTROLLER_AXIS;
					return;
				} else if (strcmp((const char*)(type + 10), "Y-") == 0) {
					(*b).second.padAxisNegative = true;
					(*b).second.padAxis = SDL_CONTROLLER_AXIS_RIGHTY;
					(*b).second.type = binding_t::CONTROLLER_AXIS;
					return;
				} else if (strcmp((const char*)(type + 10), "Y+") == 0) {
					(*b).second.padAxisNegative = false;
					(*b).second.padAxis = SDL_CONTROLLER_AXIS_RIGHTY;
					(*b).second.type = binding_t::CONTROLLER_AXIS;
					return;
				} else {
					(*b).second.type = binding_t::INVALID;
					return;
				}
			} else if (strncmp(type, "Dpad", 4) == 0) {
				if (strcmp((const char*)(type + 4), "X-") == 0) {
					(*b).second.padButton = SDL_CONTROLLER_BUTTON_DPAD_LEFT;
					(*b).second.type = binding_t::CONTROLLER_BUTTON;
					return;
				} else if (strcmp((const char*)(type + 4), "X+") == 0) {
					(*b).second.padButton = SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
					(*b).second.type = binding_t::CONTROLLER_BUTTON;
					return;
				} else if (strcmp((const char*)(type + 4), "Y-") == 0) {
					(*b).second.padButton = SDL_CONTROLLER_BUTTON_DPAD_UP;
					(*b).second.type = binding_t::CONTROLLER_BUTTON;
					return;
				} else if (strcmp((const char*)(type + 4), "Y+") == 0) {
					(*b).second.padButton = SDL_CONTROLLER_BUTTON_DPAD_DOWN;
					(*b).second.type = binding_t::CONTROLLER_BUTTON;
					return;
				} else {
					(*b).second.type = binding_t::INVALID;
					return;
				}
			} else if (strncmp(type, "LeftTrigger", 11) == 0) {
				(*b).second.padAxisNegative = false;
				(*b).second.padAxis = SDL_CONTROLLER_AXIS_TRIGGERLEFT;
				(*b).second.type = binding_t::CONTROLLER_AXIS;
				return;
			} else if (strncmp(type, "RightTrigger", 12) == 0) {
				(*b).second.padAxisNegative = false;
				(*b).second.padAxis = SDL_CONTROLLER_AXIS_TRIGGERRIGHT;
				(*b).second.type = binding_t::CONTROLLER_AXIS;
				return;
			} else {
				(*b).second.type = binding_t::INVALID;
				return;
			}
		} else {
			(*b).second.type = binding_t::INVALID;
			return;
		}
	} else if (len >= 3 && strncmp(input, "Joy", 3) == 0) {
		// joystick

		char* type = nullptr;
		Uint32 index = strtol((const char*)(input + 3), &type, 10);
		auto& list = joysticks;
		auto find = list.find(index);
		if (find != list.end()) {
			SDL_Joystick* joystick = (*find).second;
			(*b).second.joystick = joystick;
			if (strncmp(type, "Button", 6) == 0) {
				(*b).second.type = binding_t::JOYSTICK_BUTTON;
				(*b).second.joystickButton = strtol((const char*)(type + 6), nullptr, 10);
				return;
			} else if (strncmp(type, "Axis-", 5) == 0) {
				(*b).second.type = binding_t::JOYSTICK_AXIS;
				(*b).second.joystickAxisNegative = true;
				(*b).second.joystickAxis = strtol((const char*)(type + 5), nullptr, 10);
				return;
			} else if (strncmp(type, "Axis+", 5) == 0) {
				(*b).second.type = binding_t::JOYSTICK_AXIS;
				(*b).second.joystickAxisNegative = false;
				(*b).second.joystickAxis = strtol((const char*)(type + 5), nullptr, 10);
				return;
			} else if (strncmp(type, "Hat", 3) == 0) {
				(*b).second.type = binding_t::JOYSTICK_HAT;
				(*b).second.joystickHat = strtol((const char*)(type + 3), nullptr, 10);
				if (type[3]) {
					if (strncmp((const char*)(type + 4), "LeftUp", 6) == 0) {
						(*b).second.joystickHatState = SDL_HAT_LEFTUP;
						return;
					} else if (strncmp((const char*)(type + 4), "Up", 2) == 0) {
						(*b).second.joystickHatState = SDL_HAT_UP;
						return;
					} else if (strncmp((const char*)(type + 4), "RightUp", 7) == 0) {
						(*b).second.joystickHatState = SDL_HAT_RIGHTUP;
						return;
					} else if (strncmp((const char*)(type + 4), "Right", 5) == 0) {
						(*b).second.joystickHatState = SDL_HAT_RIGHT;
						return;
					} else if (strncmp((const char*)(type + 4), "RightDown", 9) == 0) {
						(*b).second.joystickHatState = SDL_HAT_RIGHTDOWN;
						return;
					} else if (strncmp((const char*)(type + 4), "Down", 4) == 0) {
						(*b).second.joystickHatState = SDL_HAT_DOWN;
						return;
					} else if (strncmp((const char*)(type + 4), "LeftDown", 8) == 0) {
						(*b).second.joystickHatState = SDL_HAT_LEFTDOWN;
						return;
					} else if (strncmp((const char*)(type + 4), "Left", 4) == 0) {
						(*b).second.joystickHatState = SDL_HAT_LEFT;
						return;
					} else if (strncmp((const char*)(type + 4), "Centered", 8) == 0) {
						(*b).second.joystickHatState = SDL_HAT_CENTERED;
						return;
					} else {
						(*b).second.type = binding_t::INVALID;
						return;
					}
				}
			} else {
				(*b).second.type = binding_t::INVALID;
				return;
			}
		}

		return;
	} else if (len >= 5 && strncmp(input, "Mouse", 5) == 0) {
		// mouse
		(*b).second.type = binding_t::MOUSE_BUTTON;
		if ( (strncmp((const char*)(input + 5), "WheelUp", 7) == 0) )
		{
			(*b).second.mouseButton = MOUSE_WHEEL_UP;
			return;
		}
		else if ( (strncmp((const char*)(input + 5), "WheelDown", 9) == 0) )
		{
			(*b).second.mouseButton = MOUSE_WHEEL_DOWN;
			return;
		}
		Uint32 index = strtol((const char*)(input + 5), nullptr, 10);
		int result = std::min(index, 15U);
		(*b).second.mouseButton = result;
		return;
	} else {
		// keyboard
		(*b).second.type = binding_t::KEYBOARD;
		(*b).second.scancode = getScancodeFromName(input);
		return;
	}
}

void Input::update() {
	for (auto& pair : bindings) {
		auto& binding = pair.second;
		float oldAnalog = binding.analog;
		binding.analog = analogOf(binding);
		bool oldBinary = binding.binary;
		binding.binary = binaryOf(binding);
		if (oldBinary != binding.binary) {
			// unconsume the input whenever it's released or pressed again.
			if ( oldBinary && !binding.binary && !binding.consumed )
			{
				// detected a 'rising' edge of the binding being released
				binding.binaryRelease = true;
			}
			else
			{
				binding.binaryRelease = false;
			}

			binding.binaryReleaseConsumed = false;
			if (!binding.binary) {
			    binding.consumed = false;
			}

			if ( binding.binary && binding.binaryHeldTicks == 0 )
			{
				// start the held detection counter
				binding.binaryHeldTicks = ticks;
			}
			else if ( !binding.binary )
			{
				// button not pressed, reset the held counter
				binding.binaryHeldTicks = 0;
			}
		}
		
		const bool analogHigh = binding.analog > analogToggleThreshold;
		if ( (oldAnalog <= analogToggleThreshold && analogHigh)
			|| (oldAnalog > analogToggleThreshold && !analogHigh))
		{
			binding.analogConsumed = false;
			if ( analogHigh && binding.analogHeldTicks == 0 )
			{
				// start the held detection counter
				binding.analogHeldTicks = ticks;
			}
			else if ( !analogHigh )
			{
				// button not pressed, reset the held counter
				binding.analogHeldTicks = 0;
			}
		}
		else if ( analogHigh )
		{
			if ( binding.analogConsumed && (ticks - binding.analogHeldTicks) > BUTTON_ANALOG_REPEAT_TICKS )
			{
				binding.analogConsumed = false;
				binding.analogHeldTicks = ticks;
			}
		}
	}
}

void Input::updateReleasedBindings()
{
	for ( auto& pair : bindings ) 
	{
		pair.second.binaryReleaseConsumed = true;
	}
}

bool Input::binaryOf(binding_t& binding) {
	if (binding.type == binding_t::CONTROLLER_AXIS ||
		binding.type == binding_t::CONTROLLER_BUTTON) {
		SDL_GameController* pad = binding.pad;
		if (binding.type == binding_t::CONTROLLER_BUTTON) {
			return SDL_GameControllerGetButton(pad, binding.padButton) == 1;
		} else {
			if (binding.padAxisNegative) {
				return SDL_GameControllerGetAxis(pad, binding.padAxis) < -16384;
			} else {
				return SDL_GameControllerGetAxis(pad, binding.padAxis) > 16384;
			}
		}
	} else if (
		binding.type == binding_t::JOYSTICK_AXIS ||
		binding.type == binding_t::JOYSTICK_BUTTON ||
		binding.type == binding_t::JOYSTICK_HAT) {
		SDL_Joystick* joystick = binding.joystick;
		if (binding.type == binding_t::JOYSTICK_BUTTON) {
			return SDL_JoystickGetButton(joystick, binding.joystickButton) == 1;
		} else if (binding.type == binding_t::JOYSTICK_AXIS) {
			if (binding.joystickAxisNegative) {
				return SDL_JoystickGetAxis(joystick, binding.joystickAxis) < -16384;
			} else {
				return SDL_JoystickGetAxis(joystick, binding.joystickAxis) > 16384;
			}
		} else {
			return SDL_JoystickGetHat(joystick, binding.joystickHat) == binding.joystickHatState;
		}
	} else if (binding.type == binding_t::MOUSE_BUTTON) {
		return mouseButtons[binding.mouseButton];
	} else if (binding.type == binding_t::KEYBOARD) {
		SDL_Scancode key = binding.scancode;
		if (key != SDL_SCANCODE_UNKNOWN) {
			return keys[(int)key];
		}
	}

	return false;
}

float Input::analogOf(binding_t& binding) {
	if (binding.type == binding_t::CONTROLLER_AXIS ||
		binding.type == binding_t::CONTROLLER_BUTTON) {
		SDL_GameController* pad = binding.pad;
		if (binding.type == binding_t::CONTROLLER_BUTTON) {
			return SDL_GameControllerGetButton(pad, binding.padButton) ? 1.f : 0.f;
		} else {
			if (binding.padAxisNegative) {
				float result = std::min(SDL_GameControllerGetAxis(pad, binding.padAxis) / 32768.f, 0.f) * -1.f;
				return (fabs(result) > deadzone) ? result : 0.f;
			} else {
				float result = std::max(SDL_GameControllerGetAxis(pad, binding.padAxis) / 32767.f, 0.f);
				return (fabs(result) > deadzone) ? result : 0.f;
			}
		}
	} else if (
		binding.type == binding_t::JOYSTICK_AXIS ||
		binding.type == binding_t::JOYSTICK_BUTTON ||
		binding.type == binding_t::JOYSTICK_HAT) {
		SDL_Joystick* joystick = binding.joystick;
		if (binding.type == binding_t::JOYSTICK_BUTTON) {
			return SDL_JoystickGetButton(joystick, binding.joystickButton) ? 1.f : 0.f;
		} else if (binding.type == binding_t::JOYSTICK_AXIS) {
			if (binding.joystickAxisNegative) {
				float result = std::min(SDL_JoystickGetAxis(joystick, binding.joystickAxis) / 32768.f, 0.f) * -1.f;
				return (fabs(result) > deadzone) ? result : 0.f;
			} else {
				float result = std::max(SDL_JoystickGetAxis(joystick, binding.joystickAxis) / 32767.f, 0.f);
				return (fabs(result) > deadzone) ? result : 0.f;
			}
		} else {
			return SDL_JoystickGetHat(joystick, binding.joystickHat) == binding.joystickHatState ? 1.f : 0.f;
		}
	} else if (binding.type == binding_t::MOUSE_BUTTON) {
		return mouseButtons[binding.mouseButton] ? 1.f : 0.f;
	} else if (binding.type == binding_t::KEYBOARD) {
		SDL_Scancode key = binding.scancode;
		if (key != SDL_SCANCODE_UNKNOWN) {
			return keys[(int)key] ? 1.f : 0.f;
		}
	}

	return 0.f;
}

SDL_Scancode Input::getScancodeFromName(const char* name) {
	auto search = scancodeNames.find(name);
	if (search == scancodeNames.end()) {
		SDL_Scancode scancode = SDL_GetScancodeFromName(name);
		if (scancode != SDL_SCANCODE_UNKNOWN) {
			scancodeNames.emplace(name, scancode);
		}
		return scancode;
	} else {
		return (*search).second;
	}
}

Input::playerControlType_t Input::getPlayerControlType()
{
#ifndef EDITOR
	if ( ::inputs.hasController(player) )
	{
		return Input::PLAYER_CONTROLLED_BY_CONTROLLER;
	}
	if ( ::inputs.bPlayerUsingKeyboardControl(player) )
	{
		return Input::PLAYER_CONTROLLED_BY_KEYBOARD;
	}
#endif // !EDITOR
	return Input::PLAYER_CONTROLLED_BY_INVALID;
}

void Input::consumeBindingsSharedWithBinding(const char* binding)
{
#ifndef EDITOR
	if ( disabled )
	{
		return;
	}
	const std::pair<std::string, binding_t> checkBinding =
		std::make_pair(binding, input(binding));
	for ( auto& b : bindings )
	{
		if ( !b.second.binary )
		{
			continue; // don't pre-consume non-pressed buttons
		}
		if ( b.second.consumed )
		{
			continue; // no need to consume again
		}
		if ( b.second.type == checkBinding.second.type )
		{
			if ( b.first == checkBinding.first )
			{
				continue; // skip the hotbar bindings
			}
			if ( b.second.type == binding_t::CONTROLLER_AXIS ||
				b.second.type == binding_t::CONTROLLER_BUTTON )
			{
				if ( b.second.type == binding_t::CONTROLLER_BUTTON )
				{
					if ( b.second.padButton == checkBinding.second.padButton )
					{
						b.second.consumed = true;
					}
				}
				else
				{
					if ( b.second.padAxis == checkBinding.second.padAxis )
					{
						b.second.consumed = true;
					}
				}
			}
			else if (
				b.second.type == binding_t::JOYSTICK_AXIS ||
				b.second.type == binding_t::JOYSTICK_BUTTON ||
				b.second.type == binding_t::JOYSTICK_HAT )
			{
				if ( b.second.type == binding_t::JOYSTICK_BUTTON )
				{
					if ( b.second.joystickButton == checkBinding.second.joystickButton )
					{
						b.second.consumed = true;
					}
				}
				else if ( b.second.type == binding_t::JOYSTICK_AXIS )
				{
					if ( b.second.joystickAxis == checkBinding.second.joystickAxis )
					{
						b.second.consumed = true;
					}
				}
				else
				{
					if ( b.second.joystickHat == checkBinding.second.joystickHat )
					{
						b.second.consumed = true;
					}
				}
			}
			else if ( b.second.type == binding_t::MOUSE_BUTTON )
			{
				if ( b.second.mouseButton == checkBinding.second.mouseButton )
				{
					b.second.consumed = true;
					if ( b.second.mouseButton == MOUSE_WHEEL_DOWN
						|| b.second.mouseButton == MOUSE_WHEEL_UP )
					{
						mouseButtons[b.second.mouseButton] = false; // manually need to clear this
					}
				}
			}
			else if ( b.second.type == binding_t::KEYBOARD )
			{
				if ( b.second.scancode == checkBinding.second.scancode )
				{
					b.second.consumed = true;
				}
			}
		}
	}
#endif
}

void Input::consumeBindingsSharedWithFaceHotbar()
{
#ifndef EDITOR
	if ( disabled )
	{
		return;
	}
	if ( players[player]->hotbar.useHotbarFaceMenu )
	{
		if ( players[player]->hotbar.faceMenuButtonHeld != Player::Hotbar_t::FaceMenuGroup::GROUP_NONE )
		{
			const std::unordered_map<std::string, binding_t> faceMenuBindings =
			{
				std::make_pair("HotbarFacebarCancel", input("HotbarFacebarCancel")),
				std::make_pair("HotbarFacebarLeft", input("HotbarFacebarLeft")),
				std::make_pair("HotbarFacebarUp", input("HotbarFacebarUp")),
				std::make_pair("HotbarFacebarRight", input("HotbarFacebarRight")),
				std::make_pair("HotbarFacebarModifierLeft", input("HotbarFacebarModifierLeft")),
				std::make_pair("HotbarFacebarModifierRight", input("HotbarFacebarModifierRight"))
			};
			for ( auto& b : bindings )
			{
				if ( !b.second.binary )
				{
					continue; // don't pre-consume non-pressed buttons
				}
				if ( b.second.consumed )
				{
					continue; // no need to consume again
				}
				for ( auto& faceMenuBinding : faceMenuBindings )
				{
					if ( b.second.type == faceMenuBinding.second.type )
					{
						if ( b.first == faceMenuBinding.first )
						{
							continue; // skip the hotbar bindings
						}
						if ( b.second.type == binding_t::CONTROLLER_AXIS ||
							b.second.type == binding_t::CONTROLLER_BUTTON ) 
						{
							if ( b.second.type == binding_t::CONTROLLER_BUTTON )
							{
								if ( b.second.padButton == faceMenuBinding.second.padButton )
								{
									b.second.consumed = true;
								}
							}
							else 
							{
								if ( b.second.padAxis == faceMenuBinding.second.padAxis )
								{
									b.second.consumed = true;
								}
							}
						}
						else if (
							b.second.type == binding_t::JOYSTICK_AXIS ||
							b.second.type == binding_t::JOYSTICK_BUTTON ||
							b.second.type == binding_t::JOYSTICK_HAT ) 
						{
							if ( b.second.type == binding_t::JOYSTICK_BUTTON ) 
							{
								if ( b.second.joystickButton == faceMenuBinding.second.joystickButton )
								{
									b.second.consumed = true;
								}
							}
							else if ( b.second.type == binding_t::JOYSTICK_AXIS ) 
							{
								if ( b.second.joystickAxis == faceMenuBinding.second.joystickAxis )
								{
									b.second.consumed = true;
								}
							}
							else 
							{
								if ( b.second.joystickHat == faceMenuBinding.second.joystickHat )
								{
									b.second.consumed = true;
								}
							}
						}
						else if ( b.second.type == binding_t::MOUSE_BUTTON ) 
						{
							if ( b.second.mouseButton == faceMenuBinding.second.mouseButton )
							{
								b.second.consumed = true;
								if ( b.second.mouseButton == MOUSE_WHEEL_DOWN
									|| b.second.mouseButton == MOUSE_WHEEL_UP )
								{
									mouseButtons[b.second.mouseButton] = false; // manually need to clear this
								}
							}
						}
						else if ( b.second.type == binding_t::KEYBOARD ) 
						{
							if ( b.second.scancode == faceMenuBinding.second.scancode )
							{
								b.second.consumed = true;
							}
						}
					}
				}
			}
		}
	}
#endif
}
