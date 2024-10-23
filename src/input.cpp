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
std::unordered_map<std::string, SDL_Keycode> Input::keycodeNames;
std::unordered_map<int, SDL_GameController*> Input::gameControllers;
std::unordered_map<int, SDL_Joystick*> Input::joysticks;
std::unordered_map<SDL_Keycode, bool> Input::keys;
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
		// NOTE disabled on public release!!!
#ifdef NINTENDO_DEBUG
		inputs[c].gamepad_system_bindings.insert(std::make_pair("ConsoleCommand1", (std::string("Pad") + std::to_string(c) + std::string("ButtonLeftBumper")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("ConsoleCommand2", (std::string("Pad") + std::to_string(c) + std::string("ButtonRightBumper")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("ConsoleCommand3", (std::string("Pad") + std::to_string(c) + std::string("ButtonBack")).c_str()));
#endif

		inputs[c].gamepad_system_bindings.insert(std::make_pair("Console Command", "/"));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("MenuUp", (std::string("Pad") + std::to_string(c) + std::string("DpadY-")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("MenuLeft", (std::string("Pad") + std::to_string(c) + std::string("DpadX-")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("MenuRight", (std::string("Pad") + std::to_string(c) + std::string("DpadX+")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("MenuDown", (std::string("Pad") + std::to_string(c) + std::string("DpadY+")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("MenuConfirm", (std::string("Pad") + std::to_string(c) + std::string("ButtonA")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("MenuCancel", (std::string("Pad") + std::to_string(c) + std::string("ButtonB")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("MenuListCancel", (std::string("Pad") + std::to_string(c) + std::string("ButtonB")).c_str()));
#ifdef NINTENDO
		inputs[c].gamepad_system_bindings.insert(std::make_pair("MenuAlt1", (std::string("Pad") + std::to_string(c) + std::string("ButtonY")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("MenuAlt2", (std::string("Pad") + std::to_string(c) + std::string("ButtonX")).c_str()));
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

        /*
#ifdef NINTENDO
		inputs[c].gamepad_system_bindings.insert(std::make_pair("HotbarFacebarLeft", (std::string("Pad") + std::to_string(c) + std::string("ButtonY")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("HotbarFacebarUp", (std::string("Pad") + std::to_string(c) + std::string("ButtonX")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("HotbarFacebarRight", (std::string("Pad") + std::to_string(c) + std::string("ButtonB")).c_str()));
#else
		inputs[c].gamepad_system_bindings.insert(std::make_pair("HotbarFacebarLeft", (std::string("Pad") + std::to_string(c) + std::string("ButtonX")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("HotbarFacebarUp", (std::string("Pad") + std::to_string(c) + std::string("ButtonY")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("HotbarFacebarRight", (std::string("Pad") + std::to_string(c) + std::string("ButtonB")).c_str()));
#endif
        */
		inputs[c].gamepad_system_bindings.insert(std::make_pair("HotbarFacebarModifierLeft", (std::string("Pad") + std::to_string(c) + std::string("ButtonLeftBumper")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("HotbarFacebarModifierRight", (std::string("Pad") + std::to_string(c) + std::string("ButtonRightBumper")).c_str()));
		//inputs[c].gamepad_system_bindings.insert(std::make_pair("HotbarFacebarCancel", (std::string("Pad") + std::to_string(c) + std::string("DpadY+")).c_str()));

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

		inputs[c].gamepad_system_bindings.insert(std::make_pair("PaperDollContextMenu", (std::string("Pad") + std::to_string(c) + std::string("ButtonLeftStick")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("LogHome", (std::string("Pad") + std::to_string(c) + std::string("ButtonLeftStick")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("LogEnd", (std::string("Pad") + std::to_string(c) + std::string("ButtonRightStick")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("LogPageDown", (std::string("Pad") + std::to_string(c) + std::string("ButtonRightBumper")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("LogPageUp", (std::string("Pad") + std::to_string(c) + std::string("ButtonLeftBumper")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("LogScrollDown", (std::string("Pad") + std::to_string(c) + std::string("StickRightY+")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("LogClose", (std::string("Pad") + std::to_string(c) + std::string("ButtonB")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("LogScrollUp", (std::string("Pad") + std::to_string(c) + std::string("StickRightY-")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("MinimapPing", (std::string("Pad") + std::to_string(c) + std::string("ButtonA")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("MinimapClose", (std::string("Pad") + std::to_string(c) + std::string("ButtonB")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("MinimapRight", (std::string("Pad") + std::to_string(c) + std::string("StickRightX+")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("MinimapLeft", (std::string("Pad") + std::to_string(c) + std::string("StickRightX-")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("MinimapDown", (std::string("Pad") + std::to_string(c) + std::string("StickRightY+")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("MinimapUp", (std::string("Pad") + std::to_string(c) + std::string("StickRightY-")).c_str()));
		inputs[c].gamepad_system_bindings.insert(std::make_pair("ResetPortraitRotation", (std::string("Pad") + std::to_string(c) + std::string("ButtonRightStick")).c_str()));

		inputs[c].gamepad_system_bindings.insert(std::make_pair("GamepadLoginA", (std::string("Pad") + std::to_string(c) + std::string("ButtonA")).c_str()));
#ifndef NINTENDO
		inputs[c].gamepad_system_bindings.insert(std::make_pair("GamepadLoginB", (std::string("Pad") + std::to_string(c) + std::string("ButtonB")).c_str()));
#endif
		inputs[c].gamepad_system_bindings.insert(std::make_pair("GamepadLoginStart", (std::string("Pad") + std::to_string(c) + std::string("ButtonStart")).c_str()));

		inputs[c].kb_system_bindings.insert(std::make_pair("GamepadScreenshot", "F6"));
		inputs[c].kb_system_bindings.insert(std::make_pair("MenuMouseWheelUp", "MouseWheelUp")); // consumed automatically by frame.cpp
		inputs[c].kb_system_bindings.insert(std::make_pair("MenuMouseWheelDown", "MouseWheelDown")); // consumed automatically by frame.cpp
		inputs[c].kb_system_bindings.insert(std::make_pair("MenuMouseWheelUpAlt", "MouseWheelUp"));
		inputs[c].kb_system_bindings.insert(std::make_pair("MenuMouseWheelDownAlt", "MouseWheelDown"));
		inputs[c].kb_system_bindings.insert(std::make_pair("InventoryCharacterRotateLeftMouse", "MouseWheelUp"));
		inputs[c].kb_system_bindings.insert(std::make_pair("InventoryCharacterRotateRightMouse", "MouseWheelDown"));
		inputs[c].kb_system_bindings.insert(std::make_pair("MenuLeftClick", "Mouse1"));
		inputs[c].kb_system_bindings.insert(std::make_pair("MenuMiddleClick", "Mouse2"));
		inputs[c].kb_system_bindings.insert(std::make_pair("MenuRightClick", "Mouse3"));
		inputs[c].kb_system_bindings.insert(std::make_pair("InspectWithMouse", "Mouse1"));
		inputs[c].kb_system_bindings.insert(std::make_pair("MinimapPing", "Mouse1"));
		inputs[c].kb_system_bindings.insert(std::make_pair("ResetPortraitRotation", "Mouse2"));

		inputs[c].kb_system_bindings.insert(std::make_pair("KeyboardLogin", "Space"));

		inputs[c].kb_system_bindings.insert(std::make_pair("LogHome", "Home"));
		inputs[c].kb_system_bindings.insert(std::make_pair("LogEnd", "End"));
		inputs[c].kb_system_bindings.insert(std::make_pair("LogPageDown", "PageDown"));
		inputs[c].kb_system_bindings.insert(std::make_pair("LogPageUp", "PageUp"));
		inputs[c].kb_system_bindings.insert(std::make_pair("LogScrollDown", "MouseWheelDown"));
		inputs[c].kb_system_bindings.insert(std::make_pair("LogScrollUp", "MouseWheelUp"));

		inputs[c].kb_system_bindings.insert(std::make_pair("MenuUp", "Up"));
		inputs[c].kb_system_bindings.insert(std::make_pair("MenuLeft", "Left"));
		inputs[c].kb_system_bindings.insert(std::make_pair("MenuRight", "Right"));
		inputs[c].kb_system_bindings.insert(std::make_pair("MenuDown", "Down"));
		inputs[c].kb_system_bindings.insert(std::make_pair("MenuConfirm", "Space"));
		inputs[c].kb_system_bindings.insert(std::make_pair("MenuCancel", "Escape"));
		inputs[c].kb_system_bindings.insert(std::make_pair("MenuListCancel", "Escape"));
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
	if (multiplayer != SINGLE && player != 0) {
		return inputs[0].analog(binding);
	}
    if (disabled) { return 0.f; }
	auto b = bindings.find(binding);
	return b != bindings.end() ? (*b).second.analog : 0.f;
}

bool Input::binary(const char* binding) const {
	if (multiplayer != SINGLE && player != 0) {
		return inputs[0].binary(binding);
	}
    if (disabled) { return false; }
	auto b = bindings.find(binding);
	if (b == bindings.end()) {
		return false;
	} else {
		auto& bind = b->second;
		return bind.binary;
	}
	//return b != bindings.end() ? (*b).second.binary : false;
}

bool Input::binaryToggle(const char* binding) const {
	if (multiplayer != SINGLE && player != 0) {
		return inputs[0].binaryToggle(binding);
	}
    if (disabled) { return false; }
	auto b = bindings.find(binding);
	if (b == bindings.end()) {
		return false;
	} else {
		auto& bind = b->second;
		return bind.binary && !bind.consumed;
	}
	//return b != bindings.end() ? (*b).second.binary && !(*b).second.consumed : false;
}

bool Input::consumeBinary(const char* binding) {
	if (multiplayer != SINGLE && player != 0) {
		return inputs[0].consumeBinary(binding);
	}
	auto b = bindings.find(binding);
	if (b != bindings.end() && !(*b).second.consumed) {
		(*b).second.consumed = true;
		return disabled == false;
	} else {
		return false;
	}
}

bool Input::consumeBinaryToggle(const char* binding) {
	if (multiplayer != SINGLE && player != 0) {
		return inputs[0].consumeBinaryToggle(binding);
	}
	auto b = bindings.find(binding);
	if (b != bindings.end() && (*b).second.binary && !(*b).second.consumed) {
		(*b).second.consumed = true;
		return disabled == false;
	} else {
		return false;
	}
}

bool Input::binaryHeldToggle(const char* binding) const {
	if (multiplayer != SINGLE && player != 0) {
		return inputs[0].binaryHeldToggle(binding);
	}
    if (disabled) { return false; }
	auto b = bindings.find(binding);
	return b != bindings.end() 
		? ((*b).second.binary && !(*b).second.consumed && (ticks - (*b).second.heldTicks) > BUTTON_HELD_TICKS)
		: false;
}

const char* Input::binding(const char* binding) const {
	if (multiplayer != SINGLE && player != 0) {
		return inputs[0].binding(binding);
	}
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
	if ( getPlayerControlType() == playerControlType_t::PLAYER_CONTROLLED_BY_KEYBOARD )
	{
		printlog("keyboard bindings for player %d", player);
	    for (auto& binding : getKeyboardBindings() )
	    {
		    bind(binding.first.c_str(), binding.second.c_str());
	    }
	}
	if ( getPlayerControlType() == playerControlType_t::PLAYER_CONTROLLED_BY_CONTROLLER )
	{
		printlog("controller bindings for player %d", player);
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
	if ( getPlayerControlType() == playerControlType_t::PLAYER_CONTROLLED_BY_JOYSTICK )
	{
		printlog("joystick bindings for player %d", player);
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
	if (multiplayer != SINGLE && player != 0) {
		return inputs[0].input(binding);
	}
	auto b = bindings.find(binding);
	return b != bindings.end() ? (*b).second : Input::binding_t();
}

Input::ControllerType Input::getControllerType() const {
    return getControllerType(player);
}

#ifndef EDITOR
static ConsoleVariable<int> cvar_forceGlyphs("/forceglyphs", -1, "Force use of specific controller glyphs");
#endif

Input::ControllerType Input::getControllerType(int index) {
#if defined(EDITOR)
    return ControllerType::Xbox;
#elif defined(NINTENDO)
    // nintendo switch joycons
    if (*cvar_forceGlyphs >= 0) {
        return (ControllerType)*cvar_forceGlyphs;
    } else {
        return ControllerType::NintendoSwitch;
    }
#else
    if (*cvar_forceGlyphs >= 0) {
        return (ControllerType)*cvar_forceGlyphs;
    } else {
#ifdef STEAMWORKS
        if (SteamUtils()->IsSteamRunningOnSteamDeck()) {
            return ControllerType::SteamDeck;
        }
#endif
        // SDL lets us differentiate controller types
        const int device = ::inputs.getControllerID(index);
        auto type = SDL_GameControllerTypeForIndex(device);
        switch(type) {
        default:
        case SDL_CONTROLLER_TYPE_UNKNOWN: return ControllerType::Xbox;
        case SDL_CONTROLLER_TYPE_XBOX360: return ControllerType::Xbox;
        case SDL_CONTROLLER_TYPE_XBOXONE: return ControllerType::Xbox;
        case SDL_CONTROLLER_TYPE_PS3: return ControllerType::PlayStation;
        case SDL_CONTROLLER_TYPE_PS4: return ControllerType::PlayStation;
        case SDL_CONTROLLER_TYPE_PS5: return ControllerType::PlayStation;
        case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO: return ControllerType::NintendoSwitch;
        //case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_LEFT: return ControllerType::NintendoSwitch;
        //case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT: return ControllerType::NintendoSwitch;
        //case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_PAIR: return ControllerType::NintendoSwitch;
        }
    }
#endif
}

const char* Input::getKeyboardGlyph(int index) {
    return "*#images/ui/Glyphs/G_Control_KBM_01.png";
}

const char* Input::getKeyboardGlyph() const {
    return "*#images/ui/Glyphs/G_Control_KBM_01.png";
}

const char* Input::getControllerGlyph(int index) {
    switch (getControllerType(index)) {
    default:
    case ControllerType::Xbox:
        return "*#images/ui/Glyphs/G_Control_Xbox_02.png";
    case ControllerType::NintendoSwitch:
        return "*#images/ui/Glyphs/G_Control_Switch_01.png";
    case ControllerType::PlayStation:
        return "*#images/ui/Glyphs/G_Control_PS5_01.png";
    }
}

const char* Input::getControllerGlyph() const {
    return getControllerGlyph(player);
}

std::string Input::getGlyphPathForInput(const char* input, bool pressed, ControllerType type)
{
#ifndef EDITOR
    if (*cvar_hideGlyphs) {
        return "";
    }
#endif
    if (!input || !input[0]) {
        return "";
    }
    
    // base path
    const std::string rootPath = "images/ui/Glyphs/";
    
    if (type == ControllerType::PlayStation) {
        static const std::unordered_map<std::string, std::pair<std::string, std::string>> mappings = {
            {"Mouse1", {"Mouse/Mouse_LClick_Pressed_00.png", "Mouse/Mouse_LClick_Unpressed_00.png"}},
            {"Mouse2", {"Mouse/Mouse_MClick_Pressed_00.png", "Mouse/Mouse_MClick_Unpressed_00.png"}},
            {"Mouse3", {"Mouse/Mouse_RClick_Pressed_00.png", "Mouse/Mouse_RClick_Unpressed_00.png"}},
			{"Mouse4", {"Mouse/Mouse_M4_Pressed_00.png", "Mouse/Mouse_M4_Unpressed_00.png"}},
			{"Mouse5", {"Mouse/Mouse_M5_Pressed_00.png", "Mouse/Mouse_M5_Unpressed_00.png"}},
            {"MouseWheelDown", {"Mouse/Mouse_MWheelDown_Pressed_00.png", "Mouse/Mouse_MWheelDown_Unpressed_00.png"}},
            {"MouseWheelUp", {"Mouse/Mouse_MWheelUp_Pressed_00.png", "Mouse/Mouse_MWheelUp_Unpressed_00.png"}},
            {"ButtonA", {"G_PS_X00.png", "G_PS_X_Press00.png"}},
            {"ButtonB", {"G_PS_O00.png", "G_PS_O_Press00.png"}},
            {"ButtonX", {"G_PS_Box00.png", "G_PS_Box_Press00.png"}},
            {"ButtonY", {"G_PS_Tri00.png", "G_PS_Tri_Press00.png"}},
            {"ButtonLeftBumper", {"Button_PS_L1_00.png", "Button_PS_L1_Press_00.png"}},
            {"ButtonRightBumper", {"Button_PS_R1_00.png", "Button_PS_R1_Press_00.png"}},
            {"ButtonLeftStick", {"Stick_PS_L_00.png", "Stick_PS_L_Pressed_00.png"}},
            {"ButtonRightStick", {"Stick_PS_R_00.png", "Stick_PS_R_Pressed_00.png"}},
            {"ButtonStart", {"Button_OptC.png", "Button_OptC_Press00.png"}},
            {"ButtonBack", {"Button_Touchpad_PS5_00B.png", "Button_Touchpad_PS5_00C.png"}},
            {"StickLeftX-", {"Stick_PS_L_Left_00.png", "Stick_PS_L_Left_Pressed_00.png"}},
            {"StickLeftX+", {"Stick_PS_L_Right_00.png", "Stick_PS_L_Right_Pressed_00.png"}},
            {"StickLeftY-", {"Stick_PS_L_Up_00.png", "Stick_PS_L_Up_Pressed_00.png"}},
            {"StickLeftY+", {"Stick_PS_L_Down_00.png", "Stick_PS_L_Down_Pressed_00.png"}},
            {"StickRightX-", {"Stick_PS_R_Left_00.png", "Stick_PS_R_Left_Pressed_00.png"}},
            {"StickRightX+", {"Stick_PS_R_Right_00.png", "Stick_PS_R_Right_Pressed_00.png"}},
            {"StickRightY-", {"Stick_PS_R_Up_00.png", "Stick_PS_R_Up_Pressed_00.png"}},
            {"StickRightY+", {"Stick_PS_R_Down_00.png", "Stick_PS_R_Down_Pressed_00.png"}},
            {"LeftTrigger", {"Button_PS_L2_00.png", "Button_PS_L2_Press_00.png"}},
            {"RightTrigger", {"Button_PS_R2_00.png", "Button_PS_R2_Press_00.png"}},
            {"DpadX-", {"G_Direct_Left_Press00.png", "G_Direct_00.png"}},
            {"DpadX+", {"G_Direct_Right_Press00.png", "G_Direct_00.png"}},
            {"DpadY-", {"G_Direct_Up_Press00.png", "G_Direct_00.png"}},
            {"DpadY+", {"G_Direct_Down_Press00.png", "G_Direct_00.png"}},
        };
        
        // look for glyph in table
        auto find = mappings.find(input);
        if (find != mappings.end()) {
            auto& glyphs = find->second;
            return pressed ? rootPath + glyphs.second : rootPath + glyphs.first;
        }
    }
    else if (type == ControllerType::NintendoSwitch) {
        static const std::unordered_map<std::string, std::pair<std::string, std::string>> mappings = {
            {"Mouse1", {"Mouse/Mouse_LClick_Pressed_00.png", "Mouse/Mouse_LClick_Unpressed_00.png"}},
            {"Mouse2", {"Mouse/Mouse_MClick_Pressed_00.png", "Mouse/Mouse_MClick_Unpressed_00.png"}},
            {"Mouse3", {"Mouse/Mouse_RClick_Pressed_00.png", "Mouse/Mouse_RClick_Unpressed_00.png"}},
			{"Mouse4", {"Mouse/Mouse_M4_Pressed_00.png", "Mouse/Mouse_M4_Unpressed_00.png"}},
			{"Mouse5", {"Mouse/Mouse_M5_Pressed_00.png", "Mouse/Mouse_M5_Unpressed_00.png"}},
            {"MouseWheelDown", {"Mouse/Mouse_MWheelDown_Pressed_00.png", "Mouse/Mouse_MWheelDown_Unpressed_00.png"}},
            {"MouseWheelUp", {"Mouse/Mouse_MWheelUp_Pressed_00.png", "Mouse/Mouse_MWheelUp_Unpressed_00.png"}},
            {"ButtonA", {"Button_Xbox_DarkA_00.png", "Button_Xbox_DarkA_Press_00.png"}},
            {"ButtonB", {"Button_Xbox_DarkB_00.png", "Button_Xbox_DarkB_Press_00.png"}},
            {"ButtonX", {"Button_Xbox_DarkX_00.png", "Button_Xbox_DarkX_Press_00.png"}},
            {"ButtonY", {"Button_Xbox_DarkY_00.png", "Button_Xbox_DarkY_Press_00.png"}},
            {"ButtonLeftBumper", {"G_Switch_L00.png", "G_Switch_L_Press00.png"}},
            {"ButtonRightBumper", {"G_Switch_R00.png", "G_Switch_R_Press00.png"}},
            {"ButtonLeftStick", {"Stick_Switch_L_00.png", "Stick_Switch_L_Pressed_00.png"}},
            {"ButtonRightStick", {"Stick_Switch_R_00.png", "Stick_Switch_R_Pressed_00.png"}},
            {"ButtonStart", {"PlusMed00.png", "PlusMed_Press00.png"}},
            {"ButtonBack", {"MinusMed00.png", "MinusMed_Press00.png"}},
            {"StickLeftX-", {"Stick_Switch_L_Left_00.png", "Stick_Switch_L_Left_Pressed_00.png"}},
            {"StickLeftX+", {"Stick_Switch_L_Right_00.png", "Stick_Switch_L_Right_Pressed_00.png"}},
            {"StickLeftY-", {"Stick_Switch_L_Up_00.png", "Stick_Switch_L_Up_Pressed_00.png"}},
            {"StickLeftY+", {"Stick_Switch_L_Down_00.png", "Stick_Switch_L_Down_Pressed_00.png"}},
            {"StickRightX-", {"Stick_Switch_R_Left_00.png", "Stick_Switch_R_Left_Pressed_00.png"}},
            {"StickRightX+", {"Stick_Switch_R_Right_00.png", "Stick_Switch_R_Right_Pressed_00.png"}},
            {"StickRightY-", {"Stick_Switch_R_Up_00.png", "Stick_Switch_R_Up_Pressed_00.png"}},
            {"StickRightY+", {"Stick_Switch_R_Down_00.png", "Stick_Switch_R_Down_Pressed_00.png"}},
            {"LeftTrigger", {"G_Switch_ZL00.png", "G_Switch_ZL_Press00.png"}},
            {"RightTrigger", {"G_Switch_ZR00.png", "G_Switch_ZR_Press00.png"}},
            {"DpadX-", {"G_Switch_Direct_Left_Press00.png", "G_Switch_Direct_00.png"}},
            {"DpadX+", {"G_Switch_Direct_Right_Press00.png", "G_Switch_Direct_00.png"}},
            {"DpadY-", {"G_Switch_Direct_Up_Press00.png", "G_Switch_Direct_00.png"}},
            {"DpadY+", {"G_Switch_Direct_Down_Press00.png", "G_Switch_Direct_00.png"}},
        };
        
        // look for glyph in table
        auto find = mappings.find(input);
        if (find != mappings.end()) {
            auto& glyphs = find->second;
            return pressed ? rootPath + glyphs.second : rootPath + glyphs.first;
        }
    }
    else if (type == ControllerType::Xbox) {
        static const std::unordered_map<std::string, std::pair<std::string, std::string>> mappings =
        {
            {"Mouse1", {"Mouse/Mouse_LClick_Pressed_00.png", "Mouse/Mouse_LClick_Unpressed_00.png"}},
            {"Mouse2", {"Mouse/Mouse_MClick_Pressed_00.png", "Mouse/Mouse_MClick_Unpressed_00.png"}},
            {"Mouse3", {"Mouse/Mouse_RClick_Pressed_00.png", "Mouse/Mouse_RClick_Unpressed_00.png"}},
			{"Mouse4", {"Mouse/Mouse_M4_Pressed_00.png", "Mouse/Mouse_M4_Unpressed_00.png"}},
			{"Mouse5", {"Mouse/Mouse_M5_Pressed_00.png", "Mouse/Mouse_M5_Unpressed_00.png"}},
            {"MouseWheelDown", {"Mouse/Mouse_MWheelDown_Pressed_00.png", "Mouse/Mouse_MWheelDown_Unpressed_00.png"}},
            {"MouseWheelUp", {"Mouse/Mouse_MWheelUp_Pressed_00.png", "Mouse/Mouse_MWheelUp_Unpressed_00.png"}},
            {"ButtonA", {"Button_Xbox_DarkA_00.png", "Button_Xbox_DarkA_Press_00.png"}},
            {"ButtonB", {"Button_Xbox_DarkB_00.png", "Button_Xbox_DarkB_Press_00.png"}},
            {"ButtonX", {"Button_Xbox_DarkX_00.png", "Button_Xbox_DarkX_Press_00.png"}},
            {"ButtonY", {"Button_Xbox_DarkY_00.png", "Button_Xbox_DarkY_Press_00.png"}},
            {"ButtonLeftBumper", {"Button_Xbox_LB_00.png", "Button_Xbox_LB_Press_00.png"}},
            {"ButtonRightBumper", {"Button_Xbox_RB_00.png", "Button_Xbox_RB_Press_00.png"}},
            {"ButtonLeftStick", {"Stick_Xbox_L_00.png", "Stick_Xbox_L_Pressed_00.png"}},
            {"ButtonRightStick", {"Stick_Xbox_R_00.png", "Stick_Xbox_R_Pressed_00.png"}},
            {"ButtonStart", {"Button_Xbox_Menu_00.png", "Button_Xbox_Menu_Press_00.png"}},
            {"ButtonBack", {"Button_Xbox_View_00.png", "Button_Xbox_View_Press_00.png"}},
            {"StickLeftX-", {"Stick_Xbox_L_Left_00.png", "Stick_Xbox_L_Left_Pressed_00.png"}},
            {"StickLeftX+", {"Stick_Xbox_L_Right_00.png", "Stick_Xbox_L_Right_Pressed_00.png"}},
            {"StickLeftY-", {"Stick_Xbox_L_Up_00.png", "Stick_Xbox_L_Up_Pressed_00.png"}},
            {"StickLeftY+", {"Stick_Xbox_L_Down_00.png", "Stick_Xbox_L_Down_Pressed_00.png"}},
            {"StickRightX-", {"Stick_Xbox_R_Left_00.png", "Stick_Xbox_R_Left_Pressed_00.png"}},
            {"StickRightX+", {"Stick_Xbox_R_Right_00.png", "Stick_Xbox_R_Right_Pressed_00.png"}},
            {"StickRightY-", {"Stick_Xbox_R_Up_00.png", "Stick_Xbox_R_Up_Pressed_00.png"}},
            {"StickRightY+", {"Stick_Xbox_R_Down_00.png", "Stick_Xbox_R_Down_Pressed_00.png"}},
            {"LeftTrigger", {"Button_Xbox_LT_00.png", "Button_Xbox_LT_Press_00.png"}},
            {"RightTrigger", {"Button_Xbox_RT_00.png", "Button_Xbox_RT_Press_00.png"}},
            {"DpadX-", {"G_Direct_Left_Press00.png", "G_Direct_00.png"}},
            {"DpadX+", {"G_Direct_Right_Press00.png", "G_Direct_00.png"}},
            {"DpadY-", {"G_Direct_Up_Press00.png", "G_Direct_00.png"}},
            {"DpadY+", {"G_Direct_Down_Press00.png", "G_Direct_00.png"}},
        };
        
        // look for glyph in table
        auto find = mappings.find(input);
        if (find != mappings.end()) {
            auto& glyphs = find->second;
            return pressed ? rootPath + glyphs.second : rootPath + glyphs.first;
        }
    }
    else if (type == ControllerType::SteamDeck) {
        static const std::unordered_map<std::string, std::pair<std::string, std::string>> mappings =
        {
            {"Mouse1", {"Mouse/Mouse_LClick_Pressed_00.png", "Mouse/Mouse_LClick_Unpressed_00.png"}},
            {"Mouse2", {"Mouse/Mouse_MClick_Pressed_00.png", "Mouse/Mouse_MClick_Unpressed_00.png"}},
            {"Mouse3", {"Mouse/Mouse_RClick_Pressed_00.png", "Mouse/Mouse_RClick_Unpressed_00.png"}},
			{"Mouse4", {"Mouse/Mouse_M4_Pressed_00.png", "Mouse/Mouse_M4_Unpressed_00.png"}},
			{"Mouse5", {"Mouse/Mouse_M5_Pressed_00.png", "Mouse/Mouse_M5_Unpressed_00.png"}},
            {"MouseWheelDown", {"Mouse/Mouse_MWheelDown_Pressed_00.png", "Mouse/Mouse_MWheelDown_Unpressed_00.png"}},
            {"MouseWheelUp", {"Mouse/Mouse_MWheelUp_Pressed_00.png", "Mouse/Mouse_MWheelUp_Unpressed_00.png"}},
            {"ButtonA", {"Button_Xbox_DarkA_00.png", "Button_Xbox_DarkA_Press_00.png"}},
            {"ButtonB", {"Button_Xbox_DarkB_00.png", "Button_Xbox_DarkB_Press_00.png"}},
            {"ButtonX", {"Button_Xbox_DarkX_00.png", "Button_Xbox_DarkX_Press_00.png"}},
            {"ButtonY", {"Button_Xbox_DarkY_00.png", "Button_Xbox_DarkY_Press_00.png"}},
            {"ButtonLeftBumper", {"Button_PS_L1_00.png", "Button_PS_L1_Press_00.png"}},
            {"ButtonRightBumper", {"Button_PS_R1_00.png", "Button_PS_R1_Press_00.png"}},
            {"ButtonLeftStick", {"Stick_Xbox_L_00.png", "Stick_Xbox_L_Pressed_00.png"}},
            {"ButtonRightStick", {"Stick_Xbox_R_00.png", "Stick_Xbox_R_Pressed_00.png"}},
            {"ButtonStart", {"Button_Xbox_Menu_00.png", "Button_Xbox_Menu_Press_00.png"}},
            {"ButtonBack", {"Button_Xbox_View_00.png", "Button_Xbox_View_Press_00.png"}},
            {"StickLeftX-", {"Stick_Xbox_L_Left_00.png", "Stick_Xbox_L_Left_Pressed_00.png"}},
            {"StickLeftX+", {"Stick_Xbox_L_Right_00.png", "Stick_Xbox_L_Right_Pressed_00.png"}},
            {"StickLeftY-", {"Stick_Xbox_L_Up_00.png", "Stick_Xbox_L_Up_Pressed_00.png"}},
            {"StickLeftY+", {"Stick_Xbox_L_Down_00.png", "Stick_Xbox_L_Down_Pressed_00.png"}},
            {"StickRightX-", {"Stick_Xbox_R_Left_00.png", "Stick_Xbox_R_Left_Pressed_00.png"}},
            {"StickRightX+", {"Stick_Xbox_R_Right_00.png", "Stick_Xbox_R_Right_Pressed_00.png"}},
            {"StickRightY-", {"Stick_Xbox_R_Up_00.png", "Stick_Xbox_R_Up_Pressed_00.png"}},
            {"StickRightY+", {"Stick_Xbox_R_Down_00.png", "Stick_Xbox_R_Down_Pressed_00.png"}},
            {"LeftTrigger", {"Button_PS_L2_00.png", "Button_PS_L2_Press_00.png"}},
            {"RightTrigger", {"Button_PS_R2_00.png", "Button_PS_R2_Press_00.png"}},
            {"DpadX-", {"G_Direct_Left_Press00.png", "G_Direct_00.png"}},
            {"DpadX+", {"G_Direct_Right_Press00.png", "G_Direct_00.png"}},
            {"DpadY-", {"G_Direct_Up_Press00.png", "G_Direct_00.png"}},
            {"DpadY+", {"G_Direct_Down_Press00.png", "G_Direct_00.png"}},
        };
        
        // look for glyph in table
        auto find = mappings.find(input);
        if (find != mappings.end()) {
            auto& glyphs = find->second;
            return pressed ? rootPath + glyphs.second : rootPath + glyphs.first;
        }
    }

	// if the above lookups don't work, it's probably a keyboard glyph
	auto keycode = getKeycodeFromName(input);
	return GlyphHelper.getGlyphPath(keycode, pressed);
}

std::string Input::getGlyphPathForBinding(const char* binding, bool pressed) const
{
#ifndef EDITOR
	if (*cvar_hideGlyphs) {
		return "";
	}
#endif
	return getGlyphPathForBinding(input(binding), pressed);
}

std::string Input::getGlyphPathForBinding(const binding_t& binding, bool pressed)
{
	const auto prefix = binding.input.substr(0, 3);
	if (prefix == "Pad" || prefix == "Joy") {
        const int index = binding.input[3] - '0';
		return getGlyphPathForInput(binding.input.c_str() + 4, pressed, getControllerType(index));
	} else {
		return getGlyphPathForInput(binding.input.c_str(), pressed, ControllerType::Xbox);
	}
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
		Uint32 index = (Uint32)strtol((const char*)(input + 3), &type, 10);
		bool foundControllerForPlayer = false;
		SDL_GameController* pad = nullptr;
#ifndef EDITOR
		if ( auto controller = ::inputs.getController(player) )
		{
			foundControllerForPlayer = true;
			pad = controller->getControllerDevice();
		}
#endif
		if ( foundControllerForPlayer ) {
			(*b).second.pad = pad;
			(*b).second.padIndex = index;
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
					(*b).second.padButton = getControllerType() == ControllerType::PlayStation ?
                        SDL_CONTROLLER_BUTTON_TOUCHPAD : SDL_CONTROLLER_BUTTON_BACK;
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
		Uint32 index = (Uint32)strtol((const char*)(input + 3), &type, 10);
		auto& list = joysticks;
		auto find = list.find(index);
		if (find != list.end()) {
			SDL_Joystick* joystick = (*find).second;
			(*b).second.joystick = joystick;
			if (strncmp(type, "Button", 6) == 0) {
				(*b).second.type = binding_t::JOYSTICK_BUTTON;
				(*b).second.joystickButton = (Uint32)strtol((const char*)(type + 6), nullptr, 10);
				return;
			} else if (strncmp(type, "Axis-", 5) == 0) {
				(*b).second.type = binding_t::JOYSTICK_AXIS;
				(*b).second.joystickAxisNegative = true;
				(*b).second.joystickAxis = (Uint32)strtol((const char*)(type + 5), nullptr, 10);
				return;
			} else if (strncmp(type, "Axis+", 5) == 0) {
				(*b).second.type = binding_t::JOYSTICK_AXIS;
				(*b).second.joystickAxisNegative = false;
				(*b).second.joystickAxis = (Uint32)strtol((const char*)(type + 5), nullptr, 10);
				return;
			} else if (strncmp(type, "Hat", 3) == 0) {
				(*b).second.type = binding_t::JOYSTICK_HAT;
				(*b).second.joystickHat = (Uint32)strtol((const char*)(type + 3), nullptr, 10);
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
		Uint32 index = (Uint32)strtol((const char*)(input + 5), nullptr, 10);
		int result = std::min(index, 15U);
		(*b).second.mouseButton = result;
		return;
	} else {
		// keyboard
		(*b).second.type = binding_t::KEYBOARD;
		(*b).second.keycode = getKeycodeFromName(input);
		return;
	}
}

void Input::update() {
	for (auto& pair : bindings) {
		auto& binding = pair.second;
		//const float oldAnalog = binding.analog;
		binding.analog = analogOf(binding);
		const bool oldBinary = binding.binary;
		binding.binary = binaryOf(binding);
		if (oldBinary != binding.binary) {
			if (binding.binary) {
				if (binding.heldTicks == 0) {
					binding.heldTicks = ticks; // start the held detection counter
				}
			} else {
			    binding.consumed = false;
				binding.heldTicks = 0; // button not pressed, reset the held counter
			}
		}
	}
}

bool Input::binaryOf(binding_t& binding) {
	if (binding.type == binding_t::CONTROLLER_AXIS ||
		binding.type == binding_t::CONTROLLER_BUTTON) {
#ifdef NINTENDO
		if (binding.type == binding_t::CONTROLLER_BUTTON) {
			switch (binding.padButton) {
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_A: return nxGetControllerState(binding.padIndex, nxInput::ButtonB) == 1;
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_B: return nxGetControllerState(binding.padIndex, nxInput::ButtonA) == 1;
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_X: return nxGetControllerState(binding.padIndex, nxInput::ButtonY) == 1;
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_Y: return nxGetControllerState(binding.padIndex, nxInput::ButtonX) == 1;
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_UP: return nxGetControllerState(binding.padIndex, nxInput::ButtonUp) == 1;
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return nxGetControllerState(binding.padIndex, nxInput::ButtonRight) == 1;
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_DOWN: return nxGetControllerState(binding.padIndex, nxInput::ButtonDown) == 1;
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_LEFT: return nxGetControllerState(binding.padIndex, nxInput::ButtonLeft) == 1;
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_START: return nxGetControllerState(binding.padIndex, nxInput::ButtonPlus) == 1;
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_BACK: return nxGetControllerState(binding.padIndex, nxInput::ButtonMinus) == 1;
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_LEFTSTICK: return nxGetControllerState(binding.padIndex, nxInput::LeftStickClick) == 1;
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_RIGHTSTICK: return nxGetControllerState(binding.padIndex, nxInput::RightStickClick) == 1;
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return nxGetControllerState(binding.padIndex, nxInput::ButtonL) == 1;
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return nxGetControllerState(binding.padIndex, nxInput::ButtonR) == 1;
			default: return false;
			}
		}
		else {
			if (binding.padAxisNegative) {
				switch (binding.padAxis) {
				case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTX:
					return nxGetControllerState(binding.padIndex, nxInput::LeftStickX) < (INT16_MIN / 2);
				case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTY:
					return nxGetControllerState(binding.padIndex, nxInput::LeftStickY) < (INT16_MIN / 2);
				case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_RIGHTX:
					return nxGetControllerState(binding.padIndex, nxInput::RightStickX) < (INT16_MIN / 2);
				case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_RIGHTY:
					return nxGetControllerState(binding.padIndex, nxInput::RightStickY) < (INT16_MIN / 2);
				default:
					return false;
				}
			}
			else {
				switch (binding.padAxis) {
				case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTX:
					return nxGetControllerState(binding.padIndex, nxInput::LeftStickX) > (INT16_MAX / 2);
				case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTY:
					return nxGetControllerState(binding.padIndex, nxInput::LeftStickY) > (INT16_MAX / 2);
				case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_RIGHTX:
					return nxGetControllerState(binding.padIndex, nxInput::RightStickX) > (INT16_MAX / 2);
				case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_RIGHTY:
					return nxGetControllerState(binding.padIndex, nxInput::RightStickY) > (INT16_MAX / 2);
				case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_TRIGGERLEFT:
					return nxGetControllerState(binding.padIndex, nxInput::ButtonZL) == 1;
				case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
					return nxGetControllerState(binding.padIndex, nxInput::ButtonZR) == 1;
				default:
					return false;
				}
			}
		}
#else
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
#endif
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
		SDL_Keycode key = binding.keycode;
		if (key != SDLK_UNKNOWN) {
			return keys[(int)key];
		}
	}

	return false;
}

float Input::analogOf(binding_t& binding) {
	if (binding.type == binding_t::CONTROLLER_AXIS ||
		binding.type == binding_t::CONTROLLER_BUTTON) {
#ifdef NINTENDO
		if (binding.type == binding_t::CONTROLLER_BUTTON) {
			switch (binding.padButton) {
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_A: return (float)nxGetControllerState(binding.padIndex, nxInput::ButtonB);
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_B: return (float)nxGetControllerState(binding.padIndex, nxInput::ButtonA);
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_X: return (float)nxGetControllerState(binding.padIndex, nxInput::ButtonY);
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_Y: return (float)nxGetControllerState(binding.padIndex, nxInput::ButtonX);
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_UP: return (float)nxGetControllerState(binding.padIndex, nxInput::ButtonUp);
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return (float)nxGetControllerState(binding.padIndex, nxInput::ButtonRight);
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_DOWN: return (float)nxGetControllerState(binding.padIndex, nxInput::ButtonDown);
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_LEFT: return (float)nxGetControllerState(binding.padIndex, nxInput::ButtonLeft);
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_START: return (float)nxGetControllerState(binding.padIndex, nxInput::ButtonPlus);
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_BACK: return (float)nxGetControllerState(binding.padIndex, nxInput::ButtonMinus);
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_LEFTSTICK: return (float)nxGetControllerState(binding.padIndex, nxInput::LeftStickClick);
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_RIGHTSTICK: return (float)nxGetControllerState(binding.padIndex, nxInput::RightStickClick);
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return (float)nxGetControllerState(binding.padIndex, nxInput::ButtonL);
			case SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return (float)nxGetControllerState(binding.padIndex, nxInput::ButtonR);
			default: return 0.f;
			}
		}
		else {
			float result = 0.f;
			if (binding.padAxisNegative) {
				switch (binding.padAxis) {
				case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTX:
					result = std::max((float)nxGetControllerState(binding.padIndex, nxInput::LeftStickX) / INT16_MIN, 0.f); break;
				case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTY:
					result = std::max((float)nxGetControllerState(binding.padIndex, nxInput::LeftStickY) / INT16_MIN, 0.f); break;
				case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_RIGHTX:
					result = std::max((float)nxGetControllerState(binding.padIndex, nxInput::RightStickX) / INT16_MIN, 0.f); break;
				case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_RIGHTY:
					result = std::max((float)nxGetControllerState(binding.padIndex, nxInput::RightStickY) / INT16_MIN, 0.f); break;
				}
			}
			else {
				switch (binding.padAxis) {
				case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTX:
					result = std::max((float)nxGetControllerState(binding.padIndex, nxInput::LeftStickX) / INT16_MAX, 0.f); break;
				case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTY:
					result = std::max((float)nxGetControllerState(binding.padIndex, nxInput::LeftStickY) / INT16_MAX, 0.f); break;
				case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_RIGHTX:
					result = std::max((float)nxGetControllerState(binding.padIndex, nxInput::RightStickX) / INT16_MAX, 0.f); break;
				case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_RIGHTY:
					result = std::max((float)nxGetControllerState(binding.padIndex, nxInput::RightStickY) / INT16_MAX, 0.f); break;
				case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_TRIGGERLEFT:
					result = nxGetControllerState(binding.padIndex, nxInput::ButtonZL); break;
				case SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
					result = nxGetControllerState(binding.padIndex, nxInput::ButtonZR); break;
				}
			}
			return (fabs(result) > deadzone) ? result : 0.f;
		}
#else
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
#endif
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
		SDL_Keycode key = binding.keycode;
		if (key != SDLK_UNKNOWN) {
			return keys[(int)key] ? 1.f : 0.f;
		}
	}

	return 0.f;
}

SDL_Keycode Input::getKeycodeFromName(const char* name) {
	auto search = keycodeNames.find(name);
	if (search == keycodeNames.end()) {
		SDL_Keycode keycode = SDL_GetKeyFromName(name);
		if (keycode != SDLK_UNKNOWN) {
            keycodeNames.emplace(name, keycode);
		}
		return keycode;
	} else {
		return (*search).second;
	}
}

Input::playerControlType_t Input::getPlayerControlType()
{
	if (multiplayer != SINGLE && player != 0) {
		return inputs[0].getPlayerControlType();
	}
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

std::vector<std::string> Input::getBindingsForInput(const char* input) const {
    std::vector<std::string> result;
    for (auto& b : bindings) {
        const bool isController =
            b.second.type == binding_t::bindtype_t::CONTROLLER_AXIS ||
            b.second.type == binding_t::bindtype_t::CONTROLLER_BUTTON;
        if (isController) {
            if (b.second.input.substr(4) == input) {
                result.emplace_back(b.first);
            }
        } else {
            if (b.second.input == input) {
                result.emplace_back(b.first);
            }
        }
    }
    return result;
}

bool Input::bindingIsSharedWithKeyboardSystemBinding(const char* binding)
{
	if ( multiplayer != SINGLE && player != 0 ) {
		return inputs[0].bindingIsSharedWithKeyboardSystemBinding(binding);
	}
#ifndef EDITOR
	if ( disabled )
	{
		return false;
	}

	const std::pair<std::string, binding_t> checkBinding =
		std::make_pair(binding, input(binding));
	if ( !checkBinding.second.isBindingUsingKeyboard() )
	{
		return false;
	}

	if ( checkBinding.second.type == Input::binding_t::bindtype_t::KEYBOARD )
	{
		switch ( checkBinding.second.keycode )
		{
			case SDLK_LSHIFT:
			case SDLK_RSHIFT:
			case SDLK_LALT:
			case SDLK_RALT:
			case SDLK_LCTRL:
			case SDLK_RCTRL:
			case SDLK_SLASH:
				return true;
			default:
				break;
		}
	}

	if ( checkBinding.second.input.find("Mouse") != std::string::npos )
	{
		return true;
	}
#endif
	return false;
}

void Input::consumeBindingsSharedWithBinding(const char* binding)
{
	if (multiplayer != SINGLE && player != 0) {
		inputs[0].consumeBindingsSharedWithBinding(binding);
		return;
	}
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
				}
			}
			else if ( b.second.type == binding_t::KEYBOARD )
			{
				if ( b.second.keycode == checkBinding.second.keycode )
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
	if (multiplayer != SINGLE && player != 0) {
		inputs[0].consumeBindingsSharedWithFaceHotbar();
		return;
	}
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
				std::make_pair("Hotbar Right", input("Hotbar Right")),
				std::make_pair("Hotbar Left", input("Hotbar Left")),
				std::make_pair("Hotbar Up / Select", input("Hotbar Up / Select")),
				std::make_pair("Hotbar Down / Cancel", input("Hotbar Down / Cancel")),
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
							}
						}
						else if ( b.second.type == binding_t::KEYBOARD ) 
						{
							if ( b.second.keycode == faceMenuBinding.second.keycode )
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
