#include "MainMenu.hpp"
#include "Frame.hpp"
#include "Image.hpp"
#include "Field.hpp"
#include "Button.hpp"
#include "Slider.hpp"
#include "Text.hpp"
#include "GameUI.hpp"

#include "../init.hpp"
#include "../net.hpp"
#include "../player.hpp"
#include "../stat.hpp"
#include "../menu.hpp"
#include "../scores.hpp"
#include "../mod_tools.hpp"
#include "../interface/interface.hpp"
#include "../draw.hpp"
#include "../engine/audio/sound.hpp"
#include "../classdescriptions.hpp"
#include "../lobbies.hpp"
#include "../interface/consolecommand.hpp"
#include "../interface/ui.hpp"
#include "../eos.hpp"
#include "../colors.hpp"

#include <cassert>
#include <functional>

// quick restart:

#ifdef NINTENDO
#define NETWORK_PORT_CLIENT 56175
#define NETWORK_SCAN_PORT_CLIENT 56176
#else
#define NETWORK_PORT_CLIENT 0
#define NETWORK_SCAN_PORT_CLIENT 0
#endif

// all platforms but windows need to restart the game
// to apply video mode changes (resolution, window mode, etc)
#ifndef WINDOWS
#define VIDEO_RESTART_NEEDED
#endif

namespace MainMenu {
    int pause_menu_owner = 0;
	bool cursor_delete_mode = false;
	Frame* main_menu_frame = nullptr;

    constexpr int MIN_FPS = 30;
    constexpr int MAX_FPS = 300;
    constexpr int AUTO_FPS = MAX_FPS + 1;
	constexpr int MAX_LOBBY_FILTERS_SAVED = 32;

	// ALL NEW menu options:
	std::string current_audio_device;
	float master_volume = 1.f;
	bool arachnophobia_filter = false;
	bool hidden_roomcode = false;
	ConsoleVariable<bool> vertical_splitscreen("/vertical_splitscreen", false);
    ConsoleVariable<bool> staggered_splitscreen("/split_staggered", true);
    ConsoleVariable<bool> clipped_splitscreen("/split_clipped", true);
    static ConsoleVariable<int> clipped_size("/split_clipped_percent", 20);
    ConsoleVariable<bool> cvar_fastRestart("/fastrestart", false, "if true, game restarts 1 second after last player death");
	ConsoleVariable<bool> cvar_mkb_world_tooltips("/mkb_world_tooltips", true);
	ConsoleVariable<bool> cvar_mkb_facehotbar("/mkb_facehotbar", false);
	ConsoleVariable<bool> cvar_gamepad_facehotbar("/gamepad_facehotbar", true);
	ConsoleVariable<float> cvar_worldtooltip_scale("/worldtooltip_scale", 100.0);
	ConsoleVariable<float> cvar_worldtooltip_scale_splitscreen("/worldtooltip_scale_splitscreen", 150.0);
	ConsoleVariable<float> cvar_enemybar_scale("/enemybar_scale", 100.0);
    ConsoleVariable<int> cvar_desiredFps("/desiredfps", AUTO_FPS);
    ConsoleVariable<int> cvar_displayHz("/displayhz", 0);
	ConsoleVariable<bool> cvar_hdrEnabled("/hdr_enabled", true);
	static const int numFilters = NUM_SERVER_FLAGS + 3;
	enum Filter : int {
		UNCHECKED,
		OFF,
		ON,
		NUM,
	};
	static Filter lobbyFilters[numFilters] = { Filter::UNCHECKED };

	static ConsoleCommand ccmd_dumpcache("/dumpcache", "Dump UI asset caches",
	    [](int argc, const char** argv){
	    Image::dumpCache();
	    Text::dumpCache();
	    Font::dumpCache();
	    });

    // If you want to add new player-visible bindings, ADD THEM HERE:
    struct DefaultBinding {
        const std::string action;
        const std::string keyboard;
        const std::string gamepad;
        const std::string joystick;
    };
    struct BindingLayout {
        const std::string name;
        const std::vector<DefaultBinding> bindings;
    };
    static const std::vector<BindingLayout> defaultBindings = {
        {
            "Standard",
            {
                {"Attack", "Mouse1", "RightTrigger", emptyBinding},
                {"Use", "Mouse3", "ButtonA", emptyBinding},
                {"Cast Spell", "F", "ButtonLeftBumper", emptyBinding},
                {"Defend", "Space", "LeftTrigger", emptyBinding},
                {"Sneak", "Space", "LeftTrigger", emptyBinding},
                {"Character Status", "Tab", "ButtonBack", emptyBinding},
                {"Pause Game", hiddenBinding, "ButtonStart", emptyBinding},
                {"Spell List", "B", hiddenBinding, emptyBinding},
                {"Skill Sheet", "K", hiddenBinding, emptyBinding},
                {"Autosort Inventory", "R", hiddenBinding, emptyBinding},
                {"Command NPC", "Q", "DpadX-", emptyBinding},
                {"Show NPC Commands", "C", "DpadX+", emptyBinding},
                {"Cycle NPCs", "E", "DpadY-", emptyBinding},
                {"Open Map", "M", hiddenBinding, emptyBinding},
                {"Open Log", "L", hiddenBinding, emptyBinding},
                {"Toggle Minimap", "`", "ButtonRightStick", emptyBinding},
#ifdef NINTENDO
                {"Hotbar Left", "MouseWheelUp", "ButtonY", emptyBinding},
                {"Hotbar Right", "MouseWheelDown", "ButtonB", emptyBinding},
                {"Hotbar Up / Select", "Mouse2", "ButtonX", emptyBinding},
#else
                {"Hotbar Left", "MouseWheelUp", "ButtonX", emptyBinding},
                {"Hotbar Right", "MouseWheelDown", "ButtonB", emptyBinding},
                {"Hotbar Up / Select", "Mouse2", "ButtonY", emptyBinding},
#endif
                {"Hotbar Down / Cancel", hiddenBinding, "DpadY+", emptyBinding},
#ifdef NINTENDO
                {"Interact Tooltip Next", "R", "DpadY+", emptyBinding },
#else
				{"Interact Tooltip Next", "R", "DpadY+", emptyBinding },
#endif
                {"Interact Tooltip Prev", emptyBinding, emptyBinding, emptyBinding },
                {"Expand Inventory Tooltip", "X", hiddenBinding, emptyBinding },
                {"Quick Turn", emptyBinding, "ButtonLeftStick", emptyBinding },
                {"Chat", "Return", hiddenBinding, emptyBinding},
                {"Move Forward", "W", hiddenBinding, emptyBinding},
                {"Move Left", "A", hiddenBinding, emptyBinding},
                {"Move Backward", "S", hiddenBinding, emptyBinding},
                {"Move Right", "D", hiddenBinding, emptyBinding},
                {"Turn Left", "Left", hiddenBinding, emptyBinding},
                {"Turn Right", "Right", hiddenBinding, emptyBinding},
                {"Look Up", "Up", hiddenBinding, emptyBinding},
                {"Look Down", "Down", hiddenBinding, emptyBinding},
                {"Screenshot", "F6", hiddenBinding, hiddenBinding},
				{"Hotbar Slot 1", "1", hiddenBinding, hiddenBinding},
				{"Hotbar Slot 2", "2", hiddenBinding, hiddenBinding},
				{"Hotbar Slot 3", "3", hiddenBinding, hiddenBinding},
				{"Hotbar Slot 4", "4", hiddenBinding, hiddenBinding},
				{"Hotbar Slot 5", "5", hiddenBinding, hiddenBinding},
				{"Hotbar Slot 6", "6", hiddenBinding, hiddenBinding},
				{"Hotbar Slot 7", "7", hiddenBinding, hiddenBinding},
				{"Hotbar Slot 8", "8", hiddenBinding, hiddenBinding},
				{"Hotbar Slot 9", "9", hiddenBinding, hiddenBinding},
				{"Hotbar Slot 10", "0", hiddenBinding, hiddenBinding},
            }
        },
        {
            "Expert",
            {
                {"Attack", "Mouse1", "RightTrigger", emptyBinding},
                {"Use", "Mouse3", "ButtonRightBumper", emptyBinding},
                {"Cast Spell", "F", "ButtonLeftBumper", emptyBinding},
                {"Defend", "Space", "LeftTrigger", emptyBinding},
                {"Sneak", "Space", "LeftTrigger", emptyBinding},
                {"Character Status", "Tab", "ButtonBack", emptyBinding},
                {"Pause Game", hiddenBinding, "ButtonStart", emptyBinding},
                {"Spell List", "B", hiddenBinding, emptyBinding},
                {"Skill Sheet", "K", hiddenBinding, emptyBinding},
                {"Autosort Inventory", "R", hiddenBinding, emptyBinding},
                {"Command NPC", "Q", "DpadX-", emptyBinding},
                {"Show NPC Commands", "C", "DpadX+", emptyBinding},
                {"Cycle NPCs", "E", "DpadY-", emptyBinding},
                {"Open Map", "M", hiddenBinding, emptyBinding},
                {"Open Log", "L", hiddenBinding, emptyBinding},
                {"Toggle Minimap", "`", "ButtonRightStick", emptyBinding},
#ifdef NINTENDO
                {"Hotbar Left", "MouseWheelUp", "ButtonY", emptyBinding},
                {"Hotbar Right", "MouseWheelDown", "ButtonA", emptyBinding},
                {"Hotbar Up / Select", "Mouse2", "ButtonX", emptyBinding},
#else
                {"Hotbar Left", "MouseWheelUp", "ButtonX", emptyBinding},
                {"Hotbar Right", "MouseWheelDown", "ButtonB", emptyBinding},
                {"Hotbar Up / Select", "Mouse2", "ButtonY", emptyBinding},
#endif
                {"Hotbar Down / Cancel", hiddenBinding, "DpadY+", emptyBinding},
#ifdef NINTENDO
                {"Interact Tooltip Next", "R", "ButtonB", emptyBinding },
#else
                {"Interact Tooltip Next", "R", "ButtonA", emptyBinding },
#endif
                {"Interact Tooltip Prev", emptyBinding, emptyBinding, emptyBinding },
                {"Expand Inventory Tooltip", "X", hiddenBinding, emptyBinding },
                {"Quick Turn", emptyBinding, "ButtonLeftStick", emptyBinding },
                {"Chat", "Return", hiddenBinding, emptyBinding},
                {"Move Forward", "W", hiddenBinding, emptyBinding},
                {"Move Left", "A", hiddenBinding, emptyBinding},
                {"Move Backward", "S", hiddenBinding, emptyBinding},
                {"Move Right", "D", hiddenBinding, emptyBinding},
                {"Turn Left", "Left", hiddenBinding, emptyBinding},
                {"Turn Right", "Right", hiddenBinding, emptyBinding},
                {"Look Up", "Up", hiddenBinding, emptyBinding},
                {"Look Down", "Down", hiddenBinding, emptyBinding},
                {"Screenshot", "F6", hiddenBinding, hiddenBinding},
				{"Hotbar Slot 1", "1", hiddenBinding, hiddenBinding},
				{"Hotbar Slot 2", "2", hiddenBinding, hiddenBinding},
				{"Hotbar Slot 3", "3", hiddenBinding, hiddenBinding},
				{"Hotbar Slot 4", "4", hiddenBinding, hiddenBinding},
				{"Hotbar Slot 5", "5", hiddenBinding, hiddenBinding},
				{"Hotbar Slot 6", "6", hiddenBinding, hiddenBinding},
				{"Hotbar Slot 7", "7", hiddenBinding, hiddenBinding},
				{"Hotbar Slot 8", "8", hiddenBinding, hiddenBinding},
				{"Hotbar Slot 9", "9", hiddenBinding, hiddenBinding},
				{"Hotbar Slot 10", "0", hiddenBinding, hiddenBinding},
            }
        },
        {
            "Classic",
            {
                {"Attack", "Mouse1", "RightTrigger", emptyBinding},
                {"Use", "Mouse3", "ButtonA", emptyBinding},
                {"Cast Spell", "F", "ButtonLeftBumper", emptyBinding},
                {"Defend", "Space", "LeftTrigger", emptyBinding},
                {"Sneak", "Space", "LeftTrigger", emptyBinding},
                {"Character Status", "Tab", "ButtonBack", emptyBinding},
                {"Pause Game", hiddenBinding, "ButtonStart", emptyBinding},
                {"Spell List", "B", hiddenBinding, emptyBinding},
                {"Skill Sheet", "K", hiddenBinding, emptyBinding},
                {"Autosort Inventory", "R", hiddenBinding, emptyBinding},
#ifdef NINTENDO
                {"Command NPC", "Q", "ButtonY", emptyBinding},
                {"Show NPC Commands", "C", "ButtonX", emptyBinding},
                {"Cycle NPCs", "E", "ButtonB", emptyBinding},
#else
                {"Command NPC", "Q", "ButtonX", emptyBinding},
                {"Show NPC Commands", "C", "ButtonY", emptyBinding},
                {"Cycle NPCs", "E", "ButtonB", emptyBinding},
#endif
                {"Open Map", "M", hiddenBinding, emptyBinding},
                {"Open Log", "L", hiddenBinding, emptyBinding},
                {"Toggle Minimap", "`", "ButtonRightStick", emptyBinding},
                {"Hotbar Left", "MouseWheelUp", "DpadX-", emptyBinding},
                {"Hotbar Right", "MouseWheelDown", "DpadX+", emptyBinding},
                {"Hotbar Up / Select", "Mouse2", "DpadY-", emptyBinding},
                {"Hotbar Down / Cancel", hiddenBinding, "DpadY+", emptyBinding},
                {"Interact Tooltip Next", "R", "ButtonB", emptyBinding },
                {"Interact Tooltip Prev", emptyBinding, emptyBinding, emptyBinding },
                {"Expand Inventory Tooltip", "X", hiddenBinding, emptyBinding },
                {"Quick Turn", emptyBinding, "ButtonRightBumper", emptyBinding },
                {"Chat", "Return", hiddenBinding, emptyBinding},
                {"Move Forward", "W", hiddenBinding, emptyBinding},
                {"Move Left", "A", hiddenBinding, emptyBinding},
                {"Move Backward", "S", hiddenBinding, emptyBinding},
                {"Move Right", "D", hiddenBinding, emptyBinding},
                {"Turn Left", "Left", hiddenBinding, emptyBinding},
                {"Turn Right", "Right", hiddenBinding, emptyBinding},
                {"Look Up", "Up", hiddenBinding, emptyBinding},
                {"Look Down", "Down", hiddenBinding, emptyBinding},
                {"Screenshot", "F6", hiddenBinding, hiddenBinding},
				{"Hotbar Slot 1", "1", hiddenBinding, hiddenBinding},
				{"Hotbar Slot 2", "2", hiddenBinding, hiddenBinding},
				{"Hotbar Slot 3", "3", hiddenBinding, hiddenBinding},
				{"Hotbar Slot 4", "4", hiddenBinding, hiddenBinding},
				{"Hotbar Slot 5", "5", hiddenBinding, hiddenBinding},
				{"Hotbar Slot 6", "6", hiddenBinding, hiddenBinding},
				{"Hotbar Slot 7", "7", hiddenBinding, hiddenBinding},
				{"Hotbar Slot 8", "8", hiddenBinding, hiddenBinding},
				{"Hotbar Slot 9", "9", hiddenBinding, hiddenBinding},
				{"Hotbar Slot 10", "0", hiddenBinding, hiddenBinding},
            }
        },
        {
            "Minimal",
            {
                {"Attack", "Mouse1", "RightTrigger", emptyBinding},
                {"Use", "Mouse3", "ButtonA", emptyBinding},
                {"Cast Spell", "F", "ButtonX", emptyBinding},
                {"Defend", "Space", "LeftTrigger", emptyBinding},
                {"Sneak", "Space", "LeftTrigger", emptyBinding},
                {"Character Status", "Tab", "ButtonBack", emptyBinding},
                {"Pause Game", hiddenBinding, "ButtonStart", emptyBinding},
                {"Spell List", "B", hiddenBinding, emptyBinding},
                {"Skill Sheet", "K", hiddenBinding, emptyBinding},
                {"Autosort Inventory", "R", hiddenBinding, emptyBinding},
                {"Command NPC", "Q", "DpadX-", emptyBinding},
                {"Show NPC Commands", "C", "DpadX+", emptyBinding},
                {"Cycle NPCs", "E", "DpadY-", emptyBinding},
                {"Open Map", "M", hiddenBinding, emptyBinding},
                {"Open Log", "L", hiddenBinding, emptyBinding},
                {"Toggle Minimap", "`", emptyBinding, emptyBinding},
#ifdef NINTENDO
                {"Hotbar Left", "MouseWheelUp", "ButtonLeftBumper", emptyBinding},
                {"Hotbar Right", "MouseWheelDown", "ButtonRightBumper", emptyBinding},
                {"Hotbar Up / Select", "Mouse2", "ButtonX", emptyBinding},
#else
                {"Hotbar Left", "MouseWheelUp", "ButtonLeftBumper", emptyBinding},
                {"Hotbar Right", "MouseWheelDown", "ButtonRightBumper", emptyBinding},
                {"Hotbar Up / Select", "Mouse2", "ButtonY", emptyBinding},
#endif
                {"Hotbar Down / Cancel", hiddenBinding, emptyBinding, emptyBinding},
                {"Interact Tooltip Next", "R", "ButtonB", emptyBinding },
                {"Interact Tooltip Prev", emptyBinding, emptyBinding, emptyBinding },
                {"Expand Inventory Tooltip", "X", hiddenBinding, emptyBinding },
                {"Quick Turn", emptyBinding, emptyBinding, emptyBinding },
                {"Chat", "Return", hiddenBinding, emptyBinding},
                {"Move Forward", "W", hiddenBinding, emptyBinding},
                {"Move Left", "A", hiddenBinding, emptyBinding},
                {"Move Backward", "S", hiddenBinding, emptyBinding},
                {"Move Right", "D", hiddenBinding, emptyBinding},
                {"Turn Left", "Left", hiddenBinding, emptyBinding},
                {"Turn Right", "Right", hiddenBinding, emptyBinding},
                {"Look Up", "Up", hiddenBinding, emptyBinding},
                {"Look Down", "Down", hiddenBinding, emptyBinding},
                {"Screenshot", "F6", hiddenBinding, hiddenBinding},
				{"Hotbar Slot 1", "1", hiddenBinding, hiddenBinding},
				{"Hotbar Slot 2", "2", hiddenBinding, hiddenBinding},
				{"Hotbar Slot 3", "3", hiddenBinding, hiddenBinding},
				{"Hotbar Slot 4", "4", hiddenBinding, hiddenBinding},
				{"Hotbar Slot 5", "5", hiddenBinding, hiddenBinding},
				{"Hotbar Slot 6", "6", hiddenBinding, hiddenBinding},
				{"Hotbar Slot 7", "7", hiddenBinding, hiddenBinding},
				{"Hotbar Slot 8", "8", hiddenBinding, hiddenBinding},
				{"Hotbar Slot 9", "9", hiddenBinding, hiddenBinding},
				{"Hotbar Slot 10", "0", hiddenBinding, hiddenBinding},
            }
        },
    };

    static const char defaultControlLayout[] = "Standard";

    inline static const auto& getBindings(const char* name) {
        for (auto& layout : defaultBindings) {
            if (layout.name == name) {
                return layout.bindings;
            }
        }
        return defaultBindings[0].bindings; // Should be "Standard"
    }

	static int main_menu_buttons_height = 0;
	static Uint32 main_menu_ticks = 0u;
	static float main_menu_cursor_bob = 0.f;
	static int main_menu_cursor_x = 0;
	static int main_menu_cursor_y = 0;

    // if anything but none, causes the video mode to change
    // and reopens the settings menu to the specified tab
    enum VideoRefresh : int {
        None = 0,
        General = 1 << 0,
        Video = 1 << 1,
    };
    static int video_refresh = VideoRefresh::None;

	static FadeDestination main_menu_fade_destination = FadeDestination::None;
	static std::string tutorial_map_destination;

	enum class LobbyType {
	    None,
		LobbyLocal,
		LobbyLAN,
		LobbyOnline,
		LobbyJoined
	};

	static LobbyType currentLobbyType = LobbyType::None;
	static bool playersInLobby[MAXPLAYERS];
	static bool playerSlotsLocked[MAXPLAYERS];
	static bool newPlayer[MAXPLAYERS];
	static void* saved_invite_lobby = nullptr;

    bool story_active = false;
    bool isCutsceneActive() {
        return story_active;
    }

	bool isMenuOpen() {
		return main_menu_frame != nullptr;
	}

    static bool isMouseVisible() {
        auto cmouse = inputs.getVirtualMouse(inputs.getPlayerIDAllowedKeyboard());
        auto vmouse = intro ? cmouse : inputs.getVirtualMouse(getMenuOwner());
        return !vmouse->lastMovementFromController;
    }

	void beginFade(FadeDestination fd) {
		main_menu_fade_destination = fd;
		fadeout = true;
		fadefinished = false;
	}

	static const char* bigfont_outline = "fonts/pixelmix.ttf#16#2";
	static const char* bigfont_no_outline = "fonts/pixelmix.ttf#16#0";
	static const char* smallfont_outline = "fonts/pixel_maz_multiline.ttf#16#2";
	static const char* smallfont_no_outline = "fonts/pixel_maz_multiline.ttf#16#0";
	static const char* menu_option_font = "fonts/kongtext.ttf#16#2";
	static const char* banner_font = "fonts/pixelmix.ttf#16#2";

    // Inventory sorting options
	struct InventorySorting {
		bool hotbarWeapons = false;
		bool hotbarArmor = false;
		bool hotbarAmulets = false;
		bool hotbarBooks = false;
		bool hotbarTools = true;
		bool hotbarThrown = true;
		bool hotbarGems = false;
		bool hotbarPotions = false;
		bool hotbarScrolls = false;
		bool hotbarStaves = false;
		bool hotbarFood = false;
		bool hotbarSpells = true;
		int sortWeapons = 6;
		int sortArmor = 4;
		int sortAmulets = 3;
		int sortBooks = 2;
		int sortTools = 1;
		int sortThrown = 5;
		int sortGems = -2;
		int sortPotions = -6;
		int sortScrolls = -4;
		int sortStaves = -3;
		int sortFood = -5;
		int sortEquipped = 0;
		inline void save();
		static inline InventorySorting load();
		static inline InventorySorting reset();
		bool serialize(FileInterface*);
	};

	struct LastCreatedCharacter {
		static const int NUM_LAST_CHARACTERS = 6;
		static const int LASTCHAR_LAN_PERSONA_INDEX = 4;
		static const int LASTCHAR_ONLINE_PERSONA_INDEX = 5;
		int characterClass[NUM_LAST_CHARACTERS];
		int characterAppearance[NUM_LAST_CHARACTERS];
		int characterSex[NUM_LAST_CHARACTERS];
		int characterRace[NUM_LAST_CHARACTERS];
		std::string characterName[NUM_LAST_CHARACTERS];
		inline void save();
		static inline LastCreatedCharacter load();
		static inline LastCreatedCharacter reset();
		bool serialize(FileInterface*);
		LastCreatedCharacter()
		{
			for ( int i = 0; i < NUM_LAST_CHARACTERS; ++i )
			{
				characterClass[i] = -1;
				characterAppearance[i] = -1;
				characterSex[i] = -1;
				characterRace[i] = -1;
				characterName[i] = "";
			}
		}
	};

    // Binding options
	struct Bindings {
		std::unordered_map<std::string, std::string> kb_mouse_bindings[MAX_SPLITSCREEN];
		std::unordered_map<std::string, std::string> gamepad_bindings[MAX_SPLITSCREEN];
		std::unordered_map<std::string, std::string> joystick_bindings[MAX_SPLITSCREEN];
		inline void save();
		static inline Bindings load();
		static inline Bindings reset(const char* profile);
		bool serialize(FileInterface*);
	};

    // Minimap options
	struct Minimap {
		int map_scale = 100;
		int icon_scale = 100;
		int foreground_opacity = 50;
		int background_opacity = 0;
		inline void save();
		static inline Minimap load();
		static inline Minimap reset();
		bool serialize(FileInterface*);
	};

    // Message options
	struct Messages {
		bool combat = true;
		bool status = true;
		bool inventory = true;
		bool equipment = true;
		bool world = true;
		bool chat = true;
		bool progression = true;
		bool interaction = true;
		bool inspection = true;
		bool hint = true;
		bool obituary = true;
		inline void save();
		static inline Messages load();
		static inline Messages reset();
		bool serialize(FileInterface*);
	};

    // Video options
	struct Video {
		int window_mode = 0; // 0 = windowed, 1 = borderless, 2 = fullscreen
		int display_id = 0;
        int hz = 0;
		int resolution_x = 1280;
		int resolution_y = 720;
		bool vsync_enabled = true;
		float gamma = 100.f;
		inline bool save();
		static inline Video load();
		static inline Video reset();
		bool serialize(FileInterface*);
	};
	static struct Video old_video;

    // All menu options combined
	struct AllSettings {
	    std::vector<std::pair<std::string, std::string>> mods;
	    bool crossplay_enabled;
	    bool fast_restart = false;
		float world_tooltip_scale = 100.f;
		float world_tooltip_scale_splitscreen = 150.f;
		float enemybar_scale = 100.f;
		bool add_items_to_hotbar_enabled;
		InventorySorting inventory_sorting;
		LastCreatedCharacter lastCharacter;
		bool use_on_release_enabled;
        bool ui_filter_enabled = false;
        float ui_scale = 100.f;
		Minimap minimap;
		bool show_messages_enabled;
		Messages show_messages;
		bool show_player_nametags_enabled;
		bool show_hud_enabled;
		bool show_ip_address_enabled;
		bool content_control_enabled;
		bool colorblind_mode_enabled;
		bool arachnophobia_filter_enabled;
		bool shaking_enabled;
		bool bobbing_enabled;
		bool light_flicker_enabled;
        struct Video video;
		bool use_frame_interpolation = true;
		bool vertical_split_enabled;
		bool staggered_split_enabled;
		bool clipped_split_enabled;
		float item_tooltip_height = 100.f;
		int shootmode_crosshair = 0;
		int shootmode_crosshair_opacity = 50;
		bool hdr_enabled = true;
		float fov;
		float fps;
		std::string audio_device;
		float master_volume;
		float gameplay_volume;
		float ambient_volume;
		float environment_volume;
		float notification_volume;
		float music_volume;
		bool minimap_pings_enabled;
		bool player_monster_sounds_enabled;
		bool out_of_focus_audio_enabled;
		Bindings bindings;
		bool numkeys_in_inventory_enabled;
		bool mkb_world_tooltips_enabled = true;
		bool mkb_facehotbar = false;
		bool gamepad_facehotbar = true;
		float mouse_sensitivity;
		bool reverse_mouse_enabled;
		bool smooth_mouse_enabled;
		bool rotation_speed_limit_enabled;
		float turn_sensitivity_x;
		float turn_sensitivity_y;
		bool gamepad_camera_invert_x;
		bool gamepad_camera_invert_y;
		bool classic_mode_enabled;
		bool hardcore_mode_enabled;
		bool friendly_fire_enabled;
		bool keep_inventory_enabled;
		bool hunger_enabled;
		bool minotaur_enabled;
		bool random_traps_enabled;
		bool extra_life_enabled;
		bool cheats_enabled;
		bool skipintro;
		int port_number;
		bool show_lobby_code = false;
		std::vector<int> lobby_filter_settings;
		inline int save(); // non-zero if video needs restart
		static inline AllSettings load(bool video);
		static inline AllSettings reset();
		bool serialize(FileInterface*);
		AllSettings()
		{
			lobby_filter_settings.resize(MAX_LOBBY_FILTERS_SAVED);
			for ( auto& filter : lobby_filter_settings )
			{
				filter = 0;
			}
		};
	};

	int getMenuOwner() {
	    return intro ? clientnum : pause_menu_owner;
	}

	static inline void soundMove() {
		playSound(495, 48);
	}

	static inline void soundActivate() {
		playSound(493, 48);
	}

	static inline void soundCancel() {
		playSound(499, 48);
	}

	static inline void soundToggle() {
		playSound(492, 48);
	}

	static inline void soundCheckmark() {
		playSound(494, 48);
	}

	static inline void soundSlider(bool deafen_unless_gamepad = false) {
	    if (inputs.getVirtualMouse(getMenuOwner())->draw_cursor && deafen_unless_gamepad) {
	        return;
	    }
	    static Uint32 timeSinceLastTick = 0;
	    if (main_menu_ticks - timeSinceLastTick >= fpsLimit / 10) {
	        timeSinceLastTick = main_menu_ticks;
	        playSound(497, 48);
	    }
	}

	static inline void soundWarning() {
		playSound(496, 48);
	}

	static inline void soundError() {
		playSound(498, 48);
	}

	static inline void soundDeleteSave() {
		playSound(153, 48);
	}

	static BaronyRNG RNG;

    static int old_classes[MAXPLAYERS];
    static int old_races[MAXPLAYERS];
    static Uint32 old_appearances[MAXPLAYERS];
    static sex_t old_sexes[MAXPLAYERS];

/******************************************************************************/

	static ConsoleCommand ccmd_testFontDel("/testfont_del", "delete test font window",
	    [](int argc, const char** argv){
        assert(gui);
        auto frame = gui->findFrame("test_font_frame");
        if (frame) {
            frame->removeSelf();
        }
	    });

	static ConsoleCommand ccmd_testFont("/testfont", "display test font window (args: fontFilename minSize maxSize outlineSize)",
	    [](int argc, const char** argv){
        const char* font = argc >= 2 ? argv[1] : "fonts/alphbeta.ttf";
        const int minSize = argc >= 3 ? (int)strtol(argv[2], nullptr, 10) : 4;
        const int maxSize = argc >= 4 ? (int)strtol(argv[3], nullptr, 10) : 16;
        const int outline = argc >= 5 ? (int)strtol(argv[4], nullptr, 10) : 2;

        auto frame = gui->addFrame("test_font_frame");
        frame->setSize(SDL_Rect{16, 16, Frame::virtualScreenX - 32, Frame::virtualScreenY - 32});
        frame->setColor(makeColor(127, 127, 127, 255));
        frame->setBorder(0);

        auto button = frame->addButton("close");
        button->setSize(SDL_Rect{frame->getSize().w - 32, 0, 32, 32});
        button->setFont(smallfont_outline);
        button->setText("x");
        button->setCallback([](Button& button){
            auto parent = static_cast<Frame*>(button.getParent());
            parent->removeSelf();
            });

        const char str[] = "The quick brown fox jumps over the lazy dog.";

        assert(gui);
        char buf[256];
        int y = 0;
        for (int size = maxSize; size >= minSize; --size) {
            snprintf(buf, sizeof(buf), "%s#%d#%d", font, size, outline);
            auto field = frame->addField("field", sizeof(str));
            field->setText(str);
            field->setFont(buf);
            auto text = field->getTextObject();
            field->setSize(SDL_Rect{0, y, frame->getSize().w, (int)text->getHeight()});
            y += text->getHeight();
        }
	    });

/******************************************************************************/

	static void resetLobbyJoinFlowState() {
#ifdef STEAMWORKS
	    requestingLobbies = false;
        connectingToLobby = false;
        connectingToLobbyWindow = false;
        joinLobbyWaitingForHostResponse = false;
#endif
#ifdef USE_EOS
	    EOS.bRequestingLobbies = false;
	    EOS.bConnectingToLobby = false;
	    EOS.bConnectingToLobbyWindow = false;
	    EOS.bJoinLobbyWaitingForHostResponse = false;
#endif
	}

	static void flushP2PPackets(int msMin, int msMax) {
	    if (!directConnect) {
		    if (LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM) {
#ifdef STEAMWORKS
		        CSteamID newSteamID;

			    // if we got a packet, flush any remaining packets from the queue.
			    Uint32 startTicks = SDL_GetTicks();
			    Uint32 checkTicks = startTicks;
			    while ((checkTicks - startTicks) < msMin) {
				    SteamAPI_RunCallbacks();
				    Uint32 packetlen = 0;
				    if (SteamNetworking()->IsP2PPacketAvailable(&packetlen, 0)) {
					    packetlen = std::min<int>(packetlen, NET_PACKET_SIZE - 1);
					    Uint32 bytesRead = 0;
					    char buffer[NET_PACKET_SIZE];
					    if (SteamNetworking()->ReadP2PPacket(buffer, packetlen, &bytesRead, &newSteamID, 0)) {
						    checkTicks = SDL_GetTicks(); // found a packet, extend the wait time.
					    }
					    buffer[4] = '\0';
					    if ( (int)buffer[3] < '0'
						    && (int)buffer[0] == 0
						    && (int)buffer[1] == 0
						    && (int)buffer[2] == 0 ) {
						    printlog("[Steam Lobby]: Clearing P2P packet queue: received: %d", (int)buffer[3]);
					    } else {
						    printlog("[Steam Lobby]: Clearing P2P packet queue: received: %s", buffer);
					    }
				    }
				    SDL_Delay(10);
				    if ((SDL_GetTicks() - startTicks) > msMax) {
					    break;
				    }
			    }
#endif
		    }
		    else if (LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY) {
#if defined USE_EOS
		        EOS_ProductUserId newRemoteProductId = nullptr;

			    // if we got a packet, flush any remaining packets from the queue.
			    Uint32 startTicks = SDL_GetTicks();
			    Uint32 checkTicks = startTicks;
			    while ((checkTicks - startTicks) < msMin) {
				    EOS_Platform_Tick(EOS.PlatformHandle);
				    if (EOS.HandleReceivedMessagesAndIgnore(&newRemoteProductId)) {
					    checkTicks = SDL_GetTicks(); // found a packet, extend the wait time.
				    }
				    SDL_Delay(1);
				    if ((SDL_GetTicks() - startTicks) > msMax) {
					    break;
				    }
			    }
#endif // USE_EOS
		    }
	    }
	}

/******************************************************************************/

	static void settingsGeneral(Button&);
	static void settingsVideo(Button&);
	static void settingsAudio(Button&);
	static void settingsControls(Button&);
	static void settingsOnline(Button&);
	static void settingsGame(Button&);

	static void archivesLeaderboards(Button&);
	static void archivesDungeonCompendium(Button&);
	static void archivesAchievements(Button&);
	static void archivesStoryIntroduction(Button&);
	static void archivesCredits(Button&);
	static void archivesBackToMainMenu(Button&);

	static void playNew(Button&);
	static void playContinue(Button&);

	static void mainPlayGame(Button&);
	static void mainPlayModdedGame(Button&);
	static void mainArchives(Button&);
	static void mainAssignControllers(Button&);
	static void mainSettings(Button&);
	static void mainEditor(Button&);
	static void mainClose(Button&);
	static void mainEndLife(Button&);
	static void mainDropOut(Button&);
	static void mainRestartGame(Button&);
	static void mainReturnToHallofTrials(Button&);
	static void mainQuitToMainMenu(Button&);
	static void mainQuitToDesktop(Button&);

	static void characterCardGameFlagsMenu(int index);
	static void characterCardLobbySettingsMenu(int index);
	static void characterCardRaceMenu(int index, bool details, int selection);
	static void characterCardClassMenu(int index, bool details, int selection);

    static void createControllerPrompt(int index, bool show_player_text, void (*after_func)());
	static void createCharacterCard(int index);
	static void createLockedStone(int index);
	static void createStartButton(int index);
	static void createInviteButton(int index);
	static void createWaitingStone(int index);
	static void createReadyStone(int index, bool local, bool ready);
	static void createCountdownTimer();
	static void createLobby(LobbyType);
	static void createLobbyBrowser(Button&);
	static void createLocalOrNetworkMenu();
	static void refreshLobbyBrowser();

    static void sendPlayerOverNet();
    static void sendReadyOverNet(int index, bool ready);
    static void checkReadyStates();
    static void sendChatMessageOverNet(Uint32 color, const char* msg, size_t len);
    static void sendSvFlagsOverNet();
    static void doKeepAlive();
	static void handleNetwork();
	static void saveLastCharacter(const int index, int multiplayer);

/******************************************************************************/

    static constexpr int firePixelSize = 4;
    static constexpr Uint8 fireDefault = 63;

    static SDL_Surface* fireSurface = nullptr;
    static TempTexture* fireTexture = nullptr;

	static ConsoleVariable<bool> cvar_story_fire_fx("/story_fire_fx", false);

    static void fireStart() {
		if ( !*cvar_story_fire_fx )
		{
			return;
		}
        assert(!fireSurface);
        assert(!fireTexture);
	    fireSurface = SDL_CreateRGBSurface(0,
	        Frame::virtualScreenX / firePixelSize,
	        Frame::virtualScreenY / firePixelSize,
	        32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
		assert(fireSurface);
	    SDL_LockSurface(fireSurface);
        const int fireSize = (Frame::virtualScreenX * Frame::virtualScreenY) / (firePixelSize * firePixelSize);
	    Uint32* const sp = (Uint32*)fireSurface->pixels;
	    Uint32* const ep = (Uint32*)fireSurface->pixels + fireSize;
		constexpr Uint32 defaultColor = makeColor(0, 0, 0, fireDefault);
	    for (Uint32* p = sp; p < ep; ++p) {
            *p = defaultColor;
        }
	    SDL_UnlockSurface(fireSurface);
        fireTexture = new TempTexture();
    }

    static void fireStop() {
	    if (fireTexture) {
	        delete fireTexture;
	        fireTexture = nullptr;
	    }
	    if (fireSurface) {
            SDL_FreeSurface(fireSurface);
            fireSurface = nullptr;
	    }
    }

    static inline void fireUpdate(Uint32* p) {
        const int w = Frame::virtualScreenX / firePixelSize;
	    const int diff = std::max(0, RNG.uniform(-3, 1));
	    const int below = (p[w] & 0xff000000) >> 24;
	    const int intensity = std::max(below - diff, 0);
	    Uint32* const newPixel = std::max(p - diff, (Uint32*)fireSurface->pixels);
	    *newPixel = makeColor(0, 0, 0, intensity);
    }

    static void fire() {
		if ( !*cvar_story_fire_fx )
		{
			return;
		}
	    SDL_LockSurface(fireSurface);
        const int w = Frame::virtualScreenX / firePixelSize;
        const int fireSize = (Frame::virtualScreenX * Frame::virtualScreenY) / (firePixelSize * firePixelSize);
        const int size = fireSize - w;
	    Uint32* const sp = (Uint32*)fireSurface->pixels;
	    Uint32* const mp = (Uint32*)fireSurface->pixels + size;
	    for (Uint32* p = sp; p < mp; ++p) {
            fireUpdate(p);
	    }
	    constexpr Uint32 defaultColor = makeColor(0, 0, 0, fireDefault);
		Uint32* const ep = (Uint32*)fireSurface->pixels + fireSize;
	    for (Uint32* p = mp; p < ep; ++p) {
            *p = defaultColor;
	    }
	    SDL_UnlockSurface(fireSurface);
	    fireTexture->load(fireSurface, false, false);
    }

/******************************************************************************/

	void setupSplitscreen() {
		if (multiplayer != SINGLE) {
			splitscreen = false;
		    for (int c = 0; c < MAXPLAYERS; ++c) {
		        players[c]->bSplitscreen = false;
				players[c]->camera().winx = 0;
				players[c]->camera().winy = 0;
				players[c]->camera().winw = xres;
				players[c]->camera().winh = yres;
			}
#ifdef NINTENDO
			fpsLimit = 60;
#endif
			return;
		}

	    int playercount = 0;
	    for (int c = 0; c < MAX_SPLITSCREEN; ++c) {
		    if (client_disconnected[c]) {
			    continue;
		    }
			++playercount;
	    }
		splitscreen = playercount > 1;
		
		// on nintendo, we have to limit the FPS to 30 for 3+ players.
#ifdef NINTENDO
		fpsLimit = playercount > 2 ? 30 : 60;
#endif

		int c, playerindex;
		for (c = 0, playerindex = 0; c < MAX_SPLITSCREEN; ++c, ++playerindex) {
			if (client_disconnected[c]) {
				--playerindex;
				continue;
			}
			if (*vertical_splitscreen) {
				players[c]->splitScreenType = Player::SPLITSCREEN_VERTICAL;
			} else {
				players[c]->splitScreenType = Player::SPLITSCREEN_DEFAULT;
			}
			players[c]->bSplitscreen = splitscreen;

			if (!splitscreen) {
				players[c]->camera().winx = 0;
				players[c]->camera().winy = 0;
				players[c]->camera().winw = xres;
				players[c]->camera().winh = yres;
			} else {
				if (playercount == 1) {
					players[c]->camera().winx = 0;
					players[c]->camera().winy = 0;
					players[c]->camera().winw = xres;
					players[c]->camera().winh = yres;
				} else if (playercount == 2) {
				    const bool clipped = *clipped_splitscreen;
				    const bool staggered = *staggered_splitscreen;
					if (players[c]->splitScreenType == Player::SPLITSCREEN_VERTICAL) {
					    const int clip = (yres * *clipped_size) / 100;

						// divide screen vertically
						players[c]->camera().winx = playerindex * xres / 2;
						players[c]->camera().winy = clipped ? (staggered ? playerindex * clip : clip / 2) : 0;
						players[c]->camera().winw = xres / 2;
						players[c]->camera().winh = clipped ? (yres - clip) : yres;
					} else {
					    const int clip = (xres * *clipped_size) / 100;

						// divide screen horizontally
						players[c]->camera().winx = clipped ? (staggered ? playerindex * clip : clip / 2) : 0;
						players[c]->camera().winy = playerindex * yres / 2;
						players[c]->camera().winw = clipped ? (xres - clip) : xres;
						players[c]->camera().winh = yres / 2;
					}
				} else if (playercount >= 3) {
					// divide screen into quadrants
					players[c]->camera().winx = (playerindex % 2) * xres / 2;
					players[c]->camera().winy = (playerindex / 2) * yres / 2;
					players[c]->camera().winw = xres / 2;
					players[c]->camera().winh = yres / 2;
				}
			}

			inputs.getVirtualMouse(c)->x = players[c]->camera_x1() + players[c]->camera_width() / 2;
			inputs.getVirtualMouse(c)->y = players[c]->camera_y1() + players[c]->camera_height() / 2;
		}
	}

	static ConsoleCommand ccmd_setupSplitscreen("/split_refresh", "Refresh splitscreen layout",
        [](int argc, const char** argv){
            setupSplitscreen();
        });

/******************************************************************************/

	static void updateMenuCursor(Widget& widget) {
		Frame* buttons = static_cast<Frame*>(&widget);
		bool buttonSelected = false;
		for (auto button : buttons->getButtons()) {
			if (button->isSelected()) {
				main_menu_cursor_x = button->getSize().x - 80;
				main_menu_cursor_y = button->getSize().y - 9 + buttons->getSize().y;
				buttonSelected = true;
				break;
			}
		}

		// bob cursor
		const float bobrate = (float)PI * 2.f / (float)fpsLimit;
		main_menu_cursor_bob += bobrate;
		if (main_menu_cursor_bob >= (float)PI * 2.f) {
			main_menu_cursor_bob -= (float)PI * 2.f;
		}

		// update cursor position
		if (main_menu_frame) {
			auto cursor = main_menu_frame->findImage("cursor");
			if (cursor) {
				cursor->disabled = !buttonSelected;
				int diff = main_menu_cursor_y - cursor->pos.y;
				if (diff > 0) {
					diff = std::max(1, diff / 2);
				} else if (diff < 0) {
					diff = std::min(-1, diff / 2);
				}
				cursor->pos = SDL_Rect{
					main_menu_cursor_x + (int)(sinf(main_menu_cursor_bob) * 16.f) - 16,
					diff + cursor->pos.y,
					37 * 2,
					23 * 2
				};
			}
		}
	}

	static void tickMainMenu(Widget& widget) {
		++main_menu_ticks;
		auto back = widget.findWidget("back", false);
		if (back) {
		    back->setDisabled(widget.findWidget("dimmer", false) != nullptr);
		}
	}

	static void updateSliderArrows(Frame& frame) {
		bool drawSliders = false;
		auto selectedWidget = frame.findSelectedWidget(getMenuOwner());
		if (selectedWidget && selectedWidget->getType() == Widget::WIDGET_SLIDER) {
			auto slider = static_cast<Slider*>(selectedWidget);
			if (slider->isActivated() && slider->getOrientation() == Slider::SLIDER_HORIZONTAL) {
				drawSliders = true;
				auto left = frame.findImage("slider_left");
				if (left) {
					left->pos.x = slider->getHandleSize().x - 32;
					left->pos.y = slider->getRailSize().y + slider->getRailSize().h / 2 - 22;
					if (left->disabled) {
						left->disabled = false;
						soundActivate();
					}
				}
				auto right = frame.findImage("slider_right");
				if (right) {
					right->pos.x = slider->getHandleSize().x + slider->getHandleSize().w + 2;
					right->pos.y = slider->getRailSize().y + slider->getRailSize().h / 2 - 22;
					if (right->disabled) {
						right->disabled = false;
					}
				}
			}
		}
		if (drawSliders == false) {
			auto left = frame.findImage("slider_left");
			if (left) {
				left->disabled = true;
			}
			auto right = frame.findImage("slider_right");
			if (right) {
				right->disabled = true;
			}
		}
	}

	static void updateSettingSelection(Frame& frame) {
		auto& images = frame.getImages();
		for (auto image : images) {
			if (image->path == "*images/ui/Main Menus/Settings/Settings_Left_BackingSelect00.png") {
				image->path = "*images/ui/Main Menus/Settings/Settings_Left_Backing00.png";
			}
			if (image->path == "*images/ui/Main Menus/Settings/GenericWindow/Settings_Left_BackingSelect_Short00.png") {
				image->path = "*images/ui/Main Menus/Settings/GenericWindow/Settings_Left_Backing_Short00.png";
			}
		}
		auto selectedWidget = frame.findSelectedWidget(getMenuOwner());
		if (selectedWidget) {
			std::string setting;
			auto name = std::string(selectedWidget->getName());
			if (selectedWidget->getType() == Widget::WIDGET_SLIDER) {
				setting = name.substr(sizeof("setting_") - 1, name.size() - (sizeof("_slider") - 1) - (sizeof("setting_") - 1));
			} else if (selectedWidget->getType() == Widget::WIDGET_FIELD) {
			    setting = name.substr(sizeof("setting_") - 1, name.size() - (sizeof("_text_field") - 1) - (sizeof("setting_") - 1));
			} else if (selectedWidget->getType() == Widget::WIDGET_BUTTON) {
				auto button = static_cast<Button*>(selectedWidget);
				auto customize = "*images/ui/Main Menus/Settings/Settings_Button_Customize00.png";
				auto binding = "*images/ui/Main Menus/Settings/GenericWindow/Settings_Button_Binding00.png";
				auto dropdown = "*images/ui/Main Menus/Settings/Settings_Drop_ScrollBG02.png";
				auto dropdown_wide = "*images/ui/Main Menus/Settings/Settings_WideDrop_ScrollBG00.png";

				// Maybe we need a more sensible way to identify these button types.
				auto boolean_button_text = "Off          On";
				if (strcmp(button->getBackground(), customize) == 0) {
					setting = name.substr(sizeof("setting_") - 1, name.size() - (sizeof("_customize_button") - 1) - (sizeof("setting_") - 1));
				} else if (strcmp(button->getBackground(), binding) == 0) {
					setting = name.substr(sizeof("setting_") - 1, name.size() - (sizeof("_binding_button") - 1) - (sizeof("setting_") - 1));
				} else if (strcmp(button->getBackground(), dropdown) == 0) {
					setting = name.substr(sizeof("setting_") - 1, name.size() - (sizeof("_dropdown_button") - 1) - (sizeof("setting_") - 1));
				} else if (strcmp(button->getBackground(), dropdown_wide) == 0) {
					setting = name.substr(sizeof("setting_") - 1, name.size() - (sizeof("_dropdown_button") - 1) - (sizeof("setting_") - 1));
				} else if (strcmp(button->getText(), boolean_button_text) == 0) {
					setting = name.substr(sizeof("setting_") - 1, name.size() - (sizeof("_button") - 1) - (sizeof("setting_") - 1));
				} else {
				    assert(0 && "Unknown setting type!");
				}
			}
			if (!setting.empty()) {
				auto image = frame.findImage((std::string("setting_") + setting + std::string("_image")).c_str());
				if (image && image->path == "*images/ui/Main Menus/Settings/Settings_Left_Backing00.png") {
					image->path = "*images/ui/Main Menus/Settings/Settings_Left_BackingSelect00.png";
				}
				else if (image && image->path == "*images/ui/Main Menus/Settings/GenericWindow/Settings_Left_Backing_Short00.png") {
					image->path = "*images/ui/Main Menus/Settings/GenericWindow/Settings_Left_BackingSelect_Short00.png";
				}
				auto field = frame.findField((std::string("setting_") + setting + std::string("_field")).c_str());
				if (field) {
					static Widget* current_selected_widget = nullptr;
					if (current_selected_widget != selectedWidget) {
						current_selected_widget = selectedWidget;
						auto settings = static_cast<Frame*>(frame.getParent());
						auto tooltip = settings->findField("tooltip"); assert(tooltip);
						tooltip->setText(field->getGuide());
					}
				}
			}
		}
	}

	static Button* createBackWidget(Frame* parent, void (*callback)(Button&), SDL_Rect offset = SDL_Rect{4, 4, 0, 0}) {
		auto back = parent->addFrame("back");
		back->setSize(SDL_Rect{offset.x, offset.y, 78, 36});
		back->setActualSize(SDL_Rect{0, 0, 78, 36});
		back->setColor(0);
		back->setBorderColor(0);
		back->setBorder(0);
		auto backdrop = back->addImage(
			back->getActualSize(),
			0xffffffff,
			"images/ui/BackButton/UI_ButtonBack_base.png",
			"backdrop"
		);

		auto back_button = back->addButton("back_button");
		back_button->setSize(SDL_Rect{6, 0, 66, 36});
		back_button->setTextOffset(SDL_Rect{10, 12, 0, 0});
		back_button->setColor(0);
		back_button->setBorderColor(0);
		back_button->setHighlightColor(0);
		back_button->setBorder(0);
		back_button->setText("Back");
		back_button->setFont(smallfont_outline);
		back_button->setHJustify(Button::justify_t::LEFT);
		back_button->setVJustify(Button::justify_t::TOP);
		back_button->setCallback(callback);
		back_button->setGlyphPosition(Widget::glyph_position_t::CENTERED_RIGHT);
		back_button->setWidgetSearchParent(parent->getName());
		back_button->setWidgetBack("back_button");
		back_button->setTickCallback([](Widget& widget) {
			if (widget.isSelected()) {
			    widget.setButtonsOffset(SDL_Rect{23, 4, 0, 0,});
			} else {
			    widget.setButtonsOffset(SDL_Rect{1, 4, 0, 0,});
			}

			auto button = static_cast<Button*>(&widget); assert(button);
			auto parent = static_cast<Frame*>(widget.getParent()); assert(parent);
			auto image = parent->findImage("backdrop"); assert(image);
			if (button->isCurrentlyPressed()) {
				image->path = "images/ui/BackButton/UI_ButtonBack_press.png";
			} else {
				if (button->isHighlighted()) {
					image->path = "images/ui/BackButton/UI_ButtonBack_high.png";
				} else {
					image->path = "images/ui/BackButton/UI_ButtonBack_base.png";
				}
			}
			});

		return back_button;
	}

	static Frame* createPrompt(const char* name, bool issmall = true) {
	    if (!main_menu_frame) {
	        return nullptr;
	    }
		if (main_menu_frame->findFrame(name)) {
			return nullptr;
		}

		auto dimmer = main_menu_frame->addFrame("dimmer");
		dimmer->setSize(SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY});
		dimmer->setActualSize(dimmer->getSize());
		dimmer->setColor(makeColor(0, 0, 0, 63));
		dimmer->setBorder(0);

		auto frame = dimmer->addFrame(name);
		frame->setColor(0);
		frame->setBorder(0);
		if ( issmall ) {
		    frame->setSize(SDL_Rect{(Frame::virtualScreenX - 364) / 2, (Frame::virtualScreenY - 176) / 2, 364, 176});
		    frame->setActualSize(SDL_Rect{0, 0, 364, 176});
		    frame->addImage(
			    frame->getActualSize(),
			    0xffffffff,
			    "*#images/ui/Main Menus/Disconnect/UI_Disconnect_Window00.png",
			    "background"
		    );
		} else {
		    frame->setSize(SDL_Rect{(Frame::virtualScreenX - 548) / 2, (Frame::virtualScreenY - 264) / 2, 548, 264});
		    frame->setActualSize(SDL_Rect{0, 0, 548, 264});
		    frame->addImage(
			    frame->getActualSize(),
			    0xffffffff,
			    "*#images/ui/Main Menus/TextboxWindowL_00.png",
			    "background"
		    );
		}

		return frame;
	}

	static void closePrompt(const char* name) {
        if (!main_menu_frame) {
			return;
		}
        auto prompt = main_menu_frame->findFrame(name);
        if (prompt) {
            auto dimmer = static_cast<Frame*>(prompt->getParent()); assert(dimmer);
            dimmer->removeSelf();
        } else {
            printlog("no '%s' to delete!\n", name);
        }
	}

	static Frame* textFieldPrompt(
		const char* field_text,
	    const char* tip_text,
		const char* guide_text,
		const char* okay_text,
		const char* cancel_text,
		void (*okay_callback)(Button&),
		void (*cancel_callback)(Button&)
	) {
		soundActivate();

	    Frame* frame = createPrompt("text_field_prompt");
	    if (!frame) {
	        return nullptr;
	    }

		auto text_box = frame->addImage(
			SDL_Rect{(364 - 246) / 2, 32, 246, 36},
			0xffffffff,
			"*images/ui/Main Menus/TextField_00.png",
			"text_box"
		);

		constexpr int field_buffer_size = 128;

		auto tip = frame->addField("tip", field_buffer_size);
		tip->setSize(SDL_Rect{(364 - 242) / 2, 36, 242, 28});
		tip->setFont(smallfont_outline);
		tip->setText(tip_text);
		tip->setUserData(const_cast<void*>((const void*)tip_text));
		tip->setHJustify(Field::justify_t::LEFT);
		tip->setVJustify(Field::justify_t::CENTER);
		tip->setColor(makeColor(166, 123, 81, 127));
		tip->setBackgroundColor(makeColor(52, 30, 22, 255));
		tip->setTickCallback([](Widget& widget){
	        auto tip = static_cast<Field*>(&widget);
		    auto parent = static_cast<Frame*>(widget.getParent());
		    auto field = parent->findField("field");
		    if (field && field->getText()[0] != '\0') {
		        tip->setText("");
		    } else {
		        tip->setText((const char*)tip->getUserData());
		    }
		    });

		auto field = frame->addField("field", field_buffer_size);
		field->setGlyphPosition(Widget::glyph_position_t::CENTERED_RIGHT);
		field->setSelectorOffset(SDL_Rect{-7, -7, 7, 7});
		field->setButtonsOffset(SDL_Rect{11, 0, 0, 0});
		field->setEditable(true);
		field->setScroll(true);
		field->setGuide(guide_text);
		field->setSize(SDL_Rect{(364 - 242) / 2, 36, 242, 28});
		field->setFont(smallfont_outline);
		field->setText(field_text);
		field->setHJustify(Field::justify_t::LEFT);
		field->setVJustify(Field::justify_t::CENTER);
		field->setColor(makeColor(166, 123, 81, 255));
		field->setWidgetSearchParent(field->getParent()->getName());
		field->setWidgetBack("cancel");
		field->setWidgetDown("okay");
		field->select();
        field->activate();
		field->setTickCallback([](Widget& widget) {
			if (!main_menu_frame) {
				return;
			}
			auto selectedWidget = main_menu_frame->findSelectedWidget(widget.getOwner());
			if (!selectedWidget) {
				auto field = static_cast<Field*>(&widget);
				field->select();
			}
			});

		auto okay = frame->addButton("okay");
		okay->setBackground("*images/ui/Main Menus/Disconnect/UI_Disconnect_Button_GoBack00.png");
		okay->setBackgroundHighlighted("*images/ui/Main Menus/Disconnect/UI_Disconnect_Button_GoBackHigh00.png");
		okay->setBackgroundActivated("*images/ui/Main Menus/Disconnect/UI_Disconnect_Button_GoBackPress00.png");
		okay->setSize(SDL_Rect{58, 78, 108, 52});
		okay->setColor(makeColor(255, 255, 255, 255));
		okay->setHighlightColor(makeColor(255, 255, 255, 255));
		okay->setTextColor(makeColor(255, 255, 255, 255));
		okay->setTextHighlightColor(makeColor(255, 255, 255, 255));
		okay->setFont(smallfont_outline);
		okay->setText(okay_text);
		okay->setWidgetSearchParent(okay->getParent()->getName());
		okay->setWidgetUp("field");
		okay->setWidgetRight("cancel");
		okay->setWidgetBack("cancel");
		okay->setCallback(okay_callback);

		auto cancel = frame->addButton("cancel");
		cancel->setBackground("*images/ui/Main Menus/Disconnect/UI_Disconnect_Button_Abandon00.png");
		cancel->setBackgroundHighlighted("*images/ui/Main Menus/Disconnect/UI_Disconnect_Button_AbandonHigh00.png");
		cancel->setBackgroundActivated("*images/ui/Main Menus/Disconnect/UI_Disconnect_Button_AbandonPress00.png");
		cancel->setSize(SDL_Rect{174, 78, 130, 52});
		cancel->setColor(makeColor(255, 255, 255, 255));
		cancel->setHighlightColor(makeColor(255, 255, 255, 255));
		cancel->setTextColor(makeColor(255, 255, 255, 255));
		cancel->setTextHighlightColor(makeColor(255, 255, 255, 255));
		cancel->setFont(smallfont_outline);
		cancel->setText(cancel_text);
		cancel->setWidgetSearchParent(cancel->getParent()->getName());
		cancel->setWidgetUp("field");
		cancel->setWidgetLeft("okay");
		cancel->setWidgetBack("cancel");
		cancel->setCallback(cancel_callback);

		return frame;
	}

	static const char* closeTextField() {
		if (!main_menu_frame) {
			return "";
		}
        auto prompt = main_menu_frame->findFrame("text_field_prompt"); assert(prompt);
        auto field = prompt->findField("field"); assert(field);
        auto dimmer = static_cast<Frame*>(prompt->getParent()); assert(dimmer);
        dimmer->removeSelf();
	    return field->getText(); // note: this will only be valid for one frame!
	}

	static Frame* binaryPrompt(
		const char* window_text,
		const char* okay_text,
		const char* cancel_text,
		void (*okay_callback)(Button&),
		void (*cancel_callback)(Button&),
		bool leftRed = true,
		bool rightRed = false
	) {
		soundActivate();

	    Frame* frame = createPrompt("binary_prompt");
	    if (!frame) {
	        return nullptr;
	    }

		auto text = frame->addField("text", 1024);
		text->setSize(SDL_Rect{30, 28, 304, 46});
		text->setFont(smallfont_no_outline);
		text->setText(window_text);
		text->setJustify(Field::justify_t::CENTER);

		auto okay = frame->addButton("okay");
		okay->setSize(SDL_Rect{leftRed ? 58 : 72, 78, leftRed ? 130 : 108, 52});
		okay->setBackground(leftRed ?
		    "*images/ui/Main Menus/Disconnect/UI_Disconnect_Button_Abandon00.png" :
		    "*images/ui/Main Menus/Disconnect/UI_Disconnect_Button_GoBack00.png");
		okay->setBackgroundHighlighted(leftRed ?
		    "*images/ui/Main Menus/Disconnect/UI_Disconnect_Button_AbandonHigh00.png" :
		    "*images/ui/Main Menus/Disconnect/UI_Disconnect_Button_GoBackHigh00.png");
		okay->setBackgroundActivated(leftRed ?
		    "*images/ui/Main Menus/Disconnect/UI_Disconnect_Button_AbandonPress00.png" :
		    "*images/ui/Main Menus/Disconnect/UI_Disconnect_Button_GoBackPress00.png");
		okay->setColor(makeColor(255, 255, 255, 255));
		okay->setHighlightColor(makeColor(255, 255, 255, 255));
		okay->setTextColor(makeColor(255, 255, 255, 255));
		okay->setTextHighlightColor(makeColor(255, 255, 255, 255));
		okay->setFont(smallfont_outline);
		okay->setText(okay_text);
		okay->setWidgetSearchParent(okay->getParent()->getName());
		okay->setWidgetRight("cancel");
		okay->setWidgetBack("cancel");
		okay->setCallback(okay_callback);
		okay->select();
		okay->setTickCallback([](Widget& widget) {
			if (!main_menu_frame) {
				return;
			}
			auto selectedWidget = main_menu_frame->findSelectedWidget(widget.getOwner());
			if (!selectedWidget) {
				auto button = static_cast<Button*>(&widget);
				button->select();
			}
			});

		auto cancel = frame->addButton("cancel");
		cancel->setSize(SDL_Rect{leftRed ? 196 : 188, 78, rightRed ? 130 : 108, 52});
		cancel->setBackground(rightRed ?
		    "*images/ui/Main Menus/Disconnect/UI_Disconnect_Button_Abandon00.png" :
		    "*images/ui/Main Menus/Disconnect/UI_Disconnect_Button_GoBack00.png");
		cancel->setBackgroundHighlighted(rightRed ?
		    "*images/ui/Main Menus/Disconnect/UI_Disconnect_Button_AbandonHigh00.png" :
		    "*images/ui/Main Menus/Disconnect/UI_Disconnect_Button_GoBackHigh00.png");
		cancel->setBackgroundActivated(rightRed ?
		    "*images/ui/Main Menus/Disconnect/UI_Disconnect_Button_AbandonPress00.png" :
		    "*images/ui/Main Menus/Disconnect/UI_Disconnect_Button_GoBackPress00.png");
		cancel->setColor(makeColor(255, 255, 255, 255));
		cancel->setHighlightColor(makeColor(255, 255, 255, 255));
		cancel->setTextColor(makeColor(255, 255, 255, 255));
		cancel->setTextHighlightColor(makeColor(255, 255, 255, 255));
		cancel->setFont(smallfont_outline);
		cancel->setText(cancel_text);
		cancel->setWidgetSearchParent(okay->getParent()->getName());
		cancel->setWidgetLeft("okay");
		cancel->setWidgetBack("cancel");
		cancel->setCallback(cancel_callback);

		return frame;
	}

	static void closeBinary() {
	    closePrompt("binary_prompt");
	}

	static Frame* trinaryPrompt(
		const char* window_text,
		const char* option1_text,
		const char* option2_text,
		const char* option3_text,
		void (*option1_callback)(Button&),
		void (*option2_callback)(Button&),
		void (*option3_callback)(Button&)
	) {
		soundActivate();

	    Frame* frame = createPrompt("trinary_prompt", false);
	    if (!frame) {
	        return nullptr;
	    }

		auto text = frame->addField("text", 1024);
		text->setSize(SDL_Rect{30, 12, frame->getSize().w - 60, frame->getSize().h - 96});
		text->setFont(smallfont_outline);
		text->setText(window_text);
		text->setJustify(Field::justify_t::CENTER);

		struct Option {
		    const char* text;
		    void (*callback)(Button&);
		};
		Option options[] = {
		    {option1_text, option1_callback},
		    {option2_text, option2_callback},
		    {option3_text, option3_callback},
		};
		constexpr int num_options = sizeof(options) / sizeof(options[0]);

        int x = 0;
        const int offx = (frame->getSize().w - 112 - 160 - 112) / 2;
        const int offy = frame->getSize().h - 96;
        for (int c = 0; c < num_options; ++c) {
            const std::string name = std::string("option") + std::to_string(c + 1);
		    auto button = frame->addButton(name.c_str());
		    if (c == 1) {
		        button->setSize(SDL_Rect{offx + x, offy, 156, 52});
		        button->setBackground("*images/ui/Main Menus/Disconnect/UI_Disconnect_Button_GoBack01.png");
		        button->setBackgroundHighlighted("*images/ui/Main Menus/Disconnect/UI_Disconnect_Button_GoBackHigh01.png");
		        button->setBackgroundActivated("*images/ui/Main Menus/Disconnect/UI_Disconnect_Button_GoBackPress01.png");
		    } else {
		        button->setSize(SDL_Rect{offx + x, offy, 108, 52});
		        button->setBackground("*images/ui/Main Menus/Disconnect/UI_Disconnect_Button_GoBack00.png");
		        button->setBackgroundHighlighted("*images/ui/Main Menus/Disconnect/UI_Disconnect_Button_GoBackHigh00.png");
		        button->setBackgroundActivated("*images/ui/Main Menus/Disconnect/UI_Disconnect_Button_GoBackPress00.png");
		    }
		    button->setColor(makeColor(255, 255, 255, 255));
		    button->setHighlightColor(makeColor(255, 255, 255, 255));
		    button->setTextColor(makeColor(255, 255, 255, 255));
		    button->setTextHighlightColor(makeColor(255, 255, 255, 255));
		    button->setFont(smallfont_outline);
		    button->setText(options[c].text);
		    button->setWidgetSearchParent(frame->getName());
		    if (c < num_options - 1) {
                const std::string name = std::string("option") + std::to_string(c + 2);
		        button->setWidgetRight(name.c_str());
		    }
		    if (c > 0) {
                const std::string name = std::string("option") + std::to_string(c);
		        button->setWidgetLeft(name.c_str());
		    }
		    button->setWidgetBack("option3");
		    button->setCallback(options[c].callback);
		    x += button->getSize().w + 4;
		}

		auto selected = frame->findButton("option2");
		if (selected) {
		    selected->select();
			selected->setTickCallback([](Widget& widget){
				if (!main_menu_frame) {
					return;
				}
				auto selectedWidget = main_menu_frame->findSelectedWidget(widget.getOwner());
				if (!selectedWidget) {
					auto button = static_cast<Button*>(&widget);
					button->select();
				}
				});
		}

		return frame;
	}

	static void closeTrinary() {
	    closePrompt("trinary_prompt");
	}

	static Frame* cancellablePrompt(
	    const char* name,
		const char* window_text,
		const char* cancel_text,
	    void (*tick_callback)(Widget&),
		void (*cancel_callback)(Button&)
	) {
		soundActivate();

	    Frame* frame = createPrompt(name);
	    if (!frame) {
	        return nullptr;
	    }

		auto text = frame->addField("text", 128);
		text->setSize(SDL_Rect{30, 16, frame->getSize().w - 60, 64});
		text->setFont(smallfont_no_outline);
		text->setText(window_text);
		text->setJustify(Field::justify_t::CENTER);
		text->setHideSelectors(true);
		text->setHideGlyphs(true);
		text->setTickCallback(tick_callback);

		auto cancel = frame->addButton("cancel");
		cancel->setSize(SDL_Rect{(frame->getActualSize().w - 130) / 2, 82, 130, 52});
		cancel->setBackground("*images/ui/Main Menus/Disconnect/UI_Disconnect_Button_Abandon00.png");
		cancel->setBackgroundHighlighted("*images/ui/Main Menus/Disconnect/UI_Disconnect_Button_AbandonHigh00.png");
		cancel->setBackgroundActivated("*images/ui/Main Menus/Disconnect/UI_Disconnect_Button_AbandonPress00.png");
		cancel->setColor(makeColor(255, 255, 255, 255));
		cancel->setHighlightColor(makeColor(255, 255, 255, 255));
		cancel->setTextColor(makeColor(255, 255, 255, 255));
		cancel->setTextHighlightColor(makeColor(255, 255, 255, 255));
		cancel->setFont(smallfont_outline);
		cancel->setText(cancel_text);
		cancel->setCallback(cancel_callback);
		cancel->setWidgetBack("cancel");
		cancel->select();
		cancel->setTickCallback([](Widget& widget) {
			if (!main_menu_frame) {
				return;
			}
			auto selectedWidget = main_menu_frame->findSelectedWidget(widget.getOwner());
			if (!selectedWidget) {
				auto button = static_cast<Button*>(&widget);
				button->select();
			}
			});

		return frame;
	}

	static Frame* monoPromptGeneric(
	    const char* window_text,
	    const char* okay_text,
	    void (*okay_callback)(Button&)
	) {
		soundActivate();

	    Frame* frame = createPrompt("mono_prompt");
	    if (!frame) {
	        return nullptr;
	    }

		auto text = frame->addField("text", 128);
		text->setSize(SDL_Rect{30, 28, 304, 46});
		text->setFont(smallfont_no_outline);
		text->setText(window_text);
		text->setJustify(Field::justify_t::CENTER);

		auto okay = frame->addButton("okay");
		okay->setSize(SDL_Rect{(frame->getActualSize().w - 108) / 2, 78, 108, 52});
		okay->setBackground("*images/ui/Main Menus/Disconnect/UI_Disconnect_Button_GoBack00.png");
		okay->setBackgroundHighlighted("*images/ui/Main Menus/Disconnect/UI_Disconnect_Button_GoBackHigh00.png");
		okay->setBackgroundActivated("*images/ui/Main Menus/Disconnect/UI_Disconnect_Button_GoBackPress00.png");
		okay->setColor(makeColor(255, 255, 255, 255));
		okay->setHighlightColor(makeColor(255, 255, 255, 255));
		okay->setTextColor(makeColor(255, 255, 255, 255));
		okay->setTextHighlightColor(makeColor(255, 255, 255, 255));
		okay->setFont(smallfont_outline);
		okay->setText(okay_text);
		okay->setCallback(okay_callback);
		okay->select();
		okay->setTickCallback([](Widget& widget) {
			if (!main_menu_frame) {
				return;
			}
			auto selectedWidget = main_menu_frame->findSelectedWidget(widget.getOwner());
			if (!selectedWidget) {
				auto button = static_cast<Button*>(&widget);
				button->select();
			}
			});

		return frame;
	}

	static Frame* errorPrompt(
	    const char* window_text,
	    const char* okay_text,
	    void (*okay_callback)(Button&)
	) {
		soundError();
		return monoPromptGeneric(window_text, okay_text, okay_callback);
	}

	static Frame* monoPrompt(
	    const char* window_text,
	    const char* okay_text,
	    void (*okay_callback)(Button&)
	) {
		soundActivate();
		return monoPromptGeneric(window_text, okay_text, okay_callback);
	}

	static void closeMono() {
	    closePrompt("mono_prompt");
	}

	static Frame* textPrompt(
	    const char* name,
	    const char* window_text,
	    void (*tick_callback)(Widget&),
	    bool issmall = true
	    ) {
		soundActivate();

	    Frame* frame = createPrompt(name, issmall);
	    if (!frame) {
	        return nullptr;
	    }

		auto text = frame->addField("text", 128);
		text->setSize(SDL_Rect{30, 12, frame->getSize().w - 60, frame->getSize().h - 34});
		text->setFont(smallfont_no_outline);
		text->setText(window_text);
		text->setJustify(Field::justify_t::CENTER);
		text->setHideSelectors(true);
		text->setHideGlyphs(true);
		text->setTickCallback(tick_callback);
		text->select();

		return frame;
	}

	static void closeText() {
	    closePrompt("text_prompt");
	}

    static void connectionErrorPrompt(const char* str) {
        resetLobbyJoinFlowState();
        errorPrompt(str, "Okay",
            [](Button& button) {
            soundCancel();
            multiplayer = SINGLE;
            closeMono();
            });
    };

	static void systemErrorPrompt(const char* str) {
#ifdef NINTENDO
		char buf[1024];
		snprintf(buf, sizeof(buf), "%s\n\nPlease try again later.", str);
		nxErrorPrompt(str, buf, 22222);
#else
		connectionErrorPrompt(str);
#endif
	}

    static void disconnectPrompt(const char* text) {
        errorPrompt(
            text,
            "Okay",
            [](Button& button){
                soundCancel();
                beginFade(FadeDestination::RootMainMenu);
                closeMono();
            }
        );
    }

    static void openDLCPrompt(int which) {
		static int dlcPromptIndex;
		dlcPromptIndex = which;
#if defined(NINTENDO) || defined(STEAMWORKS) || defined(USE_EOS)
#ifdef NINTENDO
		const char* window_text = "Would you like to browse this\nDLC in the Nintendo eShop?";
#else
		const char* window_text = "Would you like to browse this\nDLC in the online store?";
#endif
		binaryPrompt(window_text, "Yes", "No",
			[](Button& button){
#if defined(STEAMWORKS)
				soundActivate();
				openURLTryWithOverlay("https://store.steampowered.com/dlc/371970/Barony/");
#elif defined(NINTENDO)
                nxShowAllDLC();
                /*if (nxShowDLCPage(dlcPromptIndex) == false) {
                    soundError();
                }*/
#elif defined(USE_EOS)
				soundActivate();
				openURLTryWithOverlay("https://store.epicgames.com/en-US/all-dlc/barony");
#endif
				// fixes a bug where you could get spammed with 100s of browser tabs...
				mousestatus[SDL_BUTTON_LEFT] = 0;
				Input::mouseButtons[SDL_BUTTON_LEFT] = 0;
				closeBinary();
			},
			[](Button& button){
				soundCancel();
				closeBinary();
			}, false);
#else
        textFieldPrompt("", "Enter DLC Key...", "Enter DLC Serial Key", "Confirm", "Cancel",
            [](Button& button){ // okay
                soundActivate();

                static std::string text;

                auto frame = static_cast<Frame*>(button.getParent()); assert(frame);
                auto field = frame->findField("field"); assert(field);
                text = field->getText();
                closeTextField();

                if (text.empty()) {
                    auto buttons = main_menu_frame->findFrame("buttons"); assert(buttons);
                    auto play = buttons->findButton("Play Game");
                    play->select();
                } else {
                    static Uint32 window_ticks;
                    window_ticks = ticks;
                    textPrompt("dlc_check_window", "", [](Widget& widget){
                        auto field = static_cast<Field*>(&widget);
                        auto time = ticks - window_ticks;
                        if (time % TICKS_PER_SECOND < 10) {
                            field->setText("Verifying");
                        }
                        else if (time % TICKS_PER_SECOND < 20) {
                            field->setText("Verifying.");
                        }
                        else if (time % TICKS_PER_SECOND < 30) {
                            field->setText("Verifying..");
                        }
                        else if (time % TICKS_PER_SECOND < 40) {
                            field->setText("Verifying...");
                        }
                        else {
                            field->setText("Verifying....");
                        }
                        if (time > TICKS_PER_SECOND * 2) {
                            closePrompt("dlc_check_window");
		                    std::size_t DLCHash = serialHash(text);

		                    auto prompt = [](const char* text){
		                        monoPrompt(text, "Okay", [](Button&){
		                            soundActivate();
		                            closeMono();
                                    auto buttons = main_menu_frame->findFrame("buttons"); assert(buttons);
                                    auto play = buttons->findButton("Play Game");
                                    play->select();
		                            });
		                        };

		                    if (DLCHash == 144425) {
			                    playSound(402, 92);
			                    printlog("[LICENSE]: Myths and Outcasts DLC license key found.");
			                    prompt("Myths and Outcasts DLC\nhas been unlocked!");
			                    enabledDLCPack1 = true;

                                char path[PATH_MAX] = "";
                                completePath(path, "mythsandoutcasts.key", outputdir);

                                // write the serial file
                                File* fp = nullptr;
                                if (fp = FileIO::open(path, "wb")) {
                                    fp->write(text.c_str(), sizeof(char), text.size());
                                    FileIO::close(fp);
                                }
		                    } else if ( DLCHash == 135398 ) {
			                    playSound(402, 92);
			                    printlog("[LICENSE]: Legends and Pariahs DLC license key found.");
			                    prompt("Legends and Pariahs DLC\nhas been unlocked!");
			                    enabledDLCPack2 = true;

                                char path[PATH_MAX] = "";
                                completePath(path, "legendsandpariahs.key", outputdir);

                                // write the serial file
                                File* fp = nullptr;
                                if (fp = FileIO::open(path, "wb")) {
                                    fp->write(text.c_str(), sizeof(char), text.size());
                                    FileIO::close(fp);
                                }
		                    } else {
			                    printlog("[LICENSE]: DLC license key invalid.");
		                        errorPrompt("Invalid license key", "Okay", [](Button&){
		                            soundActivate();
		                            closeMono();
                                    auto buttons = main_menu_frame->findFrame("buttons"); assert(buttons);
                                    auto play = buttons->findButton("Play Game");
                                    play->select();
		                            });
		                    }
                        }
                        });
                }
            },
            [](Button&){ // cancel
                soundCancel();
                closeTextField();
            });
#endif
    }

	static Frame* genericWindow(const char* name, const char* title, bool decorations) {
		auto dimmer = main_menu_frame->addFrame("dimmer");
		dimmer->setSize(SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY});
		dimmer->setActualSize(dimmer->getSize());
		dimmer->setColor(makeColor(0, 0, 0, 63));
		dimmer->setBorder(0);

		auto window = dimmer->addFrame(name);
		window->setSize(SDL_Rect{
			(Frame::virtualScreenX - 826) / 2,
			(Frame::virtualScreenY - 718) / 2,
			826,
			718});
		window->setActualSize(SDL_Rect{0, 0, 826, 718});
		window->setBorder(0);
		window->setColor(0);

		auto tooltip = window->addField("tooltip", 256);
		tooltip->setSize(SDL_Rect{66, 576, 646, 40});
		tooltip->setFont(smallfont_no_outline);
		tooltip->setJustify(Field::justify_t::CENTER);
		tooltip->setText("");

		auto background = window->addImage(
			window->getActualSize(),
			0xffffffff,
			decorations ?
				"*images/ui/Main Menus/Settings/GenericWindow/UI_MM14_Window00.png":
				"*images/ui/Main Menus/Settings/GenericWindow/UI_MM14_Window02.png",
			"background"
		);

		auto timber = window->addImage(
			SDL_Rect{0, 54, 826, 78},
			0xffffffff,
			"*images/ui/Main Menus/Settings/GenericWindow/UI_MM14_Window01.png",
			"timber"
		);
		timber->ontop = true;

		auto banner = window->addField("title", 64);
		banner->setSize(SDL_Rect{246, 22, 338, 24});
		banner->setFont(banner_font);
		banner->setText(title);
		banner->setJustify(Field::justify_t::CENTER);

		auto subwindow = window->addFrame("subwindow");
		subwindow->setSize(SDL_Rect{30, 64, 766, 506});
		subwindow->setActualSize(SDL_Rect{0, 0, 766, 506});
		subwindow->setScrollBarsEnabled(false);
		subwindow->setBorder(0);
		subwindow->setColor(0);

		auto rocks = subwindow->addImage(
			subwindow->getActualSize(),
			makeColor(255, 255, 255, 255),
			"*images/ui/Main Menus/Settings/GenericWindow/UI_MM14_Rocks01.png",
			"background"
		);
		rocks->tiled = true;

		auto gradient_background = subwindow->addImage(
			subwindow->getActualSize(),
			makeColor(255, 255, 255, 255),
			"#images/ui/Main Menus/Settings/Settings_Window_06_BGGradient.png",
			"gradient_background"
		);

		auto slider = subwindow->addSlider("scroll_slider");
		slider->setBorder(48);
		slider->setOrientation(Slider::SLIDER_VERTICAL);
		slider->setRailSize(SDL_Rect{712, 0, 54, 536});
		slider->setRailImage("*images/ui/Main Menus/Settings/GenericWindow/UI_MM14_ScrollBar01.png");
		slider->setHandleSize(SDL_Rect{0, 0, 34, 34});
		slider->setHandleImage("*images/ui/Main Menus/Settings/GenericWindow/UI_MM14_ScrollBoulder00.png");
		slider->setGlyphPosition(Button::glyph_position_t::CENTERED);
		slider->setCallback([](Slider& slider){
			Frame* frame = static_cast<Frame*>(slider.getParent());
			auto actualSize = frame->getActualSize();
			actualSize.y = slider.getValue();
			frame->setActualSize(actualSize);
			auto railSize = slider.getRailSize();
			railSize.y = actualSize.y;
			slider.setRailSize(railSize);
			slider.updateHandlePosition();
			auto gradient_background = frame->findImage("gradient_background");
			assert(gradient_background);
			gradient_background->pos.y = actualSize.y;
			});
		slider->setTickCallback([](Widget& widget){
			Slider* slider = static_cast<Slider*>(&widget);
			Frame* frame = static_cast<Frame*>(slider->getParent());
			auto actualSize = frame->getActualSize();
			slider->setValue(actualSize.y);
			auto railSize = slider->getRailSize();
			railSize.y = actualSize.y;
			slider->setRailSize(railSize);
			slider->updateHandlePosition();
			auto gradient_background = frame->findImage("gradient_background");
			assert(gradient_background);
			gradient_background->pos.y = actualSize.y;
			});
		slider->setWidgetSearchParent(name);
		slider->setWidgetBack("discard_and_exit");
		slider->addWidgetAction("MenuStart", "confirm_and_exit");
		slider->addWidgetAction("MenuAlt1", "restore_defaults");

		auto sliderLeft = subwindow->addImage(
			SDL_Rect{0, 0, 30, 44},
			0xffffffff,
			"*images/ui/Main Menus/Settings/AutoSort/AutoSort_SliderBox_Left00.png",
			"slider_left"
		);
		sliderLeft->disabled = true;
		sliderLeft->ontop = true;

		auto sliderRight = subwindow->addImage(
			SDL_Rect{0, 0, 30, 44},
			0xffffffff,
			"*images/ui/Main Menus/Settings/AutoSort/AutoSort_SliderBox_Right00.png",
			"slider_right"
		);
		sliderRight->disabled = true;
		sliderRight->ontop = true;

		return window;
	}

/******************************************************************************/

	static bool isConnectedToEpic() {
#ifdef USE_EOS
		return EOS.isInitialized() && EOS.CurrentUserInfo.isLoggedIn() && EOS.CurrentUserInfo.isValid();
#else
		return false;
#endif
	}

	void logoutOfEpic() {
#ifdef USE_EOS
#if defined(NINTENDO)
		EOS.stop();
		nxDisconnectFromNetwork();
#else
		LobbyHandler.crossplayEnabled = false;
		EOS.CrossplayAccountManager.logOut = true;
#endif
#endif
	}

	typedef void (*LoginCallback)(bool);
	static void loginToEpic(LoginCallback callback) {
		static LoginCallback cb;
		cb = callback;
#ifndef USE_EOS
		if (cb) {
			cb(false);
		}
#else
#ifdef NINTENDO
		if (isConnectedToEpic()) {
			if (cb) {
				cb(true);
			}
		} else {
			static bool attemptedConnection;
			attemptedConnection = false;
			nxShutdownWireless();
			nxConnectToNetwork();
			cancellablePrompt("connect_eos_prompt", "Connecting\n...", "Cancel",
				[](Widget& widget) {
				const char* str;
				auto part = ticks % TICKS_PER_SECOND;
				auto text = static_cast<Field*>(&widget);
				if (part < TICKS_PER_SECOND / 5) {
					str = "Connecting\n.";
				} else if (part < 2 * TICKS_PER_SECOND / 5) {
					str = "Connecting\n..";
				} else if (part < 3 * TICKS_PER_SECOND / 5) {
					str = "Connecting\n...";
				} else if (part < 4 * TICKS_PER_SECOND / 5) {
					str = "Connecting\n....";
				} else {
					str = "Connecting\n.....";
				}
				text->setText(str);

				if (nxConnectingToNetwork()) {
					// wait for NX connection to finish
					return;
				}
				else {
					if (nxConnectedToNetwork()) {
						if (isConnectedToEpic()) {
							printlog("[NX] successfully logged into EOS");
							closePrompt("connect_eos_prompt");
							if (cb) {
								cb(true);
							}
						}
						else {
							if (EOS.CrossplayAccountManager.isLoggingIn()) {
								// wait for EOS login to finish
								return;
							} else {
								if (!attemptedConnection) {
									attemptedConnection = true;
									EOS.initPlatform(true);
									EOS.SetNetworkAvailable(true);
									EOS.CrossplayAccountManager.trySetupFromSettingsMenu = true;
									EOS.StatGlobalManager.queryGlobalStatUser();
									printlog("[NX] logging into EOS");
								} else {
									logoutOfEpic();
									printlog("[NX] EOS login failed");
									closePrompt("connect_eos_prompt");
									if (cb) {
										cb(false);
									}
								}
							}
						}
					}
					else {
						if (nxDisplayNetworkError()) {
							printlog("[NX] Displaying network error");
							nxConnectToNetwork();
						}
						else {
							logoutOfEpic();
							printlog("[NX] failed to establish network connection, EOS connection failed");
							closePrompt("connect_eos_prompt");
							if (cb) {
								cb(false);
							}
						}
					}
				}},
				[](Button&){ // cancel
					logoutOfEpic();
					closePrompt("connect_eos_prompt");
					if (cb) {
						cb(false);
					}
				});
		}
#else // NINTENDO
		if (!isConnectedToEpic()) {
			EOS.CrossplayAccountManager.trySetupFromSettingsMenu = true;
			EOS.StatGlobalManager.queryGlobalStatUser();
		}
#endif // !NINTENDO
#endif // USE_EOS
	}

/******************************************************************************/

	inline void InventorySorting::save() {
		auto_hotbar_categories[0] = hotbarWeapons;
		auto_hotbar_categories[1] = hotbarArmor;
		auto_hotbar_categories[2] = hotbarAmulets;
		auto_hotbar_categories[3] = hotbarBooks;
		auto_hotbar_categories[4] = hotbarTools;
		auto_hotbar_categories[5] = hotbarThrown;
		auto_hotbar_categories[6] = hotbarGems;
		auto_hotbar_categories[7] = hotbarPotions;
		auto_hotbar_categories[8] = hotbarScrolls;
		auto_hotbar_categories[9] = hotbarStaves;
		auto_hotbar_categories[10] = hotbarFood;
		auto_hotbar_categories[11] = hotbarSpells;
		autosort_inventory_categories[0] = sortWeapons;
		autosort_inventory_categories[1] = sortArmor;
		autosort_inventory_categories[2] = sortAmulets;
		autosort_inventory_categories[3] = sortBooks;
		autosort_inventory_categories[4] = sortTools;
		autosort_inventory_categories[5] = sortThrown;
		autosort_inventory_categories[6] = sortGems;
		autosort_inventory_categories[7] = sortPotions;
		autosort_inventory_categories[8] = sortScrolls;
		autosort_inventory_categories[9] = sortStaves;
		autosort_inventory_categories[10] = sortFood;
		autosort_inventory_categories[11] = sortEquipped;
	}

	inline InventorySorting InventorySorting::load() {
		InventorySorting inventory_sorting;
		inventory_sorting.hotbarWeapons = auto_hotbar_categories[0];
		inventory_sorting.hotbarArmor = auto_hotbar_categories[1];
		inventory_sorting.hotbarAmulets = auto_hotbar_categories[2];
		inventory_sorting.hotbarBooks = auto_hotbar_categories[3];
		inventory_sorting.hotbarTools = auto_hotbar_categories[4];
		inventory_sorting.hotbarThrown = auto_hotbar_categories[5];
		inventory_sorting.hotbarGems = auto_hotbar_categories[6];
		inventory_sorting.hotbarPotions = auto_hotbar_categories[7];
		inventory_sorting.hotbarScrolls = auto_hotbar_categories[8];
		inventory_sorting.hotbarStaves = auto_hotbar_categories[9];
		inventory_sorting.hotbarFood = auto_hotbar_categories[10];
		inventory_sorting.hotbarSpells = auto_hotbar_categories[11];
		inventory_sorting.sortWeapons = autosort_inventory_categories[0];
		inventory_sorting.sortArmor = autosort_inventory_categories[1];
		inventory_sorting.sortAmulets = autosort_inventory_categories[2];
		inventory_sorting.sortBooks = autosort_inventory_categories[3];
		inventory_sorting.sortTools = autosort_inventory_categories[4];
		inventory_sorting.sortThrown = autosort_inventory_categories[5];
		inventory_sorting.sortGems = autosort_inventory_categories[6];
		inventory_sorting.sortPotions = autosort_inventory_categories[7];
		inventory_sorting.sortScrolls = autosort_inventory_categories[8];
		inventory_sorting.sortStaves = autosort_inventory_categories[9];
		inventory_sorting.sortFood = autosort_inventory_categories[10];
		inventory_sorting.sortEquipped = autosort_inventory_categories[11];
		return inventory_sorting;
	}

	inline InventorySorting InventorySorting::reset() {
		return InventorySorting();
	}

	bool InventorySorting::serialize(FileInterface* file) {
	    int version = 0;
	    file->property("version", version);
		file->property("hotbarWeapons", hotbarWeapons);
		file->property("hotbarArmor", hotbarArmor);
		file->property("hotbarAmulets", hotbarAmulets);
		file->property("hotbarBooks", hotbarBooks);
		file->property("hotbarTools", hotbarTools);
		file->property("hotbarThrown", hotbarThrown);
		file->property("hotbarGems", hotbarGems);
		file->property("hotbarPotions", hotbarPotions);
		file->property("hotbarScrolls", hotbarScrolls);
		file->property("hotbarStaves", hotbarStaves);
		file->property("hotbarFood", hotbarFood);
		file->property("hotbarSpells", hotbarSpells);
		file->property("sortWeapons", sortWeapons);
		file->property("sortArmor", sortArmor);
		file->property("sortAmulets", sortAmulets);
		file->property("sortBooks", sortBooks);
		file->property("sortTools", sortTools);
		file->property("sortThrown", sortThrown);
		file->property("sortGems", sortGems);
		file->property("sortPotions", sortPotions);
		file->property("sortScrolls", sortScrolls);
		file->property("sortStaves", sortStaves);
		file->property("sortFood", sortFood);
		file->property("sortEquipped", sortEquipped);
		return true;
	}

	/******************************************************************************/

	inline void LastCreatedCharacter::save() {
		for ( int i = 0; i < NUM_LAST_CHARACTERS; ++i )
		{
			LastCreatedCharacterSettings.characterClass[i] = characterClass[i];
			LastCreatedCharacterSettings.characterSex[i] = characterSex[i];
			LastCreatedCharacterSettings.characterRace[i] = characterRace[i];
			LastCreatedCharacterSettings.characterAppearance[i] = characterAppearance[i];
			LastCreatedCharacterSettings.characterName[i] = characterName[i];
		}
	}

	inline LastCreatedCharacter LastCreatedCharacter::load() {
		LastCreatedCharacter lastCharacter;
		for ( int i = 0; i < NUM_LAST_CHARACTERS; ++i )
		{
			lastCharacter.characterClass[i] = LastCreatedCharacterSettings.characterClass[i];
			lastCharacter.characterSex[i] = LastCreatedCharacterSettings.characterSex[i];
			lastCharacter.characterRace[i] = LastCreatedCharacterSettings.characterRace[i];
			lastCharacter.characterAppearance[i] = LastCreatedCharacterSettings.characterAppearance[i];
			lastCharacter.characterName[i] = LastCreatedCharacterSettings.characterName[i];
		}
		return lastCharacter;
	}

	inline LastCreatedCharacter LastCreatedCharacter::reset() {
		return LastCreatedCharacter();
	}

	bool LastCreatedCharacter::serialize(FileInterface* file) {
		int version = 0;
		file->property("version", version);

		file->propertyName("players");
		Uint32 sizeArray = NUM_LAST_CHARACTERS;
		file->beginArray(sizeArray);
		for ( int c = 0; c < NUM_LAST_CHARACTERS; ++c ) {
			file->beginObject();
			file->property("class", characterClass[c]);
			file->property("sex", characterSex[c]);
			file->property("race", characterRace[c]);
			file->property("appearance", characterAppearance[c]);
			file->property("name", characterName[c]);
			file->endObject();
		}
		file->endArray();
		return true;
	}

/******************************************************************************/

	static Bindings old_bindings;

	inline void Bindings::save() {
		for (int c = 0; c < MAX_SPLITSCREEN; ++c) {
		    Input& input = Input::inputs[c];
			input.getKeyboardBindings().clear();
			input.setKeyboardBindings(kb_mouse_bindings[c]);
			input.getGamepadBindings().clear();
			input.setGamepadBindings(gamepad_bindings[c]);
			input.getJoystickBindings().clear();
			input.setJoystickBindings(joystick_bindings[c]);

			input.refresh();

			// scan for any held buttons and make sure we re-consume them to not double press anything
			input.update();
			for ( auto& b : input.getBindings() )
			{
				input.consumeBinaryToggle(b.first.c_str());
			}
		}
		old_bindings = *this;
	}

	inline Bindings Bindings::load() {
		return old_bindings;
	}

	inline Bindings Bindings::reset(const char* profile) {
		Bindings bindings;
		for (int c = 0; c < MAX_SPLITSCREEN; ++c) {
            for (auto& binding : getBindings(profile)) {
			    bindings.kb_mouse_bindings[c].emplace(binding.action, binding.keyboard);
			    bindings.gamepad_bindings[c].emplace(binding.action, binding.gamepad);
			    bindings.joystick_bindings[c].emplace(binding.action, binding.joystick);
			}
		}
		return bindings;
	}

	bool Bindings::serialize(FileInterface* file) {
	    int version = 0;
	    file->property("version", version);
		Uint32 num_players = MAX_SPLITSCREEN;
		file->propertyName("players");
		file->beginArray(num_players);
		for (int c = 0; c < std::min(num_players, (Uint32)MAX_SPLITSCREEN); ++c) {
			file->beginObject();
			for (int j = 0; j < 3; ++j) {
				auto& bindings =
					j == 0 ? kb_mouse_bindings[c]:
					j == 1 ? gamepad_bindings[c]:
					joystick_bindings[c];
				file->propertyName(
					j == 0 ? "kb_mouse_bindings":
					j == 1 ? "gamepad_bindings":
					"joystick_bindings");
				if (file->isReading()) {
					bindings.clear();
				}
				Uint32 count = (Uint32)bindings.size();
				file->beginArray(count);
				if (file->isReading()) {
                    for (auto& binding : getBindings(defaultControlLayout)) {
                        switch (j) {
                        case 0: bindings[binding.action] = binding.keyboard; break;
                        case 1: bindings[binding.action] = binding.gamepad; break;
                        case 2: bindings[binding.action] = binding.joystick; break;
                        default: break;
                        }
                    }
					for (Uint32 index = 0; index < count; ++index) {
						file->beginObject();
						std::string binding;
						file->property("binding", binding);
						std::string input;
						file->property("input", input);
						bindings[binding] = input;
						file->endObject();
					}
				} else {
					for (auto& bind : bindings) {
						file->beginObject();
						std::string binding = bind.first;
						file->property("binding", binding);
						std::string input = bind.second;
						file->property("input", input);
						file->endObject();
					}
				}
				file->endArray();
			}
			file->endObject();
		}
		file->endArray();
		return true;
	}

/******************************************************************************/

	inline void Minimap::save() {
		minimapTransparencyForeground = 100 - foreground_opacity;
		minimapTransparencyBackground = 100 - background_opacity;
		minimapScale = map_scale;
		minimapObjectZoom = std::max(100, icon_scale);
	}

	inline Minimap Minimap::load() {
		Minimap minimap;
		minimap.foreground_opacity = 100 - minimapTransparencyForeground;
		minimap.background_opacity = 100 - minimapTransparencyBackground;
		minimap.map_scale = minimapScale;
		minimap.icon_scale = std::max(100, minimapObjectZoom);
		return minimap;
	}

	inline Minimap Minimap::reset() {
		return Minimap();
	}

	bool Minimap::serialize(FileInterface* file) {
	    int version = 0;
	    file->property("version", version);
		file->property("map_scale", map_scale);
		file->property("icon_scale", icon_scale);
		file->property("foreground_opacity", foreground_opacity);
		file->property("background_opacity", background_opacity);
		return true;
	}

/******************************************************************************/

	inline void Messages::save() {
		messagesEnabled = 0;
		messagesEnabled |= combat ? MESSAGE_COMBAT : 0;
		messagesEnabled |= status ? MESSAGE_STATUS : 0;
		messagesEnabled |= inventory ? MESSAGE_INVENTORY : 0;
		messagesEnabled |= equipment ? MESSAGE_EQUIPMENT : 0;
		messagesEnabled |= world ? MESSAGE_WORLD : 0;
		messagesEnabled |= chat ? MESSAGE_CHAT : 0;
		messagesEnabled |= progression ? MESSAGE_PROGRESSION : 0;
		messagesEnabled |= interaction ? MESSAGE_INTERACTION : 0;
		messagesEnabled |= inspection ? MESSAGE_INSPECTION : 0;
		messagesEnabled |= hint ? MESSAGE_HINT : 0;
		messagesEnabled |= obituary ? MESSAGE_OBITUARY : 0;
		messagesEnabled |= MESSAGE_MISC;
#ifndef NDEBUG
        messagesEnabled |= MESSAGE_DEBUG;
#endif
	}

	inline Messages Messages::load() {
		Messages messages;
		messages.combat = messagesEnabled & MESSAGE_COMBAT;
		messages.status = messagesEnabled & MESSAGE_STATUS;
		messages.inventory = messagesEnabled & MESSAGE_INVENTORY;
		messages.equipment = messagesEnabled & MESSAGE_EQUIPMENT;
		messages.world = messagesEnabled & MESSAGE_WORLD;
		messages.chat = messagesEnabled & MESSAGE_CHAT;
		messages.progression = messagesEnabled & MESSAGE_PROGRESSION;
		messages.interaction = messagesEnabled & MESSAGE_INTERACTION;
		messages.inspection = messagesEnabled & MESSAGE_INSPECTION;
		messages.hint = messagesEnabled & MESSAGE_HINT;
		messages.obituary = messagesEnabled & MESSAGE_OBITUARY;
		return messages;
	}

	inline Messages Messages::reset() {
		return Messages();
	}

	bool Messages::serialize(FileInterface* file) {
	    int version = 0;
	    file->property("version", version);
		file->property("combat", combat);
		file->property("status", status);
		file->property("inventory", inventory);
		file->property("equipment", equipment);
		file->property("world", world);
		file->property("chat", chat);
		file->property("progression", progression);
		file->property("interaction", interaction);
		file->property("inspection", inspection);
		file->property("hint", hint);
		file->property("obituary", obituary);
		return true;
	}

	/******************************************************************************/

	inline bool Video::save() {
	    bool result = false;

		bool new_fullscreen, new_borderless;
		switch (window_mode) {
		case 0: // windowed
			new_fullscreen = false;
			new_borderless = false;
			break;
		case 1: // borderless
			new_fullscreen = false;
			new_borderless = true;
			break;
		case 2: // fullscreen
			new_fullscreen = true;
			new_borderless = false;
			break;
		default:
			assert("Unknown video mode" && 0);
			break;
		}
		if (xres != resolution_x ||
		    yres != resolution_y ||
		    ::display_id != display_id ||
		    verticalSync != vsync_enabled ||
		    new_fullscreen != fullscreen ||
		    new_borderless != borderless) {
		    result = true;
		}

#if defined(VIDEO_RESTART_NEEDED)
		if (!initialized)
#endif
		{
			fullscreen = new_fullscreen;
			borderless = new_borderless;
			::display_id = display_id;
			xres = std::max(resolution_x, 1024);
			yres = std::max(resolution_y, 720);
			verticalSync = vsync_enabled;
		}

		*cvar_displayHz = hz;
		vidgamma = std::min(std::max(.5f, gamma / 100.f), 2.f);

		return result;
	}

	inline struct Video Video::load() {
	    Video settings;
		settings.window_mode = fullscreen ? 2 : (borderless ? 1 : 0);
		settings.display_id = ::display_id;
		settings.resolution_x = xres;
		settings.resolution_y = yres;
        settings.hz = *cvar_displayHz;
		settings.vsync_enabled = verticalSync;
		settings.gamma = vidgamma * 100.f;
		return settings;
	}

	inline struct Video Video::reset() {
	    return Video();
	}

	bool Video::serialize(FileInterface* file) {
	    int version = 1;
	    file->property("version", version);
	    file->property("window_mode", window_mode);
	    file->property("display_id", display_id);
        file->propertyVersion("hz", version >= 1, hz);
	    file->property("resolution_x", resolution_x);
	    file->property("resolution_y", resolution_y);
	    file->property("vsync_enabled", vsync_enabled);
	    file->property("gamma", gamma);
		return true;
	}

	/******************************************************************************/

	static AllSettings allSettings;

	inline int AllSettings::save() {
        int result = VideoRefresh::None;
        gamemods_mountedFilepaths = mods;
		*cvar_fastRestart = fast_restart;
		*cvar_worldtooltip_scale = world_tooltip_scale;
		*cvar_worldtooltip_scale_splitscreen = world_tooltip_scale_splitscreen;
		*cvar_enemybar_scale = enemybar_scale;
		auto_hotbar_new_items = add_items_to_hotbar_enabled;
		inventory_sorting.save();
		lastCharacter.save();
		right_click_protect = !use_on_release_enabled;
		minimap.save();
        const bool oldUIFilter = *ui_filter;
        const float oldUIScale = uiScale;
        *ui_filter = ui_filter_enabled;
		Player::WorldUI_t::tooltipHeightOffsetZ = (6 * (100 - item_tooltip_height)) / 100.f;
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			playerSettings[i].shootmodeCrosshair = shootmode_crosshair;
			playerSettings[i].shootmodeCrosshairOpacity = shootmode_crosshair_opacity;
		}
		*cvar_hdrEnabled = true;// hdr_enabled;
        uiScale = ui_scale / 100.f;
        result |= (oldUIFilter != *ui_filter || oldUIScale != uiScale) ?
            VideoRefresh::General : VideoRefresh::None;
		disable_messages = !show_messages_enabled;
		show_messages.save();
		hide_playertags = !show_player_nametags_enabled;
		nohud = !show_hud_enabled;
		broadcast = !show_ip_address_enabled;
		spawn_blood = !content_control_enabled;
		colorblind = colorblind_mode_enabled;
		arachnophobia_filter = arachnophobia_filter_enabled;
		shaking = shaking_enabled;
		bobbing = bobbing_enabled;
		flickerLights = light_flicker_enabled;
		result |= video.save() ? VideoRefresh::Video : VideoRefresh::None;
		*vertical_splitscreen = vertical_split_enabled;
		*staggered_splitscreen = staggered_split_enabled;
		*clipped_splitscreen = clipped_split_enabled;
		TimerExperiments::bUseTimerInterpolation = use_frame_interpolation;
		::fov = std::min(std::max(40.f, fov), 100.f);
        *cvar_desiredFps = (int)fps;
        if (*cvar_desiredFps == AUTO_FPS) {
            if (*cvar_displayHz) {
                fpsLimit = std::min(std::max(MIN_FPS, *cvar_displayHz), MAX_FPS);
            } else {
                SDL_DisplayMode mode;
                int result = SDL_GetCurrentDisplayMode(::display_id, &mode);
                if (!result && mode.refresh_rate) {
                    fpsLimit = std::min(std::max(MIN_FPS, mode.refresh_rate), MAX_FPS);
                } else {
                    if (result) {
                        printlog(SDL_GetError());
                    }
                    printlog("note: unknown display refresh rate, defaulting to 60hz");
                    fpsLimit = 60;
                }
            }
        } else {
            fpsLimit = std::min(std::max(MIN_FPS, *cvar_desiredFps), MAX_FPS);
        }
		current_audio_device = audio_device;
		MainMenu::master_volume = std::min(std::max(0.f, master_volume / 100.f), 1.f);
		sfxvolume = std::min(std::max(0.f, gameplay_volume / 100.f), 1.f);
		sfxAmbientVolume = std::min(std::max(0.f, ambient_volume / 100.f), 1.f);
		sfxEnvironmentVolume = std::min(std::max(0.f, environment_volume / 100.f), 1.f);
		sfxNotificationVolume = std::min(std::max(0.f, notification_volume / 100.f), 1.f);
		musvolume = std::min(std::max(0.f, music_volume / 100.f), 1.f);
		minimapPingMute = !minimap_pings_enabled;
		mute_player_monster_sounds = !player_monster_sounds_enabled;
		mute_audio_on_focus_lost = !out_of_focus_audio_enabled;
		bindings.save();
		*cvar_mkb_world_tooltips = mkb_world_tooltips_enabled;
		*cvar_mkb_facehotbar = false; //mkb_facehotbar;
		*cvar_gamepad_facehotbar = gamepad_facehotbar;
		hotbar_numkey_quick_add = numkeys_in_inventory_enabled;
		mousespeed = std::min(std::max(0.f, mouse_sensitivity), 100.f);
		reversemouse = reverse_mouse_enabled;
		smoothmouse = smooth_mouse_enabled;
		disablemouserotationlimit = true; //!rotation_speed_limit_enabled;
		gamepad_rightx_sensitivity = std::min(std::max(25.f / 32768.f, turn_sensitivity_x / 32768.f), 200.f / 32768.f);
		gamepad_righty_sensitivity = std::min(std::max(25.f / 32768.f, turn_sensitivity_y / 32768.f), 200.f / 32768.f);
		gamepad_rightx_invert = gamepad_camera_invert_x;
		gamepad_righty_invert = gamepad_camera_invert_y;
		if (multiplayer != CLIENT) {
		    svFlags = classic_mode_enabled ? svFlags | SV_FLAG_CLASSIC : svFlags & ~(SV_FLAG_CLASSIC);
		    svFlags = hardcore_mode_enabled ? svFlags | SV_FLAG_HARDCORE : svFlags & ~(SV_FLAG_HARDCORE);
		    svFlags = friendly_fire_enabled ? svFlags | SV_FLAG_FRIENDLYFIRE : svFlags & ~(SV_FLAG_FRIENDLYFIRE);
		    svFlags = keep_inventory_enabled ? svFlags | SV_FLAG_KEEPINVENTORY : svFlags & ~(SV_FLAG_KEEPINVENTORY);
		    svFlags = hunger_enabled ? svFlags | SV_FLAG_HUNGER : svFlags & ~(SV_FLAG_HUNGER);
		    svFlags = minotaur_enabled ? svFlags | SV_FLAG_MINOTAURS : svFlags & ~(SV_FLAG_MINOTAURS);
		    svFlags = random_traps_enabled ? svFlags | SV_FLAG_TRAPS : svFlags & ~(SV_FLAG_TRAPS);
		    svFlags = extra_life_enabled ? svFlags | SV_FLAG_LIFESAVING : svFlags & ~(SV_FLAG_LIFESAVING);
		    svFlags = cheats_enabled ? svFlags | SV_FLAG_CHEATS : svFlags & ~(SV_FLAG_CHEATS);
		}
	    sendSvFlagsOverNet();
		::skipintro = skipintro;
		::portnumber = (Uint16)port_number ? (Uint16)port_number : DEFAULT_PORT;
		hidden_roomcode = !show_lobby_code;
		for ( int i = 0; i < MAX_LOBBY_FILTERS_SAVED; ++i )
		{
			if ( i < numFilters )
			{
				lobbyFilters[i] = (MainMenu::Filter)lobby_filter_settings[i];
			}
		}

#if defined(USE_EOS) && defined(STEAMWORKS)
	    if ( crossplay_enabled && !LobbyHandler.crossplayEnabled )
	    {
		    crossplay_enabled = false;
			loginToEpic(nullptr);
	    }
	    else if ( !crossplay_enabled && LobbyHandler.crossplayEnabled )
	    {
			logoutOfEpic();
	    }
#endif

	    return result;
    }

	inline AllSettings AllSettings::load(bool video) {
		AllSettings settings;
		settings.mods = gamemods_mountedFilepaths;
		settings.crossplay_enabled = LobbyHandler.crossplayEnabled;
		settings.fast_restart = *cvar_fastRestart;
		settings.world_tooltip_scale = *cvar_worldtooltip_scale;
		settings.world_tooltip_scale_splitscreen = *cvar_worldtooltip_scale_splitscreen;
		settings.enemybar_scale = *cvar_enemybar_scale;
		settings.add_items_to_hotbar_enabled = auto_hotbar_new_items;
		settings.inventory_sorting = InventorySorting::load();
		settings.lastCharacter = LastCreatedCharacter::load();
		settings.use_on_release_enabled = !right_click_protect;
		settings.minimap = Minimap::load();
        settings.ui_scale = uiScale * 100.f;
        settings.ui_filter_enabled = *ui_filter;
		settings.item_tooltip_height =
			100.f * (Player::WorldUI_t::tooltipHeightOffsetZ - 6) / -6;
		settings.shootmode_crosshair = playerSettings[0].shootmodeCrosshair;
		settings.shootmode_crosshair_opacity = playerSettings[0].shootmodeCrosshairOpacity;
		settings.hdr_enabled = true;// *cvar_hdrEnabled;
		settings.show_messages_enabled = !disable_messages;
		settings.show_messages = Messages::load();
		settings.show_player_nametags_enabled = !hide_playertags;
		settings.show_hud_enabled = !nohud;
		settings.show_ip_address_enabled = !broadcast;
		settings.content_control_enabled = !spawn_blood;
		settings.colorblind_mode_enabled = colorblind;
		settings.arachnophobia_filter_enabled = arachnophobia_filter;
		settings.shaking_enabled = shaking;
		settings.bobbing_enabled = bobbing;
		settings.light_flicker_enabled = flickerLights;
		if (video) {
			settings.video = Video::load();
		} else {
			settings.video = allSettings.video;
		}
		settings.vertical_split_enabled = *vertical_splitscreen;
		settings.staggered_split_enabled = *staggered_splitscreen;
		settings.clipped_split_enabled = *clipped_splitscreen;
		settings.use_frame_interpolation = TimerExperiments::bUseTimerInterpolation;
		settings.fov = ::fov;
		settings.fps = *cvar_desiredFps;
		settings.audio_device = current_audio_device;
		settings.master_volume = MainMenu::master_volume * 100.f;
		settings.gameplay_volume = (float)sfxvolume * 100.f;
		settings.ambient_volume = (float)sfxAmbientVolume * 100.f;
		settings.environment_volume = (float)sfxEnvironmentVolume * 100.f;
		settings.notification_volume = (float)sfxNotificationVolume * 100.f;
		settings.music_volume = (float)musvolume * 100.f;
		settings.minimap_pings_enabled = !minimapPingMute;
		settings.player_monster_sounds_enabled = !mute_player_monster_sounds;
		settings.out_of_focus_audio_enabled = !mute_audio_on_focus_lost;
		settings.bindings = Bindings::load();
		settings.mkb_world_tooltips_enabled = *cvar_mkb_world_tooltips;
		settings.mkb_facehotbar = *cvar_mkb_facehotbar;
		settings.gamepad_facehotbar = *cvar_gamepad_facehotbar;
		settings.numkeys_in_inventory_enabled = hotbar_numkey_quick_add;
		settings.mouse_sensitivity = mousespeed;
		settings.reverse_mouse_enabled = reversemouse;
		settings.smooth_mouse_enabled = smoothmouse;
		settings.rotation_speed_limit_enabled = !disablemouserotationlimit;
		settings.turn_sensitivity_x = gamepad_rightx_sensitivity * 32768.0;
		settings.turn_sensitivity_y = gamepad_righty_sensitivity * 32768.0;
		settings.gamepad_camera_invert_x = gamepad_rightx_invert;
		settings.gamepad_camera_invert_y = gamepad_righty_invert;
		settings.classic_mode_enabled = svFlags & SV_FLAG_CLASSIC;
		settings.hardcore_mode_enabled = svFlags & SV_FLAG_HARDCORE;
		settings.friendly_fire_enabled = svFlags & SV_FLAG_FRIENDLYFIRE;
		settings.keep_inventory_enabled = svFlags & SV_FLAG_KEEPINVENTORY;
		settings.hunger_enabled = svFlags & SV_FLAG_HUNGER;
		settings.minotaur_enabled = svFlags & SV_FLAG_MINOTAURS;
		settings.random_traps_enabled = svFlags & SV_FLAG_TRAPS;
		settings.extra_life_enabled = svFlags & SV_FLAG_LIFESAVING;
		settings.cheats_enabled = svFlags & SV_FLAG_CHEATS;
		settings.skipintro = true;
		settings.port_number = ::portnumber;
		settings.show_lobby_code = !hidden_roomcode;
		for ( int i = 0; i < MAX_LOBBY_FILTERS_SAVED; ++i )
		{
			settings.lobby_filter_settings[i] = 0;
			if ( i < numFilters )
			{
				settings.lobby_filter_settings[i] = lobbyFilters[i];
			}
		}
		return settings;
	}

	inline AllSettings AllSettings::reset() {
		AllSettings settings;
		settings.mods = gamemods_mountedFilepaths;
		settings.crossplay_enabled = LobbyHandler.crossplayEnabled;
		settings.fast_restart = false;
		settings.world_tooltip_scale = 100.f;
		settings.world_tooltip_scale_splitscreen = 150.f;
		settings.enemybar_scale = 100.f;
		settings.add_items_to_hotbar_enabled = true;
		settings.inventory_sorting = InventorySorting::reset();
		settings.lastCharacter = LastCreatedCharacter::reset();
		settings.use_on_release_enabled = true;
		settings.minimap = Minimap::reset();
		settings.show_messages_enabled = true;
		settings.show_messages = Messages::reset();
		settings.show_player_nametags_enabled = true;
		settings.show_hud_enabled = true;
		settings.show_ip_address_enabled = true;
		settings.content_control_enabled = false;
		settings.colorblind_mode_enabled = false;
		settings.arachnophobia_filter_enabled = false;
		settings.shaking_enabled = true;
		settings.bobbing_enabled = true;
		settings.light_flicker_enabled = true;
		settings.video = Video::reset();
		settings.vertical_split_enabled = false;
		settings.clipped_split_enabled = false;
		settings.staggered_split_enabled = false;
		settings.use_frame_interpolation = true;
		settings.fov = 60;
		settings.fps = AUTO_FPS;
		settings.item_tooltip_height = 100.f;
		settings.shootmode_crosshair = 0;
		settings.shootmode_crosshair_opacity = 50;
		settings.hdr_enabled = true;
		settings.audio_device = "";
		settings.master_volume = 100.f;
		settings.gameplay_volume = 100.f;
		settings.ambient_volume = 100.f;
		settings.environment_volume = 100.f;
		settings.notification_volume = 100.f;
		settings.music_volume = 100.f;
		settings.minimap_pings_enabled = true;
		settings.player_monster_sounds_enabled = true;
		settings.out_of_focus_audio_enabled = true;
		settings.bindings = Bindings::reset(defaultControlLayout);
		settings.mkb_facehotbar = false;
		settings.gamepad_facehotbar = true;
		settings.mkb_world_tooltips_enabled = true;
		settings.numkeys_in_inventory_enabled = true;
		settings.mouse_sensitivity = 32.f;
		settings.reverse_mouse_enabled = false;
		settings.smooth_mouse_enabled = false;
		settings.rotation_speed_limit_enabled = false;
		settings.turn_sensitivity_x = 75.f;
		settings.turn_sensitivity_y = 50.f;
		settings.gamepad_camera_invert_x = false;
		settings.gamepad_camera_invert_y = false;
		settings.classic_mode_enabled = false;
		settings.hardcore_mode_enabled = false;
		settings.friendly_fire_enabled = true;
		settings.keep_inventory_enabled = false;
		settings.hunger_enabled = true;
		settings.minotaur_enabled = true;
		settings.random_traps_enabled = true;
		settings.extra_life_enabled = false;
		settings.cheats_enabled = false;
		settings.skipintro = true;
		settings.port_number = DEFAULT_PORT;
		settings.show_lobby_code = true;
		for ( int i = 0; i < MAX_LOBBY_FILTERS_SAVED; ++i )
		{
			settings.lobby_filter_settings[i] = 0;
		}
		return settings;
	}

	bool AllSettings::serialize(FileInterface* file) {
	    int version = 16;
	    file->property("version", version);
	    file->property("mods", mods);
		file->property("crossplay_enabled", crossplay_enabled);
		file->propertyVersion("fast_restart", version >= 2, fast_restart);
		file->property("add_items_to_hotbar_enabled", add_items_to_hotbar_enabled);
		if ( version < 9 )
		{
			// redo default hotbar sorting now that it is cleaner
			inventory_sorting = InventorySorting::reset();
		}
		else
		{
			file->property("inventory_sorting", inventory_sorting);
		}
		if ( version < 10 )
		{
			lastCharacter = LastCreatedCharacter::reset();
		}
		else
		{
			file->property("last_characters", lastCharacter);
		}
		file->property("use_on_release_enabled", use_on_release_enabled);
		file->property("minimap", minimap);
        file->propertyVersion("ui_filter", version >= 7, ui_filter_enabled);
        file->propertyVersion("ui_scale", version >= 7, ui_scale);
		file->propertyVersion("item_tooltip_height", version >= 11, item_tooltip_height);
		file->propertyVersion("shootmode_crosshair", version >= 15, shootmode_crosshair);
		file->propertyVersion("shootmode_crosshair_opacity", version >= 15, shootmode_crosshair_opacity);
		file->property("show_messages_enabled", show_messages_enabled);
		file->propertyVersion("message_filters", version >= 14, show_messages);
		file->property("show_player_nametags_enabled", show_player_nametags_enabled);
		file->property("show_hud_enabled", show_hud_enabled);
		file->property("show_ip_address_enabled", show_ip_address_enabled);
		file->property("content_control_enabled", content_control_enabled);
		file->property("colorblind_mode_enabled", colorblind_mode_enabled);
		file->property("arachnophobia_filter_enabled", arachnophobia_filter_enabled);
		file->property("shaking_enabled", shaking_enabled);
		file->property("bobbing_enabled", bobbing_enabled);
		file->property("light_flicker_enabled", light_flicker_enabled);
        if (version >= 1) {
            file->property("video", video);
            file->property("vertical_split_enabled", vertical_split_enabled);
            if (version >= 2) {
                file->property("clipped_split_enabled", clipped_split_enabled);
                file->property("staggered_split_enabled", staggered_split_enabled);
            }
            if ( version >= 6 )
            {
                file->property("use_frame_interpolation", use_frame_interpolation);
            }
        } else {
            int i = 0;
            float f = 0.f;
            bool b = false;
            file->property("window_mode", i);
            file->property("resolution_x", i);
            file->property("resolution_y", i);
            file->property("vsync_enabled", b);
            file->property("vertical_split_enabled", vertical_split_enabled);
            file->property("gamma", f);
        }
		file->property("fov", fov);
        if (version < 8) {
            fps = AUTO_FPS;
        } else {
            file->property("fps", fps);
        }
		file->propertyVersion("use_hdr", version >= 11, hdr_enabled);
		file->propertyVersion("audio_device", version >= 4, audio_device);
		file->property("master_volume", master_volume);
		file->property("gameplay_volume", gameplay_volume);
		file->property("ambient_volume", ambient_volume);
		file->property("environment_volume", environment_volume);
		if ( version >= 9 )
		{
			file->property("notification_volume", notification_volume);
		}
		file->property("music_volume", music_volume);
		file->property("minimap_pings_enabled", minimap_pings_enabled);
		file->property("player_monster_sounds_enabled", player_monster_sounds_enabled);
		file->property("out_of_focus_audio_enabled", out_of_focus_audio_enabled);
		file->property("bindings", bindings);
        if ( version >= 5 )
        {
            file->property("mkb_world_tooltips_enabled", mkb_world_tooltips_enabled);
            file->property("mkb_facehotbar", mkb_facehotbar);
            file->property("gamepad_facehotbar", gamepad_facehotbar);
            file->property("world_tooltip_scale", world_tooltip_scale);
            file->property("world_tooltip_scale_splitscreen", world_tooltip_scale_splitscreen);
        }
		file->propertyVersion("enemybar_scale", version >= 16, enemybar_scale);
		file->property("numkeys_in_inventory_enabled", numkeys_in_inventory_enabled);
		file->property("mouse_sensitivity", mouse_sensitivity);
		file->property("reverse_mouse_enabled", reverse_mouse_enabled);
		file->property("smooth_mouse_enabled", smooth_mouse_enabled);
		//file->property("rotation_speed_limit_enabled", rotation_speed_limit_enabled);
		file->property("turn_sensitivity_x", turn_sensitivity_x);
		file->property("turn_sensitivity_y", turn_sensitivity_y);
        if ( version >= 6 )
        {
            file->property("gamepad_camera_invert_x", gamepad_camera_invert_x);
            file->property("gamepad_camera_invert_y", gamepad_camera_invert_y);
        }
		file->property("classic_mode_enabled", classic_mode_enabled);
		file->property("hardcore_mode_enabled", hardcore_mode_enabled);
		file->property("friendly_fire_enabled", friendly_fire_enabled);
		file->property("keep_inventory_enabled", keep_inventory_enabled);
		file->property("hunger_enabled", hunger_enabled);
		file->property("minotaur_enabled", minotaur_enabled);
		file->property("random_traps_enabled", random_traps_enabled);
		file->property("extra_life_enabled", extra_life_enabled);
		file->property("cheats_enabled", cheats_enabled);
		file->property("skipintro", skipintro);
		file->property("use_model_cache", useModelCache);
		file->property("debug_keys_enabled", enableDebugKeys);
		file->property("port_number", port_number);
		file->propertyVersion("show_lobby_code", version >= 12, show_lobby_code);
		file->propertyVersion("lobby_filters", version >= 13, lobby_filter_settings);
		return true;
	}

    static const char* getMatchingProfileName(int player, bool controller) {
        auto& bindings = controller ?
            allSettings.bindings.gamepad_bindings[player]:
            allSettings.bindings.kb_mouse_bindings[player];
        for (auto& layout : defaultBindings) {
            bool matchesLayout = true;
            if (controller) {
                for (auto& binding : layout.bindings) {
                    if (binding.gamepad != hiddenBinding) {
                        if (bindings[binding.action] != binding.gamepad) {
                            matchesLayout = false;
                            break;
                        }
                    }
                }
            } else {
                for (auto& binding : layout.bindings) {
                    if (bindings[binding.action] != binding.keyboard) {
                        if (binding.keyboard != hiddenBinding) {
                            matchesLayout = false;
                            break;
                        }
                    }
                }
            }
            if (matchesLayout) {
                return layout.name.c_str();
            }
        }
        return "Custom";
    }

/******************************************************************************/

	// Story text is formatted thus:
	// carat ^ advances the image index
	// pound # adds a pause
	// asterisk * followed by a single digit changes the text box size

	static void createStoryScreen(const char* file, void (*end_func)()) {
	    char filename[PATH_MAX];
	    (void)completePath(filename, file);

        struct Story {
            int version = 1;
            bool press_a_to_advance = false;
            std::vector<std::string> text;
            std::vector<std::string> images;

            bool serialize(FileInterface* file) {
                if (file->isReading()) {
                    text.clear();
                    images.clear();
                }
                file->property("version", version);
                file->property("press_a_to_advance", press_a_to_advance);
                file->property("text", text);
                file->property("images", images);
                return true;
            }
        };

        static Story story;
        static int story_text_chars;
        static int story_text_lines;
		static int story_text_pause;
		static int story_text_box_size;
		static float story_text_scroll;
		static float story_text_writer;
		static float story_text_box_scale;
		static bool story_text_adjust_box;
		static bool story_text_end;
		static int story_image_index;
		static float story_image_fade;
		static bool story_image_advanced;
		static void (*story_end_func)();
		static int story_skip;
		static float story_skip_timer;

		bool read_result = FileHelper::readObject(filename, story);
		if (!read_result) {
		    assert(0 && "Story file not found!");
		    return;
		}
		story_text_chars = 0;
		story_text_lines = 0;
		story_text_pause = 0;
		story_text_box_size = 2;
		story_text_scroll = 0.f;
		story_text_writer = 0.f;
		story_text_box_scale = 1.f;
		story_text_adjust_box = false;
		story_text_end = false;
		story_image_index = 0;
		story_image_fade = 0.f;
		story_image_advanced = false;
		story_end_func = end_func;
		story_skip = 0;
		story_skip_timer = 0.f;

        // fire effect
		static float firetimer;
		firetimer = 0.f;
        fireStop();
		fireStart();
		auto backdrop = main_menu_frame->addFrame("backdrop");
		backdrop->setSize(main_menu_frame->getActualSize());
		backdrop->setTickCallback([](Widget& widget){
		    constexpr float ticks_per_second = 20.f;
			const float inc = ticks_per_second / fpsLimit;
			firetimer += inc;
			if (firetimer >= 1.f) {
			    firetimer -= 1.f;
                fire();
			}
		    });
		backdrop->setDrawCallback([](const Widget& widget, const SDL_Rect rect){
		    if (fireTexture) {
                Image::draw(fireTexture->texid, fireTexture->w, fireTexture->h,
                    nullptr, rect, SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY}, 0xffffffff);
	        }
		    });
		backdrop->setBorder(0);
		backdrop->setColor(0);
		backdrop->setHollow(true);

        // story image
		(void)main_menu_frame->addImage(
			main_menu_frame->getSize(),
			0xffffffff,
			story.images[0].c_str(),
			"storyboard"
		);

        story_active = true;
		static auto end_story_screen = [](){
		    if (multiplayer == CLIENT && !victory) {
		        auto next = main_menu_frame->findButton("next");
		        if (next) {
		            next->setHideGlyphs(true);
		            next->setHideKeyboardGlyphs(true);
		            next->setText("Waiting for host...");
		        }
		    } else {
		        movie = false;
		        story_active = false;
		        story_end_func();
		    }
		    };

        if (multiplayer != CLIENT || victory) {
		    auto back_button = main_menu_frame->addButton("back");
		    back_button->setHideSelectors(true);
		    back_button->setText("Skip story");
		    back_button->setColor(makeColor(0, 0, 0, 0));
		    back_button->setHighlightColor(makeColor(0, 0, 0, 0));
		    back_button->setBorderColor(makeColor(0, 0, 0, 0));
		    back_button->setTextColor(0xffffffff);
		    back_button->setTextHighlightColor(0xffffffff);
		    back_button->setFont(smallfont_outline);
		    back_button->setHJustify(Button::justify_t::RIGHT);
		    back_button->setVJustify(Button::justify_t::CENTER);
		    back_button->setSize(SDL_Rect{Frame::virtualScreenX - 416, Frame::virtualScreenY - 70, 380, 50});
		    back_button->setCallback([](Button& button){
		        ++story_skip;
		        story_skip_timer = (float)TICKS_PER_SECOND;
		        switch (story_skip) {
		        case 0: button.setText("Skip story"); break;
		        case 1: button.setText("Confirm (1)?"); break;
		        case 2: button.setText("Confirm (2)?"); break;
		        }
		        if (story_skip >= 3) {
			        end_story_screen();
		        }
			    });
		    back_button->setWidgetBack("back");
		    back_button->setHideKeyboardGlyphs(false);
		    if (inputs.hasController(getMenuOwner())) {
		        back_button->setGlyphPosition(Button::glyph_position_t::CENTERED_RIGHT);
		        back_button->setButtonsOffset(SDL_Rect{16, 0, 0, 0,});
		    } else {
		        back_button->setGlyphPosition(Button::glyph_position_t::BOTTOM_RIGHT);
		    }
		}

		auto font = Font::get(bigfont_outline); assert(font);

		auto next_button_func = [](Button&){
	        if (story_text_pause) {
		        story_text_pause = 0;
			    if (story_text_end == true) {
				    end_story_screen();
			    } else {
	                auto font = Font::get(bigfont_outline); assert(font);
				    story_text_scroll = font->height() * story_text_box_size;
			    }
	        }
	        };

		auto textbox1 = main_menu_frame->addFrame("story_text_box");
		textbox1->setColor(makeColor(0, 0, 0, 127));
		textbox1->setBorder(0);
		if (!story.press_a_to_advance) {
		    textbox1->setHideSelectors(true);
		    textbox1->setHideGlyphs(true);
		    textbox1->setWidgetBack("back");
			textbox1->select();
		} else {
		    auto next = main_menu_frame->addButton("next");
		    next->setHideSelectors(true);
		    next->setColor(makeColor(0, 0, 0, 0));
		    next->setHighlightColor(makeColor(0, 0, 0, 0));
		    next->setBorderColor(makeColor(0, 0, 0, 0));
		    next->setTextColor(0xffffffff);
		    next->setTextHighlightColor(0xffffffff);
		    next->setFont(smallfont_outline);
		    next->setHJustify(Button::justify_t::CENTER);
		    next->setVJustify(Button::justify_t::TOP);
		    auto font = Font::get(bigfont_outline); assert(font);
		    next->setSize(SDL_Rect{
		        (Frame::virtualScreenX - 160) / 2,
		        (Frame::virtualScreenY - font->height() - 4),
		        160,
		        font->height() + 4,
		        });
		    next->setCallback(next_button_func);
			next->setTickCallback([](Widget& widget){
			    auto button = static_cast<Button*>(&widget);
                button->setInvisible(story_text_pause == 0);
		        //button->setText(inputs.hasController(0) ? "" : "Continue...");
			    });
		    next->setWidgetBack("back");
		    next->select();

		    next->setGlyphPosition(Button::glyph_position_t::CENTERED_TOP);
			next->setButtonsOffset(SDL_Rect{0, 8, 0, 0});
		    next->setHideKeyboardGlyphs(false);
		}

		auto textbox2 = textbox1->addFrame("story_text_box");
		textbox2->setScrollBarsEnabled(false);
		textbox2->setAllowScrollBinds(false);
		textbox2->setHollow(true);
		textbox2->setBorder(0);

		static auto change_box_size = [](float lines){
		    assert(main_menu_frame);
		    auto font = Font::get(bigfont_outline); assert(font);
		    auto textbox1 = main_menu_frame->findFrame("story_text_box");
		    textbox1->setSize(SDL_Rect{
		        160,
		        Frame::virtualScreenY - (int)(font->height() * std::max(lines + 2.f, 0.f)),
		        Frame::virtualScreenX - 320,
		        (int)(font->height() * std::max(lines + 1.f, 0.f)),
		        });
		    textbox1->setActualSize(SDL_Rect{
		        0,
		        0,
		        textbox1->getSize().w,
		        textbox1->getSize().h,
		        });
		    auto textbox2 = textbox1->findFrame("story_text_box");
		    textbox2->setSize(SDL_Rect{
		        font->height() / 2,
		        font->height() / 2 - 2,
		        Frame::virtualScreenX - 320 - font->height(),
		        (int)(font->height() * std::max(lines, 0.f)),
		        });
		    textbox2->setActualSize(SDL_Rect{
		        textbox2->getActualSize().x,
		        textbox2->getActualSize().y,
		        textbox2->getSize().w,
		        font->height() * 100,
		        });
		    };
		change_box_size(story_text_box_scale);

		static auto adjust_box_size = [](){
		    float f = story_text_box_size;
		    float diff = story_text_box_scale - f;
			const float inc = (1.f / fpsLimit) * 8.f;
		    if (fabs(diff) < inc) {
		        story_text_box_scale -= diff;
		    } else if (signbit(diff)) {
		        story_text_box_scale += inc;
		    } else {
		        story_text_box_scale -= inc;
		    }
            change_box_size(story_text_box_scale);
		    };

		auto field = textbox2->addField("text", 1 << 16);
		field->setFont(bigfont_outline);
		field->setSize(textbox2->getActualSize());
		field->setHJustify(Field::justify_t::CENTER);
		field->setVJustify(Field::justify_t::TOP);
		field->setColor(makeColor(255, 255, 255, 255));

		textbox1->setTickCallback([](Widget& widget){
			const float inc = 1.f * ((float)TICKS_PER_SECOND / (float)fpsLimit);
			auto textbox1 = static_cast<Frame*>(&widget);
			auto story_font = Font::get(bigfont_outline); assert(story_font);
			auto storyboard = main_menu_frame->findImage("storyboard"); assert(storyboard);
			if (storyboard && !story_text_pause) {
				story_image_fade = std::max(0.f, story_image_fade - inc);
		        float factor = story_image_fade - story_font->height();
		        Uint8 c = 255 * (fabs(factor) / story_font->height());
		        storyboard->color = makeColor(c, c, c, 255);
		        if (factor <= 0.f && story_image_advanced) {
		            story_image_advanced = false;
			        story_image_index = (story_image_index + 1) % story.images.size();
			        storyboard->path = story.images[story_image_index];
		        }
			}
			if (story_text_scroll > 0.f) {
				int old_story_text_scroll = (int)story_text_scroll;
				story_text_scroll -= inc;
				if (story_text_scroll < 0.f) {
					story_text_scroll = 0.f;
				}
				if ((int)story_text_scroll != old_story_text_scroll) {
					auto textbox2 = textbox1->findFrame("story_text_box");
					assert(textbox2);
					auto size = textbox2->getActualSize();
					++size.y;
					textbox2->setActualSize(size);
				}
			} else {
				if (story_text_pause > 0) {
				    if (!story.press_a_to_advance) {
					    --story_text_pause;
				    }
					if (story_text_pause == 0) {
						if (story_text_end == true) {
							end_story_screen();
						} else {
							story_text_scroll = story_font->height() * story_text_box_size;
						}
					}
				} else {
					story_text_writer -= 1.f;
					if (story_text_writer <= 0.f) {
						story_text_writer = fmodf(story_text_writer, 1.f);
						auto textbox2 = textbox1->findFrame("story_text_box");
						assert(textbox2);
						auto text = textbox2->findField("text");
						assert(text);
						size_t text_index = 0u;
						char* buf = const_cast<char*>(text->getText());
						int chars = story_text_chars;
						size_t len = strlen(buf);
					    for (;
					        text_index < story.text.size() && chars >= story.text[text_index].size();
					        chars -= story.text[text_index].size(), ++text_index);
						if (text_index < story.text.size()) {
							char pc = story.text[text_index][std::max(chars - 1, 0)];
							char c = story.text[text_index][chars];
							char nc = story.text[text_index][chars + 1];
							++story_text_chars;
							if (c == '\n') {
							    ++story_text_lines;
							    if (story_text_lines >= story_text_box_size) {
							        story_text_lines = 0;
								    story_text_pause = fpsLimit * 5;
							    }
							    if (nc == '^') {
							        story_image_advanced = true;
							        story_image_fade = story_font->height() * 2;
							    }
							} else if (c == '^') {
							    if (pc != '\n') {
							        story_image_advanced = true;
							        story_image_fade = story_font->height() * 2;
							    }
								return; // skip printing this character
							} else if (c == '#') {
								story_text_writer += fpsLimit / 10.f;
								return; // skip printing this character
							} else if (c == '*') {
								story_text_adjust_box = true;
								return; // skip printing this character
							} else if (story_text_adjust_box) {
								story_text_adjust_box = false;
								if (c >= '1' && c <= '9') {
								    story_text_box_size = (int)(c - '0');
								    return; // skip printing this character
								} else if (c == '0') {
								    story_text_box_size = -2;
								    return; // skip printing this character
								}
							} else {
								story_text_writer += fpsLimit / 30.f;
							}
							buf[len] = c;
							buf[len + 1] = '\0';
                            text->dirty = true;
						} else {
						    auto back = main_menu_frame->findButton("back");
							if (back) {
								back->setDisabled(true);
								back->setInvisible(true);
							}
							story_text_pause = fpsLimit * 5;
							story_text_end = true;
						}
					}
				}
			}
		    adjust_box_size();
		    if (story_skip_timer > 0.f) {
		        story_skip_timer -= inc;
		        if (story_skip_timer <= 0.f) {
		            story_skip_timer = 0.f;
		            story_skip = 0;
		            auto back_button = main_menu_frame->findButton("back");
					if (back_button) {
		            	back_button->setText("Skip story");
					}
					auto next = main_menu_frame->findButton("next");
					if (next) {
						next->select();
					}
		        }
		    }
			});
	}

	static void createCreditsScreen(bool endgame) {
		/*auto back_button = main_menu_frame->addButton("back");
		back_button->setHideSelectors(true);
		back_button->setText("Return to Main Menu");
		back_button->setColor(makeColor(0, 0, 0, 0));
		back_button->setHighlightColor(makeColor(0, 0, 0, 0));
		back_button->setBorderColor(makeColor(0, 0, 0, 0));
		back_button->setTextColor(0xffffffff);
		back_button->setTextHighlightColor(0xffffffff);
		back_button->setFont(smallfont_outline);
		back_button->setHJustify(Button::justify_t::RIGHT);
		back_button->setVJustify(Button::justify_t::CENTER);
		back_button->setSize(SDL_Rect{Frame::virtualScreenX - 400, Frame::virtualScreenY - 70, 380, 50});
		back_button->setGlyphPosition(Widget::glyph_position_t::BOTTOM_RIGHT);
		back_button->setCallback([](Button& b){
			destroyMainMenu();
			createMainMenu(false);
			mainArchives(b);
			auto buttons = main_menu_frame->findFrame("buttons"); assert(buttons);
			auto credits = buttons->findButton("Credits"); assert(credits);
			credits->select();
			});
		back_button->setWidgetBack("back");
	    back_button->setHideKeyboardGlyphs(false);
		back_button->select();*/

		if (endgame) {
		    movie = true;
		    auto backdrop = main_menu_frame->addImage(
		        SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY},
			    0xffffffff,
			    "*images/ui/Main Menus/Story/black.png",
			    "backdrop"
			    );
		}

        if (endgame) {
		    auto back = createBackWidget(main_menu_frame,
		        [](Button&){
		        soundCancel();
			    destroyMainMenu();
			    createMainMenu(false);
			    });
		    back->select();
        } else {
		    auto back = createBackWidget(main_menu_frame,
		        [](Button& b){
			    destroyMainMenu();
			    createMainMenu(false);
			    mainArchives(b);
			    auto buttons = main_menu_frame->findFrame("buttons"); assert(buttons);
			    auto credits = buttons->findButton("Credits"); assert(credits);
			    credits->select();
			    });
		    back->select();
		}

		auto font = Font::get(bigfont_outline); assert(font);

		static float credits_scroll = 0.f;
		constexpr int num_credits_lines = 83;

		auto credits = main_menu_frame->addFrame("credits");
		credits->setSize(SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY});
		credits->setActualSize(SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY + font->height() * num_credits_lines});
		credits->setScrollBarsEnabled(false);
		credits->setAllowScrollBinds(false);
		credits->setHollow(true);
		credits->setBorder(0);
		credits->setTickCallback([](Widget& widget){
			const float inc = 1.f * ((float)TICKS_PER_SECOND / (float)fpsLimit);
			int old_credits_scroll = (int)credits_scroll;
			credits_scroll += inc;
			if (old_credits_scroll != (int)credits_scroll) {
				auto credits = static_cast<Frame*>(&widget);
				auto size = credits->getActualSize();
				size.y += 1;
				if (size.y >= size.h) {
					size.y = 0;
				}
				credits->setActualSize(size);
			}
			});

		// titles
		auto text1 = credits->addField("text1", 1024);
		text1->setFont(bigfont_outline);
		text1->setColor(makeColor(255, 191, 32, 255));
		text1->setHJustify(Field::justify_t::CENTER);
		text1->setVJustify(Field::justify_t::TOP);
		text1->setSize(SDL_Rect{0, Frame::virtualScreenY, Frame::virtualScreenX, font->height() * num_credits_lines});
		text1->setText(
			u8"Project lead, programming, and design\n"
			u8" \n"
			u8" \n \n \n \n \n"
			u8"Music and sound design\n"
			u8" \n"
			u8" \n \n \n \n \n"
			u8"Programming\n"
			u8" \n"
			u8" \n \n \n \n \n"
			u8"Art and design\n"
			u8" \n"
			u8" \n \n \n \n \n"
			u8"Programming and design\n"
			u8" \n"
			u8" \n \n \n \n \n"
			u8"Additional art\n"
			u8" \n"
			u8" \n \n \n \n \n"
			u8"Additional writing\n"
			u8" \n"
			u8" \n \n \n \n \n"
			u8"Special thanks\n"
			u8" \n"
			u8" \n"
			u8" \n"
			u8" \n"
			u8" \n"
			u8" \n"
			u8" \n"
			u8" \n \n \n \n \n"
			u8"Many thanks to our open-source community!\n"
			u8" \n"
			u8" \n \n \n \n \n"
			u8"Barony is a product of Turning Wheel LLC\n"
			u8" \n"
#ifdef USE_FMOD
			u8" \n"
#endif
			u8" \n"
			u8" \n \n \n \n \n"
			u8"This game is dedicated to all of our friends, family, and fans\n"
			u8"who encouraged and supported us on our journey to finish it.\n"
			u8" \n"
			u8" \n"
		);

		const char text2_str[] =
			u8" \n"
			u8"Sheridan Rathbun\n"
			u8" \n \n \n \n \n"
			u8" \n"
			u8"Chris Kukla\n"
			u8" \n \n \n \n \n"
			u8" \n"
			u8"Ciprian Elies\n"
			u8" \n \n \n \n \n"
			u8" \n"
			u8"Josiah Colborn\n"
			u8" \n \n \n \n \n"
			u8" \n"
			u8"Ben Potter\n"
			u8" \n \n \n \n \n"
			u8" \n"
			u8"Matthew Griebner\n"
			u8" \n \n \n \n \n"
			u8" \n"
			u8"Frasier Panton\n"
			u8" \n \n \n \n \n"
			u8" \n"
			u8"Our Kickstarter Backers\n"
			u8"Kevin White\n"
			u8"Julian Seeger\n"
			u8"Mathias Golinelli\n"
			u8"Sterling Rathbun\n"
			u8"Desiree Colborn\n"
			u8"Jesse Riddle\n"
			u8" \n \n \n \n \n"
			u8" \n"
			u8"Barony was made possible by many anonymous contributors.\n"
			u8" \n \n \n \n \n"
			u8" \n"
			u8"Copyright \u00A9 %s, all rights reserved\n"
#ifdef USE_FMOD
			u8"Made with FMOD Core by Firelight Technologies Pty Ltd.\n"
#endif
			u8"https://www.baronygame.com/\n"
			u8" \n \n \n \n \n"
			u8" \n"
			u8" \n"
			u8" \n"
			u8"Thank you!\n";
		
		char buf[1024];
		const char date[] = __DATE__;
		const char* year = (const char*)date + sizeof(date) - 5;
		snprintf(buf, sizeof(buf), text2_str, year);

		// entries
		auto text2 = credits->addField("text2", 1024);
		text2->setFont(bigfont_outline);
		text2->setColor(0xffffffff);
		text2->setHJustify(Field::justify_t::CENTER);
		text2->setVJustify(Field::justify_t::TOP);
		text2->setSize(SDL_Rect{0, Frame::virtualScreenY, Frame::virtualScreenX, font->height() * num_credits_lines});
		text2->setText(buf);
	}

/******************************************************************************/

	static std::string settings_tab_name;

	struct Setting {
		enum class Type : Uint8 {
			Boolean = 0,
			Slider = 1,
			Customize = 2,
			BooleanWithCustomize = 3,
			Dropdown = 4,
			Binding = 5,
			Field = 6,
		};
		Type type;
		const char* name;
	};

	void settingsApply() {
        auto save_result = allSettings.save();

		// change video mode
        if (initialized) {
            video_refresh = save_result;
#if !defined(VIDEO_RESTART_NEEDED)
            if (video_refresh & VideoRefresh::Video) {
                int x = std::max(allSettings.video.resolution_x, 1024);
                int y = std::max(allSettings.video.resolution_y, 720);
                if (!changeVideoMode(x, y)) {
                    printlog("critical error! Attempting to abort safely...\n");
                    mainloop = 0;
                }
            }
#endif
		}

		// apply splitscreen setting
	    if (!intro) {
	        setupSplitscreen();
	    }

		// transmit server flags
		if (initialized && !intro) {
			sendSvFlagsOverNet();
			messagePlayer(clientnum, MESSAGE_MISC, language[276]);
		}

		// set volume and sound driver
		if (initialized) {
			setAudioDevice(current_audio_device);
		    setGlobalVolume(master_volume, musvolume, sfxvolume, sfxAmbientVolume, sfxEnvironmentVolume, sfxNotificationVolume);
		}
	}

	void settingsMount(bool video) {
		allSettings = AllSettings::load(video);
		if (!video_refresh) {
	        old_video = allSettings.video;
	    }
	}

	bool settingsSave() {
		return FileHelper::writeObject((std::string(outputdir) + "/config/config.json").c_str(), EFileFormat::Json, allSettings);
	}

	bool settingsLoad() {
		bool result = FileHelper::readObject((std::string(outputdir) + "/config/config.json").c_str(), allSettings);
		if (result) {
		    old_bindings = allSettings.bindings;
		}
		return result;
	}

	void settingsReset() {
		allSettings = AllSettings::reset();
		old_bindings = allSettings.bindings;
	}

	static void settingsCustomizeInventorySorting(Button&);

	static void inventorySortingDefaults(Button& button) {
		soundActivate();
		allSettings.inventory_sorting = InventorySorting::reset();
		auto window = static_cast<Frame*>(button.getParent());
		auto dimmer = static_cast<Frame*>(window->getParent());
		dimmer->removeSelf();
		settingsCustomizeInventorySorting(button);
	}

	static void inventorySortingDiscard(Button& button) {
		soundCancel();
        auto window = static_cast<Frame*>(button.getParent());
        auto dimmer = static_cast<Frame*>(window->getParent());
        dimmer->removeSelf();
		if (main_menu_frame) {
			auto settings = main_menu_frame->findFrame("settings");
			if (settings) {
				auto settings_subwindow = settings->findFrame("settings_subwindow");
				if (settings_subwindow) {
					auto inventory_sorting_customize = settings_subwindow->findButton("setting_inventory_sorting_customize_button");
					if (inventory_sorting_customize) {
						inventory_sorting_customize->select();
					}
				}
			}
		}
	}

	static void inventorySortingConfirm(Button& button) {
		soundActivate();
		auto window = static_cast<Frame*>(button.getParent());
		auto dimmer = static_cast<Frame*>(window->getParent());
		dimmer->removeSelf();
		auto settings = main_menu_frame->findFrame("settings");
		if (settings) {
			auto settings_subwindow = settings->findFrame("settings_subwindow");
			if (settings_subwindow) {
				auto inventory_sorting_customize = settings_subwindow->findButton("setting_inventory_sorting_customize_button");
				if (inventory_sorting_customize) {
					inventory_sorting_customize->select();
				}
			}
		}
		allSettings.inventory_sorting.save();
	}

	static void settingsCustomizeInventorySorting(Button& button) {
		soundActivate();
        
        auto dimmer = main_menu_frame->addFrame("dimmer");
        dimmer->setSize(SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY});
        dimmer->setColor(makeColor(0, 0, 0, 63));
        dimmer->setBorder(0);

		auto window = dimmer->addFrame("inventory_sorting_window");
		window->setSize(SDL_Rect{
			(Frame::virtualScreenX - 978) / 2,
			(Frame::virtualScreenY - 718) / 2,
			978,
			718
			});
		window->setActualSize(SDL_Rect{0, 0, window->getSize().w, window->getSize().h});
		window->setColor(0);
		window->setBorder(0);
		window->setTickCallback([](Widget& widget){
			auto frame = static_cast<Frame*>(&widget);
			updateSliderArrows(*frame);
			});

		// slider arrows
		auto sliderLeft = window->addImage(
			SDL_Rect{0, 0, 30, 44},
			0xffffffff,
			"*images/ui/Main Menus/Settings/AutoSort/AutoSort_SliderBox_Left00.png",
			"slider_left"
		);
		sliderLeft->disabled = true;
		sliderLeft->ontop = true;
		auto sliderRight = window->addImage(
			SDL_Rect{0, 0, 30, 44},
			0xffffffff,
			"*images/ui/Main Menus/Settings/AutoSort/AutoSort_SliderBox_Right00.png",
			"slider_right"
		);
		sliderRight->disabled = true;
		sliderRight->ontop = true;

		// banner
		auto banner_text = window->addField("banner_text", 64);
		banner_text->setSize(SDL_Rect{14, 4, 950, 50});
		banner_text->setText("AUTOMATIC INVENTORY BEHAVIOR");
		banner_text->setFont(bigfont_outline);
		banner_text->setJustify(Field::justify_t::CENTER);

		auto populate_text = window->addField("populate_text", 64);
		populate_text->setSize(SDL_Rect{20, 64, 512, 64});
		populate_text->setText("Populate Hotbar");
		populate_text->setFont(bigfont_outline);
		populate_text->setVJustify(Field::justify_t::TOP);
		populate_text->setHJustify(Field::justify_t::LEFT);

		auto sort_text = window->addField("sort_text", 64);
		sort_text->setSize(SDL_Rect{16, 64, 942, 64});
		sort_text->setText("Auto-Sort Order");
		sort_text->setFont(bigfont_outline);
		sort_text->setVJustify(Field::justify_t::TOP);
		sort_text->setHJustify(Field::justify_t::CENTER);

		// background
		auto background = window->addImage(
			window->getActualSize(),
			0xffffffff,
			"*images/ui/Main Menus/Settings/AutoSort/AutoSort_WindowALL01.png",
			"background"
		);

		// bottom buttons
		struct Option {
			const char* name;
			const char* text;
			void (*callback)(Button&);
		};
		Option options[] = {
			{"Defaults", "Restore\nDefaults", inventorySortingDefaults},
			{"Discard", "Discard\n& Exit", inventorySortingDiscard},
			{"Confirm", "Confirm\n& Exit", inventorySortingConfirm},
		};
		const int num_options = sizeof(options) / sizeof(options[0]);
		for (int c = 0; c < num_options; ++c) {
			auto button = window->addButton(options[c].name);
			button->setSize(SDL_Rect{412 + (582 - 412) * c, 638, 164, 62});
			button->setBackground("*images/ui/Main Menus/Settings/AutoSort/Button_Basic00.png");
			button->setBackgroundHighlighted("*images/ui/Main Menus/Settings/AutoSort/Button_BasicHigh00.png");
			button->setBackgroundActivated("*images/ui/Main Menus/Settings/AutoSort/Button_BasicPress00.png");
			button->setText(options[c].text);
			button->setFont(smallfont_outline);
			button->setColor(makeColor(255, 255, 255, 255));
			button->setHighlightColor(makeColor(255, 255, 255, 255));
			if (c > 0) {
				button->setWidgetLeft(options[c - 1].name);
			}
			if (c < num_options - 1) {
				button->setWidgetRight(options[c + 1].name);
			}
			button->setWidgetUp("sort_slider10");
			button->setCallback(options[c].callback);
			button->setWidgetBack("Discard");
			button->addWidgetAction("MenuAlt1", "Defaults");
			button->addWidgetAction("MenuStart", "Confirm");
			button->setHideKeyboardGlyphs(false);
		}
		auto first_button = window->findButton(options[0].name); assert(first_button);
		first_button->select();
		first_button->setWidgetLeft("hotbar_button11");

		// hotbar toggle buttons
		void (*hotbar_callbacks[12])(Button&) = {
			[](Button& button){ soundCheckmark(); allSettings.inventory_sorting.hotbarWeapons = button.isPressed(); },
			[](Button& button){ soundCheckmark(); allSettings.inventory_sorting.hotbarArmor = button.isPressed(); },
			[](Button& button){ soundCheckmark(); allSettings.inventory_sorting.hotbarAmulets = button.isPressed(); },
			[](Button& button){ soundCheckmark(); allSettings.inventory_sorting.hotbarBooks = button.isPressed(); },
			[](Button& button){ soundCheckmark(); allSettings.inventory_sorting.hotbarTools = button.isPressed(); },
			[](Button& button){ soundCheckmark(); allSettings.inventory_sorting.hotbarThrown = button.isPressed(); },
			[](Button& button){ soundCheckmark(); allSettings.inventory_sorting.hotbarGems = button.isPressed(); },
			[](Button& button){ soundCheckmark(); allSettings.inventory_sorting.hotbarPotions = button.isPressed(); },
			[](Button& button){ soundCheckmark(); allSettings.inventory_sorting.hotbarScrolls = button.isPressed(); },
			[](Button& button){ soundCheckmark(); allSettings.inventory_sorting.hotbarStaves = button.isPressed(); },
			[](Button& button){ soundCheckmark(); allSettings.inventory_sorting.hotbarFood = button.isPressed(); },
			[](Button& button){ soundCheckmark(); allSettings.inventory_sorting.hotbarSpells = button.isPressed(); },
		};
		const int num_hotbar_buttons = sizeof(hotbar_callbacks) / sizeof(hotbar_callbacks[0]);
		for (int c = num_hotbar_buttons - 1; c >= 0; --c) {
			auto button = window->addButton((std::string("hotbar_button") + std::to_string(c)).c_str());
			button->setSize(SDL_Rect{72, 88 + c * 50, 44, 42});
			button->setBackground("*images/ui/Main Menus/Settings/AutoSort/AutoSort_Populate_Box01.png");
			button->setIcon("*#images/ui/Main Menus/Settings/AutoSort/AutoSort_Populate_Checkmark01.png");
			button->setStyle(Button::style_t::STYLE_CHECKBOX);
			button->setCallback(hotbar_callbacks[c]);
			button->setSelectorOffset(SDL_Rect{0, 6, -4, 4});
			button->setBorder(0);
			if (c > 0) {
				button->setWidgetUp((std::string("hotbar_button") + std::to_string(c - 1)).c_str());
			}
			if (c < num_hotbar_buttons - 1) {
				button->setWidgetDown((std::string("hotbar_button") + std::to_string(c + 1)).c_str());
			}
			if (c == num_hotbar_buttons - 1) {
				button->setWidgetRight("Defaults");
			} else {
				button->setWidgetRight((std::string("sort_slider") + std::to_string(c)).c_str());
			}
			switch (c) {
			case 0: button->setPressed(allSettings.inventory_sorting.hotbarWeapons); break;
			case 1: button->setPressed(allSettings.inventory_sorting.hotbarArmor); break;
			case 2: button->setPressed(allSettings.inventory_sorting.hotbarAmulets); break;
			case 3: button->setPressed(allSettings.inventory_sorting.hotbarBooks); break;
			case 4: button->setPressed(allSettings.inventory_sorting.hotbarTools); break;
			case 5: button->setPressed(allSettings.inventory_sorting.hotbarThrown); break;
			case 6: button->setPressed(allSettings.inventory_sorting.hotbarGems); break;
			case 7: button->setPressed(allSettings.inventory_sorting.hotbarPotions); break;
			case 8: button->setPressed(allSettings.inventory_sorting.hotbarScrolls); break;
			case 9: button->setPressed(allSettings.inventory_sorting.hotbarStaves); break;
			case 10: button->setPressed(allSettings.inventory_sorting.hotbarFood); break;
			case 11: button->setPressed(allSettings.inventory_sorting.hotbarSpells); break;
			default: break;
			}
			button->setWidgetBack("Discard");
			button->addWidgetAction("MenuAlt1", "Defaults");
			button->addWidgetAction("MenuStart", "Confirm");
		}

		// inventory sort sliders
		void (*sort_slider_callbacks[11])(Slider&) = {
			[](Slider& slider){ soundSlider(true); allSettings.inventory_sorting.sortWeapons = slider.getValue(); },
			[](Slider& slider){ soundSlider(true); allSettings.inventory_sorting.sortArmor = slider.getValue(); },
			[](Slider& slider){ soundSlider(true); allSettings.inventory_sorting.sortAmulets = slider.getValue(); },
			[](Slider& slider){ soundSlider(true); allSettings.inventory_sorting.sortBooks = slider.getValue(); },
			[](Slider& slider){ soundSlider(true); allSettings.inventory_sorting.sortTools = slider.getValue(); },
			[](Slider& slider){ soundSlider(true); allSettings.inventory_sorting.sortThrown = slider.getValue(); },
			[](Slider& slider){ soundSlider(true); allSettings.inventory_sorting.sortGems = slider.getValue(); },
			[](Slider& slider){ soundSlider(true); allSettings.inventory_sorting.sortPotions = slider.getValue(); },
			[](Slider& slider){ soundSlider(true); allSettings.inventory_sorting.sortScrolls = slider.getValue(); },
			[](Slider& slider){ soundSlider(true); allSettings.inventory_sorting.sortStaves = slider.getValue(); },
			[](Slider& slider){ soundSlider(true); allSettings.inventory_sorting.sortFood = slider.getValue(); },
			//[](Slider& slider){ allSettings.inventory_sorting.sortEquipped = slider.getValue(); }, // Hey, we don't have enough room for this
		};
		const int num_sliders = sizeof(sort_slider_callbacks) / sizeof(sort_slider_callbacks[0]);
		for (int c = num_sliders - 1; c >= 0; --c) {
			auto slider = window->addSlider((std::string("sort_slider") + std::to_string(c)).c_str());
			slider->setMaxValue(6.f);
			slider->setMinValue(-6.f);
			slider->setRailSize(SDL_Rect{158, 100 + 50 *c, 724, 24});
			slider->setHandleSize(SDL_Rect{0, 0, 52, 54});
			slider->setRailImage("*images/ui/Main Menus/Settings/AutoSort/transparent.png");
			slider->setHandleImage("*images/ui/Main Menus/Settings/AutoSort/AutoSort_SliderBox_BackBlack00.png");
			slider->setCallback(sort_slider_callbacks[c]);
			switch (c) {
			case 0: slider->setValue(allSettings.inventory_sorting.sortWeapons); break;
			case 1: slider->setValue(allSettings.inventory_sorting.sortArmor); break;
			case 2: slider->setValue(allSettings.inventory_sorting.sortAmulets); break;
			case 3: slider->setValue(allSettings.inventory_sorting.sortBooks); break;
			case 4: slider->setValue(allSettings.inventory_sorting.sortTools); break;
			case 5: slider->setValue(allSettings.inventory_sorting.sortThrown); break;
			case 6: slider->setValue(allSettings.inventory_sorting.sortGems); break;
			case 7: slider->setValue(allSettings.inventory_sorting.sortPotions); break;
			case 8: slider->setValue(allSettings.inventory_sorting.sortScrolls); break;
			case 9: slider->setValue(allSettings.inventory_sorting.sortStaves); break;
			case 10: slider->setValue(allSettings.inventory_sorting.sortFood); break;
			default: break;
			}
			if (c > 0) {
				slider->setWidgetUp((std::string("sort_slider") + std::to_string(c - 1)).c_str());
			}
			if (c < num_sliders - 1) {
				slider->setWidgetDown((std::string("sort_slider") + std::to_string(c + 1)).c_str());
			}
			else {
				slider->setWidgetDown("Defaults");
			}
			slider->setWidgetLeft((std::string("hotbar_button") + std::to_string(c)).c_str());
			const char* icon_img_path = nullptr;
			switch (c) {
			case 0: icon_img_path = "*images/ui/Main Menus/Settings/AutoSort/AutoSort_SliderBox_Sword00.png"; break;
			case 1: icon_img_path = "*images/ui/Main Menus/Settings/AutoSort/AutoSort_SliderBox_Helmet00.png"; break;
			case 2: icon_img_path = "*images/ui/Main Menus/Settings/AutoSort/AutoSort_SliderBox_Necklace00.png"; break;
			case 3: icon_img_path = "*images/ui/Main Menus/Settings/AutoSort/AutoSort_SliderBox_Book00.png"; break;
			case 4: icon_img_path = "*images/ui/Main Menus/Settings/AutoSort/AutoSort_SliderBox_Torch00.png"; break;
			case 5: icon_img_path = "*images/ui/Main Menus/Settings/AutoSort/AutoSort_SliderBox_Chakram00.png"; break;
			case 6: icon_img_path = "*images/ui/Main Menus/Settings/AutoSort/AutoSort_SliderBox_Gem00.png"; break;
			case 7: icon_img_path = "*images/ui/Main Menus/Settings/AutoSort/AutoSort_SliderBox_Potion00.png"; break;
			case 8: icon_img_path = "*images/ui/Main Menus/Settings/AutoSort/AutoSort_SliderBox_Scroll00.png"; break;
			case 9: icon_img_path = "*images/ui/Main Menus/Settings/AutoSort/AutoSort_SliderBox_Staff00.png"; break;
			case 10: icon_img_path = "*images/ui/Main Menus/Settings/AutoSort/AutoSort_SliderBox_Bread00.png"; break;
			default: break;
			}
			auto icon = window->addImage(
				SDL_Rect{0, 0, 52, 54},
				0xffffffff,
				icon_img_path ? icon_img_path : "",
				(std::string("sort_slider_img") + std::to_string(c)).c_str()
			);
			icon->ontop = true;
			icon->disabled = true;
			slider->setTickCallback([](Widget& widget){
				Slider* slider = static_cast<Slider*>(&widget);
				slider->setValue((int)slider->getValue());
				auto window = main_menu_frame->findFrame("inventory_sorting_window");
				if (window) {
					auto number = std::string(slider->getName()).substr(sizeof("sort_slider") - 1);
					int c = atoi(number.c_str());
					auto icon = window->findImage((std::string("sort_slider_img") + std::to_string(c)).c_str()); assert(icon);
					int x = slider->getHandleSize().x;
					int y = slider->getHandleSize().y;
					if (x || y) {
						icon->disabled = false;
						icon->pos.x = x;
						icon->pos.y = y;
					}
					if (slider->isSelected()) {
						slider->setHandleImage("*images/ui/Main Menus/Settings/AutoSort/AutoSort_SliderBox_BackBrown00.png");
					} else {
						slider->setHandleImage("*images/ui/Main Menus/Settings/AutoSort/AutoSort_SliderBox_BackBlack00.png");
					}
				}
				});
			slider->setWidgetBack("Discard");
			slider->addWidgetAction("MenuAlt1", "Defaults");
			slider->addWidgetAction("MenuStart", "Confirm");
		}
		window->addImage(
			SDL_Rect{126, 100, 788, 524},
			0xffffffff,
			"*images/ui/Main Menus/Settings/AutoSort/AutoSort_Sliders_BackingALL00.png",
			"slider_backing"
		);
		window->addImage(
			SDL_Rect{188, 98, 664, 526},
			0xffffffff,
			"*images/ui/Main Menus/Settings/AutoSort/AutoSort_Sliders_Spacers01.png",
			"slider_spacers"
		);

		// spell box
		window->addImage(
			SDL_Rect{134, 638, 52, 54},
			0xffffffff,
			"*images/ui/Main Menus/Settings/AutoSort/AutoSort_SliderBox_BackBlack00.png",
			"spellbox"
		);
		window->addImage(
			SDL_Rect{134, 638, 52, 54},
			0xffffffff,
			"*images/ui/Main Menus/Settings/AutoSort/AutoSort_SliderBox_Magic00.png",
			"spellbox_icon"
		);
	}

	enum DropdownType {
		Normal,
		Short,
		Wide,
		Short_2Slot
	};

	static void settingsOpenDropdown(Button& button, const char* name, DropdownType type, void(*entry_func)(Frame::entry_t&)) {
		std::string dropdown_name = "setting_" + std::string(name) + "_dropdown";
		auto frame = static_cast<Frame*>(button.getParent());
		auto dropdown = frame->addFrame(dropdown_name.c_str()); assert(dropdown);
		dropdown->setSize(SDL_Rect{
			button.getSize().x,
			button.getSize().y,
			type == DropdownType::Wide ? 640 : 174,
			(type == DropdownType::Short) ? 181 : ((type == DropdownType::Short_2Slot) ? 84 : 362)
			});
		dropdown->setActualSize(SDL_Rect{0, 0, dropdown->getSize().w, dropdown->getSize().h});
		dropdown->setColor(0);
		dropdown->setBorder(0);
		dropdown->setDropDown(true);

		const char* background_img;
		switch (type) {
		default:
		case DropdownType::Normal: background_img = "*images/ui/Main Menus/Settings/Settings_Drop_ScrollBG00.png"; break;
		case DropdownType::Short: background_img = "*images/ui/Main Menus/Settings/Settings_Drop_ScrollBG01.png"; break;
		case DropdownType::Wide: background_img = "*images/ui/Main Menus/Settings/Settings_WideDrop_ScrollBG01.png"; break;
		case DropdownType::Short_2Slot: background_img = "*images/ui/Main Menus/Settings/Settings_Drop_ScrollBG03_2Slot.png"; break;
		}
		auto background = dropdown->addImage(
			dropdown->getActualSize(),
			0xffffffff,
			background_img,
			"background"
		);

		int border = 4;
		auto dropdown_list = dropdown->addFrame("list");
		dropdown_list->setSize(SDL_Rect{0, border, dropdown->getSize().w, dropdown->getSize().h - border*2});
		dropdown_list->setActualSize(SDL_Rect{0, 0, dropdown_list->getSize().w, dropdown_list->getSize().h});
		dropdown_list->setColor(0);
		dropdown_list->setBorder(0);
		dropdown_list->setFont(bigfont_outline);
		dropdown_list->setListOffset(SDL_Rect{0, 11, 0, 0});
		dropdown_list->setListJustify(Frame::justify_t::CENTER);
		dropdown_list->setScrollBarsEnabled(false);
		dropdown_list->addWidgetMovement("MenuListCancel", button.getName());

		for (int i = 0;; ++i) {
			auto str = std::string("__") + std::to_string(i);
			auto find = button.getWidgetActions().find(str);
			if (find != button.getWidgetActions().end()) {
				auto entry_name = find->second.c_str();
				auto entry = dropdown_list->addEntry(entry_name, false);
				entry->text = entry_name;
				entry->click = entry_func;
				entry->ctrlClick = entry_func;
				memcpy(&entry->data, &i, sizeof(i));
				dropdown_list->resizeForEntries();
				auto size = dropdown_list->getActualSize();
				size.h += 14;
				dropdown_list->setActualSize(size);
				if (strcmp(button.getText(), entry_name) == 0) {
					dropdown_list->setSelection(i);
				}
			}
			else {
				auto str = std::string("~__") + std::to_string(i);
				auto find = button.getWidgetActions().find(str);
				if (find != button.getWidgetActions().end()) {
					auto entry_name = find->second.c_str();
					auto entry = dropdown_list->addEntry(entry_name, false);
					entry->text = entry_name;
					entry->click = [](Frame::entry_t&){soundError();};
					entry->ctrlClick = entry->click;
					entry->color = makeColor(127, 127, 127, 255);
					dropdown_list->resizeForEntries();
					auto size = dropdown_list->getActualSize();
					size.h += 14;
					dropdown_list->setActualSize(size);
					if (strcmp(button.getText(), entry_name) == 0) {
						dropdown_list->setSelection(i);
					}
				} else {
					break;
				}
			}
		}
		dropdown_list->scrollToSelection(true);
		dropdown_list->activate();

		auto selection = dropdown_list->addImage(
			SDL_Rect{8, 0, type == DropdownType::Wide ? 624 : 158, 30},
			0xffffffff,
			"*images/ui/Main Menus/Settings/Settings_Drop_SelectBacking00.png",
			"selection"
		);

		dropdown_list->setTickCallback([](Widget& widget){
			Frame* dropdown_list = static_cast<Frame*>(&widget); assert(dropdown_list);
			auto selection = dropdown_list->findImage("selection"); assert(selection);
			bool inFrame = dropdown_list->capturesMouse() || !inputs.getVirtualMouse(0)->draw_cursor;
			if (inFrame && dropdown_list->getSelection() >= 0 && dropdown_list->getSelection() < dropdown_list->getEntries().size()) {
				selection->disabled = false;
				int entrySize = 0;
				Font* _font = Font::get(bigfont_outline);
				if (_font != nullptr) {
					entrySize = _font->height();
					entrySize += entrySize / 2;
				}
				selection->pos.y = dropdown_list->getSelection() * entrySize + 8;
			} else {
				selection->disabled = true;
			}
			});
	}

	static void settingsResolutionEntry(Frame::entry_t& entry) {
		soundActivate();
		int new_xres, new_yres, new_hz;
		sscanf(entry.name.c_str(), "%d x %d @ %dhz", &new_xres, &new_yres, &new_hz);
		allSettings.video.resolution_x = new_xres;
		allSettings.video.resolution_y = new_yres;
		allSettings.video.hz = new_hz;
		auto settings = main_menu_frame->findFrame("settings"); assert(settings);
		auto settings_subwindow = settings->findFrame("settings_subwindow"); assert(settings_subwindow);
		auto button = settings_subwindow->findButton("setting_resolution_dropdown_button"); assert(button);
		auto dropdown = settings_subwindow->findFrame("setting_resolution_dropdown"); assert(dropdown);
		button->setText(entry.name.c_str());
		dropdown->removeSelf();
		button->select();
	}

	static void settingsResolutionSmall(Button& button) {
        // wide short mode?
		settingsOpenDropdown(button, "resolution", DropdownType::Wide, settingsResolutionEntry);
	}

    static void settingsResolutionBig(Button& button) {
		settingsOpenDropdown(button, "resolution", DropdownType::Wide, settingsResolutionEntry);
	}

	static void settingsDisplayDevice(Button& button) {
		settingsOpenDropdown(button, "device", DropdownType::Short, [](Frame::entry_t& entry){
			soundActivate();
			int new_device = 0;
		    if (sscanf(entry.name.c_str(), "Display %d", &new_device) == 1) {
		        --new_device;
		    }
			allSettings.video.display_id = new_device;

			auto settings = main_menu_frame->findFrame("settings"); assert(settings);
			auto settings_subwindow = settings->findFrame("settings_subwindow"); assert(settings_subwindow);
			auto button = settings_subwindow->findButton("setting_device_dropdown_button"); assert(button);
			auto dropdown = settings_subwindow->findFrame("setting_device_dropdown"); assert(dropdown);
			button->setText(entry.name.c_str());
			dropdown->removeSelf();
			button->select();

		    std::list<resolution> resolutions;
		    getResolutionList(allSettings.video.display_id, resolutions);
		    std::vector<std::string> resolutions_formatted;
		    resolutions_formatted.reserve(resolutions.size());

		    int index;
		    std::list<resolution>::iterator it;
		    for (index = 0, it = resolutions.begin(); it != resolutions.end(); ++it, ++index) {
			    auto& res = *it;
			    char buf[32];
			    snprintf(buf, sizeof(buf), "%d x %d @ %dhz", res.x, res.y, res.hz);
			    resolutions_formatted.push_back(std::string(buf));
		    }

			auto resolution_button = settings_subwindow->findButton("setting_resolution_dropdown_button"); assert(resolution_button);
			auto& list = const_cast<std::unordered_map<std::string, std::string>&>(resolution_button->getWidgetActions());
			list.clear();
		    for (int i = 0; i < resolutions_formatted.size(); ++i) {
				resolution_button->addWidgetAction((std::string("__") + std::to_string(i)).c_str(), resolutions_formatted[i].c_str());
		    }
			});
	}

#if defined(USE_FMOD)
	struct AudioDriver {
		char name[64];
		FMOD_GUID guid;
		int system_rate;
		FMOD_SPEAKERMODE speaker_mode;
		int speaker_mode_channels;
	};
	static std::vector<AudioDriver> audio_drivers;

	static void settingsAudioDevice(Button& button) {
		settingsOpenDropdown(button, "device", DropdownType::Wide, [](Frame::entry_t& entry){
			soundActivate();

			// store driver
			unsigned int index; memcpy(&index, &entry.data, sizeof(index));
			if (index < audio_drivers.size()) {
				const auto& driver = audio_drivers[index];
				uint32_t _1; memcpy(&_1, &driver.guid.Data1, sizeof(_1));
				uint64_t _2; memcpy(&_2, &driver.guid.Data4, sizeof(_2));
				char guid_string[25];
                snprintf(guid_string, sizeof(guid_string), FMOD_AUDIO_GUID_FMT, _1, _2);
				allSettings.audio_device = guid_string;
				fmod_system->setDriver(index);
			}

			auto settings = main_menu_frame->findFrame("settings"); assert(settings);
			auto settings_subwindow = settings->findFrame("settings_subwindow"); assert(settings_subwindow);
			auto button = settings_subwindow->findButton("setting_device_dropdown_button"); assert(button);
			auto dropdown = settings_subwindow->findFrame("setting_device_dropdown"); assert(dropdown);
			button->setText(entry.name.c_str());
			dropdown->removeSelf();
			button->select();
			});
	}
#endif

	static void settingsMkbHotbarLayout(Button& button) {
		settingsOpenDropdown(button, "mkb_facehotbar", DropdownType::Short_2Slot, [](Frame::entry_t& entry) {
			soundActivate();
			if ( entry.name == "Classic" )
			{
				allSettings.mkb_facehotbar = false;
			}
			else
			{
				allSettings.mkb_facehotbar = true;
			}
			auto settings = main_menu_frame->findFrame("settings"); assert(settings);
			auto settings_subwindow = settings->findFrame("settings_subwindow"); assert(settings_subwindow);
			auto button = settings_subwindow->findButton("setting_mkb_facehotbar_dropdown_button"); assert(button);
			auto dropdown = settings_subwindow->findFrame("setting_mkb_facehotbar_dropdown"); assert(dropdown);
			button->setText(entry.name.c_str());
			dropdown->removeSelf();
			button->select();
		});
	}

	static void settingsGamepadHotbarLayout(Button& button) {
		settingsOpenDropdown(button, "gamepad_facehotbar", DropdownType::Short_2Slot, [](Frame::entry_t& entry) {
			soundActivate();
			if ( entry.name == "Classic" )
			{
				allSettings.gamepad_facehotbar = false;
			}
			else
			{
				allSettings.gamepad_facehotbar = true;
			}
			auto settings = main_menu_frame->findFrame("settings"); assert(settings);
			auto settings_subwindow = settings->findFrame("settings_subwindow"); assert(settings_subwindow);
			auto button = settings_subwindow->findButton("setting_gamepad_facehotbar_dropdown_button"); assert(button);
			auto dropdown = settings_subwindow->findFrame("setting_gamepad_facehotbar_dropdown"); assert(dropdown);
			button->setText(entry.name.c_str());
			dropdown->removeSelf();
			button->select();
		});
	}

	static void settingsWindowMode(Button& button) {
		settingsOpenDropdown(button, "window_mode", DropdownType::Short, [](Frame::entry_t& entry){
			soundActivate();
			do {
				if (entry.name == "Windowed") {
					allSettings.video.window_mode = 0;
					break;
				}
                if (entry.name == "Bordered") {
                    allSettings.video.window_mode = 0;
                    break;
                }
				if (entry.name == "Borderless") {
					allSettings.video.window_mode = 1;
					break;
				}
                if (entry.name == "Fullscreen") {
                    allSettings.video.window_mode = 2;
                    break;
                }
			} while (0);
			auto settings = main_menu_frame->findFrame("settings"); assert(settings);
			auto settings_subwindow = settings->findFrame("settings_subwindow"); assert(settings_subwindow);
			auto button = settings_subwindow->findButton("setting_window_mode_dropdown_button"); assert(button);
			auto dropdown = settings_subwindow->findFrame("setting_window_mode_dropdown"); assert(dropdown);
			button->setText(entry.name.c_str());
			dropdown->removeSelf();
			button->select();
			});
	}

	static int settingsAddSubHeader(Frame& frame, int y, const char* name, const char* text, bool generic_window = false) {
		std::string fullname = std::string("subheader_") + name;
		auto image = frame.addImage(
			SDL_Rect{0, y, frame.getSize().w, 42},
			0xffffffff,
			generic_window?
			"*images/ui/Main Menus/Settings/GenericWindow/UI_MM14_HeaderBacking00.png":
			"*images/ui/Main Menus/Settings/Settings_SubHeading_Backing00.png",
			(fullname + "_image").c_str()
		);
		auto field = frame.addField((fullname + "_field").c_str(), 128);
		field->setSize(image->pos);
		field->setFont(bigfont_outline);
		field->setText(text);
		field->setJustify(Field::justify_t::CENTER);
		Text* text_image = Text::get(field->getText(), field->getFont(),
			field->getTextColor(), field->getOutlineColor());
		int w = text_image->getWidth();
		auto fleur_left = frame.addImage(
			SDL_Rect{ (image->pos.w - w) / 2 - 26 - 8, y + 6, 26, 30 },
			0xffffffff,
			"*images/ui/Main Menus/Settings/Settings_SubHeading_Fleur00.png",
			(fullname + "_fleur_left").c_str()
		);
		auto fleur_right = frame.addImage(
			SDL_Rect{ (image->pos.w + w) / 2 + 8, y + 6, 26, 30 },
			0xffffffff,
			"*images/ui/Main Menus/Settings/Settings_SubHeading_Fleur00.png",
			(fullname + "_fleur_right").c_str()
		);
		return image->pos.h + 6;
	}

	static int settingsAddOption(
		Frame& frame,
		int y,
		const char* name,
		const char* text,
		const char* tip,
		bool _short = false
	) {
		std::string fullname = std::string("setting_") + name;
		auto image = frame.addImage(
			SDL_Rect{0, y, _short ? 278 : 382, 52},
			0xffffffff,
			_short ?
			"*images/ui/Main Menus/Settings/GenericWindow/Settings_Left_Backing_Short00.png":
			"*images/ui/Main Menus/Settings/Settings_Left_Backing00.png",
			(fullname + "_image").c_str()
		);
		auto field = frame.addField((fullname + "_field").c_str(), 128);
		auto size = image->pos; size.x += 24; size.w -= 24;
		field->setSize(size);
		field->setFont(bigfont_outline);
		field->setText(text);
		field->setHJustify(Field::justify_t::LEFT);
		field->setVJustify(Field::justify_t::CENTER);
		field->setGuide(tip);
		return size.h + 10;
	}

	static bool settingsBind(int player_index, int device_index, const char* binding, const char* input);
	static bool bind_mode = false;

	static int settingsAddBinding(
		Frame& frame,
		int y,
		int player_index,
		int device_index,
		const char* binding,
		const char* tip,
		void (*callback)(Button&))
	{
		std::string fullname = std::string("setting_") + binding;
		int result = settingsAddOption(frame, y, binding, binding, tip);
		auto button = frame.addButton((fullname + "_binding_button").c_str());
		button->setSize(SDL_Rect{
			390,
			y + 4,
			320,
			44});
		button->setFont(smallfont_outline);
		auto& bindings =
			device_index == 0 ? allSettings.bindings.kb_mouse_bindings[player_index]:
			device_index == 1 ? allSettings.bindings.gamepad_bindings[player_index]:
			allSettings.bindings.joystick_bindings[player_index];
		auto find = bindings.find(binding);
		if (find != bindings.end()) {
		    button->setText(find->second.c_str());
		    auto glyph = Input::getGlyphPathForInput(button->getText(),
                false, Input::getControllerType(player_index));
		    button->setIcon(glyph.c_str());
		} else {
			button->setText(emptyBinding);
		}
		button->setJustify(Button::justify_t::CENTER);
		button->setCallback(callback);
		button->setBackground("*images/ui/Main Menus/Settings/GenericWindow/Settings_Button_Binding00.png");
		button->setBackgroundHighlighted("*images/ui/Main Menus/Settings/GenericWindow/Settings_Button_BindingHigh00.png");
		button->setBackgroundActivated("*images/ui/Main Menus/Settings/GenericWindow/Settings_Button_BindingPress00.png");
		button->setHighlightColor(makeColor(255,255,255,255));
		button->setColor(makeColor(255,255,255,255));
		button->setTextHighlightColor(makeColor(255,255,255,255));
		button->setTextColor(makeColor(255,255,255,255));
        button->setDontSearchAncestors(true);
		button->setWidgetSearchParent(frame.getParent()->getName());
		button->setWidgetBack("discard_and_exit");
		button->addWidgetAction("MenuAlt1", "restore_defaults");
		button->addWidgetAction("MenuStart", "confirm_and_exit");
		button->setGlyphPosition(Button::glyph_position_t::CENTERED_BOTTOM);
		button->setUserData((void*)binding);

		button->setTickCallback([](Widget& widget){
			auto button = static_cast<Button*>(&widget);
			const int player = widget.getOwner();
			auto& input = Input::inputs[player];

			// Press X or Y to clear binding
			if (widget.isSelected() && !bind_mode) {
				if (inputs.hasController(player) && input.consumeBinaryToggle("MenuAlt2")) {
					auto binding = (const char*)widget.getUserData();
					(void)settingsBind(player, 1 /* Gamepad */, binding, nullptr);

					button->setText(emptyBinding);
					button->setIcon("");

					char buf[256];
					assert(main_menu_frame);
					auto bindings = main_menu_frame->findFrame("bindings"); assert(bindings);
					auto tooltip = bindings->findField("tooltip"); assert(tooltip);
					snprintf(buf, sizeof(buf), "Deleted \"%s\" binding.", binding);
					tooltip->setText(buf);
				}
			}

			// cycle icon
			const bool pressed = ticks % TICKS_PER_SECOND >= TICKS_PER_SECOND / 2;
			button->setIcon(Input::getGlyphPathForInput(button->getText(), pressed, Input::getControllerType(player)).c_str());
			});

		return result;
	}

	static int settingsAddField(
		Frame& frame,
		int y,
		const char* name,
		const char* text,
		const char* tip,
		const char* data,
		void (*callback)(Field&))
	{
	    constexpr int field_buffer_size = 32;
		std::string fullname = std::string("setting_") + name;
		int result = settingsAddOption(frame, y, name, text, tip);

		auto text_box = frame.addImage(
			SDL_Rect{390, y + 8, 246, 36},
			0xffffffff,
			"*images/ui/Main Menus/TextField_00.png",
			"text_box"
		);

		auto field = frame.addField((fullname + "_text_field").c_str(), field_buffer_size);
		field->setGlyphPosition(Widget::glyph_position_t::CENTERED_RIGHT);
		field->setSelectorOffset(SDL_Rect{-7, -7, 7, 7});
		field->setButtonsOffset(SDL_Rect{11, 0, 0, 0});
		field->setEditable(true);
		field->setScroll(true);
		field->setGuide(tip);
		field->setSize(SDL_Rect{392, y + 12, 242, 28});
		field->setFont(smallfont_outline);
		field->setText(data);
		field->setHJustify(Field::justify_t::LEFT);
		field->setVJustify(Field::justify_t::CENTER);
		field->setCallback(callback);
		field->setTickCallback([](Widget& widget){
		    auto field = static_cast<Field*>(&widget);
		    (*field->getCallback())(*field);
		    });
		field->setColor(makeColor(166, 123, 81, 255));
		field->setBackgroundColor(makeColor(52, 30, 22, 255));
		field->setBackgroundSelectAllColor(makeColor(52, 30, 22, 255));
		field->setBackgroundActivatedColor(makeColor(52, 30, 22, 255));
        field->setDontSearchAncestors(true);
		field->setWidgetSearchParent(frame.getParent()->getName());
		field->setWidgetBack("discard_and_exit");
		field->setWidgetPageLeft("tab_left");
		field->setWidgetPageRight("tab_right");
		field->addWidgetAction("MenuAlt1", "restore_defaults");
		field->addWidgetAction("MenuStart", "confirm_and_exit");
		return result;
	}

	static int settingsAddBooleanOption(
		Frame& frame,
		int y,
		const char* name,
		const char* text,
		const char* tip,
		bool on,
		void (*callback)(Button&))
	{
		std::string fullname = std::string("setting_") + name;
		int result = settingsAddOption(frame, y, name, text, tip);
		auto button = frame.addButton((fullname + "_button").c_str());
		button->setSize(SDL_Rect{
			390,
			y + 2,
			158,
			48});
		button->setFont(smallfont_outline);
		button->setText("Off          On");
		button->setJustify(Button::justify_t::CENTER);
		button->setCallback(callback);
		button->setPressed(on);
		button->setBackground("*images/ui/Main Menus/Settings/Settings_SwitchOff00.png");
		button->setBackgroundActivated("*images/ui/Main Menus/Settings/Settings_SwitchOn00.png");
		button->setStyle(Button::style_t::STYLE_TOGGLE);
		button->setHighlightColor(makeColor(255,255,255,255));
		button->setColor(makeColor(255,255,255,255));
		button->setTextHighlightColor(makeColor(255,255,255,255));
		button->setTextColor(makeColor(255,255,255,255));
        button->setDontSearchAncestors(true);
		button->setWidgetSearchParent(frame.getParent()->getName());
		button->setWidgetBack("discard_and_exit");
		button->setWidgetPageLeft("tab_left");
		button->setWidgetPageRight("tab_right");
		button->addWidgetAction("MenuAlt1", "restore_defaults");
		button->addWidgetAction("MenuStart", "confirm_and_exit");
		return result;
	}

	static int settingsAddBooleanWithCustomizeOption(
		Frame& frame,
		int y,
		const char* name,
		const char* text,
		const char* tip,
		bool on,
		void (*callback)(Button&),
		void (*customize_callback)(Button&))
	{
		std::string fullname = std::string("setting_") + name;
		int result = settingsAddBooleanOption(frame, y, name, text, tip, on, callback);
		auto button = frame.addButton((fullname + "_customize_button").c_str());
		button->setSize(SDL_Rect{
			574,
			y + 4,
			158,
			44});
		button->setFont(smallfont_outline);
		button->setText("Customize");
		button->setJustify(Button::justify_t::CENTER);
		button->setCallback(customize_callback);
		button->setBackground("*images/ui/Main Menus/Settings/Settings_Button_Customize00.png");
		button->setBackgroundHighlighted("*images/ui/Main Menus/Settings/Settings_Button_CustomizeHigh00.png");
		button->setBackgroundActivated("*images/ui/Main Menus/Settings/Settings_Button_CustomizePress00.png");
		button->setHighlightColor(makeColor(255,255,255,255));
		button->setColor(makeColor(255,255,255,255));
		button->setTextHighlightColor(makeColor(255,255,255,255));
		button->setTextColor(makeColor(255,255,255,255));
        button->setDontSearchAncestors(true);
		button->setWidgetSearchParent(frame.getParent()->getName());
		button->setWidgetLeft((fullname + "_button").c_str());
		button->setWidgetBack("discard_and_exit");
		button->setWidgetPageLeft("tab_left");
		button->setWidgetPageRight("tab_right");
		button->addWidgetAction("MenuAlt1", "restore_defaults");
		button->addWidgetAction("MenuStart", "confirm_and_exit");
		auto boolean = frame.findButton((fullname + "_button").c_str()); assert(boolean);
        boolean->setDontSearchAncestors(true);
		boolean->setWidgetSearchParent(frame.getParent()->getName());
		boolean->setWidgetRight((fullname + "_customize_button").c_str());
		boolean->setWidgetBack("discard_and_exit");
		boolean->setWidgetPageLeft("tab_left");
		boolean->setWidgetPageRight("tab_right");
		boolean->addWidgetAction("MenuAlt1", "restore_defaults");
		boolean->addWidgetAction("MenuStart", "confirm_and_exit");
		return result;
	}

	static int settingsAddCustomize(
		Frame& frame,
		int y,
		const char* name,
		const char* text,
		const char* tip,
		void (*callback)(Button&))
	{
		std::string fullname = std::string("setting_") + name;
		int result = settingsAddOption(frame, y, name, text, tip);
		auto button = frame.addButton((fullname + "_customize_button").c_str());
		button->setSize(SDL_Rect{
			390,
			y + 4,
			158,
			44});
		button->setFont(smallfont_outline);
		button->setText("Customize");
		button->setJustify(Button::justify_t::CENTER);
		button->setCallback(callback);
		button->setBackground("*images/ui/Main Menus/Settings/Settings_Button_Customize00.png");
		button->setBackgroundHighlighted("*images/ui/Main Menus/Settings/Settings_Button_CustomizeHigh00.png");
		button->setBackgroundActivated("*images/ui/Main Menus/Settings/Settings_Button_CustomizePress00.png");
		button->setHighlightColor(makeColor(255,255,255,255));
		button->setColor(makeColor(255,255,255,255));
		button->setTextHighlightColor(makeColor(255,255,255,255));
		button->setTextColor(makeColor(255,255,255,255));
        button->setDontSearchAncestors(true);
		button->setWidgetSearchParent(frame.getParent()->getName());
		button->setWidgetBack("discard_and_exit");
		button->setWidgetPageLeft("tab_left");
		button->setWidgetPageRight("tab_right");
		button->addWidgetAction("MenuAlt1", "restore_defaults");
		button->addWidgetAction("MenuStart", "confirm_and_exit");
		return result;
	}

	static int settingsAddDropdown(
		Frame& frame,
		int y,
		const char* name,
		const char* text,
		const char* tip,
		bool wide,
		const std::vector<const char*>& items,
		const char* selected,
		void (*callback)(Button&),
		const std::set<int>& grayed_items = {})
	{
		std::string fullname = std::string("setting_") + name;
		int result = settingsAddOption(frame, y, name, text, tip);
		auto button = frame.addButton((fullname + "_dropdown_button").c_str());
		button->setSize(SDL_Rect{
			390,
			y,
			wide ? 640 : 174,
			52});
		button->setFont(bigfont_outline);
		button->setText(selected);
		button->setJustify(Button::justify_t::CENTER);
		button->setCallback(callback);
		if (wide) {
			button->setBackground("*images/ui/Main Menus/Settings/Settings_WideDrop_ScrollBG00.png");
			button->setBackgroundHighlighted("*images/ui/Main Menus/Settings/Settings_WideDrop_ScrollBG00_Highlighted.png");
		} else {
			button->setBackground("*images/ui/Main Menus/Settings/Settings_Drop_ScrollBG02.png");
			button->setBackgroundHighlighted("*images/ui/Main Menus/Settings/Settings_Drop_ScrollBG02_Highlighted.png");
		}
		button->setHighlightColor(makeColor(255,255,255,255));
		button->setColor(makeColor(255,255,255,255));
		button->setTextHighlightColor(makeColor(255,255,255,255));
		button->setTextColor(makeColor(255,255,255,255));
        button->setDontSearchAncestors(true);
		button->setWidgetSearchParent(frame.getParent()->getName());
		button->setWidgetBack("discard_and_exit");
		button->setWidgetPageLeft("tab_left");
		button->setWidgetPageRight("tab_right");
		button->addWidgetAction("MenuAlt1", "restore_defaults");
		button->addWidgetAction("MenuStart", "confirm_and_exit");
		for (int i = 0; i < items.size(); ++i) {
			if (grayed_items.find(i) == grayed_items.end()) {
				button->addWidgetAction((std::string("__") + std::to_string(i)).c_str(), items[i]);
			} else {
				button->addWidgetAction((std::string("~__") + std::to_string(i)).c_str(), items[i]);
			}
		}
		return result;
	}

    static const char* sliderPercent(float v) {
        static char buf[8];
        snprintf(buf, sizeof(buf), "%d%%", (int)v);
        return buf;
    }

	static int settingsAddSlider(
		Frame& frame,
		int y,
		const char* name,
		const char* text,
		const char* tip,
		float value,
		float minValue,
		float maxValue,
		const char* (*fmt)(float value),
		void (*callback)(Slider&),
		bool _short = false)
	{
		std::string fullname = std::string("setting_") + name;
		int result = settingsAddOption(frame, y, name, text, tip, _short);
		auto box = frame.addImage(
			SDL_Rect{_short ? 298 : 402, y + 4, 132, 44},
			0xffffffff,
			"*images/ui/Main Menus/Settings/Settings_Value_Backing00.png",
			(fullname + "_box").c_str()
		);
		auto field = frame.addField((fullname + "_text").c_str(), 8);
		field->setSize(box->pos);
		field->setJustify(Field::justify_t::CENTER);
		field->setFont(smallfont_outline);
        field->setTickCallback([](Widget& widget){
            auto field = static_cast<Field*>(&widget); assert(field);
            auto frame = static_cast<Frame*>(widget.getParent());
            auto name = std::string(widget.getName());
            auto setting = name.substr(sizeof("setting_") - 1, name.size() - (sizeof("_text") - 1) - (sizeof("setting_") - 1));
            auto slider = frame->findSlider((std::string("setting_") + setting + std::string("_slider")).c_str()); assert(slider);
            auto fmt = reinterpret_cast<const char* (*)(float)>(slider->getUserData());
            if (fmt) {
                field->setText(fmt(slider->getValue()));
            } else {
                char buf[8];
                snprintf(buf, sizeof(buf), "%d", (int)slider->getValue());
                field->setText(buf);
            }
            });
		auto slider = frame.addSlider((fullname + "_slider").c_str());
		slider->setOrientation(Slider::orientation_t::SLIDER_HORIZONTAL);
		slider->setMinValue(minValue);
		slider->setMaxValue(maxValue);
		slider->setBorder(16);
		slider->setValue(value);
		slider->setRailSize(SDL_Rect{field->getSize().x + field->getSize().w + 32, y + 14, _short ? 238 : 450, 24});
		slider->setHandleSize(SDL_Rect{0, 0, 52, 42});
		slider->setCallback(callback);
		slider->setColor(makeColor(255,255,255,255));
		slider->setHighlightColor(makeColor(255,255,255,255));
		slider->setHandleImage("*images/ui/Main Menus/Settings/Settings_ValueSlider_Slide00.png");
		if (_short) {
			slider->setRailImage("*images/ui/Main Menus/Settings/GenericWindow/Settings_ValueSlider_Backing_Short00.png");
		} else {
			slider->setRailImage("*images/ui/Main Menus/Settings/Settings_ValueSlider_Backing00.png");
		}
        slider->setDontSearchAncestors(true);
		slider->setWidgetSearchParent(frame.getParent()->getName());
		slider->setWidgetBack("discard_and_exit");
		slider->setWidgetPageLeft("tab_left");
		slider->setWidgetPageRight("tab_right");
		slider->addWidgetAction("MenuAlt1", "restore_defaults");
		slider->addWidgetAction("MenuStart", "confirm_and_exit");
		slider->setGlyphPosition(Button::glyph_position_t::CENTERED);
        slider->setUserData(reinterpret_cast<void*>(fmt));
		return result;
	}

	static Frame* settingsSubwindowSetup(Button& button) {
		/*if (settings_tab_name == button.getName()) {
			return nullptr;
		}*/
		if (!video_refresh) {
		    soundActivate();
		}
		settings_tab_name = button.getName();

		assert(main_menu_frame);
		auto settings = main_menu_frame->findFrame("settings"); assert(settings);
		auto settings_subwindow = settings->findFrame("settings_subwindow");
		if (settings_subwindow) {
			settings_subwindow->removeSelf();
		}
		settings_subwindow = settings->addFrame("settings_subwindow");
		settings_subwindow->setScrollBarsEnabled(false);
		settings_subwindow->setSize(SDL_Rect{16, 71 * 2, 547 * 2, 223 * 2});
		settings_subwindow->setActualSize(SDL_Rect{0, 0, 547 * 2, 223 * 2});
		settings_subwindow->setColor(0);
		settings_subwindow->setBorder(0);
		auto rock_background = settings_subwindow->addImage(
			settings_subwindow->getActualSize(),
			makeColor(255, 255, 255, 255),
			"*images/ui/Main Menus/Settings/Settings_Window_06_BGPattern.png",
			"background"
		);
		rock_background->tiled = true;
		auto gradient_background = settings_subwindow->addImage(
			settings_subwindow->getActualSize(),
			makeColor(255, 255, 255, 255),
			"#images/ui/Main Menus/Settings/Settings_Window_06_BGGradient.png",
			"gradient_background"
		);
		auto slider = settings_subwindow->addSlider("scroll_slider");
		slider->setBorder(48);
		slider->setOrientation(Slider::SLIDER_VERTICAL);
		slider->setRailSize(SDL_Rect{1026, 0, 54, 474});
		slider->setRailImage("*images/ui/Main Menus/Settings/Settings_Slider_Backing05.png");
		slider->setHandleSize(SDL_Rect{0, 0, 34, 34});
		slider->setHandleImage("*images/ui/Main Menus/Settings/Settings_Slider_Boulder00.png");
		slider->setGlyphPosition(Button::glyph_position_t::CENTERED);
		slider->setCallback([](Slider& slider){
			Frame* frame = static_cast<Frame*>(slider.getParent());
			auto actualSize = frame->getActualSize();
			actualSize.y = slider.getValue();
			frame->setActualSize(actualSize);
			auto railSize = slider.getRailSize();
			railSize.y = actualSize.y;
			slider.setRailSize(railSize);
			slider.updateHandlePosition();
			auto gradient_background = frame->findImage("gradient_background");
			assert(gradient_background);
			gradient_background->pos.y = actualSize.y;
			});
		slider->setTickCallback([](Widget& widget){
			Slider* slider = static_cast<Slider*>(&widget);
			Frame* frame = static_cast<Frame*>(slider->getParent());
			auto actualSize = frame->getActualSize();
			slider->setValue(actualSize.y);
			auto railSize = slider->getRailSize();
			railSize.y = actualSize.y;
			slider->setRailSize(railSize);
			slider->updateHandlePosition();
			auto gradient_background = frame->findImage("gradient_background");
			assert(gradient_background);
			gradient_background->pos.y = actualSize.y;
			});
		slider->setWidgetSearchParent("settings");
		slider->setWidgetBack("discard_and_exit");
		slider->addWidgetAction("MenuStart", "confirm_and_exit");
		slider->addWidgetAction("MenuAlt1", "restore_defaults");
		slider->setWidgetPageLeft("tab_left");
		slider->setWidgetPageRight("tab_right");
		auto sliderLeft = settings_subwindow->addImage(
			SDL_Rect{0, 0, 30, 44},
			0xffffffff,
			"*images/ui/Main Menus/Settings/AutoSort/AutoSort_SliderBox_Left00.png",
			"slider_left"
		);
		sliderLeft->disabled = true;
		sliderLeft->ontop = true;
		auto sliderRight = settings_subwindow->addImage(
			SDL_Rect{0, 0, 30, 44},
			0xffffffff,
			"*images/ui/Main Menus/Settings/AutoSort/AutoSort_SliderBox_Right00.png",
			"slider_right"
		);
		sliderRight->disabled = true;
		sliderRight->ontop = true;
		return settings_subwindow;
	}

	static std::pair<std::string, std::string> getFullSettingNames(const Setting& setting) {
		switch (setting.type) {
		case Setting::Type::Boolean:
			return std::make_pair(
				std::string("setting_") + std::string(setting.name) + std::string("_button"),
				std::string(""));
		case Setting::Type::Slider:
			return std::make_pair(
				std::string("setting_") + std::string(setting.name) + std::string("_slider"),
				std::string(""));
		case Setting::Type::Customize:
			return std::make_pair(
				std::string("setting_") + std::string(setting.name) + std::string("_customize_button"),
				std::string(""));
		case Setting::Type::BooleanWithCustomize:
			return std::make_pair(
				std::string("setting_") + std::string(setting.name) + std::string("_button"),
				std::string("setting_") + std::string(setting.name) + std::string("_customize_button"));
		case Setting::Type::Dropdown:
			return std::make_pair(
				std::string("setting_") + std::string(setting.name) + std::string("_dropdown_button"),
				std::string(""));
		case Setting::Type::Binding:
			return std::make_pair(
				std::string("setting_") + std::string(setting.name) + std::string("_binding_button"),
				std::string(""));
		case Setting::Type::Field:
			return std::make_pair(
				std::string("setting_") + std::string(setting.name) + std::string("_text_field"),
				std::string(""));
		default:
			return std::make_pair(std::string(""), std::string(""));
		}
	}

	static void settingsSelect(Frame& frame, const Setting& setting) {
		auto names = getFullSettingNames(setting);
		auto widget = frame.findWidget(names.first.c_str(), false);
        if (widget) {
            widget->select();
        }
	}

	static void genericSubwindowFinalizeBasic(Frame& frame, int y) {
		auto size = frame.getActualSize();
		const int height = std::max(size.h, y);
		frame.setActualSize(SDL_Rect{0, 0, size.w, height});
		auto rock_background = frame.findImage("background"); assert(rock_background);
		rock_background->pos = frame.getActualSize();
		auto slider = frame.findSlider("scroll_slider"); assert(slider);
		slider->setValue(0.f);
		slider->setMinValue(0.f);
		slider->setMaxValue(height - size.h);
	}

	static void settingsSubwindowFinalize(Frame& frame, int y, const Setting& setting) {
		genericSubwindowFinalizeBasic(frame, y);
		auto names = getFullSettingNames(setting);
		auto slider = frame.findSlider("scroll_slider"); assert(slider);
		slider->setWidgetLeft(names.first.c_str());

		// rescues focus if it is lost somehow
		static std::string rescueSetting;
		rescueSetting = names.first;
		frame.setTickCallback([](Widget& widget){
			assert(main_menu_frame);
			auto selectedWidget = main_menu_frame->findSelectedWidget(getMenuOwner());
			if (!selectedWidget) {
				auto rescue = widget.findWidget(rescueSetting.c_str(), true);
				if (rescue) {
					rescue->select();
				}
			}
			auto frame = static_cast<Frame*>(&widget);
			updateSettingSelection(*frame);
			updateSliderArrows(*frame);
			});
	}

	static void hookSettingToSetting(Frame& frame, const Setting& setting1, const Setting& setting2) {
		auto names1 = getFullSettingNames(setting1);
		auto names2 = getFullSettingNames(setting2);
		auto widget11 = frame.findWidget(names1.first.c_str(), false); assert(widget11);
		auto widget12 = names1.second.empty() ? nullptr : frame.findWidget(names1.second.c_str(), false);
		auto widget21 = frame.findWidget(names2.first.c_str(), false); assert(widget21);
		auto widget22 = names2.second.empty() ? nullptr : frame.findWidget(names2.second.c_str(), false);
		widget11->setWidgetDown(names2.first.c_str());
		widget21->setWidgetUp(names1.first.c_str());
		if (widget12) {
			if (widget22) {
				widget12->setWidgetDown(names2.second.c_str());
				widget22->setWidgetUp(names1.second.c_str());
			} else {
				widget12->setWidgetDown(names2.first.c_str());
			}
		} else {
			if (widget22) {
				widget22->setWidgetUp(names1.first.c_str());
			}
		}
	}

	static void hookSettings(Frame& frame, const std::vector<Setting>& settings) {
		for (auto it = settings.begin(); std::next(it) != settings.end(); ++it) {
			auto& setting1 = (*it);
			auto& setting2 = (*std::next(it));
			hookSettingToSetting(frame, setting1, setting2);
		}
	}

	static Frame* settingsGenericWindow(
		const char* name,
		const char* title,
		void (*defaults_callback)(Button&),
		void (*discard_callback)(Button&),
		void (*confirm_callback)(Button&))
	{
		auto window = genericWindow(name, title, true);

		auto defaults = window->addButton("restore_defaults");
		defaults->setBackground("*images/ui/Main Menus/Settings/GenericWindow/UI_MM14_ButtonStandard00.png");
		defaults->setBackgroundHighlighted("*images/ui/Main Menus/Settings/GenericWindow/UI_MM14_ButtonStandardHigh00.png");
		defaults->setBackgroundActivated("*images/ui/Main Menus/Settings/GenericWindow/UI_MM14_ButtonStandardPress00.png");
		defaults->setColor(makeColor(255, 255, 255, 255));
		defaults->setHighlightColor(makeColor(255, 255, 255, 255));
		defaults->setTextColor(makeColor(255, 255, 255, 255));
		defaults->setTextHighlightColor(makeColor(255, 255, 255, 255));
		defaults->setSize(SDL_Rect{156, 630, 164, 62});
		defaults->setText("Restore\nDefaults");
		defaults->setFont(smallfont_outline);
		defaults->setWidgetSearchParent(name);
		defaults->setWidgetBack("discard_and_exit");
		defaults->addWidgetAction("MenuStart", "confirm_and_exit");
		defaults->addWidgetAction("MenuAlt1", "restore_defaults");
		defaults->setWidgetRight("discard_and_exit");
		defaults->setCallback(defaults_callback);
		defaults->setHideKeyboardGlyphs(false);

		auto discard = window->addButton("discard_and_exit");
		discard->setBackground("*images/ui/Main Menus/Settings/GenericWindow/UI_MM14_ButtonStandard00.png");
		discard->setBackgroundHighlighted("*images/ui/Main Menus/Settings/GenericWindow/UI_MM14_ButtonStandardHigh00.png");
		discard->setBackgroundActivated("*images/ui/Main Menus/Settings/GenericWindow/UI_MM14_ButtonStandardPress00.png");
		discard->setColor(makeColor(255, 255, 255, 255));
		discard->setHighlightColor(makeColor(255, 255, 255, 255));
		discard->setTextColor(makeColor(255, 255, 255, 255));
		discard->setTextHighlightColor(makeColor(255, 255, 255, 255));
		discard->setText("Discard\n& Exit");
		discard->setFont(smallfont_outline);
		discard->setSize(SDL_Rect{
			(window->getActualSize().w - 164) / 2,
			630,
			164,
			62}
		);
		discard->setCallback(discard_callback);
		discard->setWidgetSearchParent(name);
		discard->setWidgetBack("discard_and_exit");
		discard->addWidgetAction("MenuStart", "confirm_and_exit");
		discard->addWidgetAction("MenuAlt1", "restore_defaults");
		discard->setWidgetLeft("restore_defaults");
		discard->setWidgetRight("confirm_and_exit");
		discard->setHideKeyboardGlyphs(false);

		auto confirm = window->addButton("confirm_and_exit");
		confirm->setBackground("*images/ui/Main Menus/Settings/GenericWindow/UI_MM14_ButtonStandard00.png");
		confirm->setBackgroundHighlighted("*images/ui/Main Menus/Settings/GenericWindow/UI_MM14_ButtonStandardHigh00.png");
		confirm->setBackgroundActivated("*images/ui/Main Menus/Settings/GenericWindow/UI_MM14_ButtonStandardPress00.png");
		confirm->setColor(makeColor(255, 255, 255, 255));
		confirm->setHighlightColor(makeColor(255, 255, 255, 255));
		confirm->setTextColor(makeColor(255, 255, 255, 255));
		confirm->setTextHighlightColor(makeColor(255, 255, 255, 255));
		confirm->setText("Confirm\n& Exit");
		confirm->setFont(smallfont_outline);
		confirm->setSize(SDL_Rect{504, 630, 164, 62});
		confirm->setCallback(confirm_callback);
		confirm->setWidgetSearchParent(name);
		confirm->setWidgetBack("discard_and_exit");
		confirm->addWidgetAction("MenuStart", "confirm_and_exit");
		confirm->addWidgetAction("MenuAlt1", "restore_defaults");
		confirm->setWidgetLeft("discard_and_exit");
		confirm->setHideKeyboardGlyphs(false);
		confirm->select();

		return window;
	}

	static void settingsMinimap(Button& button) {
		soundActivate();
		auto window = settingsGenericWindow("minimap", "MINIMAP",
			[](Button& button){ // restore defaults
				auto parent = static_cast<Frame*>(button.getParent()); assert(parent);
				auto parent_background = static_cast<Frame*>(parent->getParent()); assert(parent_background);
				parent_background->removeSelf();
				allSettings.minimap = Minimap::reset();
				settingsMinimap(button);
			},
			[](Button& button){ // discard & exit
				soundCancel();
				auto parent = static_cast<Frame*>(button.getParent()); assert(parent);
				auto parent_background = static_cast<Frame*>(parent->getParent()); assert(parent_background);
				parent_background->removeSelf();
				allSettings.minimap.load();
			    auto settings = main_menu_frame->findFrame("settings"); assert(settings);
			    auto settings_subwindow = settings->findFrame("settings_subwindow"); assert(settings_subwindow);
			    auto previous = settings_subwindow->findButton("setting_minimap_settings_customize_button"); assert(previous);
			    previous->select();
			},
			[](Button& button){ // confirm & exit
				soundActivate();
				auto parent = static_cast<Frame*>(button.getParent()); assert(parent);
				auto parent_background = static_cast<Frame*>(parent->getParent()); assert(parent_background);
				parent_background->removeSelf();
				allSettings.minimap.save();
			    auto settings = main_menu_frame->findFrame("settings"); assert(settings);
			    auto settings_subwindow = settings->findFrame("settings_subwindow"); assert(settings_subwindow);
			    auto previous = settings_subwindow->findButton("setting_minimap_settings_customize_button"); assert(previous);
			    previous->select();
			});
		assert(window);
		auto subwindow = window->findFrame("subwindow"); assert(subwindow);
		int y = 0;

		y += settingsAddSubHeader(*subwindow, y, "scale_header", "Scale", true);

		/*y += settingsAddSlider(*subwindow, y, "map_scale", "Map scale",
			"Scale the map to be larger or smaller.",
            allSettings.minimap.map_scale, 25, 100, sliderPercent,
			[](Slider& slider){ allSettings.minimap.map_scale = slider.getValue(); }, true);*/

		y += settingsAddSlider(*subwindow, y, "icon_scale", "Icon scale",
			"Scale the size of icons on the map (such as players and allies)",
			allSettings.minimap.icon_scale, 100, 200, sliderPercent,
			[](Slider& slider){ allSettings.minimap.icon_scale = slider.getValue(); }, true);

		y += settingsAddSubHeader(*subwindow, y, "transparency_header", "Transparency", true);

		y += settingsAddSlider(*subwindow, y, "foreground_opacity", "Foreground opacity",
			"Set the opacity of the minimap's foreground.",
			allSettings.minimap.foreground_opacity, 0, 100, sliderPercent,
			[](Slider& slider){ allSettings.minimap.foreground_opacity = slider.getValue(); }, true);

		y += settingsAddSlider(*subwindow, y, "background_opacity", "Background opacity",
			"Set the opacity of the minimap's background.",
			allSettings.minimap.background_opacity, 0, 100, sliderPercent,
			[](Slider& slider){ allSettings.minimap.background_opacity = slider.getValue(); }, true);

		hookSettings(*subwindow,
			{/*{Setting::Type::Slider, "map_scale"},*/
			{Setting::Type::Slider, "icon_scale"},
			{Setting::Type::Slider, "foreground_opacity"},
			{Setting::Type::Slider, "background_opacity"},
			});
		settingsSubwindowFinalize(*subwindow, y, {Setting::Type::Slider, "icon_scale"});
		settingsSelect(*subwindow, {Setting::Type::Slider, "icon_scale"});
	}

	static void settingsMessages(Button& button) {
		soundActivate();
		auto window = settingsGenericWindow("messages", "MESSAGES",
			[](Button& button){ // restore defaults
				auto parent = static_cast<Frame*>(button.getParent()); assert(parent);
				auto parent_background = static_cast<Frame*>(parent->getParent()); assert(parent_background);
				parent_background->removeSelf();
				allSettings.show_messages = Messages::reset();
				settingsMessages(button);
			},
			[](Button& button){ // discard & exit
				soundCancel();
				auto parent = static_cast<Frame*>(button.getParent()); assert(parent);
				auto parent_background = static_cast<Frame*>(parent->getParent()); assert(parent_background);
				parent_background->removeSelf();
				allSettings.show_messages.load();
			    auto settings = main_menu_frame->findFrame("settings"); assert(settings);
			    auto settings_subwindow = settings->findFrame("settings_subwindow"); assert(settings_subwindow);
			    auto previous = settings_subwindow->findButton("setting_show_messages_customize_button"); assert(previous);
			    previous->select();
			},
			[](Button& button){ // confirm & exit
				soundActivate();
				auto parent = static_cast<Frame*>(button.getParent()); assert(parent);
				auto parent_background = static_cast<Frame*>(parent->getParent()); assert(parent_background);
				parent_background->removeSelf();
				allSettings.show_messages.save();
			    auto settings = main_menu_frame->findFrame("settings"); assert(settings);
			    auto settings_subwindow = settings->findFrame("settings_subwindow"); assert(settings_subwindow);
			    auto previous = settings_subwindow->findButton("setting_show_messages_customize_button"); assert(previous);
			    previous->select();
			});
		assert(window);
		auto subwindow = window->findFrame("subwindow"); assert(subwindow);
		int y = 0;

		y += settingsAddSubHeader(*subwindow, y, "categories_header", "Categories", true);

		y += settingsAddBooleanOption(*subwindow, y, "messages_combat", "Combat messages",
			"Enable report of damage received or given in combat.",
			allSettings.show_messages.combat, [](Button& button) {soundToggle(); allSettings.show_messages.combat = button.isPressed(); });

		y += settingsAddBooleanOption(*subwindow, y, "messages_status", "Status messages",
			"Enable report of character status changes and other passive effects.",
			allSettings.show_messages.status, [](Button& button){soundToggle(); allSettings.show_messages.status = button.isPressed();});

		y += settingsAddBooleanOption(*subwindow, y, "messages_inventory", "Inventory messages",
			"Enable report of inventory and item appraisal messages.",
			allSettings.show_messages.inventory, [](Button& button){soundToggle(); allSettings.show_messages.inventory = button.isPressed();});

		y += settingsAddBooleanOption(*subwindow, y, "messages_equipment", "Equipment messages",
			"Enable report of player equipment changes.",
			allSettings.show_messages.equipment, [](Button& button){soundToggle(); allSettings.show_messages.equipment = button.isPressed();});

		y += settingsAddBooleanOption(*subwindow, y, "messages_world", "World messages",
			"Enable report of diegetic messages, such as speech and text.",
			allSettings.show_messages.world, [](Button& button){soundToggle(); allSettings.show_messages.world = button.isPressed();});

		y += settingsAddBooleanOption(*subwindow, y, "messages_chat", "Player chat",
			"Enable multiplayer chat.",
			allSettings.show_messages.chat, [](Button& button){soundToggle(); allSettings.show_messages.chat = button.isPressed();});

		y += settingsAddBooleanOption(*subwindow, y, "messages_progression", "Progression messages",
			"Enable report of player character progression messages (ie level-ups).",
			allSettings.show_messages.progression, [](Button& button){soundToggle(); allSettings.show_messages.progression = button.isPressed();});

		y += settingsAddBooleanOption(*subwindow, y, "messages_interaction", "Interaction messages",
			"Enable report of player interactions with the world.",
			allSettings.show_messages.interaction, [](Button& button){soundToggle(); allSettings.show_messages.interaction = button.isPressed();});

		y += settingsAddBooleanOption(*subwindow, y, "messages_inspection", "Inspection messages",
			"Enable player inspections of world objects.",
			allSettings.show_messages.inspection, [](Button& button){soundToggle(); allSettings.show_messages.inspection = button.isPressed();});

		y += settingsAddBooleanOption(*subwindow, y, "messages_hint", "Hint messages",
			"Enable cryptic hints for certain items, world events, etc.",
			allSettings.show_messages.hint, [](Button& button){soundToggle(); allSettings.show_messages.hint = button.isPressed();});

		y += settingsAddBooleanOption(*subwindow, y, "messages_obituary", "Obituary messages",
			"Enable obituary messages for player deaths.",
			allSettings.show_messages.obituary, [](Button& button){soundToggle(); allSettings.show_messages.obituary = button.isPressed();});

		hookSettings(*subwindow,
			{{Setting::Type::Boolean, "messages_combat"},
			{Setting::Type::Boolean, "messages_status"},
			{Setting::Type::Boolean, "messages_inventory"},
			{Setting::Type::Boolean, "messages_equipment"},
			{Setting::Type::Boolean, "messages_world"},
			{Setting::Type::Boolean, "messages_chat"},
			{Setting::Type::Boolean, "messages_progression"},
			{Setting::Type::Boolean, "messages_interaction"},
			{Setting::Type::Boolean, "messages_inspection"},
			{Setting::Type::Boolean, "messages_hint"},
			{Setting::Type::Boolean, "messages_obituary"},
			});
		settingsSubwindowFinalize(*subwindow, y, {Setting::Type::Boolean, "messages_combat"});
		settingsSelect(*subwindow, {Setting::Type::Boolean, "messages_combat"});
	}

	static int getDeviceIndexForName(const char* name) {
		if (strcmp(name, "KB & Mouse") == 0) { return 0; }
		if (strcmp(name, "Gamepad") == 0) { return 1; }
		if (strcmp(name, "Joystick") == 0) { return 2; }
		else { return -1; }
	}

	static bool settingsBind(int player_index, int device_index, const char* binding, const char* input) {
		assert(binding);
		auto& bindings =
			device_index == 0 ? allSettings.bindings.kb_mouse_bindings[player_index]:
			device_index >= 1 && device_index <= 4 ? allSettings.bindings.gamepad_bindings[player_index]:
			allSettings.bindings.joystick_bindings[player_index];
		if (input == nullptr) {
			bindings.erase(binding);
			return true;
		} else {
			std::string input_to_store;
			if (device_index >= 1 && device_index <= 4 && strncmp(input, "Pad", 3) == 0) {
				input_to_store = input + 4;
			}
			else if (device_index >= 5 && device_index <= 8 && strncmp(input, "Joy", 3) == 0) {
				input_to_store = input + 4;
			}
			else if (device_index == 0 && strncmp(input, "Pad", 3) && strncmp(input, "Joy", 3)) {
				input_to_store = input;
			}
			if (input_to_store.empty()) {
				return false;
			} else {
				auto find = bindings.find(binding);
				if (find == bindings.end()) {
					bindings.insert(std::make_pair(binding, input_to_store.c_str()));
				} else {
					find->second = input_to_store.c_str();
				}
				return true;
			}
		}
	}

	static void settingsBindings(int player_index, int device_index, const char* profile, Setting setting_to_select) {
		soundActivate();

		static Button* bound_button;
		static std::string bound_binding;
		static std::string bound_input;
		static int bound_player;
		static int bound_device;
        static const char* bound_profile;

		bind_mode = false;
		bound_button = nullptr;
		bound_binding = "";
		bound_input = "";
		bound_player = player_index;
		bound_device = device_index;
        bound_profile = profile;

		auto window = settingsGenericWindow("bindings", "BINDINGS",
			[](Button& button){ // restore defaults
				auto parent = static_cast<Frame*>(button.getParent()); assert(parent);
				auto parent_background = static_cast<Frame*>(parent->getParent()); assert(parent_background);
				parent_background->removeSelf();
				allSettings.bindings = Bindings::reset(defaultControlLayout);
				const int player = multiplayer == CLIENT ? 0 : getMenuOwner();
				settingsBindings(player, inputs.hasController(player) ? 1 : 0, defaultControlLayout,
				    {Setting::Type::Dropdown, "player_dropdown_button"});
			},
			[](Button& button){ // discard & exit
				soundCancel();
				auto parent = static_cast<Frame*>(button.getParent()); assert(parent);
				auto parent_background = static_cast<Frame*>(parent->getParent()); assert(parent_background);
				parent_background->removeSelf();
				allSettings.bindings.load();
			    auto settings = main_menu_frame->findFrame("settings"); assert(settings);
			    auto settings_subwindow = settings->findFrame("settings_subwindow"); assert(settings_subwindow);
			    auto previous = settings_subwindow->findButton("setting_bindings_customize_button"); assert(previous);
			    previous->select();
			},
			[](Button& button){ // confirm & exit
				soundActivate();
				auto parent = static_cast<Frame*>(button.getParent()); assert(parent);
				auto parent_background = static_cast<Frame*>(parent->getParent()); assert(parent_background);
				parent_background->removeSelf();
				allSettings.bindings.save();
			    auto settings = main_menu_frame->findFrame("settings"); assert(settings);
			    auto settings_subwindow = settings->findFrame("settings_subwindow"); assert(settings_subwindow);
			    auto previous = settings_subwindow->findButton("setting_bindings_customize_button"); assert(previous);
			    previous->select();
			});
		assert(window);
		auto subwindow = window->findFrame("subwindow"); assert(subwindow);

		std::vector<Setting> bindings;
		bindings.reserve(getBindings(profile).size());
		for (auto& binding : getBindings(profile)) {
            const std::string& str = *(&binding.keyboard + device_index);
            if (str != hiddenBinding) {
		        bindings.push_back({Setting::Type::Binding, binding.action.c_str()});
		    }
		}

		int y = 0;
		y += settingsAddSubHeader(*subwindow, y, "bindings_header", "Profiles", true);
        
        static std::vector<std::string> players;
        static std::vector<const char*> player_ptrs;
        if (players.empty()) {
            players.reserve(MAX_SPLITSCREEN);
            for (int c = 0; c < MAX_SPLITSCREEN; ++c) {
                std::string str = "Player ";
                str += std::to_string(c + 1);
                players.emplace_back(str);
                player_ptrs.emplace_back(players.back().c_str());
            }
        }

		std::string player_str = "Player " + std::to_string(player_index + 1);
		y += settingsAddDropdown(*subwindow, y, "player_dropdown_button", "Player",
			"Select the player whose controls you wish to customize.", false,
            player_ptrs, player_str.c_str(),
			[](Button& button){
				soundActivate();
				settingsOpenDropdown(button, "player_dropdown", DropdownType::Short,
					[](Frame::entry_t& entry){
						soundActivate();
						auto parent = main_menu_frame->findFrame("bindings");
						auto parent_background = static_cast<Frame*>(parent->getParent()); assert(parent_background);
						parent_background->removeSelf();
						int player_index = (int)(entry.name.back() - '1');
						settingsBindings(player_index, bound_device, getMatchingProfileName(player_index, bound_device == 1),
						    {Setting::Type::Dropdown, "player_dropdown_button"});
					});
			});

		const std::vector<const char*> devices = {
		    "KB & Mouse",
		    "Gamepad",
		    //"Joystick", // Maybe for the future.
		};

#ifndef NINTENDO
		y += settingsAddDropdown(*subwindow, y, "device_dropdown_button", "Device",
			"Select a controller for the given player.", false, devices, devices[device_index],
			[](Button& button){
				soundActivate();
				settingsOpenDropdown(button, "device_dropdown", DropdownType::Short,
					[](Frame::entry_t& entry){
						soundActivate();
						auto parent = main_menu_frame->findFrame("bindings");
						auto parent_background = static_cast<Frame*>(parent->getParent()); assert(parent_background);
						parent_background->removeSelf();
						int device_index = getDeviceIndexForName(entry.text.c_str());
						settingsBindings(bound_player, device_index, getMatchingProfileName(bound_player, device_index == 1),
						    {Setting::Type::Dropdown, "device_dropdown_button"});
					});
			});
#endif
        
        std::vector<const char*> layouts;
        for (auto& layout : defaultBindings) {
            layouts.emplace_back(layout.name.c_str());
        }
        
        y += settingsAddDropdown(*subwindow, y, "profile_dropdown_button", "Profile",
            "Select a predefined binding layout for the given player.", false, layouts, profile,
            [](Button& button){
                soundActivate();
                settingsOpenDropdown(button, "profile_dropdown", DropdownType::Short,
                    [](Frame::entry_t& entry){
                        soundActivate();
                        auto parent = main_menu_frame->findFrame("bindings");
                        auto parent_background = static_cast<Frame*>(parent->getParent()); assert(parent_background);
                        parent_background->removeSelf();
                        const char* profile = entry.text.c_str();
                        allSettings.bindings.kb_mouse_bindings[bound_player].clear();
                        allSettings.bindings.gamepad_bindings[bound_player].clear();
                        allSettings.bindings.joystick_bindings[bound_player].clear();
                        for (auto& binding : getBindings(profile)) {
                            allSettings.bindings.kb_mouse_bindings[bound_player].emplace(binding.action, binding.keyboard);
                            allSettings.bindings.gamepad_bindings[bound_player].emplace(binding.action, binding.gamepad);
                            allSettings.bindings.joystick_bindings[bound_player].emplace(binding.action, binding.joystick);
                        }
                        settingsBindings(bound_player, bound_device, profile,
                            {Setting::Type::Dropdown, "profile_dropdown_button"});
                    });
            });

		y += settingsAddSubHeader(*subwindow, y, "bindings_header", "Bindings", true);

		for (auto& binding : bindings) {
			char tip[256];
			if (inputs.hasController(getMenuOwner())) {
#ifdef NINTENDO
				snprintf(tip, sizeof(tip), "Bind a button to %s,\nor press X to delete the current binding", binding.name);
#else
				snprintf(tip, sizeof(tip), "Bind a button to %s,\nor press Y to delete the current binding", binding.name);
#endif
			} else {
				snprintf(tip, sizeof(tip), "Bind an input device to %s", binding.name);
			}
			y += settingsAddBinding(*subwindow, y, player_index, device_index, binding.name, tip,
				[](Button& button){
					soundToggle();
					auto name = std::string(button.getName());
					bind_mode = true;
					bound_button = &button;
					bound_input = button.getText();
					bound_binding = name.substr(sizeof("setting_") - 1, name.size() - (sizeof("_binding_button") - 1) - (sizeof("setting_") - 1));
					button.setText(". . .");
					auto subwindow = static_cast<Frame*>(button.getParent()); assert(subwindow);
					auto settings = static_cast<Frame*>(subwindow->getParent()); assert(settings);
					auto tooltip = settings->findField("tooltip"); assert(tooltip);
					char buf[256];
					
					if (inputs.hasController(getMenuOwner())) {
						snprintf(buf, sizeof(buf),
							"Binding \"%s\".\n"
							"The next button you press will be bound to this action.",
							bound_binding.c_str());
					} else {
						snprintf(buf, sizeof(buf),
							"Binding \"%s\". Press ESC to cancel or DEL to delete the binding.\n"
							"The next input you activate will be bound to this action.",
							bound_binding.c_str());
					}

					tooltip->setText(buf);
					Input::lastInputOfAnyKind = "";
					for (int c = 0; c < MAXPLAYERS; ++c) {
						Input::inputs[c].setDisabled(true);
					}
					for (auto button : subwindow->getButtons()) {
						button->setDisabled(true);
					}

					auto bindings = main_menu_frame->findFrame("bindings"); assert(bindings);
					auto confirm = bindings->findButton("confirm_and_exit"); assert(confirm);
					auto discard = bindings->findButton("discard_and_exit"); assert(discard);
					auto defaults = bindings->findButton("restore_defaults"); assert(defaults);
					confirm->setDisabled(true);
					discard->setDisabled(true);
					defaults->setDisabled(true);
				});
		}

		window->setTickCallback([](Widget& widget){
			if (bind_mode) {
				if (bound_button && !Input::lastInputOfAnyKind.empty()) {
					auto bindings = main_menu_frame->findFrame("bindings"); assert(bindings);
					auto tooltip = bindings->findField("tooltip"); assert(tooltip);
					if (Input::lastInputOfAnyKind == "Escape") {
						bound_button->setText(bound_input.c_str());
		                auto glyph = Input::getGlyphPathForInput(bound_button->getText(), false, Input::getControllerType(bound_player));
		                bound_button->setIcon(glyph.c_str());
						char buf[256];
						snprintf(buf, sizeof(buf), "Cancelled rebinding \"%s\"", bound_binding.c_str());
						tooltip->setText(buf);
					} else if (Input::lastInputOfAnyKind == "Delete") {
						(void)settingsBind(bound_player, bound_device, bound_binding.c_str(), nullptr);
						bound_button->setText(emptyBinding);
		                bound_button->setIcon("");
						char buf[256];
						snprintf(buf, sizeof(buf), "Deleted \"%s\" binding.", bound_binding.c_str());
						tooltip->setText(buf);
					} else {
						bool result = settingsBind(bound_player, bound_device, bound_binding.c_str(), Input::lastInputOfAnyKind.c_str());
						if (!result) {
							goto bind_failed;
						}
						auto begin = Input::lastInputOfAnyKind.substr(0, 3);
						std::string newinput = begin == "Pad" || begin == "Joy" ?
								Input::lastInputOfAnyKind.substr(4) : Input::lastInputOfAnyKind;
						bound_button->setText(newinput.c_str());
		                auto glyph = Input::getGlyphPathForInput(bound_button->getText(), false, Input::getControllerType(bound_player));
		                bound_button->setIcon(glyph.c_str());
						char buf[256];
						snprintf(buf, sizeof(buf), "Bound \"%s\" to \"%s\"", bound_binding.c_str(), newinput.c_str());
						tooltip->setText(buf);
					}
					bound_button = nullptr;
bind_failed:
                    // fix a bug where this wasn't always cleared...
                    mousestatus[SDL_BUTTON_LEFT] = 0;
#ifdef NINTENDO
					if ( Input::lastInputOfAnyKind.size() >= 4 )
					{
						if (Input::lastInputOfAnyKind.substr(4) == "ButtonY") {
							Input::inputs[widget.getOwner()].consumeBinary("MenuAlt1");
						}
						if (Input::lastInputOfAnyKind.substr(4) == "ButtonX") {
							Input::inputs[widget.getOwner()].consumeBinary("MenuAlt2");
						}
					}
#else
					if ( Input::lastInputOfAnyKind.size() >= 4 )
					{
						if (Input::lastInputOfAnyKind.substr(4) == "ButtonX") {
							Input::inputs[widget.getOwner()].consumeBinary("MenuAlt1");
						}
						if (Input::lastInputOfAnyKind.substr(4) == "ButtonY") {
							Input::inputs[widget.getOwner()].consumeBinary("MenuAlt2");
						}
					}
#endif
				}
				else if (!bound_button) {
					auto bindings = main_menu_frame->findFrame("bindings"); assert(bindings);
					auto confirm = bindings->findButton("confirm_and_exit"); assert(confirm);
					auto discard = bindings->findButton("discard_and_exit"); assert(discard);
					auto defaults = bindings->findButton("restore_defaults"); assert(defaults);
					confirm->setDisabled(false);
					discard->setDisabled(false);
					defaults->setDisabled(false);

					auto subwindow = bindings->findFrame("subwindow"); assert(subwindow);
					for (auto button : subwindow->getButtons()) {
						button->setDisabled(false);
					}

					for (int c = 0; c < MAXPLAYERS; ++c) {
						Input::inputs[c].setDisabled(false);
					}
					bound_binding = "";
					bind_mode = false;
				}
			}
			});

		hookSettings(*subwindow,
			{{Setting::Type::Dropdown, "player_dropdown_button"},
#ifndef NINTENDO
			{Setting::Type::Dropdown, "device_dropdown_button"},
#endif
            {Setting::Type::Dropdown, "profile_dropdown_button"},
			bindings[0],
			});
		hookSettings(*subwindow, bindings);
		settingsSubwindowFinalize(*subwindow, y, setting_to_select);
		settingsSelect(*subwindow, setting_to_select);
	}

	static const std::vector<const char*> crosshairs =
	{ 
		"Dot",
		"Dot (Large)",
		"Plus (Small)",
		"Plus (Medium)",
		"Plus (Large)",
		"Cross",
		"Carat",
		"Circle",
		"Dots (3x)",
		"Dots (4x)",
		":)",
		"@" };
	static void settingsCrosshairType(Button& button) {
		settingsOpenDropdown(button, "shootmode_crosshair", DropdownType::Short, [](Frame::entry_t& entry) {
			soundActivate();

		int index = 0;
		for ( auto str : crosshairs )
		{
			if ( entry.name == str )
			{
				allSettings.shootmode_crosshair = index;
				break;
			}
			++index;
		}
		auto settings = main_menu_frame->findFrame("settings"); assert(settings);
		auto settings_subwindow = settings->findFrame("settings_subwindow"); assert(settings_subwindow);
		auto button = settings_subwindow->findButton("setting_shootmode_crosshair_dropdown_button"); assert(button);
		auto dropdown = settings_subwindow->findFrame("setting_shootmode_crosshair_dropdown"); assert(dropdown);
		button->setText(entry.name.c_str());
		dropdown->removeSelf();
		button->select();
			});
	}

	static void settingsGeneral(Button& button) {
		Frame* settings_subwindow;
		if ((settings_subwindow = settingsSubwindowSetup(button)) == nullptr) {
			auto settings = main_menu_frame->findFrame("settings"); assert(settings);
			auto settings_subwindow = settings->findFrame("settings_subwindow"); assert(settings_subwindow);
			settingsSelect(*settings_subwindow, {Setting::Type::Boolean, "fast_restart"});
			return;
		}
		int y = 0;

		y += settingsAddSubHeader(*settings_subwindow, y, "general", "General Options");
		y += settingsAddBooleanOption(*settings_subwindow, y, "fast_restart", "Instant Restart on Gameover",
			"Automatically restarts the game quickly after dying.",
			allSettings.fast_restart, [](Button& button){soundToggle(); allSettings.fast_restart = button.isPressed();});

		y += settingsAddSubHeader(*settings_subwindow, y, "inventory", "Inventory Options");
		y += settingsAddBooleanOption(*settings_subwindow, y, "add_items_to_hotbar", "Add Items to Hotbar",
			"Automatically fill the hotbar with recently collected items.",
			allSettings.add_items_to_hotbar_enabled, [](Button& button){soundToggle(); allSettings.add_items_to_hotbar_enabled = button.isPressed();});
		y += settingsAddCustomize(*settings_subwindow, y, "inventory_sorting", "Inventory Sorting",
			"Customize the way items are automatically sorted in your inventory.",
			[](Button& button){allSettings.inventory_sorting = InventorySorting::load(); settingsCustomizeInventorySorting(button);});
#ifndef NINTENDO
		y += settingsAddBooleanOption(*settings_subwindow, y, "use_on_release", "Use on Release",
			"Activate an item as soon as the Use key is released in the inventory window.",
			allSettings.use_on_release_enabled, [](Button& button){soundToggle(); allSettings.use_on_release_enabled = button.isPressed();});
#endif

		y += settingsAddSubHeader(*settings_subwindow, y, "hud", "HUD Options");
#ifndef NINTENDO
        y += settingsAddSlider(*settings_subwindow, y, "ui_scale", "HUD Scaling",
            "Scale the UI to a larger or smaller size. (Recommended values: 50%, 75%, or 100%)",
            allSettings.ui_scale, 50.f, 100.f, sliderPercent,
            [](Slider& slider){soundSlider(true); allSettings.ui_scale = floorf(slider.getValue());});
#endif
		y += settingsAddSlider(*settings_subwindow, y, "enemybar_scale", "Enemy Health Bar Scaling",
			"Control size of in-world popups for enemy health bars.",
			allSettings.enemybar_scale, 50, 100, sliderPercent, [](Slider& slider) {soundSlider(true); allSettings.enemybar_scale = slider.getValue(); });
		y += settingsAddSlider(*settings_subwindow, y, "world_tooltip_scale", "Popup Scaling",
			"Control size of in-world popups for items, gravestones and NPC dialogue.",
			allSettings.world_tooltip_scale, 100, 200, sliderPercent, [](Slider& slider) {soundSlider(true); allSettings.world_tooltip_scale = slider.getValue(); });
		y += settingsAddSlider(*settings_subwindow, y, "world_tooltip_scale_splitscreen", "Popup Scaling (Splitscreen)",
			"Control size of in-world popups for items, gravestones and NPC dialogue in splitscreen.",
			allSettings.world_tooltip_scale_splitscreen, 100, 200, sliderPercent, [](Slider& slider) {soundSlider(true); allSettings.world_tooltip_scale_splitscreen = slider.getValue(); });
		y += settingsAddSlider(*settings_subwindow, y, "item_tooltip_height", "Item Tooltip Height",
			"Adjust the vertical position of in-world item tooltip popups.",
			allSettings.item_tooltip_height, 50, 100, sliderPercent, [
			](Slider& slider) {soundSlider(true); allSettings.item_tooltip_height = slider.getValue(); });
		y += settingsAddSlider(*settings_subwindow, y, "shootmode_crosshair_opacity", "Crosshair Opacity",
			"Adjust the opacity of the crosshair.",
			allSettings.shootmode_crosshair_opacity, 0, 100, sliderPercent, [
			](Slider& slider) {soundSlider(true); allSettings.shootmode_crosshair_opacity = slider.getValue(); });
		const char* selected_mode = crosshairs[allSettings.shootmode_crosshair];
		y += settingsAddDropdown(*settings_subwindow, y, "shootmode_crosshair", "Crosshair Type", "Adjust the appearance of the crosshair.",
			false, crosshairs, selected_mode, settingsCrosshairType);

#ifndef NINTENDO
        y += settingsAddBooleanOption(*settings_subwindow, y, "ui_filter", "Filter Scaling",
            "Scaled UI elements will have softer edges if this is enabled, at the cost of some sharpness.",
            allSettings.ui_filter_enabled, [](Button& button){soundToggle(); allSettings.ui_filter_enabled = button.isPressed();});
#endif

		y += settingsAddCustomize(*settings_subwindow, y, "minimap_settings", "Minimap Settings",
			"Customize the appearance of the in-game minimap.",
			[](Button& button){allSettings.minimap = Minimap::load(); settingsMinimap(button);});
		y += settingsAddBooleanWithCustomizeOption(*settings_subwindow, y, "show_messages", "Show Messages",
			"Customize which messages will be logged to the player, if any.",
			allSettings.show_messages_enabled, [](Button& button){soundToggle(); allSettings.show_messages_enabled = button.isPressed();},
			[](Button& button){allSettings.show_messages = Messages::load(); settingsMessages(button);});
		y += settingsAddBooleanOption(*settings_subwindow, y, "show_player_nametags", "Show Player Nametags",
			"Display the name of each player character above their avatar.",
			allSettings.show_player_nametags_enabled, [](Button& button){soundToggle(); allSettings.show_player_nametags_enabled = button.isPressed();});


		y += settingsAddSubHeader(*settings_subwindow, y, "accessibility", "Accessibility");
		y += settingsAddBooleanOption(*settings_subwindow, y, "content_control", "Content Control",
			"Disable the appearance of blood and other explicit kinds of content in the game",
			allSettings.content_control_enabled, [](Button& button) {soundToggle(); allSettings.content_control_enabled = button.isPressed(); });
		y += settingsAddBooleanOption(*settings_subwindow, y, "colorblind_mode", "Colorblind Mode",
			"Change the appearance of certain UI elements to improve visibility for certain colorblind individuals.",
			allSettings.colorblind_mode_enabled, [](Button& button) {soundToggle(); allSettings.colorblind_mode_enabled = button.isPressed(); });
		const char* arachnophobia_desc;
		if ( intro ) {
			arachnophobia_desc = "Replace all giant spiders in the game with hostile crustaceans.";
		}
		else {
			arachnophobia_desc = "Replace all giant spiders in the game with hostile crustaceans. (Updates at end of current dungeon level)";
		}
		y += settingsAddBooleanOption(*settings_subwindow, y, "arachnophobia_filter", "Arachnophobia Filter",
			arachnophobia_desc, allSettings.arachnophobia_filter_enabled,
			[](Button& button) {soundToggle(); allSettings.arachnophobia_filter_enabled = button.isPressed(); });
		y += settingsAddBooleanOption(*settings_subwindow, y, "shaking", "Shaking",
			"Toggle the camera's ability to twist and roll when the player stumbles or receives damage.",
			allSettings.shaking_enabled, [](Button& button) {soundToggle(); allSettings.shaking_enabled = button.isPressed(); });
		y += settingsAddBooleanOption(*settings_subwindow, y, "bobbing", "Bobbing",
			"Toggle the camera's ability to bob steadily as the player moves.",
			allSettings.bobbing_enabled, [](Button& button) {soundToggle(); allSettings.bobbing_enabled = button.isPressed(); });
		y += settingsAddBooleanOption(*settings_subwindow, y, "light_flicker", "Light Flicker",
			"Toggle the flickering appearance of torches and other light fixtures in the game world.",
			allSettings.light_flicker_enabled, [](Button& button) {soundToggle(); allSettings.light_flicker_enabled = button.isPressed(); });
#if 0
		y += settingsAddBooleanOption(*settings_subwindow, y, "show_hud", "Show HUD",
			"Toggle the display of health and other status bars in game when the inventory is closed.",
			allSettings.show_hud_enabled, [](Button& button){soundToggle(); allSettings.show_hud_enabled = button.isPressed();});
#endif

#ifndef NINTENDO
		hookSettings(*settings_subwindow,
			{{Setting::Type::Boolean, "fast_restart"},

			// inventory options
			{Setting::Type::Boolean, "add_items_to_hotbar"},
			{Setting::Type::Customize, "inventory_sorting"},
			{Setting::Type::Boolean, "use_on_release"},

			// hud options
            {Setting::Type::Slider, "ui_scale"},
			{Setting::Type::Slider, "enemybar_scale"},
			{Setting::Type::Slider, "world_tooltip_scale"},
			{Setting::Type::Slider, "world_tooltip_scale_splitscreen"},
			{Setting::Type::Slider, "item_tooltip_height"},
			{Setting::Type::Slider, "shootmode_crosshair_opacity"},
			{Setting::Type::Dropdown, "shootmode_crosshair"},
            {Setting::Type::Boolean, "ui_filter"},

			{Setting::Type::Customize, "minimap_settings"},
			{Setting::Type::BooleanWithCustomize, "show_messages"},
			{Setting::Type::Boolean, "show_player_nametags"},

			// accessibility
			{Setting::Type::Boolean, "content_control"},
			{Setting::Type::Boolean, "colorblind_mode"},
			{Setting::Type::Boolean, "arachnophobia_filter"},
			{Setting::Type::Boolean, "shaking"},
			{Setting::Type::Boolean, "bobbing"},
			{Setting::Type::Boolean, "light_flicker"},
			//{Setting::Type::Boolean, "show_hud"},
        });
#else
		hookSettings(*settings_subwindow,
			{{Setting::Type::Boolean, "fast_restart"},
			// inventory options
			{Setting::Type::Boolean, "add_items_to_hotbar"},
			{Setting::Type::Customize, "inventory_sorting"},

			// hud options
			{Setting::Type::Slider, "enemybar_scale"},
			{Setting::Type::Slider, "world_tooltip_scale"},
			{Setting::Type::Slider, "world_tooltip_scale_splitscreen"},
			{Setting::Type::Slider, "item_tooltip_height"},
			{Setting::Type::Slider, "shootmode_crosshair_opacity"},
			{Setting::Type::Dropdown, "shootmode_crosshair"},

			{Setting::Type::Customize, "minimap_settings"},
			{Setting::Type::BooleanWithCustomize, "show_messages"},
			{Setting::Type::Boolean, "show_player_nametags"},

			// accessibility
			{Setting::Type::Boolean, "content_control"},
			{Setting::Type::Boolean, "colorblind_mode"},
			{Setting::Type::Boolean, "arachnophobia_filter"},
			{Setting::Type::Boolean, "shaking"},
			{Setting::Type::Boolean, "bobbing"},
			{Setting::Type::Boolean, "light_flicker"},
        });
#endif

		settingsSubwindowFinalize(*settings_subwindow, y, {Setting::Type::Boolean, "fast_restart"});
		settingsSelect(*settings_subwindow, {Setting::Type::Boolean, "fast_restart"});
	}

	static void settingsVideo(Button& button) {
		Frame* settings_subwindow;
		if ((settings_subwindow = settingsSubwindowSetup(button)) == nullptr) {
			auto settings = main_menu_frame->findFrame("settings"); assert(settings);
			auto settings_subwindow = settings->findFrame("settings_subwindow"); assert(settings_subwindow);
#ifdef NINTENDO
			settingsSelect(*settings_subwindow, {Setting::Type::Boolean, "vertical_split"});
#else
			settingsSelect(*settings_subwindow, {Setting::Type::Dropdown, "resolution"});
#endif
			return;
		}
		int y = 0;

#ifndef NINTENDO
		int selected_res = -1;
		std::list<resolution> resolutions;
		getResolutionList(allSettings.video.display_id, resolutions);
		std::vector<std::string> resolutions_formatted;
		std::vector<const char*> resolutions_formatted_ptrs;
		resolutions_formatted.reserve(resolutions.size());
		resolutions_formatted_ptrs.reserve(resolutions.size());

		int index;
		std::list<resolution>::iterator it;
		for (index = 0, it = resolutions.begin(); it != resolutions.end(); ++it, ++index) {
			auto& res = *it;
			char buf[32];
			snprintf(buf, sizeof(buf), "%d x %d @ %dhz", res.x, res.y, res.hz);
			resolutions_formatted.push_back(std::string(buf));
			resolutions_formatted_ptrs.push_back(resolutions_formatted.back().c_str());
			if (allSettings.video.resolution_x == res.x && allSettings.video.resolution_y == res.y) {
				if (selected_res == -1) {
					selected_res = index;
				}
			}
		}
		if (selected_res == -1) {
			selected_res = 0;
		}

		int num_displays = getNumDisplays();
		std::vector<std::string> displays_formatted;
		std::vector<const char*> displays_formatted_ptrs;
		displays_formatted.reserve(num_displays);
		displays_formatted_ptrs.reserve(num_displays);
		for (int c = 0; c < num_displays; ++c) {
			displays_formatted.push_back("Display " + std::to_string(c + 1));
			displays_formatted_ptrs.push_back(displays_formatted.back().c_str());
		}

        const std::vector<const char*> modes = {"Windowed", "Borderless", "Fullscreen"};
		const char* selected_mode = borderless ? "Borderless" : (fullscreen ? "Fullscreen" : "Windowed");

		y += settingsAddSubHeader(*settings_subwindow, y, "display", "Display Mode");
        y += settingsAddDropdown(*settings_subwindow, y, "resolution", "Resolution", "Change the current window resolution.",
            true, resolutions_formatted_ptrs, resolutions_formatted_ptrs[selected_res],
            resolutions_formatted.size() > 5 ? settingsResolutionBig : settingsResolutionSmall);
        y += settingsAddDropdown(*settings_subwindow, y, "device", "Device", "Change the current display device.",
            false, displays_formatted_ptrs, displays_formatted_ptrs[allSettings.video.display_id],
            settingsDisplayDevice);
        y += settingsAddDropdown(*settings_subwindow, y, "window_mode", "Window Mode", "Change the current display mode.",
            false, modes, selected_mode, settingsWindowMode);
		y += settingsAddBooleanOption(*settings_subwindow, y, "vsync", "Vertical Sync",
			"Prevent screen-tearing by locking the game's refresh rate to the current display.",
			allSettings.video.vsync_enabled, [](Button& button){soundToggle(); allSettings.video.vsync_enabled = button.isPressed();});
#endif
		y += settingsAddSubHeader(*settings_subwindow, y, "options", "Display Options");
		y += settingsAddSlider(*settings_subwindow, y, "gamma", "Gamma",
			"Adjust the brightness of the visuals in-game.",
			allSettings.video.gamma, 50, 200, sliderPercent, [](Slider& slider){soundSlider(true); allSettings.video.gamma = slider.getValue();});
		y += settingsAddSlider(*settings_subwindow, y, "fov", "Field of View",
			"Adjust the vertical field-of-view of the in-game camera.",
			allSettings.fov, 40, 100, nullptr, [](Slider& slider){soundSlider(true); allSettings.fov = slider.getValue();});
#ifndef NINTENDO
        auto sliderFPS = [](float v) -> const char* {
            if ((int)v == AUTO_FPS) {
                return "Auto";
            } else {
                static char buf[8];
                snprintf(buf, sizeof(buf), "%d", (int)v);
                return buf;
            }
        };
        
		y += settingsAddSlider(*settings_subwindow, y, "fps", "FPS limit",
			"Limit the frame-rate of the game window. Do not set this higher than your refresh rate. (Recommended: Auto)",
			allSettings.fps ? allSettings.fps : AUTO_FPS, MIN_FPS, AUTO_FPS, sliderFPS, [](Slider& slider){soundSlider(true); allSettings.fps = slider.getValue();});
		/*y += settingsAddBooleanOption(*settings_subwindow, y, "hdr_enabled", "High Dynamic Range (HDR)",
			"Increases color contrast of the rendered world with both brightened and darkened areas.",
			allSettings.hdr_enabled, [](Button& button) {soundToggle(); allSettings.hdr_enabled = button.isPressed(); });*/
		y += settingsAddBooleanOption(*settings_subwindow, y, "use_frame_interpolation", "Camera Interpolation",
			"Smooth player camera by interpolating camera movements over additional frames.",
			allSettings.use_frame_interpolation, [](Button& button) {soundToggle(); allSettings.use_frame_interpolation = button.isPressed();});
#endif
		y += settingsAddBooleanOption(*settings_subwindow, y, "vertical_split", "Vertical Splitscreen",
			"For splitscreen with two-players: divide the screen along a vertical line rather than a horizontal one.",
			allSettings.vertical_split_enabled, [](Button& button){soundToggle(); allSettings.vertical_split_enabled = button.isPressed();});
		y += settingsAddBooleanOption(*settings_subwindow, y, "clipped_split", "Clipped Splitscreen",
			"For splitscreen with two-players: reduce each viewport by 20% to preserve aspect ratio.",
			allSettings.clipped_split_enabled, [](Button& button){soundToggle(); allSettings.clipped_split_enabled = button.isPressed();});
		y += settingsAddBooleanOption(*settings_subwindow, y, "staggered_split", "Staggered Splitscreen",
			"For splitscreen with two-players: stagger each viewport so they each rest in a corner of the display.",
			allSettings.staggered_split_enabled, [](Button& button){soundToggle(); allSettings.staggered_split_enabled = button.isPressed();});

#ifndef NINTENDO
		hookSettings(*settings_subwindow,{
            {Setting::Type::Dropdown, "resolution"},
			{Setting::Type::Dropdown, "device"},
			{Setting::Type::Dropdown, "window_mode"},
			{Setting::Type::Boolean, "vsync"},
			{Setting::Type::Slider, "gamma"},
			{Setting::Type::Slider, "fov"},
			{Setting::Type::Slider, "fps"},
			/*{Setting::Type::Boolean, "hdr_enabled"},*/
			{Setting::Type::Boolean, "use_frame_interpolation"},
			{Setting::Type::Boolean, "vertical_split"},
			{Setting::Type::Boolean, "clipped_split"},
			{Setting::Type::Boolean, "staggered_split"},

			});

		settingsSubwindowFinalize(*settings_subwindow, y, {Setting::Type::Dropdown, "resolution"});
		settingsSelect(*settings_subwindow, {Setting::Type::Dropdown, "resolution"});
#else
		hookSettings(*settings_subwindow,{
			{Setting::Type::Slider, "gamma"},
			{Setting::Type::Slider, "fov"},
			{Setting::Type::Boolean, "vertical_split"},
			{Setting::Type::Boolean, "clipped_split"},
			{Setting::Type::Boolean, "staggered_split"},
			});

		settingsSubwindowFinalize(*settings_subwindow, y, {Setting::Type::Slider, "gamma"});
		settingsSelect(*settings_subwindow, {Setting::Type::Slider, "gamma"});
#endif
	}

	static void settingsAudio(Button& button) {
		Frame* settings_subwindow;
		if ((settings_subwindow = settingsSubwindowSetup(button)) == nullptr) {
			auto settings = main_menu_frame->findFrame("settings"); assert(settings);
			auto settings_subwindow = settings->findFrame("settings_subwindow"); assert(settings_subwindow);
#if defined(NINTENDO) || !defined(USE_FMOD)
			settingsSelect(*settings_subwindow, {Setting::Type::Slider, "master_volume"});
#else
			settingsSelect(*settings_subwindow, {Setting::Type::Dropdown, "device"});
#endif
			return;
		}
		int y = 0;

#if !defined(NINTENDO) && defined(USE_FMOD)
		int selected_device = 0;
		int num_drivers = 0;
		(void)fmod_system->getNumDrivers(&num_drivers);

		audio_drivers.clear();
		audio_drivers.reserve(num_drivers);
		for (int c = 0; c < num_drivers; ++c) {
			AudioDriver d;
			(void)fmod_system->getDriverInfo(c, d.name, sizeof(d.name), &d.guid,
				&d.system_rate, &d.speaker_mode, &d.speaker_mode_channels);
			memcpy(d.name + 48, "...", 4); // long names get truncated
			audio_drivers.push_back(d);

			uint32_t _1; memcpy(&_1, &d.guid.Data1, sizeof(_1));
			uint64_t _2; memcpy(&_2, &d.guid.Data4, sizeof(_2));
			char guid_string[25];
			snprintf(guid_string, sizeof(guid_string), FMOD_AUDIO_GUID_FMT, _1, _2);
			if (!selected_device && allSettings.audio_device == guid_string) {
				selected_device = c;
			}
		}
		std::vector<const char*> drivers_formatted_ptrs;
		drivers_formatted_ptrs.reserve(num_drivers);
		for (auto& d : audio_drivers) {
			drivers_formatted_ptrs.push_back(d.name);
		}

		y += settingsAddSubHeader(*settings_subwindow, y, "output", "Output");
		y += settingsAddDropdown(*settings_subwindow, y, "device", "Device", "The output device for all game audio",
            true, drivers_formatted_ptrs, drivers_formatted_ptrs[selected_device],
            settingsAudioDevice);
#endif

		y += settingsAddSubHeader(*settings_subwindow, y, "volume", "Volume");
		y += settingsAddSlider(*settings_subwindow, y, "master_volume", "Master Volume",
			"Adjust the volume of all sound sources equally.",
			allSettings.master_volume, 0, 100, sliderPercent, [](Slider& slider){soundSlider(true); allSettings.master_volume = slider.getValue();
				setGlobalVolume(allSettings.master_volume / 100.0, allSettings.music_volume / 100.0, allSettings.gameplay_volume / 100.0,
				allSettings.ambient_volume / 100.0, allSettings.environment_volume / 100.0, allSettings.notification_volume / 100.0);});
		y += settingsAddSlider(*settings_subwindow, y, "gameplay_volume", "Gameplay Volume",
			"Adjust the volume of most game sound effects.",
			allSettings.gameplay_volume, 0, 100, sliderPercent, [](Slider& slider){soundSlider(true); allSettings.gameplay_volume = slider.getValue();
				setGlobalVolume(allSettings.master_volume / 100.0, allSettings.music_volume / 100.0, allSettings.gameplay_volume / 100.0,
				allSettings.ambient_volume / 100.0, allSettings.environment_volume / 100.0, allSettings.notification_volume / 100.0);});
		y += settingsAddSlider(*settings_subwindow, y, "ambient_volume", "Ambient Volume",
			"Adjust the volume of ominous subterranean sound-cues.",
			allSettings.ambient_volume, 0, 100, sliderPercent, [](Slider& slider){soundSlider(true); allSettings.ambient_volume = slider.getValue();
				setGlobalVolume(allSettings.master_volume / 100.0, allSettings.music_volume / 100.0, allSettings.gameplay_volume / 100.0,
				allSettings.ambient_volume / 100.0, allSettings.environment_volume / 100.0, allSettings.notification_volume / 100.0);});
		y += settingsAddSlider(*settings_subwindow, y, "environment_volume", "Environment Volume",
			"Adjust the volume of flowing water and lava.",
			allSettings.environment_volume, 0, 100, sliderPercent, [](Slider& slider){soundSlider(true); allSettings.environment_volume = slider.getValue();
				setGlobalVolume(allSettings.master_volume / 100.0, allSettings.music_volume / 100.0, allSettings.gameplay_volume / 100.0,
				allSettings.ambient_volume / 100.0, allSettings.environment_volume / 100.0, allSettings.notification_volume / 100.0);});
		y += settingsAddSlider(*settings_subwindow, y, "notification_volume", "Notification Volume",
			"Adjust the volume of skill increase and level up notifications.",
			allSettings.notification_volume, 0, 100, sliderPercent, [](Slider& slider) {soundSlider(true); allSettings.notification_volume = slider.getValue();
		setGlobalVolume(allSettings.master_volume / 100.0, allSettings.music_volume / 100.0, allSettings.gameplay_volume / 100.0,
			allSettings.ambient_volume / 100.0, allSettings.environment_volume / 100.0, allSettings.notification_volume / 100.0); });
		y += settingsAddSlider(*settings_subwindow, y, "music_volume", "Music Volume",
			"Adjust the volume of the game's soundtrack.",
			allSettings.music_volume, 0, 100, sliderPercent, [](Slider& slider){soundSlider(true); allSettings.music_volume = slider.getValue();
				setGlobalVolume(allSettings.master_volume / 100.0, allSettings.music_volume / 100.0, allSettings.gameplay_volume / 100.0,
				allSettings.ambient_volume / 100.0, allSettings.environment_volume / 100.0, allSettings.notification_volume / 100.0);});

		y += settingsAddSubHeader(*settings_subwindow, y, "options", "Options");
		y += settingsAddBooleanOption(*settings_subwindow, y, "minimap_pings", "Minimap Pings",
			"Toggle the ability to hear pings on the minimap",
			allSettings.minimap_pings_enabled, [](Button& button){soundToggle(); allSettings.minimap_pings_enabled = button.isPressed();});
		y += settingsAddBooleanOption(*settings_subwindow, y, "player_monster_sounds", "Player Monster Sounds",
			"Toggle the chance to emit monstrous mumbles when playing a non-human character.",
			allSettings.player_monster_sounds_enabled, [](Button& button){soundToggle(); allSettings.player_monster_sounds_enabled = button.isPressed();});
#ifndef NINTENDO
		y += settingsAddBooleanOption(*settings_subwindow, y, "out_of_focus_audio", "Out-of-Focus Audio",
			"Enable audio sources even when the game window is out-of-focus.",
			allSettings.out_of_focus_audio_enabled, [](Button& button){soundToggle(); allSettings.out_of_focus_audio_enabled = button.isPressed();});
#endif

#ifndef NINTENDO
		hookSettings(*settings_subwindow,
			{
#ifdef USE_FMOD
			{Setting::Type::Dropdown, "device"},
#endif
			{Setting::Type::Slider, "master_volume"},
			{Setting::Type::Slider, "gameplay_volume"},
			{Setting::Type::Slider, "ambient_volume"},
			{Setting::Type::Slider, "environment_volume"},
			{Setting::Type::Slider, "notification_volume"},
			{Setting::Type::Slider, "music_volume"},
			{Setting::Type::Boolean, "minimap_pings"},
			{Setting::Type::Boolean, "player_monster_sounds"},
			{Setting::Type::Boolean, "out_of_focus_audio"}});
#else
		hookSettings(*settings_subwindow,
			{{Setting::Type::Slider, "master_volume"},
			{Setting::Type::Slider, "gameplay_volume"},
			{Setting::Type::Slider, "ambient_volume"},
			{Setting::Type::Slider, "environment_volume"},
			{Setting::Type::Slider, "notification_volume"},
			{Setting::Type::Slider, "music_volume"},
			{Setting::Type::Boolean, "minimap_pings"},
			{Setting::Type::Boolean, "player_monster_sounds"}});
#endif

#if !defined(NINTENDO) && defined(USE_FMOD)
		settingsSubwindowFinalize(*settings_subwindow, y, {Setting::Type::Dropdown, "device"});
		settingsSelect(*settings_subwindow, {Setting::Type::Dropdown, "device"});
#else
		settingsSubwindowFinalize(*settings_subwindow, y, {Setting::Type::Slider, "master_volume"});
		settingsSelect(*settings_subwindow, {Setting::Type::Slider, "master_volume"});
#endif
	}

	static void settingsControls(Button& button) {
		Frame* settings_subwindow;
		if ((settings_subwindow = settingsSubwindowSetup(button)) == nullptr) {
			auto settings = main_menu_frame->findFrame("settings"); assert(settings);
			auto settings_subwindow = settings->findFrame("settings_subwindow"); assert(settings_subwindow);
			settingsSelect(*settings_subwindow, {Setting::Type::Customize, "bindings"});
			return;
		}
		int y = 0;

#ifndef NINTENDO
		y += settingsAddSubHeader(*settings_subwindow, y, "general", "General Settings");
		y += settingsAddCustomize(*settings_subwindow, y, "bindings", "Bindings",
			"Modify controls for mouse, keyboard, gamepads, and other peripherals.",
			[](Button&){
			    allSettings.bindings = Bindings::load();
				const int player = multiplayer == CLIENT ? 0 : getMenuOwner();
			    settingsBindings(player, inputs.hasController(player) ? 1 : 0, getMatchingProfileName(player, inputs.hasController(player)),
			        {Setting::Type::Dropdown, "player_dropdown_button"});
			    });

		y += settingsAddSubHeader(*settings_subwindow, y, "mouse_and_keyboard", "Mouse & Keyboard");
		std::vector<const char*> mkb_facehotbar_strings = { "Classic", "Modern" };
		y += settingsAddSlider(*settings_subwindow, y, "mouse_sensitivity", "Mouse Sensitivity",
			"Control the speed by which mouse movement affects camera movement.",
			allSettings.mouse_sensitivity, 0, 100, nullptr, [](Slider& slider){soundSlider(true); allSettings.mouse_sensitivity = slider.getValue();});
		/*y += settingsAddDropdown(*settings_subwindow, y, "mkb_facehotbar", "Hotbar Layout",
			"Classic: Flat 10 slot layout. Modern: Grouped 3x3 slot layout.", false,
			mkb_facehotbar_strings, mkb_facehotbar_strings[allSettings.mkb_facehotbar ? 1 : 0], settingsMkbHotbarLayout);*/
		y += settingsAddBooleanOption(*settings_subwindow, y, "numkeys_in_inventory", "Number Keys in Inventory",
			"Allow the player to bind inventory items to the hotbar using the number keys on their keyboard.",
			allSettings.numkeys_in_inventory_enabled, [](Button& button){soundToggle(); allSettings.numkeys_in_inventory_enabled = button.isPressed();});
		y += settingsAddBooleanOption(*settings_subwindow, y, "reverse_mouse", "Reverse Mouse",
			"Reverse mouse up and down movement for controlling the orientation of the player.",
			allSettings.reverse_mouse_enabled, [](Button& button){soundToggle(); allSettings.reverse_mouse_enabled = button.isPressed();});
		y += settingsAddBooleanOption(*settings_subwindow, y, "smooth_mouse", "Smooth Mouse",
			"Smooth the movement of the mouse over a few frames of input.",
			allSettings.smooth_mouse_enabled, [](Button& button){soundToggle(); allSettings.smooth_mouse_enabled = button.isPressed();});
		/*y += settingsAddBooleanOption(*settings_subwindow, y, "rotation_speed_limit", "Rotation Speed Limit",
			"Limit how fast the player can rotate by moving the mouse.",
			allSettings.rotation_speed_limit_enabled, [](Button& button){soundToggle(); allSettings.rotation_speed_limit_enabled = button.isPressed();});*/
		y += settingsAddBooleanOption(*settings_subwindow, y, "mkb_world_tooltips", "Interact Aim Assist",
			"Disable to always use precise cursor targeting on interactable objects and remove interact popups.",
			allSettings.mkb_world_tooltips_enabled, [](Button& button) {soundToggle(); allSettings.mkb_world_tooltips_enabled = button.isPressed(); });
#endif

#ifdef NINTENDO
		y += settingsAddSubHeader(*settings_subwindow, y, "gamepad", "Controller Settings");
		y += settingsAddCustomize(*settings_subwindow, y, "bindings", "Bindings",
			"Change controller bindings.",
			[](Button&){
			    allSettings.bindings = Bindings::load();
				const int player = multiplayer == CLIENT ? 0 : getMenuOwner();
			    settingsBindings(player, inputs.hasController(player) ? 1 : 0, getMatchingProfileName(player, inputs.hasController(player)),
			        {Setting::Type::Dropdown, "player_dropdown_button"});
			    });
#else
		y += settingsAddSubHeader(*settings_subwindow, y, "gamepad", "Gamepad Settings");
#endif
		std::vector<const char*> gamepad_facehotbar_strings = { "Modern", "Classic" };
		y += settingsAddDropdown(*settings_subwindow, y, "gamepad_facehotbar", "Hotbar Layout",
			"Modern: Grouped 3x3 slot layout using held buttons. Classic: Flat 10 slot layout with simpler controls.", false,
			gamepad_facehotbar_strings, gamepad_facehotbar_strings[allSettings.gamepad_facehotbar ? 0 : 1], settingsGamepadHotbarLayout);
		y += settingsAddSlider(*settings_subwindow, y, "turn_sensitivity_x", "Turn Sensitivity X",
			"Affect the horizontal sensitivity of the control stick used for turning.",
			allSettings.turn_sensitivity_x, 25.f, 200.f, sliderPercent, [](Slider& slider){soundSlider(true); allSettings.turn_sensitivity_x = slider.getValue();});
		y += settingsAddSlider(*settings_subwindow, y, "turn_sensitivity_y", "Turn Sensitivity Y",
			"Affect the vertical sensitivity of the control stick used for turning.",
			allSettings.turn_sensitivity_y, 25.f, 200.f, sliderPercent, [](Slider& slider){soundSlider(true); allSettings.turn_sensitivity_y = slider.getValue();});

		y += settingsAddBooleanOption(*settings_subwindow, y, "gamepad_camera_invert_x", "Invert Camera Look X",
			"Enable to invert left/right look controls of the player camera.",
			allSettings.gamepad_camera_invert_x, [](Button& button) {soundToggle(); allSettings.gamepad_camera_invert_x = button.isPressed(); });
		y += settingsAddBooleanOption(*settings_subwindow, y, "gamepad_camera_invert_y", "Invert Camera Look Y",
			"Enable to invert up/down look controls of the player camera.",
			allSettings.gamepad_camera_invert_y, [](Button& button) {soundToggle(); allSettings.gamepad_camera_invert_y = button.isPressed(); });

#ifndef NINTENDO
		hookSettings(*settings_subwindow,
			{{Setting::Type::Customize, "bindings"},
			{Setting::Type::Slider, "mouse_sensitivity"},
			//{Setting::Type::Dropdown, "mkb_facehotbar"},
			{Setting::Type::Boolean, "numkeys_in_inventory"},
			{Setting::Type::Boolean, "reverse_mouse"},
			{Setting::Type::Boolean, "smooth_mouse"},
			//{Setting::Type::Boolean, "rotation_speed_limit"},
			{Setting::Type::Boolean, "mkb_world_tooltips"},
			{Setting::Type::Dropdown, "gamepad_facehotbar"},
			{Setting::Type::Slider, "turn_sensitivity_x"},
			{Setting::Type::Slider, "turn_sensitivity_y"},
			{Setting::Type::Boolean, "gamepad_camera_invert_x"},
			{Setting::Type::Boolean, "gamepad_camera_invert_y"},
		});
#else
		hookSettings(*settings_subwindow,
			{{Setting::Type::Customize, "bindings"},
			{Setting::Type::Dropdown, "gamepad_facehotbar"},
			{Setting::Type::Slider, "turn_sensitivity_x"},
			{Setting::Type::Slider, "turn_sensitivity_y"},
			{Setting::Type::Boolean, "gamepad_camera_invert_x"},
			{Setting::Type::Boolean, "gamepad_camera_invert_y"},
		});
#endif

		settingsSubwindowFinalize(*settings_subwindow, y, {Setting::Type::Customize, "bindings"});
		settingsSelect(*settings_subwindow, {Setting::Type::Customize, "bindings"});
	}

	static void settingsOnline(Button& button) {
		Frame* settings_subwindow;
		if ((settings_subwindow = settingsSubwindowSetup(button)) == nullptr) {
			auto settings = main_menu_frame->findFrame("settings"); assert(settings);
			auto settings_subwindow = settings->findFrame("settings_subwindow"); assert(settings_subwindow);
		    settingsSelect(*settings_subwindow, {Setting::Type::Field, "port_number"});
			return;
		}
		int y = 0;

#if 0
		y += settingsAddSubHeader(*settings_subwindow, y, "general", "General");
		y += settingsAddBooleanOption(*settings_subwindow, y, "show_ip_address", "Streamer Mode",
			"If you're a streamer and know what doxxing is, definitely switch this on.",
			allSettings.show_ip_address_enabled, [](Button& button){soundToggle(); allSettings.show_ip_address_enabled = button.isPressed();});
#endif

        char port_desc[1024];
        snprintf(port_desc, sizeof(port_desc), "The port number to use when hosting a LAN lobby. (Default: %d)", DEFAULT_PORT);

        char buf[16];
        snprintf(buf, sizeof(buf), "%hu", (Uint16)allSettings.port_number);
		y += settingsAddSubHeader(*settings_subwindow, y, "lan", "LAN");
		y += settingsAddField(*settings_subwindow, y, "port_number", "Port",
		    port_desc, buf, [](Field& field){allSettings.port_number = (Uint16)strtol(field.getText(), nullptr, 10);});

#if defined(USE_EOS) && (defined(STEAMWORKS) || defined(NINTENDO))
		y += settingsAddSubHeader(*settings_subwindow, y, "crossplay", "Crossplay");
		y += settingsAddBooleanOption(*settings_subwindow, y, "crossplay", "Crossplay Enabled",
		    "Enable crossplay through Epic Online Services",
		    allSettings.crossplay_enabled, [](Button& button){soundToggle(); allSettings.crossplay_enabled = button.isPressed();});

		hookSettings(*settings_subwindow,
			{{Setting::Type::Field, "port_number"},
			{Setting::Type::Boolean, "crossplay"}});
#else
		hookSettings(*settings_subwindow,
			{{Setting::Type::Field, "port_number"}});
#endif

		settingsSubwindowFinalize(*settings_subwindow, y, {Setting::Type::Field, "port_number"});
		settingsSelect(*settings_subwindow, {Setting::Type::Field, "port_number"});
	}

	static void settingsGame(Button& button) {
		Frame* settings_subwindow;
		if ((settings_subwindow = settingsSubwindowSetup(button)) == nullptr) {
			auto settings = main_menu_frame->findFrame("settings"); assert(settings);
			auto settings_subwindow = settings->findFrame("settings_subwindow"); assert(settings_subwindow);
			settingsSelect(*settings_subwindow, {Setting::Type::Boolean, "hunger"});
			return;
		}
		int y = 0;

		if (multiplayer == CLIENT) {
			allSettings.classic_mode_enabled = svFlags & SV_FLAG_CLASSIC;
			allSettings.hardcore_mode_enabled = svFlags & SV_FLAG_HARDCORE;
			allSettings.friendly_fire_enabled = svFlags & SV_FLAG_FRIENDLYFIRE;
			allSettings.keep_inventory_enabled = svFlags & SV_FLAG_KEEPINVENTORY;
			allSettings.hunger_enabled = svFlags & SV_FLAG_HUNGER;
			allSettings.minotaur_enabled = svFlags & SV_FLAG_MINOTAURS;
			allSettings.random_traps_enabled = svFlags & SV_FLAG_TRAPS;
			allSettings.extra_life_enabled = svFlags & SV_FLAG_LIFESAVING;
			allSettings.cheats_enabled = svFlags & SV_FLAG_CHEATS;
		}

		y += settingsAddSubHeader(*settings_subwindow, y, "game", "Game Settings");
		y += settingsAddBooleanOption(*settings_subwindow, y, "hunger", "Hunger",
			"When hunger is off, passive HP regeneration is disabled and eating food heals the player directly.",
			allSettings.hunger_enabled, [](Button& button){soundToggle(); allSettings.hunger_enabled = button.isPressed();});
		y += settingsAddBooleanOption(*settings_subwindow, y, "minotaur", "Minotaur",
			"Toggle the minotaur's ability to spawn on many levels after a certain amount of time.",
			allSettings.minotaur_enabled, [](Button& button){soundToggle(); allSettings.minotaur_enabled = button.isPressed();});
		y += settingsAddBooleanOption(*settings_subwindow, y, "random_traps", "Random Traps",
			"Toggle the random placement of traps throughout each level.",
			allSettings.random_traps_enabled, [](Button& button){soundToggle(); allSettings.random_traps_enabled = button.isPressed();});
		y += settingsAddBooleanOption(*settings_subwindow, y, "friendly_fire", "Friendly Fire",
			"Enable players to harm each other and their allies.",
			allSettings.friendly_fire_enabled, [](Button& button){soundToggle(); allSettings.friendly_fire_enabled = button.isPressed();});
		y += settingsAddBooleanOption(*settings_subwindow, y, "hardcore_mode", "Hardcore Mode",
			"Greatly increases the difficulty of all combat encounters.",
			allSettings.hardcore_mode_enabled, [](Button& button){soundToggle(); allSettings.hardcore_mode_enabled = button.isPressed();});
		y += settingsAddBooleanOption(*settings_subwindow, y, "classic_mode", "Classic Mode",
			"Toggle this option to make the game end after the battle with Baron Herx.",
			allSettings.classic_mode_enabled, [](Button& button){soundToggle(); allSettings.classic_mode_enabled = button.isPressed();});
		y += settingsAddBooleanOption(*settings_subwindow, y, "keep_inventory", "Keep Inventory after Death",
			"When a player dies, they retain their inventory when revived on the next level.",
			allSettings.keep_inventory_enabled, [](Button& button){soundToggle(); allSettings.keep_inventory_enabled = button.isPressed();});
		y += settingsAddBooleanOption(*settings_subwindow, y, "extra_life", "Extra Life",
			"Start the game with an Amulet of Life-saving, to prevent one death.",
			allSettings.extra_life_enabled, [](Button& button){soundToggle(); allSettings.extra_life_enabled = button.isPressed();});
#ifndef NINTENDO
		y += settingsAddBooleanOption(*settings_subwindow, y, "cheats", "Cheats",
			"Toggle the ability to activate cheatcodes during gameplay.",
			allSettings.cheats_enabled, [](Button& button){soundToggle(); allSettings.cheats_enabled = button.isPressed();});
#endif

#ifndef NINTENDO
		hookSettings(*settings_subwindow,
			{{Setting::Type::Boolean, "hunger"},
			{Setting::Type::Boolean, "minotaur"},
			{Setting::Type::Boolean, "random_traps"},
			{Setting::Type::Boolean, "friendly_fire"},
			{Setting::Type::Boolean, "hardcore_mode"},
			{Setting::Type::Boolean, "classic_mode"},
			{Setting::Type::Boolean, "keep_inventory"},
			{Setting::Type::Boolean, "extra_life"},
			{Setting::Type::Boolean, "cheats"}});
#else
		hookSettings(*settings_subwindow,
			{{Setting::Type::Boolean, "hunger"},
			{Setting::Type::Boolean, "minotaur"},
			{Setting::Type::Boolean, "random_traps"},
			{Setting::Type::Boolean, "friendly_fire"},
			{Setting::Type::Boolean, "hardcore_mode"},
			{Setting::Type::Boolean, "classic_mode"},
			{Setting::Type::Boolean, "keep_inventory"},
			{Setting::Type::Boolean, "extra_life"}});
#endif

		settingsSubwindowFinalize(*settings_subwindow, y, {Setting::Type::Boolean, "hunger"});
		settingsSelect(*settings_subwindow, {Setting::Type::Boolean, "hunger"});

		if (multiplayer == CLIENT) {
			static const std::unordered_map<std::string, int> options = {
				{"setting_hunger_button", SV_FLAG_HUNGER},
				{"setting_minotaur_button", SV_FLAG_MINOTAURS},
				{"setting_random_traps_button", SV_FLAG_TRAPS},
				{"setting_friendly_fire_button", SV_FLAG_FRIENDLYFIRE},
				{"setting_hardcore_mode_button", SV_FLAG_HARDCORE},
				{"setting_classic_mode_button", SV_FLAG_CLASSIC},
				{"setting_keep_inventory_button", SV_FLAG_KEEPINVENTORY},
				{"setting_extra_life_button", SV_FLAG_LIFESAVING},
				{"setting_cheats_button", SV_FLAG_CHEATS},
			};
			for (auto& button : settings_subwindow->getButtons()) {
				button->setDisabled(true);
				button->setColor(makeColor(127, 127, 127, 255));
				button->setTextColor(makeColor(127, 127, 127, 255));
			}
			auto updater = settings_subwindow->addFrame("updater");
			updater->setInvisible(true);
			updater->setTickCallback([](Widget& widget){
				auto settings_subwindow = static_cast<Frame*>(widget.getParent());
				for (auto& button : settings_subwindow->getButtons()) {
					auto find = options.find(button->getName());
					if (find != options.end()) {
						auto flag = find->second;
						button->setPressed(svFlags & flag);
					}
				}
				});
		}
	}

/******************************************************************************/

    static void createLeaderboards() {
        assert(main_menu_frame);

        static score_t* selectedScore;
        selectedScore = nullptr;

        auto dimmer = main_menu_frame->addFrame("dimmer");
        dimmer->setSize(SDL_Rect{
            0, 0,
            Frame::virtualScreenX,
            Frame::virtualScreenY
            });
        dimmer->setBorder(0);
        dimmer->setColor(makeColor(0, 0, 0, 63));

        auto window = dimmer->addFrame("leaderboards");
        window->setSize(SDL_Rect{
            (Frame::virtualScreenX - 992) / 2,
            (Frame::virtualScreenY - 720) / 2,
            992,
            720});
        window->setActualSize(SDL_Rect{0, 0, 992, 720});
        window->setBorder(0);
        window->setColor(0);

		auto back = createBackWidget(window, [](Button& button){
			soundCancel();
			auto frame = static_cast<Frame*>(button.getParent());
			frame = static_cast<Frame*>(frame->getParent());
			frame = static_cast<Frame*>(frame->getParent());
			frame->removeSelf();
			if (!main_menu_frame) {
				return;
			}
			auto buttons = main_menu_frame->findFrame("buttons"); assert(buttons);
			auto leaderboards = buttons->findButton("Leaderboards"); assert(leaderboards);
			leaderboards->select();
			});
		back->select();

        auto background = window->addImage(
			SDL_Rect{10, 0, 972, 714},
			0xffffffff,
			"*images/ui/Main Menus/Leaderboards/AA_Window_03.png",
			"background"
		    );

		auto timber = window->addImage(
			SDL_Rect{0, 138, 992, 582},
			0xffffffff,
			"*images/ui/Main Menus/Leaderboards/AA_Window_Overlay_00.png",
		    "timber"
		    );
		timber->ontop = true;

		auto banner = window->addField("banner", 128);
		banner->setFont(banner_font);
#ifdef NINTENDO
		banner->setText("HIGHSCORES");
#else
		banner->setText("LEADERBOARDS");
#endif
		banner->setSize(SDL_Rect{330, 30, 338, 24});
		banner->setJustify(Field::justify_t::CENTER);

        auto list = window->addFrame("list");
        list->setSize(SDL_Rect{76, 148, 278, 468});
        list->setActualSize(SDL_Rect{0, 0, 278, 468});
        list->setScrollBarsEnabled(false);
        list->setBorder(0);
        list->setColor(0);

        auto subframe = window->addFrame("subframe");
        subframe->setSize(SDL_Rect{354, 148, 608, 468});
        subframe->setBorder(0);
        subframe->setColor(0);
        subframe->setInvisible(true);
        subframe->setTickCallback([](Widget& widget){
            widget.setInvisible(selectedScore == nullptr);
            });

        static real_t portrait_rotation;
        portrait_rotation = (2.0 * PI) - (PI / 6.0);

        auto portrait = subframe->addFrame("portrait");
        portrait->setSize(SDL_Rect{0, 0, 284, 280});
        portrait->setBorder(0);
        portrait->setColor(0);
        portrait->setDrawCallback([](const Widget&, const SDL_Rect rect){
			drawCharacterPreview(0, rect, 50, portrait_rotation);
            });
        portrait->setTickCallback([](Widget& widget){
            auto frame = static_cast<Frame*>(&widget);
            auto& input = Input::inputs[widget.getOwner()];
            real_t speed = PI / fpsLimit;
            if (frame->capturesMouse()) {
                portrait_rotation += (real_t)input.binaryToggle("MenuMouseWheelDown") * speed * 10;
                portrait_rotation -= (real_t)input.binaryToggle("MenuMouseWheelUp") * speed * 10;
            }
            portrait_rotation += input.analog("MenuScrollRight") * speed;
            portrait_rotation -= input.analog("MenuScrollLeft") * speed;
            });

		auto conduct_panel = subframe->addImage(
		    SDL_Rect{4, 348, 276, 116},
		    0xffffffff,
		    "*images/ui/Main Menus/Leaderboards/AA_Box_AdventurerInfo_00.png",
		    "conduct_panel"
		    );

		auto conduct = subframe->addFrame("conduct");
		conduct->setFont(smallfont_outline);
		conduct->setSize(SDL_Rect{6, 360, 272, 102});
		conduct->setActualSize(SDL_Rect{0, 0, 272, 102});
		conduct->setScrollBarsEnabled(false);
		conduct->setListOffset(SDL_Rect{0, 4, 0, 0});
		conduct->setEntrySize(28);
		conduct->setBorder(0);
		conduct->setColor(0);
		conduct->setSelectorOffset(SDL_Rect{0, -6, 0, 0,});
		conduct->setWidgetSearchParent("leaderboards");
		conduct->addWidgetMovement("MenuListCancel", "conduct");
		conduct->addWidgetAction("MenuCancel", "back_button");
		conduct->addWidgetAction("MenuAlt1", "delete_entry");
        conduct->addWidgetAction("MenuPageLeft", "tab_left");
        conduct->addWidgetAction("MenuPageRight", "tab_right");
        conduct->addWidgetAction("MenuPageLeftAlt", "category_left");
        conduct->addWidgetAction("MenuPageRightAlt", "category_right");
        conduct->setWidgetRight("kills_left");
        conduct->setSelectedEntryColor(makeColor(151, 115, 58, 255));
		conduct->setScrollWithLeftControls(false);
		conduct->setClickable(true);
        conduct->setGlyphPosition(Widget::glyph_position_t::BOTTOM_RIGHT);

        auto victory_plate = subframe->addImage(
            SDL_Rect{2, 280, 280, 82},
            0xffffffff,
            "*images/ui/Main Menus/Leaderboards/AA_VictoryPlate_DeadEnd_Plate_00A.png",
            "victory_plate"
            );

        auto victory_plate_header = subframe->addImage(
            SDL_Rect{34, 194, 214, 100},
            0xffffffff,
            "*images/ui/Main Menus/Leaderboards/AA_VictoryPlate_Gold_Image_00.png",
            "victory_plate_header"
            );
        victory_plate_header->ontop = true;

        auto victory_plate_text = subframe->addField("victory_plate_text", 1024);
        victory_plate_text->setSize(SDL_Rect{26, 290, 232, 62});
        victory_plate_text->setFont(smallfont_outline);
        victory_plate_text->setJustify(Field::justify_t::CENTER);

        struct Victory {
            const char* text;
            const char* plate_image;
            const char* header_image;
            Uint32 textColor;
            Uint32 outlineColor;
        };
        static Victory victories[] = {
            { // defeat (victory = 0)
                "Here lies\n%s\nRequiescat In Pace",
                "*images/ui/Main Menus/Leaderboards/AA_VictoryPlate_DeadEnd_Plate_00A.png",
                "*images/ui/Main Menus/Leaderboards/AA_VictoryPlate_DeadEnd_Image_01B.png",
                makeColor(151, 115, 58, 255),
                makeColor(21, 9, 8, 255)
            },
            { // classic victory (victory = 1)
                "Make Way For\n%s\nthe Triumphant!",
                "*images/ui/Main Menus/Leaderboards/AA_VictoryPlate_Gold_Plate_00.png",
                "*images/ui/Main Menus/Leaderboards/AA_VictoryPlate_Gold_Image_01B.png",
                makeColor(230, 183, 20, 255),
                makeColor(82, 31, 4, 255)
            },
            { // classic hell victory (victory = 2)
                "Bow Before\n%s\nthe Eternal!",
                "*images/ui/Main Menus/Leaderboards/AA_VictoryPlate_Gold_Plate_00.png",
                "*images/ui/Main Menus/Leaderboards/AA_VictoryPlate_Gold_Image_01B.png",
                makeColor(230, 183, 20, 255),
                makeColor(82, 31, 4, 255)
            },
            { // neutral (beast) victory (victory = 3)
                "Long Live\n%s\nthe Baron!",
                "*images/ui/Main Menus/Leaderboards/AA_VictoryPlate_Gold_Plate_00.png",
                "*images/ui/Main Menus/Leaderboards/AA_VictoryPlate_Gold_Image_01B.png",
                makeColor(230, 183, 20, 255),
                makeColor(82, 31, 4, 255)
            },
            { // good (human) victory (victory = 4)
                "All Hail\n%s\nthe Baron!",
                "*images/ui/Main Menus/Leaderboards/AA_VictoryPlate_GoodEnd_Plate_00.png",
                "*images/ui/Main Menus/Leaderboards/AA_VictoryPlate_GoodEnd_Image_01B.png",
                makeColor(110, 107, 224, 255),
                makeColor(22, 16, 30, 255)
            },
            { // evil (demon) victory (victory = 5)
                "Tremble Before\n%s\nthe Baron!",
                "*images/ui/Main Menus/Leaderboards/AA_VictoryPlate_EvilEnd_Plate_00.png",
                "*images/ui/Main Menus/Leaderboards/AA_VictoryPlate_EvilEnd_Image_01B.png",
                makeColor(223, 42, 42, 255),
                makeColor(52, 10, 28, 255)
            },
        };
        static constexpr int num_victories = sizeof(victories) / sizeof(victories[0]);

        auto right_panel = subframe->addImage(
			SDL_Rect{284, 0, 338, 476},
			0xffffffff,
			"*images/ui/Main Menus/Leaderboards/AA_Window_SubwindowR_00.png",
		    "right_panel"
		    );

		auto character_title = subframe->addField("character_title", 256);
		character_title->setFont(smallfont_outline);
		character_title->setSize(SDL_Rect{296, 2, 306, 26});
		character_title->setColor(makeColor(203, 171, 101, 255));
		character_title->setJustify(Field::justify_t::CENTER);

		auto character_counters_titles = subframe->addField("character_counters_titles", 256);
		character_counters_titles->setFont(smallfont_outline);
		character_counters_titles->setSize(SDL_Rect{346, 28, 206, 62});
		character_counters_titles->setColor(makeColor(151, 115, 58, 255));
		character_counters_titles->setHJustify(Field::justify_t::LEFT);
		character_counters_titles->setVJustify(Field::justify_t::TOP);
		character_counters_titles->setText("XP:\nGold:\nDungeon Level:");

		auto character_counters = subframe->addField("character_counters", 256);
		character_counters->setFont(smallfont_outline);
		character_counters->setSize(SDL_Rect{346, 28, 206, 62});
		character_counters->setColor(makeColor(151, 115, 58, 255));
		character_counters->setHJustify(Field::justify_t::RIGHT);
		character_counters->setVJustify(Field::justify_t::TOP);

		Field* character_attributes[6];
        for (int c = 0; c < 6; ++c) {
            std::string name = std::string("character_attribute") + std::to_string(c);
		    character_attributes[c] = subframe->addField(name.c_str(), 32);
		    character_attributes[c]->setFont(smallfont_outline);
		    character_attributes[c]->setSize(SDL_Rect{360 + 124 * (c / 3), 93 + (30 * (c % 3)), 96, 28});
		    character_attributes[c]->setColor(makeColor(151, 115, 58, 255));
		    character_attributes[c]->setHJustify(Field::justify_t::LEFT);
		    character_attributes[c]->setVJustify(Field::justify_t::CENTER);
		}

		auto kills_banner = subframe->addField("kills_banner", 64);
		kills_banner->setFont(bigfont_outline);
		kills_banner->setSize(SDL_Rect{426, 188, 182, 34});
		kills_banner->setColor(makeColor(203, 171, 101, 255));
		kills_banner->setHJustify(Field::justify_t::LEFT);
		kills_banner->setVJustify(Field::justify_t::CENTER);
		kills_banner->setText("Kills:");

		auto kills_left = subframe->addFrame("kills_left");
		kills_left->setScrollBarsEnabled(false);
		kills_left->setFont(smallfont_outline);
		kills_left->setSize(SDL_Rect{300, 226, 290, 182});
		kills_left->setActualSize(SDL_Rect{0, 0, 144, 182});
		kills_left->setListOffset(SDL_Rect{0, 4, 0, 0});
		kills_left->setEntrySize(28);
		kills_left->setBorder(0);
		kills_left->setColor(0);
		kills_left->setWidgetSearchParent("leaderboards");
		kills_left->addWidgetMovement("MenuListCancel", "kills_left");
		kills_left->addWidgetAction("MenuCancel", "back_button");
		kills_left->addWidgetAction("MenuAlt1", "delete_entry");
        kills_left->addWidgetAction("MenuPageLeft", "tab_left");
        kills_left->addWidgetAction("MenuPageRight", "tab_right");
        kills_left->addWidgetAction("MenuPageLeftAlt", "category_left");
        kills_left->addWidgetAction("MenuPageRightAlt", "category_right");
		kills_left->setWidgetLeft("conduct");
		kills_left->addSyncScrollTarget("kills_right");
        kills_left->setSelectedEntryColor(makeColor(101, 78, 39, 255));
		kills_left->setSelectorOffset(SDL_Rect{-12, -4, 18, 0,});
		kills_left->setScrollWithLeftControls(false);
		kills_left->setClickable(true);

		auto kills_right = subframe->addFrame("kills_right");
		kills_right->setScrollBarsEnabled(false);
		kills_right->setAllowScrollBinds(false);
		kills_right->setHideSelectors(true);
		kills_right->setHollow(true);
		kills_right->setFont(smallfont_outline);
		kills_right->setSize(SDL_Rect{446, 226, 144, 182});
		kills_right->setActualSize(SDL_Rect{0, 0, 144, 182});
		kills_right->setListOffset(SDL_Rect{0, 4, 0, 0});
		kills_right->setEntrySize(28);
		kills_right->setBorder(0);
		kills_right->setColor(0);

		auto time_and_score_titles = subframe->addField("time_and_score_titles", 256);
		time_and_score_titles->setFont(bigfont_outline);
		time_and_score_titles->setSize(SDL_Rect{350, 408, 194, 60});
		time_and_score_titles->setColor(makeColor(203, 171, 101, 255));
		time_and_score_titles->setHJustify(Field::justify_t::LEFT);
		time_and_score_titles->setVJustify(Field::justify_t::CENTER);
		time_and_score_titles->setText("Time:\nScore:");

		auto time_and_score = subframe->addField("time_and_score", 256);
		time_and_score->setFont(bigfont_outline);
		time_and_score->setSize(SDL_Rect{350, 408, 194, 60});
		time_and_score->setColor(makeColor(203, 171, 101, 255));
		time_and_score->setHJustify(Field::justify_t::RIGHT);
		time_and_score->setVJustify(Field::justify_t::CENTER);

        enum BoardType {
            LOCAL_SINGLE,
            LOCAL_MULTI,
            ONLINE_FRIENDS,
            ONLINE_WORLD
        };
        static BoardType boardType;
        boardType = BoardType::LOCAL_SINGLE;

        static auto updateStats = [](const Button& button, score_t* score){
            if (!score) {
                return;
            }

            assert(main_menu_frame);
            auto window = main_menu_frame->findFrame("leaderboards"); assert(window);
            auto subframe = window->findFrame("subframe"); assert(subframe);
            auto victory_plate_text = subframe->findField("victory_plate_text"); assert(victory_plate_text);

            if (score->victory < 0 || score->victory >= num_victories) {
                return;
            }
            auto& victory = victories[score->victory];

            char victory_text[1024];
            if (boardType == BoardType::LOCAL_SINGLE || boardType == BoardType::LOCAL_MULTI) {
                snprintf(victory_text, sizeof(victory_text), victory.text, score->stats->name);
            } else {
#ifdef STEAMWORKS
                const int index = (int)strtol(button.getName() + 3, nullptr, 10) - 1;
                snprintf(victory_text, sizeof(victory_text), victory.text,
                    g_SteamLeaderboards->leaderBoardSteamUsernames[index].c_str());
#endif
            }

            victory_plate_text->setText(victory_text);
            victory_plate_text->setTextColor(victory.textColor);
            victory_plate_text->setOutlineColor(victory.outlineColor);

            auto victory_plate = subframe->findImage("victory_plate");
            assert(victory_plate);
            victory_plate->path = victory.plate_image;

            auto victory_plate_header = subframe->findImage("victory_plate_header");
            assert(victory_plate_header);
            victory_plate_header->path = victory.header_image;

		    auto conduct = subframe->findFrame("conduct");
		    assert(conduct);
		    conduct->clearEntries();
		    conduct->setWidgetLeft(button.getName());
		    conduct->setActualSize(SDL_Rect{0, 0, 272, 102});
            conduct->setTickCallback([](Widget& widget){
                auto frame = static_cast<Frame*>(&widget);
                frame->setAllowScrollBinds(frame->isSelected());
                });
            
		    auto conduct_header = conduct->addEntry("header", true);
		    conduct_header->text = " Voluntary Challenges:";
		    conduct_header->color = makeColor(203, 171, 101, 255);

		    struct Conduct {
		        bool achieved;
		        const char* name;
		        const char* text;
		    };

		    Conduct conducts[] = {
		        {(bool)score->conductGameChallenges[CONDUCT_CHEATS_ENABLED], "cheats_enabled", u8" \x1E You cheated."},
		        {(bool)score->conductGameChallenges[CONDUCT_MODDED], "modded", u8" \x1E You played with mods."},
		        {(bool)score->conductGameChallenges[CONDUCT_MULTIPLAYER], "multiplayer", u8" \x1E You played with friends."},
		        {(bool)score->conductGameChallenges[CONDUCT_HARDCORE], "hardcore", u8" \x1E You were hardcore."},
		        {(bool)score->conductGameChallenges[CONDUCT_CLASSIC_MODE], "classic_mode", u8" \x1E You played Classic Mode."},
		        {score->conductPenniless, "penniless", u8" \x1E You were penniless."},
		        {score->conductFoodless, "foodless", u8" \x1E You ate nothing."},
		        {score->conductVegetarian &&
		            !score->conductFoodless, "vegetarian", u8" \x1E You were vegetarian."},
		        {score->conductIlliterate, "illiterate", u8" \x1E You were illiterate."},
		        {(bool)score->conductGameChallenges[CONDUCT_BRAWLER], "brawler", u8" \x1E You used no weapons."},
		        {(bool)score->conductGameChallenges[CONDUCT_RANGED_ONLY] &&
		            !(bool)score->conductGameChallenges[CONDUCT_BRAWLER], "ranged_only", u8" \x1E Only used ranged weapons."},
		        {(bool)score->conductGameChallenges[CONDUCT_BLESSED_BOOTS_SPEED], "blessed_boots_speed", u8" \x1E You were extremely quick."},
		        {(bool)score->conductGameChallenges[CONDUCT_BOOTS_SPEED] &&
		            !(bool)score->conductGameChallenges[CONDUCT_BLESSED_BOOTS_SPEED], "boots_speed", u8" \x1E You were very quick."},
		        {(bool)score->conductGameChallenges[CONDUCT_KEEPINVENTORY] &&
		            (bool)score->conductGameChallenges[CONDUCT_MULTIPLAYER], "keep_inventory", u8" \x1E Kept items after death."},
		        {(bool)score->conductGameChallenges[CONDUCT_LIFESAVING], "life_saving", u8" \x1E You had an extra life."},
		        {(bool)score->conductGameChallenges[CONDUCT_ACCURSED], "accursed", u8" \x1E You were accursed."},
		    };
		    constexpr int num_conducts = sizeof(conducts) / sizeof(conducts[0]);

            bool atLeastOneConduct = false;
            for (int c = 0; c < num_conducts; ++c) {
		        if (conducts[c].achieved) {
		            atLeastOneConduct = true;
		            auto entry = conduct->addEntry(conducts[c].name, true);
		            entry->text = conducts[c].text;
		            entry->color = makeColor(203, 171, 101, 255);
		        }
		    }
		    if (!atLeastOneConduct) {
	            auto entry = conduct->addEntry("none", true);
	            entry->text = " None";
	            entry->color = makeColor(203, 171, 101, 255);
		    }

            char buf[1024];

            auto character_title = subframe->findField("character_title");
            assert(character_title);
            snprintf(buf, sizeof(buf), "LVL %d %s %s",
                score->stats->LVL,
                language[3821 + score->stats->playerRace],
                playerClassLangEntry(score->classnum, 0));
            character_title->setText(buf);

            auto character_counters = subframe->findField("character_counters");
            assert(character_counters);
            snprintf(buf, sizeof(buf), "%d/100\n%d\n%d",
                score->stats->EXP,
                score->stats->GOLD,
                score->dungeonlevel);
            character_counters->setText(buf);

            const char* attributes[6] = {"STR", "DEX", "CON", "INT", "PER", "CHR"};
            const Sint32 attr_i[6] = {
                score->stats->STR, score->stats->DEX, score->stats->CON,
                score->stats->INT, score->stats->PER, score->stats->CHR,
            };
		    Field* character_attributes[6];
            for (int c = 0; c < 6; ++c) {
                std::string name = std::string("character_attribute") + std::to_string(c);
		        character_attributes[c] = subframe->findField(name.c_str());
		        assert(character_attributes[c]);
                snprintf(buf, sizeof(buf), "  %s %4d",
                    attributes[c], attr_i[c]);
                character_attributes[c]->setText(buf);
		    }

            auto kills_left = subframe->findFrame("kills_left"); assert(kills_left);
		    kills_left->setActualSize(SDL_Rect{0, 0, 144, 182});
		    kills_left->clearEntries();

            auto kills_right = subframe->findFrame("kills_right"); assert(kills_right);
		    kills_right->setActualSize(SDL_Rect{0, 0, 144, 182});
		    kills_right->clearEntries();

            bool noKillsAtAll = true;
            auto kills = kills_left;
            for (int c = 0; c < NUMMONSTERS; ++c) {
                int num_kills = score->kills[c];
                if (c == LICH_FIRE || c == LICH_ICE) {
                    continue;
                }
                if (c == LICH) {
                    num_kills += score->kills[LICH_FIRE] + score->kills[LICH_ICE];
                }
                if (num_kills <= 0) {
                    continue;
                }
                auto name = num_kills == 1 ?
                    getMonsterLocalizedName((Monster)c) :
                    getMonsterLocalizedPlural((Monster)c);
                snprintf(buf, sizeof(buf), "%3d %s", num_kills, name.c_str());
                auto kill = kills->addEntry(buf, true);
                kill->color = makeColor(203, 171, 101, 255);
                kill->text = buf;
                kill->clickable = (kills == kills_left);
                kills = (kills == kills_left) ? kills_right : kills_left;
                noKillsAtAll = false;
            }
            if (noKillsAtAll) {
                auto entry = kills_left->addEntry("no_kills", true);
                entry->color = makeColor(151, 115, 58, 255);
                entry->text = " None";
            }

            const Uint32 time = score->completionTime / TICKS_PER_SECOND;
            const Uint32 hour = time / 3600;
            const Uint32 min = (time / 60) % 60;
            const Uint32 sec = time % 60;

            int total_score = totalScore(score);

		    auto time_and_score = subframe->findField("time_and_score");
		    assert(time_and_score);
            snprintf(buf, sizeof(buf), "%.2u:%.2u:%.2u\n%d",
                hour, min, sec, total_score);
            time_and_score->setText(buf);
            };

        static const char* categories[] = {
	        "None",
	        "Fastest Time\nNormal", "Highest Score\nNormal",
	        "Fastest Time\nMultiplayer", "Highest Score\nMultiplayer",
	        "Fastest Time\nHell Route", "Highest Score\nHell Route",
	        "Fastest Time\nHardcore", "Highest Score\nHardcore",
	        "Fastest Time\nClassic", "Highest Score\nClassic",
	        "Fastest Time\nClassic Hardcore", "Highest Score\nClassic Hardcore",
	        "Fastest Time\nMultiplayer Classic", "Highest Score\nMultiplayer Classic",
	        "Fastest Time\nMultiplayer Hell Route", "Highest Score\nMultiplayer Hell Route",
	        "Fastest Time\nNormal - Monsters Only", "Highest Score\nNormal - Monsters Only",
	        "Fastest Time\nMultiplayer - Monsters Only", "Highest Score\nMultiplayer - Monsters Only",
	        "Fastest Time\nHell Route - Monsters Only", "Highest Score\nHell Route - Monsters Only",
	        "Fastest Time\nHardcore - Monsters Only", "Highest Score\nHardcore - Monsters Only",
	        "Fastest Time\nClassic - Monsters Only", "Highest Score\nClassic - Monsters Only",
	        "Fastest Time\nClassic Hardcore - Monsters Only", "Highest Score\nClassic Hardcore - Monsters Only",
            "Fastest Time\nMultiplayer Classic - Monsters Only", "Highest Score\nMultiplayer Classic - Monsters Only",
	        "Fastest Time\nMultiplayer Hell Route - Monsters Only", "Highest Score\nMultiplayer Hell Route - Monsters Only",
        };
        static constexpr int num_categories = sizeof(categories) / sizeof(categories[0]);
        static int category;
        category = 1;
        
        static auto set_links = [](const char* name){
            assert(main_menu_frame);
            auto window = main_menu_frame->findFrame("leaderboards"); assert(window);
            auto list = window->findFrame("list"); assert(list);
            auto category_right = window->findButton("category_right"); assert(category_right);
            auto category_left = window->findButton("category_left"); assert(category_left);
            auto delete_entry = window->findButton("delete_entry"); assert(delete_entry);
            auto slider = window->findSlider("scroll_slider"); assert(slider);
            auto subframe = window->findFrame("subframe"); assert(subframe);
            auto conduct = subframe->findFrame("conduct"); assert(conduct);
            category_right->setWidgetUp(name);
            category_left->setWidgetUp(name);
            delete_entry->setWidgetUp(name);
            slider->setWidgetRight(name);
            conduct->setWidgetLeft(name);
            };

        static const char* fmt = "  #%d %s";

        static auto add_score = [](score_t* score, const char* name, const char* prev, const char* next, int index){
            auto window = main_menu_frame->findFrame("leaderboards"); assert(window);
            auto list = window->findFrame("list"); assert(list);

            const int y = 6 + 38 * index;

            char buf[128];
            snprintf(buf, sizeof(buf), fmt, index + 1, name);
            
            if (index == 0) {
                set_links(buf);
            }

            auto button = list->addButton(buf);
            button->setUserData(score);
            button->setHJustify(Button::justify_t::LEFT);
            button->setVJustify(Button::justify_t::CENTER);
            button->setFont(smallfont_outline);
            button->setText(buf);
            button->setColor(makeColor(255,255,255,255));
            button->setHighlightColor(makeColor(255,255,255,255));
            button->setTextColor(makeColor(203,171,101,255));
            button->setTextHighlightColor(makeColor(231,213,173,255));
            button->setGlyphPosition(Widget::glyph_position_t::CENTERED_RIGHT);
            button->setHideGlyphs(true);
            button->setWidgetSearchParent("leaderboards");
            button->addWidgetAction("MenuCancel", "back_button");
            button->addWidgetAction("MenuAlt1", "delete_entry");
            button->addWidgetAction("MenuPageLeft", "tab_left");
            button->addWidgetAction("MenuPageRight", "tab_right");
            button->addWidgetAction("MenuPageLeftAlt", "category_left");
            button->addWidgetAction("MenuPageRightAlt", "category_right");
            button->setWidgetRight("conduct");
            button->setWidgetUp(prev ? prev : "");
            button->setWidgetDown(next ? next : "");
            button->setSize(SDL_Rect{0, y, 278, 36});
            button->setBackground("*images/ui/Main Menus/Leaderboards/AA_NameList_Unselected_00.png");
            button->setBackgroundHighlighted("*images/ui/Main Menus/Leaderboards/AA_NameList_Selected_00.png");
            button->setBackgroundActivated("*images/ui/Main Menus/Leaderboards/AA_NameList_Selected_00.png");
            button->setCallback([](Button& button){
                if (isMouseVisible()) {
                    soundActivate();
                }
                auto list = static_cast<Frame*>(button.getParent());
                button.setTextColor(makeColor(231,213,173,255));
                button.setBackground("*images/ui/Main Menus/Leaderboards/AA_NameList_Selected_00.png");
                for (auto b : list->getButtons()) {
                    if (b == &button) {
                        continue;
                    }
                    b->setTextColor(makeColor(203,171,101,255));
                    b->setBackground("*images/ui/Main Menus/Leaderboards/AA_NameList_Unselected_00.png");
                }
                auto score = (score_t*)button.getUserData();
                selectedScore = score;
                updateStats(button, score);
                loadScore(score);
                });
            button->setTickCallback([](Widget& widget){
                auto button = static_cast<Button*>(&widget);
                auto list = static_cast<Frame*>(button->getParent());
                if (button->isSelected()) {
                    if (button->getSize().y < list->getActualSize().y) {
                        auto next = button->getWidgetMovements().find("MenuDown");
                        if (next != button->getWidgetMovements().end() && !next->second.empty()) {
                            auto result = list->findButton(next->second.c_str());
                            if (result) {
                                result->select();
                            }
                        }
                    }
                    if (button->getSize().y + button->getSize().h > list->getActualSize().y + list->getSize().h) {
                        auto next = button->getWidgetMovements().find("MenuUp");
                        if (next != button->getWidgetMovements().end() && !next->second.empty()) {
                            auto result = list->findButton(next->second.c_str());
                            if (result) {
                                result->select();
                            }
                        }
                    }
                    if (!isMouseVisible()) {
                        const char* unselected = "*images/ui/Main Menus/Leaderboards/AA_NameList_Unselected_00.png";
                        if (strcmp(button->getBackground(), unselected) == 0) {
                            button->activate();
                        }
                    }
                }
                });

            if (index == 0) {
                button->select();
                selectedScore = score;
                updateStats(*button, score);
                loadScore(score);
            }

            auto size = list->getActualSize();
            size.h = std::max(list->getSize().h, y + 38);
            list->setActualSize(size);
            };

        struct DownloadedScores {
            void deleteAll() {
                for (auto score : scores) {
                    scoreDeconstructor(score);
                }
                scores.clear();
            }
            DownloadedScores() = default;
            ~DownloadedScores() {
                deleteAll();
            }
            std::vector<score_t*> scores;
        };
        static DownloadedScores downloadedScores;
        static int scores_loaded;
        scores_loaded = 0;

        static auto repopulate_list = [](BoardType type){
            downloadedScores.deleteAll();
            selectedScore = nullptr;
            boardType = type;

            auto window = main_menu_frame->findFrame("leaderboards"); assert(window);
            auto list = window->findFrame("list"); assert(list);
            list->clear();

            auto size = list->getActualSize();
            size.h = list->getSize().h;
            size.y = 0;
            list->setActualSize(size);

            if (boardType == BoardType::LOCAL_SINGLE || boardType == BoardType::LOCAL_MULTI) {
                auto scores = boardType == BoardType::LOCAL_SINGLE ?
                    &topscores : &topscoresMultiplayer;
                if (scores->first) {
                    (void)window->remove("wait_message");
                    int index = 0;
                    for (auto node = scores->first; node != nullptr;
                        node = node->next, ++index) {
                        auto score = (score_t*)node->element;
                        char prev_buf[128] = "";
                        if (node->prev) {
                            auto prev = (score_t*)node->prev->element;
                            snprintf(prev_buf, sizeof(prev_buf), fmt, index, prev->stats->name);
                        } else {
							auto prev = (score_t*)node->list->last->element;
							snprintf(prev_buf, sizeof(prev_buf), fmt, list_Size(scores), prev->stats->name);
						}
                        char next_buf[128] = "";
                        if (node->next) {
                            auto next = (score_t*)node->next->element;
                            snprintf(next_buf, sizeof(next_buf), fmt, index + 2, next->stats->name);
                        } else {
							auto next = (score_t*)node->list->first->element;
							snprintf(next_buf, sizeof(next_buf), fmt, 1, next->stats->name);
						}
                        add_score(score, score->stats->name, prev_buf, next_buf, index);
                    }
                } else {
                    auto field = window->findField("wait_message");
                    if (!field) {
                        field = window->addField("wait_message", 1024);
                        field->setFont(bigfont_outline);
                        field->setText("No scores found.");
                        field->setSize(SDL_Rect{30, 148, 932, 468});
                        field->setJustify(Field::justify_t::CENTER);
                    } else {
                        field->setText("No scores found.");
                    }
                    set_links("");
                }
            } else {
#ifdef STEAMWORKS
                scores_loaded = 0;
                g_SteamLeaderboards->FindLeaderboard(
                    CSteamLeaderboards::leaderboardNames[category].c_str());
                auto field = window->findField("wait_message");
                if (!field) {
                    field = window->addField("wait_message", 1024);
                    field->setFont(bigfont_outline);
                    field->setText("Downloading scores...");
                    field->setSize(SDL_Rect{30, 148, 932, 468});
                    field->setJustify(Field::justify_t::CENTER);
                } else {
                    field->setText("Downloading scores...");
                }
                set_links("");
#endif
            }
            };

        auto disableIfNotOnline = [](Widget& widget){
            bool invisible = boardType == BoardType::LOCAL_SINGLE ||
                boardType == BoardType::LOCAL_MULTI;
            widget.setInvisible(invisible);
            auto window = static_cast<Frame*>(widget.getParent());
            auto category_panel = window->findImage("category_panel");
            if (category_panel) {
                category_panel->disabled = invisible;
            }
            };

        auto category_panel = window->addImage(
            SDL_Rect{
                (window->getSize().w - 400) / 2,
                630, 400, 62,
            },
            0xffffffff,
            "*images/ui/Main Menus/Leaderboards/AA_Plate_ModeSelector_00.png",
            "category_panel"
        );
        category_panel->disabled = true;

        auto panel_pos = category_panel->pos;

        auto category_text = window->addField("category_text", 256);
        category_text->setSize(panel_pos);
        category_text->setJustify(Field::justify_t::CENTER);
        category_text->setFont(smallfont_outline);
        category_text->setColor(makeColor(170, 134, 102, 255));
        category_text->setText(categories[category]);
        category_text->setTickCallback(disableIfNotOnline);
        category_text->setInvisible(true);

        auto category_left = window->addButton("category_left");
        category_left->setSize(SDL_Rect{panel_pos.x - 24, 646, 20, 30});
		category_left->setBackground("*images/ui/Main Menus/Leaderboards/AA_Button_LArrowTiny_00.png");
		category_left->setBackgroundHighlighted("*images/ui/Main Menus/Leaderboards/AA_Button_LArrowTinyHigh_00.png");
		category_left->setBackgroundActivated("*images/ui/Main Menus/Leaderboards/AA_Button_LArrowTinyPress_00.png");
		category_left->setColor(makeColor(255, 255, 255, 255));
		category_left->setHighlightColor(makeColor(255, 255, 255, 255));
		category_left->setWidgetSearchParent(window->getName());
		category_left->setTickCallback(disableIfNotOnline);
		category_left->setCallback([](Button& button){
            if (!button.isInvisible()) {
                soundActivate();
                --category;
                if (category <= 0) {
                    category = num_categories - 1;
                }
                repopulate_list(boardType);
                auto window = static_cast<Frame*>(button.getParent());
                auto category_text = window->findField("category_text");
                if (category_text) {
                    category_text->setText(categories[category]);
                }
            }
		    });
        category_left->addWidgetAction("MenuCancel", "back_button");
        category_left->addWidgetAction("MenuAlt1", "delete_entry");
        category_left->addWidgetAction("MenuPageLeft", "tab_left");
        category_left->addWidgetAction("MenuPageRight", "tab_right");
        category_left->addWidgetAction("MenuPageLeftAlt", "category_left");
        category_left->addWidgetAction("MenuPageRightAlt", "category_right");
        category_left->setWidgetRight("category_right");
        category_left->setInvisible(true);

        auto category_right = window->addButton("category_right");
        category_right->setSize(SDL_Rect{panel_pos.x + panel_pos.w + 4, 646, 20, 30});
		category_right->setBackground("*images/ui/Main Menus/Leaderboards/AA_Button_RArrowTiny_00.png");
		category_right->setBackgroundHighlighted("*images/ui/Main Menus/Leaderboards/AA_Button_RArrowTinyHigh_00.png");
		category_right->setBackgroundActivated("*images/ui/Main Menus/Leaderboards/AA_Button_RArrowTinyPress_00.png");
		category_right->setColor(makeColor(255, 255, 255, 255));
		category_right->setHighlightColor(makeColor(255, 255, 255, 255));
		category_right->setTickCallback(disableIfNotOnline);
		category_right->setCallback([](Button& button){
            if (!button.isInvisible()) {
                soundActivate();
                ++category;
                if (category >= num_categories) {
                    category = 1;
                }
                repopulate_list(boardType);
                auto window = static_cast<Frame*>(button.getParent());
                auto category_text = window->findField("category_text");
                if (category_text) {
                    category_text->setText(categories[category]);
                }
            }
		    });
		category_right->setWidgetSearchParent(window->getName());
        category_right->addWidgetAction("MenuCancel", "back_button");
        category_right->addWidgetAction("MenuAlt1", "delete_entry");
        category_right->addWidgetAction("MenuPageLeft", "tab_left");
        category_right->addWidgetAction("MenuPageRight", "tab_right");
        category_right->addWidgetAction("MenuPageLeftAlt", "category_left");
        category_right->addWidgetAction("MenuPageRightAlt", "category_right");
        category_right->setWidgetRight("delete_entry");
        category_right->setWidgetLeft("category_left");
        category_right->setInvisible(true);

        // poll for downloaded scores
#ifdef STEAMWORKS
        list->setTickCallback([](Widget& widget){
            if (boardType != BoardType::ONLINE_FRIENDS &&
                boardType != BoardType::ONLINE_WORLD) {
                return;
            }
            auto window = static_cast<Frame*>(widget.getParent());
            if (scores_loaded == 0 && g_SteamLeaderboards->b_LeaderboardInit) {
                scores_loaded++;
                g_SteamLeaderboards->DownloadScores(
                    boardType == BoardType::ONLINE_FRIENDS ?
                    k_ELeaderboardDataRequestFriends :
                    k_ELeaderboardDataRequestGlobal,
                    0, CSteamLeaderboards::k_numEntriesToRetrieve);
            }
            else if (scores_loaded == 1 && g_SteamLeaderboards->b_ScoresDownloaded) {
                scores_loaded++;
                if (g_SteamLeaderboards->m_nLeaderboardEntries == 0) {
                    auto field = window->findField("wait_message");
                    if (!field) {
                        field = window->addField("wait_message", 1024);
                        field->setFont(bigfont_outline);
                        field->setText("No scores found.");
                        field->setSize(SDL_Rect{30, 148, 932, 468});
                        field->setJustify(Field::justify_t::CENTER);
                    } else {
                        field->setText("No scores found.");
                    }
                } else {
                    (void)window->remove("wait_message");
                    int num_scores = g_SteamLeaderboards->m_nLeaderboardEntries;
                    for (int index = 0; index < num_scores; ++index) {
                        steamLeaderboardReadScore(g_SteamLeaderboards->downloadedTags[index]);
                        auto score = scoreConstructor(clientnum);
                        downloadedScores.scores.push_back(score);
                        auto name = g_SteamLeaderboards->leaderBoardSteamUsernames[index].c_str();
                        char prev_buf[128] = "";
                        if (index > 0) {
                            snprintf(prev_buf, sizeof(prev_buf), fmt, index,
                                g_SteamLeaderboards->leaderBoardSteamUsernames[index - 1].c_str());
                        } else {
							snprintf(prev_buf, sizeof(prev_buf), fmt, num_scores,
								g_SteamLeaderboards->leaderBoardSteamUsernames[num_scores - 1].c_str());
						}
                        char next_buf[128] = "";
                        if (index < num_scores - 1) {
                            snprintf(next_buf, sizeof(next_buf), fmt, index + 2,
                                g_SteamLeaderboards->leaderBoardSteamUsernames[index + 1].c_str());
                        } else {
							snprintf(next_buf, sizeof(next_buf), fmt, 1,
								g_SteamLeaderboards->leaderBoardSteamUsernames[0].c_str());
						}
                        add_score(score, name, prev_buf, next_buf, index);
                    }
                    if (num_scores == 0) {
                        set_links("");
                    }
                }
            }
            });
#endif

#define TAB_FN(X) [](Button& button){\
    soundActivate();\
    button.select();\
    repopulate_list(X);\
}

        struct Tab {
            const char* name;
            const char* text;
            void (*func)(Button& button);
        };
        static const Tab tabs[] = {
            {"local", "Local\nSingleplayer", TAB_FN(BoardType::LOCAL_SINGLE)},
            {"lan", "Local\nMultiplayer", TAB_FN(BoardType::LOCAL_MULTI)},
#ifdef STEAMWORKS
            // TODO for now these are disabled, @wallofjustice make better leaderboards in future
            //{"friends", "Leaderboard\nFriends", TAB_FN(BoardType::ONLINE_FRIENDS)},
            //{"world", "Leaderboard\nWorld", TAB_FN(BoardType::ONLINE_WORLD)},
#endif
        };
        static constexpr int num_tabs = sizeof(tabs) / sizeof(tabs[0]);
        for (int c = 0; c < num_tabs; ++c) {
            Button* tab = window->addButton(tabs[c].name);
            tab->setText(tabs[c].text);
            tab->setFont(bigfont_outline);
			tab->setBackground("*images/ui/Main Menus/Leaderboards/AA_Button_Subtitle_Unselected_00.png");
			tab->setBackgroundHighlighted("*images/ui/Main Menus/Leaderboards/AA_Button_Subtitle_UnselectedHigh_00.png");
			tab->setBackgroundActivated("*images/ui/Main Menus/Leaderboards/AA_Button_Subtitle_UnselectedPress_00.png");
			tab->setColor(makeColor(255, 255, 255, 255));
			tab->setHighlightColor(makeColor(255, 255, 255, 255));
			tab->setCallback(tabs[c].func);

			tab->setTickCallback([](Widget& widget){
			    auto tab = static_cast<Button*>(&widget);
			    int index = 0;
			    for (; index < num_tabs; ++index) {
			        if (!strcmp(tabs[index].name, tab->getName())) {
			            break;
			        }
			    }
			    if (index == (int)boardType) {
					tab->setBackground("*images/ui/Main Menus/Leaderboards/AA_Button_Subtitle_Selected_00.png");
					tab->setBackgroundHighlighted("*images/ui/Main Menus/Leaderboards/AA_Button_Subtitle_SelectedHigh_00.png");
					tab->setBackgroundActivated("*images/ui/Main Menus/Leaderboards/AA_Button_Subtitle_SelectedPress_00.png");
                    if (!main_menu_frame->findSelectedWidget(widget.getOwner())) {
                        widget.select(); // rescue focus
                    }
			    } else {
					tab->setBackground("*images/ui/Main Menus/Leaderboards/AA_Button_Subtitle_Unselected_00.png");
					tab->setBackgroundHighlighted("*images/ui/Main Menus/Leaderboards/AA_Button_Subtitle_UnselectedHigh_00.png");
					tab->setBackgroundActivated("*images/ui/Main Menus/Leaderboards/AA_Button_Subtitle_UnselectedPress_00.png");
			    }
			    });

            constexpr int fullw = 184 * num_tabs + 20 * (num_tabs - 1);
            constexpr int xbegin = (992 - fullw) / 2;
            const int x = xbegin + (184 + 20) * c;
            tab->setSize(SDL_Rect{x, 70, 184, 64});

		    tab->setWidgetSearchParent(window->getName());
            tab->addWidgetAction("MenuCancel", "back_button");
            tab->addWidgetAction("MenuAlt1", "delete_entry");
            tab->addWidgetAction("MenuPageLeft", "tab_left");
            tab->addWidgetAction("MenuPageRight", "tab_right");
            tab->addWidgetAction("MenuPageLeftAlt", "category_left");
            tab->addWidgetAction("MenuPageRightAlt", "category_right");
            if (c > 0) {
                tab->setWidgetLeft(tabs[c - 1].name);
            }
            if (c < num_tabs - 1) {
                tab->setWidgetRight(tabs[c + 1].name);
            }
        }

		auto tab_left = window->addButton("tab_left");
		tab_left->setSize(SDL_Rect{40, 72, 38, 58});
		tab_left->setBackground("*images/ui/Main Menus/Leaderboards/AA_Button_LArrow_00.png");
		tab_left->setBackgroundHighlighted("*images/ui/Main Menus/Leaderboards/AA_Button_LArrowHigh_00.png");
		tab_left->setBackgroundActivated("*images/ui/Main Menus/Leaderboards/AA_Button_LArrowPress_00.png");
		tab_left->setColor(makeColor(255, 255, 255, 255));
		tab_left->setHighlightColor(makeColor(255, 255, 255, 255));
		tab_left->setGlyphPosition(Widget::glyph_position_t::BOTTOM_LEFT);
		tab_left->setCallback([](Button& button){
		    auto window = static_cast<Frame*>(button.getParent());
            int tab_index = static_cast<int>(boardType);
            if (tab_index > 0) {
                auto tab = window->findButton(tabs[tab_index - 1].name); assert(tab);
                tab->activate();
            }
		    });
		tab_left->setWidgetSearchParent(window->getName());
        tab_left->addWidgetAction("MenuCancel", "back_button");
        tab_left->addWidgetAction("MenuAlt1", "delete_entry");
        tab_left->addWidgetAction("MenuPageLeft", "tab_left");
        tab_left->addWidgetAction("MenuPageRight", "tab_right");
        tab_left->addWidgetAction("MenuPageLeftAlt", "category_left");
        tab_left->addWidgetAction("MenuPageRightAlt", "category_right");
        tab_left->setWidgetRight(tabs[0].name);

		auto tab_right = window->addButton("tab_right");
		tab_right->setSize(SDL_Rect{914, 72, 38, 58});
		tab_right->setBackground("*images/ui/Main Menus/Leaderboards/AA_Button_RArrow_00.png");
		tab_right->setBackgroundHighlighted("*images/ui/Main Menus/Leaderboards/AA_Button_RArrowHigh_00.png");
		tab_right->setBackgroundActivated("*images/ui/Main Menus/Leaderboards/AA_Button_RArrowPress_00.png");
		tab_right->setColor(makeColor(255, 255, 255, 255));
		tab_right->setHighlightColor(makeColor(255, 255, 255, 255));
		tab_right->setGlyphPosition(Widget::glyph_position_t::BOTTOM_RIGHT);
		tab_right->setCallback([](Button& button){
		    auto window = static_cast<Frame*>(button.getParent());
            int tab_index = static_cast<int>(boardType);
            if (tab_index < num_tabs - 1) {
                auto tab = window->findButton(tabs[tab_index + 1].name); assert(tab);
                tab->activate();
            }
		    });
		tab_right->setWidgetSearchParent(window->getName());
        tab_right->addWidgetAction("MenuCancel", "back_button");
        tab_right->addWidgetAction("MenuAlt1", "delete_entry");
        tab_right->addWidgetAction("MenuPageLeft", "tab_left");
        tab_right->addWidgetAction("MenuPageRight", "tab_right");
        tab_right->addWidgetAction("MenuPageLeftAlt", "category_left");
        tab_right->addWidgetAction("MenuPageRightAlt", "category_right");
        tab_right->setWidgetLeft(tabs[num_tabs - 1].name);

        auto slider = window->addSlider("scroll_slider");
        slider->setRailSize(SDL_Rect{38, 170, 30, 420});
        slider->setHandleSize(SDL_Rect{0, 0, 34, 34});
		slider->setRailImage("*images/ui/Main Menus/Leaderboards/AA_Scroll_Bar_00.png");
		slider->setHandleImage("*images/ui/Main Menus/Leaderboards/AA_Scroll_Slider_00.png");
		slider->setOrientation(Slider::orientation_t::SLIDER_VERTICAL);
		slider->setBorder(24);
		slider->setValue(0.f);
		slider->setMinValue(0.f);
		slider->setCallback([](Slider& slider){
			Frame* frame = static_cast<Frame*>(slider.getParent());
			Frame* list = frame->findFrame("list"); assert(list);
			auto actualSize = list->getActualSize();
			actualSize.y = slider.getValue();
			list->setActualSize(actualSize);
			});
		slider->setTickCallback([](Widget& widget){
			Slider* slider = static_cast<Slider*>(&widget);
			Frame* frame = static_cast<Frame*>(slider->getParent());
			Frame* list = frame->findFrame("list"); assert(list);
			auto actualSize = list->getActualSize();
			slider->setValue(actualSize.y);
		    slider->setMaxValue((float)std::max(0, actualSize.h - list->getSize().h));
			});
		slider->setWidgetSearchParent(window->getName());
        slider->setWidgetSearchParent("leaderboards");
        slider->addWidgetAction("MenuCancel", "back_button");
        slider->addWidgetAction("MenuAlt1", "delete_entry");
        slider->addWidgetAction("MenuPageLeft", "tab_left");
        slider->addWidgetAction("MenuPageRight", "tab_right");
        slider->addWidgetAction("MenuPageLeftAlt", "category_left");
        slider->addWidgetAction("MenuPageRightAlt", "category_right");

		auto delete_entry = window->addButton("delete_entry");
		delete_entry->setSize(SDL_Rect{740, 630, 164, 62});
		delete_entry->setBackground("*images/ui/Main Menus/Leaderboards/AA_Button_00.png");
		delete_entry->setBackgroundHighlighted("*images/ui/Main Menus/Leaderboards/AA_ButtonHigh_00.png");
		delete_entry->setBackgroundActivated("*images/ui/Main Menus/Leaderboards/AA_ButtonPress_00.png");
		delete_entry->setColor(makeColor(255, 255, 255, 255));
		delete_entry->setHighlightColor(makeColor(255, 255, 255, 255));
		delete_entry->setGlyphPosition(Widget::glyph_position_t::CENTERED_BOTTOM);
		delete_entry->setFont(smallfont_outline);
		delete_entry->setText("Delete Entry");
		delete_entry->setTickCallback([](Widget& widget){
            if (boardType == BoardType::LOCAL_SINGLE || boardType == BoardType::LOCAL_MULTI) {
                auto scores = boardType == BoardType::LOCAL_SINGLE ?
                    &topscores : &topscoresMultiplayer;
                widget.setInvisible(scores->first == nullptr);
            }
            else if (boardType == BoardType::ONLINE_FRIENDS || boardType == BoardType::ONLINE_WORLD) {
                widget.setInvisible(true);
            }
		    });
		delete_entry->setCallback([](Button& button){
            if (boardType != BoardType::LOCAL_SINGLE && boardType != BoardType::LOCAL_MULTI) {
                // don't ever delete online scores
                return;
            }
            if (selectedScore) {
                char prompt[1024];
                const char* fmt = "Are you sure you want to delete\n\"%s\"?";
                snprintf(prompt, sizeof(prompt), fmt, selectedScore->stats->name);
                binaryPrompt(
                    prompt,
                    "Yes", "No",
                    [](Button& button){ // Yes
                        soundActivate();
		                soundDeleteSave();
		                assert(main_menu_frame);
		                auto leaderboards = main_menu_frame->findFrame("leaderboards");
		                auto list = leaderboards->findFrame("list");

		                int index = 0;
		                for (auto b : list->getButtons()) {
		                    if (b->getUserData() == selectedScore) {
		                        break;
		                    }
		                    ++index;
		                }
		                (void)deleteScore(boardType == BoardType::LOCAL_MULTI, index);
                        saveAllScores(boardType == BoardType::LOCAL_MULTI ? SCORESFILE_MULTIPLAYER : SCORESFILE);
		                repopulate_list(boardType);
		                closeBinary();
                        },
                    [](Button& button){ // No
		                soundCancel();
		                closeBinary();
                        assert(main_menu_frame);
                        auto window = main_menu_frame->findFrame("leaderboards"); assert(window);
                        auto list = window->findFrame("list"); assert(list);
                        for (auto button : list->getButtons()) {
                            auto score = (score_t*)button->getUserData();
                            if (score == selectedScore) {
                                button->select();
                                break;
                            }
                        }
                        });
                auto scores = boardType == BoardType::LOCAL_SINGLE ?
                    &topscores : &topscoresMultiplayer;
            } else {
                errorPrompt(
                    "Please select a score\nto delete first", "Okay",
                    [](Button& button){
		                soundCancel();
		                repopulate_list(boardType);
		                closeMono();
                        });
            }
		    });
		delete_entry->setWidgetSearchParent(window->getName());
        delete_entry->addWidgetAction("MenuCancel", "back_button");
        delete_entry->addWidgetAction("MenuAlt1", "delete_entry");
        delete_entry->addWidgetAction("MenuPageLeft", "tab_left");
        delete_entry->addWidgetAction("MenuPageRight", "tab_right");
        delete_entry->addWidgetAction("MenuPageLeftAlt", "category_left");
        delete_entry->addWidgetAction("MenuPageRightAlt", "category_right");
        delete_entry->setWidgetLeft("category_right");

        Button* tab = window->findButton(tabs[0].name);
        tab->select();
        tab->activate();
    }

	static void createSimpleAchievementsWindow() {
		soundActivate();

		if ( achievementsNeedResort )
		{
			sortAchievementsForDisplay();
		}

		auto window = genericWindow("achievements", "ACHIEVEMENTS", false);
		assert(window);

		auto back_button = createBackWidget(window,[](Button& button){
			soundCancel();
			auto frame = static_cast<Frame*>(button.getParent());
			frame = static_cast<Frame*>(frame->getParent());
			frame = static_cast<Frame*>(frame->getParent());
			frame->removeSelf();
			if (!main_menu_frame) {
				return;
			}
			auto buttons = main_menu_frame->findFrame("buttons"); assert(buttons);
			auto achievements = buttons->findButton("Achievements"); assert(achievements);
			achievements->select();
			});
		back_button->select();

		auto subwindow = window->findFrame("subwindow");
		assert(subwindow);

		auto slider = subwindow->findSlider("scroll_slider"); assert(slider);
		slider->setWidgetBack("back_button");

		int y = 0;

		// count all the different types of achievements
		const int num_achievements = (int)achievementNames.size();
		int num_unlocked = 0;
		int num_locked = 0;
		int num_hidden = 0;
		for (auto& item : achievementNames) {
			if (achievementUnlocked(item.first.c_str())) {
				++num_unlocked;
			} else {
				++num_locked;
				if (achievementHidden.find(item.first.c_str()) != achievementHidden.end()) {
					++num_hidden;
				}
			}
		}

		// set tooltip text
		char tooltip_buf[256] = { '\0' };
		const int percent = (num_unlocked * 100) / num_achievements;
		snprintf(tooltip_buf, sizeof(tooltip_buf), "Unlocked %d / %d achievements (%d%%)",
			num_unlocked, num_achievements, percent);
		auto tooltip = window->findField("tooltip"); assert(tooltip);
		tooltip->setText(tooltip_buf);

		const char* explanation_text =
			"Complete optional in-game challenges\nfor fun and bragging rights";

		// explanation text
		auto explanation = window->addField("explanation", 256);
		explanation->setSize(SDL_Rect{74, 624, 680, 72});
		explanation->setFont(bigfont_outline);
		explanation->setTextColor(makeColor(170, 134, 102, 255));
		explanation->setOutlineColor(makeColor(29, 16, 11, 255));
		explanation->setText(explanation_text);
		explanation->setJustify(Field::justify_t::CENTER);

		// function to add achievement to the list
		auto add_achievement = [](
			Frame& subwindow,
			const char* name,
			int num_hidden,
			bool locked,
			int statisticUpdateCurrent,
			int statisticUpdateMax,
			int y) {
			const SDL_Rect r{4, y, subwindow.getSize().w - 56, 80};
			auto frame = subwindow.addFrame(name ? name : "hidden achievements");
			frame->setHollow(true);
			frame->setBorder(0);
			frame->setSize(r);
			createGenericWindowDecorations(*frame);
            SDL_Rect r2 = r; r2.x = 0; r2.y = 0;
            sizeWindowDecorations(*frame, r2);

			const char* achName = nullptr;
			const char* achDesc = nullptr;
			if (name) {
				auto achNameFind = achievementNames.find(name);
				if (achNameFind != achievementNames.end()) {
					achName = achNameFind->second.c_str();
				}

				auto achDescFind = achievementDesc.find(name);
				if (achDescFind != achievementDesc.end()) {
					achDesc = achDescFind->second.c_str();
				}
			}

			// achievement title
			const char* title = achName ? achName :
				(num_hidden ?
				"Hidden achievements":
				"Hidden achievement");
			auto headerField = frame->addField("header", 64);
			headerField->setFont(smallfont_outline);
			headerField->setColor(makeColorRGB(255, 255, 0));
			headerField->setSize(SDL_Rect{80, 8, r.w - 80, r.h - 8});
			headerField->setHJustify(Field::justify_t::LEFT);
			headerField->setVJustify(Field::justify_t::TOP);
			headerField->setText(title);

			// description
			constexpr int longest_line = 62;
			int offset = 0;
			char buf[256];
			if (name) {
				assert(achDesc);
				snprintf(buf, sizeof(buf), "%s", achDesc);
				for (int c = 0; c < sizeof(buf) && buf[c] != '\0'; ++c) {
					if (c >= longest_line && buf[c] == ' ') {
						offset += 8;
						buf[c] = '\n';
						break;
					}
				}
			} else {
				const char* fmt = num_hidden > 1 ?
					"%d additional achievements remain...":
					"%d additional achievement remains...";
				snprintf(buf, sizeof(buf), fmt, num_hidden);
			}
			auto mainField = frame->addField("main", 256);
			mainField->setFont(smallfont_outline);
			mainField->setColor(makeColor(255, 255, 255, 255));
			mainField->setSize(SDL_Rect{80, 8 + offset, r.w - 80, r.h - 8 - offset});
			mainField->setHJustify(Field::justify_t::LEFT);
			mainField->setVJustify(Field::justify_t::CENTER);
			mainField->setText(buf);

			if (locked) {
				if (statisticUpdateMax > 0) {
					// progress bar
					const int width = 256;
					const int height = 20;
					const SDL_Rect pr{ r.w - width - 8, 8, width, height };
					const int percent = (statisticUpdateCurrent * 100) / statisticUpdateMax;
					const int size = (pr.w * percent) / 100;

					auto progressBarBackground = frame->addImage(pr, makeColorRGB(0, 48, 16),
						"images/system/white.png", "progressBarBackground");
					if (size) {
						const SDL_Rect pbr{ pr.x, pr.y, size < pr.w ? size : pr.w, pr.h };
						auto progressBar = frame->addImage(pbr, makeColorRGB(0, 160, 48),
							"images/system/white.png", "progressBar");
					}

					// progress bar text
					auto progressField = frame->addField("progress", 32);
					progressField->setColor(makeColor(255, 255, 255, 255));
					progressField->setFont(smallfont_outline);

					char progress_str[32] = { '\0' };
					if (statisticUpdateMax > 0) {
						const int len = snprintf(progress_str, sizeof(progress_str), "%d / %d",
							statisticUpdateCurrent < statisticUpdateMax ? statisticUpdateCurrent : statisticUpdateMax, statisticUpdateMax);
					}
					progressField->setInvisible(false);
					progressField->setSize(SDL_Rect{pr.x, pr.y, pr.w, pr.h + 1});
					progressField->setHJustify(Field::justify_t::CENTER);
					progressField->setVJustify(Field::justify_t::CENTER);
					progressField->setText(progress_str);
				}
			} else {
				// unlock time
				assert(name);
				auto it = achievementUnlockTime.find(name);
				if (it != achievementUnlockTime.end()) {
					char buffer[64];
					time_t t = (time_t)it->second;

					char tbuf[64];
					getTimeAndDateFormatted(t, tbuf, sizeof(tbuf));
					snprintf(buffer, sizeof(buffer), "Unlocked %s", tbuf);

					auto unlockField = frame->addField("unlock", 64);
					unlockField->setFont(smallfont_outline);
					unlockField->setColor(makeColorRGB(255, 255, 0));
					unlockField->setSize(SDL_Rect{80, 8, r.w - 84, r.h - 8});
					unlockField->setHJustify(Field::justify_t::RIGHT);
					unlockField->setVJustify(Field::justify_t::TOP);
					unlockField->setText(buffer);
				}
			}

			// image path
			std::string path;
			if (locked) {
				if (name) {
					path = std::string("*#images/achievements/") + name + std::string("_l.png");
				} else {
					path = "*#images/achievements/LOCKED_ACHIEVEMENT.png";
				}
			} else {
				assert(name);
				path = std::string("*#images/achievements/") + name + std::string(".png");
			}
			frame->addImage(SDL_Rect{8, 8, 64, 64}, 0xffffffff, path.c_str(), "ach_image");

			return frame->getSize().h + 4;
			};

		// list unlocked achievements
		if (num_unlocked > 0) {
			y += settingsAddSubHeader(*subwindow, y, "unlocked", "Unlocked", true);
			for (auto& item : achievementNamesSorted) {
				if (!achievementUnlocked(item.first.c_str())) {
					continue;
				}

				int statMax = 0;
				for (auto& it : steamStatAchStringsAndMaxVals) {
					if (it.first == item.first) {
						statMax = it.second;
						break;
					}
				}

				y += add_achievement(*subwindow, item.first.c_str(), num_hidden, false,
					statMax, statMax, y);
			}
		}

		// list locked achievements
		if (num_locked > 0) {
			y += settingsAddSubHeader(*subwindow, y, "locked", "Locked", true);
			for (auto& item : achievementNamesSorted) {
				if (achievementUnlocked(item.first.c_str())) {
					continue;
				}
				if (achievementHidden.find(item.first.c_str()) != achievementHidden.end()) {
					continue;
				}

				int statCur = 0;
				auto progIt = achievementProgress.find(item.first);
				if (progIt != achievementProgress.end()) {
					statCur = g_SteamStats[progIt->second].m_iValue;
				}

				int statMax = 0;
				for (auto& it : steamStatAchStringsAndMaxVals) {
					if (it.first == item.first) {
						statMax = it.second;
						break;
					}
				}

				y += add_achievement(*subwindow, item.first.c_str(), num_hidden, true,
					statCur, statMax, y);
			}
			if (num_hidden) {
				y += add_achievement(*subwindow, nullptr, num_hidden, true,
					0, 0, y);
			}
		}
		
		genericSubwindowFinalizeBasic(*subwindow, y);
	}

/******************************************************************************/

	static void archivesLeaderboards(Button& button) {
		if (0) {
		    // test cutscene
		    soundActivate();
		    destroyMainMenu();
		    createDummyMainMenu();
		    beginFade(MainMenu::FadeDestination::EndingHuman);
		} else {
		    createLeaderboards();
		}
	}

	static void archivesDungeonCompendium(Button& button) {
		soundActivate();
	}

	static void archivesAchievements(Button& button) {
		createSimpleAchievementsWindow();
	}

	static void archivesStoryIntroduction(Button& button) {
		soundActivate();
		destroyMainMenu();
		createDummyMainMenu();
		beginFade(MainMenu::FadeDestination::IntroStoryScreen);
	}

	static void archivesCredits(Button& button) {
		soundActivate();
		destroyMainMenu();
		createDummyMainMenu();
        createCreditsScreen(false);
	}

	static void archivesBackToMainMenu(Button& button) {
		soundCancel();
        destroyMainMenu();
        createMainMenu(false);
		if (!main_menu_frame) {
			return;
		}
		auto buttons = main_menu_frame->findFrame("buttons"); assert(buttons);
		auto selection = buttons->findButton("Adventure Archives");
		if (selection) {
			selection->select();
		}
	}

/******************************************************************************/

    static Frame* toggleLobbyChatWindow();

    struct LobbyChatMessage {
        Uint32 timestamp;
        Uint32 color;
        std::string msg;
    };

    static Uint32 new_lobby_chat_message_alert = 0;
    static const constexpr int lobby_chat_max_messages = 200;
    static std::list<LobbyChatMessage> lobby_chat_messages;
    static ConsoleVariable<std::string> lobby_chat_font("/chat_font",
        "fonts/PixelMaz_monospace.ttf#32#2");

    static void addLobbyChatMessage(Uint32 color, const char* msg, bool add_to_list = true) {
		if (currentLobbyType == LobbyType::LobbyLocal) {
			// chat messages disabled in local lobbies
			return;
		}
        if (!msg || !msg[0]) {
			// check input
            return;
        }

        constexpr Uint32 seconds_in_day = 86400;
		const Uint32 seconds = getTime() % seconds_in_day;

        if (add_to_list) {
            playSound(238, 64);
            new_lobby_chat_message_alert = ticks;
            lobby_chat_messages.emplace_back(LobbyChatMessage{seconds, color, msg});
            if (lobby_chat_messages.size() > lobby_chat_max_messages) {
                lobby_chat_messages.pop_front();
            }
        }

        if (!main_menu_frame) {
            return;
        }
        auto lobby = main_menu_frame->findFrame("lobby");
        if (!lobby) {
            return;
        }

        auto frame = lobby->findFrame("chat window");
        if (!frame) {
            //frame = toggleLobbyChatWindow();
            return;
        }

        if (add_to_list) {
            // window is already open, so cancel the visual notification
            new_lobby_chat_message_alert = 0;
        }

        const int w = frame->getSize().w;
        const int h = frame->getSize().h;

        auto subframe = frame->findFrame("subframe"); assert(subframe);
        auto subframe_size = subframe->getActualSize();
        int y = subframe_size.h;

        static ConsoleVariable<bool> timestamp_messages("/chat_timestamp", false);

        char buf[1024];
        const Uint32 hour = seconds / 3600;
        const Uint32 min = (seconds / 60) % 60;
        const Uint32 sec = seconds % 60;
        const int result = *timestamp_messages ?
            snprintf(buf, sizeof(buf), "[%.2u:%.2u:%.2u] %s",
                hour, min, sec, msg):
            snprintf(buf, sizeof(buf), "%s", msg);
        const int size = std::min(std::max(0, (int)sizeof(buf)), result);

        auto field = subframe->addField("field", size + 1);
        auto text = Text::get(buf, lobby_chat_font->c_str(),
            uint32ColorWhite, uint32ColorBlack);
        const int text_h = (int)text->getHeight() * (1) + 2; // (1) = string lines
        const int text_w = (int)text->getWidth();
        field->setSize(SDL_Rect{8, y, text_w, text_h});
        field->setFont(lobby_chat_font->c_str());
        field->setColor(color);
        field->setText(buf);

        //char tooltip_buf[32];
        //(void)snprintf(tooltip_buf, sizeof(tooltip_buf), "[%.2u:%.2u:%.2u]", hour, min, sec);
        //field->setTooltip(tooltip_buf);

        const int new_w = std::max(subframe_size.w, text_w + 8);

        y += text_h;
        if (subframe_size.y >= subframe_size.h - subframe->getSize().h) {
            // advance scroll because we're already at bottom
            const int limit = new_w > w ?
                y - subframe->getSize().h + 16:
                y - subframe->getSize().h;
            subframe->setActualSize(SDL_Rect{subframe_size.x,
                std::max(0, limit), new_w, y});
        } else {
            // retain scroll position because we're looking at past history
            subframe->setActualSize(SDL_Rect{
                subframe_size.x, subframe_size.y, new_w, y});
        }
    }

    static Frame* toggleLobbyChatWindow() {
		if (!main_menu_frame) {
			return nullptr;
		}
        auto lobby = main_menu_frame->findFrame("lobby"); assert(lobby);
        auto frame = lobby->findFrame("chat window");
        if (frame) {
            frame->removeSelf();
            return nullptr;
        }

        new_lobby_chat_message_alert = 0;

        const SDL_Rect size = lobby->getSize();
        const int w = 848;
        const int h = 320;

        static ConsoleVariable<Vector4> chatBgColor("/chat_background_color", Vector4{22.f, 24.f, 29.f, 223.f});

        frame = lobby->addFrame("chat window");
		frame->setOwner(clientnum);
        frame->setSize(SDL_Rect{(size.w - w) - 16, 64, w, h});
        frame->setBorderColor(makeColor(51, 33, 26, 255));
        frame->setColor(makeColor(chatBgColor->x, chatBgColor->y, chatBgColor->z, chatBgColor->w));
        frame->setBorder(0);
        frame->setTickCallback([](Widget& widget){
            const int player = clientnum;
            auto frame = static_cast<Frame*>(&widget);
            auto lobby = static_cast<Frame*>(frame->getParent());

            const int w = frame->getSize().w;
            const int h = frame->getSize().h;

            auto subframe = frame->findFrame("subframe"); assert(subframe);
            auto subframe_size = subframe->getActualSize();

            if (Input::inputs[player].consumeBinaryToggle("LogHome")) {
                Input::inputs[player].consumeBindingsSharedWithBinding("LogHome");
                subframe_size.x = 0;
                subframe_size.y = 0;
                subframe->setActualSize(subframe_size);
            }
            if (Input::inputs[player].consumeBinaryToggle("LogEnd")) {
                Input::inputs[player].consumeBindingsSharedWithBinding("LogEnd");
                const int limit = subframe_size.w > w ?
                    subframe_size.h - subframe->getSize().h + 16:
                    subframe_size.h - subframe->getSize().h;
                subframe_size.x = 0;
                subframe_size.y = std::max(0, limit);
                subframe->setActualSize(subframe_size);
            }
            if (Input::inputs[player].consumeBinaryToggle("LogPageUp")) {
                Input::inputs[player].consumeBindingsSharedWithBinding("LogPageUp");
                subframe_size.y -= subframe->getSize().h;
                subframe_size.y = std::max(0, subframe_size.y);
                subframe->setActualSize(subframe_size);
            }
            if (Input::inputs[player].consumeBinaryToggle("LogPageDown")) {
                Input::inputs[player].consumeBindingsSharedWithBinding("LogPageDown");
                subframe_size.y += subframe->getSize().h;
                const int limit = subframe_size.w > w ?
                    subframe_size.h - subframe->getSize().h + 16:
                    subframe_size.h - subframe->getSize().h;
                subframe_size.y = std::min(std::max(0, limit), subframe_size.y);
                subframe->setActualSize(subframe_size);
            }
		    if (Input::inputs[player].consumeBinaryToggle("LogClose")) {
				frame->removeSelf();
				auto card = lobby->findFrame((std::string("card") + std::to_string(clientnum)).c_str());
				if (!card) {
				    return;
				}
				auto ready = card->findButton("ready");
				if (!ready) {
				    return;
				}
				ready->select();
		    }
            });

        // frame images
        {
            frame->addImage(
                SDL_Rect{0, 0, 16, 32},
                0xffffffff,
                "*#images/ui/MapAndLog/Hover_TL00.png",
                "TL");
            frame->addImage(
                SDL_Rect{16, 0, w - 32, 32},
                0xffffffff,
                "*#images/ui/MapAndLog/Hover_T00.png",
                "T");
            frame->addImage(
                SDL_Rect{w - 16, 0, 16, 32},
                0xffffffff,
                "*#images/ui/MapAndLog/Hover_TR00.png",
                "TR");
            auto L = frame->addImage(
                SDL_Rect{0, 32, 4, h - 64},
                0xffffffff,
                "*#images/ui/MapAndLog/Hover_L00.png",
                "L");
            L->ontop = true;
            auto R = frame->addImage(
                SDL_Rect{w - 4, 32, 4, h - 64},
                0xffffffff,
                "*#images/ui/MapAndLog/Hover_R00.png",
                "R");
            R->ontop = true;
            frame->addImage(
                SDL_Rect{0, h - 32, 16, 32},
                0xffffffff,
                "*#images/ui/MapAndLog/Hover_BL01.png",
                "BL");
            frame->addImage(
                SDL_Rect{16, h - 32, w - 32, 32},
                0xffffffff,
                "*#images/ui/MapAndLog/Hover_B01.png",
                "B");
            frame->addImage(
                SDL_Rect{w - 16, h - 32, 16, 32},
                0xffffffff,
                "*#images/ui/MapAndLog/Hover_BR01.png",
                "BR");
        }

        auto subframe = frame->addFrame("subframe");
        subframe->setScrollWithLeftControls(false);
        subframe->setSize(SDL_Rect{0, 32, w, h - 64});
        subframe->setActualSize(SDL_Rect{0, 0, w, 4});
        subframe->setBorderColor(makeColor(22, 24, 29, 255));
        subframe->setSliderColor(makeColor(44, 48, 58, 255));
        subframe->setColor(makeColor(0, 0, 0, 0));
        subframe->setScrollBarsEnabled(true);
        //subframe->setBorder(2);
        subframe->setBorder(0);

        for (auto& msg : lobby_chat_messages) {
            addLobbyChatMessage(msg.color, msg.msg.c_str(), false);
        }

        auto label = frame->addField("label", 64);
        label->setSize(SDL_Rect{16, 0, w - 40, 32});
        label->setHJustify(Field::justify_t::LEFT);
        label->setVJustify(Field::justify_t::CENTER);
        label->setFont(bigfont_outline);
#ifdef NINTENDO
        label->setText("Messages");
#else
        label->setText("Chat");
#endif

#ifndef NINTENDO
        auto chat_buffer = frame->addField("buffer", 1024);
        chat_buffer->setSize(SDL_Rect{4, h - 32, w - 8, 32});
        chat_buffer->setHJustify(Field::justify_t::LEFT);
        chat_buffer->setVJustify(Field::justify_t::CENTER);
		chat_buffer->setSelectorOffset(SDL_Rect{-7, -7, 7, 7});
		chat_buffer->setButtonsOffset(SDL_Rect{11, 0, 0, 0});
        chat_buffer->setFont(lobby_chat_font->c_str());
        chat_buffer->setColor(makeColor(201, 162, 100, 255));
        chat_buffer->setEditable(true);
        chat_buffer->setCallback([](Field& field){
            auto text = field.getText();
            if (text && *text) {
                int len;
                char buf[1024];
	            if (directConnect) {
	                char shortname[32];
	                stringCopy(shortname, stats[clientnum]->name, sizeof(shortname), sizeof(Stat::name));
	                len = snprintf(buf, sizeof(buf), "%s: %s", shortname, text);
	            } else {
	                len = snprintf(buf, sizeof(buf), "%s: %s", players[clientnum]->getAccountName(), text);
	            }
	            if (len > 0) {
					Uint32 color = playerColor(clientnum, colorblind_lobby, false);
                    sendChatMessageOverNet(color, buf, len);
                }
                field.setText("");
#ifndef NINTENDO
                field.activate();
#endif
            }
            });
        chat_buffer->setTickCallback([](Widget& widget){
            auto field = static_cast<Field*>(&widget);
            if (!field->isActivated()) {
                field->setText("");
            }
            });
        chat_buffer->setWidgetSearchParent(frame->getName());
        chat_buffer->setWidgetBack("close");

        auto chat_tooltip = frame->addField("tooltip", 128);
        chat_tooltip->setSize(SDL_Rect{4, h - 32, w - 8, 32});
        chat_tooltip->setHJustify(Field::justify_t::LEFT);
        chat_tooltip->setVJustify(Field::justify_t::CENTER);
        chat_tooltip->setFont(lobby_chat_font->c_str());
        chat_tooltip->setColor(makeColor(201, 162, 100, 255));
        chat_tooltip->setText("Enter message here...");
        chat_tooltip->setTickCallback([](Widget& widget){
            auto frame = static_cast<Frame*>(widget.getParent());
            auto chat_buffer = frame->findField("buffer"); assert(chat_buffer);
            const bool hidden =
                chat_buffer->getText()[0] != '\0' ||
                chat_buffer->isActivated();
            widget.setDisabled(hidden);
            });
#endif

#ifndef NINTENDO
        auto close_button = frame->addButton("close");
        close_button->setSize(SDL_Rect{frame->getSize().w - 30, 4, 26, 26});
	    close_button->setColor(makeColor(255, 255, 255, 255));
	    close_button->setHighlightColor(makeColor(255, 255, 255, 255));
	    close_button->setText("X");
	    close_button->setFont(smallfont_outline);
	    close_button->setHideGlyphs(true);
	    close_button->setHideKeyboardGlyphs(true);
	    close_button->setHideSelectors(true);
	    close_button->setMenuConfirmControlType(0);
	    close_button->setBackground("*#images/ui/Shop/Button_X_00.png");
	    close_button->setBackgroundHighlighted("*#images/ui/Shop/Button_XHigh_00.png");
	    close_button->setBackgroundActivated("*#images/ui/Shop/Button_XPress_00.png");
	    close_button->setTextHighlightColor(makeColor(201, 162, 100, 255));
        close_button->setCallback([](Button& button){
            auto frame = static_cast<Frame*>(button.getParent());
            auto lobby = static_cast<Frame*>(frame->getParent());
			frame->removeSelf();
			auto card = lobby->findFrame((std::string("card") + std::to_string(clientnum)).c_str());
			if (!card) {
			    return;
			}
			auto ready = card->findButton("ready");
			if (!ready) {
			    return;
			}
			ready->select();
            });
        close_button->setWidgetSearchParent(frame->getName());
        close_button->setWidgetDown("buffer");
	    close_button->setWidgetBack("close");
#endif

        return frame;
    }

	static void disconnectFromLobby(bool informRemotes = true) {
		if (informRemotes) {
			if (multiplayer == SERVER) {
				// send disconnect message to clients
				for (int c = 1; c < MAXPLAYERS; c++) {
					if (client_disconnected[c]) {
						continue;
					}
					strcpy((char*)net_packet->data, "DISC");
					net_packet->data[4] = clientnum;
					net_packet->address.host = net_clients[c - 1].host;
					net_packet->address.port = net_clients[c - 1].port;
					net_packet->len = 5;
					sendPacketSafe(net_sock, -1, net_packet, c - 1);
				}
			} else if (multiplayer == CLIENT) {
				// send disconnect message to server
				strcpy((char*)net_packet->data, "DISC");
				net_packet->data[4] = clientnum;
				net_packet->address.host = net_server.host;
				net_packet->address.port = net_server.port;
				net_packet->len = 5;
				sendPacketSafe(net_sock, -1, net_packet, 0);
			}

			// this short delay makes sure that the disconnect message gets out
			Uint32 timetoshutdown = SDL_GetTicks();
			while (SDL_GetTicks() - timetoshutdown < 200)
			{
				pollNetworkForShutdown();
			}
		}

        resetLobbyJoinFlowState();

	    // reset multiplayer status
	    clientnum = 0;
	    multiplayer = SINGLE;
	    client_disconnected[0] = false;
	    for ( int c = 1; c < MAXPLAYERS; c++ ) {
		    client_disconnected[c] = true;
	    }
		currentLobbyType = LobbyType::None;

		gameModeManager.currentSession.restoreSavedServerFlags();

	    closeNetworkInterfaces();

		// hide all mouses
		for (int c = 0; c < MAXPLAYERS; ++c) {
			auto vmouse = inputs.getVirtualMouse(c);
			vmouse->lastMovementFromController = true;
			vmouse->draw_cursor = false;
		}

#ifdef NINTENDO
		nxEnableAutoSleep();
		nxEndParentalControls();
		nxShutdownWireless();
		logoutOfEpic();
#endif

#ifdef STEAMWORKS
	    if (currentLobby) {
		    SteamMatchmaking()->LeaveLobby(*static_cast<CSteamID*>(currentLobby));
		    cpp_Free_CSteamID(currentLobby);
		    currentLobby = NULL;
	    }
#endif

#ifdef USE_EOS
	    if (EOS.CurrentLobbyData.currentLobbyIsValid()) {
		    EOS.leaveLobby();
	    }
#endif
	}

	static void doKeepAlive() {
		if (multiplayer == SERVER) {
			for (int i = 1; i < MAXPLAYERS; i++) {
				if (client_disconnected[i]) {
					continue;
				}
				bool clientHasLostP2P = false;
				if (!directConnect && LobbyHandler.getHostingType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM) {
#ifdef STEAMWORKS
					if (!steamIDRemote[i - 1]) {
						clientHasLostP2P = true;
					}
#endif
				} else if (!directConnect && LobbyHandler.getHostingType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY) {
#ifdef USE_EOS
					if (!EOS.P2PConnectionInfo.isPeerStillValid(i - 1)) {
						clientHasLostP2P = true;
					}
#endif
				}
				if (*cvar_enableKeepAlives) {
					if (clientHasLostP2P || (ticks - client_keepalive[i] > TICKS_PER_SECOND * TIMEOUT_TIME)) {
						client_disconnected[i] = true;
						strncpy((char*)(net_packet->data), "DISC", 4);
						net_packet->data[4] = i;
						net_packet->len = 5;
						for (int c = 1; c < MAXPLAYERS; c++) {
							if (client_disconnected[c]) {
								continue;
							}
							net_packet->address.host = net_clients[c - 1].host;
							net_packet->address.port = net_clients[c - 1].port;
							sendPacketSafe(net_sock, -1, net_packet, c - 1);
						}

						char buf[1024];
						snprintf(buf, sizeof(buf), "*** %s has timed out ***", players[i]->getAccountName());
						addLobbyChatMessage(uint32ColorYellow, buf);

						if (directConnect) {
							createWaitingStone(i);
						} else {
							createInviteButton(i);
						}
						continue;
					}
				}
			}
		} else if (multiplayer == CLIENT) {
			bool hostHasLostP2P = false;
			if (!directConnect) {
			    if (LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM) {
#ifdef STEAMWORKS
                    if (!connectingToLobby && !connectingToLobbyWindow) {
				        if (!steamIDRemote[0]) {
					        hostHasLostP2P = true;
				        }
				    }
#endif
			    } else if (LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY) {
#ifdef USE_EOS
                    if (!EOS.bConnectingToLobby && !EOS.bConnectingToLobbyWindow) {
				        if (!EOS.P2PConnectionInfo.isPeerStillValid(0)) {
					        hostHasLostP2P = true;
				        }
				    }
#endif
			    }
			}

			if (hostHasLostP2P) {
			    int error_code = -1;
			    if (LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY) {
#ifdef USE_EOS
                    error_code = EOS.ConnectingToLobbyStatus;
#endif // USE_EOS
			    }
			    else if (LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM) {
#ifdef STEAMWORKS
			        error_code = connectingToLobbyStatus;
#endif //STEAMWORKS
			    }
			    auto error_str = LobbyHandler_t::getLobbyJoinFailedConnectString(error_code);
                disconnectFromLobby();
	            destroyMainMenu();
	            createMainMenu(false);
                connectionErrorPrompt(error_str.c_str());
			}

			if (*cvar_enableKeepAlives) {
				if (ticks - client_keepalive[0] > TICKS_PER_SECOND * TIMEOUT_TIME) {
					// timeout after X seconds of no messages from server
					disconnectFromLobby();
					destroyMainMenu();
					createMainMenu(false);
					timedOut();
				}
			}
		}

		// send keepalive messages every second
		if (*cvar_enableKeepAlives) {
			if (ticks % (TICKS_PER_SECOND * 1) == 0 && multiplayer != SINGLE) {
				strcpy((char*)net_packet->data, "KPAL");
				net_packet->data[4] = clientnum;
				net_packet->len = 5;
				if (multiplayer == CLIENT) {
					net_packet->address.host = net_server.host;
					net_packet->address.port = net_server.port;
					sendPacketSafe(net_sock, -1, net_packet, 0);
				} else if (multiplayer == SERVER) {
					for (int i = 1; i < MAXPLAYERS; i++) {
						if (client_disconnected[i]) {
							continue;
						}
						net_packet->address.host = net_clients[i - 1].host;
						net_packet->address.port = net_clients[i - 1].port;
						sendPacketSafe(net_sock, -1, net_packet, i - 1);
					}
				}
			}
		}
	}

	static void saveLastCharacter(const int index, int multiplayer)
	{
		if ( !loadingsavegame )
		{
			if ( multiplayer != SINGLE )
			{
				if ( index == clientnum )
				{
					if ( directConnect )
					{
						LastCreatedCharacterSettings.characterAppearance[LastCreatedCharacter::LASTCHAR_LAN_PERSONA_INDEX] = stats[clientnum]->appearance;
						LastCreatedCharacterSettings.characterSex[LastCreatedCharacter::LASTCHAR_LAN_PERSONA_INDEX] = stats[clientnum]->sex;
						LastCreatedCharacterSettings.characterRace[LastCreatedCharacter::LASTCHAR_LAN_PERSONA_INDEX] = stats[clientnum]->playerRace;
						LastCreatedCharacterSettings.characterClass[LastCreatedCharacter::LASTCHAR_LAN_PERSONA_INDEX] = client_classes[clientnum];
						LastCreatedCharacterSettings.characterName[LastCreatedCharacter::LASTCHAR_LAN_PERSONA_INDEX] = stats[clientnum]->name;
					}
					else
					{
						LastCreatedCharacterSettings.characterAppearance[LastCreatedCharacter::LASTCHAR_ONLINE_PERSONA_INDEX] = stats[clientnum]->appearance;
						LastCreatedCharacterSettings.characterSex[LastCreatedCharacter::LASTCHAR_ONLINE_PERSONA_INDEX] = stats[clientnum]->sex;
						LastCreatedCharacterSettings.characterRace[LastCreatedCharacter::LASTCHAR_ONLINE_PERSONA_INDEX] = stats[clientnum]->playerRace;
						LastCreatedCharacterSettings.characterClass[LastCreatedCharacter::LASTCHAR_ONLINE_PERSONA_INDEX] = client_classes[clientnum];
						LastCreatedCharacterSettings.characterName[LastCreatedCharacter::LASTCHAR_ONLINE_PERSONA_INDEX] = stats[clientnum]->name;
					}
				}
			}
			else
			{
				LastCreatedCharacterSettings.characterAppearance[index] = stats[index]->appearance;
				LastCreatedCharacterSettings.characterSex[index] = stats[index]->sex;
				LastCreatedCharacterSettings.characterRace[index] = stats[index]->playerRace;
				LastCreatedCharacterSettings.characterClass[index] = client_classes[index];
				LastCreatedCharacterSettings.characterName[index] = stats[index]->name;
			}
		}
	}

	static void sendPlayerOverNet() {
	    if (multiplayer != SERVER && multiplayer != CLIENT) {
	        return;
	    }
	    for (Uint8 player = 0; player < MAXPLAYERS; ++player) {
	        if (player != clientnum) {
	            continue;
	        }
	        // packet header
	        memcpy(net_packet->data, "PLYR", 4);
		    net_packet->data[4] = player;

		    // encode name
		    stringCopy((char*)net_packet->data + 5, stats[player]->name, 32, sizeof(Stat::name));

		    // encode class, sex, race, and appearance
            SDLNet_Write32((Uint32)client_classes[player], &net_packet->data[37]);
            SDLNet_Write32((Uint32)stats[player]->sex, &net_packet->data[41]);
            Uint32 raceAndAppearance =
                ((stats[player]->appearance & 0xff) << 8) |
                (stats[player]->playerRace & 0xff);
            SDLNet_Write32(raceAndAppearance, &net_packet->data[45]);

            // send packet
            net_packet->len = 49;
	        if (multiplayer == SERVER) {
		        for (int i = 1; i < MAXPLAYERS; i++ ) {
			        if ( client_disconnected[i] ) {
				        continue;
			        }
			        net_packet->address.host = net_clients[i - 1].host;
			        net_packet->address.port = net_clients[i - 1].port;
			        sendPacketSafe(net_sock, -1, net_packet, i - 1);
		        }
		    } else if (multiplayer == CLIENT) {
		        net_packet->address.host = net_server.host;
		        net_packet->address.port = net_server.port;
		        sendPacketSafe(net_sock, -1, net_packet, 0);
		    }
		}
	}

	static void kickPlayer(int index) {
	    if (multiplayer == SERVER) {
            strcpy((char*)net_packet->data, "KICK");
            net_packet->address.host = net_clients[index - 1].host;
            net_packet->address.port = net_clients[index - 1].port;
            net_packet->len = 4;
            sendPacketSafe(net_sock, -1, net_packet, index - 1);

			char buf[1024];
			snprintf(buf, sizeof(buf), "*** %s has been kicked ***", players[index]->getAccountName());
			addLobbyChatMessage(uint32ColorRed, buf);

			client_disconnected[index] = true;

#ifdef STEAMWORKS
            if (steamIDRemote[index - 1]) {
                cpp_Free_CSteamID(steamIDRemote[index - 1]);
			    steamIDRemote[index - 1] = nullptr;
			}
#endif

		    // inform other players
	        for (int c = 1; c < MAXPLAYERS; ++c) {
	            if (client_disconnected[c]) {
	                continue;
	            }
	            strcpy((char*)net_packet->data, "DISC");
	            net_packet->data[4] = index;
	            net_packet->address.host = net_clients[c - 1].host;
	            net_packet->address.port = net_clients[c - 1].port;
	            net_packet->len = 5;
	            sendPacketSafe(net_sock, -1, net_packet, c - 1);
	        }

		    if (directConnect) {
                createWaitingStone(index);
            } else {
                createInviteButton(index);
            }
	        checkReadyStates();
	    }
	}

	static void lockSlot(int index, bool locked) {
	    if (multiplayer == SERVER) {
	        playerSlotsLocked[index] = locked;
	        if (locked) {
	            if (!client_disconnected[index]) {
	                kickPlayer(index);
	            }
	            createLockedStone(index);
	        } else {
	            if (client_disconnected[index]) {
		            if (directConnect) {
                        createWaitingStone(index);
                    } else {
                        createInviteButton(index);
                    }
                }
	        }
            checkReadyStates();
	    }
	}

	static void sendReadyOverNet(int index, bool ready) {
	    if (multiplayer != SERVER && multiplayer != CLIENT) {
	        return;
	    }

        // packet header
        memcpy(net_packet->data, "REDY", 4);
	    net_packet->data[4] = (Uint8)index;

	    // data
	    net_packet->data[5] = ready ? (Uint8)1u : (Uint8)0u;

        // send packet
        net_packet->len = 6;
        if (multiplayer == SERVER) {
	        for (int i = 1; i < MAXPLAYERS; i++ ) {
		        if ( client_disconnected[i] ) {
			        continue;
		        }
		        net_packet->address.host = net_clients[i - 1].host;
		        net_packet->address.port = net_clients[i - 1].port;
		        sendPacketSafe(net_sock, -1, net_packet, i - 1);
	        }
	    } else if (multiplayer == CLIENT) {
	        net_packet->address.host = net_server.host;
	        net_packet->address.port = net_server.port;
	        sendPacketSafe(net_sock, -1, net_packet, 0);
	    }
	}

	static void sendSvFlagsOverNet() {
	    if (multiplayer == SERVER) {
	        memcpy(net_packet->data, "SVFL", 4);
			SDLNet_Write32(svFlags, &net_packet->data[4]);
			net_packet->len = 8;
			for (int c = 1; c < MAXPLAYERS; c++) {
				if (client_disconnected[c]) {
					continue;
				}
				net_packet->address.host = net_clients[c - 1].host;
				net_packet->address.port = net_clients[c - 1].port;
				sendPacketSafe(net_sock, -1, net_packet, c - 1);
			}
#ifdef STEAMWORKS
			if (LobbyHandler.getHostingType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM) {
				if (!directConnect && currentLobby) {
					char svFlagsChar[16];
					snprintf(svFlagsChar, 15, "%d", svFlags);
					SteamMatchmaking()->SetLobbyData(*static_cast<CSteamID*>(currentLobby), "svFlags", svFlagsChar);
				}
			}
#endif
	    } else if (multiplayer == CLIENT) {
	        memcpy(net_packet->data, "SVFL", 4);
			net_packet->len = 4;
			net_packet->address.host = net_server.host;
			net_packet->address.port = net_server.port;
			sendPacketSafe(net_sock, -1, net_packet, 0);
	    }
	}

	static void sendChatMessageOverNet(Uint32 color, const char* msg, size_t len) {
	    if (multiplayer != SERVER && multiplayer != CLIENT) {
	        return;
	    }

		memcpy((char*)net_packet->data, "CMSG", 4);
		SDLNet_Write32(color, &net_packet->data[4]);
		stringCopy((char*)net_packet->data + 8, msg, 256, len);
		net_packet->len = 8 + (int)len + 1;
		net_packet->data[net_packet->len - 1] = 0;

        // send packet
        if (multiplayer == SERVER) {
	        for (int i = 1; i < MAXPLAYERS; i++ ) {
		        if ( client_disconnected[i] ) {
			        continue;
		        }
		        net_packet->address.host = net_clients[i - 1].host;
		        net_packet->address.port = net_clients[i - 1].port;
		        sendPacketSafe(net_sock, -1, net_packet, i - 1);
	        }
	        addLobbyChatMessage(color, msg);
	    } else if (multiplayer == CLIENT) {
	        net_packet->address.host = net_server.host;
	        net_packet->address.port = net_server.port;
	        sendPacketSafe(net_sock, -1, net_packet, 0);
	    }
	}

	static void updateLobby() {
	    if ( !directConnect && LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM )
		{
#ifdef STEAMWORKS
			// update server name
			if ( currentLobby )
			{
				const char* lobbyName = SteamMatchmaking()->GetLobbyData( *static_cast<CSteamID*>(currentLobby), "name");
				if ( lobbyName )
				{
					if ( strcmp(lobbyName, currentLobbyName) )
					{
						if ( multiplayer == CLIENT )
						{
							// update the lobby name on our end
							snprintf( currentLobbyName, 31, "%s", lobbyName );
						}
						else if ( multiplayer == SERVER )
						{
							// update the backend's copy of the lobby name
							SteamMatchmaking()->SetLobbyData(*static_cast<CSteamID*>(currentLobby), "name", currentLobbyName);
						}
					}
				}
				if ( multiplayer == SERVER )
				{
					const char* lobbyTimeStr = SteamMatchmaking()->GetLobbyData(*static_cast<CSteamID*>(currentLobby), "lobbyModifiedTime");
					if ( lobbyTimeStr )
					{
						Uint32 lobbyTime = static_cast<Uint32>(atoi(lobbyTimeStr));
						if ( SteamUtils()->GetServerRealTime() >= lobbyTime + 3 )
						{
							//printlog("Updated server time");
							char modifiedTime[32];
							snprintf(modifiedTime, 31, "%d", SteamUtils()->GetServerRealTime());
							SteamMatchmaking()->SetLobbyData(*static_cast<CSteamID*>(currentLobby), "lobbyModifiedTime", modifiedTime);
						}
					}
				}
			}
#endif
		}
		else if ( !directConnect && LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY )
		{
#if defined USE_EOS
			// update server name
			if ( multiplayer == CLIENT )
			{
				// update the lobby name on our end
				snprintf(EOS.currentLobbyName, 31, "%s", EOS.CurrentLobbyData.LobbyAttributes.lobbyName.c_str());
			}
			else if ( multiplayer == SERVER )
			{
				// update the backend's copy of the lobby name and other properties
				if ( ticks % TICKS_PER_SECOND == 0 && EOS.CurrentLobbyData.currentLobbyIsValid() )
				{
					if ( EOS.CurrentLobbyData.LobbyAttributes.lobbyName.compare(EOS.currentLobbyName) != 0
						&& strcmp(EOS.currentLobbyName, "") != 0 )
					{
						EOS.CurrentLobbyData.updateLobbyForHost(EOSFuncs::LobbyData_t::HostUpdateLobbyTypes::LOBBY_UPDATE_MAIN_MENU);
					}
					else if ( EOS.CurrentLobbyData.LobbyAttributes.serverFlags != svFlags )
					{
						EOS.CurrentLobbyData.updateLobbyForHost(EOSFuncs::LobbyData_t::HostUpdateLobbyTypes::LOBBY_UPDATE_MAIN_MENU);
					}
					else if ( EOS.CurrentLobbyData.LobbyAttributes.PermissionLevel != static_cast<Uint32>(EOS.currentPermissionLevel) )
					{
						EOS.CurrentLobbyData.updateLobbyForHost(EOSFuncs::LobbyData_t::HostUpdateLobbyTypes::LOBBY_UPDATE_MAIN_MENU);
					}
					else if ( EOS.CurrentLobbyData.LobbyAttributes.friendsOnly != EOS.bFriendsOnly )
					{
						EOS.CurrentLobbyData.updateLobbyForHost(EOSFuncs::LobbyData_t::HostUpdateLobbyTypes::LOBBY_UPDATE_MAIN_MENU);
					}
				}
			}
#endif
		}
		else if ( directConnect )
		{
#ifdef NINTENDO
			if ( ticks % TICKS_PER_SECOND == 0 ) {
				int numplayers = 0;
				for (int c = 0; c < MAXPLAYERS; ++c) {
					if (!client_disconnected[c]) {
						++numplayers;
					}
				}

				char address[64];
				nxGetWirelessAddress(address, sizeof(address));
				bool result = nxUpdateLobby(address, MainMenu::getHostname(), svFlags, numplayers);
				if (!result) {
					disconnectFromLobby();
					destroyMainMenu();
					createMainMenu(false);
				}
			}
#endif
		}
	}

	static std::unordered_map<Uint32, void(*)()> serverPacketHandlers = {
		// network scan
		{'SCAN', [](){
		    handleScanPacket();
		}},

		// update player attributes
		{'PLYR', [](){
		    // forward to other players
			for (int i = 1; i < MAXPLAYERS; i++ ) {
				if ( client_disconnected[i] ) {
					continue;
				}
				net_packet->address.host = net_clients[i - 1].host;
				net_packet->address.port = net_clients[i - 1].port;
				sendPacketSafe(net_sock, -1, net_packet, i - 1);
			}

			const Uint8 player = std::min(net_packet->data[4], (Uint8)(MAXPLAYERS - 1));
			if (!loadingsavegame) {
				stats[player]->clearStats();
			}

	        stringCopy(stats[player]->name, (char*)(&net_packet->data[5]), sizeof(Stat::name), 32);
	        client_classes[player] = (int)SDLNet_Read32(&net_packet->data[37]);
	        stats[player]->sex = static_cast<sex_t>((int)SDLNet_Read32(&net_packet->data[41]));
	        Uint32 raceAndAppearance = SDLNet_Read32(&net_packet->data[45]);
	        stats[player]->appearance = (raceAndAppearance & 0xFF00) >> 8;
	        stats[player]->playerRace = (raceAndAppearance & 0xFF);

			if (!loadingsavegame) {
				initClass(player);
			}
		}},

		// update ready status
		{'REDY', [](){
		    // forward to other players
			for (int i = 1; i < MAXPLAYERS; i++ ) {
				if ( client_disconnected[i] ) {
					continue;
				}
				net_packet->address.host = net_clients[i - 1].host;
				net_packet->address.port = net_clients[i - 1].port;
				sendPacketSafe(net_sock, -1, net_packet, i - 1);
			}
			const Uint8 player = std::min(net_packet->data[4], (Uint8)(MAXPLAYERS - 1));
		    Uint8 status = net_packet->data[5];
		    createReadyStone((int)player, false, status ? true : false);
		}},

		// got a chat message from client
		{'CMSG', [](){
		    // forward to other players
			for (int i = 1; i < MAXPLAYERS; i++ ) {
				if ( client_disconnected[i] ) {
					continue;
				}
				net_packet->address.host = net_clients[i - 1].host;
				net_packet->address.port = net_clients[i - 1].port;
				sendPacketSafe(net_sock, -1, net_packet, i - 1);
			}
			const Uint32 color = SDLNet_Read32(&net_packet->data[4]);
			addLobbyChatMessage(color, (char*)(&net_packet->data[8]));
		}},

		// received client ping
		{'PING', [](){
			const int j = net_packet->data[4];
			if (j <= 0 || j >= MAXPLAYERS ) {
				return;
			}
			if (client_disconnected[j] || players[j]->isLocalPlayer()) {
				return;
			}
			memcpy((char*)net_packet->data, "PING", 4);
			net_packet->address.host = net_clients[j - 1].host;
			net_packet->address.port = net_clients[j - 1].port;
			net_packet->len = 5;
			sendPacketSafe(net_sock, -1, net_packet, j - 1);
		}},

		// player disconnected
		{'DISC', [](){
			const Uint8 player = std::min(net_packet->data[4], (Uint8)(MAXPLAYERS - 1));
            if (player == 0) {
                // yeah right
                return;
            }
			client_disconnected[player] = true;

#ifdef STEAMWORKS
            if (steamIDRemote[player - 1]) {
                cpp_Free_CSteamID(steamIDRemote[player - 1]);
			    steamIDRemote[player - 1] = nullptr;
			}
#endif

		    // forward to other players
			for (int c = 1; c < MAXPLAYERS; c++) {
				if (client_disconnected[c]) {
					continue;
				}
				net_packet->address.host = net_clients[c - 1].host;
				net_packet->address.port = net_clients[c - 1].port;
				net_packet->len = 5;
				sendPacketSafe(net_sock, -1, net_packet, c - 1);
			}

			char buf[1024];
			snprintf(buf, sizeof(buf), "*** %s has left the game ***", players[player]->getAccountName());
			addLobbyChatMessage(uint32ColorYellow, buf);

		    if (directConnect) {
                createWaitingStone(player);
            } else {
                createInviteButton(player);
            }
		}},

		// client requesting new svFlags
		{'SVFL', [](){
			// update svFlags for everyone
			SDLNet_Write32(svFlags, &net_packet->data[4]);
			net_packet->len = 8;
			for (int c = 1; c < MAXPLAYERS; c++) {
				if (client_disconnected[c]) {
					continue;
				}
				net_packet->address.host = net_clients[c - 1].host;
				net_packet->address.port = net_clients[c - 1].port;
				sendPacketSafe(net_sock, -1, net_packet, c - 1);
			}
		}},

		// keepalive
		{'KPAL', [](){
			const Uint8 player = std::min(net_packet->data[4], (Uint8)(MAXPLAYERS - 1));
			client_keepalive[player] = ticks;
		}},

		// the client sent a gameplayer preferences update
		{'GPPR', []() {
			GameplayPreferences_t::receivePacket();
		}},
    };

	static void handlePacketsAsServer() {
#ifdef STEAMWORKS
		CSteamID newSteamID;
#endif
#if defined USE_EOS
		EOS_ProductUserId newRemoteProductId = nullptr;
#endif

        updateLobby();

		for (int numpacket = 0; numpacket < PACKET_LIMIT; numpacket++) {
			if (directConnect) {
				if (!SDLNet_UDP_Recv(net_sock, net_packet)) {
					break;
				}
			} else {
				if (LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM) {
#ifdef STEAMWORKS
					uint32_t packetlen = 0;
					if (!SteamNetworking()->IsP2PPacketAvailable(&packetlen, 0)) {
						break;
					}
					packetlen = std::min<int>(packetlen, NET_PACKET_SIZE - 1);
					Uint32 bytesRead = 0;
					if (!SteamNetworking()->ReadP2PPacket(net_packet->data, packetlen, &bytesRead, &newSteamID, 0)) {
						continue;
					}
					net_packet->len = packetlen;
					if (packetlen < sizeof(uint32_t)) {
						continue; // junk packet, skip
					}

					CSteamID mySteamID = SteamUser()->GetSteamID();
					if (mySteamID.ConvertToUint64() == newSteamID.ConvertToUint64()) {
						continue;
					}
#endif // STEAMWORKS
				} else if (LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY) {
#ifdef USE_EOS
					if (!EOS.HandleReceivedMessages(&newRemoteProductId)) {
						continue;
					}
					EOS.P2PConnectionInfo.insertProductIdIntoPeers(newRemoteProductId);
#endif // USE_EOS
				}
			}

            // unwrap a packet if it's marked "safe"
			if (handleSafePacket()) {
				continue;
			}

			Uint32 packetId = SDLNet_Read32(&net_packet->data[0]);

			if (packetId == 'JOIN') {
			    int playerNum = MAXPLAYERS;

			    // when processing connection requests using Steamworks or EOS,
			    // we can reject join requests out of hand if the player with
			    // the associated ID is already connected to this server.
			    if (!directConnect) {
				    if (LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM) {
#ifdef STEAMWORKS
					    bool skipJoin = false;
					    for (int c = 1; c < MAXPLAYERS; c++) {
						    if (client_disconnected[c] || !steamIDRemote[c - 1]) {
							    continue;
						    }
						    if (newSteamID.ConvertToUint64() == (static_cast<CSteamID*>(steamIDRemote[c - 1]))->ConvertToUint64()) {
							    // we've already accepted this player. NEXT!
							    skipJoin = true;
							    break;
						    }
					    }
					    if (skipJoin) {
						    return;
					    }
#endif // STEAMWORKS
				    } else if (LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY) {
#ifdef USE_EOS
					    bool skipJoin = false;
					    EOSFuncs::logInfo("newRemoteProductId: %s", EOSFuncs::Helpers_t::productIdToString(newRemoteProductId));
					    if (newRemoteProductId && EOS.P2PConnectionInfo.isPeerIndexed(newRemoteProductId)) {
						    if (EOS.P2PConnectionInfo.getIndexFromPeerId(newRemoteProductId) >= 0) {
							    // we've already accepted this player. NEXT!
							    skipJoin = true;
						    }
					    }
					    if (skipJoin) {
						    return;
					    }
#endif // USE_EOS
				    }
			    }

			    // process incoming join request
			    NetworkingLobbyJoinRequestResult result = lobbyPlayerJoinRequest(playerNum, playerSlotsLocked);

			    // finalize connections for Steamworks / EOS
			    if (result == NetworkingLobbyJoinRequestResult::NET_LOBBY_JOIN_P2P_FAILURE) {
				    if (LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM) {
#ifdef STEAMWORKS
					    for (int responses = 0; responses < 5; ++responses) {
						    SteamNetworking()->SendP2PPacket(newSteamID, net_packet->data, net_packet->len, k_EP2PSendReliable, 0);
						    SDL_Delay(5);
					    }
#endif // STEAMWORKS
				    } else if ( LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY ) {
#ifdef USE_EOS
					    for ( int responses = 0; responses < 5; ++responses ) {
						    EOS.SendMessageP2P(newRemoteProductId, net_packet->data, net_packet->len);
						    SDL_Delay(5);
					    }
#endif // USE_EOS
				    }
				}
				else if ( result == NetworkingLobbyJoinRequestResult::NET_LOBBY_JOIN_P2P_SUCCESS ) {
					if ( LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM ) {
#ifdef STEAMWORKS
						if ( steamIDRemote[playerNum - 1] ) {
							cpp_Free_CSteamID(steamIDRemote[playerNum - 1]);
						}
						steamIDRemote[playerNum - 1] = new CSteamID();
						*static_cast<CSteamID*>(steamIDRemote[playerNum - 1]) = newSteamID;
						for ( int responses = 0; responses < 5; ++responses ) {
							SteamNetworking()->SendP2PPacket(*static_cast<CSteamID*>(steamIDRemote[playerNum - 1]), net_packet->data, net_packet->len, k_EP2PSendReliable, 0);
							SDL_Delay(5);
						}
#endif // STEAMWORKS
					}
					else if ( LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY ) {
#if defined USE_EOS
						EOS.P2PConnectionInfo.assignPeerIndex(newRemoteProductId, playerNum - 1);
						for ( int responses = 0; responses < 5; ++responses ) {
							EOS.SendMessageP2P(EOS.P2PConnectionInfo.getPeerIdFromIndex(playerNum - 1), net_packet->data, net_packet->len);
							SDL_Delay(5);
						}
#endif
					}
					sendSvFlagsOverNet();
				}

			    // assume success after this point
			    if (result != NET_LOBBY_JOIN_DIRECTIP_SUCCESS &&
			        result != NET_LOBBY_JOIN_P2P_SUCCESS) {
			        printlog("Player failed to join lobby");
			    }

			    // finally, open a player card!
			    if (playerNum >= 1 && playerNum < MAXPLAYERS) {
		            createReadyStone(playerNum, false, false);
			    }
			}

		    auto find = serverPacketHandlers.find(packetId);
		    if (find == serverPacketHandlers.end()) {
                // error
		        printlog("Got a mystery packet: %c%c%c%c",
		            (char)net_packet->data[0],
		            (char)net_packet->data[1],
		            (char)net_packet->data[2],
		            (char)net_packet->data[3]);
		    } else {
		        (*(find->second))(); // handle packet
		    }
		}
	}

	static std::unordered_map<Uint32, void(*)()> clientPacketHandlers = {
	    // game start
	    {'STRT', [](){
            destroyMainMenu();
            createDummyMainMenu();
	        lobbyWindowSvFlags = SDLNet_Read32(&net_packet->data[4]);
	        uniqueGameKey = SDLNet_Read32(&net_packet->data[8]);
	        local_rng.seedBytes(&uniqueGameKey, sizeof(uniqueGameKey));
	        net_rng.seedBytes(&uniqueGameKey, sizeof(uniqueGameKey));
	        beginFade(FadeDestination::GameStart);
	        numplayers = MAXPLAYERS;
	        if (net_packet->data[12] == 0) {
	            // is this necessary? I don't think so
		        //loadingsavegame = 0;
	        }
	    }},

		// we can get an ENTU packet if the server already started and we missed it somehow
	    /*{'ENTU', [](){
            destroyMainMenu();
            createDummyMainMenu();
		    beginFade(FadeDestination::GameStart);

		    // NOTE we may not get a unique game key or server flags this way!!
	    }},*/

	    // new player
	    {'JOIN', [](){
	        const int player = std::min(net_packet->data[4], (Uint8)(MAXPLAYERS - 1));
		    client_disconnected[player] = false;
		    client_classes[player] = net_packet->data[5];
		    stats[player]->sex = static_cast<sex_t>(net_packet->data[6]);
		    stats[player]->appearance = net_packet->data[7];
		    stats[player]->playerRace = net_packet->data[8];
		    stringCopy(stats[player]->name, (char*)(&net_packet->data[9]), sizeof(Stat::name), 32);

		    /*char buf[1024];
		    snprintf(buf, sizeof(buf), "*** %s has joined the game ***", players[player]->getAccountName());
		    addLobbyChatMessage(uint32ColorBaronyBlue, buf);*/

	        if (player != clientnum) {
	            createReadyStone((int)player, false, false);
	        }
	    }},

	    // lock/unlock a player slot
	    {'LOCK', [](){
	        const int player = std::min(net_packet->data[4], (Uint8)(MAXPLAYERS - 1));
	        const bool locked = net_packet->data[5];
	        playerSlotsLocked[player] = locked;

            // these functions automatically create locked
            // stones instead if the player is now locked
	        if (directConnect) {
	            createWaitingStone(player);
	        } else {
	            createInviteButton(player);
	        }
	        checkReadyStates();
	    }},

	    // update player attributes
	    {'PLYR', [](){
	        const int player = std::min(net_packet->data[4], (Uint8)(MAXPLAYERS - 1));
		    if (player != clientnum) {
				if (!loadingsavegame) {
					stats[player]->clearStats();
				}
                stringCopy(stats[player]->name, (char*)(&net_packet->data[5]), sizeof(Stat::name), 32);
                client_classes[player] = (int)SDLNet_Read32(&net_packet->data[37]);
                stats[player]->sex = static_cast<sex_t>((int)SDLNet_Read32(&net_packet->data[41]));
                Uint32 raceAndAppearance = SDLNet_Read32(&net_packet->data[45]);
                stats[player]->appearance = (raceAndAppearance & 0xFF00) >> 8;
                stats[player]->playerRace = (raceAndAppearance & 0xFF);
				if (!loadingsavegame) {
					initClass(player);
				}
            }
            checkReadyStates();
	    }},

	    // update ready status
	    {'REDY', [](){
	        const int player = std::min(net_packet->data[4], (Uint8)(MAXPLAYERS - 1));
	        Uint8 status = net_packet->data[5];
	        if (player != clientnum) {
	            createReadyStone((int)player, false, status ? true : false);
	        }
	    }},

		// received ping back from server
		{'PING', [](){
			char buf[1024];
			snprintf(buf, sizeof(buf), "*** ping time = %4d ms ***", (SDL_GetTicks() - pingtime));
			addLobbyChatMessage(uint32ColorBaronyBlue, buf);
		}},

		// player disconnect
	    {'DISC', [](){
		    const int playerDisconnected = std::min(net_packet->data[4], (Uint8)(MAXPLAYERS - 1));
			client_disconnected[playerDisconnected] = true;
		    if (playerDisconnected == clientnum || playerDisconnected == 0) {
			    // we got dropped
                disconnectFromLobby(false);
	            destroyMainMenu();
	            createMainMenu(false);
                connectionErrorPrompt("The lobby has been closed\nby the host.");
		    } else {
		        char buf[1024];
		        snprintf(buf, sizeof(buf), "*** %s has left the game ***", players[playerDisconnected]->getAccountName());
		        addLobbyChatMessage(uint32ColorYellow, buf);
			    if (directConnect) {
	                createWaitingStone(playerDisconnected);
	            } else {
	                createInviteButton(playerDisconnected);
	            }
		        checkReadyStates();
		    }
	    }},

	    // kicked
	    {'KICK', [](){
            disconnectFromLobby(false);
            destroyMainMenu();
            createMainMenu(false);
            connectionErrorPrompt("You have been kicked\nfrom the lobby.");
	    }},

	    // got a chat message
	    {'CMSG', [](){
	        const Uint32 color = SDLNet_Read32(&net_packet->data[4]);
		    addLobbyChatMessage(color, (char*)(&net_packet->data[8]));
	    }},

	    // update svFlags
	    {'SVFL', [](){
		    lobbyWindowSvFlags = SDLNet_Read32(&net_packet->data[4]);
	    }},

	    // keepalive
	    {'KPAL', [](){
		    return; // just a keep alive
	    }},

		// the server sent a game player preferences update
		{'GPPR', []() {
			GameplayPreferences_t::receivePacket();
		}},

		// the server requested a game player preferences update
		{'GPPU', []() {
			gameplayPreferences[clientnum].sendToServer();
		}},

		// the server sent a game config update
		{'GOPT', []() {
			GameplayPreferences_t::receiveGameConfig();
		}},
	};

	static void handlePacketsAsClient() {
	    if (receivedclientnum == false) {
#ifdef STEAMWORKS
			CSteamID newSteamID;
			if (!directConnect && LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM) {
				if (ticks - client_keepalive[0] >= 30 * TICKS_PER_SECOND) {
				    // 30 second timeout
					auto error_code = static_cast<int>(LobbyHandler_t::LOBBY_JOIN_TIMEOUT);
					auto error_str = LobbyHandler_t::getLobbyJoinFailedConnectString(error_code);
					disconnectFromLobby(false);
					closePrompt("connect_prompt");
					connectionErrorPrompt(error_str.c_str());
					connectingToLobbyStatus = EResult::k_EResultOK;
				}
			}
#endif
#if defined USE_EOS
			EOS_ProductUserId newRemoteProductId = nullptr;
			if (!directConnect && LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY) {
				if (ticks - client_keepalive[0] >= 30 * TICKS_PER_SECOND) {
				    // 30 second timeout
					auto error_code = static_cast<int>(LobbyHandler_t::LOBBY_JOIN_TIMEOUT);
					auto error_str = LobbyHandler_t::getLobbyJoinFailedConnectString(error_code);
					disconnectFromLobby(false);
					closePrompt("connect_prompt");
					connectionErrorPrompt(error_str.c_str());
					EOS.ConnectingToLobbyStatus = static_cast<int>(EOS_EResult::EOS_Success);
				}
			}
#endif

			// trying to connect to the server and get a player number
			// receive the packet:
			bool gotPacket = false;
			if (directConnect) {
			    if (SDLNet_UDP_Recv(net_sock, net_packet)) {
			        if (!handleSafePacket()) {
			            Uint32 packetId = SDLNet_Read32(&net_packet->data[0]);
			            if (packetId == 'HELO') {
					        gotPacket = true;
					    }
					}
			    }
			} else if (LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM) {
#ifdef STEAMWORKS
				for (Uint32 numpacket = 0; numpacket < PACKET_LIMIT && net_packet; numpacket++) {
					Uint32 packetlen = 0;
					if ( !SteamNetworking()->IsP2PPacketAvailable(&packetlen, 0) ) {
						break;
					}
					packetlen = std::min<int>(packetlen, NET_PACKET_SIZE - 1);
					Uint32 bytesRead = 0;
					if (!SteamNetworking()->ReadP2PPacket(
					    net_packet->data, packetlen,
					    &bytesRead, &newSteamID, 0)) {
						continue;
					}
					net_packet->len = packetlen;
					if (packetlen < sizeof(uint32_t)) {
						continue;
					}

					CSteamID mySteamID = SteamUser()->GetSteamID();
					if (mySteamID.ConvertToUint64() == newSteamID.ConvertToUint64()) {
						continue;
					}

			        if (!handleSafePacket()) {
			            Uint32 packetId = SDLNet_Read32(&net_packet->data[0]);
			            if (packetId == 'HELO') {
					        gotPacket = true;
					    }
					}
					break;
				}
#endif
			} else if (LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY) {
#ifdef USE_EOS
				for (Uint32 numpacket = 0; numpacket < PACKET_LIMIT; numpacket++) {
					if (!EOS.HandleReceivedMessages(&newRemoteProductId)) {
						continue;
					}

			        if (!handleSafePacket()) {
			            Uint32 packetId = SDLNet_Read32(&net_packet->data[0]);
			            if (packetId == 'HELO') {
					        gotPacket = true;
					    }
					}
					break;
				}
#endif // USE_EOS
			}

			// parse the packet:
			if (gotPacket) {
				clientnum = (int)SDLNet_Read32(&net_packet->data[4]);
				if (clientnum >= MAXPLAYERS || clientnum <= 0) {
                    int error = clientnum;
                    clientnum = 0;
					printlog("connection attempt denied by server, error code: %d.\n", error);
				    //flushP2PPackets(2000, 5000);

#ifdef STEAMWORKS
					if (!directConnect) {
						if (currentLobby) {
							SteamMatchmaking()->LeaveLobby(*static_cast<CSteamID*>(currentLobby));
							cpp_Free_CSteamID(currentLobby);
							currentLobby = NULL;
						}
					}
#endif
#if defined USE_EOS
					if (!directConnect) {
						if (EOS.CurrentLobbyData.currentLobbyIsValid()) {
							EOS.leaveLobby();
						}
					}
#endif

                    const char* error_str;
                    switch (error) {
                    case MAXPLAYERS + 0: error_str = language[1378]; break;
                    case MAXPLAYERS + 1: error_str = language[1379]; break;
                    case MAXPLAYERS + 2: error_str = language[1380]; break;
                    case MAXPLAYERS + 3: error_str = language[1381]; break;
                    case MAXPLAYERS + 4: error_str = language[1382]; break;
                    case MAXPLAYERS + 5: error_str = language[1383]; break;
                    default: error_str = language[1384]; break;
                    }

					// display error message
                    closePrompt("connect_prompt");
                    connectionErrorPrompt(error_str);

                    // reset connection
                    disconnectFromLobby(false);

#ifdef NINTENDO
					// recover wireless state
					if (directConnect) {
						nxShutdownWireless();
						if (!nxInitWireless()) {
							destroyMainMenu();
							createMainMenu(false);
						}
					}
#endif
                    return;
				} else {
					// join game succeeded, advance to lobby
					client_keepalive[0] = ticks;
					receivedclientnum = true;
					printlog("connected to server.\n");
					client_disconnected[clientnum] = false;
					if (!loadingsavegame) {
						stats[clientnum]->appearance = stats[0]->appearance;
					}

					// now set up everybody else
					for (int c = 0; c < MAXPLAYERS; c++) {
						const int chunk_size = loadingsavegame ?
							6 + 32 + 6 * 10:	// 6 bytes for player stats, 32 for name, 60 for equipment
							6 + 32;				// 6 bytes for player stats, 32 for name
							
						stats[c]->clearStats();
						client_disconnected[c] = net_packet->data[8 + c * chunk_size + 0]; // connectedness
						playerSlotsLocked[c] = net_packet->data[8 + c * chunk_size + 1]; // locked state
						client_classes[c] = net_packet->data[8 + c * chunk_size + 2]; // class
						stats[c]->sex = static_cast<sex_t>(net_packet->data[8 + c * chunk_size + 3]); // sex
						stats[c]->appearance = net_packet->data[8 + c * chunk_size + 4]; // appearance
						stats[c]->playerRace = net_packet->data[8 + c * chunk_size + 5]; // player race
						stringCopy(stats[c]->name, (char*)(net_packet->data + 8 + c * chunk_size + 6), sizeof(Stat::name), 32); // name

						if (loadingsavegame) {
							Item** player_slots[] = {
								&stats[c]->helmet,
								&stats[c]->breastplate,
								&stats[c]->gloves,
								&stats[c]->shoes,
								&stats[c]->shield,
								&stats[c]->weapon,
								&stats[c]->cloak,
								&stats[c]->amulet,
								&stats[c]->ring,
								&stats[c]->mask,
							};
							constexpr int num_slots = sizeof(player_slots) / sizeof(player_slots[0]);
							for (int e = 0; e < num_slots; ++e) {
								auto& slot = *player_slots[e];
								auto type = SDLNet_Read16(net_packet->data + 8 + c * chunk_size + 6 + 32 + e * 6);
								auto appearance = SDLNet_Read32(net_packet->data + 8 + c * chunk_size + 6 + 32 + e * 6 + 2);
								if (type != 0xffff) {
									slot = newItem((ItemType)type, Status::EXCELLENT, 0, 1, (Uint32)appearance, true, nullptr);
								}
							}
						} else {
							initClass(c);
						}
					}

					// open lobby
                    closePrompt("connect_prompt");
					createLobby(LobbyType::LobbyJoined);

                    // TODO subscribe to mods!
#if 0
#ifdef STEAMWORKS
					if (!directConnect && LobbyHandler.getJoiningType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM) {
						const char* serverNumModsChar = SteamMatchmaking()->GetLobbyData(*static_cast<CSteamID*>(currentLobby), "svNumMods");
						int serverNumModsLoaded = atoi(serverNumModsChar);
						if ( serverNumModsLoaded > 0 )
						{
							// subscribe to server loaded mods button
							button = newButton();
							strcpy(button->label, language[2984]);
							button->sizex = strlen(language[2984]) * 12 + 8;
							button->sizey = 20;
							button->x = subx2 - 4 - button->sizex;
							button->y = suby2 - 24;
							button->action = &buttonGamemodsSubscribeToHostsModFiles;
							button->visible = 1;
							button->focused = 1;

							// mount server mods button
							button = newButton();
							strcpy(button->label, language[2985]);
							button->sizex = strlen(language[2985]) * 12 + 8;
							button->sizey = 20;
							button->x = subx2 - 4 - button->sizex;
							button->y = suby2 - 24;
							button->action = &buttonGamemodsMountHostsModFiles;
							button->visible = 0;
							button->focused = 1;

							g_SteamWorkshop->CreateQuerySubscribedItems(k_EUserUGCList_Subscribed, k_EUGCMatchingUGCType_All, k_EUserUGCListSortOrder_LastUpdatedDesc);
							g_SteamWorkshop->subscribedCallStatus = 0;
						}
					}
#endif // STEAMWORKS
#endif
				}
			}
		} else { // aka, if (receivedclientnum == true)
#ifdef STEAMWORKS
		    CSteamID newSteamID;
		    joinLobbyWaitingForHostResponse = false;
#endif
#ifdef USE_EOS
		    EOS.bJoinLobbyWaitingForHostResponse = false;
#endif
		    for (int numpacket = 0; numpacket < PACKET_LIMIT && net_packet; numpacket++) {
			    if (directConnect) {
				    if (!SDLNet_UDP_Recv(net_sock, net_packet)) {
					    break;
				    }
			    } else {
				    if (LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM) {
#ifdef STEAMWORKS
					    uint32_t packetlen = 0;
					    if (!SteamNetworking()->IsP2PPacketAvailable(&packetlen, 0)) {
						    break;
					    }
					    packetlen = std::min<int>(packetlen, NET_PACKET_SIZE - 1);
					    Uint32 bytesRead = 0;
					    if (!SteamNetworking()->ReadP2PPacket(net_packet->data, packetlen, &bytesRead, &newSteamID, 0)) {
					        //Sometimes if a host closes a lobby, it can crash here for a client.
					        //NOTE: Are we sure about this in 2022?
						    continue;
					    }
					    net_packet->len = packetlen;
					    if (packetlen < sizeof(uint32_t)) {
						    continue;
					    }

					    CSteamID mySteamID = SteamUser()->GetSteamID();
					    if (mySteamID.ConvertToUint64() == newSteamID.ConvertToUint64()) {
						    continue;
					    }
#endif // STEAMWORKS
				    } else if (LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY) {
#ifdef USE_EOS
					    EOS_ProductUserId remoteId = nullptr;
					    if ( !EOS.HandleReceivedMessages(&remoteId) ) {
						    continue;
					    }
#endif // USE_EOS
				    }
			    }

                // unwrap a packet if it's marked "safe"
			    if (handleSafePacket()) {
				    continue;
			    }

			    Uint32 packetId = SDLNet_Read32(&net_packet->data[0]);
			    client_keepalive[0] = ticks;

			    auto find = clientPacketHandlers.find(packetId);
			    if (find == clientPacketHandlers.end()) {
                    // error
			        printlog("Got a mystery packet: %c%c%c%c",
			            (char)net_packet->data[0],
			            (char)net_packet->data[1],
			            (char)net_packet->data[2],
			            (char)net_packet->data[3]);
			    } else {
			        (*(find->second))(); // handle packet
			    }
			}
		}
	}

	static void handleNetwork() {
		if (!net_packet) {
			// I don't know why this can happen, but it can,
			// and it causes the game to crash. So stop it!
			return;
		}
	    if (multiplayer == SERVER) {
	        handlePacketsAsServer();
	    } else if (multiplayer == CLIENT) {
	        handlePacketsAsClient();
	    }
        doKeepAlive();

        // push username to lobby
        if (multiplayer != SINGLE && !directConnect) {
            if (LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY) {
#ifdef USE_EOS
			    if (EOS.CurrentLobbyData.currentLobbyIsValid()) {
			        if (multiplayer != CLIENT || clientnum != 0) {
			            if (EOS.CurrentLobbyData.getClientnumMemberAttribute(EOS.CurrentUserInfo.getProductUserIdHandle()) < 0) {
				            if (EOS.CurrentLobbyData.assignClientnumMemberAttribute(EOS.CurrentUserInfo.getProductUserIdHandle(), clientnum)) {
					            EOS.CurrentLobbyData.modifyLobbyMemberAttributeForCurrentUser();
				            }
				        }
			        }
			    }
#endif
			}

            if (LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM) {
#ifdef STEAMWORKS
				if (currentLobby) {
			        if (multiplayer != CLIENT || clientnum != 0) {
					    const char* memberNumChar = SteamMatchmaking()->GetLobbyMemberData(
					        *static_cast<CSteamID*>(currentLobby), SteamUser()->GetSteamID(), "clientnum");
					    if (memberNumChar) {
						    std::string str = memberNumChar;
						    if (str.empty() || std::to_string(clientnum) != str) {
							    SteamMatchmaking()->SetLobbyMemberData(*static_cast<CSteamID*>(currentLobby),
							        "clientnum", std::to_string(clientnum).c_str());
							    printlog("[STEAM Lobbies]: Updating clientnum %d to lobby member data", clientnum);
						    }
					    }
					}
				}
#endif
			}
		}
	}

	static void setupNetGameAsServer() {
	    // allocate data for client connections
	    net_clients = (IPaddress*) malloc(sizeof(IPaddress) * MAXPLAYERS);
	    net_tcpclients = (TCPsocket*) malloc(sizeof(TCPsocket) * MAXPLAYERS);
	    for (int c = 0; c < MAXPLAYERS; c++) {
		    net_tcpclients[c] = NULL;
	    }

	    // allocate packet data
	    net_packet = SDLNet_AllocPacket(NET_PACKET_SIZE);
	    assert(net_packet);

        // setup game
	    clientnum = 0;
	    multiplayer = SERVER;
	    if (loadingsavegame) {
			auto info = getSaveGameInfo(false);
			for (int c = 0; c < MAXPLAYERS; ++c) {
				if (info.players_connected[c]) {
					loadGame(c, info);
				}
			}
	    }
	}

#ifdef STEAMWORKS
	CSteamID* getLobbySteamID(const char* name) {
	    int lobbyID = -1;
        const char str[] = "steam:";
        size_t len = sizeof(str) - 1;
        if (strncmp(name, str, len) == 0) {
            lobbyID = (int)strtol(name + len, nullptr, 10);
        }
        if (lobbyID >= 0 && lobbyID < numSteamLobbies) {
            return (CSteamID*)lobbyIDs[lobbyID];
        } else {
            return nullptr;
        }
	}
#endif

#ifdef USE_EOS
    EOSFuncs::LobbyData_t* getLobbyEpic(const char* name) {
        int lobbyID = -1;
        const char str[] = "epic:";
        size_t len = sizeof(str) - 1;
        if (strncmp(name, str, len) == 0) {
            lobbyID = (int)strtol(name + len, nullptr, 10);
        }
        if (lobbyID >= 0 && lobbyID < EOS.LobbySearchResults.resultsSortedForDisplay.size()) {
			return EOS.LobbySearchResults.getResultFromDisplayedIndex(lobbyID);
        } else {
            return nullptr;
        }
    }
#endif

    static void sendJoinRequest() {
	    printlog("sending join request...\n");

		SaveGameInfo info;
		if (loadingsavegame) {
			info = getSaveGameInfo(false);
		}

	    const Uint32 index = loadingsavegame ?
	        std::min(info.player_num, MAXPLAYERS - 1) : 0;

	    // construct packet
	    memcpy(net_packet->data, "JOIN", 4);
	    stringCopy((char*)net_packet->data + 4, stats[index]->name, 32, sizeof(Stat::name));
	    SDLNet_Write32((Uint32)client_classes[index], &net_packet->data[36]);
	    SDLNet_Write32((Uint32)stats[index]->sex, &net_packet->data[40]);
	    Uint32 appearanceAndRace = ((Uint8)stats[index]->appearance << 8); // store in bits 8 - 15
	    appearanceAndRace |= (Uint8)stats[index]->playerRace; // store in bits 0 - 7
	    SDLNet_Write32(appearanceAndRace, &net_packet->data[44]);
	    stringCopy((char*)net_packet->data + 48, VERSION, 8, sizeof(VERSION));
	    net_packet->data[56] = index;
	    if (loadingsavegame) {
		    // send over the map seed being used
		    SDLNet_Write32(info.mapseed, &net_packet->data[57]);
	    } else {
		    SDLNet_Write32(0, &net_packet->data[57]);
	    }
	    SDLNet_Write32(loadingsavegame, &net_packet->data[61]); // send unique game key
	    net_packet->address.host = net_server.host;
	    net_packet->address.port = net_server.port;
	    net_packet->len = 65;

	    /*if (!directConnect) {
		    sendPacket(net_sock, -1, net_packet, 0);
		    SDL_Delay(5);
		    sendPacket(net_sock, -1, net_packet, 0);
		    SDL_Delay(5);
		    sendPacket(net_sock, -1, net_packet, 0);
		    SDL_Delay(5);
		    sendPacket(net_sock, -1, net_packet, 0);
		    SDL_Delay(5);
		    sendPacket(net_sock, -1, net_packet, 0);
		    SDL_Delay(5);
	    } else {
		    sendPacket(net_sock, -1, net_packet, 0);
		}*/
		sendPacket(net_sock, -1, net_packet, 0);
    }

	static char last_address[128] = "";

	static bool connectToServer(const char* address, void* pLobby, LobbyType lobbyType) {
	    if ((!address || address[0] == '\0') && (!pLobby || lobbyType != LobbyType::LobbyOnline)) {
	        soundError();
	        return false;
	    }

	    // reset keepalive
	    client_keepalive[0] = ticks;

	    // open wait prompt
        cancellablePrompt("connect_prompt", "", "Cancel", [](Widget& widget){
            char buf[256];
            int diff = ticks - client_keepalive[0];
            int part = diff % TICKS_PER_SECOND;
            int seconds = diff / TICKS_PER_SECOND;
            auto text = static_cast<Field*>(&widget);
            if (part < TICKS_PER_SECOND / 4) {
                snprintf(buf, sizeof(buf), "Joining lobby...\n\n%ds", seconds);
            } else if (part < 2 * TICKS_PER_SECOND / 4) {
                snprintf(buf, sizeof(buf), "Joining lobby...\n\n%ds.", seconds);
            } else if (part < 3 * TICKS_PER_SECOND / 4) {
                snprintf(buf, sizeof(buf), "Joining lobby...\n\n%ds..", seconds);
            } else {
                snprintf(buf, sizeof(buf), "Joining lobby...\n\n%ds...", seconds);
            }
            text->setText(buf);

            // here is the connection polling loop
			if (directConnect) {
				if (seconds >= 15) {
					systemErrorPrompt("Failed to connect to lobby.");
					closePrompt("connect_prompt");
#ifdef NINTENDO
					// recover wireless state
					nxShutdownWireless();
					if (!nxInitWireless()) {
						destroyMainMenu();
						createMainMenu(false);
					}
#endif
				}
			} else {
#ifdef STEAMWORKS
                if (LobbyHandler.getJoiningType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM) {
                    if (joinLobbyWaitingForHostResponse) {
		                if (connectingToLobbyStatus != EResult::k_EResultOK) {
		                    resetLobbyJoinFlowState();
							closePrompt("connect_prompt");

			                auto error_str = LobbyHandler_t::getLobbyJoinFailedConnectString(static_cast<int>(connectingToLobbyStatus));
							connectionErrorPrompt(error_str.c_str());
			                connectingToLobbyStatus = EResult::k_EResultOK;
			                return;
		                }
		                if (!connectingToLobby) {
		                    if (connectingToLobbyWindow) {
		                        resetLobbyJoinFlowState();

			                    // record CSteamID of lobby owner (and everybody else)
			                    // shouldn't this be in steam.cpp?
			                    for (int c = 0; c < MAXPLAYERS; ++c) {
				                    if (steamIDRemote[c]) {
					                    cpp_Free_CSteamID(steamIDRemote[c]);
					                    steamIDRemote[c] = NULL;
				                    }
			                    }
			                    const int lobbyMembers = SteamMatchmaking()->GetNumLobbyMembers(*static_cast<CSteamID*>(::currentLobby));
			                    for (int c = 0; c < lobbyMembers; ++c) {
				                    steamIDRemote[c] = cpp_SteamMatchmaking_GetLobbyMember(currentLobby, c);
			                    }
			                }

			                sendJoinRequest();
			            }
			            return;
		            }
                }
#endif
#ifdef USE_EOS
                if (LobbyHandler.getJoiningType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY) {
                    if (EOS.bJoinLobbyWaitingForHostResponse) {
		                if (EOS.ConnectingToLobbyStatus != static_cast<int>(EOS_EResult::EOS_Success)) {
		                    resetLobbyJoinFlowState();
							closePrompt("connect_prompt");

			                auto error_str = LobbyHandler_t::getLobbyJoinFailedConnectString(static_cast<int>(EOS.ConnectingToLobbyStatus));
							connectionErrorPrompt(error_str.c_str());
			                EOS.ConnectingToLobbyStatus = static_cast<int>(EOS_EResult::EOS_Success);
			                return;
		                }
		                if (!EOS.bConnectingToLobby) {
		                    if (EOS.bConnectingToLobbyWindow) {
		                        resetLobbyJoinFlowState();
			                }
			                sendJoinRequest();
			            }
			            return;
		            }
                }
#endif
            }
            },
            [](Button&){ // cancel
			closePrompt("connect_prompt");
            disconnectFromLobby(false);

#ifdef NINTENDO
			// recover wireless state
			if (directConnect) {
				nxShutdownWireless();
				if (!nxInitWireless()) {
					destroyMainMenu();
					createMainMenu(false);
				}
			}
#endif
            });

        // setup game state
	    multiplayer = CLIENT;
	    if (loadingsavegame) {
			auto info = getSaveGameInfo(false);
			for (int c = 0; c < MAXPLAYERS; ++c) {
				if (info.players_connected[c]) {
					loadGame(c, info);
				}
			}
	    }

	    // close any existing net interfaces
	    closeNetworkInterfaces();

	    // allocate packet data
	    net_packet = SDLNet_AllocPacket(NET_PACKET_SIZE);
	    assert(net_packet);

        // initialize connection
	    if (lobbyType == LobbyType::LobbyOnline) {
#ifdef STEAMWORKS
            {
                CSteamID* lobby = nullptr;
                if (address) {
                    const char steam_str[] = "steam:";
	                if (strncmp(address, steam_str, sizeof(steam_str) - 1) == 0) {
		                lobby = getLobbySteamID(address);
	                }
	                else if ((char)tolower((int)address[0]) == 's' && strlen(address) == 5) {
		                // save address for next time
		                stringCopyUnsafe(last_address, address, sizeof(last_address));
	                    connectingToLobby = true;
	                    connectingToLobbyWindow = true;
                        joinLobbyWaitingForHostResponse = true;
                        LobbyHandler.setLobbyJoinType(LobbyHandler_t::LobbyServiceType::LOBBY_STEAM);
                        LobbyHandler.setP2PType(LobbyHandler_t::LobbyServiceType::LOBBY_STEAM);
                        flushP2PPackets(100, 200);
                        requestingLobbies = true;
                        cpp_SteamMatchmaking_RequestLobbyList(address + 1);
		                return true;
	                }
	            }
	            else if (pLobby) {
	                lobby = static_cast<CSteamID*>(pLobby);
	            }
                if (lobby) {
		            connectingToLobby = true;
		            connectingToLobbyWindow = true;
                    joinLobbyWaitingForHostResponse = true;
                    //selectedSteamLobby = (int)strtol(address + 6, nullptr, 10);
                    LobbyHandler.setLobbyJoinType(LobbyHandler_t::LobbyServiceType::LOBBY_STEAM);
                    LobbyHandler.setP2PType(LobbyHandler_t::LobbyServiceType::LOBBY_STEAM);

                    flushP2PPackets(100, 200);
	                LobbyHandler.steamValidateAndJoinLobby(*lobby);
		            return true;
	            }
	        }
#endif
#ifdef USE_EOS
            {
                EOSFuncs::LobbyData_t* lobby = nullptr;
                if (address) {
                    const char epic_str[] = "epic:";
	                if (strncmp(address, epic_str, sizeof(epic_str) - 1) == 0) {
		                lobby = getLobbyEpic(address);
		            }
		            else if ((char)tolower((int)address[0]) == 'e' && strlen(address) == 5) {
						// save address for next time
						stringCopyUnsafe(last_address, address, sizeof(last_address));
#ifdef STEAMWORKS
						if (!LobbyHandler.crossplayEnabled) {
							// can't join an epic lobby if crossplay is not enabled
							connectionErrorPrompt("Failed to join lobby.\nCrossplay required.");
							goto failed;
						}
#endif
                        memcpy(EOS.lobbySearchByCode, address + 1, 4);
                        EOS.lobbySearchByCode[4] = '\0';
                        EOS.LobbySearchResults.useLobbyCode = true;
                        EOS.bConnectingToLobby = true;
                        EOS.bConnectingToLobbyWindow = true;
                        EOS.bJoinLobbyWaitingForHostResponse = true;
                        LobbyHandler.setLobbyJoinType(LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY);
                        LobbyHandler.setP2PType(LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY);
                        flushP2PPackets(100, 200);
                        EOS.searchLobbies(
                            EOSFuncs::LobbyParameters_t::LobbySearchOptions::LOBBY_SEARCH_ALL,
	                        EOSFuncs::LobbyParameters_t::LobbyJoinOptions::LOBBY_JOIN_FIRST_SEARCH_RESULT,
	                        "");
                        EOS.LobbySearchResults.useLobbyCode = false;
                        return true;
	                }
		        }
		        else if (pLobby) {
		            lobby = static_cast<EOSFuncs::LobbyData_t*>(pLobby);
		        }
	            if (lobby) {
                    EOS.bConnectingToLobby = true;
		            EOS.bConnectingToLobbyWindow = true;
                    EOS.bJoinLobbyWaitingForHostResponse = true;
                    LobbyHandler.setLobbyJoinType(LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY);
                    LobbyHandler.setP2PType(LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY);
		            strncpy(EOS.currentLobbyName, lobby->LobbyAttributes.lobbyName.c_str(), 31);

                    flushP2PPackets(100, 200);

		            // EOS.searchLobbies() nukes the lobby list, so we need to copy this.
		            std::string lobbyId = lobby->LobbyId.c_str();
		            EOS.searchLobbies(
		                EOSFuncs::LobbyParameters_t::LobbySearchOptions::LOBBY_SEARCH_BY_LOBBYID,
			            EOSFuncs::LobbyParameters_t::LobbyJoinOptions::LOBBY_JOIN_FIRST_SEARCH_RESULT,
			            lobbyId.c_str());
			        return true;
			    }
			}
#endif
			connectionErrorPrompt("Unable to join lobby.\nInvalid room code.");
	        goto failed;
        } else if (lobbyType == LobbyType::LobbyLAN) {
#ifdef NINTENDO
			assert(pLobby);
			int* lobby = static_cast<int*>(pLobby);
			if (!nxJoinLobby(*lobby)) {
				goto failed;
			}
#endif

            // copy address
            char address_copy[128];
            int address_len = (int)strlen(address);
            address_len = std::min(address_len, (int)(sizeof(address_copy) - 1));
            memcpy(address_copy, address, address_len);
            address_copy[address_len] = '\0';
            Uint16 port;

            // find appended port number
            int port_index = 0;
	        for (; port_index <= address_len; ++port_index) {
		        if (address_copy[port_index] == ':') {
		            address_copy[port_index] = '\0';
		            ++port_index;
			        break;
		        }
	        }

            // read port number
		    char *port_err;
		    port = (Uint16)strtol(&address_copy[port_index], &port_err, 10);
		    if (*port_err != '\0' || port < 1024) {
			    printlog("warning: invalid port number (%hu). Using default (%hu)\n", port, DEFAULT_PORT);
                port = DEFAULT_PORT;
		    }

		    // save address for next time
		    stringCopyUnsafe(last_address, address, sizeof(last_address));

		    // resolve host's address
		    printlog("resolving host's address at %s...\n", address);
		    if (SDLNet_ResolveHost(&net_server, address_copy, port) == -1) {
			    char buf[1024];
			    snprintf(buf, sizeof(buf), "Failed to resolve host at:\n%s", address);
				systemErrorPrompt(buf);
				printlog(buf);
				goto failed;
		    }

		    // open sockets
		    printlog("opening UDP socket...\n");
		    if (!(net_sock = SDLNet_UDP_Open(NETWORK_PORT_CLIENT))) {
			    char buf[1024];
			    snprintf(buf, sizeof(buf), "Failed to open UDP socket.");
				systemErrorPrompt(buf);
				printlog(buf);
				goto failed;
		    }

		    printlog("successfully contacted server at %s.\n", address);
		    sendJoinRequest();
	        return true;
	    }

	    // connection initiation failed for unknown reason
		connectionErrorPrompt("Failed to join lobby.");

failed:
	    closePrompt("connect_prompt");
	    disconnectFromLobby(false);

#ifdef NINTENDO
		// recover wireless state
		if (directConnect) {
			nxShutdownWireless();
			if (!nxInitWireless()) {
				destroyMainMenu();
				createMainMenu(false);
			} else {
				refreshLobbyBrowser();
			}
		}
#endif

	    return false;
	}

/******************************************************************************/

	enum class DLC {
		Base,
		MythsAndOutcasts,
		LegendsAndPariahs
	};

	struct Class {
		const char* name;
		DLC dlc;
		const char* image;
		const char* image_highlighted;
		const char* image_locked;
	};

	static const std::unordered_map<std::string, Class> classes = {
		{"barbarian", {
			"Barbarian", DLC::Base,
			"ClassSelect_Icon_Barbarian_00.png",
			"ClassSelect_Icon_BarbarianOn_00.png",
			"ClassSelect_Icon_BarbarianLocked_00.png",
			}},
		{"warrior", {
			"Warrior", DLC::Base,
			"ClassSelect_Icon_Warrior_00.png",
			"ClassSelect_Icon_WarriorOn_00.png",
			"ClassSelect_Icon_WarriorLocked_00.png",
			}},
		{"healer", {
			"Healer", DLC::Base,
			"ClassSelect_Icon_Healer_00.png",
			"ClassSelect_Icon_HealerOn_00.png",
			"ClassSelect_Icon_HealerLocked_00.png",
			}},
		{"rogue", {
			"Rogue", DLC::Base,
			"ClassSelect_Icon_Rogue_00.png",
			"ClassSelect_Icon_RogueOn_00.png",
			"ClassSelect_Icon_RogueLocked_00.png",
			}},
		{"wanderer", {
			"Wanderer", DLC::Base,
			"ClassSelect_Icon_Wanderer_00.png",
			"ClassSelect_Icon_WandererOn_00.png",
			"ClassSelect_Icon_WandererLocked_00.png",
			}},
		{"cleric", {
			"Cleric", DLC::Base,
			"ClassSelect_Icon_Cleric_00.png",
			"ClassSelect_Icon_ClericOn_00.png",
			"ClassSelect_Icon_ClericLocked_00.png",
			}},
		{"merchant", {
			"Merchant", DLC::Base,
			"ClassSelect_Icon_Merchant_00.png",
			"ClassSelect_Icon_MerchantOn_00.png",
			"ClassSelect_Icon_MerchantLocked_00.png",
			}},
		{"wizard", {
			"Wizard", DLC::Base,
			"ClassSelect_Icon_Wizard_00.png",
			"ClassSelect_Icon_WizardOn_00.png",
			"ClassSelect_Icon_WizardLocked_00.png",
			}},
		{"arcanist", {
			"Arcanist", DLC::Base,
			"ClassSelect_Icon_Arcanist_00.png",
			"ClassSelect_Icon_ArcanistOn_00.png",
			"ClassSelect_Icon_ArcanistLocked_00.png",
			}},
		{"joker", {
			"Joker", DLC::Base,
			"ClassSelect_Icon_Jester_00.png",
			"ClassSelect_Icon_JesterOn_00.png",
			"ClassSelect_Icon_JesterLocked_00.png",
			}},
		{"sexton", {
			"Sexton", DLC::Base,
			"ClassSelect_Icon_Sexton_00.png",
			"ClassSelect_Icon_SextonOn_00.png",
			"ClassSelect_Icon_SextonLocked_00.png",
			}},
		{"ninja", {
			"Ninja", DLC::Base,
			"ClassSelect_Icon_Ninja_00.png",
			"ClassSelect_Icon_NinjaOn_00.png",
			"ClassSelect_Icon_NinjaLocked_00.png",
			}},
		{"monk", {
			"Monk", DLC::Base,
			"ClassSelect_Icon_Monk_00.png",
			"ClassSelect_Icon_MonkOn_00.png",
			"ClassSelect_Icon_MonkLocked_00.png",
			}},
		{"conjurer", {
			"Conjurer", DLC::MythsAndOutcasts,
			"ClassSelect_Icon_Conjurer_00.png",
			"ClassSelect_Icon_ConjurerOn_00.png",
			"ClassSelect_Icon_ConjurerLocked_00.png",
			}},
		{"accursed", {
			"Accursed", DLC::MythsAndOutcasts,
			"ClassSelect_Icon_Accursed_00.png",
			"ClassSelect_Icon_AccursedOn_00.png",
			"ClassSelect_Icon_AccursedLocked_00.png",
			}},
		{"mesmer", {
			"Mesmer", DLC::MythsAndOutcasts,
			"ClassSelect_Icon_Mesmer_00.png",
			"ClassSelect_Icon_MesmerOn_00.png",
			"ClassSelect_Icon_MesmerLocked_00.png",
			}},
		{"brewer", {
			"Brewer", DLC::MythsAndOutcasts,
			"ClassSelect_Icon_Brewer_00.png",
			"ClassSelect_Icon_BrewerOn_00.png",
			"ClassSelect_Icon_BrewerLocked_00.png",
			}},
		{"mechanist", {
			"Mechanist", DLC::LegendsAndPariahs,
			"ClassSelect_Icon_Mechanist_00.png",
			"ClassSelect_Icon_MechanistOn_00.png",
			"ClassSelect_Icon_MechanistLocked_00.png",
			}},
		{"punisher", {
			"Punisher", DLC::LegendsAndPariahs,
			"ClassSelect_Icon_Punisher_00.png",
			"ClassSelect_Icon_PunisherOn_00.png",
			"ClassSelect_Icon_PunisherLocked_00.png",
			}},
		{"shaman", {
			"Shaman", DLC::LegendsAndPariahs,
			"ClassSelect_Icon_Shaman_00.png",
			"ClassSelect_Icon_ShamanOn_00.png",
			"ClassSelect_Icon_ShamanLocked_00.png",
			}},
		{"hunter", {
			"Hunter", DLC::LegendsAndPariahs,
			"ClassSelect_Icon_Hunter_00.png",
			"ClassSelect_Icon_HunterOn_00.png",
			"ClassSelect_Icon_HunterLocked_00.png",
			}},
	};

	static const char* classes_in_order[] = {
		"barbarian", "warrior", "healer",
		"rogue", "wanderer", "cleric", "merchant",
		"wizard", "arcanist", "joker", "sexton",
		"ninja", "monk", "conjurer", "accursed",
		"mesmer", "brewer", "mechanist", "punisher",
		"shaman", "hunter"
	};

	static const char* races[] = {
	    "Human",
		"Skeleton",
		"Vampire",
		"Succubus",
		"Goatman",
		"Automaton",
		"Incubus",
		"Goblin",
		"Insectoid",
	};
	static constexpr int num_races = sizeof(races) / sizeof(races[0]);

	bool ClassDescriptions::init = false;
	std::unordered_map<int, ClassDescriptions::DescData_t> ClassDescriptions::data;

	bool RaceDescriptions::init = false;
	std::unordered_map<std::string, RaceDescriptions::DescData_t> RaceDescriptions::data;

	void ClassDescriptions::readFromFile()
	{
		const std::string filename = "data/class_descriptions.json";
		if ( !PHYSFS_getRealDir(filename.c_str()) )
		{
			printlog("[JSON]: Error: Could not locate json file %s", filename.c_str());
			return;
		}

		std::string inputPath = PHYSFS_getRealDir(filename.c_str());
		inputPath.append(PHYSFS_getDirSeparator());
		inputPath.append(filename.c_str());

		File* fp = FileIO::open(inputPath.c_str(), "rb");
		if ( !fp )
		{
			printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
			return;
		}

		char buf[32000];
		const int count = (int)fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
		buf[count] = '\0';
		rapidjson::StringStream is(buf);
		FileIO::close(fp);

		rapidjson::Document d;
		d.ParseStream(is);
		if ( !d.HasMember("version") || !d.HasMember("descriptions") )
		{
			printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
			return;
		}

		data.clear();

		static constexpr Uint32 good = makeColorRGB(0, 191, 255);
		static constexpr Uint32 decent = makeColorRGB(0, 191, 255);
		static constexpr Uint32 average = makeColorRGB(191, 191, 191);
		static constexpr Uint32 poor = makeColorRGB(255, 64, 0);
		static constexpr Uint32 bad = makeColorRGB(255, 64, 0);

		auto& classes = d["descriptions"];
		Stat tmpStats(0);
		for ( auto it = classes.MemberBegin(); it != classes.MemberEnd(); ++it )
		{
			std::string classname = it->name.GetString();
			int key = it->value["id"].GetInt();
			auto& classEntry = data[key];
			classEntry.internal_name = classname;

			tmpStats.clearStats();
			initClassStats(key, &tmpStats);
			classEntry.hp = tmpStats.HP;
			classEntry.mp = tmpStats.MP;

			for ( auto it2 = it->value["desc"].Begin(); it2 != it->value["desc"].End(); )
			{
				std::string line = (it2->GetString());
				if ( line.size() > 0 && line[0] == '-' )
				{
					line[0] = '\x1E';
				}
				classEntry.text += line;

				++it2;
				if ( it2 != it->value["desc"].End() )
				{
					classEntry.text += '\n';
				}
			}
			for ( auto it2 = it->value["line_spacing"].Begin(); it2 != it->value["line_spacing"].End(); ++it2 )
			{
				classEntry.linePaddings.push_back(it2->GetInt());
			}

			int c = 0;
			for ( auto it2 = it->value["survival_complexity"].Begin(); it2 != it->value["survival_complexity"].End(); ++it2 )
			{
				int value = it2->GetInt();
				classEntry.survivalComplexity.push_back(std::make_tuple(value, "", 0));
				auto& survivalComplexity = classEntry.survivalComplexity.back();
				switch ( value )
				{
					case 1: 
						std::get<1>(survivalComplexity) = "*"; 
						std::get<2>(survivalComplexity) = (c == 0 ? bad : good);
						break;
					case 2: 
						std::get<1>(survivalComplexity) = "**"; 
						std::get<2>(survivalComplexity) = (c == 0 ? poor : decent);
						break;
					case 3: 
						std::get<1>(survivalComplexity) = "***"; 
						std::get<2>(survivalComplexity) = (c == 0 ? average : average);
						break;
					case 4: 
						std::get<1>(survivalComplexity) = "****"; 
						std::get<2>(survivalComplexity) = (c == 0 ? decent : poor);
						break;
					case 5: 
						std::get<1>(survivalComplexity) = "*****"; 
						std::get<2>(survivalComplexity) = (c == 0 ? good : bad); 
						break;
					default:
						break;
				}
				++c;
			}
			for ( auto it2 = it->value["stats"].Begin(); it2 != it->value["stats"].End(); ++it2 )
			{
				std::string value = it2->GetString();
				classEntry.statRatingsStrings.push_back(value);
				if ( value == "bad" )
				{
					classEntry.statRatings.push_back(bad);
				}
				else if ( value == "poor" )
				{
					classEntry.statRatings.push_back(poor);
				}
				else if ( value == "average" )
				{
					classEntry.statRatings.push_back(average);
				}
				else if ( value == "decent" )
				{
					classEntry.statRatings.push_back(decent);
				}
				else if ( value == "good" )
				{
					classEntry.statRatings.push_back(good);
				}
				else
				{
					classEntry.statRatings.push_back(0);
				}
			}
		}
		init = true;
		printlog("[JSON]: Successfully read json file %s", inputPath.c_str());
	}

	RaceDescriptions::DescData_t& RaceDescriptions::getMonsterDescriptionData(int type) 
	{ 
		return data[monstertypename[type]]; 
	}

	void RaceDescriptions::readFromFile()
	{
		const std::string filename = "data/race_descriptions.json";
		if ( !PHYSFS_getRealDir(filename.c_str()) )
		{
			printlog("[JSON]: Error: Could not locate json file %s", filename.c_str());
			return;
		}

		std::string inputPath = PHYSFS_getRealDir(filename.c_str());
		inputPath.append(PHYSFS_getDirSeparator());
		inputPath.append(filename.c_str());

		File* fp = FileIO::open(inputPath.c_str(), "rb");
		if ( !fp )
		{
			printlog("[JSON]: Error: Could not locate json file %s", inputPath.c_str());
			return;
		}

		static char buf[16000];
		const int count = (int)fp->read(buf, sizeof(buf[0]), sizeof(buf) - 1);
		buf[count] = '\0';
		rapidjson::StringStream is(buf);
		FileIO::close(fp);

		rapidjson::Document d;
		d.ParseStream(is);
		if ( !d.HasMember("version") || !d.HasMember("descriptions") )
		{
			printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
			return;
		}

		data.clear();

		auto& races = d["descriptions"];
		for ( auto it = races.MemberBegin(); it != races.MemberEnd(); ++it )
		{
			std::string key = it->name.GetString();
			auto& raceEntry = data[key];
			raceEntry.title = it->value["title"].GetString();
			std::vector<std::string> textLeftLines;
			std::vector<std::string> textRightLines;
			for ( auto it2 = it->value["left_align"].Begin(); it2 != it->value["left_align"].End(); )
			{
				std::string line = (it2->GetString());
				if ( line.size() > 0 && line[0] == '-' )
				{
					line[0] = '\x1E';
				}
				raceEntry.textLeft += line;
				textLeftLines.push_back(line);
				++it2;
				if ( it2 != it->value["left_align"].End() )
				{
					raceEntry.textLeft += '\n';
				}
			}
			raceEntry.traitsBasedOnPlayerRace = "";
			for ( auto it2 = it->value["traits_active_if_polymorphed_out_of"].Begin(); 
				it2 != it->value["traits_active_if_polymorphed_out_of"].End(); )
			{
				raceEntry.traitsBasedOnPlayerRace += (textLeftLines[it2->GetInt()]);
				++it2;
				if ( it2 != it->value["traits_active_if_polymorphed_out_of"].End() )
				{
					raceEntry.traitsBasedOnPlayerRace += '\n';
				}
			}
			raceEntry.traitsBasedOnMonsterType = "";
			for ( auto it2 = it->value["traits_active_if_polymorphed_into"].Begin();
				it2 != it->value["traits_active_if_polymorphed_into"].End(); )
			{
				raceEntry.traitsBasedOnMonsterType += (textLeftLines[it2->GetInt()]);
				++it2;
				if ( it2 != it->value["traits_active_if_polymorphed_into"].End() )
				{
					raceEntry.traitsBasedOnMonsterType += '\n';
				}
			}
			for ( auto it2 = it->value["right_align"].Begin(); it2 != it->value["right_align"].End(); )
			{
				std::string line = (it2->GetString());
				if ( line.size() > 0 && line[0] == '-' )
				{
					line[0] = '\x1E';
				}
				raceEntry.textRight += line;
				textRightLines.push_back(line);
				++it2;
				if ( it2 != it->value["right_align"].End() )
				{
					raceEntry.textRight += '\n';
				}
			}

			auto& highlights = it->value["text_line_highlights"];
			int minProLine = 999;
			int spellProLine = 999;
			int maxProLine = 0;
			for ( auto it2 = highlights["traits_lines"].Begin(); it2 != highlights["traits_lines"].End(); ++it2 )
			{
				raceEntry.traitLines.insert(it2->GetInt());
			}
			for ( auto it2 = highlights["benefit_lines"].Begin(); it2 != highlights["benefit_lines"].End(); ++it2 )
			{
				raceEntry.proLines.insert(it2->GetInt());
				if ( it2->GetInt() > 1 ) // skip racial spells
				{
					minProLine = std::min(minProLine, it2->GetInt());
				}
				else
				{
					spellProLine = std::min(spellProLine, it2->GetInt());
				}
				maxProLine = std::max(maxProLine, it2->GetInt());
			}
			for ( auto it2 = it->value["line_spacing"].Begin(); it2 != it->value["line_spacing"].End(); ++it2 )
			{
				raceEntry.linePaddings.push_back(it2->GetInt());
			}
			
			int numweaknesses = 0;
			int numresistances = 0;
			for ( size_t index = minProLine; index < textLeftLines.size(); ++index )
			{
				if ( textLeftLines[index] == "" ) {	break; }
				if ( raceEntry.resistances != "" )
				{
					raceEntry.resistances += '\n';
				}
				raceEntry.resistances += textLeftLines[index];
				++numresistances;
			}
			for ( size_t index = maxProLine; index < textLeftLines.size(); ++index )
			{
				if ( textLeftLines[index] == "" ) { break; }
				if ( raceEntry.friendlyWith != "" )
				{
					raceEntry.friendlyWith += '\n';
				}
				raceEntry.friendlyWith += textLeftLines[index];
			}
			for ( size_t index = minProLine; index < textRightLines.size(); ++index )
			{
				if ( textRightLines[index] == "" ) { break; }
				if ( raceEntry.weaknesses != "" )
				{
					raceEntry.weaknesses += '\n';
				}
				raceEntry.weaknesses += textRightLines[index];
				++numweaknesses;
			}
			while ( numweaknesses > numresistances )
			{
				raceEntry.resistances += '\n';
				++numresistances;
			}
			if ( spellProLine < 999 )
			{
				for ( size_t index = spellProLine; index < textLeftLines.size(); ++index )
				{
					if ( textLeftLines[index] == "" ) { break; }
					if ( index > spellProLine )
					{
						if ( textLeftLines[index][0] != '\x1E' ) { break; }
					}
					if ( raceEntry.racialSpells != "" )
					{
						raceEntry.racialSpells += '\n';
					}
					raceEntry.racialSpells += textLeftLines[index];
				}
			}
		}
		init = true;
		printlog("[JSON]: Successfully read json file %s", inputPath.c_str());
	}

	constexpr int num_classes = sizeof(classes_in_order) / sizeof(classes_in_order[0]);

    constexpr Uint32 color_dlc0 = makeColorRGB(169, 185, 212);
    constexpr Uint32 color_dlc1 = makeColorRGB(241, 129, 78);
    constexpr Uint32 color_dlc2 = makeColorRGB(255, 53, 206);
	constexpr Uint32 color_traits = makeColorRGB(184, 146, 109);
	constexpr Uint32 color_pro = makeColorRGB(88, 203, 255);
	constexpr Uint32 color_con = makeColorRGB(255, 56, 56);
	constexpr Uint32 color_energy = makeColorRGB(21, 255, 0);
	constexpr Uint32 color_heat = makeColorRGB(241, 129, 78);

	std::vector<const char*> reducedClassList(int index) {
		std::vector<const char*> result;
		result.reserve(num_classes);
		for (int c = CLASS_BARBARIAN; c <= CLASS_HUNTER; ++c) {
			if (isCharacterValidFromDLC(*stats[index], c) == VALID_OK_CHARACTER) {
				result.emplace_back(classes_in_order[c]);
			}
		}
		return result;
	}

	void RaceDescriptions::update_details_text(Frame& card, void* stats) {
		
		Monster race = HUMAN;
		if ( static_cast<Stat*>(stats)->appearance == 0 && static_cast<Stat*>(stats)->playerRace != RACE_HUMAN )
		{
			race = getMonsterFromPlayerRace(static_cast<Stat*>(stats)->playerRace);
		}
		Monster modifiedRace = static_cast<Stat*>(stats)->type;
		if ( arachnophobia_filter )
		{
			if ( modifiedRace == SPIDER )
			{
				modifiedRace = CRAB;
			}
			if ( race == SPIDER )
			{
				race = CRAB;
			}
		}

		auto& raceDescriptionData = RaceDescriptions::getMonsterDescriptionData(modifiedRace);
		auto& raceDescriptionDataBase = RaceDescriptions::getMonsterDescriptionData(race);
		auto details_text = card.findField("details"); assert(details_text);
		auto details_text_right = card.findField("details_right"); assert(details_text_right);
		std::string details_text_buf = "";
		std::string details_text_right_buf = "";
		if ( details_text ) {
			details_text->clearLinesToColor();
			details_text->clearIndividualLinePadding();
			details_text->setPaddingPerLine(-2);
			details_text_right->clearLinesToColor();
			details_text_right->clearIndividualLinePadding();
			details_text_right->setPaddingPerLine(-2);

			int line = 0;
			int numIndividualPadding = 0;

			size_t c = 0;
			if ( raceDescriptionDataBase.racialSpells.size() > 0 )
			{
				details_text_buf += raceDescriptionDataBase.racialSpells;
				if ( race != modifiedRace )
				{
					// Add (Insectoid) etc if we're polymorphed from our base race.
					char buf[64] = "";
					auto localizedName = getMonsterLocalizedName(race);
					camelCaseString(localizedName);
					snprintf(buf, sizeof(buf), " (%s)", localizedName.c_str());
					for ( size_t d = 0; d < details_text_buf.size(); ++d )
					{
						// first newline "Innate Spells", insert the monster type name here
						if ( details_text_buf[d] == '\n' ) 
						{
							details_text_buf.insert(d, buf);
							break;
						}
					}
				}
				details_text_buf += '\n';
				details_text_buf += '\n';
				details_text->addColorToLine(line, color_pro);

				for ( ; c < details_text_buf.size(); ++c )
				{
					if ( details_text_buf[c] == '\n' )
					{
						++line;
						details_text_right_buf += '\n';
					}
				}
				details_text->setIndividualLinePadding(line - 1, -16);
				details_text_right->setIndividualLinePadding(line - 1, -16);
				++numIndividualPadding;
			}

			details_text_buf += Player::CharacterSheet_t::getHoverTextString("race_traits");
			details_text_buf += '\n';
			details_text->addColorToLine(line, color_traits);
			details_text_buf += raceDescriptionData.traitsBasedOnMonsterType;
			if ( race != modifiedRace )
			{
				if ( raceDescriptionDataBase.traitsBasedOnPlayerRace != "" )
				{
					details_text_buf += '\n';
					details_text_buf += raceDescriptionDataBase.traitsBasedOnPlayerRace;
				}
			}
			details_text_buf += '\n';
			details_text_buf += '\n';
			for ( ; c < details_text_buf.size(); ++c )
			{
				if ( details_text_buf[c] == '\n' )
				{ 
					++line;
					details_text_right_buf += '\n';
				}
			}

			details_text->setIndividualLinePadding(line - 1, -16);
			++numIndividualPadding;
			details_text->addColorToLine(line, color_pro);
			details_text_right->setIndividualLinePadding(line - 1, -16);
			details_text_right->addColorToLine(line, color_con);

			details_text_buf += raceDescriptionData.resistances;
			details_text_right_buf += raceDescriptionData.weaknesses;
			details_text_buf += '\n';
			details_text_buf += '\n';
			for ( ; c < details_text_buf.size(); ++c )
			{
				if ( details_text_buf[c] == '\n' ) { ++line; }
			}

			details_text->setIndividualLinePadding(line - 1, -16);
			++numIndividualPadding;
			details_text->addColorToLine(line, color_pro);
			details_text_buf += raceDescriptionData.friendlyWith;
			details_text->setText(details_text_buf.c_str());
			details_text_right->setText(details_text_right_buf.c_str());

			if ( auto actualFont = Font::get(details_text->getFont()) )
			{
				const int numlines = details_text->getNumTextLines();
				const int pad = details_text->getPaddingPerLine();
				const int actualHeight = actualFont->height(true);
				int height = 0;
				for ( int line = 0; line < numlines; ++line )
				{
					height += actualHeight + pad;
				}

				if ( height < Frame::virtualScreenY / 2 )
				{
					int extraSpace = (Frame::virtualScreenY / 2) - height / std::max(1,  numIndividualPadding);
					extraSpace = std::min(8, extraSpace);
					for ( int line = 0; line < numlines; ++line )
					{
						if ( int pad = details_text->getIndividualLinePadding(line) )
						{
							pad += extraSpace;
							details_text->setIndividualLinePadding(line, pad);
						}
						if ( int pad = details_text_right->getIndividualLinePadding(line) )
						{
							pad += extraSpace;
							details_text_right->setIndividualLinePadding(line, pad);
						}
					}
				}
			}
		}
	}

	void ClassDescriptions::update_stat_growths(Frame& card, int classnum, int shapeshiftedType)
	{
		// stats definitions
		const char* class_stats_text[] = {
			"STR", "DEX", "CON", "INT", "PER", "CHR"
		};
		constexpr int num_class_stats = sizeof(class_stats_text) / sizeof(class_stats_text[0]);

		switch ( shapeshiftedType )
		{
			case RAT:
				classnum = 100;
				break;
			case SPIDER:
				classnum = 101;
				break;
			case TROLL:
				classnum = 102;
				break;
			case CREATURE_IMP:
				classnum = 103;
				break;
			default:
				break;
		}

		for ( int c = 0; c < num_class_stats; ++c )
		{
			static char buf[16];
			snprintf(buf, sizeof(buf), "%d", c);
			auto field = card.findField(buf);
			field->setColor(ClassDescriptions::data[classnum].statRatings[c]);

			char buf2[32];
			snprintf(buf2, sizeof(buf2), "stat img bottom %d", c);
			auto class_stat_img_bottom = card.findImage(buf2);
			if ( !class_stat_img_bottom )
			{
				return;
			}

			if ( ClassDescriptions::data[classnum].statRatingsStrings[c] == "bad" )
			{
				//class_stat_img_top->disabled = true;
				class_stat_img_bottom->disabled = false;

				class_stat_img_bottom->path =
					"*#images/ui/Main Menus/Play/PlayerCreation/ClassSelection/statgrowth_lo2.png";
			}
			else if ( ClassDescriptions::data[classnum].statRatingsStrings[c] == "poor" )
			{
				//class_stat_img_top->disabled = true;
				class_stat_img_bottom->disabled = false;

				class_stat_img_bottom->path =
					"*#images/ui/Main Menus/Play/PlayerCreation/ClassSelection/statgrowth_lo1.png";
			}
			else if ( ClassDescriptions::data[classnum].statRatingsStrings[c] == "decent" )
			{
				//class_stat_img_top->disabled = false;
				class_stat_img_bottom->disabled = false;

				class_stat_img_bottom->path =
					"*#images/ui/Main Menus/Play/PlayerCreation/ClassSelection/statgrowth_hi1.png";
			}
			else if ( ClassDescriptions::data[classnum].statRatingsStrings[c] == "good" )
			{
				//class_stat_img_top->disabled = false;
				class_stat_img_bottom->disabled = false;

				class_stat_img_bottom->path =
					"*#images/ui/Main Menus/Play/PlayerCreation/ClassSelection/statgrowth_hi2.png";
			}
			else
			{
				//class_stat_img_top->disabled = true;
				class_stat_img_bottom->disabled = false;

				class_stat_img_bottom->path =
					"*#images/ui/Main Menus/Play/PlayerCreation/ClassSelection/statgrowth_neutral.png";
			}
		}
	}

	void RaceDescriptions::update_details_text(Frame& card) {
		const int index = card.getOwner();
		const int race = stats[index]->playerRace;

	    // color title
	    Uint32 color_race;
	    if (race >= RACE_SKELETON && race <= RACE_GOATMAN) {
	        color_race = color_dlc1;
	    }
	    else if (race >= RACE_AUTOMATON && race <= RACE_INSECTOID) {
	        color_race = color_dlc2;
	    }
	    else {
	        color_race = color_dlc0;
	    }

		auto& raceDescriptionData = RaceDescriptions::getRaceDescriptionData(race);

	    auto details_title = card.findField("details_title");
	    if (details_title) {
	        details_title->clearLinesToColor();
	        details_title->setText(raceDescriptionData.title.c_str());
            details_title->setColor(color_race);
	    }

	    auto details_text = card.findField("details");
	    if (details_text) {
	        details_text->clearLinesToColor();
			details_text->setText(raceDescriptionData.textLeft.c_str());
			for ( auto line : raceDescriptionData.traitLines )
			{
				details_text->addColorToLine(line, color_traits);
			}
			for ( auto line : raceDescriptionData.proLines )
			{
				details_text->addColorToLine(line, color_pro);
			}
			details_text->clearIndividualLinePadding();
			for ( auto line = 0; line < raceDescriptionData.linePaddings.size(); ++line )
			{
				details_text->setIndividualLinePadding(line, raceDescriptionData.linePaddings[line]);
			}
	    }
	    auto details_text_right = card.findField("details_right");
	    if (details_text_right) {
	        details_text_right->clearLinesToColor();
			details_text_right->setText(raceDescriptionData.textRight.c_str());

            // we expect first word in the right column to always be "Weaknesses"
            int c;
            for (c = 0; raceDescriptionData.textRight[c] == '\n'; ++c);
            details_text_right->addColorToLine(c, color_con);
			details_text_right->clearIndividualLinePadding();
			for ( auto line = 0; line < raceDescriptionData.linePaddings.size(); ++line )
			{
				details_text_right->setIndividualLinePadding(line, raceDescriptionData.linePaddings[line]);
			}
	    }
    }

	static void race_button_fn(Button& button, bool override_dlc) {
        const int index = button.getOwner();
		auto frame = static_cast<Frame*>(button.getParent()); assert(frame);
        bool success = false;
		for (int c = 0; c < num_races; ++c) {
			auto race = races[c];
			if (strcmp(button.getName(), race) == 0) {
				if (!override_dlc &&
                    ((!enabledDLCPack1 && c >= 1 && c <= 4) ||
					(!enabledDLCPack2 && c >= 5 && c <= 8))) {
					// this class is not available to the player
					button.setPressed(false);
					openDLCPrompt(c >= 5 ? 1 : 0);
					return;
				} else {
                    success = true;
					soundToggle();
					stats[index]->playerRace = c;
					if (stats[index]->playerRace == RACE_SUCCUBUS) {
						stats[index]->appearance = 0;
						stats[index]->sex = FEMALE;
						auto card = static_cast<Frame*>(frame->getParent()); assert(card);
						auto bottom = card->findFrame("bottom"); assert(bottom);
						auto female = bottom->findButton("female");
						auto male = bottom->findButton("male");
						female->setPressed(stats[index]->sex == FEMALE);
						female->setColor(stats[index]->sex == FEMALE ? makeColorRGB(255, 255, 255) : makeColorRGB(127, 127, 127));
						female->setHighlightColor(stats[index]->sex == FEMALE ? makeColorRGB(255, 255, 255) : makeColorRGB(127, 127, 127));
						male->setPressed(stats[index]->sex == MALE);
						male->setColor(stats[index]->sex == MALE ? makeColorRGB(255, 255, 255) : makeColorRGB(127, 127, 127));
						male->setHighlightColor(stats[index]->sex == MALE ? makeColorRGB(255, 255, 255) : makeColorRGB(127, 127, 127));
					}
					else if (stats[index]->playerRace == RACE_INCUBUS) {
						stats[index]->appearance = 0;
						stats[index]->sex = MALE;
						auto card = static_cast<Frame*>(frame->getParent()); assert(card);
						auto bottom = card->findFrame("bottom"); assert(bottom);
						auto female = bottom->findButton("female");
						auto male = bottom->findButton("male");
						female->setPressed(stats[index]->sex == FEMALE);
						female->setColor(stats[index]->sex == FEMALE ? makeColorRGB(255, 255, 255) : makeColorRGB(127, 127, 127));
						female->setHighlightColor(stats[index]->sex == FEMALE ? makeColorRGB(255, 255, 255) : makeColorRGB(127, 127, 127));
						male->setPressed(stats[index]->sex == MALE);
						male->setColor(stats[index]->sex == MALE ? makeColorRGB(255, 255, 255) : makeColorRGB(127, 127, 127));
						male->setHighlightColor(stats[index]->sex == MALE ? makeColorRGB(255, 255, 255) : makeColorRGB(127, 127, 127));
					}
					else if (stats[index]->playerRace == RACE_HUMAN) {
                        auto appearances = frame->findFrame("appearances");
                        if (inputs.hasController(index)) {
                            // get the appearance that is currently selected in the UI
                            stats[index]->appearance = std::max(0, appearances->getSelection());
                        } else {
                            // pick a random appearance
                            stats[index]->appearance = RNG.uniform(0, NUMAPPEARANCES - 1);
                        }
                        if (appearances) {
                            appearances->setSelection(stats[index]->appearance);
                            appearances->scrollToSelection();
                        }
					}
					else {
						stats[index]->appearance = 0;
					}
					if (isCharacterValidFromDLC(*stats[index], client_classes[index]) != VALID_OK_CHARACTER) {
						// perhaps the class is not valid for this race.
						// if so, change the class to the default (Barbarian)
						client_classes[index] = 0;
					}
				}
				break;
			}
		}
		for (int c = 0; c < num_races; ++c) {
			// clear other buttons
			auto race = races[c];
			auto other_button = frame->findButton(race);
			if (other_button != &button) {
				other_button->setPressed(false);
			}
		}
		auto disable_abilities = frame->findButton("disable_abilities");
		if (disable_abilities) {
			disable_abilities->setPressed(false);
		}
		stats[index]->clearStats();
		initClass(index);
        
        if (!override_dlc) {
            sendPlayerOverNet();
            saveLastCharacter(index, multiplayer);
            if (success) {
                if (inputs.hasController(index)) {
                    createCharacterCard(index);
                    auto lobby = main_menu_frame->findFrame("lobby"); assert(lobby);
                    auto card = lobby->findFrame((std::string("card") + std::to_string(index)).c_str()); assert(card);
                    auto button = card->findButton("race"); assert(button);
                    button->select();
                }
            }
        }

		auto card = static_cast<Frame*>(frame->getParent());
		if (card) {
		    RaceDescriptions::update_details_text(*card);
		}
	}

	static void male_button_fn(Button& button, int index) {
		button.setColor(makeColor(255, 255, 255, 255));
		button.setHighlightColor(makeColor(255, 255, 255, 255));
		auto bottom = static_cast<Frame*>(button.getParent()); assert(bottom);
		auto card = static_cast<Frame*>(bottom->getParent()); assert(card);
		auto female = bottom->findButton("female");
		stats[index]->sex = MALE;
		if (female) {
			female->setPressed(stats[index]->sex == FEMALE);
			female->setColor(stats[index]->sex == FEMALE ? makeColorRGB(255, 255, 255) : makeColorRGB(127, 127, 127));
			female->setHighlightColor(stats[index]->sex == FEMALE ? makeColorRGB(255, 255, 255) : makeColorRGB(127, 127, 127));
		}
		if (stats[index]->playerRace == RACE_SUCCUBUS) {
		    auto subframe = card->findFrame("subframe");
		    auto succubus = subframe ? subframe->findButton("Succubus") : nullptr;
		    if (succubus) {
			    succubus->setPressed(false);
		    }
		    if (enabledDLCPack2) {
			    stats[index]->playerRace = RACE_INCUBUS;
			    auto race = card->findButton("race");
			    if (race) {
				    race->setText("Incubus");
			    }
			    auto incubus = subframe ? subframe->findButton("Incubus") : nullptr;
			    if (incubus) {
				    incubus->setPressed(true);
			    }
			    if (client_classes[index] == CLASS_MESMER && stats[index]->appearance == 0) {
				    if (isCharacterValidFromDLC(*stats[index], client_classes[index]) != VALID_OK_CHARACTER) {
					    client_classes[index] = CLASS_PUNISHER;
					    auto class_button = card->findButton("class");
					    if (class_button) {
						    class_button->setIcon("*images/ui/Main Menus/Play/PlayerCreation/ClassSelection/ClassSelect_Icon_Punisher_00.png");
					    }
				    }
			    }
		    } else {
			    stats[index]->playerRace = RACE_HUMAN;
			    auto race = card->findButton("race");
			    if (race) {
				    race->setText("Human");
			    }
			    auto human = subframe ? subframe->findButton("Human") : nullptr;
			    if (human) {
				    human->setPressed(true);
			    }
		    }
		}
		stats[index]->clearStats();
		initClass(index);
		sendPlayerOverNet();
		saveLastCharacter(index, multiplayer);
		RaceDescriptions::update_details_text(*card);
	}

	static void female_button_fn(Button& button, int index) {
		button.setColor(makeColor(255, 255, 255, 255));
		button.setHighlightColor(makeColor(255, 255, 255, 255));
		auto bottom = static_cast<Frame*>(button.getParent()); assert(bottom);
		auto card = static_cast<Frame*>(bottom->getParent()); assert(card);
		auto male = bottom->findButton("male");
		stats[index]->sex = FEMALE;
		if (male) {
			male->setPressed(stats[index]->sex == MALE);
			male->setColor(stats[index]->sex == MALE ? makeColorRGB(255, 255, 255) : makeColorRGB(127, 127, 127));
			male->setHighlightColor(stats[index]->sex == MALE ? makeColorRGB(255, 255, 255) : makeColorRGB(127, 127, 127));
		}
		if (stats[index]->playerRace == RACE_INCUBUS) {
		    auto subframe = card->findFrame("subframe");
			auto incubus = subframe ? subframe->findButton("Incubus") : nullptr;
			if (incubus) {
				incubus->setPressed(false);
			}
			if (enabledDLCPack1) {
				stats[index]->playerRace = RACE_SUCCUBUS;
				auto race = card->findButton("race");
				if (race) {
					race->setText("Succubus");
				}
				auto succubus = subframe ? subframe->findButton("Succubus") : nullptr;
				if (succubus) {
					succubus->setPressed(true);
				}
				if (client_classes[index] == CLASS_PUNISHER && stats[index]->appearance == 0) {
					if (isCharacterValidFromDLC(*stats[index], client_classes[index]) != VALID_OK_CHARACTER) {
						client_classes[index] = CLASS_MESMER;
						auto class_button = card->findButton("class");
						if (class_button) {
							class_button->setIcon("*images/ui/Main Menus/Play/PlayerCreation/ClassSelection/ClassSelect_Icon_Mesmer_00.png");
						}
					}
				}
			} else {
				stats[index]->playerRace = RACE_HUMAN;
				auto race = card->findButton("race");
				if (race) {
					race->setText("Human");
				}
				auto human = subframe ? subframe->findButton("Human") : nullptr;
				if (human) {
					human->setPressed(true);
				}
			}
		}
		stats[index]->clearStats();
		initClass(index);
		sendPlayerOverNet();
		saveLastCharacter(index, multiplayer);
		RaceDescriptions::update_details_text(*card);
	}

	static Frame* initCharacterCard(int index, int height) {
		auto lobby = main_menu_frame->findFrame("lobby");
        if (!lobby) {
            return nullptr;
        }

		auto card = lobby->findFrame((std::string("card") + std::to_string(index)).c_str());
		if (card) {
			card->removeSelf();
		}
        
        const int pos = (Frame::virtualScreenX / 8) * (index * 2 + 1);
		card = lobby->addFrame((std::string("card") + std::to_string(index)).c_str());
		card->setSize(SDL_Rect{pos - 324 / 2, Frame::virtualScreenY - height, 324, height});
		card->setActualSize(SDL_Rect{0, 0, card->getSize().w, card->getSize().h});
		card->setColor(0);
		card->setBorder(0);
		card->setOwner(index);
		card->setClickable(true);
		card->setHideSelectors(true);
		card->setHideGlyphs(true);

		card->setTickCallback([](Widget& widget){
		    const int player = widget.getOwner();
			if (multiplayer == SINGLE) {
				if (inputs.getPlayerIDAllowedKeyboard() != player && !inputs.hasController(player)) {
					createStartButton(player);
				}
			}
		    });

		return card;
	}

	static void characterCardGameFlagsMenu(int index) {
		bool local = currentLobbyType == LobbyType::LobbyLocal;

		auto card = initCharacterCard(index, 664);
        if (!card) {
            return;
        }

		if (multiplayer == CLIENT) {
			allSettings.classic_mode_enabled = lobbyWindowSvFlags & SV_FLAG_CLASSIC;
			allSettings.hardcore_mode_enabled = lobbyWindowSvFlags & SV_FLAG_HARDCORE;
			allSettings.friendly_fire_enabled = lobbyWindowSvFlags & SV_FLAG_FRIENDLYFIRE;
			allSettings.keep_inventory_enabled = lobbyWindowSvFlags & SV_FLAG_KEEPINVENTORY;
			allSettings.hunger_enabled = lobbyWindowSvFlags & SV_FLAG_HUNGER;
			allSettings.minotaur_enabled = lobbyWindowSvFlags & SV_FLAG_MINOTAURS;
			allSettings.random_traps_enabled = lobbyWindowSvFlags & SV_FLAG_TRAPS;
			allSettings.extra_life_enabled = lobbyWindowSvFlags & SV_FLAG_LIFESAVING;
			allSettings.cheats_enabled = lobbyWindowSvFlags & SV_FLAG_CHEATS;
		}

		static void (*back_fn)(int) = [](int index){
			characterCardLobbySettingsMenu(index);
			if (multiplayer != CLIENT) {
			    svFlags = allSettings.classic_mode_enabled ? svFlags | SV_FLAG_CLASSIC : svFlags & ~(SV_FLAG_CLASSIC);
			    svFlags = allSettings.hardcore_mode_enabled ? svFlags | SV_FLAG_HARDCORE : svFlags & ~(SV_FLAG_HARDCORE);
			    svFlags = allSettings.friendly_fire_enabled ? svFlags | SV_FLAG_FRIENDLYFIRE : svFlags & ~(SV_FLAG_FRIENDLYFIRE);
			    svFlags = allSettings.keep_inventory_enabled ? svFlags | SV_FLAG_KEEPINVENTORY : svFlags & ~(SV_FLAG_KEEPINVENTORY);
			    svFlags = allSettings.hunger_enabled ? svFlags | SV_FLAG_HUNGER : svFlags & ~(SV_FLAG_HUNGER);
			    svFlags = allSettings.minotaur_enabled ? svFlags | SV_FLAG_MINOTAURS : svFlags & ~(SV_FLAG_MINOTAURS);
			    svFlags = allSettings.random_traps_enabled ? svFlags | SV_FLAG_TRAPS : svFlags & ~(SV_FLAG_TRAPS);
			    svFlags = allSettings.extra_life_enabled ? svFlags | SV_FLAG_LIFESAVING : svFlags & ~(SV_FLAG_LIFESAVING);
			    svFlags = allSettings.cheats_enabled ? svFlags | SV_FLAG_CHEATS : svFlags & ~(SV_FLAG_CHEATS);
			    sendSvFlagsOverNet();
			}
			auto lobby = main_menu_frame->findFrame("lobby"); assert(lobby);
			auto card = lobby->findFrame((std::string("card") + std::to_string(index)).c_str()); assert(card);
			auto button = card->findButton("custom_difficulty"); assert(button);
			button->select();
		};

		auto back = createBackWidget(card,[](Button& button){soundCancel(); back_fn(button.getOwner());});
        if (multiplayer == CLIENT) {
            back->setTickCallback([](Widget& widget){
				if (!main_menu_frame) {
					return;
				}
                if (!main_menu_frame->findSelectedWidget(widget.getOwner())) {
                    widget.select(); // rescue cursor
                }
                });
        }

		auto backdrop = card->addImage(
			card->getActualSize(),
			0xffffffff,
#ifdef NINTENDO
			"*images/ui/Main Menus/Play/PlayerCreation/LobbySettings/GameSettings/CustomDifficulty_Window_02.png",
#else
			"*images/ui/Main Menus/Play/PlayerCreation/LobbySettings/GameSettings/CustomDifficulty_Window_01.png",
#endif
			"backdrop"
		);

		auto header = card->addField("header", 64);
		header->setSize(SDL_Rect{30, 8, 264, 50});
		header->setFont(smallfont_outline);
		header->setText("CUSTOM DIFFICULTY");
		header->setJustify(Field::justify_t::CENTER);

		const char* game_settings_text[] = {
			"Disable Hunger",
			"Disable Random\nMinotaurs",
			"Enable Life Saving\nAmulet",
			"Keep Items on Death",
			"Disable Random Traps",
			"Disable Friendly Fire",
			"Enable Classic\nEndings",
			"Enable Hardcore\nDifficulty",
#ifndef NINTENDO
			"Enable Cheats",
#endif
		};

		int num_settings = sizeof(game_settings_text) / sizeof(game_settings_text[0]);

		for (int c = 0; c < num_settings; ++c) {
			auto label = card->addField((std::string("label") + std::to_string(c)).c_str(), 128);
			label->setSize(SDL_Rect{48, 60 + 50 * c, 194, 64});
			label->setFont(smallfont_outline);
			label->setText(game_settings_text[c]);
			label->setColor(makeColor(166, 123, 81, 255));
			label->setHJustify(Field::justify_t::LEFT);
			label->setVJustify(Field::justify_t::CENTER);

			auto setting = card->addButton((std::string("setting") + std::to_string(c)).c_str());
			setting->setIcon("*images/ui/Main Menus/Play/PlayerCreation/LobbySettings/GameSettings/Fill_Checked_00.png");
			setting->setStyle(Button::style_t::STYLE_CHECKBOX);
			setting->setSize(SDL_Rect{238, 66 + 50 * c, 44, 44});
			setting->setHighlightColor(0);
			setting->setBorderColor(0);
			setting->setBorder(0);
			setting->setColor(0);
			setting->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
			setting->addWidgetAction("MenuStart", "confirm");
			setting->addWidgetAction("MenuPageRightAlt", "chat");
			setting->addWidgetAction("MenuPageLeftAlt", "privacy");
			setting->setWidgetBack("back_button");
			if (c > 0) {
				setting->setWidgetUp((std::string("setting") + std::to_string(c - 1)).c_str());
			}
			if (c < num_settings - 1) {
				setting->setWidgetDown((std::string("setting") + std::to_string(c + 1)).c_str());
			} else {
				setting->setWidgetDown("confirm");
			}
			if (c == 0) {
				setting->select();
			}
            
            setting->setUserData((void*)(intptr_t)c);
			setting->setDisabled(index != 0);
			switch (c) {
			case 0:
				setting->setPressed(!allSettings.hunger_enabled);
				setting->setCallback([](Button& button){soundCheckmark(); allSettings.hunger_enabled = !button.isPressed();});
                if (multiplayer != CLIENT) {
                    setting->setTickCallback([](Widget& widget) {
						if (!main_menu_frame) {
							return;
						}
                        if (!main_menu_frame->findSelectedWidget(widget.getOwner())) {
                            widget.select(); // rescue cursor
                        }
                    });
                }
				break;
			case 1:
				setting->setPressed(!allSettings.minotaur_enabled);
				setting->setCallback([](Button& button){soundCheckmark(); allSettings.minotaur_enabled = !button.isPressed();});
				break;
			case 2:
				setting->setPressed(allSettings.extra_life_enabled);
				setting->setCallback([](Button& button){soundCheckmark(); allSettings.extra_life_enabled = button.isPressed();});
				break;
			case 3:
				setting->setPressed(allSettings.keep_inventory_enabled);
				setting->setCallback([](Button& button){soundCheckmark(); allSettings.keep_inventory_enabled = button.isPressed();});
				break;
			case 4:
				setting->setPressed(!allSettings.random_traps_enabled);
				setting->setCallback([](Button& button){soundCheckmark(); allSettings.random_traps_enabled = !button.isPressed();});
				break;
			case 5:
				setting->setPressed(!allSettings.friendly_fire_enabled);
				setting->setCallback([](Button& button){soundCheckmark(); allSettings.friendly_fire_enabled = !button.isPressed();});
				break;
			case 6:
				setting->setPressed(allSettings.classic_mode_enabled);
				setting->setCallback([](Button& button){soundCheckmark(); allSettings.classic_mode_enabled = button.isPressed();});
				break;
			case 7:
				setting->setPressed(allSettings.hardcore_mode_enabled);
				setting->setCallback([](Button& button){soundCheckmark(); allSettings.hardcore_mode_enabled = button.isPressed();});
				break;
			case 8:
				setting->setPressed(allSettings.cheats_enabled);
				setting->setCallback([](Button& button){soundCheckmark(); allSettings.cheats_enabled = button.isPressed();});
				break;
			}
		}

		auto achievements = card->addField("achievements", 256);
		achievements->setSize(SDL_Rect{54, 526, 214, 50});
		achievements->setFont(smallfont_no_outline);
		achievements->setJustify(Field::justify_t::CENTER);
		achievements->setTickCallback([](Widget& widget){
			Field* achievements = static_cast<Field*>(&widget);
            if (multiplayer != CLIENT) {
                if (allSettings.cheats_enabled ||
                    allSettings.extra_life_enabled ||
                    gamemods_disableSteamAchievements) {
                    achievements->setColor(makeColor(180, 37, 37, 255));
                    achievements->setText("ACHIEVEMENTS DISABLED");
                } else {
                    achievements->setColor(makeColor(37, 90, 255, 255));
                    achievements->setText("ACHIEVEMENTS ENABLED");
                }
            } else {
                if ((lobbyWindowSvFlags & SV_FLAG_CHEATS) ||
                    (lobbyWindowSvFlags & SV_FLAG_LIFESAVING) ||
                    gamemods_disableSteamAchievements) {
                    achievements->setColor(makeColor(180, 37, 37, 255));
                    achievements->setText("ACHIEVEMENTS DISABLED");
                } else {
                    achievements->setColor(makeColor(37, 90, 255, 255));
                    achievements->setText("ACHIEVEMENTS ENABLED");
                }
                Frame* card = static_cast<Frame*>(widget.getParent());
                for (auto button : card->getButtons()) {
                    auto i = reinterpret_cast<intptr_t>(button->getUserData());
                    switch (i) {
                    case 0:
                        button->setPressed(!(lobbyWindowSvFlags & SV_FLAG_HUNGER));
                        break;
                    case 1:
                        button->setPressed(!(lobbyWindowSvFlags & SV_FLAG_MINOTAURS));
                        break;
                    case 2:
                        button->setPressed((lobbyWindowSvFlags & SV_FLAG_LIFESAVING));
                        break;
                    case 3:
                        button->setPressed((lobbyWindowSvFlags & SV_FLAG_KEEPINVENTORY));
                        break;
                    case 4:
                        button->setPressed(!(lobbyWindowSvFlags & SV_FLAG_TRAPS));
                        break;
                    case 5:
                        button->setPressed(!(lobbyWindowSvFlags & SV_FLAG_FRIENDLYFIRE));
                        break;
                    case 6:
                        button->setPressed((lobbyWindowSvFlags & SV_FLAG_CLASSIC));
                        break;
                    case 7:
                        button->setPressed((lobbyWindowSvFlags & SV_FLAG_HARDCORE));
                        break;
                    case 8:
                        button->setPressed((lobbyWindowSvFlags & SV_FLAG_CHEATS));
                        break;
                    }
                }
            }
			});
		(*achievements->getTickCallback())(*achievements);

		/*auto confirm = card->addButton("confirm");
		confirm->setFont(bigfont_outline);
		confirm->setText("Confirm");
		confirm->setColor(makeColor(255, 255, 255, 255));
		confirm->setHighlightColor(makeColor(255, 255, 255, 255));
		confirm->setBackground("*images/ui/Main Menus/Play/PlayerCreation/LobbySettings/GameSettings/CustomDiff_ButtonConfirm_00.png");
		confirm->setSize(SDL_Rect{62, 606, 202, 52});
		confirm->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
		confirm->addWidgetAction("MenuStart", "confirm");
		confirm->setWidgetBack("back_button");
		confirm->setWidgetUp((std::string("setting") + std::to_string(num_settings - 1)).c_str());
		confirm->setCallback([](Button& button){soundActivate(); back_fn(button.getOwner());});*/
	}

	static void characterCardLobbySettingsMenu(int index) {
		const bool local = currentLobbyType == LobbyType::LobbyLocal || currentLobbyType == LobbyType::LobbyJoined;
		const bool online = currentLobbyType == LobbyType::LobbyOnline;

		auto card = initCharacterCard(index, 424);
        if (!card) {
            return;
        }
        
		const std::string name = std::string("card") + std::to_string(index);

		static void (*back_fn)(int) = [](int index){
			createCharacterCard(index);
			auto lobby = main_menu_frame->findFrame("lobby"); assert(lobby);
			auto card = lobby->findFrame((std::string("card") + std::to_string(index)).c_str()); assert(card);
			auto button = card->findButton("game_settings"); assert(button);
			button->select();
		};

		(void)createBackWidget(card,[](Button& button){soundCancel(); back_fn(button.getOwner());});

		auto backdrop = card->addImage(
			card->getActualSize(),
			0xffffffff,
			"*images/ui/Main Menus/Play/PlayerCreation/LobbySettings/UI_LobbySettings_Window00.png",
			"backdrop"
		);

		auto header = card->addField("header", 64);
		header->setSize(SDL_Rect{30, 8, 264, 50});
		header->setFont(smallfont_outline);
		header->setText("LOBBY SETTINGS");
		header->setJustify(Field::justify_t::CENTER);

		auto custom_difficulty = card->addButton("custom_difficulty");
		custom_difficulty->setColor(makeColor(255, 255, 255, 255));
		custom_difficulty->setHighlightColor(makeColor(255, 255, 255, 255));
		custom_difficulty->setSize(SDL_Rect{102, 68, 120, 48});
		custom_difficulty->setBackground("*images/ui/Main Menus/Play/PlayerCreation/LobbySettings/UI_LobbySettings_Button_Customize00A.png");
		custom_difficulty->setBackgroundHighlighted("*images/ui/Main Menus/Play/PlayerCreation/LobbySettings/UI_LobbySettings_Button_CustomizeHigh00A.png");
		custom_difficulty->setBackgroundActivated("*images/ui/Main Menus/Play/PlayerCreation/LobbySettings/UI_LobbySettings_Button_CustomizePress00A.png");
		custom_difficulty->setFont(smallfont_outline);
		custom_difficulty->setText("Game Flags");
		custom_difficulty->setWidgetSearchParent(name.c_str());
		custom_difficulty->addWidgetAction("MenuStart", "confirm");
		custom_difficulty->addWidgetAction("MenuPageRightAlt", "chat");
		custom_difficulty->addWidgetAction("MenuPageLeftAlt", "privacy");
		custom_difficulty->setWidgetBack("back_button");
		custom_difficulty->setWidgetUp("hard");
		custom_difficulty->setWidgetDown(online ? "invite" : "player_count_2");
		custom_difficulty->setWidgetRight("custom");
		custom_difficulty->setCallback([](Button& button){soundActivate(); characterCardGameFlagsMenu(button.getOwner());});
		custom_difficulty->setTickCallback([](Widget& widget){
			// rescue player selection
			if (!main_menu_frame) {
				return;
			}
			auto selected_widget = main_menu_frame->findSelectedWidget(widget.getOwner());
			if (!selected_widget) {
				widget.select();
			}
			});
		custom_difficulty->select();

		auto invite_label = card->addField("invite_label", 64);
#ifdef NINTENDO
		invite_label->setSize(SDL_Rect{ 82, 158, 122, 26 });
#else
		invite_label->setSize(SDL_Rect{ 82, 146, 122, 26 });
#endif
		invite_label->setFont(smallfont_outline);
		invite_label->setText("Invite Only");
		invite_label->setJustify(Field::justify_t::CENTER);
		if (!online) {
			invite_label->setColor(makeColor(70, 62, 59, 255));

			auto invite = card->addImage(
				SDL_Rect{204, 146, 26, 26},
				0xffffffff,
				"*images/ui/Main Menus/sublist_item-locked.png",
				"invite"
			);
		} else {
			invite_label->setColor(makeColor(166, 123, 81, 255));

			auto invite = card->addButton("invite");
#ifdef NINTENDO
			invite->setSize(SDL_Rect{ 202, 156, 30, 30 });
#else
			invite->setSize(SDL_Rect{ 202, 144, 30, 30 });
#endif
			invite->setBackground("*images/ui/Main Menus/sublist_item-unpicked.png");
			invite->setBackgroundHighlighted("*images/ui/Main Menus/sublist_item-unpickedHigh.png");
			invite->setBackgroundActivated("*images/ui/Main Menus/sublist_item-unpickedPress.png");
			invite->setIcon("*images/ui/Main Menus/sublist_item-picked.png");
			invite->setStyle(Button::style_t::STYLE_RADIO);
			invite->setBorder(0);
			invite->setBorderColor(0);
			invite->setColor(0xffffffff);
			invite->setHighlightColor(0xffffffff);
			invite->setWidgetSearchParent(name.c_str());
			invite->addWidgetAction("MenuStart", "confirm");
			invite->addWidgetAction("MenuPageRightAlt", "chat");
			invite->addWidgetAction("MenuPageLeftAlt", "privacy");
			invite->setWidgetBack("back_button");
			invite->setWidgetUp("custom_difficulty");
#ifdef NINTENDO
			invite->setWidgetDown("open");
#else
			invite->setWidgetDown("friends");
#endif
            if (LobbyHandler.getHostingType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY) {
#ifdef USE_EOS
                if (EOS.currentPermissionLevel == EOS_ELobbyPermissionLevel::EOS_LPL_JOINVIAPRESENCE) {
                    invite->setPressed(true);
                }
#endif // USE_EOS
            }
            else if (LobbyHandler.getHostingType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM) {
#ifdef STEAMWORKS
                if (::currentLobbyType == k_ELobbyTypeInvisible) {
                    invite->setPressed(true);
                }
#endif // STEAMWORKS
            }
			if (index != 0) {
				invite->setCallback([](Button&){soundError();});
			} else {
			    invite->setCallback([](Button& button){
			        soundActivate();
			        auto parent = static_cast<Frame*>(button.getParent());
#ifndef NINTENDO
					auto friends = parent->findButton("friends"); assert(friends);
			        friends->setPressed(false);
#endif
					auto open = parent->findButton("open"); assert(open);
			        open->setPressed(false);

                    if (LobbyHandler.getHostingType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY) {
#ifdef USE_EOS
                        EOS.currentPermissionLevel = EOS_ELobbyPermissionLevel::EOS_LPL_JOINVIAPRESENCE;
                        EOS.bFriendsOnly = false;
#endif // USE_EOS
                    }
                    else if (LobbyHandler.getHostingType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM) {
#ifdef STEAMWORKS
                        ::currentLobbyType = k_ELobbyTypeInvisible;
                        auto lobby = static_cast<CSteamID*>(::currentLobby);
                        SteamMatchmaking()->SetLobbyType(*lobby, ::currentLobbyType);
                        SteamMatchmaking()->SetLobbyData(*lobby, "friends_only", "false");
						SteamMatchmaking()->SetLobbyData(*lobby, "invite_only", "true");
#endif // STEAMWORKS
                    }
			        });
			}
		}

#ifndef NINTENDO
		auto friends_label = card->addField("friends_label", 64);
		friends_label->setSize(SDL_Rect{82, 178, 122, 26});
		friends_label->setFont(smallfont_outline);
		friends_label->setText("Friends Only");
		friends_label->setJustify(Field::justify_t::CENTER);
		if (!online) {
			friends_label->setColor(makeColor(70, 62, 59, 255));

			auto friends = card->addImage(
				SDL_Rect{204, 178, 26, 26},
				0xffffffff,
				"*images/ui/Main Menus/sublist_item-locked.png",
				"friends"
			);
		} else {
			friends_label->setColor(makeColor(166, 123, 81, 255));

			auto friends = card->addButton("friends");
			friends->setSize(SDL_Rect{202, 176, 30, 30});
			friends->setBackground("*images/ui/Main Menus/sublist_item-unpicked.png");
			friends->setBackgroundHighlighted("*images/ui/Main Menus/sublist_item-unpickedHigh.png");
			friends->setBackgroundActivated("*images/ui/Main Menus/sublist_item-unpickedPress.png");
			friends->setIcon("*images/ui/Main Menus/sublist_item-picked.png");
			friends->setStyle(Button::style_t::STYLE_RADIO);
			friends->setBorder(0);
			friends->setBorderColor(0);
			friends->setColor(0xffffffff);
			friends->setHighlightColor(0xffffffff);
			friends->setWidgetSearchParent(name.c_str());
			friends->addWidgetAction("MenuStart", "confirm");
			friends->addWidgetAction("MenuPageRightAlt", "chat");
			friends->addWidgetAction("MenuPageLeftAlt", "privacy");
			friends->setWidgetBack("back_button");
			friends->setWidgetUp("invite");
			friends->setWidgetDown("open");
            if (LobbyHandler.getHostingType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY) {
#ifdef USE_EOS
                if (EOS.currentPermissionLevel == EOS_ELobbyPermissionLevel::EOS_LPL_PUBLICADVERTISED) {
                    if (EOS.bFriendsOnly) {
                        friends->setPressed(true);
                    }
                }
#endif // USE_EOS
            }
            else if (LobbyHandler.getHostingType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM) {
#ifdef STEAMWORKS
                if (::currentLobbyType == k_ELobbyTypePublic) {
                    auto lobby = static_cast<CSteamID*>(::currentLobby);
                    const char* friends_only = SteamMatchmaking()->GetLobbyData(*lobby, "friends_only");
                    if (friends_only && stringCmp(friends_only, "true", 4, 4) == 0) {
                        friends->setPressed(true);
                    }
                }
#endif // STEAMWORKS
            }
			if (index != 0) {
				friends->setCallback([](Button&){soundError();});
			} else {
			    friends->setCallback([](Button& button){
			        soundActivate();
			        auto parent = static_cast<Frame*>(button.getParent());
			        auto invite = parent->findButton("invite"); assert(invite);
			        auto open = parent->findButton("open"); assert(open);
			        invite->setPressed(false);
			        open->setPressed(false);

                    if (LobbyHandler.getHostingType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY) {
#ifdef USE_EOS
                        EOS.currentPermissionLevel = EOS_ELobbyPermissionLevel::EOS_LPL_PUBLICADVERTISED;
                        EOS.bFriendsOnly = true;
#endif // USE_EOS
                    }
                    else if (LobbyHandler.getHostingType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM) {
#ifdef STEAMWORKS
                        ::currentLobbyType = k_ELobbyTypePublic;
                        auto lobby = static_cast<CSteamID*>(::currentLobby);
                        SteamMatchmaking()->SetLobbyType(*lobby, ::currentLobbyType);
                        SteamMatchmaking()->SetLobbyData(*lobby, "friends_only", "true");
						SteamMatchmaking()->SetLobbyData(*lobby, "invite_only", "false");
#endif // STEAMWORKS
                    }
			        });
			}
		}
#endif

		auto open_label = card->addField("open_label", 64);
#ifdef NINTENDO
		open_label->setSize(SDL_Rect{ 82, 198, 122, 26 });
#else
		open_label->setSize(SDL_Rect{ 82, 210, 122, 26 });
#endif
		open_label->setFont(smallfont_outline);
		open_label->setText("Open Lobby");
		open_label->setJustify(Field::justify_t::CENTER);
		if (!online) {
			open_label->setColor(makeColor(70, 62, 59, 255));

			auto open = card->addImage(
				SDL_Rect{204, 210, 26, 26},
				0xffffffff,
				"*images/ui/Main Menus/sublist_item-locked.png",
				"open"
			);
		} else {
			open_label->setColor(makeColor(166, 123, 81, 255));

			auto open = card->addButton("open");
#ifdef NINTENDO
			open->setSize(SDL_Rect{ 202, 196, 30, 30 });
#else
			open->setSize(SDL_Rect{ 202, 208, 30, 30 });
#endif
			open->setBackground("*images/ui/Main Menus/sublist_item-unpicked.png");
			open->setBackgroundHighlighted("*images/ui/Main Menus/sublist_item-unpickedHigh.png");
			open->setBackgroundActivated("*images/ui/Main Menus/sublist_item-unpickedPress.png");
			open->setIcon("*images/ui/Main Menus/sublist_item-picked.png");
			open->setStyle(Button::style_t::STYLE_RADIO);
			open->setBorder(0);
			open->setBorderColor(0);
			open->setColor(0xffffffff);
			open->setHighlightColor(0xffffffff);
			open->setWidgetSearchParent(name.c_str());
			open->addWidgetAction("MenuStart", "confirm");
			open->addWidgetAction("MenuPageRightAlt", "chat");
			open->addWidgetAction("MenuPageLeftAlt", "privacy");
			open->setWidgetBack("back_button");
#ifdef NINTENDO
			open->setWidgetUp("invite");
#else
			open->setWidgetUp("friends");
#endif
			open->setWidgetDown("player_count_2");
            if (LobbyHandler.getHostingType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY) {
#ifdef USE_EOS
                if (EOS.currentPermissionLevel == EOS_ELobbyPermissionLevel::EOS_LPL_PUBLICADVERTISED) {
                    if (!EOS.bFriendsOnly) {
                        open->setPressed(true);
                    }
                }
#endif // USE_EOS
            }
            else if (LobbyHandler.getHostingType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM) {
#ifdef STEAMWORKS
                if (::currentLobbyType == k_ELobbyTypePublic) {
                    auto lobby = static_cast<CSteamID*>(::currentLobby);
                    const char* friends = SteamMatchmaking()->GetLobbyData(*lobby, "friends_only");
                    if (!friends || stringCmp(friends, "true", 4, 4)) {
                        open->setPressed(true);
                    }
                }
#endif // STEAMWORKS
            }
			if (index != 0) {
				open->setCallback([](Button&){soundError();});
			} else {
			    open->setCallback([](Button& button){
			        soundActivate();
			        auto parent = static_cast<Frame*>(button.getParent());
			        auto invite = parent->findButton("invite"); assert(invite);
			        invite->setPressed(false);
#ifndef NINTENDO
					auto friends = parent->findButton("friends"); assert(friends);
			        friends->setPressed(false);
#endif

                    if (LobbyHandler.getHostingType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY) {
#ifdef USE_EOS
                        EOS.currentPermissionLevel = EOS_ELobbyPermissionLevel::EOS_LPL_PUBLICADVERTISED;
                        EOS.bFriendsOnly = false;
#endif // USE_EOS
                    }
                    else if (LobbyHandler.getHostingType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM) {
#ifdef STEAMWORKS
                        ::currentLobbyType = k_ELobbyTypePublic;
                        auto lobby = static_cast<CSteamID*>(::currentLobby);
                        SteamMatchmaking()->SetLobbyType(*lobby, ::currentLobbyType);
                        SteamMatchmaking()->SetLobbyData(*lobby, "friends_only", "false");
						SteamMatchmaking()->SetLobbyData(*lobby, "invite_only", "false");
#endif // STEAMWORKS
                    }
			        });
			}
		}

		auto player_count_label = card->addField("player_count_label", 64);
		player_count_label->setSize(SDL_Rect{40, 266, 116, 40});
		player_count_label->setFont(smallfont_outline);
		player_count_label->setText("Set Player\nCount");
		player_count_label->setJustify(Field::justify_t::CENTER);

        for (int c = 0; c < 3; ++c) {
            const std::string button_name = std::string("player_count_") + std::to_string(c + 2);
		    auto player_count = card->addButton(button_name.c_str());
		    player_count->setSize(SDL_Rect{156 + 44 * c, 266, 40, 40});
		    player_count->setFont(smallfont_outline);
		    player_count->setText(std::to_string(c + 2).c_str());
		    player_count->setBorder(0);
		    player_count->setTextColor(uint32ColorWhite);
	        player_count->setTextHighlightColor(uint32ColorWhite);
		    player_count->setBackground("*#images/ui/Main Menus/Play/PlayerCreation/LobbySettings/UI_LobbySettings_Button_Tiny00A.png");
		    player_count->setBackgroundHighlighted("*#images/ui/Main Menus/Play/PlayerCreation/LobbySettings/UI_LobbySettings_Button_Tiny00B_Highlighted.png");
		    player_count->setBackgroundActivated("*#images/ui/Main Menus/Play/PlayerCreation/LobbySettings/UI_LobbySettings_Button_Tiny00C_Pressed.png");
		    player_count->setColor(uint32ColorWhite);
		    player_count->setWidgetSearchParent(name.c_str());
			player_count->addWidgetAction("MenuStart", "confirm");
			player_count->addWidgetAction("MenuPageRightAlt", "chat");
			player_count->addWidgetAction("MenuPageLeftAlt", "privacy");
			player_count->setWidgetBack("back_button");
			player_count->setWidgetUp(online ? "open" : "custom_difficulty");
		    player_count->setWidgetLeft((std::string("player_count_") + std::to_string(c + 1)).c_str());
		    player_count->setWidgetRight((std::string("player_count_") + std::to_string(c + 3)).c_str());
		    player_count->setWidgetDown((std::string("kick_player_") + std::to_string(c + 2)).c_str());
			if (index != 0) {
				player_count->setCallback([](Button&){soundError();});
			} else {
				switch (c) {
				case 0:
					player_count->setCallback([](Button&){
						if (client_disconnected[2] && client_disconnected[3]) {
							lockSlot(1, false);
							lockSlot(2, true);
							lockSlot(3, true);
							soundActivate();
						} else {
							if (!client_disconnected[2] && !client_disconnected[3]) {
								char prompt[1024];
								snprintf(prompt, sizeof(prompt), "This will kick %s\nand %s!",
									players[2]->getAccountName(), players[3]->getAccountName());
								binaryPrompt(prompt, "Okay", "Go Back",
									[](Button&){ // okay
										lockSlot(1, false);
										lockSlot(2, true);
										lockSlot(3, true);
										soundActivate();
										closeBinary();
										},
									[](Button&){ // go back
										soundCancel();
										closeBinary();
										});
							}
							else if (!client_disconnected[2]) {
								char prompt[1024];
								snprintf(prompt, sizeof(prompt), "This will kick %s.\nAre you sure?", players[2]->getAccountName());
								binaryPrompt(prompt, "Okay", "Go Back",
									[](Button&){ // okay
										lockSlot(1, false);
										lockSlot(2, true);
										lockSlot(3, true);
										soundActivate();
										closeBinary();
										},
									[](Button&){ // go back
										soundCancel();
										closeBinary();
										});
							}
							else if (!client_disconnected[3]) {
								char prompt[1024];
								snprintf(prompt, sizeof(prompt), "This will kick %s.\nAre you sure?", players[3]->getAccountName());
								binaryPrompt(prompt, "Okay", "Go Back",
									[](Button&){ // okay
										lockSlot(1, false);
										lockSlot(2, true);
										lockSlot(3, true);
										soundActivate();
										closeBinary();
										},
									[](Button&){ // go back
										soundCancel();
										closeBinary();
										});
							}
						}
						});
					break;
				case 1:
					player_count->setCallback([](Button&){
						if (client_disconnected[3]) {
							lockSlot(1, false);
							lockSlot(2, false);
							lockSlot(3, true);
							soundActivate();
						} else {
							char prompt[1024];
							snprintf(prompt, sizeof(prompt), "This will kick %s.\nAre you sure?", players[3]->getAccountName());
							binaryPrompt(prompt, "Yes", "No",
								[](Button&){ // yes
									lockSlot(1, false);
									lockSlot(2, false);
									lockSlot(3, true);
									soundActivate();
									closeBinary();
									},
								[](Button&){ // no
									soundCancel();
									closeBinary();
									});
						}
						});
					break;
				case 2:
					player_count->setCallback([](Button&){
						lockSlot(1, false);
						lockSlot(2, false);
						lockSlot(3, false);
						soundActivate();
						});
					break;
				}
			}
        }

		auto kick_player_label = card->addField("kick_player_label", 64);
		kick_player_label->setSize(SDL_Rect{40, 310, 116, 40});
		kick_player_label->setFont(smallfont_outline);
		kick_player_label->setText("Kick Player");
		kick_player_label->setJustify(Field::justify_t::CENTER);

        for (int c = 0; c < 3; ++c) {
            const std::string button_name = std::string("kick_player_") + std::to_string(c + 2);
		    auto kick_player = card->addButton(button_name.c_str());
		    kick_player->setSize(SDL_Rect{156 + 44 * c, 310, 40, 40});
		    kick_player->setFont(smallfont_outline);
		    kick_player->setText(std::to_string(c + 2).c_str());
		    kick_player->setBorder(0);
		    kick_player->setTextColor(uint32ColorWhite);
	        kick_player->setTextHighlightColor(uint32ColorWhite);
		    kick_player->setBackground("*#images/ui/Main Menus/Play/PlayerCreation/LobbySettings/UI_LobbySettings_Button_Tiny00A.png");
		    kick_player->setBackgroundHighlighted("*#images/ui/Main Menus/Play/PlayerCreation/LobbySettings/UI_LobbySettings_Button_Tiny00B_Highlighted.png");
		    kick_player->setBackgroundActivated("*#images/ui/Main Menus/Play/PlayerCreation/LobbySettings/UI_LobbySettings_Button_Tiny00C_Pressed.png");
		    kick_player->setColor(uint32ColorWhite);
		    kick_player->setWidgetSearchParent(name.c_str());
			kick_player->addWidgetAction("MenuStart", "confirm");
			kick_player->addWidgetAction("MenuPageRightAlt", "chat");
			kick_player->addWidgetAction("MenuPageLeftAlt", "privacy");
			kick_player->setWidgetBack("back_button");
		    kick_player->setWidgetLeft((std::string("kick_player_") + std::to_string(c + 1)).c_str());
		    kick_player->setWidgetRight((std::string("kick_player_") + std::to_string(c + 3)).c_str());
		    kick_player->setWidgetUp((std::string("player_count_") + std::to_string(c + 2)).c_str());
			if (index != 0) {
				kick_player->setCallback([](Button&){soundError();});
			} else {
				switch (c) {
				case 0:
					kick_player->setCallback([](Button&){
						if (client_disconnected[1]) {
							soundError();
							return;
						}
						char prompt[1024];
						snprintf(prompt, sizeof(prompt), "Are you sure you want\nto kick %s?", players[1]->getAccountName());
						binaryPrompt(prompt, "Yes", "No",
							[](Button&){ // yes
								soundActivate();
								closeBinary();
								kickPlayer(1);
								},
							[](Button&){ // no
								soundCancel();
								closeBinary();
								});
						});
					break;
				case 1:
					kick_player->setCallback([](Button&){
						if (client_disconnected[2]) {
							soundError();
							return;
						}
						char prompt[1024];
						snprintf(prompt, sizeof(prompt), "Are you sure you want\nto kick %s?", players[2]->getAccountName());
						binaryPrompt(prompt, "Yes", "No",
							[](Button&){ // yes
								soundActivate();
								closeBinary();
								kickPlayer(2);
								},
							[](Button&){ // no
								soundCancel();
								closeBinary();
								});
						});
					break;
				case 2:
					kick_player->setCallback([](Button&){
						if (client_disconnected[3]) {
							soundError();
							return;
						}
						char prompt[1024];
						snprintf(prompt, sizeof(prompt), "Are you sure you want\nto kick %s?", players[3]->getAccountName());
						binaryPrompt(prompt, "Yes", "No",
							[](Button&){ // yes
								soundActivate();
								closeBinary();
								kickPlayer(3);
								},
							[](Button&){ // no
								soundCancel();
								closeBinary();
								});
						});
					break;
				}
			}
        }

        // can't lock slots in local games or saved games
		if (local || loadingsavegame) {
			player_count_label->setColor(makeColor(70, 62, 59, 255));
			for (int c = 0; c < 3; ++c) {
			    auto player_count = card->findButton((std::string("player_count_") + std::to_string(c + 2)).c_str());
			    player_count->setBackground("*#images/ui/Main Menus/Play/PlayerCreation/LobbySettings/UI_LobbySettings_Button_Tiny00D_Gray.png");
			    player_count->setBackgroundHighlighted("*#images/ui/Main Menus/Play/PlayerCreation/LobbySettings/UI_LobbySettings_Button_Tiny00D_Gray.png");
		        player_count->setDisabled(true);
		    }
		} else {
		    player_count_label->setColor(makeColor(166, 123, 81, 255));
		}

        // can't kick players in local games
		if (local) {
			kick_player_label->setColor(makeColor(70, 62, 59, 255));
			for (int c = 0; c < 3; ++c) {
			    auto kick_player = card->findButton((std::string("kick_player_") + std::to_string(c + 2)).c_str());
			    kick_player->setBackground("*#images/ui/Main Menus/Play/PlayerCreation/LobbySettings/UI_LobbySettings_Button_Tiny00D_Gray.png");
			    kick_player->setBackgroundHighlighted("*#images/ui/Main Menus/Play/PlayerCreation/LobbySettings/UI_LobbySettings_Button_Tiny00D_Gray.png");
		        kick_player->setDisabled(true);
		    }
		} else {
		    kick_player_label->setColor(makeColor(166, 123, 81, 255));
		}
	}

	static void characterCardLobbySettingsMenuOLD(int index) {
#if 0
	    /*
	     * NOTE: This is the old lobby settings menu that includes
	     * Difficulty options. It is disabled for now!
	     */

		bool local = currentLobbyType == LobbyType::LobbyLocal;

		auto card = initCharacterCard(index, 580);
        if (!card) {
            return;
        }

		static void (*back_fn)(int) = [](int index){
			createCharacterCard(index);
			auto lobby = main_menu_frame->findFrame("lobby"); assert(lobby);
			auto card = lobby->findFrame((std::string("card") + std::to_string(index)).c_str()); assert(card);
			auto button = card->findButton("game_settings"); assert(button);
			button->select();
		};

		(void)createBackWidget(card,[](Button& button){soundCancel(); back_fn(button.getOwner());});

		auto backdrop = card->addImage(
			card->getActualSize(),
			0xffffffff,
			"*images/ui/Main Menus/Play/PlayerCreation/LobbySettings/GameSettings_Window_01.png",
			"backdrop"
		);

		auto header = card->addField("header", 64);
		header->setSize(SDL_Rect{30, 8, 264, 50});
		header->setFont(smallfont_outline);
		header->setText("GAME SETTINGS");
		header->setJustify(Field::justify_t::CENTER);

		auto difficulty_header = card->addField("difficulty_header", 64);
		difficulty_header->setSize(SDL_Rect{78, 62, 166, 26});
		difficulty_header->setFont(smallfont_outline);
		difficulty_header->setText("DIFFICULTY SETTINGS");
		difficulty_header->setJustify(Field::justify_t::CENTER);

		auto easy_label = card->addField("easy_label", 64);
		easy_label->setSize(SDL_Rect{78, 102, 134, 26});
		easy_label->setFont(smallfont_outline);
		easy_label->setText("Practice");
		easy_label->setJustify(Field::justify_t::CENTER);
		easy_label->setColor(makeColor(83, 166, 98, 255));

		static const char* modes[] = {
			"easy", "normal", "hard", "custom"
		};

		static auto clear_difficulties = [](Button& button){
			Frame* frame = static_cast<Frame*>(button.getParent());
			for (auto mode : modes) {
				if (strcmp(button.getName(), mode) == 0) {
					continue;
				}
				auto other_button = frame->findButton(mode);
				if (other_button) {
				    other_button->setPressed(false);
				}
			}
		};

		auto easy = card->addButton("easy");
		easy->setSize(SDL_Rect{210, 100, 30, 30});
		easy->setIcon("*images/ui/Main Menus/Play/PlayerCreation/LobbySettings/Fill_Round_00.png");
		easy->setStyle(Button::style_t::STYLE_RADIO);
		easy->setBorder(0);
		easy->setColor(0);
		easy->setBorderColor(0);
		easy->setHighlightColor(0);
		easy->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
		easy->addWidgetAction("MenuStart", "confirm");
		easy->setWidgetBack("back_button");
		easy->setWidgetDown("normal");
		easy->setCallback([](Button& button){
			clear_difficulties(button);
			if (multiplayer != CLIENT) {
			    svFlags = SV_FLAG_CLASSIC | SV_FLAG_KEEPINVENTORY | SV_FLAG_LIFESAVING;
			    sendSvFlagsOverNet();
			}
			});
		if (svFlags == (SV_FLAG_CLASSIC | SV_FLAG_KEEPINVENTORY | SV_FLAG_LIFESAVING)) {
			easy->setPressed(true);
			easy->select();
		}

		auto normal_label = card->addField("normal_label", 64);
		normal_label->setSize(SDL_Rect{68, 140, 144, 26});
		normal_label->setFont(smallfont_outline);
		normal_label->setText("Normal");
		normal_label->setVJustify(Field::justify_t::TOP);
		normal_label->setHJustify(Field::justify_t::CENTER);
		normal_label->setColor(makeColor(206, 165, 40, 255));

		auto normal = card->addButton("normal");
		normal->setSize(SDL_Rect{210, 138, 30, 30});
		normal->setIcon("*images/ui/Main Menus/Play/PlayerCreation/LobbySettings/Fill_Round_00.png");
		normal->setStyle(Button::style_t::STYLE_RADIO);
		normal->setBorder(0);
		normal->setColor(0);
		normal->setBorderColor(0);
		normal->setHighlightColor(0);
		normal->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
		normal->addWidgetAction("MenuStart", "confirm");
		normal->setWidgetBack("back_button");
		normal->setWidgetUp("easy");
		normal->setWidgetDown("hard");
		normal->setCallback([](Button& button){
			clear_difficulties(button);
			if (multiplayer != CLIENT) {
			    svFlags = SV_FLAG_FRIENDLYFIRE | SV_FLAG_KEEPINVENTORY | SV_FLAG_HUNGER | SV_FLAG_MINOTAURS | SV_FLAG_TRAPS;
			    sendSvFlagsOverNet();
			}
			});
		if (svFlags == (SV_FLAG_FRIENDLYFIRE | SV_FLAG_KEEPINVENTORY | SV_FLAG_HUNGER | SV_FLAG_MINOTAURS | SV_FLAG_TRAPS)) {
			normal->setPressed(true);
			normal->select();
		}

		// NOTE: on non-english languages, normal text will need to be used.
        // here's the code for that, but how to gate it?
		/*auto hard_label = card->addField("hard_label", 64);
		hard_label->setSize(SDL_Rect{68, 178, 144, 26});
		hard_label->setFont(smallfont_outline);
		hard_label->setText("Nightmare");
		hard_label->setVJustify(Field::justify_t::TOP);
		hard_label->setHJustify(Field::justify_t::CENTER);
		hard_label->setColor(makeColor(124, 15, 10, 255));*/

		auto hard_label = card->addImage(
			SDL_Rect{84, 158, 120, 50},
			0xffffffff,
			"*images/ui/Main Menus/Play/PlayerCreation/LobbySettings/GameSettings_Text_Nightmare_00.png",
			"hard_label"
		);

		auto hard = card->addButton("hard");
		hard->setSize(SDL_Rect{210, 176, 30, 30});
		hard->setIcon("*images/ui/Main Menus/Play/PlayerCreation/LobbySettings/Fill_Round_00.png");
		hard->setStyle(Button::style_t::STYLE_RADIO);
		hard->setBorder(0);
		hard->setColor(0);
		hard->setBorderColor(0);
		hard->setHighlightColor(0);
		hard->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
		hard->addWidgetAction("MenuStart", "confirm");
		hard->setWidgetBack("back_button");
		hard->setWidgetUp("normal");
		hard->setWidgetDown("custom");
		hard->setCallback([](Button& button){
			clear_difficulties(button);
			if (multiplayer != CLIENT) {
			    svFlags = SV_FLAG_HARDCORE | SV_FLAG_FRIENDLYFIRE | SV_FLAG_HUNGER | SV_FLAG_MINOTAURS | SV_FLAG_TRAPS;
			    sendSvFlagsOverNet();
			}
			});
		if (svFlags == (SV_FLAG_HARDCORE | SV_FLAG_FRIENDLYFIRE | SV_FLAG_HUNGER | SV_FLAG_MINOTAURS | SV_FLAG_TRAPS)) {
			hard->setPressed(true);
			hard->select();
		}

		auto custom_difficulty = card->addButton("custom_difficulty");
		custom_difficulty->setColor(makeColor(255, 255, 255, 255));
		custom_difficulty->setHighlightColor(makeColor(255, 255, 255, 255));
		custom_difficulty->setSize(SDL_Rect{84, 210, 114, 40});
		custom_difficulty->setBackground("*images/ui/Main Menus/Play/PlayerCreation/LobbySettings/UI_GameSettings_Button_Custom_01.png");
		custom_difficulty->setFont(smallfont_outline);
		custom_difficulty->setText("Custom");
		custom_difficulty->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
		custom_difficulty->addWidgetAction("MenuStart", "confirm");
		custom_difficulty->setWidgetBack("back_button");
		custom_difficulty->setWidgetUp("hard");
		custom_difficulty->setWidgetDown("invite");
		custom_difficulty->setWidgetRight("custom");
		custom_difficulty->setCallback([](Button& button){soundActivate(); characterCardGameFlagsMenu(button.getOwner());});

		auto custom = card->addButton("custom");
		custom->setSize(SDL_Rect{210, 216, 30, 30});
		custom->setIcon("*images/ui/Main Menus/Play/PlayerCreation/LobbySettings/Fill_Round_00.png");
		custom->setStyle(Button::style_t::STYLE_RADIO);
		custom->setBorder(0);
		custom->setColor(0);
		custom->setBorderColor(0);
		custom->setHighlightColor(0);
		custom->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
		custom->addWidgetAction("MenuStart", "confirm");
		custom->setWidgetBack("back_button");
		custom->setWidgetUp("hard");
		if (local) {
			custom->setWidgetDown("confirm");
		} else {
			custom->setWidgetDown("invite");
		}
		custom->setWidgetLeft("custom_difficulty");
		custom->setCallback([](Button& button){
			clear_difficulties(button);
			if (multiplayer != CLIENT) {
			    svFlags = allSettings.classic_mode_enabled ? svFlags | SV_FLAG_CLASSIC : svFlags & ~(SV_FLAG_CLASSIC);
			    svFlags = allSettings.hardcore_mode_enabled ? svFlags | SV_FLAG_HARDCORE : svFlags & ~(SV_FLAG_HARDCORE);
			    svFlags = allSettings.friendly_fire_enabled ? svFlags | SV_FLAG_FRIENDLYFIRE : svFlags & ~(SV_FLAG_FRIENDLYFIRE);
			    svFlags = allSettings.keep_inventory_enabled ? svFlags | SV_FLAG_KEEPINVENTORY : svFlags & ~(SV_FLAG_KEEPINVENTORY);
			    svFlags = allSettings.hunger_enabled ? svFlags | SV_FLAG_HUNGER : svFlags & ~(SV_FLAG_HUNGER);
			    svFlags = allSettings.minotaur_enabled ? svFlags | SV_FLAG_MINOTAURS : svFlags & ~(SV_FLAG_MINOTAURS);
			    svFlags = allSettings.random_traps_enabled ? svFlags | SV_FLAG_TRAPS : svFlags & ~(SV_FLAG_TRAPS);
			    svFlags = allSettings.extra_life_enabled ? svFlags | SV_FLAG_LIFESAVING : svFlags & ~(SV_FLAG_LIFESAVING);
			    svFlags = allSettings.cheats_enabled ? svFlags | SV_FLAG_CHEATS : svFlags & ~(SV_FLAG_CHEATS);
			    sendSvFlagsOverNet();
			}
			});
		if (!easy->isSelected() && !normal->isSelected() && !hard->isSelected()) {
			custom->setPressed(true);
			custom->select();
		}

		auto achievements = card->addField("achievements", 256);
		achievements->setSize(SDL_Rect{54, 260, 214, 50});
		achievements->setFont(smallfont_no_outline);
		achievements->setJustify(Field::justify_t::CENTER);
		achievements->setTickCallback([](Widget& widget){
			Field* achievements = static_cast<Field*>(&widget);
			if ((svFlags & SV_FLAG_CHEATS) ||
				(svFlags & SV_FLAG_LIFESAVING) ||
				gamemods_disableSteamAchievements) {
				achievements->setColor(makeColor(180, 37, 37, 255));
				achievements->setText("ACHIEVEMENTS DISABLED");
			} else {
				achievements->setColor(makeColor(37, 90, 255, 255));
				achievements->setText("ACHIEVEMENTS ENABLED");
			}
			});
		(*achievements->getTickCallback())(*achievements);

		auto multiplayer_header = card->addField("multiplayer_header", 64);
		multiplayer_header->setSize(SDL_Rect{70, 328, 182, 34});
		multiplayer_header->setFont(smallfont_outline);
		multiplayer_header->setText("LOBBY SETTINGS");
		multiplayer_header->setJustify(Field::justify_t::CENTER);

		auto invite_label = card->addField("invite_label", 64);
		invite_label->setSize(SDL_Rect{68, 390, 146, 26});
		invite_label->setFont(smallfont_outline);
		invite_label->setText("Invite Only");
		invite_label->setJustify(Field::justify_t::CENTER);
		if (local) {
			invite_label->setColor(makeColor(70, 62, 59, 255));

			auto invite = card->addImage(
				SDL_Rect{214, 390, 26, 26},
				0xffffffff,
				"*images/ui/Main Menus/Play/PlayerCreation/LobbySettings/Blocker_00.png",
				"invite"
			);
		} else {
			invite_label->setColor(makeColor(166, 123, 81, 255));

			auto invite = card->addButton("invite");
			invite->setSize(SDL_Rect{212, 388, 30, 30});
			invite->setIcon("*images/ui/Main Menus/Play/PlayerCreation/LobbySettings/Fill_Round_00.png");
			invite->setStyle(Button::style_t::STYLE_RADIO);
			invite->setBorder(0);
			invite->setColor(0);
			invite->setBorderColor(0);
			invite->setHighlightColor(0);
			invite->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
			invite->addWidgetAction("MenuStart", "confirm");
			invite->setWidgetBack("back_button");
			invite->setWidgetUp("custom");
			invite->setWidgetDown("friends");
			if (index != 0) {
				invite->setCallback([](Button&){soundError();});
			} else {
			}
		}

		auto friends_label = card->addField("friends_label", 64);
		friends_label->setSize(SDL_Rect{68, 428, 146, 26});
		friends_label->setFont(smallfont_outline);
		friends_label->setText("Friends Only");
		friends_label->setJustify(Field::justify_t::CENTER);
		if (local) {
			friends_label->setColor(makeColor(70, 62, 59, 255));

			auto friends = card->addImage(
				SDL_Rect{214, 428, 26, 26},
				0xffffffff,
				"*images/ui/Main Menus/Play/PlayerCreation/LobbySettings/Blocker_00.png",
				"friends"
			);
		} else {
			friends_label->setColor(makeColor(166, 123, 81, 255));

			auto friends = card->addButton("friends");
			friends->setSize(SDL_Rect{212, 426, 30, 30});
			friends->setIcon("*images/ui/Main Menus/Play/PlayerCreation/LobbySettings/Fill_Round_00.png");
			friends->setStyle(Button::style_t::STYLE_RADIO);
			friends->setBorder(0);
			friends->setColor(0);
			friends->setBorderColor(0);
			friends->setHighlightColor(0);
			friends->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
			friends->addWidgetAction("MenuStart", "confirm");
			friends->setWidgetBack("back_button");
			friends->setWidgetUp("invite");
			friends->setWidgetDown("open");
			if (index != 0) {
				friends->setCallback([](Button&){soundError();});
			} else {
			}
		}

		auto open_label = card->addField("open_label", 64);
		open_label->setSize(SDL_Rect{68, 466, 146, 26});
		open_label->setFont(smallfont_outline);
		open_label->setText("Open Lobby");
		open_label->setJustify(Field::justify_t::CENTER);
		if (local) {
			open_label->setColor(makeColor(70, 62, 59, 255));

			auto open = card->addImage(
				SDL_Rect{214, 466, 26, 26},
				0xffffffff,
				"*images/ui/Main Menus/Play/PlayerCreation/LobbySettings/Blocker_00.png",
				"open"
			);
		} else {
			open_label->setColor(makeColor(166, 123, 81, 255));

			auto open = card->addButton("open");
			open->setSize(SDL_Rect{212, 464, 30, 30});
			open->setIcon("*images/ui/Main Menus/Play/PlayerCreation/LobbySettings/Fill_Round_00.png");
			open->setStyle(Button::style_t::STYLE_RADIO);
			open->setBorder(0);
			open->setColor(0);
			open->setBorderColor(0);
			open->setHighlightColor(0);
			open->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
			open->addWidgetAction("MenuStart", "confirm");
			open->setWidgetBack("back_button");
			open->setWidgetUp("friends");
			open->setWidgetDown("confirm");
			if (index != 0) {
				open->setCallback([](Button&){soundError();});
			} else {
			}
		}

		/*auto confirm = card->addButton("confirm");
		confirm->setFont(bigfont_outline);
		confirm->setText("Confirm");
		confirm->setColor(makeColor(255, 255, 255, 255));
		confirm->setHighlightColor(makeColor(255, 255, 255, 255));
		confirm->setBackground("*images/ui/Main Menus/Play/PlayerCreation/LobbySettings/GameSettings_ButtonConfirm_00.png");
		confirm->setSize(SDL_Rect{62, 522, 202, 52});
		confirm->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
		confirm->addWidgetAction("MenuStart", "confirm");
		confirm->setWidgetBack("back_button");
		if (local) {
			confirm->setWidgetUp("custom");
		} else {
			confirm->setWidgetUp("open");
		}
		confirm->setCallback([](Button& button){soundActivate(); back_fn(button.getOwner());});*/
#endif
	}

	static void characterCardRaceMenu(int index, bool details, int selection) {
		auto card = initCharacterCard(index, details ? 664 : 488);
        if (!card) {
            return;
        }

		static int race_selection[MAXPLAYERS];

		static void (*back_fn)(int) = [](int index){
            if (inputs.hasController(index)) {
                client_classes[index] = old_classes[index];
                stats[index]->appearance = old_appearances[index];
                stats[index]->playerRace = old_races[index];
                stats[index]->sex = old_sexes[index];
                stats[index]->clearStats();
                initClass(index);
            }
			createCharacterCard(index);
			auto lobby = main_menu_frame->findFrame("lobby"); assert(lobby);
			auto card = lobby->findFrame((std::string("card") + std::to_string(index)).c_str()); assert(card);
			auto button = card->findButton("race"); assert(button);
			button->select();
		};

		(void)createBackWidget(card,[](Button& button){soundCancel(); back_fn(button.getOwner());});

		auto backdrop = card->addImage(
			card->getActualSize(),
			0xffffffff,
			details ?
			    "*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/RaceSelect_ScrollList_Details.png":
			    "*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/RaceSelect_ScrollList.png",
			"backdrop"
		);

		auto header = card->addField("header", 64);
		header->setSize(SDL_Rect{30, 8, 264, 50});
		header->setFont(smallfont_outline);
		header->setText("RACE SELECTION");
		header->setJustify(Field::justify_t::CENTER);

		if (details) {
		    const auto font = smallfont_no_outline;

		    auto details_title = card->addField("details_title", 1024);
		    details_title->setFont(font);
		    details_title->setSize(SDL_Rect{40, 68, 242, 300 });
		    details_title->setHJustify(Field::justify_t::CENTER);

		    auto details_text = card->addField("details", 1024);
		    details_text->setFont(font);
		    details_text->setSize(SDL_Rect{40, 68, 242, 300});

		    auto details_text_right = card->addField("details_right", 1024);
		    details_text_right->setFont(font);
		    details_text_right->setSize(SDL_Rect{161, 68, 121, 300 });

			RaceDescriptions::update_details_text(*card);
		}

		auto subframe = card->addFrame("subframe");
		subframe->setSize(details ?
		    SDL_Rect{38, 382, 234, 106}:
		    SDL_Rect{38, 68, 234, 208});
		subframe->setActualSize(SDL_Rect{0, 0, 234, 36 * num_races});
		subframe->setBorder(0);
		subframe->setColor(0);
		subframe->setTickCallback([](Widget& widget){
		    auto subframe = static_cast<Frame*>(&widget); assert(subframe);
		    auto card = static_cast<Frame*>(widget.getParent()); assert(card);
		    auto gradient = card->findImage("gradient"); assert(gradient);

            const auto grad_size = gradient->pos.h / 2;
            const auto size = subframe->getSize();
		    const auto asize = subframe->getActualSize();
		    const float fade = ((asize.h - grad_size) - (asize.y + size.h)) / (float)grad_size;
		    const float b_fade = std::min(std::max(0.f, fade), 1.f);
		    gradient->color = makeColor(255, 255, 255, 127 * b_fade);
		    });

		auto slider = card->addSlider("scroll_slider");
		slider->setRailSize(details ? SDL_Rect{278, 376, 12, 118} : SDL_Rect{278, 62, 12, 220});
		slider->setHandleSize(SDL_Rect{0, 0, 20, 28});
		slider->setHandleImage("*images/ui/Sliders/HUD_Magic_Slider_Emerald_01.png");
		slider->setOrientation(Slider::orientation_t::SLIDER_VERTICAL);
		slider->setBorder(14);
		slider->setMinValue(0.f);
		slider->setMaxValue(subframe->getActualSize().h - subframe->getSize().h);
		slider->setCallback([](Slider& slider){
			Frame* frame = static_cast<Frame*>(slider.getParent());
			Frame* subframe = frame->findFrame("subframe"); assert(subframe);
			auto actualSize = subframe->getActualSize();
			actualSize.y = slider.getValue();
			subframe->setActualSize(actualSize);
			});
		slider->setTickCallback([](Widget& widget){
			Slider* slider = static_cast<Slider*>(&widget);
			Frame* frame = static_cast<Frame*>(slider->getParent());
			Frame* subframe = frame->findFrame("subframe"); assert(subframe);
			auto actualSize = subframe->getActualSize();
			slider->setValue(actualSize.y);
			});
		slider->setWidgetSearchParent(card->getName());
		slider->setWidgetLeft(races[0]);
	    slider->setWidgetBack("back_button");
	    slider->setGlyphPosition(Widget::glyph_position_t::CENTERED);
	    slider->setHideSelectors(true);

		auto hover_image = subframe->addImage(
			SDL_Rect{0, 4 + 36 * stats[index]->playerRace, 234, 30},
			0xffffffff,
			"*#images/ui/Main Menus/Play/PlayerCreation/RaceSelection/sublist_item-hover.png",
		    "hover");

		auto gradient = card->addImage(
		    SDL_Rect{38, details ? 446 : 234, 234, 42},
		    0xffffffff,
			"*#images/ui/Main Menus/Play/PlayerCreation/RaceSelection/sublist_gradient.png",
		    "gradient");
		gradient->ontop = true;

        for (int c = 0; c < num_races; ++c) {
		    auto race = subframe->addButton(races[c]);
		    race->setSize(SDL_Rect{0, c * 36 + 2, 30, 30});
		    if (!enabledDLCPack1 && c >= 1 && c <= 4) {
		        race->setBackground("*#images/ui/Main Menus/sublist_item-locked.png");
		    }
		    else if (!enabledDLCPack2 && c >= 5 && c <= 8) {
		        race->setBackground("*#images/ui/Main Menus/sublist_item-locked.png");
		    }
		    else {
		        race->setBackground("*#images/ui/Main Menus/sublist_item-unpicked.png");
		        race->setBackgroundHighlighted("*#images/ui/Main Menus/sublist_item-unpickedHigh.png");
		        race->setBackgroundActivated("*#images/ui/Main Menus/sublist_item-unpickedPress.png");
		    }
		    race->setIcon("*#images/ui/Main Menus/sublist_item-picked.png");
		    race->setStyle(Button::style_t::STYLE_RADIO);
		    race->setBorder(0);
		    race->setColor(0xffffffff);
		    race->setBorderColor(0);
		    race->setHighlightColor(0xffffffff);
		    race->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
            if (c == 0) {
                race->addWidgetAction("MenuRight", "appearance_downarrow");
                race->addWidgetAction("MenuLeft", "appearance_uparrow");
            }
		    race->addWidgetAction("MenuStart", "confirm");
		    race->addWidgetAction("MenuPageRightAlt", "chat");
		    race->addWidgetAction("MenuPageLeftAlt", "privacy");
		    race->setWidgetBack("back_button");
		    if (c < num_races - 1) {
		        race->setWidgetDown(races[c + 1]);
		    }
		    /*else {
		        race->setWidgetDown("disable_abilities");
		    }*/
		    if (c > 0) {
		        race->setWidgetUp(races[c - 1]);
		    }
		    race->setGlyphPosition(Widget::glyph_position_t::CENTERED);
		    race->addWidgetAction("MenuPageLeft", "male");
		    race->addWidgetAction("MenuPageRight", "female");
		    race->addWidgetAction("MenuAlt1", "disable_abilities");
		    race->addWidgetAction("MenuAlt2", "show_race_info");
            race->setCallback([](Button& button){
                soundActivate();
                race_button_fn(button, false);
                });
		    if (stats[index]->playerRace == c) {
			    race->setPressed(true);
		    }
		    if ((stats[index]->playerRace == c && selection == -1) ||
		        (selection >= 0 && selection == c)) {
			    race->select();
			    race->scrollParent();
		    }
		    race->setTickCallback([](Widget& widget){
		        if (widget.isSelected()) {
		            auto button = static_cast<Button*>(&widget); assert(button);
		            auto subframe = static_cast<Frame*>(widget.getParent()); assert(subframe);
		            auto hover = subframe->findImage("hover"); assert(hover);
		            hover->pos.y = button->getSize().y;
		            race_selection[widget.getOwner()] = (hover->pos.y - 2) / 36;
                    
                    const int index = widget.getOwner();
                    if (inputs.hasController(index)) {
                        race_button_fn(*button, true);
                    }
		        }

				// rescue this player's focus
                if (strcmp(widget.getName(), "Human") == 0) {
                    if (!main_menu_frame) {
                        return;
                    }
                    auto selectedWidget = main_menu_frame->findSelectedWidget(widget.getOwner());
                    if (!selectedWidget) {
                        // TODO - last race is always being rescued when cancelling DLC prompt
                        widget.select(); // select this widget
                    }
                }
		        });

		    auto label = subframe->addField((std::string(races[c]) + "_label").c_str(), 64);
		    if (c >= 1 && c <= 4) {
		        label->setColor(color_dlc1);
		    } else if (c >= 5 && c <= 8) {
		        label->setColor(color_dlc2);
		    } else {
		        label->setColor(color_dlc0);
		    }
		    label->setText(races[c]);
		    label->setFont(smallfont_outline);
		    label->setSize(SDL_Rect{32, c * 36, 96, 36});
		    label->setHJustify(Field::justify_t::LEFT);
		    label->setVJustify(Field::justify_t::CENTER);
		}
        
        static const char* appearance_names[] = {
            "Landguard", "Northborn", "Firebrand", "Hardbred",
            "Highwatch", "Gloomforge", "Pyrebloom", "Snakestone",
            "Windclan", "Warblood", "Millbound", "Sunstalk",
            "Claymount", "Stormward", "Tradefell", "Nighthill",
            "Baytower", "Whetsong"
        };

        constexpr int num_appearances = sizeof(appearance_names) / sizeof(appearance_names[0]);

		auto appearances = subframe->addFrame("appearances");
		appearances->setSize(SDL_Rect{102, 0, 122, 36});
		appearances->setActualSize(SDL_Rect{0, 4, 122, 36});
		appearances->setFont("fonts/pixel_maz.ttf#32#2");
		appearances->setBorder(0);
		appearances->setListOffset(SDL_Rect{12, 8, 0, 0});
		appearances->setScrollBarsEnabled(false);
		appearances->setAllowScrollBinds(false);
		appearances->setClickable(true);
		appearances->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
		appearances->addWidgetMovement("MenuListCancel", "appearances");
		appearances->addWidgetMovement("MenuListConfirm", "appearances");
		appearances->addWidgetAction("MenuStart", "confirm");
		appearances->addWidgetAction("MenuPageRightAlt", "chat");
		appearances->addWidgetAction("MenuPageLeftAlt", "privacy");
		appearances->setWidgetBack("back_button");
	    appearances->addWidgetAction("MenuPageLeft", "male");
	    appearances->addWidgetAction("MenuPageRight", "female");
	    appearances->addWidgetAction("MenuAlt1", "disable_abilities");
	    appearances->addWidgetAction("MenuAlt2", "show_race_info");
		appearances->setWidgetLeft(races[0]);
		appearances->setWidgetDown(races[1]);
		appearances->setTickCallback([](Widget& widget){
			auto frame = static_cast<Frame*>(&widget);
			auto parent = static_cast<Frame*>(frame->getParent());
			auto backdrop = frame->findImage("background"); assert(backdrop);
			auto box = frame->findImage("selection_box"); assert(box);
			box->pos.y = frame->getActualSize().y;
			backdrop->pos.y = frame->getActualSize().y + 4;
            auto human = parent->findButton("Human"); assert(human);
            auto appearance_uparrow = parent->findButton("appearance_uparrow"); assert(appearance_uparrow);
            auto appearance_downarrow = parent->findButton("appearance_downarrow"); assert(appearance_downarrow);
			auto controlType = Input::inputs[widget.getOwner()].getPlayerControlType();
            const bool selected = controlType == Input::playerControlType_t::PLAYER_CONTROLLED_BY_KEYBOARD ?
                frame->isActivated() : frame->isActivated() || human->isSelected();
			const bool deselected = controlType == Input::playerControlType_t::PLAYER_CONTROLLED_BY_KEYBOARD ?
				(!frame->isSelected() && !appearance_uparrow->isSelected() && !appearance_downarrow->isSelected()) :
				!frame->isActivated() || !human->isSelected();
			if (selected) {
                box->disabled = false;
				appearance_uparrow->setDisabled(false);
				appearance_uparrow->setInvisible(false);
				appearance_downarrow->setDisabled(false);
				appearance_downarrow->setInvisible(false);
			} else if (deselected) {
                box->disabled = true;
				appearance_uparrow->setDisabled(true);
				appearance_uparrow->setInvisible(true);
				appearance_downarrow->setDisabled(true);
				appearance_downarrow->setInvisible(true);
			}
	        if (widget.isSelected()) {
	            auto hover = parent->findImage("hover"); assert(hover);
	            hover->pos.y = frame->getSize().y + 2;
	            race_selection[widget.getOwner()] = (hover->pos.y - 2) / 36;
	        }
			});

		auto appearance_backdrop = appearances->addImage(
			SDL_Rect{2, 4, 118, 28},
			0xffffffff,
			"*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_TextBack_00.png",
			"background"
		);

		auto appearance_selected = appearances->addImage(
			SDL_Rect{0, 0, 122, 36},
			0xffffffff,
			"*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_Textbox_00.png",
			"selection_box"
		);
		appearance_selected->disabled = true;

		auto appearance_uparrow = subframe->addButton("appearance_uparrow");
		appearance_uparrow->setSize(SDL_Rect{92, 2, 20, 32});
		appearance_uparrow->setBackground("*images/ui/Main Menus/sublist_item-pickleft.png");
		appearance_uparrow->setBackgroundHighlighted("*images/ui/Main Menus/sublist_item-pickleftHigh.png");
		appearance_uparrow->setBackgroundActivated("*images/ui/Main Menus/sublist_item-pickleftPress.png");
		appearance_uparrow->setHighlightColor(makeColor(255, 255, 255, 255));
		appearance_uparrow->setColor(makeColor(255, 255, 255, 255));
		appearance_uparrow->setDisabled(true);
		appearance_uparrow->setInvisible(true);
		appearance_uparrow->setOntop(true);
		appearance_uparrow->setCallback([](Button& button){
			auto card = static_cast<Frame*>(button.getParent());
			auto appearances = card->findFrame("appearances"); assert(appearances);
			int selection = (int)stats[button.getOwner()]->appearance - 1;
            if (selection < 0) {
                selection = num_appearances - 1;
            }
			appearances->setSelection(selection);
			appearances->scrollToSelection();
			appearances->activateSelection();
            if (!inputs.hasController(button.getOwner())) {
                button.select();
            }
			});
	    appearance_uparrow->setTickCallback([](Widget& widget){
	        if (widget.isSelected()) {
	            auto button = static_cast<Button*>(&widget); assert(button);
	            auto subframe = static_cast<Frame*>(widget.getParent()); assert(subframe);
	            auto hover = subframe->findImage("hover"); assert(hover);
	            hover->pos.y = button->getSize().y;
	            race_selection[widget.getOwner()] = (hover->pos.y - 2) / 36;
	        }
	        });
		appearance_uparrow->addWidgetAction("MenuStart", "confirm");
		appearance_uparrow->addWidgetAction("MenuPageRightAlt", "chat");
		appearance_uparrow->addWidgetAction("MenuPageLeftAlt", "privacy");
		appearance_uparrow->setWidgetBack("back_button");
	    appearance_uparrow->addWidgetAction("MenuPageLeft", "male");
	    appearance_uparrow->addWidgetAction("MenuPageRight", "female");
	    appearance_uparrow->addWidgetAction("MenuAlt1", "disable_abilities");
	    appearance_uparrow->addWidgetAction("MenuAlt2", "show_race_info");

		auto appearance_downarrow = subframe->addButton("appearance_downarrow");
		appearance_downarrow->setSize(SDL_Rect{214, 2, 20, 32});
		appearance_downarrow->setBackground("*images/ui/Main Menus/sublist_item-pickright.png");
		appearance_downarrow->setBackgroundHighlighted("*images/ui/Main Menus/sublist_item-pickrightHigh.png");
		appearance_downarrow->setBackgroundActivated("*images/ui/Main Menus/sublist_item-pickrightPress.png");
		appearance_downarrow->setHighlightColor(makeColor(255, 255, 255, 255));
		appearance_downarrow->setColor(makeColor(255, 255, 255, 255));
		appearance_downarrow->setDisabled(true);
		appearance_downarrow->setInvisible(true);
		appearance_downarrow->setOntop(true);
		appearance_downarrow->setCallback([](Button& button){
			auto card = static_cast<Frame*>(button.getParent());
			auto appearances = card->findFrame("appearances"); assert(appearances);
            int selection = (int)stats[button.getOwner()]->appearance + 1;
            if (selection >= num_appearances) {
                selection = 0;
            }
			appearances->setSelection(selection);
			appearances->scrollToSelection();
			appearances->activateSelection();
            if (!inputs.hasController(button.getOwner())) {
                button.select();
            }
			});
	    appearance_downarrow->setTickCallback([](Widget& widget){
	        if (widget.isSelected()) {
	            auto button = static_cast<Button*>(&widget); assert(button);
	            auto subframe = static_cast<Frame*>(widget.getParent()); assert(subframe);
	            auto hover = subframe->findImage("hover"); assert(hover);
	            hover->pos.y = button->getSize().y;
	            race_selection[widget.getOwner()] = (hover->pos.y - 2) / 36;
	        }
	        });
		appearance_downarrow->addWidgetAction("MenuStart", "confirm");
		appearance_downarrow->addWidgetAction("MenuPageRightAlt", "chat");
		appearance_downarrow->addWidgetAction("MenuPageLeftAlt", "privacy");
		appearance_downarrow->setWidgetBack("back_button");
	    appearance_downarrow->addWidgetAction("MenuPageLeft", "male");
	    appearance_downarrow->addWidgetAction("MenuPageRight", "female");
	    appearance_downarrow->addWidgetAction("MenuAlt1", "disable_abilities");
	    appearance_downarrow->addWidgetAction("MenuAlt2", "show_race_info");

		static auto appearance_fn = [](Frame::entry_t& entry, int index){
			if (stats[index]->playerRace != RACE_HUMAN) {
				return;
			}
			stats[index]->appearance = std::stoi(entry.name);
		};

		for (int c = 0; c < num_appearances; ++c) {
			auto name = appearance_names[c];
			auto entry = appearances->addEntry(std::to_string(c).c_str(), true);
			entry->color = color_dlc0;
			entry->text = name;
            entry->click = [](Frame::entry_t& entry){
                soundActivate();
                const int player = entry.parent.getOwner();
                appearance_fn(entry, player);
                if (!inputs.hasController(player)) {
                    entry.parent.activate();
                }
            };
			entry->selected = entry->click;
			if (stats[index]->appearance == c && stats[index]->playerRace == RACE_HUMAN) {
				appearances->setSelection(c);
				appearances->scrollToSelection();
			}
		}

		auto bottom = card->addFrame("bottom");
		bottom->setSize(details ?
		    SDL_Rect{0, 494, 324, 170}:
		    SDL_Rect{0, 282, 324, 170});
		bottom->setBorder(0);
		bottom->setColor(0);

		auto disable_abilities_text = bottom->addField("disable_abilities_text", 256);
		disable_abilities_text->setSize(SDL_Rect{44, 0, 154, 48});
		disable_abilities_text->setFont(smallfont_outline);
		disable_abilities_text->setColor(makeColor(166, 123, 81, 255));
		disable_abilities_text->setText("Disable monster\nrace abilities");
		disable_abilities_text->setHJustify(Field::justify_t::LEFT);
		disable_abilities_text->setVJustify(Field::justify_t::CENTER);
		disable_abilities_text->setTickCallback([](Widget& widget){
			auto field = static_cast<Field*>(&widget); assert(field);
			auto parent = static_cast<Frame*>(widget.getParent()); assert(parent);
			auto button = parent->findButton("disable_abilities"); assert(button);
			const auto player = widget.getOwner();
			if (stats[player]->playerRace == RACE_HUMAN) {
				field->setTextColor(makeColor(127, 96, 81, 255));
				button->setDisabled(true);
				button->setPressed(false);
			} else {
				field->setTextColor(makeColor(255, 191, 127, 255));
				button->setDisabled(false);
			}
			});

		auto disable_abilities = bottom->addButton("disable_abilities");
		disable_abilities->setSize(SDL_Rect{194, 2, 44, 44});
		disable_abilities->setIcon("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/Fill_Checked_00.png");
		disable_abilities->setColor(0);
		disable_abilities->setBorderColor(0);
		disable_abilities->setBorder(0);
		disable_abilities->setHighlightColor(0);
		disable_abilities->setDisabled(true); // the above tick function will clear this if it can be used
		disable_abilities->setStyle(Button::style_t::STYLE_CHECKBOX);
		disable_abilities->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
		disable_abilities->addWidgetAction("MenuStart", "confirm");
		disable_abilities->addWidgetAction("MenuPageRightAlt", "chat");
		disable_abilities->addWidgetAction("MenuPageLeftAlt", "privacy");
		disable_abilities->setWidgetBack("back_button");
		disable_abilities->setWidgetDown("show_race_info");
		disable_abilities->setWidgetUp(races[num_races - 1]);
		if (stats[index]->playerRace != RACE_HUMAN) {
			disable_abilities->setPressed(stats[index]->appearance != 0);
		}
		static auto disable_abilities_fn = [](Button& button, int index){
			if (stats[index]->playerRace == RACE_HUMAN) {
				soundError();
			} else {
				stats[index]->appearance = button.isPressed() ? 1 : 0;
				auto check = isCharacterValidFromDLC(*stats[index], client_classes[index]);
				if (check != VALID_OK_CHARACTER) {
					// player tried to play a class they haven't unlocked for this race
					// revert them to a barbarian.
					soundError();
					client_classes[index] = CLASS_BARBARIAN;
					stats[index]->clearStats();
					initClass(index);
					sendPlayerOverNet();
					saveLastCharacter(index, multiplayer);
				} else {
					soundCheckmark();
				}
			}
		};
		disable_abilities->setCallback([](Button& button){disable_abilities_fn(button, button.getOwner());});
	    disable_abilities->addWidgetAction("MenuPageLeft", "male");
	    disable_abilities->addWidgetAction("MenuPageRight", "female");
	    disable_abilities->addWidgetAction("MenuAlt1", "disable_abilities");
	    disable_abilities->addWidgetAction("MenuAlt2", "show_race_info");

		auto male_button = bottom->addButton("male");
		male_button->setPressed(stats[index]->sex == MALE);
		male_button->setColor(stats[index]->sex == MALE ? makeColorRGB(255, 255, 255) : makeColorRGB(127, 127, 127));
		male_button->setHighlightColor(stats[index]->sex == MALE ? makeColorRGB(255, 255, 255) : makeColorRGB(127, 127, 127));
		male_button->setStyle(Button::style_t::STYLE_RADIO);
		if (stats[index]->playerRace == RACE_AUTOMATON) {
			male_button->setIcon("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonMAutoOn_00.png");
			male_button->setBackground("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonMAuto_00.png");
			male_button->setBackgroundHighlighted("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonMAutoHigh_00.png");
			male_button->setBackgroundActivated("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonMAutoPress_00.png");
		} else {
			male_button->setIcon("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonMaleOn_00.png");
			male_button->setBackground("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonMale_00.png");
			male_button->setBackgroundHighlighted("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonMaleHigh_00.png");
			male_button->setBackgroundActivated("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonMalePress_00.png");
		}
		male_button->setSize(SDL_Rect{44, details ? 48 : 60, 58, 52});
		male_button->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
		male_button->addWidgetAction("MenuStart", "confirm");
		male_button->addWidgetAction("MenuPageRightAlt", "chat");
		male_button->addWidgetAction("MenuPageLeftAlt", "privacy");
		male_button->setWidgetBack("back_button");
		male_button->setWidgetUp("disable_abilities");
		male_button->setWidgetDown("confirm");
		male_button->setWidgetRight("female");
		male_button->setCallback([](Button& button){soundActivate(); male_button_fn(button, button.getOwner());});
	    male_button->addWidgetAction("MenuPageLeft", "male");
	    male_button->addWidgetAction("MenuPageRight", "female");
	    male_button->addWidgetAction("MenuAlt1", "disable_abilities");
	    male_button->addWidgetAction("MenuAlt2", "show_race_info");
		male_button->setTickCallback([](Widget& widget){
			const int index = widget.getOwner();
			auto button = static_cast<Button*>(&widget); assert(button);
			if (stats[index]->playerRace == RACE_AUTOMATON) {
				button->setIcon("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonMAutoOn_00.png");
				button->setBackground("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonMAuto_00.png");
				button->setBackgroundHighlighted("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonMAutoHigh_00.png");
				button->setBackgroundActivated("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonMAutoPress_00.png");
			} else {
				button->setIcon("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonMaleOn_00.png");
				button->setBackground("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonMale_00.png");
				button->setBackgroundHighlighted("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonMaleHigh_00.png");
				button->setBackgroundActivated("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonMalePress_00.png");
			}
			});

		auto female_button = bottom->addButton("female");
		female_button->setPressed(stats[index]->sex == FEMALE);
		female_button->setColor(stats[index]->sex == FEMALE ? makeColorRGB(255, 255, 255) : makeColorRGB(127, 127, 127));
		female_button->setHighlightColor(stats[index]->sex == FEMALE ? makeColorRGB(255, 255, 255) : makeColorRGB(127, 127, 127));
		female_button->setStyle(Button::style_t::STYLE_RADIO);
		if (stats[index]->playerRace == RACE_AUTOMATON) {
			female_button->setIcon("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonFAutoOn_00.png");
			female_button->setBackground("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonFAuto_00.png");
			female_button->setBackgroundHighlighted("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonFAutoHigh_00.png");
			female_button->setBackgroundActivated("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonFAutoPress_00.png");
		} else {
			female_button->setIcon("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonFemaleOn_00.png");
			female_button->setBackground("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonFemale_00.png");
			female_button->setBackgroundHighlighted("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonFemaleHigh_00.png");
			female_button->setBackgroundActivated("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonFemalePress_00.png");
		}
		female_button->setSize(SDL_Rect{106, details ? 48 : 60, 58, 52});
		female_button->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
		female_button->addWidgetAction("MenuStart", "confirm");
		female_button->addWidgetAction("MenuPageRightAlt", "chat");
		female_button->addWidgetAction("MenuPageLeftAlt", "privacy");
		female_button->setWidgetBack("back_button");
		female_button->setWidgetUp("disable_abilities");
		female_button->setWidgetDown("confirm");
		female_button->setWidgetLeft("male");
		female_button->setWidgetRight("show_race_info");
		female_button->setCallback([](Button& button){soundActivate(); female_button_fn(button, button.getOwner());});
	    female_button->addWidgetAction("MenuPageLeft", "male");
	    female_button->addWidgetAction("MenuPageRight", "female");
	    female_button->addWidgetAction("MenuAlt1", "disable_abilities");
	    female_button->addWidgetAction("MenuAlt2", "show_race_info");
		female_button->setTickCallback([](Widget& widget){
			const int index = widget.getOwner();
			auto button = static_cast<Button*>(&widget); assert(button);
			if (stats[index]->playerRace == RACE_AUTOMATON) {
				button->setIcon("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonFAutoOn_00.png");
				button->setBackground("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonFAuto_00.png");
				button->setBackgroundHighlighted("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonFAutoHigh_00.png");
				button->setBackgroundActivated("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonFAutoPress_00.png");
			} else {
				button->setIcon("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonFemaleOn_00.png");
				button->setBackground("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonFemale_00.png");
				button->setBackgroundHighlighted("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonFemaleHigh_00.png");
				button->setBackgroundActivated("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonFemalePress_00.png");
			}
			});

		auto show_race_info = bottom->addButton("show_race_info");
		show_race_info->setFont(smallfont_outline);
		if (details) {
		    show_race_info->setText("Hide Race\nInfo");
		} else {
		    show_race_info->setText("Show Race\nInfo");
		}
		show_race_info->setColor(makeColor(255, 255, 255, 255));
		show_race_info->setHighlightColor(makeColor(255, 255, 255, 255));
		show_race_info->setBackground("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonShowDetails_00.png");
		show_race_info->setBackgroundHighlighted("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonShowDetailsHigh_00.png");
		show_race_info->setBackgroundActivated("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonShowDetailsPress_00.png");
		show_race_info->setSize(SDL_Rect{168, details ? 48 : 60, 110, 52});
		show_race_info->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
		show_race_info->addWidgetAction("MenuStart", "confirm");
		show_race_info->addWidgetAction("MenuPageRightAlt", "chat");
		show_race_info->addWidgetAction("MenuPageLeftAlt", "privacy");
		show_race_info->setWidgetBack("back_button");
		show_race_info->setWidgetUp("disable_abilities");
		show_race_info->setWidgetDown("confirm");
		show_race_info->setWidgetLeft("female");
		if (details) {
		    show_race_info->setCallback([](Button& button){
				characterCardRaceMenu(button.getOwner(), false, race_selection[button.getOwner()]);
				soundActivate();
				});
	    } else {
		    show_race_info->setCallback([](Button& button){
				characterCardRaceMenu(button.getOwner(), true, race_selection[button.getOwner()]);
				soundActivate();
				});
	    }
	    show_race_info->addWidgetAction("MenuPageLeft", "male");
	    show_race_info->addWidgetAction("MenuPageRight", "female");
	    show_race_info->addWidgetAction("MenuAlt1", "disable_abilities");
	    show_race_info->addWidgetAction("MenuAlt2", "show_race_info");

		/*auto confirm = card->addButton("confirm");
		confirm->setFont(bigfont_outline);
		confirm->setText("Confirm");
		confirm->setColor(makeColor(255, 255, 255, 255));
		confirm->setHighlightColor(makeColor(255, 255, 255, 255));
		confirm->setBackground("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonConfirm_00.png");
		confirm->setSize(SDL_Rect{62, 430, 202, 52});
		confirm->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
		confirm->addWidgetAction("MenuStart", "confirm");
		confirm->setWidgetBack("back_button");
		confirm->setWidgetUp("female");
		confirm->setCallback([](Button& button){soundActivate(); back_fn(button.getOwner());});*/
	}

	static void characterCardClassMenu(int index, bool details, int selection) {
        static int class_selection[MAXPLAYERS];
        
		auto reduced_class_list = reducedClassList(index);
		auto card = initCharacterCard(index, details? 664 : 446);
        if (!card) {
            return;
        }

		static void (*back_fn)(int) = [](int index){
            if (inputs.hasController(index)) {
                client_classes[index] = old_classes[index];
                stats[index]->clearStats();
                initClass(index);
            }
            createCharacterCard(index);
            
            assert(main_menu_frame);
            auto lobby = main_menu_frame->findFrame("lobby"); assert(lobby);
            auto card = lobby->findFrame((std::string("card") + std::to_string(index)).c_str()); assert(card);
            auto button = card->findButton("class"); assert(button);
            button->select();
		};

		(void)createBackWidget(card,[](Button& button){soundCancel(); back_fn(button.getOwner());});

		auto backdrop = card->addImage(
			card->getActualSize(),
			0xffffffff,
			details ?
			    "*#images/ui/Main Menus/Play/PlayerCreation/ClassSelection/ClassSelect_Details.png":
			    "*#images/ui/Main Menus/Play/PlayerCreation/ClassSelection/ClassSelect_Window_04.png",
			"backdrop"
		);

		auto header = card->addField("header", 64);
		header->setSize(SDL_Rect{32, 14, 260, 38});
		header->setFont(smallfont_outline);
		header->setText("CLASS SELECTION");
		header->setJustify(Field::justify_t::CENTER);

		/*auto class_name_header = card->addField("class_name_header", 64);
		class_name_header->setSize(SDL_Rect{98, 70, 128, 26});
		class_name_header->setFont(smallfont_outline);
		class_name_header->setText("Fix this");
		class_name_header->setHJustify(Field::justify_t::CENTER);
		class_name_header->setVJustify(Field::justify_t::BOTTOM);*/

        if (details) {
  		    static auto class_desc_fn = [](Field& field, int index){
			    const int i = std::min(std::max(0, client_classes[index]), (Sint32)(ClassDescriptions::data.size() - 1));
				field.setText(ClassDescriptions::data[i].text.c_str());
			    if (i < CLASS_CONJURER) {
			        field.addColorToLine(0, color_dlc0);
			    } else if (i < CLASS_MACHINIST) {
			        field.addColorToLine(0, color_dlc1);
			    } else {
			        field.addColorToLine(0, color_dlc2);
			    }

				field.clearIndividualLinePadding();
				for ( auto line = 0; line < ClassDescriptions::data[i].linePaddings.size(); ++line )
				{
					field.setIndividualLinePadding(line, ClassDescriptions::data[i].linePaddings[line]);
				}
		    };

		    auto class_desc = card->addField("class_desc", 1024);
		    class_desc->setSize(SDL_Rect{42, 68, 240, 220});
		    class_desc->setFont(smallfont_no_outline);
		    class_desc->setTickCallback([](Widget& widget){class_desc_fn(*static_cast<Field*>(&widget), widget.getOwner());});
		    (*class_desc->getTickCallback())(*class_desc);

            // stats definitions
		    const char* class_stats_text[] = {
		        "STR", "DEX", "CON", "INT", "PER", "CHR"
		    };
		    constexpr int num_class_stats = sizeof(class_stats_text) / sizeof(class_stats_text[0]);
		    constexpr SDL_Rect bottom{44, 302, 236, 68};
		    constexpr int column = bottom.w / num_class_stats;

		    for (int c = 0; c < num_class_stats; ++c) {
		        static auto class_stat_fn = [](Field& field, int index){
			        const int i = std::min(std::max(0, client_classes[index]), (Sint32)(ClassDescriptions::data.size() - 1));
			        const int s = (int)strtol(field.getName(), nullptr, 10);
			        field.setColor(ClassDescriptions::data[i].statRatings[s]);

					if ( auto parent = static_cast<Frame*>(field.getParent()) )
					{
						char buf[32];
						//snprintf(buf, sizeof(buf), "stat img top %d", s);
						//auto class_stat_img_top = parent->findImage(buf);

						snprintf(buf, sizeof(buf), "stat img bottom %d", s);
						auto class_stat_img_bottom = parent->findImage(buf);
						if ( /*!class_stat_img_top ||*/ !class_stat_img_bottom ) 
						{
							return;
						}
						if ( ClassDescriptions::data[i].statRatingsStrings[s] == "bad" )
						{
							//class_stat_img_top->disabled = true;
							class_stat_img_bottom->disabled = false;

							class_stat_img_bottom->path = 
								"*#images/ui/Main Menus/Play/PlayerCreation/ClassSelection/statgrowth_lo2.png";
						}
						else if ( ClassDescriptions::data[i].statRatingsStrings[s] == "poor" )
						{
							//class_stat_img_top->disabled = true;
							class_stat_img_bottom->disabled = false;

							class_stat_img_bottom->path =
								"*#images/ui/Main Menus/Play/PlayerCreation/ClassSelection/statgrowth_lo1.png";
						}
						else if ( ClassDescriptions::data[i].statRatingsStrings[s] == "decent" )
						{
							//class_stat_img_top->disabled = false;
							class_stat_img_bottom->disabled = false;

							class_stat_img_bottom->path =
								"*#images/ui/Main Menus/Play/PlayerCreation/ClassSelection/statgrowth_hi1.png";
						}
						else if ( ClassDescriptions::data[i].statRatingsStrings[s] == "good" )
						{
							//class_stat_img_top->disabled = false;
							class_stat_img_bottom->disabled = false;

							class_stat_img_bottom->path =
								"*#images/ui/Main Menus/Play/PlayerCreation/ClassSelection/statgrowth_hi2.png";
						}
						else
						{
							//class_stat_img_top->disabled = true;
							class_stat_img_bottom->disabled = false;

							class_stat_img_bottom->path =
								"*#images/ui/Main Menus/Play/PlayerCreation/ClassSelection/statgrowth_neutral.png";
						}
					}
		        };
		        static char buf[16];
		        snprintf(buf, sizeof(buf), "%d", c);
		        auto class_stat = card->addField(buf, 16);
		        class_stat->setSize(SDL_Rect{
		            bottom.x + column * c, bottom.y, column, bottom.h});
		        class_stat->setHJustify(Field::justify_t::CENTER);
		        class_stat->setVJustify(Field::justify_t::TOP);
		        class_stat->setFont(smallfont_outline);
		        class_stat->setText(class_stats_text[c]);
		        class_stat->setTickCallback([](Widget& widget){class_stat_fn(*static_cast<Field*>(&widget), widget.getOwner());});

				SDL_Rect imgPos = class_stat->getSize();
				imgPos.x += imgPos.w / 2;
				imgPos.w = 14;
				imgPos.x -= imgPos.w / 2;
				imgPos.h = 16;
				imgPos.y -= imgPos.h - 4;

				static char buf2[32];
				/*snprintf(buf2, sizeof(buf2), "stat img top %d", c);
				auto class_stat_img_top = card->addImage(imgPos, 0xFFFFFFFF,
					"*#images/ui/Main Menus/Play/PlayerCreation/ClassSelection/statgrowth_hi2.png", buf2);
				class_stat_img_top->disabled = true;*/
				snprintf(buf2, sizeof(buf2), "stat img bottom %d", c);
				imgPos.y = class_stat->getSize().y + 17;
				auto class_stat_img_bottom = card->addImage(imgPos, 0xFFFFFFFF,
					"*#images/ui/Main Menus/Play/PlayerCreation/ClassSelection/statgrowth_lo2.png", buf2);
				class_stat_img_bottom->disabled = true;

				(*class_stat->getTickCallback())(*class_stat);
		    }

			// hpmp header
			{
				SDL_Rect hpmp_size{ 48, 339, 52, 44 };
				auto hpmp_header = card->addField("hpmp_header", 32);
				hpmp_header->setFont(smallfont_outline);
				hpmp_header->setColor(makeColorRGB(209, 166, 161));
				hpmp_header->setText("HP:\nMP:");
				hpmp_header->setHJustify(Field::justify_t::LEFT);
				hpmp_header->setVJustify(Field::justify_t::TOP);
				hpmp_header->setSize(hpmp_size);
				hpmp_header->setPaddingPerLine(-4);

				static constexpr int hpmp_buf_size = 32;
				static auto hpmp_fn = [](Field& field, int index) {
					const int i = std::min(std::max(0, client_classes[index]), (Sint32)(ClassDescriptions::data.size() - 1));
					char buf[hpmp_buf_size];
					snprintf(buf, sizeof(buf), "%d\n%d",
						ClassDescriptions::data[i].hp,
						ClassDescriptions::data[i].mp);
					field.setText(buf);
				};

				auto hpmp_values = card->addField("hpmp_values", 128);
				hpmp_size.x += 32;
                hpmp_values->setFont(smallfont_outline);
                hpmp_values->setPaddingPerLine(-4);
				hpmp_values->setColor(makeColorRGB(209, 166, 161));
				hpmp_values->setText("20\n20");
				hpmp_values->setHJustify(Field::justify_t::RIGHT);
				if ( hpmp_values->getHJustify() == Field::justify_t::RIGHT )
				{
					hpmp_size.x -= 26;
				}
				hpmp_values->setVJustify(Field::justify_t::TOP);
				hpmp_values->setSize(hpmp_size);
				hpmp_values->setTickCallback([](Widget& widget) {hpmp_fn(*static_cast<Field*>(&widget), widget.getOwner()); });
				(*hpmp_values->getTickCallback())(*hpmp_values);
			}

		    // difficulty header
		    constexpr SDL_Rect difficulty_size{115, 339, 158, 44};
		    auto difficulty_header = card->addField("difficulty_header", 128);
		    difficulty_header->setFont(smallfont_outline);
		    difficulty_header->setColor(makeColorRGB(209, 166, 161));
		    difficulty_header->setText("Survival:\nComplexity:");
		    difficulty_header->setHJustify(Field::justify_t::LEFT);
		    difficulty_header->setVJustify(Field::justify_t::TOP);
		    difficulty_header->setSize(difficulty_size);
			difficulty_header->setPaddingPerLine(-4);

		    // difficulty stars
		    static constexpr int star_buf_size = 32;
	        static auto stars_fn = [](Field& field, int index){
		        const int i = std::min(std::max(0, client_classes[index]), (Sint32)(ClassDescriptions::data.size() - 1));
		        for (int c = 0; c < 2; ++c) {
					field.addColorToLine(c, std::get<2>(ClassDescriptions::data[i].survivalComplexity[c]));
		        }
		        char buf[star_buf_size];
		        snprintf(buf, sizeof(buf), "%s\n%s", 
					std::get<1>(ClassDescriptions::data[i].survivalComplexity[0]).c_str(), 
					std::get<1>(ClassDescriptions::data[i].survivalComplexity[1]).c_str());
		        field.setText(buf);
	        };

		    auto difficulty_stars = card->addField("difficulty_stars", star_buf_size);
		    difficulty_stars->setFont(smallfont_outline);
		    difficulty_stars->setHJustify(Field::justify_t::RIGHT);
		    difficulty_stars->setVJustify(Field::justify_t::TOP);
		    difficulty_stars->setSize(difficulty_size);
	        difficulty_stars->setTickCallback([](Widget& widget){stars_fn(*static_cast<Field*>(&widget), widget.getOwner());});
			difficulty_stars->setPaddingPerLine(-4);
	        (*difficulty_stars->getTickCallback())(*difficulty_stars);
        } else {
		    static auto class_name_fn = [](Field& field, int index){
			    const int i = std::min(std::max(0, client_classes[index]), num_classes - 1);
			    auto find = classes.find(classes_in_order[i]);
			    if (find != classes.end()) {
				    field.setText(find->second.name);
			    }
			    if (i < CLASS_CONJURER) {
			        field.setColor(color_dlc0);
			    } else if (i < CLASS_MACHINIST) {
			        field.setColor(color_dlc1);
			    } else {
			        field.setColor(color_dlc2);
			    }
		    };

		    auto class_name = card->addField("class_name", 64);
		    class_name->setSize(SDL_Rect{66, 64, 192, 46});
		    class_name->setHJustify(Field::justify_t::CENTER);
		    class_name->setVJustify(Field::justify_t::CENTER);
		    class_name->setFont(smallfont_outline);
		    class_name->setTickCallback([](Widget& widget){class_name_fn(*static_cast<Field*>(&widget), widget.getOwner());});
		    (*class_name->getTickCallback())(*class_name);
		}

		const int height = std::max(254, 6 + 54 * (int)(num_classes / 4 + ((num_classes % 4) ? 1 : 0)));

		auto subframe = card->addFrame("subframe");
		subframe->setScrollBarsEnabled(false);
		if (details) {
		    subframe->setSize(SDL_Rect{34, 392, 226, 144});
		} else {
		    subframe->setSize(SDL_Rect{34, 120, 226, 198});
		}
		subframe->setActualSize(SDL_Rect{0, 0, 226, height});
		subframe->setBorder(0);
		subframe->setColor(0);

		if (subframe->getActualSize().h > subframe->getSize().h) {
			auto slider = card->addSlider("scroll_slider");
			if (details) {
			    slider->setRailSize(SDL_Rect{260, 394, 30, 142});
			    slider->setRailImage("*images/ui/Main Menus/Play/PlayerCreation/ClassSelection/ClassSelect_ScrollBar_01.png");
			} else {
			    slider->setRailSize(SDL_Rect{260, 122, 30, 196});
			    slider->setRailImage("*images/ui/Main Menus/Play/PlayerCreation/ClassSelection/ClassSelect_ScrollBar_00.png");
			}
			slider->setHandleSize(SDL_Rect{0, 0, 34, 34});
			slider->setHandleImage("*images/ui/Main Menus/Play/PlayerCreation/ClassSelection/ClassSelect_ScrollBar_SliderB_00.png");
			slider->setOrientation(Slider::orientation_t::SLIDER_VERTICAL);
			slider->setBorder(24);
			slider->setMinValue(0.f);
			slider->setMaxValue(subframe->getActualSize().h - subframe->getSize().h);
			slider->setCallback([](Slider& slider){
				Frame* frame = static_cast<Frame*>(slider.getParent());
				Frame* subframe = frame->findFrame("subframe"); assert(subframe);
				auto actualSize = subframe->getActualSize();
				actualSize.y = slider.getValue();
				subframe->setActualSize(actualSize);
				});
			slider->setTickCallback([](Widget& widget){
				Slider* slider = static_cast<Slider*>(&widget);
				Frame* frame = static_cast<Frame*>(slider->getParent());
				Frame* subframe = frame->findFrame("subframe"); assert(subframe);
				auto actualSize = subframe->getActualSize();
				slider->setValue(actualSize.y);
				});
			slider->setWidgetSearchParent(card->getName());
			slider->setWidgetLeft(reduced_class_list[0]);
		    slider->setWidgetBack("back_button");
		}

		auto class_info = card->addButton("class_info");
		class_info->setColor(makeColor(255, 255, 255, 255));
		class_info->setHighlightColor(makeColor(255, 255, 255, 255));
		if (details) {
		    class_info->setSize(SDL_Rect{42, 542, 194, 36});
            class_info->setBackground("*images/ui/Main Menus/Play/PlayerCreation/ClassSelection/ClassSelect_Button_InfoOn_02.png");
            class_info->setBackgroundHighlighted("*images/ui/Main Menus/Play/PlayerCreation/ClassSelection/ClassSelect_Button_InfoHighOn_02.png");
            class_info->setBackgroundActivated("*images/ui/Main Menus/Play/PlayerCreation/ClassSelection/ClassSelect_Button_InfoPress_02.png");
		} else {
		    class_info->setSize(SDL_Rect{42, 324, 194, 36});
            class_info->setBackground("*images/ui/Main Menus/Play/PlayerCreation/ClassSelection/ClassSelect_Button_Info_02.png");
            class_info->setBackgroundHighlighted("*images/ui/Main Menus/Play/PlayerCreation/ClassSelection/ClassSelect_Button_InfoHigh_02.png");
            class_info->setBackgroundActivated("*images/ui/Main Menus/Play/PlayerCreation/ClassSelection/ClassSelect_Button_InfoPressOff_02.png");
		}
		class_info->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
        class_info->setWidgetRight("randomize_class");
		class_info->addWidgetAction("MenuStart", "confirm");
		class_info->addWidgetAction("MenuPageRightAlt", "chat");
		class_info->addWidgetAction("MenuPageLeftAlt", "privacy");
        class_info->addWidgetAction("MenuAlt1", "randomize_class");
		class_info->addWidgetAction("MenuAlt2", "class_info");
		class_info->setWidgetBack("back_button");
        class_info->setTextOffset(SDL_Rect{-8, 0, 0, 0});
        class_info->setFont(smallfont_outline);
		class_info->setGlyphPosition(Widget::glyph_position_t::CENTERED_BOTTOM);
		if (details) {
            class_info->setText("Hide Class Info");
		    class_info->setCallback([](Button& button){
				characterCardClassMenu(button.getOwner(), false, class_selection[button.getOwner()]);
				soundActivate();
				});
		} else {
            class_info->setText("Show Class Info");
		    class_info->setCallback([](Button& button){
				characterCardClassMenu(button.getOwner(), true, class_selection[button.getOwner()]);
				soundActivate();
				});
		}
        
        auto randomize_class = card->addButton("randomize_class");
        randomize_class->setColor(makeColor(255, 255, 255, 255));
        randomize_class->setHighlightColor(makeColor(255, 255, 255, 255));
        if (details) {
            randomize_class->setSize(SDL_Rect{242, 540, 36, 40});
        } else {
            randomize_class->setSize(SDL_Rect{242, 322, 36, 40});
        }
        randomize_class->setBackground("*images/ui/Main Menus/Play/PlayerCreation/ClassSelection/ClassSelect_Icon_Randomize_00.png");
        randomize_class->setBackgroundHighlighted("*images/ui/Main Menus/Play/PlayerCreation/ClassSelection/ClassSelect_Icon_RandomizeHigh_00.png");
        randomize_class->setBackgroundActivated("*images/ui/Main Menus/Play/PlayerCreation/ClassSelection/ClassSelect_Icon_RandomizePress_00.png");
        randomize_class->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
        randomize_class->setWidgetLeft("class_info");
        randomize_class->addWidgetAction("MenuStart", "confirm");
        randomize_class->addWidgetAction("MenuPageRightAlt", "chat");
        randomize_class->addWidgetAction("MenuPageLeftAlt", "privacy");
        randomize_class->addWidgetAction("MenuAlt1", "randomize_class");
        randomize_class->addWidgetAction("MenuAlt2", "class_info");
        randomize_class->setWidgetBack("back_button");
        randomize_class->setGlyphPosition(Widget::glyph_position_t::CENTERED_BOTTOM);
        randomize_class->setCallback([](Button& button){
            const int index = button.getOwner();
            soundActivate();

            auto reduced_class_list = reducedClassList(index);
            auto random_class = reduced_class_list[RNG.uniform(0, (int)reduced_class_list.size() - 1)];
            for (int c = 0; c < num_classes; ++c) {
               if (strcmp(random_class, classes_in_order[c]) == 0) {
                   client_classes[index] = c;
                   break;
               }
            }
            if (inputs.hasController(index)) {
                auto frame = static_cast<Frame*>(button.getParent());
                auto subframe = frame->findFrame("subframe"); assert(subframe);
                for (auto button : subframe->getButtons()) {
                    if (strcmp(button->getName(), classes_in_order[client_classes[index]]) == 0) {
                        button->select();
                        break;
                    }
                }
            }

            stats[index]->clearStats();
            initClass(index);
            sendPlayerOverNet();
            saveLastCharacter(index, multiplayer);
        });

		const int current_class = std::min(std::max(0, client_classes[index]), num_classes - 1);
		auto current_class_name = classes_in_order[current_class];

        bool selected_button = false;
		static const std::string prefix = "*images/ui/Main Menus/Play/PlayerCreation/ClassSelection/";
		for (int c = num_classes - 1; c >= 0; --c) {
			auto name = classes_in_order[c];
			auto find = classes.find(name);
			assert(find != classes.end());
			auto& full_class = find->second;
			auto button = subframe->addButton(name);
			switch (full_class.dlc) {
			case DLC::Base:
				button->setBackground((prefix + "ClassSelect_IconBGBase_00.png").c_str());
				button->setBackgroundHighlighted((prefix + "ClassSelect_IconBGBaseHigh_00.png").c_str());
				button->setBackgroundActivated((prefix + "ClassSelect_IconBGBasePress_00.png").c_str());
				break;
			case DLC::MythsAndOutcasts:
				button->setBackground((prefix + "ClassSelect_IconBGMyths_00.png").c_str());
				button->setBackgroundHighlighted((prefix + "ClassSelect_IconBGMythsHigh_00.png").c_str());
				button->setBackgroundActivated((prefix + "ClassSelect_IconBGMythsPress_00.png").c_str());
				break;
			case DLC::LegendsAndPariahs:
				button->setBackground((prefix + "ClassSelect_IconBGLegends_00.png").c_str());
				button->setBackgroundHighlighted((prefix + "ClassSelect_IconBGLegendsHigh_00.png").c_str());
				button->setBackgroundActivated((prefix + "ClassSelect_IconBGLegendsPress_00.png").c_str());
				break;
			}
			if (isCharacterValidFromDLC(*stats[index], c) == VALID_OK_CHARACTER) {
				if (strcmp(name, current_class_name) == 0) {
					button->setIcon((prefix + full_class.image_highlighted).c_str());
				} else {
					button->setIcon((prefix + full_class.image).c_str());
				}
			} else {
				button->setIcon((prefix + full_class.image_locked).c_str());
			}
			button->setColor(makeColor(255, 255, 255, 255));
			button->setHighlightColor(makeColor(255, 255, 255, 255));
			button->setSize(SDL_Rect{8 + (c % 4) * 54, 6 + (c / 4) * 54, 54, 54});
			if ((!selection && !strcmp(name, current_class_name)) ||
			    (selection && c == selection)) {
			    button->select();
			    button->scrollParent();
			    selected_button = true;
			}
			button->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
			button->setWidgetLeft(c == 0 ? classes_in_order[num_classes - 1] : classes_in_order[c - 1]);
            button->setWidgetRight(c == num_classes - 1 ? classes_in_order[0] : classes_in_order[c + 1]);
			if (c == 0) {
				button->setWidgetUp(classes_in_order[num_classes - 1]);
			} else if (c == 1) {
				button->setWidgetUp(classes_in_order[num_classes - 4]);
			} else if (c == 2) {
                button->setWidgetUp(classes_in_order[num_classes - 3]);
            } else if (c == 3) {
                button->setWidgetUp(classes_in_order[num_classes - 2]);
            } else {
                button->setWidgetUp(classes_in_order[c - 4]);
            }
            if (c == num_classes - 1) {
                button->setWidgetDown(classes_in_order[0]);
            } else if (c == num_classes - 2) {
                button->setWidgetDown(classes_in_order[3]);
            } else if (c == num_classes - 3) {
                button->setWidgetDown(classes_in_order[2]);
            } else if (c == num_classes - 4) {
                button->setWidgetDown(classes_in_order[1]);
            } else {
                button->setWidgetDown(classes_in_order[c + 4]);
            }
			button->addWidgetAction("MenuStart", "confirm");
			button->addWidgetAction("MenuPageRightAlt", "chat");
			button->addWidgetAction("MenuPageLeftAlt", "privacy");
            button->addWidgetAction("MenuAlt1", "randomize_class");
			button->addWidgetAction("MenuAlt2", "class_info");
			button->setWidgetBack("back_button");

			// add a lock icon
			if (isCharacterValidFromDLC(*stats[index], c) != VALID_OK_CHARACTER) {
				const auto lock_name = std::string(button->getName()) + "lock";
				auto lock = subframe->addImage(
					button->getSize(),
					0xffffffff,
					"*#images/ui/Main Menus/Play/PlayerCreation/ClassSelection/ClassLocked_Icon_00.png",
					lock_name.c_str());
				lock->ontop = true;
			}

			static auto button_fn = [](Button& button, int index){
				// figure out which class button we clicked based on name
				int c = 0;
				for (; c < num_classes; ++c) {
					if (strcmp(button.getName(), classes_in_order[c]) == 0) {
						break;
					}
				}

                // set class
                bool success = false;
                if (c < num_classes) {
                    auto check = isCharacterValidFromDLC(*stats[index], c);
                    if (check != VALID_OK_CHARACTER) {
                        switch (check) {
                        default:
                        case INVALID_CHARACTER:
                        case INVALID_REQUIRE_ACHIEVEMENT:
                            soundError();
                            break;
                        case INVALID_REQUIREDLC1:
                            openDLCPrompt(0);
                            break;
                        case INVALID_REQUIREDLC2:
                            openDLCPrompt(1);
                            break;
                        }
                    } else {
                        success = true;
                        soundActivate();
                        button.setColor(makeColor(255, 255, 255, 255)); // highlight this button
                        client_classes[index] = c;
                    }
                    
                    stats[index]->clearStats();
                    initClass(index);
                    sendPlayerOverNet();
                    saveLastCharacter(index, multiplayer);
                }
                
                // if using a gamepad, back out to the previous menu
                if (success) {
                    if (inputs.hasController(index)) {
                        createCharacterCard(index);
                        auto lobby = main_menu_frame->findFrame("lobby"); assert(lobby);
                        auto card = lobby->findFrame((std::string("card") + std::to_string(index)).c_str()); assert(card);
                        auto button = card->findButton("class"); assert(button);
                        button->select();
                    }
                }
			};

			button->setCallback([](Button& button){button_fn(button, button.getOwner());});
			button->setTickCallback([](Widget& widget){
			    auto button = static_cast<Button*>(&widget);
				const int index = widget.getOwner();

				int class_index = 0;
				for (int c = 0; c < num_classes; ++c) {
					if (strcmp(button->getName(), classes_in_order[c]) == 0) {
						if (button->isSelected()) {
							class_selection[widget.getOwner()] = c;
						}
						class_index = c;
						break;
					}
				}

				// you can request a particular class from your party with a hotkey online
			    if (button->isSelected()) {
					if (currentLobbyType != LobbyType::LobbyLocal) {
						const int player = widget.getOwner();
						if (inputs.hasController(player)) {
							auto& input = Input::inputs[player];
							size_t len = strlen(widget.getName());
							if (stringCmp(widget.getName(), "random", len, 6) && input.consumeBinaryToggle("MenuPageLeft")) {
								constexpr Uint32 waitingPeriod = 3;
								static Uint32 lastClassRequest = 0;
								char buf[1024];
								if (ticks - lastClassRequest >= TICKS_PER_SECOND * waitingPeriod) {
									int len = snprintf(buf, sizeof(buf), "%s: We need a %s.",
										players[player]->getAccountName(), widget.getName());
									Uint32 color = playerColor(player, colorblind_lobby, false);
									sendChatMessageOverNet(color, buf, (size_t)len);
									lastClassRequest = ticks;
								} else {
									snprintf(buf, sizeof(buf), "*** Please wait %d seconds before suggesting another class. ***",
										waitingPeriod - (ticks - lastClassRequest) / TICKS_PER_SECOND);
									addLobbyChatMessage(uint32ColorBaronyBlue, buf);
								}
							}
						}
					}
			    }

				// update highlight state for class icon
				const auto name = button->getName();
				const auto find = classes.find(name);
				if (find != classes.end()) {
					const auto& full_class = find->second;
                    
                    // preview the class
                    if (button->isSelected() || button->isHighlighted() || client_classes[index] == class_index) {
                        if (inputs.hasController(index)) {
                            if (client_classes[index] != class_index) {
                                if (!button->isSelected()) {
                                    button->select();
                                }
                                client_classes[index] = class_index;
                                stats[index]->clearStats();
                                initClass(index);
                            }
                        }
                    }
                    
                    // set button icon
					if (isCharacterValidFromDLC(*stats[index], class_index) == VALID_OK_CHARACTER) {
						if (button->isSelected() || button->isHighlighted() || client_classes[index] == class_index) {
							button->setIcon((prefix + full_class.image_highlighted).c_str());
						} else {
							button->setIcon((prefix + full_class.image).c_str());
						}
					} else {
						button->setIcon((prefix + full_class.image_locked).c_str());
					}
				}

				// update lock icon
				auto subframe = static_cast<Frame*>(button->getParent());
				if (subframe) {
					const auto lock_name = std::string(button->getName()) + "lock";
					auto lock = subframe->findImage(lock_name.c_str());
					if (lock) {
						lock->path = button->isHighlighted() ?
							"*#images/ui/Main Menus/Play/PlayerCreation/ClassSelection/ClassLocked_IconHigh_00.png":
							"*#images/ui/Main Menus/Play/PlayerCreation/ClassSelection/ClassLocked_Icon_00.png";
					}
				}

				// rescue this player's focus
				if (!main_menu_frame) {
					return;
				}
				auto selectedWidget = main_menu_frame->findSelectedWidget(widget.getOwner());
				if (!selectedWidget) {
					widget.select();
				}
			    });
		}

        if (!selected_button) {
		    auto first_button = subframe->findButton(reduced_class_list[0]);
		    assert(first_button);
		    first_button->select();
		}

		/*auto confirm = card->addButton("confirm");
		confirm->setColor(makeColor(255, 255, 255, 255));
		confirm->setHighlightColor(makeColor(255, 255, 255, 255));
		confirm->setBackground("*images/ui/Main Menus/Play/PlayerCreation/Finalize_Button_ReadyBase_00.png");
		confirm->setSize(SDL_Rect{62, 430, 202, 52});
		confirm->setText("Confirm");
		confirm->setFont(bigfont_outline);
		confirm->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
		confirm->addWidgetAction("MenuStart", "confirm");
        confirm->addWidgetAction("MenuAlt1", "randomize_class");
		confirm->addWidgetAction("MenuAlt2", "class_info");
		confirm->setWidgetBack("back_button");
		confirm->setCallback([](Button& button){soundActivate(); back_fn(button.getOwner());});*/
	}

	static void createCharacterCard(int index) {
		auto lobby = main_menu_frame->findFrame("lobby");
		if (!lobby) {
		    return;
		}

		if (multiplayer == CLIENT) {
			sendSvFlagsOverNet();
		}

		auto countdown = lobby->findFrame("countdown");
		if (countdown) {
		    countdown->removeSelf();
		}
        
        // make SURE this player is valid when the character card opens
        if (isCharacterValidFromDLC(*stats[index], client_classes[index]) != VALID_OK_CHARACTER) {
            stats[index]->playerRace = RACE_HUMAN;
            stats[index]->sex = static_cast<sex_t>(RNG.getU8() % 2);
            stats[index]->appearance = RNG.uniform(0, NUMAPPEARANCES - 1);
            client_classes[index] = 0;
            stats[index]->clearStats();
            initClass(index);
        }
        
        sendReadyOverNet(index, false);
        sendPlayerOverNet();
		saveLastCharacter(index, multiplayer);

		auto card = initCharacterCard(index, 346);
        if (!card) {
            return;
        }

		(void)createBackWidget(card,[](Button& button){
			createStartButton(button.getOwner());
			checkReadyStates();
			soundCancel();
			});

		auto backdrop = card->addImage(
			card->getActualSize(),
			0xffffffff,
			"*images/ui/Main Menus/Play/PlayerCreation/Finalize_Window_01.png",
			"backdrop"
		);

		auto name_text = card->addField("name_text", 32);
		name_text->setSize(SDL_Rect{30, 30, 56, 36});
		name_text->setFont(smallfont_outline);
		name_text->setColor(makeColor(166, 123, 81, 255));
		name_text->setText("NAME:");
		name_text->setHJustify(Field::justify_t::RIGHT);
		name_text->setVJustify(Field::justify_t::CENTER);

		auto name_box = card->addImage(
			SDL_Rect{88, 30, 150, 36},
			0xffffffff,
			"*images/ui/Main Menus/Play/PlayerCreation/Finalize__NameField_00.png",
			"name_box"
		);

		auto name_field = card->addField("name", 32);
		name_field->setGlyphPosition(Widget::glyph_position_t::CENTERED_RIGHT);
		name_field->setSelectorOffset(SDL_Rect{-7, -7, 7, 7});
		name_field->setButtonsOffset(SDL_Rect{11, 0, 0, 0});
		name_field->setScroll(true);
		name_field->setGuide((std::string("Enter a name for Player ") + std::to_string(index + 1)).c_str());
		name_field->setFont(smallfont_outline);
		name_field->setText(stats[index]->name);
		name_field->setSize(SDL_Rect{90, 34, 146, 28});
		name_field->setColor(makeColor(166, 123, 81, 255));
		name_field->setBackgroundColor(makeColor(52, 30, 22, 255));
		name_field->setBackgroundSelectAllColor(makeColor(52, 30, 22, 255));
		name_field->setBackgroundActivatedColor(makeColor(52, 30, 22, 255));
		name_field->setHJustify(Field::justify_t::LEFT);
		name_field->setVJustify(Field::justify_t::CENTER);
#ifdef NINTENDO
		if (currentLobbyType == LobbyType::LobbyLocal) {
			name_field->setEditable(true);
		} else {
			name_field->setHideGlyphs(true);
		}
#else
		name_field->setEditable(true);
#endif
		name_field->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
		name_field->addWidgetAction("MenuStart", "ready");
		name_field->addWidgetAction("MenuPageRightAlt", "chat");
		name_field->addWidgetAction("MenuPageLeftAlt", "privacy");
		name_field->setWidgetBack("back_button");
		name_field->setWidgetRight("randomize_name");
		name_field->setWidgetDown("game_settings");
		static auto name_field_fn = [](const char* text, int index) {
			size_t old_len = std::min(sizeof(Stat::name), strlen(stats[index]->name) + 1);
			size_t new_len = strlen(text) + 1;
			size_t shortest_len = std::min(old_len, new_len);
			if (new_len != old_len || memcmp(stats[index]->name, text, shortest_len)) {
			    memcpy(stats[index]->name, text, new_len);
			    sendPlayerOverNet();
				saveLastCharacter(index, multiplayer);
			}
		};
		name_field->setCallback([](Field& field){name_field_fn(field.getText(), field.getOwner());});
		name_field->setTickCallback([](Widget& widget){
			Field* field = static_cast<Field*>(&widget);
			name_field_fn(field->getText(), field->getOwner());

			// rescue this player's focus
			if (!main_menu_frame) {
				return;
			}
			auto selectedWidget = main_menu_frame->findSelectedWidget(widget.getOwner());
			if (!selectedWidget) {
				widget.select();
			}
			});

		auto randomize_name = card->addButton("randomize_name");
		randomize_name->setColor(makeColor(255, 255, 255, 255));
		randomize_name->setHighlightColor(makeColor(255, 255, 255, 255));
		randomize_name->setBackground("*images/ui/Main Menus/Play/PlayerCreation/Finalize_Icon_Randomize_00.png");
		randomize_name->setBackgroundHighlighted("*images/ui/Main Menus/Play/PlayerCreation/Finalize_Icon_RandomizeHigh_00.png");
		randomize_name->setBackgroundActivated("*images/ui/Main Menus/Play/PlayerCreation/Finalize_Icon_RandomizePress_00.png");
		randomize_name->setSize(SDL_Rect{236, 22, 54, 54});
		randomize_name->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
		randomize_name->addWidgetAction("MenuStart", "ready");
		randomize_name->addWidgetAction("MenuPageRightAlt", "chat");
		randomize_name->addWidgetAction("MenuPageLeftAlt", "privacy");
		randomize_name->setWidgetBack("back_button");
		randomize_name->setWidgetLeft("name");
		randomize_name->setWidgetDown("game_settings");
		static auto randomize_name_fn = [](Button& button, int index) {
#ifdef NINTENDO
			auto& names = stats[index]->sex == sex_t::MALE ?
				randomPlayerNamesMale : randomPlayerNamesFemale;
			int choice;
			for (choice = 0; choice < names.size(); ++choice) {
				if (names[choice] == stats[index]->name) {
					++choice;
					break;
				}
			}
			choice = choice % names.size();
			auto name = names[choice].c_str();
			name_field_fn(name, index);
			auto card = static_cast<Frame*>(button.getParent());
			auto field = card->findField("name"); assert(field);
			field->setText(name);
#else
			auto& names = stats[index]->sex == sex_t::MALE ?
				randomPlayerNamesMale : randomPlayerNamesFemale;
			auto choice = RNG.uniform(0, (int)names.size() - 1);
			auto name = names[choice].c_str();
			name_field_fn(name, index);
			auto card = static_cast<Frame*>(button.getParent());
			auto field = card->findField("name"); assert(field);
			field->setText(name);
#endif
		};
		randomize_name->setCallback([](Button& button){soundActivate(); randomize_name_fn(button, button.getOwner());});
		
		auto game_settings = card->addButton("game_settings");
		game_settings->setSize(SDL_Rect{62, 76, 202, 52});
		game_settings->setColor(makeColor(255, 255, 255, 255));
		game_settings->setHighlightColor(makeColor(255, 255, 255, 255));
		game_settings->setBackground("*images/ui/Main Menus/Play/PlayerCreation/Finalize_Button_ReadyBase_00.png");
		game_settings->setBackgroundHighlighted("*images/ui/Main Menus/Play/PlayerCreation/Finalize_Button_ReadyBaseHigh_00.png");
		game_settings->setBackgroundActivated("*images/ui/Main Menus/Play/PlayerCreation/Finalize_Button_ReadyBasePress_00.png");
		game_settings->setText("View Game Settings");
		game_settings->setFont(smallfont_outline);
		game_settings->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
		game_settings->addWidgetAction("MenuStart", "ready");
		game_settings->addWidgetAction("MenuPageRightAlt", "chat");
		game_settings->addWidgetAction("MenuPageLeftAlt", "privacy");
		game_settings->setWidgetBack("back_button");
		game_settings->setWidgetUp("name");
		game_settings->setWidgetDown("male");
		game_settings->setCallback([](Button& button){soundActivate(); characterCardLobbySettingsMenu(button.getOwner());});

		auto bottom = card->addFrame("bottom");
		bottom->setSize(SDL_Rect{42, 166, 120, 52});
		bottom->setBorder(0);
		bottom->setColor(0);

		auto male_button = bottom->addButton("male");
		male_button->setPressed(stats[index]->sex == MALE);
		male_button->setColor(stats[index]->sex == MALE ? makeColorRGB(255, 255, 255) : makeColorRGB(127, 127, 127));
		male_button->setHighlightColor(stats[index]->sex == MALE ? makeColorRGB(255, 255, 255) : makeColorRGB(127, 127, 127));
		male_button->setStyle(Button::style_t::STYLE_RADIO);
		if (stats[index]->playerRace == RACE_AUTOMATON) {
			male_button->setIcon("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonMAutoOn_00.png");
			male_button->setBackground("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonMAuto_00.png");
			male_button->setBackgroundHighlighted("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonMAutoHigh_00.png");
			male_button->setBackgroundActivated("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonMAutoPress_00.png");
		} else {
			male_button->setIcon("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonMaleOn_00.png");
			male_button->setBackground("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonMale_00.png");
			male_button->setBackgroundHighlighted("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonMaleHigh_00.png");
			male_button->setBackgroundActivated("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonMalePress_00.png");
		}
		male_button->setSize(SDL_Rect{0, 0, 58, 52});
		male_button->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
		male_button->addWidgetAction("MenuStart", "ready");
		male_button->addWidgetAction("MenuPageRightAlt", "chat");
		male_button->addWidgetAction("MenuPageLeftAlt", "privacy");
		male_button->setWidgetBack("back_button");
		male_button->setWidgetRight("female");
		male_button->setWidgetUp("game_settings");
		male_button->setWidgetDown("class");
		male_button->setCallback([](Button& button){soundActivate(); male_button_fn(button, button.getOwner());});
		male_button->setTickCallback([](Widget& widget){
			const int index = widget.getOwner();
			auto button = static_cast<Button*>(&widget); assert(button);
			if (stats[index]->playerRace == RACE_AUTOMATON) {
				button->setIcon("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonMAutoOn_00.png");
				button->setBackground("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonMAuto_00.png");
				button->setBackgroundHighlighted("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonMAutoHigh_00.png");
				button->setBackgroundActivated("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonMAutoPress_00.png");
			} else {
				button->setIcon("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonMaleOn_00.png");
				button->setBackground("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonMale_00.png");
				button->setBackgroundHighlighted("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonMaleHigh_00.png");
				button->setBackgroundActivated("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonMalePress_00.png");
			}
			});

		auto female_button = bottom->addButton("female");
		female_button->setPressed(stats[index]->sex == FEMALE);
		female_button->setColor(stats[index]->sex == FEMALE ? makeColorRGB(255, 255, 255) : makeColorRGB(127, 127, 127));
		female_button->setHighlightColor(stats[index]->sex == FEMALE ? makeColorRGB(255, 255, 255) : makeColorRGB(127, 127, 127));
		female_button->setStyle(Button::style_t::STYLE_RADIO);
		if (stats[index]->playerRace == RACE_AUTOMATON) {
			female_button->setIcon("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonFAutoOn_00.png");
			female_button->setBackground("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonFAuto_00.png");
			female_button->setBackgroundHighlighted("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonFAutoHigh_00.png");
			female_button->setBackgroundActivated("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonFAutoPress_00.png");
		} else {
			female_button->setIcon("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonFemaleOn_00.png");
			female_button->setBackground("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonFemale_00.png");
			female_button->setBackgroundHighlighted("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonFemaleHigh_00.png");
			female_button->setBackgroundActivated("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonFemalePress_00.png");
		}
		female_button->setSize(SDL_Rect{62, 0, 58, 52});
		female_button->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
		female_button->addWidgetAction("MenuStart", "ready");
		female_button->addWidgetAction("MenuPageRightAlt", "chat");
		female_button->addWidgetAction("MenuPageLeftAlt", "privacy");
		female_button->setWidgetBack("back_button");
		female_button->setWidgetLeft("male");
		female_button->setWidgetRight("race");
		female_button->setWidgetUp("game_settings");
		female_button->setWidgetDown("class");
		female_button->setCallback([](Button& button){soundActivate(); female_button_fn(button, button.getOwner());});
		female_button->setTickCallback([](Widget& widget){
			const int index = widget.getOwner();
			auto button = static_cast<Button*>(&widget); assert(button);
			if (stats[index]->playerRace == RACE_AUTOMATON) {
				button->setIcon("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonFAutoOn_00.png");
				button->setBackground("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonFAuto_00.png");
				button->setBackgroundHighlighted("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonFAutoHigh_00.png");
				button->setBackgroundActivated("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonFAutoPress_00.png");
			} else {
				button->setIcon("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonFemaleOn_00.png");
				button->setBackground("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonFemale_00.png");
				button->setBackgroundHighlighted("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonFemaleHigh_00.png");
				button->setBackgroundActivated("*images/ui/Main Menus/Play/PlayerCreation/RaceSelection/UI_RaceSelection_ButtonFemalePress_00.png");
			}
			});

		auto race_button = card->addButton("race");
		race_button->setColor(makeColor(255, 255, 255, 255));
		race_button->setHighlightColor(makeColor(255, 255, 255, 255));
		race_button->setSize(SDL_Rect{166, 166, 108, 52});
		switch (stats[index]->playerRace) {
		case RACE_HUMAN: race_button->setText("Human"); break;
		case RACE_SKELETON: race_button->setText("Skeleton"); break;
		case RACE_VAMPIRE: race_button->setText("Vampire"); break;
		case RACE_SUCCUBUS: race_button->setText("Succubus"); break;
		case RACE_GOATMAN: race_button->setText("Goatman"); break;
		case RACE_AUTOMATON: race_button->setText("Automaton"); break;
		case RACE_INCUBUS: race_button->setText("Incubus"); break;
		case RACE_GOBLIN: race_button->setText("Goblin"); break;
		case RACE_INSECTOID: race_button->setText("Insectoid"); break;
		case RACE_RAT: race_button->setText("Rat"); break;
		case RACE_TROLL: race_button->setText("Troll"); break;
		case RACE_SPIDER: race_button->setText("Spider"); break;
		case RACE_IMP: race_button->setText("Imp"); break;
		}
		race_button->setFont(smallfont_outline);
		race_button->setBackground("*images/ui/Main Menus/Play/PlayerCreation/Finalize_Button_RaceBase_00.png");
		race_button->setBackgroundHighlighted("*images/ui/Main Menus/Play/PlayerCreation/Finalize_Button_RaceBaseHigh_00.png");
		race_button->setBackgroundActivated("*images/ui/Main Menus/Play/PlayerCreation/Finalize_Button_RaceBasePress_00.png");
		race_button->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
		race_button->addWidgetAction("MenuStart", "ready");
		race_button->addWidgetAction("MenuPageRightAlt", "chat");
		race_button->addWidgetAction("MenuPageLeftAlt", "privacy");
		race_button->setWidgetBack("back_button");
		race_button->setWidgetLeft("female");
		race_button->setWidgetUp("game_settings");
		race_button->setWidgetDown("class");
        race_button->setCallback([](Button& button){
            soundActivate();
            const int index = button.getOwner();
            old_classes[index] = client_classes[index];
            old_appearances[index] = stats[index]->appearance;
            old_races[index] = stats[index]->playerRace;
            old_sexes[index] = stats[index]->sex;
            characterCardRaceMenu(button.getOwner(), false, -1);
            });

		static auto randomize_class_fn = [](Button& button, int index){
			soundActivate();

			auto card = static_cast<Frame*>(button.getParent());

			// select a random sex
			stats[index]->sex = (sex_t)(RNG.getU8() % 2);

			// select a random race
			// there are 9 legal races that the player can select from the start.
			if (enabledDLCPack1 && enabledDLCPack2) {
			    stats[index]->playerRace = RNG.uniform(0, NUMPLAYABLERACES - 1);
			} else if (enabledDLCPack1) {
			    stats[index]->playerRace = RNG.uniform(0, 4);
			} else if (enabledDLCPack2) {
			    stats[index]->playerRace = RNG.uniform(0, 4);
			    if (stats[index]->playerRace > 0) {
			        stats[index]->playerRace += 4;
			    }
			} else {
			    stats[index]->playerRace = RACE_HUMAN;
			}
			auto race_button = card->findButton("race");
			if (race_button) {
				switch (stats[index]->playerRace) {
				case RACE_HUMAN: race_button->setText("Human"); break;
				case RACE_SKELETON: race_button->setText("Skeleton"); break;
				case RACE_VAMPIRE: race_button->setText("Vampire"); break;
				case RACE_SUCCUBUS: stats[index]->sex = FEMALE; race_button->setText("Succubus"); break;
				case RACE_GOATMAN: race_button->setText("Goatman"); break;
				case RACE_AUTOMATON: race_button->setText("Automaton"); break;
				case RACE_INCUBUS: stats[index]->sex = MALE; race_button->setText("Incubus"); break;
				case RACE_GOBLIN: race_button->setText("Goblin"); break;
				case RACE_INSECTOID: race_button->setText("Insectoid"); break;
				case RACE_RAT: race_button->setText("Rat"); break;
				case RACE_TROLL: race_button->setText("Troll"); break;
				case RACE_SPIDER: race_button->setText("Spider"); break;
				case RACE_IMP: race_button->setText("Imp"); break;
				}
			}

			// choose a random appearance
			const int appearance_choice = RNG.uniform(0, NUMAPPEARANCES - 1);
			if (stats[index]->playerRace == RACE_HUMAN) {
				stats[index]->appearance = appearance_choice;
			} else {
				stats[index]->appearance = 0;
			}

			// update sex buttons after race selection:
			// we might have chosen a succubus or incubus
			auto bottom = card->findFrame("bottom");
			if (bottom) {
			    auto male_button = bottom->findButton("male");
			    auto female_button = bottom->findButton("female");
			    if (male_button && female_button) {
					male_button->setPressed(stats[index]->sex == MALE);
					male_button->setColor(stats[index]->sex == MALE ? makeColorRGB(255, 255, 255) : makeColorRGB(127, 127, 127));
					male_button->setHighlightColor(stats[index]->sex == MALE ? makeColorRGB(255, 255, 255) : makeColorRGB(127, 127, 127));
					female_button->setPressed(stats[index]->sex == FEMALE);
					female_button->setColor(stats[index]->sex == FEMALE ? makeColorRGB(255, 255, 255) : makeColorRGB(127, 127, 127));
					female_button->setHighlightColor(stats[index]->sex == FEMALE ? makeColorRGB(255, 255, 255) : makeColorRGB(127, 127, 127));
			    }
			}

			// select a random class
			const auto reduced_class_list = reducedClassList(index);
			const auto class_choice = RNG.uniform(0, (int)reduced_class_list.size() - 1);
			const auto random_class = reduced_class_list[class_choice];
			for (int c = 0; c < num_classes; ++c) {
				if (strcmp(random_class, classes_in_order[c]) == 0) {
					client_classes[index] = c;
					break;
				}
			}

			stats[index]->clearStats();
			initClass(index);
			sendPlayerOverNet();
			saveLastCharacter(index, multiplayer);
		};

		auto randomize_class = card->addButton("randomize_class");
		randomize_class->setColor(makeColor(255, 255, 255, 255));
		randomize_class->setHighlightColor(makeColor(255, 255, 255, 255));
		randomize_class->setBackground("*images/ui/Main Menus/Play/PlayerCreation/Finalize_Icon_Randomize_00.png");
		randomize_class->setBackgroundHighlighted("*images/ui/Main Menus/Play/PlayerCreation/Finalize_Icon_RandomizeHigh_00.png");
		randomize_class->setBackgroundActivated("*images/ui/Main Menus/Play/PlayerCreation/Finalize_Icon_RandomizePress_00.png");
		randomize_class->setSize(SDL_Rect{236, 226, 54, 54});
		randomize_class->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
		randomize_class->addWidgetAction("MenuStart", "ready");
		randomize_class->addWidgetAction("MenuPageRightAlt", "chat");
		randomize_class->addWidgetAction("MenuPageLeftAlt", "privacy");
		randomize_class->setWidgetBack("back_button");
		randomize_class->setWidgetLeft("class");
		randomize_class->setWidgetDown("ready");
		randomize_class->setWidgetUp("race");
		randomize_class->setCallback([](Button& button){randomize_class_fn(button, button.getOwner());});

		auto class_text = card->addField("class_text", 64);
		class_text->setSize(SDL_Rect{96, 236, 138, 32});
		static auto class_text_fn = [](Field& field, int index){
			int i = std::min(std::max(0, client_classes[index]), num_classes - 1);
			auto find = classes.find(classes_in_order[i]);
			if (find != classes.end()) {
				field.setText(find->second.name);
			}
		};
		class_text->setFont(smallfont_outline);
		class_text->setJustify(Field::justify_t::CENTER);
		class_text->setTickCallback([](Widget& widget){class_text_fn(*static_cast<Field*>(&widget), widget.getOwner());});
		(*class_text->getTickCallback())(*class_text);

		static auto class_button_tick_fn = [](Button& button, int index) {
			int i = std::min(std::max(0, client_classes[index]), num_classes - 1);
			auto find = classes.find(classes_in_order[i]);
			if (find != classes.end()) {
				auto& class_info = find->second;
				switch (class_info.dlc) {
				case DLC::Base:
					button.setBackground("*images/ui/Main Menus/Play/PlayerCreation/ClassSelection/ClassSelect_IconBGBase_00.png");
					button.setBackgroundHighlighted("*images/ui/Main Menus/Play/PlayerCreation/ClassSelection/ClassSelect_IconBGBaseHigh_00.png");
					button.setBackgroundActivated("*images/ui/Main Menus/Play/PlayerCreation/ClassSelection/ClassSelect_IconBGBasePress_00.png");
					break;
				case DLC::MythsAndOutcasts:
					button.setBackground("*images/ui/Main Menus/Play/PlayerCreation/ClassSelection/ClassSelect_IconBGMyths_00.png");
					button.setBackgroundHighlighted("*images/ui/Main Menus/Play/PlayerCreation/ClassSelection/ClassSelect_IconBGMythsHigh_00.png");
					button.setBackgroundActivated("*images/ui/Main Menus/Play/PlayerCreation/ClassSelection/ClassSelect_IconBGMythsPress_00.png");
					break;
				case DLC::LegendsAndPariahs:
					button.setBackground("*images/ui/Main Menus/Play/PlayerCreation/ClassSelection/ClassSelect_IconBGLegends_00.png");
					button.setBackgroundHighlighted("*images/ui/Main Menus/Play/PlayerCreation/ClassSelection/ClassSelect_IconBGLegendsHigh_00.png");
					button.setBackgroundActivated("*images/ui/Main Menus/Play/PlayerCreation/ClassSelection/ClassSelect_IconBGLegendsPress_00.png");
					break;
				}
				button.setIcon((std::string("*images/ui/Main Menus/Play/PlayerCreation/ClassSelection/") + find->second.image_highlighted).c_str());
			}
		};

		static auto class_button_fn = [](int index){
			soundActivate();
			if (inputs.hasController(index)) {
                const char* msg;
                const auto type = Input::getControllerType(index);
                switch (type) {
                default:
                case Input::ControllerType::Xbox:
                    msg = "*** Press [LB] to suggest a class for your party ***";
                    break;
                case Input::ControllerType::NintendoSwitch:
                    msg = "*** Press [L] to suggest a class for your party ***";
                    break;
                case Input::ControllerType::PlayStation:
                    msg = "*** Press [L1] to suggest a class for your party ***";
                    break;
                }
                addLobbyChatMessage(uint32ColorBaronyBlue, msg);
			}
            old_classes[index] = client_classes[index];
			characterCardClassMenu(index, false, 0);
		};

		auto class_button = card->addButton("class");
		class_button->setColor(makeColor(255, 255, 255, 255));
		class_button->setHighlightColor(makeColor(255, 255, 255, 255));
		class_button->setSize(SDL_Rect{46, 226, 52, 52});
		class_button->setBorder(0);
		class_button->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
		class_button->addWidgetAction("MenuStart", "ready");
		class_button->addWidgetAction("MenuPageRightAlt", "chat");
		class_button->addWidgetAction("MenuPageLeftAlt", "privacy");
		class_button->setWidgetBack("back_button");
		class_button->setWidgetRight("randomize_class");
		class_button->setWidgetUp("male");
		class_button->setWidgetDown("ready");
		class_button->setTickCallback([](Widget& widget){class_button_tick_fn(*static_cast<Button*>(&widget), widget.getOwner());});
		class_button->setCallback([](Button& button){class_button_fn(button.getOwner());});
		(*class_button->getTickCallback())(*class_button);

		static auto ready_button_fn = [](Button& button, int index) {
			soundActivate();
			createReadyStone(index, true, true);
		};

		auto ready_button = card->addButton("ready");
		ready_button->setSize(SDL_Rect{62, 288, 202, 52});
		ready_button->setColor(makeColor(255, 255, 255, 255));
		ready_button->setHighlightColor(makeColor(255, 255, 255, 255));
		ready_button->setBackground("*images/ui/Main Menus/Play/PlayerCreation/Finalize_Button_ReadyBase_00.png");
		ready_button->setBackgroundHighlighted("*images/ui/Main Menus/Play/PlayerCreation/Finalize_Button_ReadyBaseHigh_00.png");
		ready_button->setBackgroundActivated("*images/ui/Main Menus/Play/PlayerCreation/Finalize_Button_ReadyBasePress_00.png");
		ready_button->setFont(bigfont_outline);
		ready_button->setText("Ready");
		ready_button->setWidgetSearchParent(((std::string("card") + std::to_string(index)).c_str()));
		ready_button->addWidgetAction("MenuPageRightAlt", "chat");
		ready_button->addWidgetAction("MenuPageLeftAlt", "privacy");
		ready_button->setWidgetBack("back_button");
		ready_button->setWidgetUp("class");
		ready_button->setCallback([](Button& button){ready_button_fn(button, button.getOwner());});
		ready_button->setTickCallback([](Widget& widget){
			if (!main_menu_frame) {
				return;
			}
			if (!main_menu_frame->findSelectedWidget(widget.getOwner())) {
				widget.select();
			}
			});
		ready_button->select();
	}

	static void createLockedStone(int index) {
	    if (multiplayer == SERVER) {
	        newPlayer[index] = true;
	    }

		auto lobby = main_menu_frame->findFrame("lobby");
        if (!lobby) {
            return;
        }

		auto card = lobby->findFrame((std::string("card") + std::to_string(index)).c_str());
		if (card) {
			card->removeSelf();
		}

        const int pos = (Frame::virtualScreenX / 8) * (index * 2 + 1);
		card = lobby->addFrame((std::string("card") + std::to_string(index)).c_str());
		card->setSize(SDL_Rect{pos - 280 / 2, Frame::virtualScreenY - 146 - 100, 280, 146});
		card->setActualSize(SDL_Rect{0, 0, card->getSize().w, card->getSize().h});
		card->setColor(0);
		card->setBorder(0);

		auto backdrop = card->addImage(
			card->getActualSize(),
			0xffffffff,
			"*images/ui/Main Menus/Play/PlayerCreation/UI_Invite_Window00.png",
			"backdrop"
		);

		auto banner = card->addField("banner", 64);
		banner->setText("LOCKED");
		banner->setFont(banner_font);
		banner->setSize(SDL_Rect{(card->getSize().w - 200) / 2, 30, 200, 100});
		banner->setVJustify(Field::justify_t::TOP);
		banner->setHJustify(Field::justify_t::CENTER);
		banner->setColor(uint32ColorPlayerX);

		auto text = card->addField("text", 128);
		text->setText("New players\ncannot join");
		text->setFont(smallfont_outline);
		text->setSize(SDL_Rect{(card->getSize().w - 200) / 2, card->getSize().h / 2, 200, 50});
		text->setVJustify(Field::justify_t::TOP);
		text->setHJustify(Field::justify_t::CENTER);
		text->setColor(uint32ColorPlayerX);
	}

	static inline int countControllers() {
        int num_controllers = 0;
        for (auto& controller : game_controllers) {
			if (controller.isActive()) {
			    ++num_controllers;
			}
		}
        return num_controllers;
	}

	static inline int countUnassignedControllers() {
	    int num_controllers = countControllers();
        for (int c = 0; c < MAXPLAYERS; ++c) {
            if (inputs.hasController(c)) {
                --num_controllers;
            }
        }
        return num_controllers;
	}

	static bool isControllerAvailable(int player, int num_controllers) {
        for (int c = 0; c < player; ++c) {
            if (!isPlayerSignedIn(c) && !playerSlotsLocked[c]) {
                return false;
            }
        }
        if (num_controllers) {
            return true;
        } else {
            return false;
        }
	}

	static void createStartButton(int index) {
	    if (playerSlotsLocked[index]) {
	        createLockedStone(index);
	        return;
	    }

		auto lobby = main_menu_frame->findFrame("lobby");
        if (!lobby) {
            return;
        }

		sendReadyOverNet(index, false);
		auto countdown = lobby->findFrame("countdown");
		if (countdown) {
		    countdown->removeSelf();
		}

#ifndef NINTENDO
		// release any controller assigned to this player
        if (inputs.hasController(index)) {
            inputs.removeControllerWithDeviceID(inputs.getControllerID(index));
			for (int c = 0; c < MAX_SPLITSCREEN; ++c) {
            	Input::inputs[c].refresh();
			}
        }
		if (currentLobbyType != LobbyType::LobbyLocal) {
		    inputs.setPlayerIDAllowedKeyboard(clientnum);
		}
#endif

		auto card = lobby->findFrame((std::string("card") + std::to_string(index)).c_str());
		if (card) {
			card->removeSelf();
		}

        const int pos = (Frame::virtualScreenX / 8) * (index * 2 + 1);
		card = lobby->addFrame((std::string("card") + std::to_string(index)).c_str());
		card->setSize(SDL_Rect{pos - 280 / 2, Frame::virtualScreenY - 146 - 100, 280, 146});
		card->setActualSize(SDL_Rect{0, 0, card->getSize().w, card->getSize().h});
		card->setColor(0);
		card->setBorder(0);
		card->setOwner(index);

		auto backdrop = card->addImage(
			card->getActualSize(),
			0xffffffff,
			"*images/ui/Main Menus/Play/PlayerCreation/UI_Invite_Window00.png",
			"backdrop"
		);

		auto banner = card->addField("invite_banner", 64);
		banner->setText((std::string("PLAYER ") + std::to_string(index + 1)).c_str());
		banner->setFont(banner_font);
		banner->setSize(SDL_Rect{(card->getSize().w - 200) / 2, 30, 200, 100});
		banner->setVJustify(Field::justify_t::TOP);
		banner->setHJustify(Field::justify_t::CENTER);
		banner->setColor(playerColor(index, colorblind_lobby, false));
		banner->setUserData((void*)(intptr_t)index);
		banner->setTickCallback([](Widget& widget) {
			auto field = static_cast<Field*>(&widget);
			auto index = reinterpret_cast<intptr_t>(field->getUserData());
			field->setColor(playerColor((int)index, colorblind_lobby, false));
		});

		auto start = card->addField("start", 128);
		start->setFont(smallfont_outline);
		start->setSize(SDL_Rect{(card->getSize().w - 200) / 2, card->getSize().h / 2, 200, 50});
		start->setVJustify(Field::justify_t::TOP);
		start->setHJustify(Field::justify_t::CENTER);
		start->setHideGlyphs(true);
		start->setHideSelectors(true);
		start->setWidgetBack("back_button");
		start->setTickCallback([](Widget& widget){
		    const int player = widget.getOwner();
			
			assert(main_menu_frame);
			if (!main_menu_frame->findSelectedWidget(widget.getOwner())) {
				widget.select();
			}

            // determine whether I should own the keyboard
#ifndef NINTENDO
			if (currentLobbyType == LobbyType::LobbyLocal) {
            	if (inputs.getPlayerIDAllowedKeyboard() != player) {
		            bool shouldOwnKeyboard = true;
		            if (isPlayerSignedIn(inputs.getPlayerIDAllowedKeyboard())) {
		                shouldOwnKeyboard = false;
		            } else {
		                for (int c = 0; c < player; ++c) {
		                    if (isPlayerSignedIn(c)) {
		                        if (inputs.getPlayerIDAllowedKeyboard() == c) {
		                            shouldOwnKeyboard = false;
		                            break;
		                        }
		                    } else if (!playerSlotsLocked[c]) {
	                            shouldOwnKeyboard = false;
	                            break;
		                    }
		                }
		            }
		            if (shouldOwnKeyboard) {
		                inputs.setPlayerIDAllowedKeyboard(player);
		            }
		        }
		    }
#endif

            // set field text
		    auto field = static_cast<Field*>(&widget);
		    if (inputs.getPlayerIDAllowedKeyboard() == player ||
		        inputs.hasController(player) || multiplayer != SINGLE) {
		        field->setText("Press to Start");
		    } else {
		        const int num_controllers = countUnassignedControllers();
		        if (isControllerAvailable(player, num_controllers)) {
		            field->setText("Press to Start");
		        } else {
		            if (num_controllers < player) {
		                field->setText("Connect Controller");
		            } else {
		                field->setText("Please Wait");
		            }
		        }
		    }

		    auto& input = Input::inputs[player];

		    // handle B to go back
		    bool no_one_logged_in = true;
		    if (currentLobbyType == LobbyType::LobbyLocal) {
		        for (int c = 0; c < MAXPLAYERS; ++c) {
		            if (isPlayerSignedIn(c)) {
		                no_one_logged_in = false;
		                break;
		            }
		        }
		    } else if (isPlayerSignedIn(clientnum)) {
		        no_one_logged_in = false;
		    }
		    if (no_one_logged_in) {
                assert(main_menu_frame);
		        if (player == clientnum &&
		            input.binary("MenuCancel") &&
		            !inputstr &&
		            !main_menu_frame->findSelectedWidget(player)) {
	                auto lobby = main_menu_frame->findFrame("lobby"); assert(lobby);
		            auto back = lobby->findFrame("back"); assert(back);
		            auto back_button = back->findButton("back_button"); assert(back_button);
		            back_button->select();
		        }
		    }

		    // handle login button presses
		    if (input.consumeBinaryToggle("GamepadLoginB") ||
		        input.consumeBinaryToggle("GamepadLoginA") ||
		        input.consumeBinaryToggle("GamepadLoginStart")) {
		        if (input.binary("GamepadLoginB")) {
	                bool nobodySignedIn = true;
		            if (multiplayer == SINGLE) {
		                for (int c = 0; c < MAXPLAYERS; ++c) {
		                    if (isPlayerSignedIn(c)) {
		                        nobodySignedIn = false;
		                        break;
		                    }
		                }
		            }
	                if (nobodySignedIn) {
	                    // if no one is signed in, B doesn't sign in... it leaves the lobby!
	                    return;
	                }
		            input.consumeBindingsSharedWithBinding("GamepadLoginB");
		        }
		        if (input.binary("GamepadLoginA")) {
		            input.consumeBindingsSharedWithBinding("GamepadLoginA");
		        }
		        if (input.binary("GamepadLoginStart")) {
		            input.consumeBindingsSharedWithBinding("GamepadLoginStart");
		        }
		        // let the next empty player slot login with the keyboard
#ifndef NINTENDO
		        if (multiplayer == SINGLE && !isPlayerSignedIn(inputs.getPlayerIDAllowedKeyboard())) {
		            for (int c = 0; c < MAXPLAYERS; ++c) {
		                if (c == player) {
		                    // skip ourselves
		                    continue;
		                }
		                if (!isPlayerSignedIn(c) && !playerSlotsLocked[c]) {
		                    inputs.setPlayerIDAllowedKeyboard(c);
		                    break;
		                }
		            }
		        }
#endif
                if (loadingsavegame) {
                    createReadyStone(player, true, true);
                } else {
                    createCharacterCard(player);
                }
		        return;
		    }
		    if (!inputstr && input.consumeBinaryToggle("KeyboardLogin")) {
				if (inputs.getPlayerIDAllowedKeyboard() == player || multiplayer != SINGLE) {
		        	input.consumeBindingsSharedWithBinding("KeyboardLogin");
#ifndef NINTENDO
					// release any controller assigned to this player
					if (inputs.hasController(player)) {
						inputs.removeControllerWithDeviceID(inputs.getControllerID(player));
						for (int c = 0; c < MAX_SPLITSCREEN; ++c) {
							Input::inputs[c].refresh();
						}
					}
#endif
					if (loadingsavegame) {
						createReadyStone(player, true, true);
					} else {
						createCharacterCard(player);
					}
					return;
				}
		    }
		    });
		start->setDrawCallback([](const Widget& widget, SDL_Rect pos){
		    const int player = widget.getOwner();
			const bool keyboardAvailable = inputs.getPlayerIDAllowedKeyboard() == player ||
				currentLobbyType != LobbyType::LobbyLocal;
		    const bool controllerAvailable = inputs.hasController(player) ||
				(currentLobbyType != LobbyType::LobbyLocal && countControllers()) ||
				isControllerAvailable(player, countUnassignedControllers());
		    const bool pressed = ticks % TICKS_PER_SECOND >= TICKS_PER_SECOND / 2;
		    const SDL_Rect viewport{0, 0, Frame::virtualScreenX, Frame::virtualScreenY};
#ifdef NINTENDO
			// draw A button
			std::string path = Input::getGlyphPathForInput("ButtonA", pressed,
                Input::ControllerType::NintendoSwitch);
			auto image = Image::get((std::string("*") + path).c_str());
			const int x = pos.x + pos.w / 2;
			const int y = pos.y + pos.h / 2 + 16;
			const int w = image->getWidth();
			const int h = image->getHeight();
			image->draw(nullptr, SDL_Rect{ x - w / 2, y - h / 2, w, h }, viewport);
#else
            if (keyboardAvailable) {
                if (controllerAvailable) {
                    // draw spacebar and A button
                    {
                        std::string path = Input::getGlyphPathForInput("Space", pressed);
		                auto image = Image::get((std::string("*") + path).c_str());
		                const int x = pos.x + pos.w / 2 - 32;
		                const int y = pos.y + pos.h / 2 + 16;
		                const int w = image->getWidth();
		                const int h = image->getHeight();
		                image->draw(nullptr, SDL_Rect{x - w / 2, y - h / 2, w, h}, viewport);
                    }
                    {
                        std::string path = Input::getGlyphPathForInput("ButtonA", pressed, Input::getControllerType(player));
		                auto image = Image::get((std::string("*") + path).c_str());
		                const int x = pos.x + pos.w / 2 + 32;
		                const int y = pos.y + pos.h / 2 + 16;
		                const int w = image->getWidth();
		                const int h = image->getHeight();
		                image->draw(nullptr, SDL_Rect{x - w / 2, y - h / 2, w, h}, viewport);
                    }
                } else {
                    // only draw spacebar
                    std::string path = Input::getGlyphPathForInput("Space", pressed);
		            auto image = Image::get((std::string("*") + path).c_str());
		            const int x = pos.x + pos.w / 2;
		            const int y = pos.y + pos.h / 2 + 16;
		            const int w = image->getWidth();
		            const int h = image->getHeight();
		            image->draw(nullptr, SDL_Rect{x - w / 2, y - h / 2, w, h}, viewport);
                }
            } else if (controllerAvailable) {
                // draw A button
                std::string path = Input::getGlyphPathForInput("ButtonA", pressed, Input::getControllerType(player));
                auto image = Image::get((std::string("*") + path).c_str());
                const int x = pos.x + pos.w / 2;
                const int y = pos.y + pos.h / 2 + 16;
                const int w = image->getWidth();
                const int h = image->getHeight();
                image->draw(nullptr, SDL_Rect{x - w / 2, y - h / 2, w, h}, viewport);
            }
#endif
		    });
	}

	static void createInviteButton(int index) {
	    if (playerSlotsLocked[index]) {
	        createLockedStone(index);
	        return;
	    }
		if (LobbyHandler.getHostingType() != LobbyHandler_t::LobbyServiceType::LOBBY_STEAM) {
			createWaitingStone(index);
			return;
		}

	    if (multiplayer == SERVER) {
	        newPlayer[index] = true;
	    }

		auto lobby = main_menu_frame->findFrame("lobby");
        if (!lobby) {
            return;
        }

		auto card = lobby->findFrame((std::string("card") + std::to_string(index)).c_str());
		if (card) {
			card->removeSelf();
		}

        const int pos = (Frame::virtualScreenX / 8) * (index * 2 + 1);
		card = lobby->addFrame((std::string("card") + std::to_string(index)).c_str());
		card->setSize(SDL_Rect{pos - 280 / 2, Frame::virtualScreenY - 146 - 100, 280, 146});
		card->setActualSize(SDL_Rect{0, 0, card->getSize().w, card->getSize().h});
		card->setColor(0);
		card->setBorder(0);

		auto backdrop = card->addImage(
			card->getActualSize(),
			0xffffffff,
			"*images/ui/Main Menus/Play/PlayerCreation/UI_Invite_Window00.png",
			"backdrop"
		);

		auto banner = card->addField("invite_banner", 64);
		banner->setText("INVITE");
		banner->setFont(banner_font);
		banner->setSize(SDL_Rect{(card->getSize().w - 200) / 2, 30, 200, 100});
		banner->setVJustify(Field::justify_t::TOP);
		banner->setHJustify(Field::justify_t::CENTER);
		banner->setColor(playerColor(index, colorblind_lobby, false));
		banner->setUserData((void*)(intptr_t)index);
		banner->setTickCallback([](Widget& widget) {
			auto field = static_cast<Field*>(&widget);
			auto index = reinterpret_cast<intptr_t>(field->getUserData());
			field->setColor(playerColor((int)index, colorblind_lobby, false));
		});

		auto invite = card->addButton("invite_button");
		invite->setText("Click to Invite");
		invite->setFont(smallfont_outline);
		invite->setSize(SDL_Rect{(card->getSize().w - 200) / 2, card->getSize().h / 2, 200, 16});
		invite->setVJustify(Button::justify_t::TOP);
		invite->setHJustify(Button::justify_t::CENTER);
		invite->setBorder(0);
		invite->setColor(0);
		invite->setBorderColor(0);
		invite->setHighlightColor(0);
		invite->setCallback([](Button&){buttonInviteFriends(NULL);});
	}

	static void createWaitingStone(int index) {
	    if (playerSlotsLocked[index]) {
	        createLockedStone(index);
	        return;
	    }

		if (multiplayer == SERVER) {
			newPlayer[index] = true;
		}

		auto lobby = main_menu_frame->findFrame("lobby");
        if (!lobby) {
            return;
        }

		auto card = lobby->findFrame((std::string("card") + std::to_string(index)).c_str());
		if (card) {
			card->removeSelf();
		}

        const int pos = (Frame::virtualScreenX / 8) * (index * 2 + 1);
		card = lobby->addFrame((std::string("card") + std::to_string(index)).c_str());
		card->setSize(SDL_Rect{pos - 280 / 2, Frame::virtualScreenY - 146 - 100, 280, 146});
		card->setActualSize(SDL_Rect{0, 0, card->getSize().w, card->getSize().h});
		card->setColor(0);
		card->setBorder(0);

		auto backdrop = card->addImage(
			card->getActualSize(),
			0xffffffff,
			"*images/ui/Main Menus/Play/PlayerCreation/UI_Invite_Window00.png",
			"backdrop"
		);

		auto banner = card->addField("banner", 64);
		banner->setText("OPEN");
		banner->setFont(banner_font);
		banner->setSize(SDL_Rect{(card->getSize().w - 200) / 2, 30, 200, 100});
		banner->setVJustify(Field::justify_t::TOP);
		banner->setHJustify(Field::justify_t::CENTER);
		banner->setColor(playerColor(index, colorblind_lobby, false));
		banner->setUserData((void*)(intptr_t)index);
		banner->setTickCallback([](Widget& widget) {
			auto field = static_cast<Field*>(&widget);
			auto index = reinterpret_cast<intptr_t>(field->getUserData());
			field->setColor(playerColor((int)index, colorblind_lobby, false));
		});

		auto text = card->addField("text", 128);
		text->setText("Waiting for\nplayer to join");
		text->setFont(smallfont_outline);
		text->setSize(SDL_Rect{(card->getSize().w - 200) / 2, card->getSize().h / 2, 200, 50});
		text->setVJustify(Field::justify_t::TOP);
		text->setHJustify(Field::justify_t::CENTER);
	}

	static void checkReadyStates() {
	    if (!main_menu_frame) {
	        return;
	    }
		auto lobby = main_menu_frame->findFrame("lobby");
		if (!lobby) {
		    return;
		}

        bool atLeastOnePlayer = false;
	    bool allReady = true;
		for (int c = 0; c < MAXPLAYERS; ++c) {
			auto card = lobby->findFrame((std::string("card") + std::to_string(c)).c_str());
            if (card) {
                auto backdrop = card->findImage("backdrop"); assert(backdrop);
                if (backdrop->path == "*images/ui/Main Menus/Play/PlayerCreation/UI_Invite_Window00.png") {
                    playersInLobby[c] = false;
                    if (multiplayer == SINGLE) {
                        if (loadingsavegame && !playerSlotsLocked[c]) {
                            allReady = false;
                        }
                    } else {
                        if (!client_disconnected[c]) {
                            // we know a player is connected to this slot, and they're not ready
                            allReady = false;
                        }
                    }
                } else if (backdrop->path == "*images/ui/Main Menus/Play/PlayerCreation/UI_Ready_Window00.png") {
                    playersInLobby[c] = true;
                    atLeastOnePlayer = true;
                } else {
                    playersInLobby[c] = true;
                    atLeastOnePlayer = true;
                    allReady = false;
                }
            }
		}
		if (allReady && atLeastOnePlayer) {
		    createCountdownTimer();
		} else {
	        auto countdown = lobby->findFrame("countdown");
	        if (countdown) {
	            countdown->removeSelf();
	        }
	        fadeout = false;
	        main_menu_fade_destination = FadeDestination::None;
		}
	}

	static void createReadyStone(int index, bool local, bool ready) {
	    if (!main_menu_frame || main_menu_frame->isToBeDeleted()) {
	        // maybe this could happen if we got a REDY packet
	        // super late or something.
	        if (!ready) {
	            fadeout = false;
	            main_menu_fade_destination = FadeDestination::None;
	        }
	        return;
	    }

		auto lobby = main_menu_frame->findFrame("lobby");
		if (!lobby || lobby->isToBeDeleted()) {
	        // maybe this could happen if we got a REDY packet
	        // super late or something.
	        if (!ready) {
	            fadeout = false;
	            main_menu_fade_destination = FadeDestination::None;
	        }
		    return;
		}

		auto card = lobby->findFrame((std::string("card") + std::to_string(index)).c_str());
		if (card) {
			card->removeSelf();
		}

        const int pos = (Frame::virtualScreenX / 8) * (index * 2 + 1);
		card = lobby->addFrame((std::string("card") + std::to_string(index)).c_str()); assert(card);
		card->setSize(SDL_Rect{pos - 280 / 2, Frame::virtualScreenY - 146 - 100, 280, 146});
		card->setActualSize(SDL_Rect{0, 0, card->getSize().w, card->getSize().h});
		card->setColor(0);
		card->setBorder(0);
		card->setOwner(index);

		auto backdrop = card->addImage(
			card->getActualSize(),
			0xffffffff,
			ready ?
			    "*images/ui/Main Menus/Play/PlayerCreation/UI_Ready_Window00.png":
			    "*images/ui/Main Menus/Play/PlayerCreation/UI_Invite_Window00.png",
			"backdrop"
		);

		// character name
		auto banner = card->addField("banner", 64); assert(banner);
		banner->setFont(banner_font);
		banner->setSize(SDL_Rect{(card->getSize().w - 260) / 2, 30, 260, 100});
		banner->setVJustify(Field::justify_t::TOP);
		banner->setHJustify(Field::justify_t::CENTER);
		banner->setColor(playerColor(index, colorblind_lobby, false));
		banner->setUserData((void*)(intptr_t)index);

		// character name needs to be updated constantly in case it gets updated over the net
		banner->setTickCallback([](Widget& widget){
		    const int player = widget.getOwner();
		    auto field = static_cast<Field*>(&widget);

            // shorten the name
            constexpr int longest_name = 22;
		    char shortname[32];
		    int len = (int)strlen(stats[player]->name);
		    if (len > longest_name) {
		        memcpy(shortname, stats[player]->name, longest_name);
		        memcpy(shortname + longest_name - 2, "...", 4);
		    } else {
		        strcpy(shortname, stats[player]->name);
		    }

			// set the name
			field->setText(shortname);

			// set color
			auto index = reinterpret_cast<intptr_t>(field->getUserData());
			field->setColor(playerColor((int)index, colorblind_lobby, false));
		    });

		// account name
		auto account = card->addField("account", 64); assert(account);
		account->setFont(smallfont_outline);
		account->setSize(SDL_Rect{ (card->getSize().w - 260) / 2, 54, 260, 76 });
		account->setVJustify(Field::justify_t::TOP);
		account->setHJustify(Field::justify_t::CENTER);
		account->setColor(playerColor(index, colorblind_lobby, false));
		account->setUserData((void*)(intptr_t)index);

		// account name needs to be updated constantly in case it gets updated over the net
		account->setTickCallback([](Widget& widget) {
			const int player = widget.getOwner();
			auto field = static_cast<Field*>(&widget);

			// set name
			char buf[64];
			snprintf(buf, sizeof(buf), "(%s)", players[player]->getAccountName());
			field->setText(buf);

			// announce new player
			if (multiplayer == SERVER) {
				if (!client_disconnected[player] && newPlayer[player] && stringCmp(players[player]->getAccountName(), "...", 3, 3)) {
					newPlayer[player] = false;

					char buf[1024];
					int len = snprintf(buf, sizeof(buf), "*** %s has joined the game ***", players[player]->getAccountName());
					if (len > 0) {
						sendChatMessageOverNet(uint32ColorBaronyBlue, buf, len);
					}
				}
			}

			auto index = reinterpret_cast<intptr_t>(field->getUserData());
			field->setColor(playerColor((int)index, colorblind_lobby, false));
			});

        if (local) {
            static auto cancel_fn = [](int index){
                if (main_menu_fade_destination == FadeDestination::GameStart ||
                    main_menu_fade_destination == FadeDestination::GameStartDummy) {
                    // fix being able to back out on the last frame
                    return;
                }
                if (loadingsavegame) {
                    createStartButton(index);
                } else {
                    createCharacterCard(index);
                }
                };

            static auto ready_fn = [](int index){
                if (main_menu_fade_destination == FadeDestination::GameStart ||
                    main_menu_fade_destination == FadeDestination::GameStartDummy) {
                    // fix being able to back out on the last frame
                    return;
                }
                createReadyStone(index, true, true);
                };

		    auto button = card->addButton("button"); assert(button);
		    if (ready) {
		        button->setText("Ready!");
		    } else {
		        button->setText("Not Ready");
		    }
		    button->setHideSelectors(true);
		    button->setFont(smallfont_outline);
		    button->setSize(SDL_Rect{(card->getSize().w - 200) / 2, card->getSize().h / 2, 200, 50});
		    button->setVJustify(Button::justify_t::TOP);
		    button->setHJustify(Button::justify_t::CENTER);
		    button->setBorder(0);
		    button->setColor(0);
		    button->setBorderColor(0);
		    button->setHighlightColor(0);
		    button->setWidgetBack("button");
		    button->setHideKeyboardGlyphs(false);
		    button->setWidgetSearchParent(card->getName());
		    button->addWidgetAction("MenuConfirm", "FraggleMaggleStiggleWortz"); // some garbage so that this glyph isn't auto-bound
		    if (ready) {
		        button->setCallback([](Button& button){cancel_fn(button.getOwner());});
		    } else {
		        button->setCallback([](Button& button){ready_fn(button.getOwner());}); 
		    }
			button->setTickCallback([](Widget& widget){
				// rescue focus
				assert(main_menu_frame);
				if (!main_menu_frame->findSelectedWidget(widget.getOwner())) {
					widget.select();
				}
				});
		    button->select();
		} else {
		    auto status = card->addField("status", 64); assert(status);
		    if (ready) {
		        status->setText("Ready!");
		    } else {
		        status->setText("Not Ready");
		    }
		    status->setFont(smallfont_outline);
		    status->setSize(SDL_Rect{(card->getSize().w - 200) / 2, card->getSize().h / 2, 200, 50});
		    status->setVJustify(Button::justify_t::TOP);
		    status->setHJustify(Button::justify_t::CENTER);
		}
        if (local) {
            sendReadyOverNet(index, ready);
        }

        // determine if all players are ready, and if so, create a countdown timer
		checkReadyStates();
	}

	static void startGame() {
	    if (multiplayer == CLIENT) {
	        if (!fadeout || main_menu_fade_destination != FadeDestination::GameStart) {
                beginFade(MainMenu::FadeDestination::GameStartDummy);
            }
	    } else {
            destroyMainMenu();
            createDummyMainMenu();
            beginFade(MainMenu::FadeDestination::GameStart);

			if (!intro && gameModeManager.currentMode == GameModeManager_t::GameModes::GAME_MODE_DEFAULT) {
				deleteSaveGame(multiplayer);
			}

	        // set unique game key
	        local_rng.seedTime();
	        local_rng.getSeed(&uniqueGameKey, sizeof(uniqueGameKey));
	        net_rng.seedBytes(&uniqueGameKey, sizeof(uniqueGameKey));

	        // send start signal to each player
	        if (multiplayer == SERVER) {
	            for (int c = 1; c < MAXPLAYERS; c++) {
		            if (client_disconnected[c]) {
			            continue;
		            }
		            if (intro) {
		                memcpy((char*)net_packet->data, "STRT", 4);
		            } else {
		                memcpy((char*)net_packet->data, "RSTR", 4);
		            }
		            SDLNet_Write32(svFlags, &net_packet->data[4]);
		            SDLNet_Write32(uniqueGameKey, &net_packet->data[8]);
		            net_packet->data[12] = loadingsavegame ? 1 : 0;
		            net_packet->address.host = net_clients[c - 1].host;
		            net_packet->address.port = net_clients[c - 1].port;
		            net_packet->len = 13;
		            sendPacketSafe(net_sock, -1, net_packet, c - 1);
	            }
	        }
	    }
	}

	static void createCountdownTimer() {
		static const char* timer_font = "fonts/pixelmix_bold.ttf#64#2";

		auto lobby = main_menu_frame->findFrame("lobby");
        if (!lobby) {
            return;
        }

		auto frame = lobby->addFrame("countdown");
		frame->setSize(SDL_Rect{(Frame::virtualScreenX - 300) / 2, 64, 300, 120});
		frame->setHollow(true);

        static Uint32 countdown_end;
        countdown_end = ticks + TICKS_PER_SECOND * 3;

		auto countdown = frame->addField("timer", 8);
		countdown->setHJustify(Field::justify_t::LEFT);
		countdown->setVJustify(Field::justify_t::TOP);
		countdown->setFont(timer_font);
		countdown->setSize(SDL_Rect{frame->getSize().w / 2 - 20, 0, frame->getSize().w, frame->getSize().h});
		countdown->setTickCallback([](Widget& widget){
		    auto countdown = static_cast<Field*>(&widget);
		    if (ticks >= countdown_end) {
		        startGame();
		    } else {
		        Uint32 fourth = TICKS_PER_SECOND / 4;

		        // 1
		        if (ticks >= countdown_end - fourth) {
		            countdown->setText("1...");
		        }
		        else if (ticks >= countdown_end - fourth * 2) {
		            countdown->setText("1..");
		        }
		        else if (ticks >= countdown_end - fourth * 3) {
		            countdown->setText("1.");
		        }
		        else if (ticks >= countdown_end - fourth * 4) {
		            countdown->setText("1");
		        }

		        // 2
		        else if (ticks >= countdown_end - fourth * 5) {
		            countdown->setText("2...");
		        }
		        else if (ticks >= countdown_end - fourth * 6) {
		            countdown->setText("2..");
		        }
		        else if (ticks >= countdown_end - fourth * 7) {
		            countdown->setText("2.");
		        }
		        else if (ticks >= countdown_end - fourth * 8) {
		            countdown->setText("2");
		        }

		        // 3
		        else if (ticks >= countdown_end - fourth * 9) {
		            countdown->setText("3...");
		        }
		        else if (ticks >= countdown_end - fourth * 10) {
		            countdown->setText("3..");
		        }
		        else if (ticks >= countdown_end - fourth * 11) {
		            countdown->setText("3.");
		        }
		        else {
		            countdown->setText("3");
		        }
		    }
		    });
		
		/*auto achievements = frame->addField("achievements", 256);
		achievements->setSize(SDL_Rect{0, frame->getSize().h - 32, frame->getSize().w, 32});
		achievements->setFont(smallfont_outline);
		achievements->setJustify(Field::justify_t::CENTER);
		achievements->setTickCallback([](Widget& widget){
			Field* achievements = static_cast<Field*>(&widget);
			if ((svFlags & SV_FLAG_CHEATS) ||
				(svFlags & SV_FLAG_LIFESAVING) ||
				gamemods_disableSteamAchievements) {
				achievements->setColor(makeColor(180, 37, 37, 255));
				achievements->setText("ACHIEVEMENTS DISABLED");
			} else {
				achievements->setColor(makeColor(37, 90, 255, 255));
				achievements->setText("ACHIEVEMENTS ENABLED");
			}
			});
		(*achievements->getTickCallback())(*achievements);*/
	}

	static void createLobby(LobbyType type) {
		destroyMainMenu();
		createDummyMainMenu();

#ifdef NINTENDO
		if (type != LobbyType::LobbyLocal) {
			nxDisableAutoSleep();
		}
#endif

		if (type == LobbyType::LobbyLocal) {
#ifdef NINTENDO
			if (!nxIsHandheldMode()) {
				nxAssignControllers(1, 4, true, true, false, false, nullptr);
			}
#else
			// just in case we're still awaiting a controller for player 1,
			// this clears it.
			Input::waitingToBindControllerForPlayer = -1;
#endif
		}

#ifndef NINTENDO
		// unassign all controllers when entering the lobby.
		// we do this because any number of slots can be locked
		// and we need to be able to assign any available controller
		for (int c = 0; c < MAXPLAYERS; ++c) {
			if (inputs.hasController(c)) {
				inputs.removeControllerWithDeviceID(inputs.getControllerID(c));
				for (int c = 0; c < MAX_SPLITSCREEN; ++c) {
					Input::inputs[c].refresh();
				}
			}
		}
#endif

		// reset ALL player stats
        if (!loadingsavegame) {
		    for (int c = 0; c < MAXPLAYERS; ++c) {
		        if (type != LobbyType::LobbyJoined && type != LobbyType::LobbyLocal && c != 0) {
		            newPlayer[c] = true;
		        }
		        if (type != LobbyType::LobbyJoined || c == clientnum) {
		            playerSlotsLocked[c] = false;

					bool replayedLastCharacter = false;
					if ( type == LobbyType::LobbyLAN )
					{
						// multiplayer/directConnect not initialised at this point for server,
						replayedLastCharacter = replayLastCharacter(c, DIRECTSERVER);
					}
					else if ( type == LobbyType::LobbyOnline )
					{
						// multiplayer not initialised at this point for server,
						// OK to pass in anything other than SINGLE
						replayedLastCharacter = replayLastCharacter(c, SERVER);
					}
					else if ( type == LobbyType::LobbyJoined )
					{
						if ( directConnect )
						{
							replayedLastCharacter = replayLastCharacter(c, DIRECTCLIENT);
						}
						else
						{
							replayedLastCharacter = replayLastCharacter(c, CLIENT);
						}
					}
					else if ( type == LobbyType::LobbyLocal )
					{
						replayedLastCharacter = replayLastCharacter(c, SINGLE);
					}
					if ( !replayedLastCharacter )
					{
						stats[c]->playerRace = RACE_HUMAN;
						stats[c]->sex = static_cast<sex_t>(RNG.getU8() % 2);
						stats[c]->appearance = RNG.uniform(0, NUMAPPEARANCES - 1);
						client_classes[c] = 0;

						stats[c]->clearStats();
						initClass(c);

						// random name
						auto& names = stats[c]->sex == sex_t::MALE ?
						    randomPlayerNamesMale : randomPlayerNamesFemale;
						const int choice = RNG.uniform(0, (int)names.size() - 1);
						auto name = names[choice].c_str();
						size_t len = names[choice].size();
						len = std::min(sizeof(Stat::name) - 1, len);
						memcpy(stats[c]->name, name, len);
						stats[c]->name[len] = '\0';
					}
			    }
			}
		}

		GameplayPreferences_t::reset();

        if (type == LobbyType::LobbyJoined) {
            sendPlayerOverNet();
			saveLastCharacter(clientnum, directConnect ? DIRECTCLIENT : CLIENT);
        }

		currentLobbyType = type;

        const int lobbySize = type == LobbyType::LobbyLocal ?
            (Frame::virtualScreenX / 4) * MAX_SPLITSCREEN:
            (Frame::virtualScreenX / 4) * MAXPLAYERS;
        
		auto lobby = main_menu_frame->addFrame("lobby");
		lobby->setOwner(clientnum);
		lobby->setSize(SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY});
		lobby->setActualSize(SDL_Rect{0, 0, lobbySize, lobby->getSize().h});
        lobby->setColor(makeColor(0, 0, 0, 127));
        lobby->setAllowScrollBinds(false);
		lobby->setBorder(0);
        
        static ConsoleVariable<float> cvar_lobbyScroll("/lobby_scroll", 10.f);
        
        auto scrollRight = lobby->addButton("scroll_right");
        scrollRight->setBackground("*#images/ui/Main Menus/Settings/Settings_Button_R00.png");
        scrollRight->setBackgroundActivated("*#images/ui/Main Menus/Settings/Settings_Button_RPress00.png");
        scrollRight->setBackgroundHighlighted("*#images/ui/Main Menus/Settings/Settings_Button_RHigh00.png");
        scrollRight->setTickCallback([](Widget& widget){
            auto button = static_cast<Button*>(&widget); assert(button);
            auto frame = static_cast<Frame*>(widget.getParent()); assert(frame);
            if (frame->getActualSize().w > frame->getSize().w) {
                button->setSize(SDL_Rect{frame->getActualSize().x + Frame::virtualScreenX - 38,
                    (Frame::virtualScreenY - 58) / 2, 38, 58});
                button->setInvisible(false);
            } else {
                button->setInvisible(true);
            }
            });
        scrollRight->setCallback([](Button& button){
            auto frame = static_cast<Frame*>(button.getParent());
            auto speed = *cvar_lobbyScroll * Frame::virtualScreenX;
            frame->setAccelerationX(speed);
            });
        
        auto scrollLeft = lobby->addButton("scroll_left");
        scrollLeft->setBackground("*#images/ui/Main Menus/Settings/Settings_Button_L00.png");
        scrollLeft->setBackgroundActivated("*#images/ui/Main Menus/Settings/Settings_Button_LPress00.png");
        scrollLeft->setBackgroundHighlighted("*#images/ui/Main Menus/Settings/Settings_Button_LHigh00.png");
        scrollLeft->setTickCallback([](Widget& widget){
            auto button = static_cast<Button*>(&widget); assert(button);
            auto frame = static_cast<Frame*>(widget.getParent()); assert(frame);
            if (frame->getActualSize().w > frame->getSize().w) {
                button->setSize(SDL_Rect{frame->getActualSize().x,
                    (Frame::virtualScreenY - 58) / 2, 38, 58});
                button->setInvisible(false);
            } else {
                button->setInvisible(true);
            }
            });
        scrollLeft->setCallback([](Button& button){
            auto frame = static_cast<Frame*>(button.getParent());
            auto speed = -*cvar_lobbyScroll * Frame::virtualScreenX;
            frame->setAccelerationX(speed);
            });

		for (int c = 0; c < MAXPLAYERS; ++c) {
			auto name = std::string("paperdoll") + std::to_string(c);
			auto paperdoll = lobby->addFrame(name.c_str());
			paperdoll->setOwner(c);
			//paperdoll->setColor(makeColor(33, 26, 24, 255));
			//paperdoll->setBorderColor(makeColor(116, 55, 0, 255));
			//paperdoll->setBorder(2);
            paperdoll->setHollow(true);
			paperdoll->setColor(0);
			paperdoll->setBorderColor(0);
			paperdoll->setBorder(0);
			paperdoll->setInvisible(true);
			paperdoll->setTickCallback([](Widget& widget){
				widget.setInvisible(true);
				int index = widget.getOwner();
				auto paperdoll = static_cast<Frame*>(&widget);
				auto lobby = static_cast<Frame*>(widget.getParent());
				auto card = lobby->findFrame((std::string("card") + std::to_string(index)).c_str());
				if (card) {
					paperdoll->setSize(SDL_Rect{
						index * Frame::virtualScreenX / 4,
						0,
						Frame::virtualScreenX / 4,
						Frame::virtualScreenY * 3 / 4
						});
					if (loadingsavegame) {
					    widget.setInvisible(playerSlotsLocked[index]);
					} else {
					    widget.setInvisible(!isPlayerSignedIn(index));
					}
				}
				});
			paperdoll->setDrawCallback([](const Widget& widget, SDL_Rect pos){
				auto angle = (330.0 + 20.0 * widget.getOwner()) * PI / 180.0;
                const int player = widget.getOwner();
                //const bool dark = isCharacterValidFromDLC(*stats[player],
                //    client_classes[player]) != VALID_OK_CHARACTER;
				drawCharacterPreview(widget.getOwner(), pos, 80, angle, false);
				});
		}

		auto banner = lobby->addFrame("banner");
        banner->setTickCallback([](Widget& widget){
            auto banner = static_cast<Frame*>(&widget); assert(banner);
            auto lobby = static_cast<Frame*>(widget.getParent()); assert(lobby);
            banner->setSize(SDL_Rect{lobby->getActualSize().x, 0, Frame::virtualScreenX, 66});
            });
        {
            auto background = banner->addImage(
                SDL_Rect{0, 0, Frame::virtualScreenX, 66},
                0xffffffff,
                "*#images/ui/Main Menus/Play/PlayerCreation/Banner.png",
                "background");

#ifdef NINTENDO
			const bool roomcodeDisabled = directConnect;
#else
			const bool roomcodeDisabled = false;
#endif

		    auto back_button = createBackWidget(banner, [](Button&){
		        if (currentLobbyType == LobbyType::LobbyLocal) {
			        soundCancel();
			        disconnectFromLobby();
			        destroyMainMenu();
			        createMainMenu(false);
#ifdef NINTENDO
					if (!nxIsHandheldMode()) {
						nxAssignControllers(1, 1, true, false, true, false, nullptr);
					}
#endif
			    } else {
			        binaryPrompt(
	                    "Are you sure you want to leave\nthis lobby?",
	                    "Yes", "No",
	                    [](Button&){ // yes
			                soundActivate();
			                disconnectFromLobby();
			                destroyMainMenu();
			                createMainMenu(false);
#ifdef NINTENDO
							if (!nxIsHandheldMode()) {
								nxAssignControllers(1, 1, true, false, true, false, nullptr);
							}
#endif
	                    },
	                    [](Button& button){ // no
			                soundCancel();
			                closeBinary();
#ifndef NINTENDO
                            // release any controller assigned to this player
                            const int index = button.getOwner();
                            if (inputs.hasController(index)) {
                                inputs.removeControllerWithDeviceID(inputs.getControllerID(index));
								for (int c = 0; c < MAX_SPLITSCREEN; ++c) {
									Input::inputs[c].refresh();
								}
                            }
#endif
	                    });
			    }
			    },
			    SDL_Rect{4, 12, 0, 0});
		    back_button->setDrawCallback([](const Widget& widget, SDL_Rect pos){
		        if (currentLobbyType == LobbyType::None) {
		            return;
		        }
		        bool no_one_logged_in = true;
		        if (currentLobbyType == LobbyType::LobbyLocal) {
		            for (int c = 0; c < MAXPLAYERS; ++c) {
		                if (isPlayerSignedIn(c)) {
		                    no_one_logged_in = false;
		                    break;
		                }
		            }
		        } else if (isPlayerSignedIn(clientnum)) {
		            no_one_logged_in = false;
		        }
		        if (no_one_logged_in && countControllers() > 0) {
		            const SDL_Rect viewport{0, 0, Frame::virtualScreenX, Frame::virtualScreenY};
		            const bool pressed = ticks % TICKS_PER_SECOND >= TICKS_PER_SECOND / 2;
                    std::string path = Input::getGlyphPathForInput("ButtonB", pressed, Input::getControllerType(widget.getOwner()));
                    auto image = Image::get((std::string("*") + path).c_str());
                    const int off_x = 1;
                    const int off_y = 4;
                    const int x = pos.x + pos.w + off_x;
                    const int y = pos.y + pos.h / 2 + off_y;
                    const int w = image->getWidth();
                    const int h = image->getHeight();
                    image->draw(nullptr, SDL_Rect{x - w / 2, y - h / 2, w, h}, viewport);
		        }
		        });
	        back_button->setWidgetRight("lobby_name");
#ifdef NINTENDO
			back_button->setHideSelectors(true);
			back_button->setHideGlyphs(true);
#endif

            // lobby name
            if (type == LobbyType::LobbyLAN || type == LobbyType::LobbyOnline || (type == LobbyType::LobbyJoined && !directConnect)) {
		        auto text_box = banner->addImage(
			        SDL_Rect{160, 10, 246, 36},
			        0xffffffff,
			        "*images/ui/Main Menus/TextField_00.png",
			        "text_box"
		        );

		        constexpr int field_buffer_size = 64;

		        auto field = banner->addField("lobby_name", field_buffer_size);
		        field->setGlyphPosition(Widget::glyph_position_t::CENTERED_RIGHT);
		        field->setSelectorOffset(SDL_Rect{-7, -7, 7, 7});
		        field->setButtonsOffset(SDL_Rect{8, 0, 0, 0});
                
#ifndef NINTENDO
                if (type == LobbyType::LobbyOnline) {
                    field->setEditable(true);
                }
#endif
                
		        field->setScroll(true);
		        field->setGuide("Set a public name for this lobby.");
		        field->setSize(SDL_Rect{162, 14, 242, 28});
		        field->setFont(smallfont_outline);
		        field->setHJustify(Field::justify_t::LEFT);
		        field->setVJustify(Field::justify_t::CENTER);
		        field->setColor(makeColor(166, 123, 81, 255));
		        field->setBackgroundColor(makeColor(52, 30, 22, 255));
		        field->setBackgroundSelectAllColor(makeColor(52, 30, 22, 255));
		        field->setBackgroundActivatedColor(makeColor(52, 30, 22, 255));
		        field->setWidgetSearchParent(banner->getName());
				field->addWidgetAction("MenuPageRightAlt", "chat");
				field->addWidgetAction("MenuPageLeftAlt", "privacy");
		        field->setWidgetRight(roomcodeDisabled ? "chat" : "privacy");
		        if (type != LobbyType::LobbyJoined) {
                    field->setCallback([](Field& field){
                        if (LobbyHandler.getHostingType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY) {
#ifdef USE_EOS
                            stringCopy(EOS.currentLobbyName, field.getText(),
                                sizeof(EOS.currentLobbyName), field.getTextLen());
#endif // USE_EOS
                        }
                        else if (LobbyHandler.getHostingType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM) {
#ifdef STEAMWORKS
                            stringCopy(currentLobbyName, field.getText(),
                                sizeof(currentLobbyName), field.getTextLen());
#endif // STEAMWORKS
                        }
	                    });
                    if (directConnect) {
                        field->setText(getHostname());
                    } else {
                        if (LobbyHandler.getHostingType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY) {
#ifdef USE_EOS
                            field->setText(EOS.currentLobbyName);
#endif // USE_EOS
                        }
                        else if (LobbyHandler.getHostingType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM) {
#ifdef STEAMWORKS
                            field->setText(currentLobbyName);
#endif // STEAMWORKS
                        }
                    }
                }
                field->setTickCallback([](Widget& widget){
					if (widget.isSelected()) {
						auto& input = Input::inputs[widget.getOwner()];
						if (input.consumeBinaryToggle("MenuCancel")) {
							widget.deselect();
						}
					}
                    auto field = static_cast<Field*>(&widget);
	                if (multiplayer == CLIENT) {
	                    if (LobbyHandler.getJoiningType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY) {
#ifdef USE_EOS
	                        field->setText(EOS.currentLobbyName);
#endif // USE_EOS
	                    }
	                    else if (LobbyHandler.getJoiningType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM) {
#ifdef STEAMWORKS
	                        field->setText(currentLobbyName);
#endif // STEAMWORKS
	                    }
		            }
		            });
		    }

			// lobby type
			const char* type_str;
			if (type == LobbyType::LobbyLocal) {
			    type_str = "Local Lobby";
			} else {
			    if (directConnect) {
#ifdef NINTENDO
                    type_str = "Wireless Lobby";
#else
					type_str = "LAN Lobby";
#endif
			    } else {
			        if (type == LobbyType::LobbyJoined) {
			            if (LobbyHandler.getJoiningType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY) {
			                type_str = "Online Lobby (Epic)";
			            }
			            else if (LobbyHandler.getJoiningType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM) {
			                type_str = "Online Lobby (Steam)";
			            }
			            else {
			                type_str = "Online Lobby";
			            }
			        } else {
			            if (LobbyHandler.getHostingType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY) {
			                type_str = "Online Lobby (Epic)";
			            }
			            else if (LobbyHandler.getHostingType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM) {
			                type_str = "Online Lobby (Steam)";
			            }
			            else {
			                type_str = "Online Lobby";
			            }
			        }
			    }
			}
			auto label = banner->addField("label", 128);
			label->setHJustify(Field::justify_t::CENTER);
			label->setVJustify(Field::justify_t::CENTER);
			label->setSize(SDL_Rect{(Frame::virtualScreenX - 256) / 2, 0, 256, 48});
		    label->setFont(bigfont_outline);
		    label->setText(type_str);

			if (type != LobbyType::LobbyLocal) {
		        static auto hide_roomcode = [](Field& roomcode, Button& button, bool hide){
		            hidden_roomcode = hide;
	                if (hide) {
	                    button.setIcon("*#images/ui/Main Menus/Play/PlayerCreation/LobbySettings/Eye01.png");
	                } else {
	                    button.setIcon("*#images/ui/Main Menus/Play/PlayerCreation/LobbySettings/Eye01.png");
	                }
		            const char privacy_char = 'x';
	                if (directConnect) {
                        const Uint16 port = ::portnumber;

                        char hostname[256] = { '\0' };
						if (currentLobbyType == LobbyType::LobbyJoined) {
							stringCopy(hostname, last_address, sizeof(hostname), sizeof(last_address));
						} else {
                            // this populates the (private) address field - not the lobby name!
#ifdef NINTENDO
							nxGetHostname(hostname, sizeof(hostname));
#else
							(void)gethostname(hostname, sizeof(hostname));
							hostname[sizeof(hostname) - 1] = '\0';
#endif
						}

						// remove trailing port number
						for (int c = 0; c < sizeof(hostname); ++c) {
							if (hostname[c] == ':') {
								hostname[c] = '\0';
								break;
							}
						}

	                    if (hide) {
                            char buf[1024];
                            snprintf(buf, sizeof(buf), "%hu", port);
                            size_t len1 = stringLen(hostname, sizeof(hostname));
                            size_t len2 = stringLen(buf, sizeof(buf));
                            char* ptr = buf;
                            for (size_t c = 0; c < len1; ++c, ++ptr) {
                                *ptr = privacy_char;
                            }
                            *ptr = ':'; ++ptr;
                            for (size_t c = 0; c < len2; ++c, ++ptr) {
                                *ptr = privacy_char;
                            }
                            *ptr = '\0'; ++ptr;
                            roomcode.setText(buf);
                        } else {
                            char buf[1024];
                            snprintf(buf, sizeof(buf), "%s:%hu", hostname, port);
                            roomcode.setText(buf);
                        }
	                } else {
		                if (hide) {
		                    size_t c;
		                    char buf[8];
                            for (c = 0; c < 5; ++c) {
                                buf[c] = privacy_char;
                            }
                            buf[c] = '\0';
	                        roomcode.setText(buf);
	                    } else {
	                        roomcode.setText(LobbyHandler.getCurrentRoomKey().c_str());
	                    }
	                }
		            };

		        // roomcode
                auto roomcode_header = banner->addField("roomcode_header", 32);
                roomcode_header->setHJustify(Field::justify_t::RIGHT);
                roomcode_header->setVJustify(Field::justify_t::CENTER);
                roomcode_header->setSize(SDL_Rect{Frame::virtualScreenX - 212 - 44 - 292 - 32, 0, 320, 35});
                roomcode_header->setFont(smallfont_outline);
                if (directConnect) {
                    roomcode_header->setText("LAN address");
                } else {
                    roomcode_header->setText("Room code");
                }
                
			    auto roomcode = banner->addField("roomcode", 128);
			    roomcode->setHJustify(Field::justify_t::RIGHT);
			    roomcode->setVJustify(Field::justify_t::CENTER);
			    roomcode->setSize(SDL_Rect{Frame::virtualScreenX - 212 - 44 - 292 - 32, 0, 320, 70});
		        roomcode->setFont(bigfont_outline);

				if (roomcodeDisabled) {
					roomcode->setInvisible(true);
				} else {
					// privacy button
					auto privacy = banner->addButton("privacy");
					privacy->setOwner(clientnum);
					privacy->setSize(SDL_Rect{Frame::virtualScreenX - 212 - 44, 8, 40, 40});
					privacy->setBackground("*#images/ui/Main Menus/Play/PlayerCreation/LobbySettings/UI_LobbySettings_Button_Tiny00A.png");
					privacy->setBackgroundHighlighted("*#images/ui/Main Menus/Play/PlayerCreation/LobbySettings/UI_LobbySettings_Button_Tiny00B_Highlighted.png");
					privacy->setBackgroundActivated("*#images/ui/Main Menus/Play/PlayerCreation/LobbySettings/UI_LobbySettings_Button_Tiny00C_Pressed.png");
					privacy->setColor(0xffffffff);
					privacy->setHighlightColor(0xffffffff);
					privacy->setTextHighlightColor(0xffffffff);
					privacy->setTextColor(0xffffffff);
					privacy->setFont(smallfont_outline);
					privacy->setAlwaysShowGlyphs(true);
					privacy->setWidgetSearchParent(banner->getName());
					privacy->addWidgetAction("MenuPageRightAlt", "chat");
					privacy->addWidgetAction("MenuPageLeftAlt", "privacy");
					privacy->setWidgetLeft("lobby_name");
					privacy->setWidgetRight("chat");
					privacy->setCallback([](Button& button){
						soundToggle();
						auto banner = static_cast<Frame*>(button.getParent()); assert(banner);
						auto roomcode = banner->findField("roomcode"); assert(roomcode);
						hide_roomcode(*roomcode, button, !hidden_roomcode);
						});
					privacy->setTickCallback([](Widget& widget){
						auto& input = Input::inputs[widget.getOwner()];

						auto banner = static_cast<Frame*>(widget.getParent()); assert(banner);
						auto roomcode = banner->findField("roomcode"); assert(roomcode);
						if ( !strcmp(roomcode->getText(), "") )
						{
							hide_roomcode(*roomcode, static_cast<Button&>(widget), hidden_roomcode);
						}

						// this refocuses the player card
						if (widget.isSelected()) {
							if (input.consumeBinaryToggle("MenuCancel")) {
								widget.deselect();
							}
						}

						// activate from anywhere, in any state
						if (input.consumeBinaryToggle("MenuPageLeftAlt")) {
							widget.select();
							widget.activate();
						}
						});

					// set default privacy
					hide_roomcode(*roomcode, *privacy, hidden_roomcode);
				}

                // chat button
			    auto chat_button = banner->addButton("chat");
				chat_button->setOwner(clientnum);
			    chat_button->setSize(SDL_Rect{Frame::virtualScreenX - 212, 8, 134, 40});
			    chat_button->setHighlightColor(0xffffffff);
		        chat_button->setColor(0xffffffff);
			    chat_button->setTextHighlightColor(0xffffffff);
		        chat_button->setTextColor(0xffffffff);
#ifdef NINTENDO
		        chat_button->setText("Messages");
#else
		        chat_button->setText("Chat");
#endif
		        chat_button->setBackground("*#images/ui/Main Menus/Play/PlayerCreation/Button_Chat00.png");
		        chat_button->setBackgroundHighlighted("*#images/ui/Main Menus/Play/PlayerCreation/Button_ChatHigh00.png");
		        chat_button->setBackgroundActivated("*#images/ui/Main Menus/Play/PlayerCreation/Button_ChatPress00.png");
		        chat_button->setFont(smallfont_outline);
		        chat_button->setCallback([](Button& button){
		            soundActivate();
		            (void)toggleLobbyChatWindow();
					button.select();
		            });
				chat_button->setAlwaysShowGlyphs(true);
		        chat_button->setWidgetSearchParent(banner->getName());
				chat_button->addWidgetAction("MenuPageRightAlt", "chat");
				chat_button->addWidgetAction("MenuPageLeftAlt", "privacy");
		        chat_button->setWidgetLeft(roomcodeDisabled ? "lobby_name" : "privacy");
		        chat_button->setTickCallback([](Widget& widget){
					auto& input = Input::inputs[widget.getOwner()];

					// this refocuses the player card
					if (widget.isSelected()) {
						if (input.consumeBinaryToggle("MenuCancel")) {
							widget.deselect();
						}
					}

					// flash text
		            auto button = static_cast<Button*>(&widget);
		            if (new_lobby_chat_message_alert) {
		                const Uint32 time = (ticks - new_lobby_chat_message_alert) % 20;
		                if (time < 10) {
			                button->setTextHighlightColor(uint32ColorBaronyBlue);
		                    button->setTextColor(uint32ColorBaronyBlue);
		                } else {
			                button->setTextHighlightColor(uint32ColorWhite);
		                    button->setTextColor(uint32ColorWhite);
		                }
		            } else {
			            button->setTextHighlightColor(uint32ColorWhite);
		                button->setTextColor(uint32ColorWhite);
		            }

					// activate from anywhere, in any state
					if (input.consumeBinaryToggle("MenuPageRightAlt")) {
						widget.select();
						widget.activate();
					}
		            });
		    }
		}

		if (type == LobbyType::LobbyLocal) {
			multiplayer = SINGLE;
            for (int c = 0; c < MAX_SPLITSCREEN; ++c) {
                createStartButton(c);
            }
		} else if (type == LobbyType::LobbyLAN) {
			setupNetGameAsServer();
			createStartButton(0);
            for (int c = 1; c < MAXPLAYERS; ++c) {
                createWaitingStone(c);
            }
		} else if (type == LobbyType::LobbyOnline) {
			setupNetGameAsServer();
			createStartButton(0);
            for (int c = 1; c < MAXPLAYERS; ++c) {
                createInviteButton(c);
            }
		} else if (type == LobbyType::LobbyJoined) {
		    for (int c = 0; c < MAXPLAYERS; ++c) {
		        if (clientnum == c) {
		            createStartButton(c);
		        } else {
		            if (client_disconnected[c]) {
		                if (directConnect) {
		                    createWaitingStone(c);
		                } else {
		                    createInviteButton(c);
		                }
		            } else {
		                createReadyStone(c, false, false);
		            }
		        }
		    }
		}

		// announce lobby in chat window
		new_lobby_chat_message_alert = 0;
		lobby_chat_messages.clear();
		//toggleLobbyChatWindow();
		if (type == LobbyType::LobbyLAN || type == LobbyType::LobbyOnline) {
            if (directConnect) {
#ifdef NINTENDO
				addLobbyChatMessage(uint32ColorBaronyBlue, "Wireless lobby opened successfully.");
#else
                addLobbyChatMessage(uint32ColorBaronyBlue, "Server hosted on LAN successfully.");
#endif
            } else {
                if (LobbyHandler.getHostingType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM) {
                    addLobbyChatMessage(uint32ColorBaronyBlue, "Lobby successfully hosted via Steam.");
                }
                else if (LobbyHandler.getHostingType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY) {
                    addLobbyChatMessage(uint32ColorBaronyBlue, "Lobby successfully hosted via Epic Online.");
                }
            }
		}
		else if (type == LobbyType::LobbyJoined) {
            if (directConnect) {
#ifdef NINTENDO
				addLobbyChatMessage(uint32ColorBaronyBlue, "Joined wireless lobby successfully.");
#else
                addLobbyChatMessage(uint32ColorBaronyBlue, "Joined LAN server successfully.");
#endif
            } else {
                if (LobbyHandler.getJoiningType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM) {
                    addLobbyChatMessage(uint32ColorBaronyBlue, "Joined lobby successfully via Steam.");
                }
                else if (LobbyHandler.getJoiningType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY) {
                    addLobbyChatMessage(uint32ColorBaronyBlue, "Joined lobby successfully via Epic Online.");
                }
            }
		}
	}

	static void createControllerPrompt(int index, bool multiple_players, void (*after_func)()) {
        /*if (inputs.hasController(index)) {
            inputs.removeControllerWithDeviceID(inputs.getControllerID(index));
        }*/

        static void (*end_func)();
		static bool clicked;
		clicked = false;
		end_func = after_func;

		static auto button_func = [](Button& button) {
		    int index = button.getOwner();
	        if (Input::waitingToBindControllerForPlayer >= 0) {
	            // this only happens if the mouse was used to click this button
	            Input::waitingToBindControllerForPlayer = -1;
	            if (inputs.hasController(index)) {
	                inputs.removeControllerWithDeviceID(inputs.getControllerID(index));
	            }
				inputs.setPlayerIDAllowedKeyboard(index);
	            inputs.getVirtualMouse(index)->draw_cursor = true;
	            inputs.getVirtualMouse(index)->lastMovementFromController = false;
	        } else {
	            // this happens if a controller was bound to the player
				inputs.getVirtualMouse(index)->draw_cursor = false;
				inputs.getVirtualMouse(index)->lastMovementFromController = true;
	            //if (inputs.getPlayerIDAllowedKeyboard() == index) {
	            //    inputs.setPlayerIDAllowedKeyboard(-1);
	            //}
	        }
		    button.deselect();
		    clicked = true;
		    };

		static auto button_tick_func = [](Widget& widget) {
		    auto button = static_cast<Button*>(&widget);
		    int index = button->getOwner();
		    auto& input = Input::inputs[index];
		    if (clicked && !input.binary("MenuConfirm")) {
	            for (auto& input : Input::inputs) {
	                input.refresh(); // this has to be deferred because it knocks out consumed statuses.
	            }
		        auto parent = static_cast<Frame*>(button->getParent());
		        parent = static_cast<Frame*>(parent->getParent());
	            parent->removeSelf();
	            parent->setDisabled(true);
	            soundActivate();
	            end_func();
		    }
		    };

        static real_t bounce;
        bounce = (real_t)0.0;
        auto prompt_tick_callback = [](Widget& widget){
            auto frame = static_cast<Frame*>(widget.getParent());
            const int index = multiplayer == CLIENT ? 0 : frame->getOwner();
			const real_t inc = (PI / fpsLimit) * 0.5f;
            for (int c = 0; c < MAXPLAYERS; ++c) {
		        bounce += inc;
		        const real_t bounce_height = fabs(sin(bounce)) * 32.0;
                const std::string name = std::string("player") + std::to_string(index);
                auto image = frame->findImage(name.c_str());
                if (image) {
		            const int h = image->pos.h;
                    const int y = frame->getSize().h - h - 32 - (int)bounce_height;
	                image->pos.y = y;
                }
                auto field = frame->findField(name.c_str());
                if (field) {
                    auto size = field->getSize();
                    size.y = frame->getSize().h - size.h - 32 - (int)bounce_height;
                    field->setSize(size);
                }
            }
            };

		char text[1024];
        if (multiple_players) {
		    snprintf(text, sizeof(text), "Press A on a controller to assign it to Player %d,\n"
		        "or click here to assign only the mouse and keyboard\n\n\n\n", index + 1);
        } else {
		    snprintf(text, sizeof(text), "Press A on a controller to activate it now,\n"
		        "or click here to use only the mouse and keyboard\n\n\n\n");
        }

        auto prompt = textPrompt("controller_prompt", text, prompt_tick_callback, false);
        prompt->setOwner(index);

        auto header_size = prompt->getActualSize(); header_size.h = 80;
        auto header = prompt->addField("header", 128);
        header->setSize(header_size);
        header->setFont(bigfont_outline);
        header->setJustify(Field::justify_t::CENTER);
        header->setText("ASSIGN CONTROLLER");

        auto dimmer = static_cast<Frame*>(prompt->getParent());
        dimmer->setOwner(index);

		auto button = prompt->addButton("button");
		button->setSize(prompt->getActualSize());
		button->setJustify(Field::justify_t::CENTER);
		button->setGlyphPosition(Widget::glyph_position_t::CENTERED);
		button->setTickCallback(button_tick_func);
		button->setCallback(button_func);
		button->setHideKeyboardGlyphs(false);
		button->setHideSelectors(true);
		button->setHighlightColor(0);
		button->setBorder(0);
		button->setColor(0);
		button->select();

	    int playercount = 0;
		if (multiplayer == SINGLE) {
			for (int c = 0; c < MAXPLAYERS; ++c) {
				if (isPlayerSignedIn(c)) {
					++playercount;
				}
			}
		} else {
			playercount = 1;
		}

	    int num = 0;
	    for (int c = 0; c < MAXPLAYERS; ++c) {
	        if ((multiplayer == SINGLE && isPlayerSignedIn(c)) || (multiplayer != SINGLE && c == 0)) {
                const char* path = inputs.hasController(c) || inputs.getPlayerIDAllowedKeyboard() != c ?
                    Input::getControllerGlyph(c) : Input::getKeyboardGlyph(c);
                auto image = Image::get(path);
                const int w = image->getWidth();
                const int h = image->getHeight();
                const int space = 100;
                const int x = (prompt->getSize().w - playercount * space + space - w) / 2 + num * space;
                const int y = prompt->getSize().h - h - 32;
	            const std::string name = std::string("player") + std::to_string(c);
                prompt->addImage(
                    SDL_Rect{x, y, w, h},
                    makeColor(255, 255, 255, 255),
                    path, name.c_str());
                auto field = prompt->addField(name.c_str(), 16);
                field->setSize(SDL_Rect{x, y, w, h});
                field->setJustify(Field::justify_t::CENTER);
				if (multiplayer == SINGLE) {
                	field->setText((std::string("P") + std::to_string(c + 1)).c_str());
				}
                field->setFont(bigfont_outline);
				field->setColor(playerColor(c, colorblind_lobby, false));
                ++num;
	        }
	    }

		Input::waitingToBindControllerForPlayer = index;
	}

/******************************************************************************/

	struct LobbyInfo {
	    std::string name;
        std::string version;
	    int players;
	    int ping;
	    bool locked;
	    Uint32 flags;
	    std::string address;
	    intptr_t index = -1;
	    LobbyInfo(
	        const char* _name = "Barony",
            const char* _version = VERSION,
	        int _players = 0,
	        int _ping = 0,
	        bool _locked = false,
	        Uint32 _flags = 0,
	        const char* _address = ""):
	        name(_name),
            version(_version),
	        players(_players),
	        ping(_ping),
	        locked(_locked),
	        flags(_flags),
	        address(_address)
	    {}
	};

    static bool lobbyFiltersEnabled = false;
	static std::vector<LobbyInfo> lobbies;
	static int selectedLobby = 0;

	static void addLobby(LobbyInfo& info) {
	    if (info.ping < 0) {
	        // probably the result of an out-of-date network scan
	        return;
	    }
        
        if (info.players <= 0) {
            // do not include empty lobbies
            return;
        }

        if (info.index == -1) {
            lobbies.push_back(info);
            lobbies.back().index = lobbies.size() - 1;
        }

        bool foundFriend = false;

        // this is an epic lobby, check if it's friends-only and filter it
        if (info.address[0] == 'e') {
#ifdef USE_EOS
            auto lobby = getLobbyEpic(info.address.c_str());
            if (lobby) {
				if ( lobby->LobbyAttributes.PermissionLevel != 0 )
				{
					// this is a invite-only lobby.
					info.locked = true;
					info.name = "Private lobby";
					lobbies.back().name = info.name;
					lobbies.back().locked = info.locked;
					//return;
				}
                for (auto& player : lobby->playersInLobby) {
                    for (auto& _friend : EOS.CurrentUserInfo.Friends) {
                        if (_friend.EpicAccountId == player.memberEpicAccountId) {
                            foundFriend = true;
                            break;
                        }
                    }
                    if (foundFriend) {
                        break;
                    }
                }
                if (lobby->LobbyAttributes.friendsOnly) {
                    if (!foundFriend) {
                        // this is a friends-only lobby, and we don't have any friends in it.
						info.locked = true;
						info.name = "Private lobby";
						lobbies.back().name = info.name;
						lobbies.back().locked = info.locked;
                        //return;
                    }
                }
            }
#endif
        }

        // this is a steam lobby, check if it's friends-only or invite-only and filter it
        if (info.address[0] == 's') {
#ifdef STEAMWORKS
            auto lobby = getLobbySteamID(info.address.c_str());
            if (lobby) {
				auto invite_only = SteamMatchmaking()->GetLobbyData(*lobby, "invite_only");
				if (invite_only && stringCmp(invite_only, "true", 4, 4) == 0) {
					// this is a invite-only lobby.
					info.locked = true;
					info.name = "Private lobby";
					lobbies.back().name = info.name;
					lobbies.back().locked = info.locked;
					//return;
				}
                const int num_friends = SteamFriends()->GetFriendCount( k_EFriendFlagImmediate );
                for (int i = 0; i < num_friends; ++i) {
                    FriendGameInfo_t friendGameInfo;
                    CSteamID steamIDFriend = SteamFriends()->GetFriendByIndex( i, k_EFriendFlagImmediate );
                    if (SteamFriends()->GetFriendGamePlayed( steamIDFriend, &friendGameInfo ) && friendGameInfo.m_steamIDLobby.IsValid()) {
                        if (friendGameInfo.m_steamIDLobby == *lobby) {
                            foundFriend = true;
                        }
                    }
                }
                auto friends = SteamMatchmaking()->GetLobbyData(*lobby, "friends_only");
                if (friends && stringCmp(friends, "true", 4, 4) == 0) {
                    if (!foundFriend) {
                        // this is a friends-only lobby, and we don't have any friends in it.
						info.locked = true;
						info.name = "Private lobby";
						lobbies.back().name = info.name;
						lobbies.back().locked = info.locked;
                        //return;
                    }
                }
            }
#endif
        }

        if (lobbyFiltersEnabled) {
            if (lobbyFilters[0] == Filter::ON && !info.locked) {
                // this lobby is locked and we don't want to show those
                printlog("skipping lobby '%s' (lobby is locked)\n", info.name.c_str());
                return;
            }
            if (lobbyFilters[0] == Filter::OFF && info.locked) {
                // this lobby isn't locked and we don't want to show those
                printlog("skipping lobby '%s' (lobby is not locked)\n", info.name.c_str());
                return;
            }
            if (lobbyFilters[1] == Filter::ON && !foundFriend) {
                printlog("skipping lobby '%s' (has no friends)\n", info.name.c_str());
                return;
            }
            if (lobbyFilters[1] == Filter::OFF && foundFriend) {
                printlog("skipping lobby '%s' (has friends)\n", info.name.c_str());
                return;
            }
            if (lobbyFilters[2] == Filter::ON && (info.flags & (SV_FLAG_CHEATS | SV_FLAG_LIFESAVING))) {
                // lobbies with cheats or +1 life do not count for
                // achievements.
                printlog("skipping lobby '%s' (achievements disabled)\n", info.name.c_str());
                return;
            }
            if (lobbyFilters[2] == Filter::OFF && !(info.flags & (SV_FLAG_CHEATS | SV_FLAG_LIFESAVING))) {
                // we're only looking for lobbies where achievements aren't enabled
                printlog("skipping lobby '%s' (achievements enabled)\n", info.name.c_str());
                return;
            }
            for (int c = 0; c < NUM_SERVER_FLAGS; ++c) {
                const bool flag = (info.flags & (1 << c)) == 0 ? false : true;
                const int index = (numFilters - NUM_SERVER_FLAGS) + c;
                if (lobbyFilters[index] == Filter::ON && !flag) {
                    // check the server flag filters
                    printlog("skipping lobby '%s' (server flag %d is off)\n", info.name.c_str(), c);
                    return;
                }
                if (lobbyFilters[index] == Filter::OFF && flag) {
                    // check the server flag filters
                    printlog("skipping lobby '%s' (server flag %d is on)\n", info.name.c_str(), c);
                    return;
                }
            }
        }

	    assert(main_menu_frame);
	    auto window = main_menu_frame->findFrame("lobby_browser_window");
	    if (!window) {
	        return;
	    }

	    auto names = window->findFrame("names"); assert(names);
	    auto players = window->findFrame("players"); assert(players);
	    //auto pings = window->findFrame("pings"); assert(pings);
		auto versions = window->findFrame("versions"); assert(versions);

	    // function to make highlight the same on all columns...
	    static auto selection_fn = [](Frame::entry_t& entry){
	        assert(main_menu_frame);
	        auto window = main_menu_frame->findFrame("lobby_browser_window"); assert(window);
	        auto names = window->findFrame("names"); assert(names);
	        auto players = window->findFrame("players"); assert(players);
			//auto pings = window->findFrame("pings"); assert(pings);
	        auto versions = window->findFrame("versions"); assert(versions);
	        auto selection = entry.parent.getSelection();
	        names->setSelection(selection);
	        players->setSelection(selection);
			versions->setSelection(selection);
			//pings->setSelection(selection);
            
            auto mouse = inputs.getVirtualMouse(entry.parent.getOwner());
            if (mouse && !mouse->draw_cursor) {
                auto lobbyId = (intptr_t)entry.data;
                selectedLobby = (int)lobbyId;
            }
            };

        // function to choose a specific lobby
        static auto activate_fn = [](Frame::entry_t& entry){
	        assert(main_menu_frame);
	        auto window = main_menu_frame->findFrame("lobby_browser_window"); assert(window);
	        auto names = window->findFrame("names"); assert(names);
	        auto players = window->findFrame("players"); assert(players);
			//auto pings = window->findFrame("pings"); assert(pings);
	        auto versions = window->findFrame("versions"); assert(versions);
	        auto selection = entry.parent.getSelection();
            names->setActivation(names->getEntries()[selection]);
            players->setActivation(players->getEntries()[selection]);
			//pings->setActivation(pings->getEntries()[selection]);
			versions->setActivation(versions->getEntries()[selection]);
			auto lobbyId = (intptr_t)entry.data;
			if (selectedLobby != lobbyId) {
				selectedLobby = (int)lobbyId;
			} else {
				if (!inputs.getVirtualMouse(getMenuOwner())->draw_cursor) {
                    if (lobbyId >= 0 && lobbyId < lobbies.size()) {
                        // pressing A on a lobby after selecting it will join that lobby
                        const auto& lobby = lobbies[lobbyId];
                        if (!lobby.locked) {
							int index = (int)lobby.index;
                            if (connectToServer(lobby.address.c_str(), &index,
                                    directConnect ? LobbyType::LobbyLAN : LobbyType::LobbyOnline)) {
                                // only deselect the list if the connection begins
                                entry.parent.deselect();
                            }
                        }
                        else {
                            errorPrompt("Unable to join lobby.\nLobby is locked.",
                                "Okay", [](Button&) {soundCancel(); closeMono(); });
                        }
                    }
				}
			}
            };

        // name cell
        auto entry_name = names->addEntry(info.name.c_str(), true);
        entry_name->click = activate_fn;
        entry_name->ctrlClick = activate_fn;
        entry_name->highlight = selection_fn;
        entry_name->selected = selection_fn;
        entry_name->color = info.locked ? makeColor(50, 56, 67, 255) : makeColor(183, 155, 119, 255);
        entry_name->text = std::string("  ") + info.name;
        entry_name->data = (info.index < 0 || info.index >= lobbies.size()) ?
            (void*)lobbies.back().index : (void*)lobbies[info.index].index;

        // players cell
        const char* players_image;
        if (info.locked) {
            players_image = "*images/ui/Main Menus/Play/LobbyBrowser/Lobby_Players_Grey.png";
        } else {
            switch (info.players) {
            case 0: players_image = "*images/ui/Main Menus/Play/LobbyBrowser/Lobby_Players_0.png"; break;
            case 1: players_image = "*images/ui/Main Menus/Play/LobbyBrowser/Lobby_Players_1.png"; break;
            case 2: players_image = "*images/ui/Main Menus/Play/LobbyBrowser/Lobby_Players_2.png"; break;
            case 3: players_image = "*images/ui/Main Menus/Play/LobbyBrowser/Lobby_Players_3.png"; break;
            case 4: players_image = "*images/ui/Main Menus/Play/LobbyBrowser/Lobby_Players_4.png"; break;
            default: players_image = "*images/ui/Main Menus/Play/LobbyBrowser/Lobby_Players_4.png"; break;
            }
        }
        auto entry_players = players->addEntry(info.name.c_str(), true);
        entry_players->click = activate_fn;
        entry_players->ctrlClick = activate_fn;
        entry_players->highlight = selection_fn;
        entry_players->selected = selection_fn;
        entry_players->color = 0xffffffff;
        entry_players->image = players_image;
        entry_players->data = (info.index < 0 || info.index >= lobbies.size()) ?
            (void*)lobbies.back().index : (void*)lobbies[info.index].index;

		auto entry_version = versions->addEntry(info.name.c_str(), true);
		entry_version->click = activate_fn;
		entry_version->ctrlClick = activate_fn;
		entry_version->highlight = selection_fn;
		entry_version->selected = selection_fn;
		entry_version->color = info.locked ? makeColor(50, 56, 67, 255) : makeColor(183, 155, 119, 255);
		entry_version->text = std::string("  ") + info.version;
		entry_version->data = (info.index < 0 || info.index >= lobbies.size()) ?
			(void*)lobbies.back().index : (void*)lobbies[info.index].index;

        //// ping cell
        //auto entry_ping = pings->addEntry(info.name.c_str(), true);
        //entry_ping->click = activate_fn;
        //entry_ping->ctrlClick = activate_fn;
        //entry_ping->highlight = selection_fn;
        //entry_ping->selected = selection_fn;
        //entry_ping->color = 0xffffffff;
        //entry_ping->data = (info.index < 0 || info.index >= lobbies.size()) ?
        //    (void*)lobbies.back().index : (void*)lobbies[info.index].index;
        //if (!info.locked) {
        //    if (info.ping < 100) {
        //        entry_ping->image = "*images/ui/Main Menus/Play/LobbyBrowser/Lobby_Ping_Green00.png";
        //    } else if (info.ping < 200) {
        //        entry_ping->image = "*images/ui/Main Menus/Play/LobbyBrowser/Lobby_Ping_Yellow00.png";
        //    } else if (info.ping < 300) {
        //        entry_ping->image = "*images/ui/Main Menus/Play/LobbyBrowser/Lobby_Ping_Orange00.png";
        //    } else {
        //        entry_ping->image = "*images/ui/Main Menus/Play/LobbyBrowser/Lobby_Ping_Red00.png";
        //    }
        //}

        auto slider = window->findSlider("scroll_slider");
        slider->setMaxValue(names->getActualSize().h - names->getSize().h);
		slider->updateHandlePosition();
	}

	static void clearLobbies() {
	    assert(main_menu_frame);
	    auto window = main_menu_frame->findFrame("lobby_browser_window");
	    if (!window) {
	        return;
	    }
	    auto names = window->findFrame("names"); assert(names);
	    auto players = window->findFrame("players"); assert(players);
	    //auto pings = window->findFrame("pings"); assert(pings);
		auto versions = window->findFrame("versions"); assert(versions);
	    names->clearEntries();
	    names->setSelection(-1);
	    names->setActivation(nullptr);
	    players->clearEntries();
	    players->setSelection(-1);
	    players->setActivation(nullptr);
	    //pings->clearEntries();
	    //pings->setSelection(-1);
	    //pings->setActivation(nullptr);
		versions->clearEntries();
		versions->setSelection(-1);
		versions->setActivation(nullptr);
	}

	static void refreshOnlineLobbies() {
#if !defined(STEAMWORKS) && !defined(USE_EOS)
        // Do nothing in DRM-free builds.
        return;
#endif

	    // close current window
#ifdef STEAMWORKS
	    if ( connectingToLobbyWindow )
	    {
		    // we quit the connection window before joining lobby, but invite was mid-flight.
		    denyLobbyJoinEvent = true;
	    }
	    else if ( joinLobbyWaitingForHostResponse )
	    {
		    // we quit the connection window after lobby join, but before host has accepted us.
		    joinLobbyWaitingForHostResponse = false;
	        auto error_str = LobbyHandler_t::getLobbyJoinFailedConnectString(
	            static_cast<int>(LobbyHandler_t::LOBBY_JOIN_CANCELLED));
	        connectionErrorPrompt(error_str.c_str());
	        disconnectFromLobby();
		    return;
	    }
#endif
#if defined USE_EOS
	    if ( EOS.bConnectingToLobbyWindow )
	    {
		    // we quit the connection window before joining lobby, but invite was mid-flight.
		    EOS.CurrentLobbyData.bDenyLobbyJoinEvent = true;
	    }
	    else if ( EOS.bJoinLobbyWaitingForHostResponse )
	    {
		    // we quit the connection window after lobby join, but before host has accepted us.
		    EOS.bJoinLobbyWaitingForHostResponse = false;
	        auto error_str = LobbyHandler_t::getLobbyJoinFailedConnectString(
	            static_cast<int>(LobbyHandler_t::LOBBY_JOIN_CANCELLED));
	        connectionErrorPrompt(error_str.c_str());
	        disconnectFromLobby();
		    return;
	    }
#endif

	    // create new window
	    cancellablePrompt("lobby_list_request", "Requesting lobby list...", "Cancel",
	        [](Widget& widget){
#if defined(STEAMWORKS)
#if defined(USE_EOS)
            if (!requestingLobbies && !EOS.bRequestingLobbies) {
#else
            if (!requestingLobbies) {
#endif
#elif defined (USE_EOS)
            if (!EOS.bRequestingLobbies) {
#else
            {
#endif
                // lobby list has returned
                closePrompt("lobby_list_request");
				
				// select names list
				assert(main_menu_frame);
				auto lobby_browser_window = main_menu_frame->findFrame("lobby_browser_window"); assert(lobby_browser_window);
				auto names = lobby_browser_window->findFrame("names"); assert(names);
				names->select();

#if defined(STEAMWORKS)
	            for (Uint32 c = 0; c < numSteamLobbies; ++c) {
	                auto lobby = (CSteamID*)lobbyIDs[c];
	                auto pchFlags = SteamMatchmaking()->GetLobbyData(*lobby, "svFlags");
	                auto flags = (int)strtol(pchFlags, nullptr, 10);
	                LobbyInfo info;
	                info.name = lobbyText[c];
                    info.version = lobbyVersion[c];
	                info.players = lobbyPlayers[c];
	                info.ping = 50; // TODO
	                info.locked = false; // this will always be false because steam only reported joinable lobbies
	                info.flags = (Uint32)flags;
	                info.address = "steam:" + std::to_string(c);
	                addLobby(info);
	            }
#endif
#if defined(USE_EOS)
	            for (int c = 0; c < EOS.LobbySearchResults.resultsSortedForDisplay.size(); ++c) {
					if ( auto lobby = EOS.LobbySearchResults.getResultFromDisplayedIndex(c) )
					{
						LobbyInfo info;
						info.name = lobby->LobbyAttributes.lobbyName;
						info.version = lobby->LobbyAttributes.gameVersion;
						info.players = MAXPLAYERS - lobby->FreeSlots;
						info.ping = 50; // TODO
						info.locked = lobby->LobbyAttributes.gameCurrentLevel != -1;
						info.flags = lobby->LobbyAttributes.serverFlags;
						info.address = "epic:" + std::to_string(c);
						addLobby(info);
					}
	            }
#endif
            }
			},
			[](Button&){ // cancel
#if defined(STEAMWORKS)
				requestingLobbies = false;
#endif
#if defined(USE_EOS)
            	EOS.bRequestingLobbies = false;
#endif

                // lobby list has returned
                closePrompt("lobby_list_request");
				
				// select names list
				assert(main_menu_frame);
				auto lobby_browser_window = main_menu_frame->findFrame("lobby_browser_window"); assert(lobby_browser_window);
				auto names = lobby_browser_window->findFrame("names"); assert(names);
				names->select();
			});

        // request new lobbies
	    LobbyHandler.selectedLobbyInList = 0;
#ifdef STEAMWORKS
	    requestingLobbies = true;
	    cpp_SteamMatchmaking_RequestLobbyList(nullptr);
#endif
#ifdef USE_EOS
	    EOS.bRequestingLobbies = true;
#ifdef STEAMWORKS
	    if ( EOS.CurrentUserInfo.bUserLoggedIn )
	    {
		    EOS.searchLobbies(EOSFuncs::LobbyParameters_t::LobbySearchOptions::LOBBY_SEARCH_ALL,
			    EOSFuncs::LobbyParameters_t::LobbyJoinOptions::LOBBY_DONT_JOIN, "");
	    }
	    else
	    {
		    EOS.bRequestingLobbies = false; // don't attempt search if not logged in
			for ( auto& result : EOS.LobbySearchResults.results )
			{
				result.ClearData();
			}
			EOS.LobbySearchResults.results.clear();
			EOS.LobbySearchResults.resultsSortedForDisplay.clear();
	    }
#else
	    EOS.searchLobbies(EOSFuncs::LobbyParameters_t::LobbySearchOptions::LOBBY_SEARCH_ALL,
		    EOSFuncs::LobbyParameters_t::LobbyJoinOptions::LOBBY_DONT_JOIN, "");
#endif
#endif // USE_EOS
	}

	struct ScanNetworkResources {
	    ~ScanNetworkResources() {
	        close();
	    }
	    void close() {
	        if (packet != nullptr) {
		        SDLNet_FreePacket(packet);
		        packet = nullptr;
	        }
	        if (sock != nullptr) {
		        SDLNet_UDP_Close(sock);
		        sock = nullptr;
	        }
	    }
        UDPsocket sock = nullptr;
        UDPpacket* packet = nullptr;
	};
	static ScanNetworkResources scan;
	static Uint32 scan_ticks = 0;

	static void refreshLobbyBrowser() {
		soundActivate();
		clearLobbies(); // clear visible list
		lobbies.clear(); // clear internal list
		selectedLobby = -1; // select no lobby at all
		scan_ticks = ticks;
		if (directConnect) {
#ifdef NINTENDO
			int result = nxScanLobbies();
			if (result >= 0) {
				printlog("[NX] scanned and found %d lobbies", result);
				for (int c = 0; c < result; ++c) {
					LobbyInfo info;
					info.name = nxLobbies[c].name;
                    info.version = VERSION; // TODO
					info.players = nxLobbies[c].numplayers;
					info.ping = nxLobbies[c].ping;
					info.locked = nxLobbies[c].ingame;
					info.flags = nxLobbies[c].svFlags;
					info.address = nxLobbies[c].address;
					addLobby(info);
				}
			}
			else {
				// error
				multiplayer = SINGLE;
				loadingsavegame = 0;
				soundError();
				closeNetworkInterfaces();
				nxEnableAutoSleep();
				nxEndParentalControls();
				nxShutdownWireless();
				logoutOfEpic();
				destroyMainMenu();
				createMainMenu(false);
			}
#else
			memcpy(scan.packet->data, "SCAN", 4);
			scan.packet->len = 4;
			scan.packet->address.host = 0xffffffff;
			SDLNet_Write16(DEFAULT_PORT, &scan.packet->address.port);
			//SDLNet_ResolveHost(&scan.packet->address, "224.0.0.150", DEFAULT_PORT);
			sendPacket(scan.sock, -1, scan.packet, 0);
#endif
		}
		else {
			refreshOnlineLobbies();
		}

		assert(main_menu_frame);
		auto window = main_menu_frame->findFrame("lobby_browser_window");
		if (window) {
			auto slider = window->findSlider("scroll_slider");
			slider->setValue(0.f);
			slider->setMinValue(0.f);
			slider->setMaxValue(0.f);
			slider->updateHandlePosition();
		}
	};

	static void createLobbyBrowser(Button& button) {
		selectedLobby = -1;
		lobbies.clear();

		enum class BrowserMode {
			Online,
			LAN,
		};
		static BrowserMode mode;
#if defined(NINTENDO)
		mode = BrowserMode::LAN;
		directConnect = true;
		logoutOfEpic();
		nxShutdownWireless();
		if (!nxInitWireless()) {
			multiplayer = SINGLE;
			loadingsavegame = 0;
			soundError();
			closeNetworkInterfaces();
			destroyMainMenu();
			createMainMenu(false);
		}
		nxDisableAutoSleep();
#elif defined(STEAMWORKS)
		mode = BrowserMode::Online;
		directConnect = false;
#elif defined(USE_EOS)
		const bool connected = isConnectedToEpic();
		mode = connected ? BrowserMode::Online : BrowserMode::LAN;
		directConnect = !connected;
#else
        mode = BrowserMode::LAN;
		directConnect = true;
#endif

        closeNetworkInterfaces();
        
#ifndef NINTENDO
        // open a socket for network scanning
        scan.close();
	    scan.sock = SDLNet_UDP_Open(NETWORK_SCAN_PORT_CLIENT);
	    assert(scan.sock);

        // allocate packet data for scanning
        scan.packet = SDLNet_AllocPacket(NET_PACKET_SIZE);
        assert(scan.packet);
#endif

		// remove "Local or Network" window
		auto frame = static_cast<Frame*>(button.getParent());
		frame = static_cast<Frame*>(frame->getParent());
		frame->removeSelf();

		auto dimmer = main_menu_frame->addFrame("dimmer");
		dimmer->setSize(SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY});
		dimmer->setActualSize(dimmer->getSize());
		dimmer->setColor(makeColor(0, 0, 0, 63));
		dimmer->setBorder(0);

		// create lobby browser window
		auto window = dimmer->addFrame("lobby_browser_window");
		const int lobbyBrowserWidth = 1280;
		window->setSize(SDL_Rect{
			(Frame::virtualScreenX - lobbyBrowserWidth) / 2,
			(Frame::virtualScreenY - 552) / 2,
			lobbyBrowserWidth,
			552});
		window->setActualSize(SDL_Rect{0, 0, lobbyBrowserWidth, 552});
		window->setColor(0);
		window->setBorder(0);

		// while the window is open, listen for SCAN packets
#ifndef NINTENDO
		window->setTickCallback([](Widget& widget){
		    if (multiplayer != CLIENT && directConnect) {
			    if (SDLNet_UDP_Recv(scan.sock, scan.packet)) {
				    Uint32 packetId = SDLNet_Read32(scan.packet->data);
				    if (packetId == 'SCAN') {
                        if (scan.packet->len > 4) {
				            char hostname[256] = { '\0' };
				            Uint32 hostname_len;
				            hostname_len = SDLNet_Read32(&scan.packet->data[4]);
				            memcpy(hostname, &scan.packet->data[8], hostname_len);

				            Uint32 offset = 8 + hostname_len;
				            int players = (int)SDLNet_Read32(&scan.packet->data[offset]);

				            int ping = (int)(ticks - scan_ticks);

				            // there's a server on the network!
				            LobbyInfo info;
				            info.name = hostname;
				            info.players = players;
				            info.ping = ping;
				            info.locked = scan.packet->data[offset + 4];
				            info.flags = SDLNet_Read32(&scan.packet->data[offset + 5]);

                            Uint32 host = scan.packet->address.host;
				            char buf[16];
#ifdef NINTENDO
							snprintf(buf, sizeof(buf), "%hhu.%hhu.%hhu.%hhu",
								(uint8_t)((host & 0xff000000) >> 24),
								(uint8_t)((host & 0x00ff0000) >> 16),
								(uint8_t)((host & 0x0000ff00) >> 8),
								(uint8_t)((host & 0x000000ff) >> 0));
#else
							snprintf(buf, sizeof(buf), "%hhu.%hhu.%hhu.%hhu",
								(uint8_t)((host & 0x000000ff) >> 0),
								(uint8_t)((host & 0x0000ff00) >> 8),
								(uint8_t)((host & 0x00ff0000) >> 16),
								(uint8_t)((host & 0xff000000) >> 24));
#endif
				            info.address = buf;
				            addLobby(info);

				            printlog("got a SCAN packet from %s\n", buf);
				        }
				    }
			    }
			}
		    });
#endif

		(void)createBackWidget(window, [](Button& button){
			multiplayer = SINGLE;
		    loadingsavegame = 0;
		    soundCancel();
		    closeNetworkInterfaces();
		    createLocalOrNetworkMenu();

			gameModeManager.currentSession.restoreSavedServerFlags();

#ifdef NINTENDO
			nxEnableAutoSleep();
			nxEndParentalControls();
			logoutOfEpic();
			nxShutdownWireless();
#endif

		    // remove parent window
		    auto frame = static_cast<Frame*>(button.getParent());
		    frame = static_cast<Frame*>(frame->getParent());
		    frame = static_cast<Frame*>(frame->getParent());
		    frame->removeSelf();
		    }, SDL_Rect{292, 4, 0, 0});

		auto background = window->addImage(
			SDL_Rect{lobbyBrowserWidth / 2 - 696 / 2, 0, 696, 552},
			0xffffffff,
			"*images/ui/Main Menus/Play/LobbyBrowser/Lobby_Window01.png",
			"background"
		);

		auto banner_title = window->addField("banner", 64);
		banner_title->setSize(SDL_Rect{538, 24, 204, 18});
		banner_title->setText("ONLINE LOBBY BROWSER");
		banner_title->setFont(smallfont_outline);
		banner_title->setJustify(Field::justify_t::CENTER);

		auto interior = window->addImage(
			SDL_Rect{ background->pos.x + 52, 70, 590, 380},
			0xffffffff,
#if defined(STEAMWORKS) && defined(USE_EOS)
			mode == BrowserMode::Online ?
				"*images/ui/Main Menus/Play/LobbyBrowser/Lobby_InteriorWindow_Online01.png":
				"*images/ui/Main Menus/Play/LobbyBrowser/Lobby_InteriorWindow_Wireless01.png",
#else
			mode == BrowserMode::Online ?
				"*images/ui/Main Menus/Play/LobbyBrowser/Lobby_InteriorWindow_Online02.png" :
				"*images/ui/Main Menus/Play/LobbyBrowser/Lobby_InteriorWindow_Wireless02.png",
#endif
			"interior"
		);

		lobbyFiltersEnabled = false;
		for ( int i = 0; i < numFilters; ++i )
		{
			if ( lobbyFilters[i] != Filter::UNCHECKED )
			{
				lobbyFiltersEnabled = true;
				break;
			}
		}


		auto frame_right = window->addFrame("frame_right");
		frame_right->setInvisible(!lobbyFiltersEnabled);
		frame_right->setSize(SDL_Rect{background->pos.x + background->pos.w - 16, 28, 304, 414});
		frame_right->setActualSize(SDL_Rect{0, 0, 304, 414});
		frame_right->setBorder(0);
		frame_right->setColor(0);
		{
		    frame_right->addImage(
			    frame_right->getActualSize(),
			    0xffffffff,
			    "*images/ui/Main Menus/Play/LobbyBrowser/Lobby_Window_Filters00.png",
			    "background"
		    );
            
            createGenericWindowDecorations(*frame_right);
            sizeWindowDecorations(*frame_right, SDL_Rect{24, 72, 256, 312});

		    auto label = frame_right->addField("label", 128);
		    label->setJustify(Field::justify_t::CENTER);
		    label->setSize(SDL_Rect{80, 48, 146, 22});
		    label->setFont(smallfont_outline);
		    label->setText("Filters");

            const char* filter_names[] = {
                "Show non-joinable",
                "Show friends only",
                "Achievements enabled",
                "Cheats enabled",
                "Friendly fire enabled",
                "Minotaurs enabled",
                "Hunger enabled",
                "Random traps enabled",
                "Hardcore mode",
                "Classic mode",
                "Keep items on death",
                "+1 Life",
            };
            constexpr int num_filter_names = sizeof(filter_names) / sizeof(filter_names[0]);

            for (int index = 0, c = 0; c < num_filter_names; ++c) {
                if (directConnect && c == 1) {
                    // skip "friends only" filter in direct connect mode.
                    continue;
                }
#ifdef NINTENDO
				if (c == 1) {
					// nintendo has no "friends-only" filter.
					continue;
				}
#endif // NINTENDO


		        auto label = frame_right->addField("filter_label", 128);
		        label->setHJustify(Field::justify_t::LEFT);
		        label->setVJustify(Field::justify_t::CENTER);
		        label->setSize(SDL_Rect{64, 72 + 24 * index, 192, 24});
		        label->setFont(smallfont_outline);
		        label->setColor(makeColor(183, 155, 119, 255));
		        label->setText(filter_names[c]);

		        std::string checkbox_name = std::string("filter_checkbox") + std::to_string(index);

                static const char* icons[(int)Filter::NUM] = {
                    "",
                    "*#images/ui/Main Menus/Play/LobbyBrowser/Lobby_Checkbox_RedXSmall00.png",
                    "*#images/ui/Main Menus/Play/LobbyBrowser/Lobby_Checkbox_PickSmall00.png",
                };

		        auto checkbox = frame_right->addButton(checkbox_name.c_str());
		        checkbox->setSize(SDL_Rect{32, 74 + index * 24, 28, 24});
		        checkbox->setBackground("*#images/ui/Main Menus/Play/LobbyBrowser/Lobby_Checkbox_BoxSmall00.png");
		        checkbox->setIcon(icons[(int)lobbyFilters[c]]);
		        checkbox->setHighlightColor(uint32ColorWhite);
		        checkbox->setColor(uint32ColorWhite);
		        checkbox->setUserData(&lobbyFilters[c]);
		        checkbox->setSelectorOffset(SDL_Rect{0, 2, -6, 0});
		        checkbox->setCallback([](Button& button){
		            soundCheckmark();
                    Filter* filter = (Filter*)button.getUserData();
                    switch (*filter) {
                    default:
                    case Filter::UNCHECKED: *filter = Filter::ON; break;
                    case Filter::ON: *filter = Filter::OFF; break;
                    case Filter::OFF: *filter = Filter::UNCHECKED; break;
                    }
                    button.setIcon(icons[(int)*filter]);
					lobbyFiltersEnabled = false;
					for ( int i = 0; i < numFilters; ++i )
					{
						if ( lobbyFilters[i] != Filter::UNCHECKED )
						{
							lobbyFiltersEnabled = true;
							break;
						}
					}
                    clearLobbies();
                    for (auto& lobby : lobbies) {
                        addLobby(lobby);
                    }

					if ( Frame* parent = static_cast<Frame*>(button.getParent()) ) 
					{
						if ( Frame* parent2 = parent->getParent() )
						{
							if ( auto slider = parent2->findSlider("scroll_slider") )
							{
								slider->setValue(0.0);
								slider->getCallback()(*slider);
							}
						}
					}
		            });
		        checkbox->setWidgetBack("filter_settings");
		        std::string next_name = std::string("filter_checkbox") + std::to_string(index + 1);
		        std::string prev_name = std::string("filter_checkbox") + std::to_string(index - 1);
		        checkbox->setWidgetDown(next_name.c_str());
		        checkbox->setWidgetUp(prev_name.c_str());
				checkbox->addWidgetAction("MenuAlt2", "refresh");
				checkbox->addWidgetAction("MenuLeft", "filter_settings");
		        ++index;
            }
		}

		lobbyFiltersEnabled = false;
		for ( int i = 0; i < numFilters; ++i )
		{
			if ( lobbyFilters[i] != Filter::UNCHECKED )
			{
				lobbyFiltersEnabled = true;
				break;
			}
		}

		auto frame_left = window->addFrame("frame_left");
		frame_left->setInvisible(true);
		frame_left->setSize(SDL_Rect{4, 28, 304, 414});
		frame_left->setActualSize(SDL_Rect{0, 0, 304, 414});
		frame_left->setBorder(0);
		frame_left->setColor(0);
		frame_left->setTickCallback([](Widget& widget){
	        if (selectedLobby >= 0 && selectedLobby < lobbies.size()) {
                widget.setInvisible(false);

                const auto& lobby = lobbies[selectedLobby];

                const char* flag_names[] = {
                    u8" \x1E Cheats\n",
                    u8" \x1E Hurt allies\n",
                    u8" \x1E Minotaurs\n",
                    u8" \x1E Hunger\n",
                    u8" \x1E Traps\n",
                    u8" \x1E Hardcore\n",
                    u8" \x1E Classic\n",
                    u8" \x1E Keep items\n",
                    u8" \x1E +1 Life\n",
                };
                constexpr int num_flag_names = sizeof(flag_names) / sizeof(flag_names[0]);

                std::string flags1, flags2;
                bool foundFlags = false;
                for (int c = 0, index = 0; c < NUM_SERVER_FLAGS; ++c) {
                    if (lobby.flags & (1 << c)) {
                        if (index & 1) {
                            flags2.append(flag_names[c]);
                        } else {
                            flags1.append(flag_names[c]);
                        }
                        foundFlags = true;
                        ++index;
                    }
                }
                if (!foundFlags) {
                    flags1.append("None");
                }
                
                auto frame = static_cast<Frame*>(&widget); assert(frame);

		        char buf[1024];
                
                auto values1 = frame->findField("values1"); assert(values1);
                const char* values1_fmt = "\n%s (%s)";
                snprintf(buf, sizeof(buf), values1_fmt, lobby.name.c_str(), lobby.version.c_str());
                values1->setText(buf);
                values1->reflowTextToFit(0);
                const int numLines = values1->getNumTextLines();
                
                std::string header_txt = "Name:";
                std::string values2_txt;
                for (int c = 0; c <= numLines; ++c) {
                    header_txt += "\n";
                    values2_txt += "\n";
                }
                header_txt += "Flags:";
                values2_txt += "\n%s";
                
                auto headers = frame->findField("headers"); assert(headers);
                headers->setText(header_txt.c_str());
                
                auto values2 = frame->findField("values2"); assert(values2);
                snprintf(buf, sizeof(buf), values2_txt.c_str(), flags1.c_str());
                values2->setText(buf);
                
                auto values3 = frame->findField("values3"); assert(values3);
                snprintf(buf, sizeof(buf), values2_txt.c_str(), flags2.c_str());
                values3->setText(buf);
            } else {
                widget.setInvisible(true);
            }
		    });
		{
		    frame_left->addImage(
			    frame_left->getActualSize(),
			    0xffffffff,
			    "*images/ui/Main Menus/Play/LobbyBrowser/Lobby_Window_Left00.png",
			    "background"
		    );
            
            createGenericWindowDecorations(*frame_left);
            sizeWindowDecorations(*frame_left, SDL_Rect{24, 72, 256, 312});

		    auto label = frame_left->addField("label", 128);
		    label->setJustify(Field::justify_t::CENTER);
		    label->setSize(SDL_Rect{78, 48, 146, 22});
		    label->setFont(smallfont_outline);
		    label->setText("Lobby Info");

		    auto headers = frame_left->addField("headers", 1024);
		    headers->setHJustify(Field::justify_t::LEFT);
		    headers->setVJustify(Field::justify_t::TOP);
		    headers->setSize(SDL_Rect{30, 76, 244, 320});
		    headers->setFont(smallfont_outline);
		    headers->setColor(makeColor(106, 192, 159, 255));

		    auto values1 = frame_left->addField("values1", 1024);
		    values1->setHJustify(Field::justify_t::LEFT);
		    values1->setVJustify(Field::justify_t::TOP);
		    values1->setSize(SDL_Rect{30, 76, 244, 320});
		    values1->setFont(smallfont_outline);
		    values1->setColor(makeColor(183, 155, 119, 255));
            
            auto values2 = frame_left->addField("values2", 1024);
            values2->setHJustify(Field::justify_t::LEFT);
            values2->setVJustify(Field::justify_t::TOP);
            values2->setSize(SDL_Rect{30, 76, 122, 320});
            values2->setFont(smallfont_outline);
            values2->setColor(makeColor(183, 155, 119, 255));
            
            auto values3 = frame_left->addField("values3", 1024);
            values3->setHJustify(Field::justify_t::LEFT);
            values3->setVJustify(Field::justify_t::TOP);
            values3->setSize(SDL_Rect{152, 76, 122, 320});
            values3->setFont(smallfont_outline);
            values3->setColor(makeColor(183, 155, 119, 255));
		}

		auto online_tab = window->addButton("online_tab");
		online_tab->setSize(SDL_Rect{background->pos.x + 230, 70, 106, 38});
		online_tab->setHighlightColor(0);
		online_tab->setBorder(0);
		online_tab->setColor(0);
		online_tab->setText("ONLINE");
		online_tab->setFont(smallfont_outline);
		online_tab->setGlyphPosition(Widget::glyph_position_t::CENTERED_LEFT);
		online_tab->setButtonsOffset(SDL_Rect{ 0, 0, 0, 0 });
		online_tab->setWidgetSearchParent(window->getName());
		online_tab->addWidgetAction("MenuPageLeft", "online_tab");
		online_tab->addWidgetAction("MenuPageRight", "lan_tab");
		online_tab->addWidgetAction("MenuStart", "join_lobby");
		online_tab->addWidgetAction("MenuAlt1", "enter_code");
		online_tab->addWidgetAction("MenuAlt2", "refresh");
		online_tab->setWidgetBack("back_button");
		online_tab->setWidgetRight("lan_tab");
		online_tab->setWidgetDown("names");
#if defined(STEAMWORKS)
		online_tab->setCallback([](Button& button){
			auto frame = static_cast<Frame*>(button.getParent());
			auto interior = frame->findImage("interior");

			interior->path = "*images/ui/Main Menus/Play/LobbyBrowser/Lobby_InteriorWindow_Online01.png";
			mode = BrowserMode::Online;
			directConnect = false;
			refreshLobbyBrowser();
			});
#elif defined(USE_EOS)
#if defined(NINTENDO)
		online_tab->setCallback([](Button& button) {
			if (nxBeginParentalControls()) {
				static Button* store_button;
				store_button = static_cast<Button*>(&button);
				auto callback = [](bool success){
					if (success) {
						auto frame = static_cast<Frame*>(store_button->getParent());
						auto interior = frame->findImage("interior");
						interior->path = "*images/ui/Main Menus/Play/LobbyBrowser/Lobby_InteriorWindow_Online02.png";
						mode = BrowserMode::Online;
						directConnect = false;
						refreshLobbyBrowser();
					} else {
						nxInitWireless();
						errorPrompt("Unable to connect to Epic Online\nOnline play is not available.", "Okay", [](Button&) {closeMono();});
						multiplayer = SINGLE;
						soundError();
					}
				};
				if (isConnectedToEpic()) {
					callback(true);
				} else {
					loginToEpic(callback);
				}
			} else {
				soundError();
			}
			});
#else
		if (isConnectedToEpic()) {
			online_tab->setCallback([](Button& button) {
				auto frame = static_cast<Frame*>(button.getParent());
				auto interior = frame->findImage("interior");
				interior->path = "*images/ui/Main Menus/Play/LobbyBrowser/Lobby_InteriorWindow_Online02.png";
				mode = BrowserMode::Online;
				directConnect = false;
				refreshLobbyBrowser();
				});
		} else {
			online_tab->setCallback([](Button& button){soundError();});
			online_tab->setTextColor(makeColor(127, 127, 127, 255));
			online_tab->setTextHighlightColor(makeColor(127, 127, 127, 255));
		}
#endif // NINTENDO
#else
		online_tab->setCallback([](Button& button){soundError();});
		online_tab->setTextColor(makeColor(127, 127, 127, 255));
		online_tab->setTextHighlightColor(makeColor(127, 127, 127, 255));
#endif

		auto lan_tab = window->addButton("lan_tab");
		lan_tab->setSize(SDL_Rect{online_tab->getSize().x + online_tab->getSize().w + 4, 70, 128, 38});
		lan_tab->setHighlightColor(0);
		lan_tab->setBorder(0);
		lan_tab->setColor(0);
#if defined(NINTENDO)
		lan_tab->setText("WIRELESS");
#else
		lan_tab->setText("LAN");
#endif
		lan_tab->setFont(smallfont_outline);
		lan_tab->setGlyphPosition(Widget::glyph_position_t::CENTERED_RIGHT);
		lan_tab->setButtonsOffset(SDL_Rect{ 0, 0, 0, 0 });
		lan_tab->setWidgetSearchParent(window->getName());
		lan_tab->addWidgetAction("MenuPageLeft", "online_tab");
		lan_tab->addWidgetAction("MenuPageRight", "lan_tab");
		lan_tab->addWidgetAction("MenuStart", "join_lobby");
		lan_tab->addWidgetAction("MenuAlt1", "enter_code");
		lan_tab->addWidgetAction("MenuAlt2", "refresh");
		lan_tab->setWidgetBack("back_button");
		lan_tab->setWidgetLeft("online_tab");
		lan_tab->setWidgetRight("refresh");
		lan_tab->setWidgetDown("names");
		lan_tab->setCallback([](Button& button){
#ifdef NINTENDO
			nxEnableAutoSleep();
			nxEndParentalControls();
			nxShutdownWireless();
			logoutOfEpic();
			if (nxInitWireless()) {
				auto frame = static_cast<Frame*>(button.getParent());
				auto interior = frame->findImage("interior");
				interior->path = "*images/ui/Main Menus/Play/LobbyBrowser/Lobby_InteriorWindow_Wireless02.png";
				mode = BrowserMode::LAN;
				directConnect = true;
				refreshLobbyBrowser();
			} else {
				multiplayer = SINGLE;
				loadingsavegame = 0;
				soundError();
				closeNetworkInterfaces();
				destroyMainMenu();
				createMainMenu(false);
			}
#else
			auto frame = static_cast<Frame*>(button.getParent());
			auto interior = frame->findImage("interior");
#if defined(STEAMWORKS) && defined(USE_EOS)
			interior->path = "*images/ui/Main Menus/Play/LobbyBrowser/Lobby_InteriorWindow_Wireless01.png";
#else
			interior->path = "*images/ui/Main Menus/Play/LobbyBrowser/Lobby_InteriorWindow_Wireless02.png";
#endif
			mode = BrowserMode::LAN;
			directConnect = true;
			refreshLobbyBrowser();
#endif
			});


		static auto enter_code_fn = [](Button& button){
#ifdef NINTENDO
			if (directConnect) {
				soundError();
				return;
			}
#endif
		    static const char* guide_ipaddr = "Enter an IP address to connect to.";
		    static const char* guide_roomcode = "Enter the code to a lobby you wish to connect to.";
		    static const char* guide;
		    guide = directConnect ? guide_ipaddr : guide_roomcode;
		    static const char* tip_ipaddr = "Enter IP address...";
		    static const char* tip_roomcode = "Enter roomcode...";
		    static const char* tip;
		    tip = directConnect ? tip_ipaddr : tip_roomcode;
            textFieldPrompt(last_address, tip, guide, "Connect", "Cancel",
                [](Button&){ // connect
                    const char* address = closeTextField(); // only valid for one frame
                    if (directConnect) {
                        soundActivate();
                        (void)connectToServer(address, nullptr, LobbyType::LobbyLAN);
                    } else {
                        soundActivate();
                        (void)connectToServer(address, nullptr, LobbyType::LobbyOnline);
                    }
                },
                [](Button&){ // cancel
                    soundCancel();
                    assert(main_menu_frame);
                    auto window = main_menu_frame->findFrame("lobby_browser_window"); assert(window);
                    auto enter_code = window->findButton("enter_code"); assert(enter_code);
                    enter_code->select();
                    (void)closeTextField();
                });
		    };

		auto enter_code = window->addButton("enter_code");
		enter_code->setSize(SDL_Rect{ online_tab->getSize().x - 50, 454, 164, 62});
		enter_code->setBackground("*images/ui/Main Menus/Play/LobbyBrowser/UI_Button_Basic00.png");
		enter_code->setBackgroundHighlighted("*images/ui/Main Menus/Play/LobbyBrowser/UI_Button_BasicHigh00.png");
		enter_code->setBackgroundActivated("*images/ui/Main Menus/Play/LobbyBrowser/UI_Button_BasicPress00.png");
		enter_code->setHighlightColor(makeColor(255, 255, 255, 255));
		enter_code->setColor(makeColor(255, 255, 255, 255));
		enter_code->setText("Enter Lobby\nCode");
		enter_code->setFont(smallfont_outline);
		enter_code->setWidgetSearchParent(window->getName());
		enter_code->addWidgetAction("MenuPageLeft", "online_tab");
		enter_code->addWidgetAction("MenuPageRight", "lan_tab");
		enter_code->addWidgetAction("MenuStart", "join_lobby");
		enter_code->addWidgetAction("MenuAlt1", "enter_code");
		enter_code->addWidgetAction("MenuAlt2", "refresh");
		enter_code->setWidgetBack("back_button");
		enter_code->setWidgetRight("join_lobby");
#if defined(STEAMWORKS) && defined(USE_EOS)
		enter_code->setWidgetUp("crossplay");
#else
		enter_code->setWidgetUp("filter_settings");
#endif
		enter_code->setTickCallback([](Widget& widget){
#ifdef NINTENDO
			auto button = static_cast<Button*>(&widget);
		    if (mode == BrowserMode::Online) {
		        button->setText("Enter Lobby\nCode");
				button->setTextColor(makeColor(255, 255, 255, 255));
				button->setHighlightColor(makeColor(255, 255, 255, 255));
				button->setColor(makeColor(255, 255, 255, 255));
		    } else if (mode == BrowserMode::LAN) {
                button->setText("Enter Lobby\nCode");
				button->setTextColor(makeColor(127, 127, 127, 255));
				button->setHighlightColor(makeColor(127, 127, 127, 255));
				button->setColor(makeColor(127, 127, 127, 255));
		    }
#else
			auto button = static_cast<Button*>(&widget);
			if (mode == BrowserMode::Online) {
				button->setText("Enter Lobby\nCode");
			}
			else if (mode == BrowserMode::LAN) {
				button->setText("Enter IP\nAddress");
			}
#endif
		    });
		enter_code->setCallback(enter_code_fn);

		static auto join_lobby_fn = [](Button& button){
	        if (selectedLobby >= 0 && selectedLobby < lobbies.size()) {
                const auto& lobby = lobbies[selectedLobby];
                if (!lobby.locked) {
                    if (connectToServer(lobby.address.c_str(), &selectedLobby,
                        directConnect ? LobbyType::LobbyLAN : LobbyType::LobbyOnline)) {
                        // we only want to deselect the button if the
                        // "joining lobby" prompt actually raises
                        button.deselect();
                    }
                } else {
					errorPrompt("Unable to join lobby.\nLobby is locked.",
						"Okay", [](Button&) {soundCancel(); closeMono();});
                }
            } else {
	            errorPrompt("Select a lobby to join first.",
	                "Okay", [](Button&){soundCancel(); closeMono();});
            }
		    };

		auto join_lobby = window->addButton("join_lobby");
		join_lobby->setSize(SDL_Rect{enter_code->getSize().x + enter_code->getSize().w + 8, 454, 164, 62});
		join_lobby->setBackground("*images/ui/Main Menus/Play/LobbyBrowser/UI_Button_Basic00.png");
		join_lobby->setBackgroundHighlighted("*images/ui/Main Menus/Play/LobbyBrowser/UI_Button_BasicHigh00.png");
		join_lobby->setBackgroundActivated("*images/ui/Main Menus/Play/LobbyBrowser/UI_Button_BasicPress00.png");
		join_lobby->setHighlightColor(makeColor(255, 255, 255, 255));
		join_lobby->setColor(makeColor(255, 255, 255, 255));
		join_lobby->setText("Join Lobby");
		join_lobby->setFont(smallfont_outline);
		join_lobby->setWidgetSearchParent(window->getName());
		join_lobby->addWidgetAction("MenuPageLeft", "online_tab");
		join_lobby->addWidgetAction("MenuPageRight", "lan_tab");
		join_lobby->addWidgetAction("MenuStart", "join_lobby");
		join_lobby->addWidgetAction("MenuAlt1", "enter_code");
		join_lobby->addWidgetAction("MenuAlt2", "refresh");
		join_lobby->setWidgetBack("back_button");
		join_lobby->setWidgetLeft("enter_code");
#if defined(STEAMWORKS) && defined(USE_EOS)
		join_lobby->setWidgetUp("crossplay");
#else
		join_lobby->setWidgetUp("filter_settings");
#endif
		join_lobby->setCallback(join_lobby_fn);

		static auto tick_callback = [](Widget& widget){
		    widget.setHideSelectors(!inputs.hasController(widget.getOwner()));
			if (!gui->findSelectedWidget(widget.getOwner())) {
				widget.select();
			}
	        };

	    constexpr Uint32 highlightColor = makeColor(22, 25, 30, 255);
	    constexpr Uint32 activatedColor = makeColor(30, 25, 22, 255);

		SDL_Rect prevColumnSize;
        // name column
        {
		    auto name_column_header = window->addField("name_column_header", 64);
		    name_column_header->setHJustify(Field::justify_t::LEFT);
		    name_column_header->setVJustify(Field::justify_t::TOP);
		    name_column_header->setFont(smallfont_no_outline);
		    name_column_header->setSize(SDL_Rect{354, 116, 380, 20});
		    name_column_header->setColor(makeColor(106, 192, 159, 255));
		    name_column_header->setText(" Lobby Name");
			name_column_header->setTickCallback([](Widget& widget) {
				Field* name_column_header = static_cast<Field*>(&widget);
				if ( lobbyFiltersEnabled )
				{
					auto names = static_cast<Frame*>(widget.getParent())->findFrame("names");
					int lobbiesFiltered = std::max(0, (int)lobbies.size() - (int)names->getEntries().size());
					if ( lobbiesFiltered > 0 )
					{
						char buf[64] = {'\0'};
						snprintf(buf, sizeof(buf), " Lobby Name (%d hidden)", lobbiesFiltered);
						name_column_header->setText(buf);
					}
					else
					{
						name_column_header->setText(" Lobby Name");
					}
				}
				else
				{
					name_column_header->setText(" Lobby Name");
				}
			});

		    auto list = window->addFrame("names");
		    list->setScrollBarsEnabled(false);
#if defined(STEAMWORKS) && defined(USE_EOS)
			list->setSize(SDL_Rect{ name_column_header->getSize().x - 4, 140, 384, 200});
			list->setActualSize(SDL_Rect{ 0, 0, 384, 200 });
#else
			list->setSize(SDL_Rect{ name_column_header->getSize().x - 4, 140, 384, 234 });
			list->setActualSize(SDL_Rect{ 0, 0, 384, 234 });
#endif
			prevColumnSize = list->getSize();
		    list->setFont(smallfont_no_outline);
		    list->setColor(0);
		    list->setBorder(0);
		    list->setEntrySize(18);
		    list->setSelectedEntryColor(highlightColor);
		    list->setActivatedEntryColor(activatedColor);

			static auto tick_callback = [](Widget& widget) {
				widget.setHideSelectors(!inputs.hasController(widget.getOwner()));
				Frame* frame = static_cast<Frame*>(&widget);
				if ( frame->isActivated() )
				{
					widget.setHideSelectors(true);
					frame->setGlyphPosition(Widget::UPPER_LEFT);
					SDL_Rect rect{ 0, 0, 0, 0 };
					rect.y = frame->getSelection() * frame->getEntrySize();
					rect.y -= frame->getActualSize().y; // subtract scroll
					rect.x -= 7;
					rect.y += 7;
					if ( rect.y < 0 ) {
						frame->setHideGlyphs(true);
					}
					else if ( rect.y > frame->getSize().h )
					{
						frame->setHideGlyphs(true);
					}
					else
					{
						frame->setHideGlyphs(false);
					}
					//rect.y = std::max(0, rect.y);
					//rect.y = std::min(frame->getSize().h, rect.y);
					frame->setButtonsOffset(rect);
				}
				else
				{
					frame->setHideGlyphs(false);
					frame->setButtonsOffset(SDL_Rect{ 0, 0, 192, 0 });
					frame->setGlyphPosition(Widget::CENTERED_BOTTOM);
				}
				if ( !gui->findSelectedWidget(widget.getOwner()) ) {
					widget.select();
				}
			};

		    list->setTickCallback(tick_callback);
		    list->setWidgetSearchParent(window->getName());
		    list->addWidgetMovement("MenuListCancel", list->getName());
		    list->addWidgetAction("MenuPageLeft", "online_tab");
		    list->addWidgetAction("MenuPageRight", "lan_tab");
		    list->addWidgetAction("MenuStart", "join_lobby");
		    list->addWidgetAction("MenuAlt1", "enter_code");
		    list->addWidgetAction("MenuAlt2", "refresh");
		    list->setWidgetBack("back_button");
		    //list->setWidgetUp("online_tab");
		    //list->setWidgetRight("players");
		    list->setWidgetDown("filter_settings");
		    list->addSyncScrollTarget("players");
		    //list->addSyncScrollTarget("pings");
			list->addSyncScrollTarget("versions");
			list->setSelectorOffset(SDL_Rect{ 0, 0, 150, 0 });
			list->setButtonsOffset(SDL_Rect{ 0, 0, 192, 0 });
		    list->select();

			auto divider = window->addImage(
				SDL_Rect{ list->getSize().x, 116, 6, 256 },
				0xffffffff,
				"*#images/ui/Main Menus/Play/LobbyBrowser/Lobby_InteriorWindow_Dividers.png",
				"divider");
			divider->ontop = true;
		}

        // players column
        {
		    auto players_column_header = window->addField("players_column_header", 32);
		    players_column_header->setHJustify(Field::justify_t::LEFT);
		    players_column_header->setVJustify(Field::justify_t::TOP);
		    players_column_header->setFont(smallfont_no_outline);
		    players_column_header->setSize(SDL_Rect{ prevColumnSize.x + prevColumnSize.w + 4, 116, 76, 20});
		    players_column_header->setColor(makeColor(106, 192, 159, 255));
		    players_column_header->setText(" Players");

		    auto list = window->addFrame("players");
		    list->setScrollBarsEnabled(false);
#if defined(STEAMWORKS) && defined(USE_EOS)
			list->setSize(SDL_Rect{ players_column_header->getSize().x - 4, 140, 78, 200});
			list->setActualSize(SDL_Rect{ 0, 0, 78, 200 });
#else
			list->setSize(SDL_Rect{ players_column_header->getSize().x - 4, 140, 78, 234 });
			list->setActualSize(SDL_Rect{ 0, 0, 78, 234 });
#endif
			prevColumnSize = list->getSize();
		    list->setFont(smallfont_no_outline);
		    list->setColor(0);
		    list->setBorder(0);
		    list->setEntrySize(18);
		    list->setSelectedEntryColor(highlightColor);
		    list->setActivatedEntryColor(activatedColor);
		    list->setTickCallback(tick_callback);
		    list->setWidgetSearchParent(window->getName());
		    list->addWidgetMovement("MenuListCancel", list->getName());
		    list->addWidgetAction("MenuPageLeft", "online_tab");
		    list->addWidgetAction("MenuPageRight", "lan_tab");
		    list->addWidgetAction("MenuStart", "join_lobby");
		    list->addWidgetAction("MenuAlt1", "enter_code");
		    list->addWidgetAction("MenuAlt2", "refresh");
		    list->setWidgetBack("back_button");
		    list->setWidgetUp("online_tab");
		    //list->setWidgetRight("pings");
			//list->setWidgetRight("versions");
		    list->setWidgetLeft("names");
		    list->setWidgetDown("filter_settings");
		    list->addSyncScrollTarget("names");
		    //list->addSyncScrollTarget("pings");
			list->addSyncScrollTarget("versions");

			auto divider = window->addImage(
				SDL_Rect{ list->getSize().x, 116, 6, 256 },
				0xffffffff,
				"*#images/ui/Main Menus/Play/LobbyBrowser/Lobby_InteriorWindow_Dividers.png",
				"divider");
			divider->ontop = true;
		}

		// version column
		{
			auto version_column_header = window->addField("version_column_header", 32);
			version_column_header->setHJustify(Field::justify_t::LEFT);
			version_column_header->setVJustify(Field::justify_t::TOP);
			version_column_header->setFont(smallfont_no_outline);
			version_column_header->setSize(SDL_Rect{ prevColumnSize.x + prevColumnSize.w + 4, 116, 70, 20 });
			version_column_header->setColor(makeColor(106, 192, 159, 255));
			version_column_header->setText(" Version");

			auto list = window->addFrame("versions");
			list->setScrollBarsEnabled(false);
#if defined(STEAMWORKS) && defined(USE_EOS)
			list->setSize(SDL_Rect{ version_column_header->getSize().x - 4, 140, version_column_header->getSize().w + 4, 200 });
			list->setActualSize(SDL_Rect{ 0, 0, 52, 200 });
#else
			list->setSize(SDL_Rect{ version_column_header->getSize().x - 4, 140, version_column_header->getSize().w + 4, 234 });
			list->setActualSize(SDL_Rect{ 0, 0, 52, 234 });
#endif
			prevColumnSize = list->getSize();
			list->setFont(smallfont_no_outline);
			list->setColor(0);
			list->setBorder(0);
			list->setEntrySize(18);
			list->setSelectedEntryColor(highlightColor);
			list->setActivatedEntryColor(activatedColor);
			list->setTickCallback(tick_callback);
			list->setWidgetSearchParent(window->getName());
			list->addWidgetMovement("MenuListCancel", list->getName());
			list->addWidgetAction("MenuPageLeft", "online_tab");
			list->addWidgetAction("MenuPageRight", "lan_tab");
			list->addWidgetAction("MenuStart", "join_lobby");
			list->addWidgetAction("MenuAlt1", "enter_code");
			list->addWidgetAction("MenuAlt2", "refresh");
			list->setWidgetBack("back_button");
			list->setWidgetUp("online_tab");
			list->setWidgetLeft("names");
			list->setWidgetDown("filter_settings");
			list->addSyncScrollTarget("names");
			list->addSyncScrollTarget("players");

			auto divider = window->addImage(
				SDL_Rect{ list->getSize().x, 116, 6, 256 },
				0xffffffff,
				"*#images/ui/Main Menus/Play/LobbyBrowser/Lobby_InteriorWindow_Dividers.png",
				"divider");
			divider->ontop = true;
		}

        // ping column
		/*{
		    auto ping_column_header = window->addField("ping_column_header", 32);
		    ping_column_header->setHJustify(Field::justify_t::LEFT);
		    ping_column_header->setVJustify(Field::justify_t::TOP);
		    ping_column_header->setFont(smallfont_no_outline);
		    ping_column_header->setSize(SDL_Rect{ prevColumnSize.x + prevColumnSize.w + 4, 116, 48, 20});
		    ping_column_header->setColor(makeColor(106, 192, 159, 255));
		    ping_column_header->setText(" Ping");

		    auto list = window->addFrame("pings");
		    list->setScrollBarsEnabled(false);
#if defined(STEAMWORKS) && defined(USE_EOS)
			list->setSize(SDL_Rect{ ping_column_header->getSize().x - 4, 140, 52, 200});
			list->setActualSize(SDL_Rect{ 0, 0, 52, 200 });
#else
			list->setSize(SDL_Rect{ ping_column_header->getSize().x - 4, 140, 52, 234 });
			list->setActualSize(SDL_Rect{ 0, 0, 52, 234 });
#endif
			prevColumnSize = list->getSize();
		    list->setFont(smallfont_no_outline);
		    list->setColor(0);
		    list->setBorder(0);
		    list->setEntrySize(18);
		    list->setSelectedEntryColor(highlightColor);
		    list->setActivatedEntryColor(activatedColor);
		    list->setTickCallback(tick_callback);
		    list->setWidgetSearchParent(window->getName());
		    list->addWidgetMovement("MenuListCancel", list->getName());
		    list->addWidgetAction("MenuPageLeft", "online_tab");
		    list->addWidgetAction("MenuPageRight", "lan_tab");
		    list->addWidgetAction("MenuStart", "join_lobby");
		    list->addWidgetAction("MenuAlt1", "enter_code");
		    list->addWidgetAction("MenuAlt2", "refresh");
		    list->setWidgetBack("back_button");
		    list->setWidgetUp("online_tab");
		    list->setWidgetLeft("players");
		    list->setWidgetDown("filter_settings");
		    list->addSyncScrollTarget("names");
		    list->addSyncScrollTarget("players");
		}*/

		auto refresh = window->addButton("refresh");
		refresh->setSize(SDL_Rect{ prevColumnSize.x + prevColumnSize.w, 62, 40, 40 });
		refresh->setBackground("*images/ui/Main Menus/Play/LobbyBrowser/Lobby_Button_Refresh00.png");
		refresh->setBackgroundHighlighted("*images/ui/Main Menus/Play/LobbyBrowser/Lobby_Button_RefreshHigh00.png");
		refresh->setBackgroundActivated("*images/ui/Main Menus/Play/LobbyBrowser/Lobby_Button_RefreshPress00.png");
		refresh->setHighlightColor(makeColor(255, 255, 255, 255));
		refresh->setColor(makeColor(255, 255, 255, 255));
		refresh->setWidgetSearchParent(window->getName());
		refresh->addWidgetAction("MenuPageLeft", "online_tab");
		refresh->addWidgetAction("MenuPageRight", "lan_tab");
		refresh->addWidgetAction("MenuStart", "join_lobby");
		refresh->addWidgetAction("MenuAlt1", "enter_code");
		refresh->addWidgetAction("MenuAlt2", "refresh");
		refresh->setWidgetBack("back_button");
		refresh->setWidgetLeft("lan_tab");
		refresh->setWidgetDown("names");
		refresh->setCallback([](Button&) {refreshLobbyBrowser(); });

		auto lobby_slider_topper = window->addImage(
			SDL_Rect{ prevColumnSize.x + prevColumnSize.w, 116, 38, 20 },
			0xffffffff,
			"*images/ui/Main Menus/Play/LobbyBrowser/Lobby_Slider_Topper00.png",
			"lobby_slider_topper"
		);

		auto slider = window->addSlider("scroll_slider");
		slider->setOrientation(Slider::SLIDER_VERTICAL);
		slider->setGlyphPosition(Widget::glyph_position_t::CENTERED);
		slider->setRailSize(SDL_Rect{ prevColumnSize.x + prevColumnSize.w, 138, 38, 234});
		slider->setHandleSize(SDL_Rect{0, 0, 34, 34});
		slider->setRailImage("*images/ui/Main Menus/Play/LobbyBrowser/Lobby_Slider_Backing01B.png");
		slider->setHandleImage("*images/ui/Main Menus/Play/LobbyBrowser/UI_Slider_Boulder00.png");
		slider->setBorder(24);
		slider->setWidgetSearchParent(window->getName());
	    slider->addWidgetAction("MenuPageLeft", "online_tab");
	    slider->addWidgetAction("MenuPageRight", "lan_tab");
	    slider->addWidgetAction("MenuStart", "join_lobby");
	    slider->addWidgetAction("MenuAlt1", "enter_code");
	    slider->addWidgetAction("MenuAlt2", "refresh");
	    slider->setWidgetBack("back_button");
	    slider->setWidgetUp("wireless_tab");
	    slider->setWidgetDown("join_lobby");
	    //slider->setWidgetLeft("pings");
		slider->setWidgetLeft("names");
		slider->setCallback([](Slider& slider){
			Frame* frame = static_cast<Frame*>(slider.getParent()); assert(frame);
			{
			    Frame* column = frame->findFrame("names"); assert(column);
			    auto actualSize = column->getActualSize();
			    actualSize.y = slider.getValue();
			    column->setActualSize(actualSize);
			}
			{
			    Frame* column = frame->findFrame("players"); assert(column);
			    auto actualSize = column->getActualSize();
			    actualSize.y = slider.getValue();
			    column->setActualSize(actualSize);
			}
			/*{
			    Frame* column = frame->findFrame("pings"); assert(column);
			    auto actualSize = column->getActualSize();
			    actualSize.y = slider.getValue();
			    column->setActualSize(actualSize);
			}*/
			{
				Frame* column = frame->findFrame("versions"); assert(column);
				auto actualSize = column->getActualSize();
				actualSize.y = slider.getValue();
				column->setActualSize(actualSize);
			}
			slider.updateHandlePosition();
			});
		slider->setTickCallback([](Widget& widget){
			Slider* slider = static_cast<Slider*>(&widget);
			Frame* frame = static_cast<Frame*>(slider->getParent()); assert(frame);
			Frame* names = frame->findFrame("names"); assert(names);
			auto actualSize = names->getActualSize();
			slider->setValue(actualSize.y);
			slider->updateHandlePosition();
			});

		auto filter_settings_fn = [](Button& button){
			auto frame = static_cast<Frame*>(button.getParent()); assert(frame);
			auto frame_right = frame->findFrame("frame_right"); assert(frame_right);

			lobbyFiltersEnabled = false;
			for ( int i = 0; i < numFilters; ++i )
			{
				if ( lobbyFilters[i] != Filter::UNCHECKED )
				{
					lobbyFiltersEnabled = true;
					break;
				}
			}

			if ( button.isSelected() && !inputs.getVirtualMouse(getMenuOwner())->draw_cursor )
			{
				frame_right->setInvisible(false);
			}
			else
			{
				frame_right->setInvisible(!lobbyFiltersEnabled && !frame_right->isInvisible());
			}

			/*clearLobbies();
			for (auto& lobby : lobbies) {
				addLobby(lobby);
			}*/

			if (!frame_right->isInvisible()) {
				if (!inputs.getVirtualMouse(getMenuOwner())->draw_cursor) {
					auto selectedWidget = frame_right->findSelectedWidget(frame_right->getOwner());
					if ( selectedWidget )
					{
						button.select();
						soundMove();
					}
					else
					{
						auto checkbox = frame_right->findButton("filter_checkbox0");
						if (checkbox) {
							checkbox->select();
						}
						soundActivate();
					}
				}
			}
			else {
				soundCancel();
				button.select();
			}
			};

#if defined(STEAMWORKS) && defined(USE_EOS)
		auto filter_settings = window->addButton("filter_settings");
		filter_settings->setSize(SDL_Rect{online_tab->getSize().x + 36, 344, 160, 32});
		filter_settings->setBackground("*images/ui/Main Menus/Play/LobbyBrowser/Lobby_Button_FilterSettings00.png");
		filter_settings->setBackgroundHighlighted("*images/ui/Main Menus/Play/LobbyBrowser/Lobby_Button_FilterSettingsHigh00.png");
		filter_settings->setBackgroundActivated("*images/ui/Main Menus/Play/LobbyBrowser/Lobby_Button_FilterSettingsPress00.png");
		filter_settings->setHighlightColor(makeColor(255, 255, 255, 255));
		filter_settings->setColor(makeColor(255, 255, 255, 255));
		filter_settings->setText("Filter Settings");
		filter_settings->setFont(smallfont_outline);
		filter_settings->setWidgetSearchParent(window->getName());
		filter_settings->addWidgetAction("MenuPageLeft", "online_tab");
		filter_settings->addWidgetAction("MenuPageRight", "lan_tab");
		filter_settings->addWidgetAction("MenuStart", "join_lobby");
		filter_settings->addWidgetAction("MenuAlt1", "enter_code");
		filter_settings->addWidgetAction("MenuAlt2", "refresh");
		filter_settings->setWidgetBack("back_button");
		filter_settings->setWidgetDown("crossplay");
		filter_settings->setWidgetUp("names");
		filter_settings->setCallback(filter_settings_fn);

		auto crossplay_label = window->addField("crossplay_label", 128);
		crossplay_label->setJustify(Field::justify_t::CENTER);
		crossplay_label->setSize(SDL_Rect{ online_tab->getSize().x - 14, 378, 96, 48});
		crossplay_label->setFont(smallfont_outline);
		crossplay_label->setText("Crossplay");

		auto crossplay = window->addButton("crossplay");
		crossplay->setSize(SDL_Rect{ filter_settings->getSize().x + 50, 378, 158, 48});
		crossplay->setJustify(Button::justify_t::CENTER);
		crossplay->setPressed(LobbyHandler.crossplayEnabled);
		crossplay->setStyle(Button::style_t::STYLE_TOGGLE);
		crossplay->setBackground("*images/ui/Main Menus/Play/LobbyBrowser/Lobby_Toggle_Off00.png");
		crossplay->setBackgroundActivated("*images/ui/Main Menus/Play/LobbyBrowser/Lobby_Toggle_On00.png");
		crossplay->setHighlightColor(makeColor(255,255,255,255));
		crossplay->setColor(makeColor(255,255,255,255));
		crossplay->setTextHighlightColor(makeColor(255,255,255,255));
		crossplay->setTextColor(makeColor(255,255,255,255));
		crossplay->setText("Off          On");
		crossplay->setFont(smallfont_outline);
		crossplay->setWidgetSearchParent(window->getName());
		crossplay->addWidgetAction("MenuPageLeft", "online_tab");
		crossplay->addWidgetAction("MenuPageRight", "lan_tab");
		crossplay->addWidgetAction("MenuStart", "join_lobby");
		crossplay->addWidgetAction("MenuAlt1", "enter_code");
		crossplay->addWidgetAction("MenuAlt2", "refresh");
		crossplay->setWidgetBack("back_button");
		crossplay->setWidgetDown("join_lobby");
		crossplay->setWidgetUp("filter_settings");
		crossplay->setCallback([](Button& button) {
			soundToggle();
			if (button.isPressed() && !LobbyHandler.crossplayEnabled) {
				loginToEpic(nullptr);
			}
			else if (!button.isPressed() && LobbyHandler.crossplayEnabled) {
				logoutOfEpic();
			}
			});
#else
		auto filter_settings = window->addButton("filter_settings");
		filter_settings->setSize(SDL_Rect{ online_tab->getSize().x + 38, 384, 158, 44});
		filter_settings->setFont(smallfont_outline);
		filter_settings->setText("Filter Settings");
		filter_settings->setJustify(Button::justify_t::CENTER);
		filter_settings->setCallback(filter_settings_fn);
		filter_settings->setBackground("*images/ui/Main Menus/Settings/Settings_Button_Customize00.png");
		filter_settings->setBackgroundHighlighted("*images/ui/Main Menus/Settings/Settings_Button_CustomizeHigh00.png");
		filter_settings->setBackgroundActivated("*images/ui/Main Menus/Settings/Settings_Button_CustomizePress00.png");
		filter_settings->setHighlightColor(makeColor(255, 255, 255, 255));
		filter_settings->setColor(makeColor(255, 255, 255, 255));
		filter_settings->setTextHighlightColor(makeColor(255, 255, 255, 255));
		filter_settings->setTextColor(makeColor(255, 255, 255, 255));
		filter_settings->setWidgetSearchParent(window->getName());
		filter_settings->addWidgetAction("MenuPageLeft", "online_tab");
		filter_settings->addWidgetAction("MenuPageRight", "lan_tab");
		filter_settings->addWidgetAction("MenuStart", "join_lobby");
		filter_settings->addWidgetAction("MenuAlt1", "enter_code");
		filter_settings->addWidgetAction("MenuAlt2", "refresh");
		filter_settings->setWidgetBack("back_button");
		filter_settings->setWidgetDown("enter_code");
		filter_settings->setWidgetUp("names");
#endif

#ifdef NDEBUG
	    // scan for lobbies immediately
	    refresh->activate();
#else
        static ConsoleVariable<bool> cvar_testLobbyBrowser("/test_lobby_browser", false,
            "Fill the lobby browser with bogus entries to test its features.");
        if (*cvar_testLobbyBrowser) {
            // test lobbies
            std::string allBlanks;
            for (int c = 0; c < 64; ++c) {
                allBlanks.append(u8"\uFFFD");
                if (c && c % 12 == 0) {
                    allBlanks.append("\n");
                }
            }
			LobbyInfo testLobbies[] = {
				{"Ben", VERSION, 1, 50, false, SV_FLAG_CHEATS},
				{"Sheridan", VERSION, 3, 50, false, SV_FLAG_FRIENDLYFIRE | SV_FLAG_MINOTAURS | SV_FLAG_HUNGER | SV_FLAG_TRAPS | SV_FLAG_CLASSIC},
				{"Paulie", VERSION, 2, 250, false, SV_FLAG_HARDCORE},
				{"Fart_Face", VERSION, 1, 420, false, SV_FLAG_KEEPINVENTORY | SV_FLAG_LIFESAVING},
				{"Tim", VERSION, 3, 90, true, SV_FLAG_MINOTAURS | SV_FLAG_KEEPINVENTORY},
				{"Johnny", VERSION, 3, 30, false, SV_FLAG_FRIENDLYFIRE},
				{"Boaty McBoatFace", VERSION, 2, 20, false},
				{"RIP_Morgan_", VERSION, 0, 120, false},
				{"This is the longest name we can fit in a Barony lobby for real.", VERSION, 4, 150, false, 0xffffffff},
				{allBlanks.c_str(), VERSION, 4, 420, false, 0xffffffff},
				{"16 PLAYER SMASH FEST", VERSION, 16, 90, false},
				{"ur mom", VERSION, 16, 160, true},
				{"waow more lobbies", VERSION, 16, 160, true},
				{"snobby lobby", VERSION, 16, 260, true},
				{"gAmERs RiSe uP!!", VERSION, 16, 0, false},
				{"a very unsuspicious lobby", VERSION, 2, 130, false},
				{"cool lobby bro!", VERSION, 3, 240, false},
				{"Ben", VERSION, 1, 50, false, SV_FLAG_CHEATS},
				{"Sheridan", VERSION, 3, 50, false, SV_FLAG_FRIENDLYFIRE | SV_FLAG_MINOTAURS | SV_FLAG_HUNGER | SV_FLAG_TRAPS | SV_FLAG_CLASSIC},
				{"Paulie", VERSION, 2, 250, false, SV_FLAG_HARDCORE},
				{"Fart_Face", VERSION, 1, 420, false, SV_FLAG_KEEPINVENTORY | SV_FLAG_LIFESAVING},
				{"Tim", VERSION, 3, 90, true, SV_FLAG_MINOTAURS | SV_FLAG_KEEPINVENTORY},
				{"Johnny", VERSION, 3, 30, false, SV_FLAG_FRIENDLYFIRE},
				{"Boaty McBoatFace", VERSION, 2, 20, false},
				{"RIP_Morgan_", VERSION, 0, 120, false},
				{"This is the longest name we can fit in a Barony lobby for real.", VERSION, 4, 150, false, 0xffffffff},
				{allBlanks.c_str(), VERSION, 4, 420, false, 0xffffffff},
				{"16 PLAYER SMASH FEST", VERSION, 16, 90, false},
				{"ur mom", VERSION, 16, 160, true},
				{"waow more lobbies", VERSION, 16, 160, true},
				{"snobby lobby", VERSION, 16, 260, true},
				{"gAmERs RiSe uP!!", VERSION, 16, 0, false},
				{"a very unsuspicious lobby", VERSION, 2, 130, false},
				{"cool lobby bro!", VERSION, 3, 240, false},
			};
			for (auto& lobby : testLobbies) {
				addLobby(lobby);
			}
		} else {
		    refresh->activate();
		}
#endif
	}

/******************************************************************************/

    static void createHallofTrialsMenu() {
        assert(main_menu_frame);

        tutorial_map_destination = "tutorial_hub";

		auto window = main_menu_frame->addFrame("hall_of_trials_menu");
		window->setSize(SDL_Rect{
			(Frame::virtualScreenX - 1164) / 2,
			(Frame::virtualScreenY - 716) / 2,
			1164,
			716});
		window->setActualSize(SDL_Rect{0, 0, 1164, 716});
		window->setBorder(0);
		window->setColor(0);

		auto background = window->addImage(
			SDL_Rect{16, 0, 1130, 714},
			0xffffffff,
			"*images/ui/Main Menus/Play/HallofTrials/HoT_Window_00.png",
			"background"
		);

		auto timber = window->addImage(
			SDL_Rect{0, 716 - 586, 1164, 586},
			0xffffffff,
			"*images/ui/Main Menus/Play/HallofTrials/HoT_Window_OverlayScaffold_00.png",
			"timber"
		);
		timber->ontop = true;

		auto subwindow = window->addFrame("subwindow");
		subwindow->setSize(SDL_Rect{22, 142, 1118, 476});
		subwindow->setActualSize(SDL_Rect{0, 0, 1118, 774});
		subwindow->setBorder(0);
		subwindow->setColor(0);

		auto rock_background = subwindow->addImage(
			subwindow->getActualSize(),
			makeColor(255, 255, 255, 255),
			"*images/ui/Main Menus/Play/HallofTrials/Settings_Window_06_BGPattern.png",
			"rock_background"
		);
		rock_background->tiled = true;

		auto gradient_background = subwindow->addImage(
			SDL_Rect{0, 0, 1164, 476},
			makeColor(255, 255, 255, 255),
			"*images/ui/Main Menus/Play/HallofTrials/HoT_Window_02_BGGradient.png",
			"gradient_background"
		);

		auto window_title = window->addField("title", 64);
		window_title->setFont(banner_font);
		window_title->setSize(SDL_Rect{412, 24, 338, 24});
		window_title->setJustify(Field::justify_t::CENTER);
		window_title->setText("TUTORIALS");

		auto subtitle = window->addField("subtitle", 1024);
		subtitle->setFont(bigfont_no_outline);
		subtitle->setColor(makeColor(170, 134, 102, 255));
		subtitle->setSize(SDL_Rect{242, 74, 684, 50});
		subtitle->setJustify(Field::justify_t::CENTER);
		subtitle->setText(
		    u8"Take on 10 challenges that teach and test your adventuring\n"
		    u8"skills, preparing you to take on the dungeon");

		(void)createBackWidget(window, [](Button& button){
			soundCancel();
			auto frame = static_cast<Frame*>(button.getParent());
			frame = static_cast<Frame*>(frame->getParent());
			frame->removeSelf();
			assert(main_menu_frame);
			auto dimmer = main_menu_frame->findFrame("dimmer"); assert(dimmer);
			auto window = dimmer->findFrame("play_game_window"); assert(window);
		    auto hall_of_trials_button = window->findButton("hall_of_trials"); assert(hall_of_trials_button);
			hall_of_trials_button->select();
			});

		auto banner = subwindow->addImage(
		    SDL_Rect{0, 88, 1118, 42},
		    0xffffffff,
		    "*images/ui/Main Menus/Play/HallofTrials/HoT_Subtitle_BGRed_00.png",
		    "banner"
		);

		auto banner_trial = subwindow->addField("banner_trial", 32);
		banner_trial->setSize(SDL_Rect{48, 88, 66, 42});
		banner_trial->setJustify(Field::justify_t::CENTER);
		banner_trial->setFont(bigfont_outline);
		banner_trial->setText("Trial");

		auto banner_time = subwindow->addField("banner_trial", 32);
		banner_time->setSize(SDL_Rect{920, 88, 116, 42});
		banner_time->setJustify(Field::justify_t::CENTER);
		banner_time->setFont(bigfont_outline);
		banner_time->setText("Best Time");

		SDL_Rect fleur_positions[4] = {
		    { 22, 94, 26, 30 },
		    { 114, 94, 26, 30 },
		    { 894, 94, 26, 30 },
		    { 1036, 94, 26, 30 },
		};
		constexpr int num_fleurs = sizeof(fleur_positions) / sizeof(fleur_positions[0]);
		for (int c = 0; c < num_fleurs; ++c) {
		    (void)subwindow->addImage(
		        fleur_positions[c],
		        0xffffffff,
		        "*images/ui/Main Menus/Play/HallofTrials/HoT_Subtitle_Flower_00.png",
		        (std::string("fleur") + std::to_string(c)).c_str()
		    );
		}

		auto slider = subwindow->addSlider("scroll_slider");
		slider->setBorder(48);
		slider->setOrientation(Slider::SLIDER_VERTICAL);
		slider->setRailSize(SDL_Rect{1118 - 54, 0, 54, 476});
		slider->setRailImage("*images/ui/Main Menus/Play/HallofTrials/HoT_Scroll_Bar_01.png");
		slider->setHandleSize(SDL_Rect{0, 0, 34, 34});
		slider->setHandleImage("*images/ui/Main Menus/Play/HallofTrials/HoT_Scroll_Boulder_00.png");
		slider->setGlyphPosition(Button::glyph_position_t::CENTERED);
		slider->setCallback([](Slider& slider){
			Frame* frame = static_cast<Frame*>(slider.getParent());
			auto actualSize = frame->getActualSize();
			actualSize.y = slider.getValue();
			frame->setActualSize(actualSize);
			auto railSize = slider.getRailSize();
			railSize.y = actualSize.y;
			slider.setRailSize(railSize);
			slider.updateHandlePosition();
			auto gradient_background = frame->findImage("gradient_background");
			assert(gradient_background);
			gradient_background->pos.y = actualSize.y;
			});
		slider->setTickCallback([](Widget& widget){
			Slider* slider = static_cast<Slider*>(&widget);
			Frame* frame = static_cast<Frame*>(slider->getParent());
			auto actualSize = frame->getActualSize();
			slider->setValue(actualSize.y);
			auto railSize = slider->getRailSize();
			railSize.y = actualSize.y;
			slider->setRailSize(railSize);
			slider->updateHandlePosition();
			auto gradient_background = frame->findImage("gradient_background");
			assert(gradient_background);
			gradient_background->pos.y = actualSize.y;
			});
		slider->setValue(0.f);
		slider->setMinValue(0.f);
		slider->setMaxValue(subwindow->getActualSize().h - subwindow->getSize().h);
		slider->setWidgetSearchParent("hall_of_trials_menu");
		slider->setWidgetLeft("tutorial_hub");
        slider->addWidgetAction("MenuStart", "enter");
        slider->addWidgetAction("MenuAlt1", "reset");
        slider->addWidgetAction("MenuCancel", "back_button");

        static auto make_button = [](Frame& subwindow, int y, const char* name, const char* label, const char* sublabel){
            auto button = subwindow.addButton(name);
            button->setSize(SDL_Rect{8, y, 884, 52});
            button->setBackground("*images/ui/Main Menus/Play/HallofTrials/HoT_Hub_NameUnselected_00.png");
            button->setBackgroundHighlighted("*images/ui/Main Menus/Play/HallofTrials/HoT_Hub_NameSelected_00.png");
            button->setHighlightColor(0xffffffff);
            button->setColor(0xffffffff);
            button->setVJustify(Button::justify_t::CENTER);
            button->setHJustify(Button::justify_t::LEFT);
            button->setFont(bigfont_no_outline);
            button->setText(label);
            button->setGlyphPosition(Widget::glyph_position_t::CENTERED_RIGHT);
            button->setCallback([](Button& button){
                soundActivate();
                if (tutorial_map_destination != button.getName()) {
                    tutorial_map_destination = button.getName();
                } else {
                    auto frame = static_cast<Frame*>(button.getParent()); assert(frame);
                    frame = static_cast<Frame*>(frame->getParent()); assert(frame);
                    auto enter = frame->findButton("enter"); assert(enter);
                    enter->activate();
                }
                });
            button->setTickCallback([](Widget& widget){
                std::string sublabel_name = widget.getName();
                sublabel_name.append("_sublabel_background");
                auto frame = static_cast<Frame*>(widget.getParent()); assert(frame);
                auto button = static_cast<Button*>(&widget); assert(button);
                auto sublabel_background = frame->findImage(sublabel_name.c_str());
                if (sublabel_background) {
                    if (button->isSelected() || tutorial_map_destination == widget.getName()) {
                        sublabel_background->path = "*images/ui/Main Menus/Play/HallofTrials/HoT_Hub_TimeSelected_00.png";
                        button->setBackground("*images/ui/Main Menus/Play/HallofTrials/HoT_Hub_NameSelected_00.png");
                    } else {
                        sublabel_background->path = "*images/ui/Main Menus/Play/HallofTrials/HoT_Hub_TimeUnselected_00.png";
                        button->setBackground("*images/ui/Main Menus/Play/HallofTrials/HoT_Hub_NameUnselected_00.png");
                    }
                }
                });
            button->setWidgetSearchParent(subwindow.getParent()->getName());
            button->addWidgetAction("MenuStart", "enter");
            button->addWidgetAction("MenuAlt1", "reset");
            button->addWidgetAction("MenuCancel", "back_button");

            std::string sublabel_name = name;
            auto sublabel_background = subwindow.addImage(
                SDL_Rect{938, y + 4, 98, 44},
                0xffffffff,
                "*images/ui/Main Menus/Play/HallofTrials/HoT_Hub_TimeUnselected_00.png",
                (sublabel_name + "_sublabel_background").c_str()
            );

            auto sublabel_text = subwindow.addField((sublabel_name + "_sublabel").c_str(), 16);
            sublabel_text->setJustify(Field::justify_t::CENTER);
            sublabel_text->setFont(bigfont_no_outline);
            sublabel_text->setSize(sublabel_background->pos);
            sublabel_text->setText(sublabel);

            return button;
        };

        // collect best times
        const auto& levels = gameModeManager.Tutorial.levels;
        std::string times[11];
        std::string total_time_str;
        Uint64 total_time = 0;
        times[0] = "Hub";
        for (int c = 1; c < 11; ++c) {
            const Uint32 time = levels[c].completionTime / TICKS_PER_SECOND;
            const Uint32 hour = time / 3600;
            const Uint32 min = (time / 60) % 60;
            const Uint32 sec = time % 60;
            char buf[16];
            snprintf(buf, sizeof(buf), "%.2u:%.2u:%.2u", hour, min, sec);
            times[c] = buf;
            total_time += time;
        }
        {
            const Uint32 hour = total_time / 3600;
            const Uint32 min = (total_time / 60) % 60;
            const Uint32 sec = total_time % 60;
            char buf[16];
            snprintf(buf, sizeof(buf), "%.2u:%.2u:%.2u ", hour, min, sec);
            total_time_str = buf;
        }

        // create buttons
        Button* tutorials[11];
        constexpr int num_tutorials = sizeof(tutorials) / sizeof(tutorials[0]);
        tutorials[0]  = make_button(*subwindow,  24, "tutorial_hub", " The Hall of Trials", times[0].c_str());
        tutorials[1]  = make_button(*subwindow, 140, "tutorial1",    " Trial 1: Dungeon Basics and Melee Fighting", times[1].c_str());
        tutorials[2]  = make_button(*subwindow, 202, "tutorial2",    " Trial 2: Bows, Arrows, and Throwing Weapons", times[2].c_str());
        tutorials[3]  = make_button(*subwindow, 264, "tutorial3",    " Trial 3: Dungeon Traps, Spikes, and Boulders", times[3].c_str());
        tutorials[4]  = make_button(*subwindow, 326, "tutorial4",    " Trial 4: Food, Appraisal, and Curses", times[4].c_str());
        tutorials[5]  = make_button(*subwindow, 388, "tutorial5",    " Trial 5: Magic, Spellbooks, and Casting", times[5].c_str());
        tutorials[6]  = make_button(*subwindow, 450, "tutorial6",    " Trial 6: Stealth and Sneak Attacks", times[6].c_str());
        tutorials[7]  = make_button(*subwindow, 512, "tutorial7",    " Trial 7: Follower Recruiting and Commands", times[7].c_str());
        tutorials[8]  = make_button(*subwindow, 574, "tutorial8",    " Trial 8: Potions and Alchemy", times[8].c_str());
        tutorials[9]  = make_button(*subwindow, 636, "tutorial9",    " Trial 9: Tinkering", times[9].c_str());
        tutorials[10] = make_button(*subwindow, 698, "tutorial10",   " Trial 10: Merchants and Shops", times[10].c_str());
        tutorials[0]->select();

        // link buttons
        for (int c = 0; c < num_tutorials; ++c) {
            if (c > 0) {
                tutorials[c]->setWidgetUp(tutorials[c - 1]->getName());
            } else {
                tutorials[c]->setWidgetUp(tutorials[num_tutorials - 1]->getName());
            }
            if (c < num_tutorials - 1) {
                tutorials[c]->setWidgetDown(tutorials[c + 1]->getName());
            } else {
                tutorials[c]->setWidgetDown(tutorials[0]->getName());
            }
        }

        // total clear time
        auto total_time_label = window->addField("total_time_label", 128);
        total_time_label->setFont(bigfont_no_outline);
        total_time_label->setSize(SDL_Rect{540, 646, 340, 30});
        total_time_label->setText(" Total Clear Time");
        total_time_label->setHJustify(Field::justify_t::LEFT);
        total_time_label->setVJustify(Field::justify_t::CENTER);

        auto total_time_field = window->addField("total_time", 16);
        total_time_field->setFont(bigfont_no_outline);
        total_time_field->setSize(SDL_Rect{540, 646, 340, 30});
        total_time_field->setText(total_time_str.c_str());
        total_time_field->setHJustify(Field::justify_t::RIGHT);
        total_time_field->setVJustify(Field::justify_t::CENTER);

        // buttons at bottom
        auto reset = window->addButton("reset");
        reset->setText("Reset Trial\nProgress");
        reset->setSize(SDL_Rect{152, 630, 164, 62});
        reset->setBackground("*images/ui/Main Menus/Play/HallofTrials/HoT_Button_00.png");
        reset->setBackgroundHighlighted("*images/ui/Main Menus/Play/HallofTrials/HoT_ButtonHigh_00.png");
        reset->setBackgroundActivated("*images/ui/Main Menus/Play/HallofTrials/HoT_ButtonPress_00.png");
        reset->setFont(smallfont_outline);
        reset->setHighlightColor(0xffffffff);
        reset->setColor(0xffffffff);
        reset->setCallback([](Button&){
	        binaryPrompt(
	            "Are you sure you want to reset\nyour best times?", "Yes", "No",
	            [](Button& button) { // Yes button
			        soundActivate();
			        soundDeleteSave();

                    // delete best times
                    for (auto& it : gameModeManager.Tutorial.levels) {
                        it.completionTime = 0;
                    }
                    gameModeManager.Tutorial.writeToDocument();

                    // update window
			        assert(main_menu_frame);
		            auto window = main_menu_frame->findFrame("hall_of_trials_menu"); assert(window);
		            window->removeSelf();
		            createHallofTrialsMenu();

                    // remove prompt
			        closeBinary();
	            },
	            [](Button& button) { // No button
			        soundCancel();

                    // select another button
			        assert(main_menu_frame);
		            auto window = main_menu_frame->findFrame("hall_of_trials_menu"); assert(window);
		            auto subwindow = window->findFrame("subwindow"); assert(subwindow);
                    auto tutorial = subwindow->findButton("tutorial_hub"); assert(tutorial);
                    tutorial->select();

                    // remove prompt
			        closeBinary();
	            }
	        );
            });

        auto enter = window->addButton("enter");
        enter->setText("Enter Level");
        enter->setSize(SDL_Rect{902, 630, 164, 62});
        enter->setBackground("*images/ui/Main Menus/Play/HallofTrials/HoT_Button_00.png");
        enter->setBackgroundHighlighted("*images/ui/Main Menus/Play/HallofTrials/HoT_ButtonHigh_00.png");
        enter->setBackgroundActivated("*images/ui/Main Menus/Play/HallofTrials/HoT_ButtonPress_00.png");
        enter->setFont(smallfont_outline);
        enter->setHighlightColor(0xffffffff);
        enter->setColor(0xffffffff);
        enter->setCallback([](Button& button){
            if (!tutorial_map_destination.empty()) {
		        destroyMainMenu();
		        createDummyMainMenu();
		        beginFade(MainMenu::FadeDestination::HallOfTrials);
		    } else {
                errorPrompt(
	                "Select a level to start first.",
	                "Okay",
	                [](Button& button){
			            soundCancel();
			            assert(main_menu_frame);
			            auto hall_of_trials = main_menu_frame->findFrame("hall_of_trials_menu"); assert(hall_of_trials);
			            auto subwindow = hall_of_trials->findFrame("subwindow"); assert(subwindow);
			            auto tutorial = subwindow->findButton("tutorial_hub"); assert(tutorial);
			            tutorial->select();
		                closeMono();
	                }
	            );
		    }
            });
    }

	static void createPlayWindow() {
		multiplayer = SINGLE;

		auto dimmer = main_menu_frame->addFrame("dimmer");
		dimmer->setSize(SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY});
		dimmer->setActualSize(dimmer->getSize());
		dimmer->setColor(makeColor(0, 0, 0, 63));
		dimmer->setBorder(0);

		auto window = dimmer->addFrame("play_game_window");
		window->setSize(SDL_Rect{
			(Frame::virtualScreenX - 218 * 2) / 2,
			(Frame::virtualScreenY - 130 * 2) / 2,
			218 * 2,
			130 * 2});
		window->setActualSize(SDL_Rect{0, 0, 218 * 2, 130 * 2});
		window->setColor(0);
		window->setBorder(0);

		auto background = window->addImage(
			window->getActualSize(),
			0xffffffff,
			"*images/ui/Main Menus/Play/UI_PlayGame_Window_02.png",
			"background"
		);

		auto banner_title = window->addField("banner", 32);
		banner_title->setSize(SDL_Rect{170, 24, 98, 18});
		banner_title->setText("PLAY GAME");
		banner_title->setFont(smallfont_outline);
		banner_title->setJustify(Field::justify_t::CENTER);

		bool continueAvailable = anySaveFileExists();

		auto hall_of_trials_button = window->addButton("hall_of_trials");
		hall_of_trials_button->setSize(SDL_Rect{134, 176, 168, 52});
		hall_of_trials_button->setBackground("*images/ui/Main Menus/Play/UI_PlayMenu_Button_HallofTrials00.png");
		hall_of_trials_button->setBackgroundHighlighted("*images/ui/Main Menus/Play/UI_PlayMenu_Button_HallofTrialsHigh00.png");
		hall_of_trials_button->setBackgroundActivated("*images/ui/Main Menus/Play/UI_PlayMenu_Button_HallofTrialsPress00.png");
		hall_of_trials_button->setHighlightColor(makeColor(255, 255, 255, 255));
		hall_of_trials_button->setColor(makeColor(255, 255, 255, 255));
		hall_of_trials_button->setText("TUTORIALS");
		hall_of_trials_button->setFont(smallfont_outline);
		hall_of_trials_button->setWidgetSearchParent(window->getName());
		if (continueAvailable) {
			hall_of_trials_button->setWidgetUp("continue");
		} else {
			hall_of_trials_button->setWidgetUp("new");
		}
		hall_of_trials_button->setWidgetBack("back_button");
		hall_of_trials_button->setCallback([](Button&){
			soundActivate();
			createHallofTrialsMenu();
			});

		(void)createBackWidget(window, [](Button& button){
			soundCancel();
			auto frame = static_cast<Frame*>(button.getParent());
			frame = static_cast<Frame*>(frame->getParent());
			frame = static_cast<Frame*>(frame->getParent());
			frame->removeSelf();
			assert(main_menu_frame);
			auto buttons = main_menu_frame->findFrame("buttons");
			if (!buttons) {
				destroyMainMenu();
				createMainMenu(false);
			} else {
				auto play_button = buttons->findButton("Play Game"); assert(play_button);
				play_button->select();
			}
			});

		auto continue_button = window->addButton("continue");
		continue_button->setSize(SDL_Rect{39 * 2, 36 * 2, 66 * 2, 50 * 2});
		continue_button->setBackground("*images/ui/Main Menus/Play/UI_PlayMenu_Button_ContinueB00.png");
		continue_button->setTextColor(makeColor(180, 180, 180, 255));
		continue_button->setTextHighlightColor(makeColor(180, 133, 13, 255));
		continue_button->setText(" \nCONTINUE");
		continue_button->setFont(smallfont_outline);
		if (continueAvailable) {
			continue_button->setBackgroundHighlighted("*images/ui/Main Menus/Play/UI_PlayMenu_Button_ContinueA00.png");
			continue_button->setCallback([](Button& button){soundActivate(); playContinue(button);});
		} else {
			continue_button->setCallback([](Button&){soundError();});
		}
		continue_button->setWidgetSearchParent(window->getName());
		continue_button->setWidgetRight("new");
		continue_button->setWidgetDown("hall_of_trials");
		continue_button->setWidgetBack("back_button");
		continue_button->setGlyphPosition(Widget::glyph_position_t::CENTERED);
		continue_button->setButtonsOffset(SDL_Rect{0, 29, 0, 0,});
		continue_button->setSelectorOffset(SDL_Rect{-1, -1, 1, 1});

		auto new_button = window->addButton("new");
		new_button->setSize(SDL_Rect{114 * 2, 36 * 2, 68 * 2, 56 * 2});
		new_button->setBackground("*images/ui/Main Menus/Play/UI_PlayMenu_NewB00.png");
		new_button->setBackgroundHighlighted("*images/ui/Main Menus/Play/UI_PlayMenu_NewA00.png");
		new_button->setTextColor(makeColor(180, 180, 180, 255));
		new_button->setTextHighlightColor(makeColor(180, 133, 13, 255));
		new_button->setText(" \nNEW");
		new_button->setFont(smallfont_outline);
		new_button->setCallback(playNew);
		new_button->setWidgetSearchParent(window->getName());
		new_button->setWidgetLeft("continue");
		new_button->setWidgetDown("hall_of_trials");
		new_button->setWidgetBack("back_button");
		new_button->setGlyphPosition(Widget::glyph_position_t::CENTERED);
		new_button->setButtonsOffset(SDL_Rect{0, 29, 0, 0,});
		new_button->setSelectorOffset(SDL_Rect{-1, -1, -3, -11});

		if (!gameModeManager.Tutorial.FirstTimePrompt.showFirstTimePrompt) {
			if (continueAvailable) {
				continue_button->select();
			} else {
				new_button->select();
			}
		} else {
			hall_of_trials_button->select();
		}
	}

	static void createOnlineLobby() {
		if (LobbyHandler.getHostingType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY) {
#ifdef USE_EOS
			EOS.createLobby();
			textPrompt("host_lobby_prompt", "Creating Epic lobby...",
				[](Widget&){
				if (EOS.CurrentLobbyData.bAwaitingCreationCallback) {
					if (!isConnectedToEpic()) {
						closePrompt("host_lobby_prompt");
#ifdef NINTENDO
						// this way if something fucked up
						// (eg user is stuck in lobby in backend)
						// the user state will be reset
						nxEnableAutoSleep();
						nxEndParentalControls();
						logoutOfEpic();
						nxErrorPrompt(
							"Failed to host Epic lobby.",
							"Failed to host Epic lobby.\n\n"
							"Please try again later.",
							33333);
#else
						errorPrompt("Failed to host Epic lobby.", "Okay",
							[](Button&) {soundCancel(); closeMono(); });
#endif
					}
					return;
				} else {
					if (EOS.CurrentLobbyData.LobbyCreationResult == EOS_EResult::EOS_Success) {
						createLobby(LobbyType::LobbyOnline);
					} else {
						closePrompt("host_lobby_prompt");
#ifdef NINTENDO
						// this way if something fucked up
						// (eg user is stuck in lobby in backend)
						// the user state will be reset
						nxEnableAutoSleep();
						nxEndParentalControls();
						logoutOfEpic();
						nxErrorPrompt(
							"Failed to host Epic lobby.",
							"Failed to host Epic lobby.\n\n"
							"Please try again later.",
							44444);
#else
						errorPrompt("Failed to host Epic lobby.", "Okay",
							[](Button&){soundCancel(); closeMono();});
#endif
					}
				}
				});
#endif // USE_EOS
		}
		else if (LobbyHandler.getHostingType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM) {
#ifdef STEAMWORKS
			for ( int c = 0; c < MAXPLAYERS; c++ ) {
				if ( steamIDRemote[c] ) {
					cpp_Free_CSteamID(steamIDRemote[c]);
					steamIDRemote[c] = NULL;
				}
			}
			::currentLobbyType = k_ELobbyTypePublic;
			cpp_SteamMatchmaking_CreateLobby(::currentLobbyType, MAXPLAYERS);
			textPrompt("host_lobby_prompt", "Creating Steam lobby...",
				[](Widget&){
				if (steamAwaitingLobbyCreation) {
					return;
				} else {
					if (::currentLobby != nullptr) {
						createLobby(LobbyType::LobbyOnline);
					} else {
						closePrompt("host_lobby_prompt");
						errorPrompt("Failed to host Steam lobby.", "Okay",
							[](Button&){soundCancel(); closeMono();});
					}
				}
				});
#endif // STEAMWORKS
		}
	}

	static void hostOnlineLobby(Button&) {
#if !defined(STEAMWORKS) && !defined(USE_EOS)
		errorPrompt("Unable to host lobby\nOnline play is not available.", "Okay", [](Button&){
			multiplayer = SINGLE;
			soundCancel();
			closeMono();
		});
#else
		auto completion = [](bool connected){
			closeNetworkInterfaces();
			randomizeHostname();
			directConnect = false;

#if defined(STEAMWORKS) && !defined(USE_EOS)
			LobbyHandler.setHostingType(LobbyHandler_t::LobbyServiceType::LOBBY_STEAM);
			createOnlineLobby();
#elif defined(STEAMWORKS) && defined(USE_EOS)
			if (LobbyHandler.crossplayEnabled) {
				const char* prompt = "Would you like to host via\nEpic Online for crossplay?";
				binaryPrompt(prompt, "Yes", "No",
					[](Button&) { // yes
						closeBinary();
						LobbyHandler.setHostingType(LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY);
						LobbyHandler.setP2PType(LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY);
						createOnlineLobby();
					},
					[](Button&) { // no
						closeBinary();
						LobbyHandler.setHostingType(LobbyHandler_t::LobbyServiceType::LOBBY_STEAM);
						LobbyHandler.setP2PType(LobbyHandler_t::LobbyServiceType::LOBBY_STEAM);
						createOnlineLobby();
					}, false, false);
			}
			else {
				LobbyHandler.setHostingType(LobbyHandler_t::LobbyServiceType::LOBBY_STEAM);
				createOnlineLobby();
			}
#elif defined(USE_EOS)
			if (connected) {
				createOnlineLobby();
			}
			else {
				errorPrompt("Unable to host lobby\nOnline play is not available.", "Okay", [](Button&) {
					soundCancel();
					closeMono();
					});
#ifdef NINTENDO
				nxEnableAutoSleep();
				nxEndParentalControls();
				logoutOfEpic();
#endif
			}
#else
#error what kind of build is this?
#endif
		};
#endif

#ifdef NINTENDO
		if (!nxBeginParentalControls())
		{
			// access to online play is not permitted
			soundError();
			return;
		}
		loginToEpic(completion);
#elif defined(STEAMWORKS) || defined(USE_EOS)
		completion(true);
#endif
	}

	static void hostLANLobby(Button&) {
		soundActivate();

		closeNetworkInterfaces();
		randomizeHostname();
		directConnect = true;

#if defined(NINTENDO)
		if (!nxInitWireless()) {
			return;
		}
		if (!nxHostLobby()) {
			return;
		}

		// resolve localhost address
		Uint16 port = ::portnumber ? ::portnumber : DEFAULT_PORT;
		int resolve = SDLNet_ResolveHost(&net_server, NULL, port);
		assert(resolve != -1);

		// open socket
		if (!(net_sock = SDLNet_UDP_Open(port))) {
			char buf[1024];
			snprintf(buf, sizeof(buf), "Failed to open UDP socket\non port %hu.", port);
			systemErrorPrompt(buf);
			nxShutdownWireless();
			return;
		}

		// create lobby
		createLobby(LobbyType::LobbyLAN);
#else
		// resolve localhost address
		Uint16 port = ::portnumber ? ::portnumber : DEFAULT_PORT;
		int resolve = SDLNet_ResolveHost(&net_server, NULL, port);
		assert(resolve != -1);

		// open socket
		if (!(net_sock = SDLNet_UDP_Open(port))) {
			char buf[1024];
			snprintf(buf, sizeof(buf), "Failed to open UDP socket\non port %hu.", port);
			errorPrompt(buf, "Okay", [](Button&){soundCancel(); closeMono();});
			return;
		}

		// create lobby
		createLobby(LobbyType::LobbyLAN);
#endif
	}

	static void createLocalOrNetworkMenu() {
		allSettings.classic_mode_enabled = svFlags & SV_FLAG_CLASSIC;
		allSettings.hardcore_mode_enabled = svFlags & SV_FLAG_HARDCORE;
		allSettings.friendly_fire_enabled = svFlags & SV_FLAG_FRIENDLYFIRE;
		allSettings.keep_inventory_enabled = svFlags & SV_FLAG_KEEPINVENTORY;
		allSettings.hunger_enabled = svFlags & SV_FLAG_HUNGER;
		allSettings.minotaur_enabled = svFlags & SV_FLAG_MINOTAURS;
		allSettings.random_traps_enabled = svFlags & SV_FLAG_TRAPS;
		allSettings.extra_life_enabled = svFlags & SV_FLAG_LIFESAVING;
		allSettings.cheats_enabled = svFlags & SV_FLAG_CHEATS;

		auto dimmer = main_menu_frame->addFrame("dimmer");
		dimmer->setSize(SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY});
		dimmer->setActualSize(dimmer->getSize());
		dimmer->setColor(makeColor(0, 0, 0, 63));
		dimmer->setBorder(0);

		// create "Local or Network" window
		auto window = dimmer->addFrame("local_or_network_window");
		window->setSize(SDL_Rect{
			(Frame::virtualScreenX - 436) / 2 - 38,
			(Frame::virtualScreenY - 494) / 2,
			496,
			494});
		window->setActualSize(SDL_Rect{0, 0, 496, 494});
		window->setColor(0);
		window->setBorder(0);
		window->setTickCallback([](Widget& widget){
		    auto window = static_cast<Frame*>(&widget); assert(window);
		    auto tooltip = window->findField("tooltip"); assert(tooltip);
		    auto local_button = window->findButton("local"); assert(local_button);
		    auto local_image = window->findImage("local_image"); assert(local_image);
		    if (local_button->isSelected()) {
                char buf[128];
                const char fmt[] = "Play singleplayer or with 2-%d\nplayers in splitscreen multiplayer";
                snprintf(buf, sizeof(buf), fmt, MAX_SPLITSCREEN);
		        tooltip->setText(buf);
                local_image->path =
#ifdef NINTENDO
		            "*images/ui/Main Menus/Play/NewGameConnectivity/UI_NewGame_Icon_CouchCoOp_00.png";
#else
		            "*images/ui/Main Menus/Play/NewGameConnectivity/UI_NewGame_Icon_CouchCoOp_00_NoNX.png";
#endif
		    } else {
                local_image->path =
#ifdef NINTENDO
		            "*images/ui/Main Menus/Play/NewGameConnectivity/UI_NewGame_Icon_CouchCoOp_00B_Unselected.png";
#else
		            "*images/ui/Main Menus/Play/NewGameConnectivity/UI_NewGame_Icon_CouchCoOp_00B_Unselected_NoNX.png";
#endif
		    }
		    auto host_lan_button = window->findButton("host_lan"); assert(host_lan_button);
		    auto host_lan_image = window->findImage("host_lan_image"); assert(host_lan_image);
		    if (host_lan_button->isSelected()) {
                char buf[128];
#ifdef NINTENDO
                const char fmt[] = "Host a game with 2-%d players\nover a wireless connection";
#else
                const char fmt[] = "Host a game with 2-%d players\nover a local area network (LAN)";
#endif
                snprintf(buf, sizeof(buf), fmt, MAXPLAYERS);
                tooltip->setText(buf);
                host_lan_image->path =
		            "*images/ui/Main Menus/Play/NewGameConnectivity/UI_NewGame_Icon_HostLAN_00.png";
		    } else {
                host_lan_image->path =
		            "*images/ui/Main Menus/Play/NewGameConnectivity/UI_NewGame_Icon_HostLAN_00B_Unselected.png";
		    }
		    auto host_online_button = window->findButton("host_online"); assert(host_online_button);
		    auto host_online_image = window->findImage("host_online_image"); assert(host_online_image);
#if defined(STEAMWORKS) || defined(USE_EOS)
		    if (host_online_button->isSelected()) {
                char buf[128];
                const char fmt[] = "Host a game with 2-%d players\nover the internet";
                snprintf(buf, sizeof(buf), fmt, MAXPLAYERS);
                tooltip->setText(buf);
                host_online_image->path =
		            "*images/ui/Main Menus/Play/NewGameConnectivity/UI_NewGame_Icon_HostOnline_00.png";
		    } else {
                host_online_image->path =
		            "*images/ui/Main Menus/Play/NewGameConnectivity/UI_NewGame_Icon_HostOnline_00B_Unselected.png";
		    }
#else
		    if (host_online_button->isSelected()) {
                char buf[128];
                const char fmt[] = "Host a game with 2-%d players\nover the internet (disabled)";
                snprintf(buf, sizeof(buf), fmt, MAXPLAYERS);
		        tooltip->setText(buf);
		    }
#endif
		    auto join_button = window->findButton("join"); assert(join_button);
		    auto join_image = window->findImage("join_image"); assert(join_image);
		    if (join_button->isSelected()) {
		        tooltip->setText("Join a multiplayer game");
                join_image->path =
		            "*images/ui/Main Menus/Play/NewGameConnectivity/UI_NewGame_Icon_LobbyBrowser_00.png";
		    } else {
                join_image->path =
		            "*images/ui/Main Menus/Play/NewGameConnectivity/UI_NewGame_Icon_LobbyBrowser_00B_Unselected.png";
		    }
		    });

		auto background = window->addImage(
			window->getActualSize(),
			0xffffffff,
			"*images/ui/Main Menus/Play/NewGameConnectivity/UI_NewGame_Window_00.png",
			"background"
		);

		auto banner_title = window->addField("banner", 32);
		banner_title->setSize(SDL_Rect{180, 24, 152, 18});
		banner_title->setText("NEW ADVENTURER");
		banner_title->setFont(smallfont_outline);
		banner_title->setJustify(Field::justify_t::CENTER);

		(void)createBackWidget(window, [](Button& button){
			soundCancel();
			auto frame = static_cast<Frame*>(button.getParent());
			frame = static_cast<Frame*>(frame->getParent());
			frame = static_cast<Frame*>(frame->getParent());
			frame->removeSelf();
			createPlayWindow();
			}, SDL_Rect{42, 4, 0, 0});

		auto local_button = window->addButton("local");
		local_button->setSize(SDL_Rect{96, 72, 164, 62});
		local_button->setBackground("*images/ui/Main Menus/Play/NewGameConnectivity/ButtonStandard/Button_Standard_Default_00.png");
		local_button->setBackgroundHighlighted("*images/ui/Main Menus/Play/NewGameConnectivity/ButtonStandard/Button_Standard_Select_00.png");
		local_button->setBackgroundActivated("*images/ui/Main Menus/Play/NewGameConnectivity/ButtonStandard/Button_Standard_Down_00.png");
		local_button->setHighlightColor(makeColor(255, 255, 255, 255));
		local_button->setColor(makeColor(255, 255, 255, 255));
		local_button->setText("Local Adventure");
		local_button->setFont(smallfont_outline);
		local_button->setWidgetSearchParent(window->getName());
		local_button->setWidgetBack("back_button");
		local_button->setWidgetDown("host_lan");
		local_button->setCallback([](Button&){soundActivate(); multiplayer = SINGLE; createLobby(LobbyType::LobbyLocal);});

		// default button to select when no other is
		local_button->setTickCallback([](Widget& widget){
			if (!gui->findSelectedWidget(widget.getOwner())) {
				widget.select();
			}
			});

		local_button->select();

		(void)window->addImage(
		    SDL_Rect{278, 76, 104, 52},
		    0xffffffff,
#ifdef NINTENDO
		    "*images/ui/Main Menus/Play/NewGameConnectivity/UI_NewGame_Icon_CouchCoOp_00B_Unselected.png",
#else
		    "*images/ui/Main Menus/Play/NewGameConnectivity/UI_NewGame_Icon_CouchCoOp_00B_Unselected_NoNX.png",
#endif
		    "local_image"
		);

		auto host_lan_button = window->addButton("host_lan");
		host_lan_button->setSize(SDL_Rect{96, 166, 164, 62});
		host_lan_button->setBackground("*images/ui/Main Menus/Play/NewGameConnectivity/ButtonStandard/Button_Standard_Default_00.png");
		host_lan_button->setBackgroundHighlighted("*images/ui/Main Menus/Play/NewGameConnectivity/ButtonStandard/Button_Standard_Select_00.png");
		host_lan_button->setBackgroundActivated("*images/ui/Main Menus/Play/NewGameConnectivity/ButtonStandard/Button_Standard_Down_00.png");
		host_lan_button->setHighlightColor(makeColor(255, 255, 255, 255));
		host_lan_button->setColor(makeColor(255, 255, 255, 255));
#ifdef NINTENDO
		host_lan_button->setText("Host Wireless\nParty");
#else
		host_lan_button->setText("Host LAN Party");
#endif
		host_lan_button->setFont(smallfont_outline);
		host_lan_button->setWidgetSearchParent(window->getName());
		host_lan_button->setWidgetBack("back_button");
		host_lan_button->setWidgetUp("local");
		host_lan_button->setWidgetDown("host_online");
		host_lan_button->setCallback(hostLANLobby);

		(void)window->addImage(
		    SDL_Rect{270, 170, 126, 50},
		    0xffffffff,
		    "*images/ui/Main Menus/Play/NewGameConnectivity/UI_NewGame_Icon_HostLAN_00B_Unselected.png",
		    "host_lan_image"
		);

		auto host_online_button = window->addButton("host_online");
		host_online_button->setSize(SDL_Rect{96, 232, 164, 62});
		host_online_button->setBackground("*images/ui/Main Menus/Play/NewGameConnectivity/ButtonStandard/Button_Standard_Default_00.png");
#if defined(STEAMWORKS) || defined(NINTENDO)
		host_online_button->setBackgroundHighlighted("*images/ui/Main Menus/Play/NewGameConnectivity/ButtonStandard/Button_Standard_Select_00.png");
		host_online_button->setBackgroundActivated("*images/ui/Main Menus/Play/NewGameConnectivity/ButtonStandard/Button_Standard_Down_00.png");
		host_online_button->setHighlightColor(makeColor(255, 255, 255, 255));
		host_online_button->setColor(makeColor(255, 255, 255, 255));
		host_online_button->setTextColor(makeColor(255, 255, 255, 255));
		host_online_button->setTextHighlightColor(makeColor(255, 255, 255, 255));
#elif defined(USE_EOS)
		if (isConnectedToEpic()) {
			host_online_button->setBackgroundHighlighted("*images/ui/Main Menus/Play/NewGameConnectivity/ButtonStandard/Button_Standard_Select_00.png");
			host_online_button->setBackgroundActivated("*images/ui/Main Menus/Play/NewGameConnectivity/ButtonStandard/Button_Standard_Down_00.png");
			host_online_button->setHighlightColor(makeColor(255, 255, 255, 255));
			host_online_button->setColor(makeColor(255, 255, 255, 255));
			host_online_button->setTextColor(makeColor(255, 255, 255, 255));
			host_online_button->setTextHighlightColor(makeColor(255, 255, 255, 255));
		} else {
			host_online_button->setHighlightColor(makeColor(127, 127, 127, 255));
			host_online_button->setColor(makeColor(127, 127, 127, 255));
			host_online_button->setTextColor(makeColor(127, 127, 127, 255));
			host_online_button->setTextHighlightColor(makeColor(127, 127, 127, 255));
		}
#else
		host_online_button->setHighlightColor(makeColor(127, 127, 127, 255));
		host_online_button->setColor(makeColor(127, 127, 127, 255));
		host_online_button->setTextColor(makeColor(127, 127, 127, 255));
		host_online_button->setTextHighlightColor(makeColor(127, 127, 127, 255));
#endif
		host_online_button->setText("Host Online Party");
		host_online_button->setFont(smallfont_outline);
		host_online_button->setWidgetSearchParent(window->getName());
		host_online_button->setWidgetBack("back_button");
		host_online_button->setWidgetUp("host_lan");
		host_online_button->setWidgetDown("join");
		host_online_button->setCallback(hostOnlineLobby);

		(void)window->addImage(
		    SDL_Rect{270, 234, 126, 50},
		    0xffffffff,
		    "*images/ui/Main Menus/Play/NewGameConnectivity/UI_NewGame_Icon_HostOnline_00B_Unselected.png",
		    "host_online_image"
		);

		auto join_button = window->addButton("join");
		join_button->setSize(SDL_Rect{96, 326, 164, 62});
		join_button->setBackground("*images/ui/Main Menus/Play/NewGameConnectivity/ButtonStandard/Button_Standard_Default_00.png");
		join_button->setBackgroundHighlighted("*images/ui/Main Menus/Play/NewGameConnectivity/ButtonStandard/Button_Standard_Select_00.png");
		join_button->setBackgroundActivated("*images/ui/Main Menus/Play/NewGameConnectivity/ButtonStandard/Button_Standard_Down_00.png");
		join_button->setHighlightColor(makeColor(255, 255, 255, 255));
		join_button->setColor(makeColor(255, 255, 255, 255));
		join_button->setText("Lobby Browser");
		join_button->setFont(smallfont_outline);
		join_button->setWidgetSearchParent(window->getName());
		join_button->setWidgetBack("back_button");
		join_button->setWidgetUp("host_online");
		join_button->setCallback(createLobbyBrowser);

		(void)window->addImage(
		    SDL_Rect{270, 324, 120, 68},
		    0xffffffff,
		    "*images/ui/Main Menus/Play/NewGameConnectivity/UI_NewGame_Icon_LobbyBrowser_00B_Unselected.png",
		    "join_image"
		);

		auto tooltip = window->addField("tooltip", 1024);
		tooltip->setSize(SDL_Rect{106, 398, 300, 48});
		tooltip->setFont(smallfont_no_outline);
		tooltip->setColor(makeColor(183, 155, 119, 255));
		tooltip->setJustify(Field::justify_t::CENTER);
		tooltip->setText("");
	}

	static bool firstTimeTutorialPrompt() {
		if (!gameModeManager.Tutorial.FirstTimePrompt.showFirstTimePrompt) {
			return false;
		}
		gameModeManager.Tutorial.FirstTimePrompt.showFirstTimePrompt = false;
		gameModeManager.Tutorial.writeToDocument();
		binaryPrompt(
			"Barony is a challenging game.\nWould you like to play a tutorial?",
			"Yes", "No",
			[](Button& button) { // Yes
				soundActivate();
				destroyMainMenu();
				createDummyMainMenu();
				tutorial_map_destination = "tutorial1";
				beginFade(MainMenu::FadeDestination::HallOfTrials);
			},
			[](Button& button) { // No
				closeBinary();
				monoPrompt(
					"You can find the Tutorials\nin the Play Game menu.",
					"Okay",
					[](Button&) {
						soundCancel();

						assert(main_menu_frame);
						auto window = main_menu_frame->findFrame("play_game_window");
						if (window) {
							auto dimmer = static_cast<Frame*>(window->getParent()); assert(dimmer);
							dimmer->removeSelf();
							createLocalOrNetworkMenu();
						}
						else {
							auto buttons = main_menu_frame->findFrame("buttons"); assert(buttons);
							auto play = buttons->findButton("Play Game"); assert(play);
							play->select();
						}

						closeMono();
					});
			}, false, false); // both buttons are yellow
		return true;
	}

	static void playNew(Button& button) {
	    loadingsavegame = 0;

	    soundActivate();
	    createLocalOrNetworkMenu();

	    // remove "Play Game" window
	    auto frame = static_cast<Frame*>(button.getParent());
	    frame = static_cast<Frame*>(frame->getParent());
	    frame->removeSelf();
	}

	static Button* savegame_selected = nullptr;
	static bool continueSingleplayer = false;
	static Button* populateContinueSubwindow(Frame& subwindow, bool singleplayer);

	static void deleteSavePrompt(bool singleplayer, int save_index) {
	    static bool delete_singleplayer;
	    static int delete_save_index;
	    delete_singleplayer = singleplayer;
	    delete_save_index = save_index;

        // extract savegame info
        auto saveGameInfo = getSaveGameInfo(singleplayer, save_index);
        const std::string& game_name = saveGameInfo.gamename;

        // create shortened player name
        char shortened_name[20] = { '\0' };
        int len = (int)game_name.size();
        strncpy(shortened_name, game_name.c_str(), std::min(len, 16));
        if (len > 16) {
            strcat(shortened_name, "...");
        }

	    // window text
	    char window_text[1024];
	    snprintf(window_text, sizeof(window_text),
	        "Are you sure you want to delete\nthe save game \"%s\"?", shortened_name);

	    binaryPrompt(
	        window_text, "Yes", "No",
	        [](Button& button) { // Yes button
			    soundActivate();
			    soundDeleteSave();

                // delete save game
                (void)deleteSaveGame(delete_singleplayer ? SINGLE : SERVER, delete_save_index);

                // find frame elements
			    assert(main_menu_frame);
		        auto window = main_menu_frame->findFrame("continue_window"); assert(window);
		        auto subwindow = window->findFrame("subwindow"); assert(subwindow);

                // repopulate save game window & select a new button
			    savegame_selected = nullptr;
                subwindow->removeSelf();
                subwindow = window->addFrame("subwindow");
	            Button* first_savegame = populateContinueSubwindow(*subwindow, delete_singleplayer);
	            if (first_savegame) {
	                first_savegame->select();
		            savegame_selected = first_savegame;
	            } else {
	                if (delete_singleplayer) {
                        auto b = window->findButton("singleplayer");
                        b->select();
	                } else {
                        auto b = window->findButton("multiplayer");
                        b->select();
	                }
	            }
			    closeBinary();
	        },
	        [](Button& button) { // No button
			    soundCancel();
			    if (savegame_selected) {
			        savegame_selected->select();
			    } else {
			        assert(main_menu_frame);
		            auto window = main_menu_frame->findFrame("continue_window"); assert(window);
	                if (delete_singleplayer) {
                        auto b = window->findButton("singleplayer");
                        b->select();
	                } else {
                        auto b = window->findButton("multiplayer");
                        b->select();
	                }
			    }
			    closeBinary();
	        }
	    );
	}

	static void loadSavePrompt(bool singleplayer, int save_index) {
	    static bool load_singleplayer;
	    static int load_save_index;
	    load_singleplayer = singleplayer;
	    load_save_index = save_index;

        // extract savegame info
        auto saveGameInfo = getSaveGameInfo(singleplayer, save_index);
        const std::string& game_name = saveGameInfo.gamename;

        // create shortened player name
        char shortened_name[20] = { '\0' };
        int len = (int)game_name.size();
        strncpy(shortened_name, game_name.c_str(), std::min(len, 16));
        if (len > 16) {
            strcat(shortened_name, "...");
        }

	    // window text
	    char window_text[1024];
	    snprintf(window_text, sizeof(window_text),
	        "Are you sure you want to load\nthe save game \"%s\"?", shortened_name);

	    binaryPrompt(
	        window_text, "Yes", "No",
	        [](Button& button) { // Yes button
                soundActivate();
                destroyMainMenu();

                savegameCurrentFileIndex = load_save_index;
                auto info = getSaveGameInfo(load_singleplayer, savegameCurrentFileIndex);
                loadingsavegame = getSaveGameUniqueGameKey(info);

                if (info.multiplayer_type == SPLITSCREEN || info.multiplayer_type == SINGLE) {
                    multiplayer = SINGLE;
                    clientnum = -1;
                    for (int c = 0; c < MAXPLAYERS; ++c) {
                        if (info.players_connected[c]) {
                            clientnum = clientnum == -1 ? c : clientnum;
                            playerSlotsLocked[c] = false;
                            loadGame(c, info);
                        } else {
                            playerSlotsLocked[c] = true;
                        }
                    }
                    assert(clientnum != -1);
#ifndef NINTENDO
                    inputs.setPlayerIDAllowedKeyboard(clientnum);
#endif
                    createLobby(LobbyType::LobbyLocal);
                } else if (info.multiplayer_type == SERVER || info.multiplayer_type == DIRECTSERVER || info.multiplayer_type == SERVERCROSSPLAY) {
                    multiplayer = SERVER;
                    for (int c = 0; c < MAXPLAYERS; ++c) {
                        newPlayer[c] = c != 0;
                        if (info.players_connected[c]) {
                            playerSlotsLocked[c] = false;
                    		loadGame(c, info);
                        } else {
                            playerSlotsLocked[c] = true;
                        }
                    }
                    if (info.multiplayer_type == SERVERCROSSPLAY) {
#ifdef STEAMWORKS
			            LobbyHandler.hostingType = LobbyHandler_t::LobbyServiceType::LOBBY_STEAM;
			            LobbyHandler.setP2PType(LobbyHandler_t::LobbyServiceType::LOBBY_STEAM);
#ifdef USE_EOS
			            if ( LobbyHandler.crossplayEnabled )
			            {
							LobbyHandler.hostingType = LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY;
				            LobbyHandler.setP2PType(LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY);
			            }
#endif
#elif defined USE_EOS
			            // if just eos, then force hosting settings to default.
			            LobbyHandler.hostingType = LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY;
			            LobbyHandler.setP2PType(LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY);
#endif
						createMainMenu(false); // just in-case the lobby hosting fails
                        hostOnlineLobby(button);
                    } else if (info.multiplayer_type == SERVER) {
			            if ( getSaveGameVersionNum(info) <= 335 )
			            {
				            // legacy support for steam ver not remembering if crossplay or not. no action.
				            // starting with v3.3.6, (mul == SERVERCROSSPLAY) detects from the savefile.
			            }
			            else
			            {
#ifdef STEAMWORKS
				            LobbyHandler.hostingType = LobbyHandler_t::LobbyServiceType::LOBBY_STEAM;
				            LobbyHandler.setP2PType(LobbyHandler_t::LobbyServiceType::LOBBY_STEAM);
#endif
			            }
						createMainMenu(false); // just in-case the lobby hosting fails
                        hostOnlineLobby(button);
                    } else if (info.multiplayer_type == DIRECTSERVER) {
						createMainMenu(false); // just in-case the lobby hosting fails
                        hostLANLobby(button);
                    }
                } else if (info.multiplayer_type == CLIENT || info.multiplayer_type == DIRECTCLIENT) {
                    for (int c = 0; c < MAXPLAYERS; ++c) {
                        if (info.players_connected[c]) {
                            playerSlotsLocked[c] = false;
                    		loadGame(c, info);
                        } else {
                            playerSlotsLocked[c] = true;
						}
                    }
                    multiplayer = SINGLE;
					clientnum = 0;
                    createDummyMainMenu();
                    createLobbyBrowser(button);
                }
	        },
	        [](Button& button) { // No button
			    soundCancel();
			    if (savegame_selected) {
			        savegame_selected->select();
			    } else {
			        assert(main_menu_frame);
		            auto window = main_menu_frame->findFrame("continue_window"); assert(window);
	                if (load_singleplayer) {
                        auto b = window->findButton("singleplayer");
                        b->select();
	                } else {
                        auto b = window->findButton("multiplayer");
                        b->select();
	                }
			    }
			    closeBinary();
	        },
	        false, false); // both buttons are yellow
	}
                  
    static void addContinuePlayerInfo(Frame& frame, SaveGameInfo& info, int player, int x, int y, bool show_pnum) {
        auto subframe = frame.addFrame("info");
        subframe->setSize(SDL_Rect{x, y, 64, 64});
        subframe->setHollow(true);
        subframe->setColor(0);
        
        // player num + level text
        char buf[16];
        auto lvl = subframe->addField("player_lvl", sizeof(buf));
        if (show_pnum) {
            snprintf(buf, sizeof(buf), "P%d\nLv%d", player + 1, info.players[player].stats.LVL);
            lvl->setTextColor(playerColor(player, colorblind, false));
            lvl->setOutlineColor(makeColorRGB(0, 0, 0));
        } else {
            snprintf(buf, sizeof(buf), "Lv%d", info.players[player].stats.LVL);
            lvl->setTextColor(makeColorRGB(255, 255, 255));
            lvl->setOutlineColor(makeColorRGB(52, 32, 23));
        }
        lvl->setHJustify(Field::justify_t::LEFT);
        lvl->setVJustify(Field::justify_t::BOTTOM);
        lvl->setSize(SDL_Rect{0, 3, 64, 64});
        lvl->setColor(0xffffffff);
        lvl->setFont(smallfont_outline);
        lvl->setText(buf);
        
        // class image
        const int num_classes = sizeof(classes_in_order) / sizeof(classes_in_order[0]);
        const int class_index = (info.players[player].char_class) % num_classes;
        const auto class_name = classes_in_order[class_index];
        const auto class_find = classes.find(class_name);
        if (class_find != classes.end()) {
            std::string class_img_path = "#*images/ui/Main Menus/Play/PlayerCreation/ClassSelection/";
            class_img_path += class_find->second.image_highlighted;
            auto class_img = subframe->addImage(
                SDL_Rect{2, 0, 54, 54},
                0xffffffff,
                class_img_path.c_str(),
                "class_img");
        }
        
        // portrait
        const std::string portrait_path =
            monsterData.getAllyIconFromSprite(playerHeadSprite(
                (Monster)getMonsterFromPlayerRace(info.players[player].race),
                (sex_t)info.players[player].stats.sex,
                (int)info.players[player].stats.appearance));
        auto portrait = subframe->addImage(
            SDL_Rect{32, 24, 32, 32},
            0xffffffff,
            portrait_path.c_str(),
            "portrait");
    }
                
	static Button* populateContinueSubwindow(Frame& subwindow, bool singleplayer) {
	    static Uint32 timeSinceScroll = 0;
		subwindow.setActualSize(SDL_Rect{0, 0, 898, 294});
		subwindow.setSize(SDL_Rect{90, 82, 898, 294});
		subwindow.setColor(0);
		subwindow.setBorder(0);
	    Button* first_savegame = nullptr;
        if (!anySaveFileExists(singleplayer)) {
            auto none_exists = subwindow.addField("none_exists", 256);
            none_exists->setSize(subwindow.getActualSize());
            none_exists->setFont(bigfont_outline);
            none_exists->setText("No compatible save files found.");
            none_exists->setJustify(Field::justify_t::CENTER);
        } else {
			// sort savegames by date/time
			using list_type = std::pair<int, SaveGameInfo>;
			std::list<list_type> savegames;
		    for (int i = 0; i < SAVE_GAMES_MAX; ++i) {
                if (saveGameExists(singleplayer, i)) {
					savegames.emplace_back(i, getSaveGameInfo(singleplayer, i));
				}
			}
			savegames.sort([](const list_type& lhs, const list_type& rhs){
				return rhs.second.timestamp < lhs.second.timestamp;
				});

			// create save game entries
			int index = 0;
			for (auto it = savegames.begin(); it != savegames.end(); ++it) {
				auto& savegame = *it;
                const int posX = index * 256 + (898 - 220) / 2;
                auto str = std::string(singleplayer ? "savegame" : "savegame_multiplayer") + std::to_string(savegame.first);
                auto savegame_book = subwindow.addButton(str.c_str());
                savegame_book->setSize(SDL_Rect{posX, 0, 220, 280});
                savegame_book->setBackground("*images/ui/Main Menus/ContinueGame/UI_Cont_SaveFile_Book_00.png");
		        savegame_book->setColor(makeColor(255, 255, 255, 255));
		        savegame_book->setHighlightColor(makeColor(255, 255, 255, 255));
		        savegame_book->setFont(smallfont_outline);
		        savegame_book->setTextColor(makeColor(255, 182, 73, 255));
		        savegame_book->setTextHighlightColor(makeColor(255, 182, 73, 255));
		        savegame_book->setTickCallback([](Widget& widget){
	                auto button = static_cast<Button*>(&widget);
	                auto frame = static_cast<Frame*>(widget.getParent());

		            Input& input = Input::inputs[widget.getOwner()];
		            bool scrolled = false;
		            scrolled |= input.binary("MenuScrollDown");
		            scrolled |= input.binary("MenuScrollUp");
		            scrolled |= input.binary("MenuScrollLeft");
		            scrolled |= input.binary("MenuScrollRight");
		            scrolled |= input.binary("MenuMouseWheelUp");
		            scrolled |= input.binary("MenuMouseWheelDown");
		            if (scrolled) {
		                timeSinceScroll = ticks;
		            } else if (widget.isSelected() && !inputs.getVirtualMouse(getMenuOwner())->draw_cursor) {
		                savegame_selected = button;
		            }

	                auto frame_pos = frame->getActualSize();
	                auto button_size = button->getSize();
	                const int diff = ((button_size.x - (898 - 220) / 2) - frame_pos.x);

		            if (ticks - timeSinceScroll > TICKS_PER_SECOND / 2) {
		                if (savegame_selected == button) {
		                    if (diff > 0) {
		                        frame_pos.x += std::max(1, diff / 8);
		                    } else if (diff < 0) {
		                        frame_pos.x += std::min(-1, diff / 8);
		                    }
		                    frame->setActualSize(frame_pos);
		                }
		            } else {
	                    if (abs(diff) < 100) {
	                        savegame_selected = button;
	                    }
		            }
		            });
		        savegame_book->setCallback([](Button& button){
		            if (savegame_selected != &button && inputs.getVirtualMouse(getMenuOwner())->draw_cursor) {
		                soundCheckmark();
	                    savegame_selected = &button;
	                    timeSinceScroll = 0;
		                return;
		            } else {
	                    savegame_selected = &button;
		                int save_index = -1;
	                    const char* name = continueSingleplayer ? "savegame" : "savegame_multiplayer";
	                    size_t name_len = strlen(name);
                        save_index = (int)strtol(button.getName() + name_len, nullptr, 10);
                        if (cursor_delete_mode) {
                            deleteSavePrompt(continueSingleplayer, save_index);
                        } else {
                            loadSavePrompt(continueSingleplayer, save_index);
                        }
		            }
		            });

				// set previous book in line
				std::string prevstr;
				auto prev = it;
				if (prev != savegames.begin()) {
					--prev;
					prevstr = std::string(singleplayer ? "savegame" : "savegame_multiplayer") + std::to_string(prev->first);
				} else {
					prevstr = std::string(singleplayer ? "savegame" : "savegame_multiplayer") + std::to_string(savegames.back().first);
				}
				savegame_book->setWidgetLeft(prevstr.c_str());

				// set next book in line
				std::string nextstr;
				auto next = it;
				++next;
				if (next == savegames.end()) {
					nextstr = std::string(singleplayer ? "savegame" : "savegame_multiplayer") + std::to_string(savegames.front().first);
				} else {
					nextstr = std::string(singleplayer ? "savegame" : "savegame_multiplayer") + std::to_string(next->first);
				}
				savegame_book->setWidgetRight(nextstr.c_str());

		        savegame_book->setWidgetBack("back_button");
		        savegame_book->addWidgetAction("MenuAlt1", "delete");
		        savegame_book->addWidgetAction("MenuConfirm", "enter");
		        savegame_book->addWidgetAction("MenuPageLeft", "singleplayer");
		        savegame_book->addWidgetAction("MenuPageRight", "multiplayer");
		        savegame_book->setWidgetSearchParent("continue_window");

		        first_savegame = first_savegame ? first_savegame : savegame_book;

		        auto& saveGameInfo = savegame.second;

                // extract savegame info
                const std::string& game_name = saveGameInfo.gamename;
                const auto timestamp = saveGameInfo.timestamp;
				int numplayers = 0;
				for (auto p : saveGameInfo.players_connected) {
					if (p) {
						++numplayers;
					}
				}
				auto time = timestamp.substr(11);
				time[2] = ':';
				time[5] = ' ';
				time[6] = ' ';
				time[7] = ' ';
				auto date = timestamp.substr(0, 10);
				date[4] = '/';
				date[7] = '/';

                // create shortened game name
                char shortened_name[20] = { '\0' };
                int len = (int)game_name.size();
                strncpy(shortened_name, game_name.c_str(), std::min(len, 16));
                if (len > 16) {
                    strcat(shortened_name, "...");
                }

				// create game type string
				char game_type[32] = { '\0' };
				switch (saveGameInfo.multiplayer_type) {
				default:
				case SINGLE: snprintf(game_type, sizeof(game_type), "Singleplayer"); break;
				case SERVER: snprintf(game_type, sizeof(game_type), "Online Host %dp (#1)", numplayers); break;
				case CLIENT: snprintf(game_type, sizeof(game_type), "Online Client %dp (#%d)", numplayers, saveGameInfo.player_num + 1); break;
				case DIRECTSERVER: snprintf(game_type, sizeof(game_type), "Local Host %dp (#1)", numplayers); break;
				case DIRECTCLIENT: snprintf(game_type, sizeof(game_type), "Local Client (#%d/%d)", saveGameInfo.player_num + 1, numplayers); break;
				case SERVERCROSSPLAY: snprintf(game_type, sizeof(game_type), "Online Host %dp (#1)", numplayers); break;
				case SPLITSCREEN: snprintf(game_type, sizeof(game_type), "Splitscreen %dp", numplayers); break;
				}

                // format book label string
		        char text[1024];
				snprintf(text, sizeof(text), "%s\n%s\n%s %s",
					shortened_name, game_type, date.c_str(), time.c_str());
		        savegame_book->setText(text);

                // offset text
                SDL_Rect offset;
                offset.x = 34;
                offset.y = 181;
				savegame_book->setPaddingPerTextLine(4);
		        savegame_book->setTextOffset(offset);
		        savegame_book->setJustify(Button::justify_t::LEFT);

		        // add savegame screenshot
		        /*auto screenshot_path = setSaveGameFileName(singleplayer, SaveFileType::SCREENSHOT, i);
                if (dataPathExists(screenshot_path.c_str(), false)) {
		            auto screenshot = subwindow.addImage(
		                SDL_Rect{saveGameCount * 256 + (898 - 220) / 2 + 32, 16, 160, 162},
		                0xffffffff,
		                screenshot_path.c_str(),
		                (str + "_screenshot").c_str()
		            );
		            screenshot->ontop = true;
		            Image* image = Image::get(screenshot_path.c_str()); assert(image);
		            screenshot->section.x = (image->getWidth() - image->getHeight()) / 2;
		            screenshot->section.w = image->getHeight();
		        }*/
                    
                auto cover = subwindow.addFrame("bookcover");
                cover->setSize(SDL_Rect{posX, 0, 220, 280});
                cover->setColor(0);
                cover->setHollow(true);

				// add savegame picture
				std::string screenshot_path = "images/ui/Main Menus/ContinueGame/savescreens/";
				if (saveGameInfo.level_track == 1) {
					switch (saveGameInfo.dungeon_lvl) {
					default: screenshot_path += "save_unknown00.png"; break;
					case 3: screenshot_path += "save_gnome00.png"; break;
					case 4: screenshot_path += "save_minetown00.png"; break;
					case 6:
					case 7: screenshot_path += "save_underworld00.png"; break;
					case 8: screenshot_path += "save_temple00.png"; break;
					case 9: screenshot_path += "save_castle00.png"; break;
					case 12: screenshot_path += "save_sokoban00.png"; break;
					case 14: screenshot_path += "save_maze00.png"; break;
					case 17: screenshot_path += "save_library00.png"; break;
					case 19:
					case 20: screenshot_path += "save_underworld00.png"; break;
					case 24: screenshot_path += "save_boss00.png"; break;
					case 29: screenshot_path += "save_lair00.png"; break;
					case 34: screenshot_path += "save_bram00.png"; break;
					}
				} else {
					switch (saveGameInfo.dungeon_lvl) {
					default: screenshot_path += "save_unknown00.png"; break;
					case 0: screenshot_path += "save_start00.png"; break;
					case 1:
					case 2:
					case 3:
					case 4: screenshot_path += "save_mines00.png"; break;
					case 5: screenshot_path += "save_minetoswamp00.png"; break;
					case 6:
					case 7:
					case 8:
					case 9: screenshot_path += "save_swamp00.png"; break;
					case 10: screenshot_path += "save_swamptolab00.png"; break;
					case 11:
					case 12:
					case 13:
					case 14: screenshot_path += "save_labyrinth00.png"; break;
					case 15: screenshot_path += "save_labtoruin00.png"; break;
					case 16:
					case 17:
					case 18:
					case 19: screenshot_path += "save_ruin00.png"; break;
					case 20: screenshot_path += "save_boss00.png"; break;
					case 21:
					case 22:
					case 23: screenshot_path += "save_hell00.png"; break;
					case 24: screenshot_path += "save_devil00.png"; break;
					case 25: screenshot_path += "save_hamlet00.png"; break;
					case 26:
					case 27:
					case 28:
					case 29: screenshot_path += "save_caves00.png"; break;
					case 30: screenshot_path += "save_cavetocitadel00.png"; break;
					case 31:
					case 32:
					case 33:
					case 34: screenshot_path += "save_citadel00.png"; break;
					case 35: screenshot_path += "save_sanctum00.png"; break;
					}
				}

				auto screenshot = cover->addImage(
					SDL_Rect{32, 16, 160, 162},
					0xffffffff,
					screenshot_path.c_str(),
					(str + "_screenshot").c_str()
				);
				Image* image = Image::get(screenshot_path.c_str()); assert(image);
				screenshot->section.x = (image->getWidth() - image->getHeight()) / 2;
				screenshot->section.w = image->getHeight();

		        // add book overlay
		        auto overlay = cover->addImage(
		            SDL_Rect{32, 16, 160, 162},
		            0xffffffff,
		            "*images/ui/Main Menus/ContinueGame/UI_Cont_SaveFile_Book_Corners_00.png",
		            (str + "_overlay").c_str()
		        );
                    
                // add player info
                if (numplayers == 1) {
                    addContinuePlayerInfo(subwindow, saveGameInfo, saveGameInfo.player_num, posX + 30, 114, false);
                }
                else if (numplayers == 2) {
                    for (int c = 0, player = 0; c < (int)saveGameInfo.players_connected.size(); ++c) {
                        if (saveGameInfo.players_connected[c]) {
                            switch (player) {
                            default:
                            case 0:
                                addContinuePlayerInfo(subwindow, saveGameInfo, c, posX + 30, 114, true);
                                break;
                            case 1:
                                addContinuePlayerInfo(subwindow, saveGameInfo, c, posX + 128, 114, true);
                                break;
                            }
                            ++player;
                        }
                    }
                }
                else if (numplayers >= 3) {
                    for (int c = 0, player = 0; c < (int)saveGameInfo.players_connected.size(); ++c) {
                        if (saveGameInfo.players_connected[c]) {
                            switch (player) {
                            default:
                            case 0:
                                addContinuePlayerInfo(subwindow, saveGameInfo, c, posX + 30, 16, true);
                                break;
                            case 1:
                                addContinuePlayerInfo(subwindow, saveGameInfo, c, posX + 128, 16, true);
                                break;
                            case 2:
                                addContinuePlayerInfo(subwindow, saveGameInfo, c, posX + 30, 114, true);
                                break;
                            case 3:
                                addContinuePlayerInfo(subwindow, saveGameInfo, c, posX + 128, 114, true);
                                break;
                            }
                            ++player;
                        }
                    }
                }

		        ++index;
		    }
		    subwindow.setActualSize(SDL_Rect{0, 0, 898 + 256 * ((int)savegames.size() - 1), 294});
        }
        return first_savegame;
	}

	static void playContinue(Button& button) {
        continueSingleplayer = ~(!anySaveFileExists(true) && anySaveFileExists(false));

        savegame_selected = nullptr;

		// remove "Play Game" window
		auto frame = static_cast<Frame*>(button.getParent());
		frame = static_cast<Frame*>(frame->getParent());
		frame->removeSelf();

		auto dimmer = main_menu_frame->addFrame("dimmer");
		dimmer->setSize(SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY});
		dimmer->setActualSize(dimmer->getSize());
		dimmer->setColor(makeColor(0, 0, 0, 63));
		dimmer->setBorder(0);

		// create Continue window
		auto window = dimmer->addFrame("continue_window");
		window->setSize(SDL_Rect{
			(Frame::virtualScreenX - 1080) / 2,
			(Frame::virtualScreenY - 474) / 2,
			1080,
			474});
		window->setActualSize(SDL_Rect{0, 0, 1080, 474});
		window->setColor(0);
		window->setBorder(0);

		auto background = window->addImage(
			window->getActualSize(),
			0xffffffff,
			"*images/ui/Main Menus/ContinueGame/UI_Cont_Window_00.png",
			"background"
		);

		auto banner_title = window->addField("banner", 32);
		banner_title->setSize(SDL_Rect{
		    (window->getActualSize().w - 256) / 2,
		    8,
		    256,
		    48});
		banner_title->setText("CONTINUE ADVENTURE");
		banner_title->setFont(bigfont_outline);
		banner_title->setJustify(Field::justify_t::CENTER);

		auto singleplayer = window->addButton("singleplayer");
		singleplayer->setText("Local Games");
		singleplayer->setFont(smallfont_outline);
		singleplayer->setSize(SDL_Rect{226, 38, 156, 36});
		singleplayer->setColor(makeColor(255, 255, 255, 255));
		singleplayer->setHighlightColor(makeColor(255, 255, 255, 255));
		singleplayer->setTextColor(
		    continueSingleplayer ?
		    makeColor(255, 255, 255, 255) :
		    makeColor(127, 127, 127, 255));
		singleplayer->setBackground(
		    continueSingleplayer ?
		    "*images/ui/Main Menus/ContinueGame/UI_Cont_Tab_Single_ON_00.png" :
		    "*images/ui/Main Menus/ContinueGame/UI_Cont_Tab_Single_OFF_00.png");
		singleplayer->setGlyphPosition(Button::glyph_position_t::CENTERED_LEFT);
		singleplayer->setWidgetSearchParent("continue_window");
		singleplayer->setWidgetBack("back_button");
		singleplayer->addWidgetAction("MenuAlt1", "delete");
		singleplayer->addWidgetAction("MenuConfirm", "enter");
		singleplayer->addWidgetAction("MenuPageLeft", "singleplayer");
		singleplayer->addWidgetAction("MenuPageRight", "multiplayer");
		singleplayer->setCallback([](Button& button){
		    continueSingleplayer = true;
            Frame* window = static_cast<Frame*>(button.getParent());
            button.setTextColor(makeColor(255, 255, 255, 255));
            button.setBackground("*images/ui/Main Menus/ContinueGame/UI_Cont_Tab_Single_ON_00.png");
            auto multiplayer = window->findButton("multiplayer");
		    multiplayer->setTextColor(makeColor(127, 127, 127, 255));
		    multiplayer->setBackground("*images/ui/Main Menus/ContinueGame/UI_Cont_Tab_Multi_OFF_00.png");
            Frame* subwindow = window->findFrame("subwindow");
            subwindow->removeSelf();
            subwindow = window->addFrame("subwindow");
		    auto first_savegame = populateContinueSubwindow(*subwindow, continueSingleplayer);
		    if (first_savegame) {
		        first_savegame->select();
		        savegame_selected = first_savegame;
		    } else {
		        button.select();
				savegame_selected = nullptr;
		    }
		    auto slider = window->findSlider("slider");
		    if (slider) {
		        const float sliderMaxSize = subwindow->getActualSize().w - subwindow->getSize().w;
		        slider->setMinValue(0.f);
		        slider->setValue(0.f);
		        slider->setMaxValue(sliderMaxSize);
	        }
		    cursor_delete_mode = false;
		    });

		auto multiplayer = window->addButton("multiplayer");
		multiplayer->setText("Net Games");
		multiplayer->setFont(smallfont_outline);
		multiplayer->setSize(SDL_Rect{702, 38, 144, 36});
		multiplayer->setColor(makeColor(255, 255, 255, 255));
		multiplayer->setHighlightColor(makeColor(255, 255, 255, 255));
		multiplayer->setTextColor(
		    continueSingleplayer ?
		    makeColor(127, 127, 127, 255) :
		    makeColor(255, 255, 255, 255));
		multiplayer->setBackground(
		    continueSingleplayer ?
		    "*images/ui/Main Menus/ContinueGame/UI_Cont_Tab_Multi_OFF_00.png" :
		    "*images/ui/Main Menus/ContinueGame/UI_Cont_Tab_Multi_ON_00.png");
		multiplayer->setGlyphPosition(Button::glyph_position_t::CENTERED_RIGHT);
		multiplayer->setWidgetSearchParent("continue_window");
		multiplayer->setWidgetBack("back_button");
		multiplayer->addWidgetAction("MenuAlt1", "delete");
		multiplayer->addWidgetAction("MenuConfirm", "enter");
		multiplayer->addWidgetAction("MenuPageLeft", "singleplayer");
		multiplayer->addWidgetAction("MenuPageRight", "multiplayer");
		multiplayer->setCallback([](Button& button){
		    continueSingleplayer = false;
            Frame* window = static_cast<Frame*>(button.getParent());
            button.setTextColor(makeColor(255, 255, 255, 255));
            button.setBackground("*images/ui/Main Menus/ContinueGame/UI_Cont_Tab_Multi_ON_00.png");
            auto singleplayer = window->findButton("singleplayer");
		    singleplayer->setTextColor(makeColor(127, 127, 127, 255));
		    singleplayer->setBackground("*images/ui/Main Menus/ContinueGame/UI_Cont_Tab_Single_OFF_00.png");
            Frame* subwindow = window->findFrame("subwindow");
            subwindow->removeSelf();
            subwindow = window->addFrame("subwindow");
		    auto first_savegame = populateContinueSubwindow(*subwindow, continueSingleplayer);
		    if (first_savegame) {
		        first_savegame->select();
		        savegame_selected = first_savegame;
		    } else {
		        button.select();
				savegame_selected = nullptr;
		    }
		    auto slider = window->findSlider("slider");
		    if (slider) {
		        const float sliderMaxSize = subwindow->getActualSize().w - subwindow->getSize().w;
		        slider->setMinValue(0.f);
		        slider->setValue(0.f);
		        slider->setMaxValue(sliderMaxSize);
		    }
		    cursor_delete_mode = false;
		    });

		auto subwindow = window->addFrame("subwindow");
		auto first_savegame = populateContinueSubwindow(*subwindow, continueSingleplayer);
		if (first_savegame) {
		    first_savegame->select();
		    savegame_selected = first_savegame;
		} else {
			if (continueSingleplayer) {
				auto b = window->findButton("singleplayer");
				b->select();
			} else {
				auto b = window->findButton("multiplayer");
				b->select();
			}
		}

		auto gradient = window->addImage(
		    subwindow->getSize(),
		    0xffffffff,
		    "*images/ui/Main Menus/ContinueGame/UI_Cont_SaveFile_Grad_01.png",
		    "gradient"
		);
		gradient->ontop = true;

		auto delete_button = window->addButton("delete");
		delete_button->setText("Delete Save");
		delete_button->setFont(smallfont_outline);
		delete_button->setSize(SDL_Rect{278, 390, 164, 62});
		delete_button->setColor(makeColor(255, 255, 255, 255));
		delete_button->setHighlightColor(makeColor(255, 255, 255, 255));
		delete_button->setBackground("*images/ui/Main Menus/ContinueGame/UI_Cont_Button_Delete_00.png");
		delete_button->setBackgroundHighlighted("*images/ui/Main Menus/ContinueGame/UI_Cont_Button_DeleteHigh_00.png");
		delete_button->setBackgroundActivated("*images/ui/Main Menus/ContinueGame/UI_Cont_Button_DeletePress_00.png");
		delete_button->setWidgetSearchParent("continue_window");
		delete_button->setWidgetBack("back_button");
		delete_button->addWidgetAction("MenuAlt1", "delete");
		delete_button->addWidgetAction("MenuConfirm", "enter");
		delete_button->addWidgetAction("MenuPageLeft", "singleplayer");
		delete_button->addWidgetAction("MenuPageRight", "multiplayer");
		delete_button->setCallback([](Button& button){
	        int save_index = -1;
	        if (savegame_selected) {
	            const char* name = continueSingleplayer ? "savegame" : "savegame_multiplayer";
	            size_t name_len = strlen(name);
	            if (strncmp(savegame_selected->getName(), name, name_len) == 0) {
                    save_index = (int)strtol(savegame_selected->getName() + name_len, nullptr, 10);
	            }
	        }
	        if (save_index >= 0) {
	            deleteSavePrompt(continueSingleplayer, save_index);
	        } else {
	            errorPrompt(
	                "Select a savegame to delete first.",
	                "Okay",
	                [](Button& button){
			            soundCancel();
			            if (savegame_selected) {
			                savegame_selected->select();
			            } else {
			                assert(main_menu_frame);
		                    auto window = main_menu_frame->findFrame("continue_window"); assert(window);
	                        if (continueSingleplayer) {
                                auto singleplayer = window->findButton("singleplayer");
                                singleplayer->select();
	                        } else {
                                auto multiplayer = window->findButton("multiplayer");
                                multiplayer->select();
	                        }
			            }
		                closeMono();
	                }
	            );
	        }
		    });

		auto enter_button = window->addButton("enter");
		enter_button->setText("Enter Dungeon");
		enter_button->setFont(smallfont_outline);
		enter_button->setSize(SDL_Rect{642, 390, 164, 62});
		enter_button->setColor(makeColor(255, 255, 255, 255));
		enter_button->setHighlightColor(makeColor(255, 255, 255, 255));
		enter_button->setBackground("*images/ui/Main Menus/ContinueGame/UI_Cont_Button_00.png");
		enter_button->setBackgroundHighlighted("*images/ui/Main Menus/ContinueGame/UI_Cont_ButtonHigh_00.png");
		enter_button->setBackgroundActivated("*images/ui/Main Menus/ContinueGame/UI_Cont_ButtonPress_00.png");
		enter_button->setWidgetSearchParent("continue_window");
		enter_button->setWidgetBack("back_button");
		enter_button->addWidgetAction("MenuAlt1", "delete");
		enter_button->addWidgetAction("MenuConfirm", "enter");
		enter_button->addWidgetAction("MenuPageLeft", "singleplayer");
		enter_button->addWidgetAction("MenuPageRight", "multiplayer");
		enter_button->setCallback([](Button& button){
	        int save_index = -1;
	        if (savegame_selected) {
	            const char* name = continueSingleplayer ? "savegame" : "savegame_multiplayer";
	            size_t name_len = strlen(name);
	            if (strncmp(savegame_selected->getName(), name, name_len) == 0) {
                    save_index = (int)strtol(savegame_selected->getName() + name_len, nullptr, 10);
	            }
	        }
	        if (save_index >= 0) {
	            loadSavePrompt(continueSingleplayer, save_index);
	        } else {
                errorPrompt(
                    "Select a savegame to load first.",
                    "Okay",
                    [](Button& button){
		                soundCancel();
		                if (savegame_selected) {
		                    savegame_selected->select();
		                } else {
			                assert(main_menu_frame);
		                    auto window = main_menu_frame->findFrame("continue_window"); assert(window);
                            if (continueSingleplayer) {
                                auto singleplayer = window->findButton("singleplayer");
                                singleplayer->select();
                            } else {
                                auto multiplayer = window->findButton("multiplayer");
                                multiplayer->select();
                            }
		                }
		                closeMono();
                    }
                );
	        }
		    });

		const float sliderMaxSize = subwindow->getActualSize().w - subwindow->getSize().w;

        Slider* slider = nullptr;
		//slider = window->addSlider("slider");
		if (slider) {
		    slider->setHandleSize(SDL_Rect{0, 0, 98, 16});
		    slider->setHandleImage("*images/ui/Main Menus/ContinueGame/UI_Cont_LRSliderBar_00.png");
		    slider->setRailSize(SDL_Rect{129, 374, 820, 16});
		    slider->setRailImage("__empty");
		    slider->setMinValue(0.f);
		    slider->setValue(0.f);
		    slider->setMaxValue(sliderMaxSize);
		    slider->setTickCallback([](Widget& widget){
		        auto slider = static_cast<Slider*>(&widget);
		        auto frame = static_cast<Frame*>(slider->getParent());
		        auto subwindow = frame->findFrame("subwindow");
		        auto size = subwindow->getActualSize();
		        slider->setValue(size.x);

		        /*auto slider_left = frame->findImage("slider_left");
		        auto slider_right = frame->findImage("slider_right");
	            slider_left->pos.x = slider->getHandleSize().x - 20;
	            slider_right->pos.x = slider->getHandleSize().x + slider->getHandleSize().w;
		        if (slider->isActivated()) {
		            slider_left->disabled = false;
		            slider_right->disabled = false;
		        } else {
		            slider_left->disabled = true;
		            slider_right->disabled = true;
		        }*/
		    });
		    slider->setCallback([](Slider& slider){
		        auto frame = static_cast<Frame*>(slider.getParent());
		        auto subwindow = frame->findFrame("subwindow");
		        auto size = subwindow->getActualSize();
		        size.x = slider.getValue();
		        subwindow->setActualSize(size);
		        savegame_selected = nullptr;
		        });

		    auto slider_left = window->addImage(
		        SDL_Rect{0, 354, 20, 30},
		        0xffffffff,
		        "*images/ui/Main Menus/ContinueGame/UI_Cont_LRSliderL_00.png",
		        "slider_left"
		    );
		    slider_left->ontop = true;
		    slider_left->disabled = true;

		    auto slider_right = window->addImage(
		        SDL_Rect{0, 354, 20, 30},
		        0xffffffff,
		        "*images/ui/Main Menus/ContinueGame/UI_Cont_LRSliderR_00.png",
		        "slider_right"
		    );
		    slider_right->ontop = true;
		    slider_right->disabled = true;
	    }

		(void)createBackWidget(window, [](Button& button){
			cursor_delete_mode = false;
			savegame_selected = nullptr;
			soundCancel();
			auto frame = static_cast<Frame*>(button.getParent());
			frame = static_cast<Frame*>(frame->getParent());
			frame = static_cast<Frame*>(frame->getParent());
			frame->removeSelf();
			createPlayWindow();
			}, SDL_Rect{16, 0, 0, 0});
	}

/******************************************************************************/

	static void mainPlayGame(Button& button) {
		soundActivate();
		createPlayWindow();
	}

	static void mainPlayModdedGame(Button& button) {
	    // WIP
		soundActivate();
		createPlayWindow();
	}

	static void mainArchives(Button& button) {
		soundActivate();

		assert(main_menu_frame);

		// change "notification" section into subsection banner
		auto notification = main_menu_frame->findFrame("notification"); assert(notification);
		const int note_y = notification->getSize().y;
		notification->removeSelf();
		notification = main_menu_frame->addFrame("notification");
		notification->setSize(SDL_Rect{
			(Frame::virtualScreenX - 204 * 2) / 2, note_y, 204 * 2, 43 * 2});
		notification->setActualSize(SDL_Rect{0, 0, notification->getSize().w, notification->getSize().h});
		auto image = notification->addImage(
		    notification->getActualSize(),
		    0xffffffff,
		    "*images/ui/Main Menus/AdventureArchives/UI_AdventureArchives_TitleGraphic00.png",
		    "background");

		// add banner text to notification
		auto banner_text = notification->addField("text", 64);
		banner_text->setJustify(Field::justify_t::CENTER);
		banner_text->setText("ADVENTURE ARCHIVES");
		banner_text->setFont(menu_option_font);
		banner_text->setColor(makeColor(180, 135, 27, 255));
		banner_text->setSize(SDL_Rect{19 * 2, 15 * 2, 166 * 2, 12 * 2});

		// disable banners
		auto banners = main_menu_frame->findFrame("banners");
		if (banners) {
		    banners->setDisabled(true);
		}

		// delete existing buttons
		auto old_buttons = main_menu_frame->findFrame("buttons");
		old_buttons->removeSelf();

		struct Option {
			const char* name;
			const char* text;
			void (*callback)(Button&);
		};
		Option options[] = {
			//{"Dungeon Compendium", "DUNGEON COMPENDIUM", archivesDungeonCompendium}, // TODO
#ifndef STEAMWORKS
#if defined(USE_EOS) || defined(LOCAL_ACHIEVEMENTS)
			{"Achievements", "ACHIEVEMENTS", archivesAchievements},
#endif
#endif
#ifdef NINTENDO
			{"Leaderboards", "HIGHSCORES", archivesLeaderboards},
#else
			{"Leaderboards", "LEADERBOARDS", archivesLeaderboards},
#endif
			{"Story Introduction", "STORY INTRODUCTION", archivesStoryIntroduction},
			{"Credits", "CREDITS", archivesCredits},
			{"Back to Main Menu", "BACK TO MAIN MENU", archivesBackToMainMenu}
		};
		const int num_options = sizeof(options) / sizeof(options[0]);

		int y = main_menu_buttons_height;

		auto buttons = main_menu_frame->addFrame("buttons");
		buttons->setTickCallback(updateMenuCursor);
		buttons->setSize(SDL_Rect{0, y, Frame::virtualScreenX, 36 * (num_options + 1)});
		buttons->setActualSize(SDL_Rect{0, 0, buttons->getSize().w, buttons->getSize().h});
		buttons->setHollow(true);
		buttons->setBorder(0);
		for (int c = 0; c < num_options; ++c) {
			auto button = buttons->addButton(options[c].name);
			button->setCallback(options[c].callback);
			button->setBorder(8);
			button->setHJustify(Button::justify_t::LEFT);
			button->setVJustify(Button::justify_t::CENTER);
			button->setText(options[c].text);
			button->setFont(menu_option_font);
			button->setBackground("*#images/ui/Main Menus/Main/UI_MainMenu_SelectorBar00.png");
			button->setHideSelectors(false);
			button->setColor(makeColor(255, 255, 255, 255));
			button->setHighlightColor(makeColor(255, 255, 255, 255));
			button->setTextColor(makeColor(180, 180, 180, 255));
			button->setTextHighlightColor(makeColor(180, 133, 13, 255));
			button->setGlyphPosition(Widget::glyph_position_t::CENTERED_RIGHT);
			button->setSize(SDL_Rect{
				(Frame::virtualScreenX - 164 * 2) / 2,
				y - buttons->getSize().y,
				164 * 2,
				16 * 2
				});
			int back = c - 1 < 0 ? num_options - 1 : c - 1;
			int forward = c + 1 >= num_options ? 0 : c + 1;
			button->setWidgetDown(options[forward].name);
			button->setWidgetUp(options[back].name);
			button->setWidgetBack("Back to Main Menu");
			y += button->getSize().h;
			//y += 4;
			if (c == num_options - 2) {
				y += button->getSize().h;
				//y += 4;
			}
		}
		y += 16;

		auto archives = buttons->findButton("Leaderboards");
		if (archives) {
			archives->select();
		}
	}

	static void mainAssignControllers(Button& button) {
	    soundActivate();
        button.deselect();
	    static auto return_to_main_menu = [](){
            if (main_menu_frame) {
	            auto buttons = main_menu_frame->findFrame("buttons");
	            if (buttons) {
	                auto button = buttons->findButton("Assign Controllers");
	                if (button) {
	                    button->select();
	                }
	            }
	        }
	    };
	    if (splitscreen) {
#ifdef NINTENDO
			int numplayers = 0;
			for (int c = 0; c < MAX_SPLITSCREEN; ++c) {
				if (isPlayerSignedIn(c)) {
					++numplayers;
				}
			}
			nxAssignControllers(numplayers, numplayers, false, false, false, false, nullptr);
#else
	        static std::vector<int> players;
	        players.clear();
	        players.reserve(MAX_SPLITSCREEN);
	        for (int c = 0; c < MAX_SPLITSCREEN; ++c) {
	            if (isPlayerSignedIn(c)) {
	                players.push_back(c);
	            }
	        }
	        if (!players.empty()) {
	            createControllerPrompt(players[0], true,
	                [](){if (players.size() >= 2) createControllerPrompt(players[1], true,
                    [](){if (players.size() >= 3) createControllerPrompt(players[2], true,
                    [](){if (players.size() >= 4) createControllerPrompt(players[3], true,
                    return_to_main_menu
                    ); else return_to_main_menu();}
                    ); else return_to_main_menu();}
                    ); else return_to_main_menu();}
                    );
	        }
#endif
	    } else {
#ifdef NINTENDO
			nxAssignControllers(1, 1, true, true, true, false, nullptr);
#else
	        createControllerPrompt(getMenuOwner(), false, return_to_main_menu);
#endif
	    }
	}

	static void mainSettings(Button& button) {
		//soundActivate(); // not needed, activated tab will do this

		settings_tab_name = "";
		settingsMount();

		auto dimmer = main_menu_frame->addFrame("dimmer");
		dimmer->setSize(SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY});
		dimmer->setActualSize(dimmer->getSize());
		dimmer->setColor(makeColor(0, 0, 0, 63));
		dimmer->setBorder(0);

		auto settings = dimmer->addFrame("settings");
		settings->setSize(SDL_Rect{(Frame::virtualScreenX - 1126) / 2, (Frame::virtualScreenY - 718) / 2, 1126, 718});
		settings->setActualSize(SDL_Rect{0, 0, settings->getSize().w, settings->getSize().h});
		settings->setColor(0);
		settings->setBorder(0);
		settings->addImage(
			settings->getActualSize(),
			0xffffffff,
			"*images/ui/Main Menus/Settings/Settings_Window04.png",
			"background"
		);
		auto timber = settings->addImage(
			SDL_Rect{0, 66 * 2, 1126, 586},
			0xffffffff,
			"*images/ui/Main Menus/Settings/Settings_TimberEdge05.png",
			"timber"
		);
		timber->ontop = true;

		settings->setTickCallback([](Widget& widget){
			auto settings = static_cast<Frame*>(&widget);
			std::vector<const char*> tabs = {
				"General",
				"Video",
				"Audio",
				"Controls",
			};
			if (intro) {
#ifndef NINTENDO
			    tabs.push_back("Online");
#endif
			} else {
				tabs.push_back("Game");
			}
			for (auto name : tabs) {
				auto button = settings->findButton(name);
				if (button) {
					if (name == settings_tab_name) {
						button->setBackground("*images/ui/Main Menus/Settings/Settings_Button_SubTitleSelect00.png");
						button->setBackgroundHighlighted("*images/ui/Main Menus/Settings/Settings_Button_SubTitleSelectHigh00.png");
						button->setBackgroundActivated("*images/ui/Main Menus/Settings/Settings_Button_SubTitleSelectPress00.png");
					} else {
						button->setBackground("*images/ui/Main Menus/Settings/Settings_Button_SubTitle00.png");
						button->setBackgroundHighlighted("*images/ui/Main Menus/Settings/Settings_Button_SubTitleHigh00.png");
						button->setBackgroundActivated("*images/ui/Main Menus/Settings/Settings_Button_SubTitlePress00.png");
					}
				}
			}
			});

		auto window_title = settings->addField("window_title", 64);
		window_title->setFont(banner_font);
		window_title->setSize(SDL_Rect{394, 26, 338, 24});
		window_title->setJustify(Field::justify_t::CENTER);
		window_title->setText("SETTINGS");

		struct Option {
			const char* name;
			void (*callback)(Button&);
		};
		std::vector<Option> tabs = {
			{"General", settingsGeneral},
			{"Video", settingsVideo},
			{"Audio", settingsAudio},
			{"Controls", settingsControls},
		};
		if (intro) {
#ifndef NINTENDO
		    tabs.push_back({"Online", settingsOnline});
#endif
		} else {
			tabs.push_back({"Game", settingsGame});
		}
		const int num_tabs = (int)tabs.size();
		for (int c = 0; c < num_tabs; ++c) {
			const int x = settings->getSize().w / (num_tabs + 1);
			auto button = settings->addButton(tabs[c].name);
			button->setCallback(tabs[c].callback);
			button->setText(tabs[c].name);
			button->setFont(banner_font);
			if (tabs[c].name == settings_tab_name) {
				button->setBackground("*images/ui/Main Menus/Settings/Settings_Button_SubTitleSelect00.png");
				button->setBackgroundHighlighted("*images/ui/Main Menus/Settings/Settings_Button_SubTitleSelectHigh00.png");
				button->setBackgroundActivated("*images/ui/Main Menus/Settings/Settings_Button_SubTitleSelectPress00.png");
			} else {
				button->setBackground("*images/ui/Main Menus/Settings/Settings_Button_SubTitle00.png");
				button->setBackgroundHighlighted("*images/ui/Main Menus/Settings/Settings_Button_SubTitleHigh00.png");
				button->setBackgroundActivated("*images/ui/Main Menus/Settings/Settings_Button_SubTitlePress00.png");
			}
			button->setSize(SDL_Rect{x + (x * c) - 184 / 2, 64, 184, 64});
			button->setColor(makeColor(255, 255, 255, 255));
			button->setHighlightColor(makeColor(255, 255, 255, 255));
			button->setWidgetSearchParent("settings");
			button->setWidgetPageLeft("tab_left");
			button->setWidgetPageRight("tab_right");
			button->addWidgetAction("MenuAlt1", "restore_defaults");
			button->addWidgetAction("MenuStart", "confirm_and_exit");
			button->setGlyphPosition(Widget::glyph_position_t::CENTERED_BOTTOM);
			if (c > 0) {
				button->setWidgetLeft(tabs[c - 1].name);
			} else {
				button->setWidgetLeft("tab_left");
			}
			if (c < num_tabs - 1) {
				button->setWidgetRight(tabs[c + 1].name);
			} else {
				button->setWidgetRight("tab_right");
			}
			button->setWidgetBack("discard_and_exit");
			if (c <= num_tabs / 2) {
				button->setWidgetDown("restore_defaults");
			} else if (c == num_tabs - 2) {
				button->setWidgetDown("discard_and_exit");
			} else if (c == num_tabs - 1) {
				button->setWidgetDown("confirm_and_exit");
			}
		}
		auto first_tab = settings->findButton(tabs[0].name);
		if (first_tab) {
			first_tab->select();
			first_tab->activate();
		}

		auto tab_left = settings->addButton("tab_left");
		tab_left->setBackground("*images/ui/Main Menus/Settings/Settings_Button_L00.png");
		tab_left->setBackgroundHighlighted("*images/ui/Main Menus/Settings/Settings_Button_LHigh00.png");
		tab_left->setBackgroundActivated("*images/ui/Main Menus/Settings/Settings_Button_LPress00.png");
		tab_left->setSize(SDL_Rect{32, 68, 38, 58});
		tab_left->setColor(makeColor(255, 255, 255, 255));
		tab_left->setHighlightColor(makeColor(255, 255, 255, 255));
		tab_left->setWidgetSearchParent("settings");
		tab_left->setWidgetBack("discard_and_exit");
		tab_left->setWidgetPageLeft("tab_left");
		tab_left->setWidgetPageRight("tab_right");
		tab_left->setWidgetRight("General");
		tab_left->setWidgetDown("restore_defaults");
		tab_left->addWidgetAction("MenuAlt1", "restore_defaults");
		tab_left->addWidgetAction("MenuStart", "confirm_and_exit");
		tab_left->setCallback([](Button&){
			auto settings = main_menu_frame->findFrame("settings"); assert(settings);
			std::vector<const char*> tabs = {
				"General",
				"Video",
				"Audio",
				"Controls",
			};
			if (intro) {
#ifndef NINTENDO
			    tabs.push_back("Online");
#endif
			} else {
				tabs.push_back("Game");
			}
			const char* prevtab = nullptr;
			for (auto tab : tabs) {
				auto button = settings->findButton(tab);
                if (button) {
                    const char* name = "*images/ui/Main Menus/Settings/Settings_Button_SubTitleSelect00.png";
                    if (strcmp(button->getBackground(), name) == 0) {
                        if (prevtab) {
                            auto prevbutton = settings->findButton(prevtab); assert(prevbutton);
                            prevbutton->select();
                            prevbutton->activate();
                        }
                        return;
                    }
                    prevtab = tab;
                }
			}
			});
		tab_left->setGlyphPosition(Button::glyph_position_t::CENTERED);

		auto tab_right = settings->addButton("tab_right");
		tab_right->setBackground("*images/ui/Main Menus/Settings/Settings_Button_R00.png");
		tab_right->setBackgroundHighlighted("*images/ui/Main Menus/Settings/Settings_Button_RHigh00.png");
		tab_right->setBackgroundActivated("*images/ui/Main Menus/Settings/Settings_Button_RPress00.png");
		tab_right->setSize(SDL_Rect{1056, 68, 38, 58});
		tab_right->setColor(makeColor(255, 255, 255, 255));
		tab_right->setHighlightColor(makeColor(255, 255, 255, 255));
		tab_right->setWidgetSearchParent("settings");
		tab_right->setWidgetBack("discard_and_exit");
		tab_right->setWidgetPageLeft("tab_left");
		tab_right->setWidgetPageRight("tab_right");
#ifdef NINTENDO
        tab_right->setWidgetLeft(intro ? "Controls" : "Game");
#else
		tab_right->setWidgetLeft(intro ? "Online" : "Game");
#endif
		tab_right->setWidgetDown("confirm_and_exit");
		tab_right->addWidgetAction("MenuAlt1", "restore_defaults");
		tab_right->addWidgetAction("MenuStart", "confirm_and_exit");
		tab_right->setCallback([](Button&){
			auto settings = main_menu_frame->findFrame("settings"); assert(settings);
			std::vector<const char*> tabs = {
				"Controls",
				"Audio",
				"Video",
				"General",
			};
			if (intro) {
#ifndef NINTENDO
				tabs.insert(tabs.begin(), "Online");
#endif
			} else {
			    tabs.insert(tabs.begin(), "Game");
			}
			const char* nexttab = nullptr;
			for (auto tab : tabs) {
                auto button = settings->findButton(tab);
                if (button) {
                    const char* name = "*images/ui/Main Menus/Settings/Settings_Button_SubTitleSelect00.png";
                    if (strcmp(button->getBackground(), name) == 0) {
                        if (nexttab) {
                            auto nextbutton = settings->findButton(nexttab); assert(nextbutton);
                            nextbutton->select();
                            nextbutton->activate();
                        }
                        return;
                    }
                    nexttab = tab;
                }
			}
			});
		tab_right->setGlyphPosition(Button::glyph_position_t::CENTERED);

		auto tooltip = settings->addField("tooltip", 256);
		tooltip->setSize(SDL_Rect{66, 594, 946, 22});
		tooltip->setFont(smallfont_no_outline);
		tooltip->setJustify(Field::justify_t::CENTER);
		tooltip->setText("");

		auto restore_defaults = settings->addButton("restore_defaults");
		restore_defaults->setBackground("*images/ui/Main Menus/Settings/Settings_Button_Basic00.png");
		restore_defaults->setBackgroundHighlighted("*images/ui/Main Menus/Settings/Settings_Button_BasicHigh00.png");
		restore_defaults->setBackgroundActivated("*images/ui/Main Menus/Settings/Settings_Button_BasicPress00.png");
		restore_defaults->setSize(SDL_Rect{84, 630, 164, 62});
		restore_defaults->setText("Restore\nDefaults");
		restore_defaults->setJustify(Button::justify_t::CENTER);
		restore_defaults->setFont(smallfont_outline);
		restore_defaults->setColor(makeColor(255, 255, 255, 255));
		restore_defaults->setHighlightColor(makeColor(255, 255, 255, 255));
		restore_defaults->setWidgetSearchParent("settings");
		restore_defaults->setWidgetBack("discard_and_exit");
		restore_defaults->setWidgetPageLeft("tab_left");
		restore_defaults->setWidgetPageRight("tab_right");
		restore_defaults->setWidgetUp("General");
		restore_defaults->setWidgetRight("discard_and_exit");
		restore_defaults->addWidgetAction("MenuAlt1", "restore_defaults");
		restore_defaults->addWidgetAction("MenuStart", "confirm_and_exit");
		restore_defaults->setHideKeyboardGlyphs(false);
		restore_defaults->setCallback([](Button& button){
			settingsReset();
			auto settings = static_cast<Frame*>(button.getParent()); assert(settings);
			std::vector<const char*> tabs = {
				"Controls",
				"Audio",
				"Video",
				"General",
			};
            if (intro) {
#ifndef NINTENDO
				tabs.insert(tabs.begin(), "Online");
#endif
			} else {
			    tabs.insert(tabs.begin(), "Game");
			}
			for (auto tab : tabs) {
				auto button = settings->findButton(tab);
                if (button) {
                    const char* name = "*images/ui/Main Menus/Settings/Settings_Button_SubTitleSelect00.png";
                    if (strcmp(button->getBackground(), name) == 0) {
                        button->select();
                        button->activate();
                        return;
                    }
                }
			}
			});

		auto discard_and_exit = settings->addButton("discard_and_exit");
		discard_and_exit->setBackground("*images/ui/Main Menus/Settings/Settings_Button_Basic00.png");
		discard_and_exit->setBackgroundHighlighted("*images/ui/Main Menus/Settings/Settings_Button_BasicHigh00.png");
		discard_and_exit->setBackgroundActivated("*images/ui/Main Menus/Settings/Settings_Button_BasicPress00.png");
		discard_and_exit->setSize(SDL_Rect{700, 630, 164, 62});
		discard_and_exit->setText("Discard\n& Exit");
		discard_and_exit->setJustify(Button::justify_t::CENTER);
		discard_and_exit->setFont(smallfont_outline);
		discard_and_exit->setColor(makeColor(255, 255, 255, 255));
		discard_and_exit->setHighlightColor(makeColor(255, 255, 255, 255));
		discard_and_exit->setCallback([](Button& button){
			soundCancel();
			setAudioDevice(current_audio_device);
		    setGlobalVolume(master_volume, musvolume, sfxvolume, sfxAmbientVolume, sfxEnvironmentVolume, sfxNotificationVolume);
			if (main_menu_frame) {
				auto buttons = main_menu_frame->findFrame("buttons"); assert(buttons);
				auto settings_button = buttons->findButton("Settings"); assert(settings_button);
				settings_button->select();
			}
			auto settings = static_cast<Frame*>(button.getParent());
			if (settings) {
				auto dimmer = static_cast<Frame*>(settings->getParent());
				dimmer->removeSelf();
			}
			});
		discard_and_exit->setWidgetSearchParent("settings");
		discard_and_exit->setWidgetBack("discard_and_exit");
		discard_and_exit->setWidgetPageLeft("tab_left");
		discard_and_exit->setWidgetPageRight("tab_right");
		discard_and_exit->setWidgetUp("Controls");
		discard_and_exit->setWidgetLeft("restore_defaults");
		discard_and_exit->setWidgetRight("confirm_and_exit");
		discard_and_exit->addWidgetAction("MenuAlt1", "restore_defaults");
		discard_and_exit->addWidgetAction("MenuStart", "confirm_and_exit");
		discard_and_exit->setHideKeyboardGlyphs(false);

		auto confirm_and_exit = settings->addButton("confirm_and_exit");
		confirm_and_exit->setBackground("*images/ui/Main Menus/Settings/Settings_Button_Basic00.png");
		confirm_and_exit->setBackgroundHighlighted("*images/ui/Main Menus/Settings/Settings_Button_BasicHigh00.png");
		confirm_and_exit->setBackgroundActivated("*images/ui/Main Menus/Settings/Settings_Button_BasicPress00.png");
		confirm_and_exit->setSize(SDL_Rect{880, 630, 164, 62});
		confirm_and_exit->setText("Confirm\n& Exit");
		confirm_and_exit->setJustify(Button::justify_t::CENTER);
		confirm_and_exit->setFont(smallfont_outline);
		confirm_and_exit->setColor(makeColor(255, 255, 255, 255));
		confirm_and_exit->setHighlightColor(makeColor(255, 255, 255, 255));
		confirm_and_exit->setCallback([](Button& button){
            settingsApply();
			if (video_refresh == VideoRefresh::None) {
			    // resolution confirm prompt makes this sound
			    soundActivate();
			}
			(void)settingsSave();

			static auto return_to_main_menu = [](Button& button){
				if (main_menu_frame) {
					auto buttons = main_menu_frame->findFrame("buttons"); assert(buttons);
					auto settings_button = buttons->findButton("Settings"); assert(settings_button);
					settings_button->select();
					auto settings = main_menu_frame->findFrame("settings");
					if (settings) {
						auto dimmer = static_cast<Frame*>(settings->getParent());
						dimmer->removeSelf();
					}
				}
				};

#if defined(VIDEO_RESTART_NEEDED)
			if (video_refresh != VideoRefresh::None) {
				if (video_refresh & VideoRefresh::Video) {
					// non-windows platforms need to throw a prompt to restart to apply video settings
					monoPrompt("You must restart Barony for\nsome settings to take effect.", "Okay",
						[](Button& button) {
							soundCancel();
							closeMono();
							return_to_main_menu(button);
						});
				} else {
					return_to_main_menu(button);
				}
				video_refresh = VideoRefresh::None;
			} else {
				return_to_main_menu(button);
			}
#else
			return_to_main_menu(button);
#endif
			});
		confirm_and_exit->setWidgetSearchParent("settings");
		confirm_and_exit->setWidgetBack("discard_and_exit");
		confirm_and_exit->setWidgetPageLeft("tab_left");
		confirm_and_exit->setWidgetPageRight("tab_right");
		confirm_and_exit->setWidgetUp("tab_right");
		confirm_and_exit->setWidgetLeft("discard_and_exit");
		confirm_and_exit->addWidgetAction("MenuAlt1", "restore_defaults");
		confirm_and_exit->addWidgetAction("MenuStart", "confirm_and_exit");
		confirm_and_exit->setHideKeyboardGlyphs(false);
	}

	static void mainEditor(Button& button) {
#if defined(WINDOWS)
	    char path[PATH_MAX];
	    completePath(path, "editor.exe");
	    stopMusic();
	    system(path);
#elif defined(LINUX)
	    char path[PATH_MAX];
	    completePath(path, "editor &");
	    stopMusic();
	    system(path);
#else
        return;
#endif
	}

	static void mainEndLife(Button& button) {
		binaryPrompt(
			"Are you sure you want to die?\nThere is no return from this.", // window text
			"End Life", // okay text
			"Cancel", // cancel text
			[](Button&){ // okay
				soundActivate();
				closeMainMenu();
		        if (multiplayer == CLIENT) {
			        // request sweet release.
			        strcpy((char*)net_packet->data, "IDIE");
			        net_packet->data[4] = clientnum;
			        net_packet->address.host = net_server.host;
			        net_packet->address.port = net_server.port;
			        net_packet->len = 5;
			        sendPacketSafe(net_sock, -1, net_packet, 0);
		        } else {
			        if (players[getMenuOwner()] && players[getMenuOwner()]->entity) {
				        players[getMenuOwner()]->entity->setHP(0);
			        }
		        }
			},
			[](Button&){ // cancel
				soundCancel();
				assert(main_menu_frame);
				auto buttons = main_menu_frame->findFrame("buttons"); assert(buttons);
				auto quit_button = buttons->findButton("End Life"); assert(quit_button);
				quit_button->select();
			    closeBinary();
			});
	}

	static void mainDropOut(Button& button) {
	    const char* prompt = "Do you want to drop out?\nThis player will be lost forever.";
		binaryPrompt(
			prompt, // window text
			"Okay", // okay text
			"Cancel", // cancel text
			[](Button&){ // okay
				client_disconnected[getMenuOwner()] = true;
				soundActivate();
				closeMainMenu();
				setupSplitscreen();
			},
			[](Button&){ // cancel
				soundCancel();
				assert(main_menu_frame);
				auto buttons = main_menu_frame->findFrame("buttons"); assert(buttons);
				Button* quit_button = buttons->findButton("Drop Out"); assert(quit_button);
				quit_button->select();
			    closeBinary();
			});
	}

	static void mainRestartGame(Button& button) {
	    const char* prompt;
	    if (gameModeManager.currentMode == GameModeManager_t::GameModes::GAME_MODE_DEFAULT) {
	        prompt = "Are you sure you want to restart?\nThis adventure will be lost forever.";
	    } else {
	        prompt = "Are you sure you want to restart\nthe current trial?";
	    }
		binaryPrompt(
			prompt, // window text
			"Restart", // okay text
			"Cancel", // cancel text
			[](Button&){ // okay
				soundActivate();
				destroyMainMenu();
				createDummyMainMenu();
				if (gameModeManager.currentMode == GameModeManager_t::GameModes::GAME_MODE_DEFAULT) {
					deleteSaveGame(multiplayer);
					beginFade(MainMenu::FadeDestination::GameStart);
				} else {
				    tutorial_map_destination = map.filename;
					beginFade(MainMenu::FadeDestination::HallOfTrials);
				}

				// set unique game key
				local_rng.seedTime();
				local_rng.getSeed(&uniqueGameKey, sizeof(uniqueGameKey));
				net_rng.seedBytes(&uniqueGameKey, sizeof(uniqueGameKey));

				if (multiplayer == SERVER) {
                    for (int c = 1; c < MAXPLAYERS; c++) {
	                    if (client_disconnected[c]) {
		                    continue;
	                    }
	                    memcpy((char*)net_packet->data, "RSTR", 4);
	                    SDLNet_Write32(svFlags, &net_packet->data[4]);
	                    SDLNet_Write32(uniqueGameKey, &net_packet->data[8]);
	                    net_packet->data[12] = 0;
	                    net_packet->address.host = net_clients[c - 1].host;
	                    net_packet->address.port = net_clients[c - 1].port;
	                    net_packet->len = 13;
	                    sendPacketSafe(net_sock, -1, net_packet, c - 1);
                    }
				}
			},
			[](Button&){ // cancel
				soundCancel();
				assert(main_menu_frame);
				auto buttons = main_menu_frame->findFrame("buttons"); assert(buttons);
				Button* quit_button;
				if (gameModeManager.currentMode == GameModeManager_t::GameModes::GAME_MODE_DEFAULT) {
				    quit_button = buttons->findButton("Restart Game"); assert(quit_button);
				} else {
				    quit_button = buttons->findButton("Restart Trial"); assert(quit_button);
				}
				quit_button->select();
			    closeBinary();
			});
	}

	static void mainReturnToHallofTrials(Button& button) {
	    const char* prompt;
	    if (strcmp(map.filename, "tutorial_hub.lmp")) {
	        prompt = "Are you sure you want to return\nto the Hall of Trials?";
	    } else {
	        prompt = "Are you sure you want to reset\nthe Hall of Trials level?";
	    }
		binaryPrompt(
			prompt, // window text
			"Okay", // okay text
			"Cancel", // cancel text
			[](Button&){ // okay
				soundActivate();
				destroyMainMenu();
				createDummyMainMenu();
				tutorial_map_destination = "tutorial_hub";
				beginFade(MainMenu::FadeDestination::HallOfTrials);
			},
			[](Button&){ // cancel
				soundCancel();
				assert(main_menu_frame);
				auto buttons = main_menu_frame->findFrame("buttons"); assert(buttons);
				auto quit = buttons->findButton("Return to Hall of Trials"); assert(quit);
				quit->select();
			    closeBinary();
			});
	}

	static void mainQuitToMainMenu(Button& button) {
	    const char* prompt;
	    if (saveGameExists(multiplayer == SINGLE)) {
	        prompt = "All progress before the current\ndungeon level will be saved.";
	    } else {
	        prompt = "Are you sure you want to return\nto the main menu?";
	    }
		binaryPrompt(
			prompt, // window text
			"Quit to Menu", // okay text
			"Cancel", // cancel text
			[](Button&){ // okay
				soundActivate();
				destroyMainMenu();
				createDummyMainMenu();
                if (saveGameExists(multiplayer == SINGLE)) {
                    beginFade(MainMenu::FadeDestination::RootMainMenu);
                } else {
                    beginFade(MainMenu::FadeDestination::Endgame);
                }
			},
			[](Button&){ // cancel
				soundCancel();
				assert(main_menu_frame);
				auto buttons = main_menu_frame->findFrame("buttons"); assert(buttons);
				auto quit_button = buttons->findButton("Quit to Main Menu"); assert(quit_button);
				quit_button->select();
			    closeBinary();
			});
	}

	static void mainQuitToDesktop(Button& button) {
		static const char* quit_messages[][3] {
			{"You want to leave, eh?\nThen get out and don't come back!", "Fine geez", "Never!"},
			{"Did the wittle gobwins\nhurt your feewings?", "Yes", "No"},
			{"Just cancel your plans.\nI'll wait.", "Good luck", "Sure"},
			{"You couldn't kill the lich anyway.", "You're right", "Oh yeah?"},
			{"Mad cuz bad!\nGit gud!", "I am anger", "Okay"},
			{"The gnomes are laughing at you!\nAre you really gonna take that?", "Yeah :(", "No way!"},
			{"Don't go now! There's a\nboulder trap around the corner!", "Kill me", "Oh thanks"},
			{"I'll tell your parents\nyou said a bad word.", "Poop", "Please no"},
			{"Please don't leave!\nThere's more treasure to loot!", "Don't care", "More loot!"},
			{"Just be glad I can't summon\nthe minotaur in real life.", "Too bad", "Point taken"},
			{"Off to leave a salty review I see?", "... yeah", "No way!"},
			{"I'd leave too.\nThis game looks just like Minecraft.", "lol", "Ouch"},
			{"Okay, I see how it is.\nSee if I'm still here tomorrow.", "Whatever", "I love you!"},
			{"Don't quit now, you were\ngoing to win the next run.", "Don't lie", "Really?"},
			{"A wimpy adventurer\nwould quit right now.", "Wimp time", "Am not!"},
			{"An adventurer is trapped\nand they need your help!", "Who cares", "I'll do it!"},
			{"A quitter says what?", "What?", "Not today"},
			{"You won't quit.\nYou're just bluffing.", "Bet", "Ya got me."},
			{"Say one more thing and\nI'm kicking you out of this game.", "Thing", "..."},
		};
		constexpr int num_quit_messages = sizeof(quit_messages) / (sizeof(const char*) * 3);

		static int quit_motd = -2;
		++quit_motd;
		if (quit_motd >= num_quit_messages) {
			quit_motd = 0;
		}
		if (quit_motd < 0) {
			quit_motd = RNG.uniform(0, num_quit_messages - 1);
		}

		binaryPrompt(
			quit_messages[quit_motd][0], // window text
			quit_messages[quit_motd][1], // okay text
			quit_messages[quit_motd][2], // cancel text
			[](Button&){ // okay
				soundActivate();
				mainloop = 0;
			},
			[](Button&){ // cancel
				soundCancel();
				assert(main_menu_frame);
				auto buttons = main_menu_frame->findFrame("buttons"); assert(buttons);
				auto quit_button = buttons->findButton("Quit");
				if (!quit_button) {
					quit_button = buttons->findButton("Quit to Desktop");
				}
				assert(quit_button);
				quit_button->select();
			    closeBinary();
			});
	}

	static void mainClose(Button& button) {
		soundActivate();
		closeMainMenu();
	}

/******************************************************************************/


    static void handleFadeFinished(bool ingame) {
		if (main_menu_fade_destination == FadeDestination::None) {
			// generally speaking, this shouldn't ever happen. if it did: fix your shit!
			// if for some reason this happens in release mode, just boot the player to the main menu.
			assert(0 &&
				"Set a FadeDestination so the new menu manager knows where to kick the player to."
				"Don't know where? Try MainMenu::FadeDestination::RootMainMenu");
			main_menu_fade_destination = FadeDestination::RootMainMenu;
		} else {
		    if (main_menu_fade_destination == FadeDestination::TitleScreen) {
				if (ingame) {
#ifdef NINTENDO
					if (!nxIsHandheldMode()) {
						nxAssignControllers(1, 1, true, false, true, false, nullptr);
					}
#endif
					// end the game, but do NOT create a highscore.
					// this is because we are coming here from some unknown place.
					// presumably the current game has been saved, so we're not ending it.
					// just return to the main menu but don't save a score
					doEndgame(false);
				}

				// return to title screen
		        destroyMainMenu();
				const int music = RNG.uniform(0, NUMINTROMUSIC - 2);
	            playMusic(intromusic[music], true, false, false);
				createTitleScreen();

#ifndef NINTENDO
				// unbind controllers
				for (int c = 1; c < MAX_SPLITSCREEN; ++c) {
					if (inputs.hasController(c)) {
						inputs.removeControllerWithDeviceID(inputs.getControllerID(c));
						Input::inputs[c].refresh();
					}
				}
#endif
		    }
			else if (main_menu_fade_destination == FadeDestination::RootMainMenu) {
				if (ingame) {
					// end current game
#ifdef NINTENDO
					if (!nxIsHandheldMode()) {
						nxAssignControllers(1, 1, true, false, true, false, nullptr);
					}
#endif
                    // end the game, but do NOT create a highscore.
                    // this is because we are coming here from some unknown place.
                    // presumably the current game has been saved, so we're not ending it.
                    // just return to the main menu but don't save a score
                    doEndgame(false);
				}

				// return to menu
				destroyMainMenu();
				const int music = RNG.uniform(0, NUMINTROMUSIC - 2);
	            playMusic(intromusic[music], true, false, false);
				createMainMenu(false);

				// join lobby we've been invited to
				if (saved_invite_lobby) {
				    connectToServer(nullptr, saved_invite_lobby, LobbyType::LobbyOnline);
				    saved_invite_lobby = nullptr;
				}

#ifndef NINTENDO
				// unbind controllers
				for (int c = 1; c < MAX_SPLITSCREEN; ++c) {
					if (inputs.hasController(c)) {
						inputs.removeControllerWithDeviceID(inputs.getControllerID(c));
						Input::inputs[c].refresh();
					}
				}
#endif
			}
            else if (main_menu_fade_destination == FadeDestination::Endgame) {
                destroyMainMenu();
                if (ingame) {
#ifdef NINTENDO
                    if (!nxIsHandheldMode()) {
                        nxAssignControllers(1, 1, true, false, true, false, nullptr);
                    }
#endif
                    // end the game AND create a highscore!
                    // this is because the game is well and truly done. There is no save file.
                    // create a highscore as token of remembrance.
                    doEndgame(true);
                }
                const int music = RNG.uniform(0, NUMINTROMUSIC - 2);
                playMusic(intromusic[music], true, false, false);
                createMainMenu(false);
                if (saved_invite_lobby) {
                    connectToServer(nullptr, saved_invite_lobby, LobbyType::LobbyOnline);
                    saved_invite_lobby = nullptr;
                }

#ifndef NINTENDO
                for (int c = 1; c < MAX_SPLITSCREEN; ++c) {
                    if (inputs.hasController(c)) {
                        inputs.removeControllerWithDeviceID(inputs.getControllerID(c));
                        Input::inputs[c].refresh();
                    }
                }
#endif
            }
			else if (main_menu_fade_destination == FadeDestination::Victory) {
#ifdef NINTENDO
				if (!nxIsHandheldMode()) {
					nxAssignControllers(1, 1, true, false, true, false, nullptr);
				}
#endif
                // end the game AND create a highscore!
                // this is because the game is well and truly done. There is no save file.
                // create a highscore as token of remembrance.
				doEndgame(true);
				destroyMainMenu();
				createDummyMainMenu();
				createCreditsScreen(true);
	            playMusic(intromusic[0], true, false, false);

#ifndef NINTENDO
				for (int c = 1; c < MAX_SPLITSCREEN; ++c) {
					if (inputs.hasController(c)) {
						inputs.removeControllerWithDeviceID(inputs.getControllerID(c));
						Input::inputs[c].refresh();
					}
				}
#endif
			}
			else if (main_menu_fade_destination == FadeDestination::IntroStoryScreen) {
				destroyMainMenu();
				createDummyMainMenu();
				createStoryScreen("data/story/intro.json", [](){beginFade(FadeDestination::RootMainMenu);});
				playMusic(introstorymusic, false, true, false);
			}
			else if (main_menu_fade_destination == FadeDestination::IntroStoryScreenNoMusicFade) {
				destroyMainMenu();
				createDummyMainMenu();
				createStoryScreen("data/story/intro.json", [](){beginFade(FadeDestination::TitleScreen);});
				playMusic(introstorymusic, false, false, false);
			}
			else if (main_menu_fade_destination >= FadeDestination::HerxMidpointHuman &&
			    main_menu_fade_destination <= FadeDestination::ClassicBaphometEndingEvil) {
				destroyMainMenu();
				createDummyMainMenu();
				struct Scene {
				    const char* filename;
				    void (*end_func)();
				};
				auto classicEnding = [](){(void)beginFade(FadeDestination::Victory);};
				auto fullEnding = [](){(void)beginFade(FadeDestination::Victory); steamAchievement("BARONY_ACH_ALWAYS_WAITING");};
				auto loadNextLevel = [](){if (multiplayer != CLIENT) {loadnextlevel = true; skipLevelsOnLoad = 0; pauseGame(1, false);}};
				auto skipHellLevels = [](){if (multiplayer != CLIENT) {loadnextlevel = true; skipLevelsOnLoad = 5; pauseGame(1, false);}};
				Scene scenes[] = {
				    {"data/story/HerxMidpointHuman.json", skipHellLevels},
				    {"data/story/HerxMidpointAutomaton.json", skipHellLevels},
				    {"data/story/HerxMidpointBeast.json", skipHellLevels},
				    {"data/story/HerxMidpointEvil.json", skipHellLevels},

				    {"data/story/BaphometMidpointHuman.json", loadNextLevel},
				    {"data/story/BaphometMidpointAutomaton.json", loadNextLevel},
				    {"data/story/BaphometMidpointBeast.json", loadNextLevel},
				    {"data/story/BaphometMidpointEvil.json", loadNextLevel},

				    {"data/story/EndingHuman.json", fullEnding},
				    {"data/story/EndingAutomaton.json", fullEnding},
				    {"data/story/EndingBeast.json", fullEnding},
				    {"data/story/EndingEvil.json", fullEnding},

				    {"data/story/ClassicEndingHuman.json", classicEnding},
				    {"data/story/ClassicEndingAutomaton.json", classicEnding},
				    {"data/story/ClassicEndingBeast.json", classicEnding},
				    {"data/story/ClassicEndingEvil.json", classicEnding},

				    {"data/story/ClassicBaphometEndingHuman.json", classicEnding},
				    {"data/story/ClassicBaphometEndingAutomaton.json", classicEnding},
				    {"data/story/ClassicBaphometEndingBeast.json", classicEnding},
				    {"data/story/ClassicBaphometEndingEvil.json", classicEnding},
				};
				constexpr int num_scenes = sizeof(scenes) / sizeof(scenes[0]);
				int scene = (int)main_menu_fade_destination - (int)FadeDestination::HerxMidpointHuman;
				assert(scene >= 0 && scene < num_scenes);
				createStoryScreen(scenes[scene].filename, scenes[scene].end_func);
				playMusic(endgamemusic, false, false, false);
			}
			else if (main_menu_fade_destination == FadeDestination::GameStart) {
				gameModeManager.setMode(GameModeManager_t::GAME_MODE_DEFAULT);

				// set save game file index
				if (!loadingsavegame) {
					const bool singleplayer = (multiplayer == SINGLE);
					bool foundEmptySlot = false;
	                for (int i = 0; i < SAVE_GAMES_MAX; ++i) {
						// look for an empty save slot
                        if (!saveGameExists(singleplayer, i)) {
                            savegameCurrentFileIndex = i;
							foundEmptySlot = true;
                            break;
                        }
                    }
					if (!foundEmptySlot) {
						// no empty save slots, look for the oldest one to overwrite
						using list_type = std::pair<int, SaveGameInfo>;
						std::list<list_type> savegames;
						for (int i = 0; i < SAVE_GAMES_MAX; ++i) {
							if (saveGameExists(singleplayer, i)) {
								savegames.emplace_back(i, getSaveGameInfo(singleplayer, i));
							}
						}
						assert(!savegames.empty());
						savegames.sort([](const list_type& lhs, const list_type& rhs) {
							return rhs.second.timestamp > lhs.second.timestamp;
							});
						savegameCurrentFileIndex = savegames.front().first;
					}
				}

				// set clientnum and client_disconnected[] based on the state of the lobby
				if (currentLobbyType == LobbyType::LobbyLocal) {
		            clientnum = -1;
		            int playercount = 0;
		            for (int c = 0; c < MAX_SPLITSCREEN; ++c) {
			            if (playersInLobby[c]) {
				            clientnum = clientnum == -1 ? c : clientnum;
				            client_disconnected[c] = false;
				            ++playercount;
			            } else {
				            client_disconnected[c] = true;
			            }
		            }
		            splitscreen = playercount >= 2; // necessary for save file to be valid
		            assert(playercount > 0 && clientnum != -1);
		        } else if (currentLobbyType != LobbyType::None) {
		            // this is an online game. make SURE the splitscreen variable is false
					// (are we really, really certain this code is needed? assert here and check)
		            splitscreen = false;
		        }

				doNewGame(!intro);
				destroyMainMenu();
				setupSplitscreen();

                // don't show the first time prompt anymore
				if (gameModeManager.Tutorial.FirstTimePrompt.showFirstTimePrompt) {
					gameModeManager.Tutorial.FirstTimePrompt.showFirstTimePrompt = false;
					gameModeManager.Tutorial.writeToDocument();
				}
			}
			else if (main_menu_fade_destination == FadeDestination::GameStartDummy) {
			    // do nothing.
			    // this state exists so that clients can begin fading without
			    // starting the game too early.
			    return;
			}
			else if (main_menu_fade_destination == FadeDestination::HallOfTrials) {
				destroyMainMenu();
				multiplayer = SINGLE;
				numplayers = 0;
				loadingsavegame = 0;
				gameModeManager.setMode(GameModeManager_t::GAME_MODE_TUTORIAL_INIT);

                // don't show the first time prompt anymore
				if (gameModeManager.Tutorial.FirstTimePrompt.showFirstTimePrompt) {
					gameModeManager.Tutorial.FirstTimePrompt.showFirstTimePrompt = false;
					gameModeManager.Tutorial.writeToDocument();
				}

                // if restarting, be sure to call gameModeManager.Tutorial.onMapRestart()
				if (ingame) {
				    int tutorialNum = -1;
		            for (int i = 0; tutorial_map_destination[i]; ++i) {
		                auto c = tutorial_map_destination[i];
		                if (c >= '0' && c <= '9') {
		                    tutorialNum = (int)strtol(tutorial_map_destination.c_str() + i, nullptr, 10);
		                }
		            }
		            if (tutorialNum > 0 && tutorialNum <= gameModeManager.Tutorial.getNumTutorialLevels()) {
				        gameModeManager.Tutorial.onMapRestart(tutorialNum);
				    }
				}

				gameModeManager.Tutorial.startTutorial(tutorial_map_destination);
				steamStatisticUpdate(STEAM_STAT_TUTORIAL_ENTERED, ESteamStatTypes::STEAM_STAT_INT, 1);
				doNewGame(!intro);
				setupSplitscreen();
			}
			fadeout = false;
			main_menu_fade_destination = FadeDestination::None;
		}
	}

	void doMainMenu(bool ingame) {
        if (video_refresh) {
			Frame::guiResize(0, 0); // resize gui for new aspect ratio
            createMainMenu(!intro);

            // return to video settings window
            assert(main_menu_frame);
            auto buttons = main_menu_frame->findFrame("buttons"); assert(buttons);
            auto settings_button = buttons->findButton("Settings"); assert(settings_button);
            settings_button->activate();
            auto settings = main_menu_frame->findFrame("settings"); assert(settings);
            if (video_refresh & VideoRefresh::Video) {
                auto video = settings->findButton("Video"); assert(video);
                video->activate();
            }
            else if (video_refresh & VideoRefresh::General) {
                auto general = settings->findButton("General"); assert(general);
                general->activate();
            }

            // setup timeout to revert resolution
            char buf[256];
            static Uint32 resolution_timeout;
            static constexpr Uint32 timeout_seconds = 10;
            resolution_timeout = ticks + timeout_seconds * TICKS_PER_SECOND;
            static const char* fmt = "Does the screen look okay?\n%llu...";
            snprintf(buf, sizeof(buf), fmt, timeout_seconds);

            // reset resolution function
		    static auto resetResolution = [](){
		        allSettings.video = old_video;
		        if (allSettings.video.save()) {
		            int x = std::max(allSettings.video.resolution_x, 1024);
		            int y = std::max(allSettings.video.resolution_y, 720);
			        if (!changeVideoMode(x, y)) {
			            printlog("critical error! Attempting to abort safely...\n");
			            mainloop = 0;
		            }
		        }
		        };

            // open prompt
            auto prompt = binaryPrompt(
                buf, "Yes", "No",
                [](Button&){ // yes
                    soundActivate();
                    closeBinary();

                    assert(main_menu_frame);
			        auto settings = main_menu_frame->findFrame("settings"); assert(settings);
			        auto settings_subwindow = settings->findFrame("settings_subwindow"); assert(settings_subwindow);
		            settingsSelect(*settings_subwindow, {Setting::Type::Dropdown, "resolution"});
                },
                [](Button&){ // no
                    soundCancel();
                    closeBinary();
                    resetResolution();

                    assert(main_menu_frame);
			        auto settings = main_menu_frame->findFrame("settings"); assert(settings);
			        auto settings_subwindow = settings->findFrame("settings_subwindow"); assert(settings_subwindow);
					settingsSelect(*settings_subwindow, { Setting::Type::Dropdown, "resolution" });
                }, false, false); // yellow buttons

            // prompt timeout
            if (prompt) {
                prompt->setTickCallback([](Widget& widget){
                    const int seconds = (resolution_timeout - ticks) / TICKS_PER_SECOND;
                    if ((int)resolution_timeout - (int)ticks > 0) {
                        auto prompt = static_cast<Frame*>(&widget);
                        auto text = prompt->findField("text");
                        char buf[256];
                        snprintf(buf, sizeof(buf), fmt, seconds + 1);
                        text->setText(buf);
                    } else {
                        soundCancel();
                        closeBinary();
                        resetResolution();
                    }
                    });
            }

            // at the end so that old_video is not overwritten
            video_refresh = VideoRefresh::None;
        }

		if (!main_menu_frame) {
		    if (ingame) {
		        if (movie || fadeout) {
		            createDummyMainMenu();
		        } else {
		            createMainMenu(ingame);
		        }
		    } else {
			    createTitleScreen();
			}
			assert(main_menu_frame);
		}

#ifdef NINTENDO
		enabledDLCPack1 = nxCheckDLC(0);
		enabledDLCPack2 = nxCheckDLC(1);
#endif

#ifdef STEAMWORKS
		enabledDLCPack1 = SteamApps()->BIsDlcInstalled(1010820);
		enabledDLCPack2 = SteamApps()->BIsDlcInstalled(1010821);
#endif // STEAMWORKS

        if (!ingame) {
            handleNetwork();

			if ( currentLobbyType != LobbyType::None )
			{
				int oldArachnophobiaFilter = GameplayPreferences_t::getGameConfigValue(GameplayPreferences_t::GOPT_ARACHNOPHOBIA);
				int oldColorblindFilter = GameplayPreferences_t::getGameConfigValue(GameplayPreferences_t::GOPT_COLORBLIND);
				for ( int i = 0; i < MAXPLAYERS; ++i )
				{
					gameplayPreferences[i].process();
				}
				colorblind_lobby = GameplayPreferences_t::getGameConfigValue(GameplayPreferences_t::GOPT_COLORBLIND);
				if ( GameplayPreferences_t::getGameConfigValue(GameplayPreferences_t::GOPT_ARACHNOPHOBIA) != oldArachnophobiaFilter )
				{
					if ( GameplayPreferences_t::gameConfig[GameplayPreferences_t::GOPT_ARACHNOPHOBIA].value != 0 )
					{
						addLobbyChatMessage(uint32ColorWhite, language[4333]);
					}
					else
					{
						addLobbyChatMessage(uint32ColorWhite, language[4334]);
					}
				}
				if ( GameplayPreferences_t::getGameConfigValue(GameplayPreferences_t::GOPT_COLORBLIND) != oldColorblindFilter )
				{
					if ( GameplayPreferences_t::gameConfig[GameplayPreferences_t::GOPT_COLORBLIND].value != 0 )
					{
						addLobbyChatMessage(uint32ColorWhite, language[4342]);
					}
					else
					{
						addLobbyChatMessage(uint32ColorWhite, language[4343]);
					}
				}
			}
        } else {
#ifdef STEAMWORKS
			if (ticks % 250 == 0) {
				bool unlocked = false;
				if (SteamUserStats()->GetAchievement("BARONY_ACH_GUDIPARIAN_BAZI", &unlocked)) {
					if ( unlocked ) {
						steamAchievement("BARONY_ACH_RANGER_DANGER");
					}
				}
			}
#endif
        }

        // if no controller is connected, you can always connect one just for the main menu.
        if (!ingame && currentLobbyType == LobbyType::None) {
            if (!inputs.hasController(getMenuOwner())) {
                Input::waitingToBindControllerForPlayer = getMenuOwner();
            }
        }

		// hide mouse if we're driving around with a controller
        auto cmouse = inputs.getVirtualMouse(inputs.getPlayerIDAllowedKeyboard());
        cmouse->draw_cursor = isMouseVisible();

		static ConsoleVariable<bool> cvar_disableFadeFinished("/test_disable_fade_finished", false);
		if (fadeout && fadealpha >= 255 && !*cvar_disableFadeFinished) {
            handleFadeFinished(ingame);
        }
	}

	static std::string getVersionString() {
        char date[64];
		strcpy(date, __DATE__ + 7);
		strcat(date, ".");
		Uint32 month = SDLNet_Read32((void*)__DATE__);
		switch (month) {
		case 'Jan ': strcat(date, "01."); break;
		case 'Feb ': strcat(date, "02."); break;
		case 'Mar ': strcat(date, "03."); break;
		case 'Apr ': strcat(date, "04."); break;
		case 'May ': strcat(date, "05."); break;
		case 'Jun ': strcat(date, "06."); break;
		case 'Jul ': strcat(date, "07."); break;
		case 'Aug ': strcat(date, "08."); break;
		case 'Sep ': strcat(date, "09."); break;
		case 'Oct ': strcat(date, "10."); break;
		case 'Nov ': strcat(date, "11."); break;
		case 'Dec ': strcat(date, "12."); break;
		}
		int day = atoi(__DATE__ + 4);
		if (day >= 10) {
			strncat(date, __DATE__ + 4, 2);
		} else {
			strcat(date, "0");
			strncat(date, __DATE__ + 5, 1);
		}

        char version_buf[256];
        snprintf(version_buf, sizeof(version_buf), "%s.%s", VERSION, date);

        return std::string(version_buf);
	}

	void createTitleScreen() {
		clientnum = 0;
		inputs.setPlayerIDAllowedKeyboard(clientnum);

		main_menu_frame = gui->addFrame("main_menu");

        main_menu_frame->setOwner(getMenuOwner());
		main_menu_frame->setBorder(0);
		main_menu_frame->setSize(SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY});
		main_menu_frame->setActualSize(SDL_Rect{0, 0, main_menu_frame->getSize().w, main_menu_frame->getSize().h});
		main_menu_frame->setColor(0);
		main_menu_frame->setTickCallback(tickMainMenu);

		const auto title_scale = 4.0;
		auto title_img = Image::get("*images/system/title.png");
		auto title = main_menu_frame->addImage(
			SDL_Rect{
				(int)(Frame::virtualScreenX - (int)title_img->getWidth() * title_scale) / 2,
				Frame::virtualScreenY / 4,
				(int)(title_img->getWidth() * title_scale),
				(int)(title_img->getHeight() * title_scale)
			},
			makeColor(255, 255, 255, 255),
			title_img->getName(),
			"title"
		);

		char buf[64];
		const char date[] = __DATE__;
		const char* year = (const char*)date + sizeof(date) - 5;
		snprintf(buf, sizeof(buf), u8"Copyright \u00A9 %s, Turning Wheel LLC", year);

		auto copyright = main_menu_frame->addField("copyright", 64);
		copyright->setFont(bigfont_outline);
		copyright->setText(buf);
		copyright->setJustify(Field::justify_t::CENTER);
		copyright->setSize(SDL_Rect{
			(Frame::virtualScreenX - 512) / 2,
			Frame::virtualScreenY - 50,
			512,
			50
			});
		copyright->setColor(0xffffffff);

        auto version_str = getVersionString();
		auto version = main_menu_frame->addField("version", 32);
		version->setFont(smallfont_outline);
		version->setText(version_str.c_str());
		version->setHJustify(Field::justify_t::RIGHT);
		version->setVJustify(Field::justify_t::BOTTOM);
		version->setSize(SDL_Rect{
			Frame::virtualScreenX - 204,
			Frame::virtualScreenY - 54,
			200,
			50
			});
		version->setColor(0xffffffff);

		auto start = main_menu_frame->addFrame("start");
		start->setBorder(0);
		start->setColor(0);
		start->setSize(SDL_Rect{
			(Frame::virtualScreenX - 300) / 2,
			(Frame::virtualScreenY) / 2 - 32,
			300,
			200
			});
		start->setActualSize(SDL_Rect{0, 0, start->getSize().w, start->getSize().h});
		start->setTickCallback([](Widget& widget){
			auto frame = static_cast<Frame*>(&widget); assert(frame);
			auto button = frame->findButton("button"); assert(button);
			auto top = frame->findImage("glow_top"); assert(top);
			auto bottom = frame->findImage("glow_bottom"); assert(bottom);
			auto left = frame->findImage("glow_left"); assert(left);
			auto right = frame->findImage("glow_right"); assert(right);
			const auto color = button->getColor();
			top->color = color;
			bottom->color = color;
			left->color = color;
			right->color = color;
			});

		auto button = start->addButton("button");
		button->setSize(SDL_Rect{
		    (start->getSize().w - 292) / 2,
		    40, 292, 120 });
		button->setBorder(0);
		button->setColor(makeColor(255, 255, 255, 255));
		button->setHighlightColor(makeColor(255, 255, 255, 255));
		button->setBackground("images/ui/Main Menus/Title/UI_Title_GradLight_Pink_01.png");
		button->setIcon("images/ui/Main Menus/Title/UI_Title_Text_PressToStart_Gold_01.png");
		button->setFont(bigfont_outline);
		button->setText("Press to Start");
		button->setGlyphPosition(Widget::glyph_position_t::CENTERED);
		button->setButtonsOffset(SDL_Rect{0, 48, 0, 0});
		button->setHideKeyboardGlyphs(false);
		//button->setSelectorOffset(SDL_Rect{32, 32, -32, -32});
		button->setHideSelectors(true);
		button->addWidgetAction("MenuConfirm", "button");
		button->setCallback([](Button&){
		    destroyMainMenu();
		    createMainMenu(false);
			settingsMount(false);
			(void)settingsSave();
			if (!firstTimeTutorialPrompt()) {
				soundActivate();
			}
		    });
		button->setTickCallback([](Widget& widget){
			const int pace = TICKS_PER_SECOND * 4;
            const real_t ang = PI * 2.0 * ((ticks % pace) / (real_t)pace);
			const uint8_t alpha = 191 + fabs(sin(ang)) * 64;
            const Uint32 newColor = makeColor(255, 255, 255, alpha);
		    auto button = static_cast<Button*>(&widget);
            button->setColor(newColor);
            button->setHighlightColor(newColor);
			auto selectedWidget = main_menu_frame->findSelectedWidget(getMenuOwner());
			if (!selectedWidget) {
				widget.select();
			}
		    });
		button->select();

		// borders
		{
			start->addImage(SDL_Rect{
				(start->getSize().w - 210) / 2,
				0, 210, 82 },
				0xffffffff,
				"images/ui/Main Menus/Title/UI_Title_Surround_GateTop_01.png",
				"top");

			start->addImage(SDL_Rect{
				(start->getSize().w - 70) / 2,
				start->getSize().h - 34, 70, 34 },
				0xffffffff,
				"images/ui/Main Menus/Title/UI_Title_Surround_GateBot_01.png",
				"bottom");

			start->addImage(SDL_Rect{64, start->getSize().h - 70, 46, 48 },
				0xffffffff,
				"images/ui/Main Menus/Title/UI_Title_Surround_GateBL_01.png",
				"left");

			start->addImage(SDL_Rect{start->getSize().w - 46 - 64, start->getSize().h - 70, 46, 48 },
				0xffffffff,
				"images/ui/Main Menus/Title/UI_Title_Surround_GateBR_01.png",
				"right");

			start->addImage(SDL_Rect{
				(start->getSize().w - 210) / 2,
				0, 210, 82 },
				0xffffffff,
				"images/ui/Main Menus/Title/UI_Title_Surround_GateTop_GlowPink_01B.png",
				"glow_top");

			start->addImage(SDL_Rect{
				(start->getSize().w - 70) / 2,
				start->getSize().h - 34, 70, 34 },
				0xffffffff,
				"images/ui/Main Menus/Title/UI_Title_Surround_GateBot_GlowPink_01B.png",
				"glow_bottom");

			start->addImage(SDL_Rect{64, start->getSize().h - 70, 46, 48 },
				0xffffffff,
				"images/ui/Main Menus/Title/UI_Title_Surround_GateBL_GlowPink_01B.png",
				"glow_left");

			start->addImage(SDL_Rect{start->getSize().w - 46 - 64, start->getSize().h - 70, 46, 48 },
				0xffffffff,
				"images/ui/Main Menus/Title/UI_Title_Surround_GateBR_GlowPink_01B.png",
				"glow_right");
		}

#ifdef STEAMWORKS
	    if (!cmd_line.empty()) {
	        printlog(cmd_line.c_str());
            steam_ConnectToLobby(cmd_line.c_str());
            cmd_line = "";
	    }
#endif // STEAMWORKS
	}

#ifdef STEAMWORKS
    class GetPlayersOnline {
    private:
        void OnGetNumberOfCurrentPlayers
        (NumberOfCurrentPlayers_t* callback, bool failure) {
            if (failure || !callback->m_bSuccess) {
                printlog("NumberOfCurrentPlayers_t failed!\n");
                return;
            }
            players = callback->m_cPlayers;
            printlog("Number of players currently online: %d\n", players);
        }
        CCallResult<GetPlayersOnline, NumberOfCurrentPlayers_t> result;
        int players = 0;
        Uint32 lastUpdate = 0;
    public:
        void operator()() {
            if (lastUpdate == ticks) {
                return;
            }
            printlog("SteamUserStats()->GetNumberOfCurrentPlayers()\n");
	        SteamAPICall_t call = SteamUserStats()->GetNumberOfCurrentPlayers();
	        result.Set(call, this, &MainMenu::GetPlayersOnline::OnGetNumberOfCurrentPlayers);
	        lastUpdate = ticks;
        }
        int current() {
            return players;
        }
    };
    static GetPlayersOnline getPlayersOnline;
#endif

	void createMainMenu(bool ingame) {
		if (!ingame) { 
			clientnum = 0;
			inputs.setPlayerIDAllowedKeyboard(clientnum);
		}

		main_menu_frame = gui->addFrame("main_menu");

        main_menu_frame->setOwner(getMenuOwner());
		main_menu_frame->setBorder(0);
		main_menu_frame->setSize(SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY});
		main_menu_frame->setActualSize(SDL_Rect{0, 0, main_menu_frame->getSize().w, main_menu_frame->getSize().h});
		main_menu_frame->setColor(ingame ? makeColor(0, 0, 0, 63) : 0);
		main_menu_frame->setTickCallback(tickMainMenu);

        if (!ingame) {
		    (void)createBackWidget(main_menu_frame,
		        [](Button&){soundCancel(); destroyMainMenu(); createTitleScreen();});
		};

		int y = 0;

		const auto title_scale = 3.0;
		auto title_img = Image::get("*images/system/title.png");
		auto title = main_menu_frame->addImage(
			SDL_Rect{
				(int)(Frame::virtualScreenX - (int)title_img->getWidth() * title_scale) / 2,
				y,
				(int)(title_img->getWidth() * title_scale),
				(int)(title_img->getHeight() * title_scale)
			},
			makeColor(255, 255, 255, 255),
			title_img->getName(),
			"title"
		);
		y += title->pos.h;

		auto notification = main_menu_frame->addFrame("notification");
		notification->setSize(SDL_Rect{(Frame::virtualScreenX - 236 * 2) / 2, y, 472, 98});
		notification->setActualSize(SDL_Rect{0, 0, notification->getSize().w, notification->getSize().h});
		notification->setTickCallback([](Widget& widget){
			assert(main_menu_frame);
			auto dimmer = main_menu_frame->findFrame("dimmer");
			widget.setInvisible(dimmer != nullptr);
			});

		if (ingame) {
			if (splitscreen) {
				const int player = getMenuOwner();
				const char* path = inputs.hasController(player) || inputs.getPlayerIDAllowedKeyboard() != player ?
                    Input::getControllerGlyph(player) : Input::getKeyboardGlyph(player);
				auto image = Image::get(path);
				const int w = image->getWidth();
				const int h = image->getHeight();
				const int space = 100;
				const int x = (notification->getSize().w - w) / 2;
				const int y = (notification->getSize().h - h) / 2;
				const std::string name = std::string("player") + std::to_string(player);
				notification->addImage(
					SDL_Rect{x, y, w, h},
					makeColor(255, 255, 255, 255),
					path, name.c_str());

				auto field = notification->addField(name.c_str(), 16);
				field->setSize(SDL_Rect{x, y, w, h});
				field->setJustify(Field::justify_t::CENTER);
				field->setText((std::string("P") + std::to_string(player + 1)).c_str());
				field->setFont(bigfont_outline);
				field->setColor(playerColor(player, colorblind_lobby, false));
			}
		}

		y += notification->getSize().h;
		y += 16;

		struct Option {
			const char* name;
			const char* text;
			void (*callback)(Button&);
		};
		std::vector<Option> options;
		if (ingame) {
	        options.insert(options.begin(), {
		        {"Back to Game", "BACK TO GAME", mainClose},
		        {"Assign Controllers", "ASSIGN CONTROLLERS", mainAssignControllers},
		        //{"Dungeon Compendium", "DUNGEON COMPENDIUM", archivesDungeonCompendium}, // TODO
#ifndef STEAMWORKS
#if defined(USE_EOS) || defined(LOCAL_ACHIEVEMENTS)
		        {"Achievements", "ACHIEVEMENTS", archivesAchievements},
#endif
#endif
		        {"Settings", "SETTINGS", mainSettings},
		        });
			if (gameModeManager.currentMode == GameModeManager_t::GameModes::GAME_MODE_DEFAULT) {
			    options.insert(options.end(), {
				    {"End Life", "END LIFE", mainEndLife},
				    });
				if (splitscreen && getMenuOwner() != clientnum) {
					// in splitscreen games, everyone but the first player can drop out of the game.
			        options.insert(options.end(), {
				        {"Drop Out", "DROP OUT", mainDropOut},
				        });
				}
				if (multiplayer == SERVER || (multiplayer == SINGLE && getMenuOwner() == clientnum)) {
					// only the first player has the power to restart the game.
			        options.insert(options.end(), {
				        {"Restart Game", "RESTART GAME", mainRestartGame},
				        });
				}
			} else {
			    if (strcmp(map.filename, "tutorial_hub.lmp")) {
			        options.insert(options.end(), {
				        {"Restart Trial", "RESTART TRIAL", mainRestartGame},
				        {"Return to Hall of Trials", "RETURN TO TRIAL HUB", mainReturnToHallofTrials},
				        });
				} else {
			        options.insert(options.end(), {
				        {"Return to Hall of Trials", "RETURN TO TRIAL HUB", mainReturnToHallofTrials},
				        });
				}
			}
	        options.insert(options.end(), {
		        {"Quit to Main Menu", "QUIT TO MAIN MENU", mainQuitToMainMenu},
		        //{"Quit to Desktop", "QUIT TO DESKTOP", mainQuitToDesktop},
		        });
		} else {
			options.insert(options.begin(), {
#if defined(NINTENDO)
				{"Play Game", "PLAY", mainPlayGame},
#else
				{"Play Game", "PLAY GAME", mainPlayGame},
				//{"Play Modded Game", "PLAY MODDED GAME", mainPlayModdedGame}, // TODO
#endif
				{"Adventure Archives", "ADVENTURE ARCHIVES", mainArchives},
				{"Settings", "SETTINGS", mainSettings},
#if !defined(NINTENDO)
#if !defined(NDEBUG)
			    {"Editor", "EDITOR", mainEditor},
#endif
				{"Quit", "QUIT", mainQuitToDesktop},
#endif
				});
		}

		const int num_options = (int)options.size();

        y = (Frame::virtualScreenY - num_options * 32) / 2 + 1;
		main_menu_buttons_height = y;

		auto buttons = main_menu_frame->addFrame("buttons");
		buttons->setTickCallback(updateMenuCursor);
		buttons->setSize(SDL_Rect{0, y, Frame::virtualScreenX, 36 * num_options});
		buttons->setActualSize(SDL_Rect{0, 0, buttons->getSize().w, buttons->getSize().h});
		buttons->setHollow(true);
		buttons->setBorder(0);
		for (int c = 0; c < num_options; ++c) {
			auto button = buttons->addButton(options[c].name);
			button->setCallback(options[c].callback);
			button->setBorder(8);
			button->setHJustify(Button::justify_t::LEFT);
			button->setVJustify(Button::justify_t::CENTER);
			button->setText(options[c].text);
			button->setFont(menu_option_font);
			button->setBackground("*#images/ui/Main Menus/Main/UI_MainMenu_SelectorBar00.png");
			button->setHideSelectors(false);
			button->setColor(makeColor(255, 255, 255, 255));
			button->setHighlightColor(makeColor(255, 255, 255, 255));
			button->setTextColor(makeColor(180, 180, 180, 255));
			button->setTextHighlightColor(makeColor(180, 133, 13, 255));
			button->setGlyphPosition(Widget::glyph_position_t::CENTERED_RIGHT);
			button->setSize(SDL_Rect{
				(Frame::virtualScreenX - 164 * 2) / 2,
				y - buttons->getSize().y,
				164 * 2,
				16 * 2
				});
			int back = c - 1 < 0 ? num_options - 1 : c - 1;
			int forward = c + 1 >= num_options ? 0 : c + 1;
#ifdef NINTENDO
            if (ingame || c + 1 < num_options || (enabledDLCPack1 && enabledDLCPack2)) {
                button->setWidgetDown(options[forward].name);
            } else {
                button->setWidgetDown("banner1");
            }
#else
			if (ingame || c + 1 < num_options) {
			    button->setWidgetDown(options[forward].name);
			} else {
			    button->setWidgetDown("banner1");
			}
#endif
			button->setWidgetUp(options[back].name);
			if (!ingame) {
			    button->setWidgetBack("back_button");
			} else {
			    // we would like to do this, but it conflicts with other in-game controls that use
			    // B or Escape and for some reason, consuming those inputs doesn't work. Why???
				//button->setWidgetBack("Back to Game");
			}
			y += button->getSize().h;
			//y += 4;
		}
		y += 16;

		if (ingame) {
			auto achievements = main_menu_frame->addField("achievements", 256);
			achievements->setSize(SDL_Rect{ 0, buttons->getSize().y + buttons->getSize().h + 2, main_menu_frame->getSize().w, 32 });
			achievements->setFont(smallfont_outline);
			achievements->setHJustify(Field::justify_t::CENTER);
			achievements->setVJustify(Field::justify_t::TOP);
			achievements->setTickCallback([](Widget& widget) {
				Field* achievements = static_cast<Field*>(&widget);
				if (conductGameChallenges[CONDUCT_CHEATS_ENABLED]
					|| conductGameChallenges[CONDUCT_LIFESAVING]
					|| gamemods_disableSteamAchievements) {
					achievements->setColor(makeColor(180, 37, 37, 255));
					achievements->setText("ACHIEVEMENTS DISABLED");
				}
				else {
					achievements->setColor(makeColor(37, 90, 255, 255));
					achievements->setText(""); // "ACHIEVEMENTS ENABLED"
				}
				});
			(*achievements->getTickCallback())(*achievements);
		}

		auto button = buttons->findButton(ingame ? "Back to Game" : "Play Game");
		if (button) {
			button->select();
			if (main_menu_cursor_x == 0 && main_menu_cursor_y == 0) {
				main_menu_cursor_x = button->getSize().x - 80;
				main_menu_cursor_y = button->getSize().y - 9 + buttons->getSize().y;
			}
			button->setTickCallback([](Widget& widget){
				if (!gui->findSelectedWidget(widget.getOwner())) {
					if (!main_menu_frame || !main_menu_frame->findWidget("dimmer", false)) {
						widget.select();
					}
				}
				});
		}

		main_menu_frame->addImage(
			SDL_Rect{
				main_menu_cursor_x + (int)(sinf(main_menu_cursor_bob) * 16.f) - 16,
				main_menu_cursor_y,
				37 * 2,
				23 * 2
			},
			0xffffffff,
			"*images/ui/Main Menus/UI_Pointer_Spear00.png",
			"cursor"
		);

		if (!ingame) {
		    const char* banner_images[][2] = {
		        {
		            "*#images/ui/Main Menus/Banners/UI_MainMenu_QoDPatchNotes1_base.png",
		            "*#images/ui/Main Menus/Banners/UI_MainMenu_QoDPatchNotes1_high.png",
		        },
		        {
		            "#images/ui/Main Menus/Banners/UI_MainMenu_DiscordLink_base.png",
		            "#images/ui/Main Menus/Banners/UI_MainMenu_DiscordLink_high.png",
		        },
		    };
		    if (!enabledDLCPack1 && !enabledDLCPack2) {
		        banner_images[0][0] = "*#images/ui/Main Menus/Banners/UI_MainMenu_ComboBanner1_base.png";
		        banner_images[0][1] = "*#images/ui/Main Menus/Banners/UI_MainMenu_ComboBanner1_high.png";
		    }
		    else if (!enabledDLCPack1) {
		        banner_images[0][0] = "*#images/ui/Main Menus/Banners/UI_MainMenu_MnOBanner1_base.png";
		        banner_images[0][1] = "*#images/ui/Main Menus/Banners/UI_MainMenu_MnOBanner1_high.png";
		    }
		    else if (!enabledDLCPack2) {
		        banner_images[0][0] = "*#images/ui/Main Menus/Banners/UI_MainMenu_LnPBanner1_base.png";
		        banner_images[0][1] = "*#images/ui/Main Menus/Banners/UI_MainMenu_LnPBanner1_high.png";
		    }
		    void(*banner_funcs[])(Button&) = {
		        [](Button&){ // banner #1
		        if (enabledDLCPack1 && enabledDLCPack2) {
                    openURLTryWithOverlay("https://turningwheelgames.com/blog/2022/11/qodbeta");
                } else {
					openDLCPrompt(enabledDLCPack1 ? 1 : 0);
                }
		        },
		        [](Button&){ // banner #2
                openURLTryWithOverlay("https://discord.gg/xDhtaR9KA2");
		        },
		    };
#ifdef NINTENDO
			const int num_banners = (enabledDLCPack1 && enabledDLCPack2) ?
                0 : 1;
#else
		    constexpr int num_banners = sizeof(banner_funcs) / sizeof(banner_funcs[0]);
#endif
		    auto banners = main_menu_frame->addFrame("banners");
		    banners->setSize(SDL_Rect{(Frame::virtualScreenX - 472) / 2, y, 472, Frame::virtualScreenY - y});
			for (int c = 0; c < num_banners; ++c) {
				std::string name = std::string("banner") + std::to_string(c + 1);
				auto banner = banners->addButton(name.c_str());
				banner->setBackground(banner_images[c][0]);
				banner->setBackgroundHighlighted(banner_images[c][1]);
				SDL_Rect bannerPos = SDL_Rect{ 0, c * 92, 472, 76 };
				if ( auto imgGet = Image::get(banner_images[c][0]) )
				{
					bannerPos.w = imgGet->getWidth();
					bannerPos.x = 472 / 2 - bannerPos.w / 2;
				}
				banner->setSize(bannerPos);
				banner->setCallback(banner_funcs[c]);
		        banner->setButtonsOffset(SDL_Rect{0, 8, 0, 0});
				banner->setColor(uint32ColorWhite);
				banner->setHighlightColor(uint32ColorWhite);
				//banner->setHideSelectors(true);

				if (c == 0) {
#ifdef NINTENDO
					banner->setWidgetUp("Settings");
#else
				    banner->setWidgetUp("Quit");
#endif
				} else {
				    banner->setWidgetUp((std::string("banner") + std::to_string(c + 0)).c_str());
				}
				if (c == num_banners - 1) {
				    banner->setWidgetDown("Play Game");
				} else {
				    banner->setWidgetDown((std::string("banner") + std::to_string(c + 2)).c_str());
				}
				banner->setWidgetBack("back_button");

				y += banner->getSize().h;
				y += 16;
			}
			banners->setTickCallback([](Widget& widget){
				assert(main_menu_frame);
				auto dimmer = main_menu_frame->findFrame("dimmer");
				widget.setInvisible(dimmer != nullptr);
				});

			char buf[64];
			const char date[] = __DATE__;
			const char* year = (const char*)date + sizeof(date) - 5;
			snprintf(buf, sizeof(buf), u8"Copyright \u00A9 %s, Turning Wheel LLC", year);

			auto copyright = main_menu_frame->addField("copyright", 64);
			copyright->setFont(bigfont_outline);
			copyright->setText(buf);
			copyright->setJustify(Field::justify_t::CENTER);
			copyright->setSize(SDL_Rect{
				(Frame::virtualScreenX - 512) / 2,
				Frame::virtualScreenY - 50,
				512,
				50
				});
			copyright->setColor(0xffffffff);

			auto version_str = getVersionString();
			auto version = main_menu_frame->addField("version", 32);
			version->setFont(smallfont_outline);
			version->setText(version_str.c_str());
			version->setHJustify(Field::justify_t::RIGHT);
			version->setVJustify(Field::justify_t::BOTTOM);
			version->setSize(SDL_Rect{
				Frame::virtualScreenX - 204,
				Frame::virtualScreenY - 54,
				200,
				50
				});
			version->setColor(0xffffffff);

#ifdef STEAMWORKS
			auto online_players = main_menu_frame->addField("online_players", 32);
			online_players->setFont(smallfont_outline);
			online_players->setHJustify(Field::justify_t::RIGHT);
			online_players->setVJustify(Field::justify_t::TOP);
			online_players->setSize(SDL_Rect{Frame::virtualScreenX - 200, 4, 200, 50});
			online_players->setColor(0xffffffff);
			online_players->setTickCallback([](Widget& widget){
			    auto online_players = static_cast<Field*>(&widget);
                if (ticks % (TICKS_PER_SECOND * 5) == 0) {
                    getPlayersOnline();
                }
                int players = getPlayersOnline.current();
                if (players == 0) {
                    online_players->setText("Players online: ... ");
                } else {
                    char buf[256];
                    snprintf(buf, sizeof(buf), "Players online: %d ", players);
                    online_players->setText(buf);
                }
			    });
#endif
		}
	}

	void destroyMainMenu() {
		if (main_menu_frame) {
			main_menu_frame->removeSelf();
			main_menu_frame = nullptr;
		}
#ifndef NINTENDO
		scan.close(); // close network scan resources
#endif
		resetLobbyJoinFlowState();
		cursor_delete_mode = false;
		story_active = false;
		movie = false;
	}

	void createDummyMainMenu() {
		main_menu_frame = gui->addFrame("main_menu");
        main_menu_frame->setOwner(getMenuOwner());
		main_menu_frame->setSize(SDL_Rect{0, 0, Frame::virtualScreenX, Frame::virtualScreenY});
		main_menu_frame->setActualSize(SDL_Rect{0, 0, main_menu_frame->getSize().w, main_menu_frame->getSize().h});
		main_menu_frame->setHollow(true);
		main_menu_frame->setBorder(0);
		main_menu_frame->setTickCallback(tickMainMenu);
	}

	void closeMainMenu() {
		destroyMainMenu();
		gamePaused = false;
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			Input::inputs[i].consumeBindingsSharedWithBinding("MenuConfirm");
		}
	}

	void disconnectedFromServer(const char* text) {
	    // when a player is disconnected from the server
	    if (multiplayer != SINGLE) {
	        multiplayer = SINGLE;
		    disconnectFromLobby();
	        destroyMainMenu();
		    createDummyMainMenu();
#ifndef NINTENDO
	        inputs.setPlayerIDAllowedKeyboard(clientnum);
#endif
            disconnectPrompt(text);
            if (!intro) {
	            pauseGame(2, 0);
	        }
	    }
	}

	void timedOut() {
#ifdef NINTENDO
		multiplayer = SINGLE;
		disconnectFromLobby();
		destroyMainMenu();
		createDummyMainMenu();
		if (!intro) {
			pauseGame(2, 0);
		}
		beginFade(FadeDestination::RootMainMenu);
		nxEnableAutoSleep();
		nxEndParentalControls();
		nxShutdownWireless();
		logoutOfEpic();
		if (intro) {
			nxErrorPrompt(
				"The network connection to the game has been lost.",
				"The network connection to the game has been lost.\n\n"
				"Please re-establish your network connection and try again later.",
				11111);
		} else {
			nxErrorPrompt(
				"The network connection to the game has been lost.",
				"The network connection to the game has been lost.\n\n"
				"All game progress up to the current dungeon level has been saved.\n\n"
				"Please re-establish your network connection and try again later.",
				11111);
		}
#else
		disconnectedFromServer("The network connection\nhas been lost.");
#endif
	}

    void openGameoverWindow(int player, bool tutorial) {
        static ConsoleVariable<bool> purple_window("/gameover_purple", true);

        // determine if any other players are alive
		bool survivingPlayer = false;
		for (int c = 0; c < MAXPLAYERS; c++) {
			if (!client_disconnected[c] && players[c]->entity) {
				survivingPlayer = true;
				break;
			}
		}

        // determine if we made highscore list
        int placement = 1;
	    score_t* score = scoreConstructor(player);
	    Uint32 total = totalScore(score);
	    list_t* scoresPtr = multiplayer == SINGLE ? &topscores : &topscoresMultiplayer;
        for (auto node = scoresPtr->first; node != nullptr; node = node->next) {
            if (total > totalScore((score_t*)node->element)) {
                break;
            }
            ++placement;
        }
	    const bool madetop = placement <= MAXTOPSCORES;
	    scoreDeconstructor((void*)score);

		// identify all inventory items
		if (!survivingPlayer) {
		    for (int i = 0; i < MAXPLAYERS; ++i) {
			    if (!players[i]->isLocalPlayer()) {
				    continue;
			    }
	            //players[i]->shootmode = false; // open inventory
			    for (auto node = stats[i]->inventory.first; node != NULL; node = node->next) {
				    Item* item = (Item*)node->element;
				    item->identified = true;
			    }
		    }
		}

		const SDL_Rect size{
		    players[player]->camera_virtualx1(),
		    players[player]->camera_virtualy1(),
		    players[player]->camera_virtualWidth(),
		    players[player]->camera_virtualHeight(),
		};

		Frame* dimmer = gameUIFrame[player]->addFrame("gameover");
		dimmer->setActualSize(SDL_Rect{0, 0, size.w, size.h});
		dimmer->setColor(makeColor(0, 0, 0, 63));
		dimmer->setSize(size);
		dimmer->setOwner(player);
		dimmer->setBorder(0);
		dimmer->setTickCallback([](Widget& widget){
		    auto dimmer = static_cast<Frame*>(&widget);
		    if (stats[widget.getOwner()]->HP > 0) {
		        dimmer->removeSelf();
		    }
			if (gamePaused) {
				dimmer->removeSelf();
			}
		    });

		Frame* window = dimmer->addFrame("window");
		window->setSize(SDL_Rect{(size.w - 500) / 2, -336, 500, 336});
		window->setActualSize(SDL_Rect{0, 0, 500, 336});
        window->setBorder(0);
        window->setColor(0);

        window->setTickCallback([](Widget& widget){
            auto window = static_cast<Frame*>(&widget);
            auto parent = static_cast<Frame*>(window->getParent());
            auto size = window->getSize();
            auto height = (parent->getSize().h - size.h) / 2;
            if (size.y < height) {
                const int fallspeed = 80 * ((real_t)TICKS_PER_SECOND / fpsLimit);
                size.y += fallspeed;
                if (size.y >= height) {
                    size.y = height;
                    playSound(511, 48); // death knell
                    if (*cvar_fastRestart) {
                        auto restart = window->findButton("restart");
                        if (restart) {
                            restart->select();
                            restart->activate();
                        }
                    }
                }
                window->setSize(size);
            }
            });

        auto background = window->addImage(
			SDL_Rect{0, 0, 500, 336},
			0xffffffff,
			*purple_window?
			    "*images/ui/GameOver/UI_GameOver_BG_02E.png":
			    "*images/ui/GameOver/UI_GameOver_BG_02D.png",
			"background"
            );

        auto banner = window->addField("banner", 1024);
        banner->setColor(makeColor(201, 162, 100, 255));
        banner->setSize(SDL_Rect{110, 90, 280, 20});
        banner->setFont(smallfont_no_outline);
        banner->setJustify(Field::justify_t::CENTER);
        if (survivingPlayer || (multiplayer == SINGLE && !splitscreen)) {
            banner->setText("You have died.");
        } else {
            banner->setText("Your party has been wiped out.");
        }

        const char* deathStrings[] = {
            "Unknown",
            "Monster", // unused
            "Item", // unused
            "Betrayal",
            "Attempted Robbery",
            "Trespassing",
            "Arrow Trap",
            "Bear Trap",
            "Spike Trap",
            "Magic Trap",
            "Bomb Trap",
            "Boulder",
            "Lava",
            "Hot Water",
            "Invocation",
            "Starvation",
            "Poison",
            "Bleeding",
            "Burning",
            "Strangulation",
            "Funny Potion",
            "Bottomless Pit",
            "Lack of Fuel",
            "Fountain",
            "Sink",
            "Alchemy",
        };

        std::string cause_of_death;
        switch (stats[player]->killer) {
        case KilledBy::MONSTER: {
            if (stats[player]->killer_name.empty()) {
                cause_of_death = getMonsterLocalizedName(stats[player]->killer_monster);
                cause_of_death[0] = (char)toupper((int)cause_of_death[0]);
            } else {
                cause_of_death = stats[player]->killer_name;
            }
            break;
        }
        case KilledBy::ITEM: {
            cause_of_death = items[stats[player]->killer_item].getIdentifiedName();
            break;
        }
        default: {
            cause_of_death = deathStrings[(int)stats[player]->killer];
            break;
        }
        }

        const char* eulogy;
        switch (RNG.uniform(0, 9)) {
        default:
        case 0: eulogy = "We hardly knew ye."; break;
        case 1: eulogy = "Rest In Peace."; break;
        case 2: eulogy = "You will be missed."; break;
        case 3: eulogy = "Until we meet again."; break;
        case 4: eulogy = "Now at rest."; break;
        case 5: eulogy = "Mourn not my death."; break;
        case 6: eulogy = "Passed into memory."; break;
        case 7: eulogy = "In Loving Memory."; break;
        case 8: eulogy = "They sleep in memory."; break;
        case 9: eulogy = "Gone but not forgotten."; break;
        }

        char epitaph_buf[1024];
        if (tutorial) {
            snprintf(epitaph_buf, sizeof(epitaph_buf), "%s\nKilled by: %s\n\n%s",
                stats[player]->name, cause_of_death.c_str(), eulogy);
        } else {
            snprintf(epitaph_buf, sizeof(epitaph_buf), "%s\nKilled by: %s\n\n%s",
                stats[player]->name, cause_of_death.c_str(), eulogy);
        }

        auto epitaph = window->addField("epitaph", 1024);
        epitaph->setSize(SDL_Rect{106, 122, 288, 90});
        epitaph->setFont(smallfont_outline);
        if (*purple_window) {
            epitaph->setTextColor(makeColor(156, 172, 184, 255));
            epitaph->setOutlineColor(makeColor(43, 32, 46, 255));
        } else {
            epitaph->setTextColor(makeColor(168, 184, 156, 255));
            epitaph->setOutlineColor(makeColor(46, 55, 57, 255));
        }
        epitaph->setJustify(Field::justify_t::CENTER);
        epitaph->setText(epitaph_buf);

        auto footer = window->addField("footer", 1024);
        footer->setSize(SDL_Rect{94, 224, 312, 48});
        footer->setFont(smallfont_outline);
        footer->setTextColor(makeColor(170, 134, 102, 255));
        footer->setOutlineColor(makeColor(29, 16, 11, 255));
        footer->setJustify(Field::justify_t::CENTER);

        if (tutorial) {
            footer->setText("Learn from your failure\nto complete the test!");
        } else {
            if (survivingPlayer) {
                footer->setText("You will be revived if your\nparty makes it to the next level.");
            } else {
                char highscore_buf[256];
                if (madetop) {
                    snprintf(highscore_buf, sizeof(highscore_buf),
                        "You placed #%d\nin local highscores!", placement);
                } else {
                    snprintf(highscore_buf, sizeof(highscore_buf),
                        "You failed to place\nin local highscores.");
                }
                footer->setText(highscore_buf);
            }
        }

        auto dismiss_tick = [](Widget& widget){
            if (!gamePaused) {
                if (!widget.isSelected() && !widget.isToBeDeleted()) {
                    auto parent = static_cast<Frame*>(widget.getParent());
                    if (parent) {
                        for (auto button : parent->getButtons()) {
                            if (button->isSelected()) {
                                return;
                            }
                        }
                    }
                    widget.select();
                }
            }
            };

        if (survivingPlayer || multiplayer == CLIENT) {
            auto dismiss = window->addButton("dismiss");
            dismiss->setSize(SDL_Rect{(500 - 90) / 2, 294, 90, 34});
            dismiss->setColor(makeColor(255, 255, 255, 255));
            dismiss->setHighlightColor(makeColor(255, 255, 255, 255));
            dismiss->setBackground("images/ui/GameOver/UI_GameOver_Button_Dismiss_02.png");
            dismiss->setBackgroundHighlighted("images/ui/GameOver/UI_GameOver_Button_DismissHigh_02.png");
            dismiss->setBackgroundActivated("images/ui/GameOver/UI_GameOver_Button_DismissPress_02.png");
            dismiss->setText("Dismiss");
            dismiss->setFont(smallfont_outline);
            dismiss->setTextColor(makeColor(170, 134, 102, 255));
            dismiss->setTextHighlightColor(makeColor(170, 134, 102, 255));
            dismiss->setTickCallback(dismiss_tick);
            dismiss->setCallback([](Button& button){
                soundCancel();
                auto window = static_cast<Frame*>(button.getParent());
                auto frame = static_cast<Frame*>(window->getParent());
                frame->removeSelf();
                });
            dismiss->select();
        } else {
            auto quit = window->addButton("quit");
            quit->setSize(SDL_Rect{76, 294, 124, 34});
            quit->setColor(makeColor(255, 255, 255, 255));
            quit->setHighlightColor(makeColor(255, 255, 255, 255));
            quit->setBackground("images/ui/GameOver/UI_GameOver_Button_Quit_02.png");
            quit->setBackgroundHighlighted("images/ui/GameOver/UI_GameOver_Button_QuitHigh_02.png");
            quit->setBackgroundActivated("images/ui/GameOver/UI_GameOver_Button_QuitPress_02.png");
            quit->setText(tutorial ? "Back to Hub" : "Quit to Main");
            quit->setFont(smallfont_outline);
            quit->setTextColor(makeColor(170, 134, 102, 255));
            quit->setTextHighlightColor(makeColor(170, 134, 102, 255));
            if (tutorial) {
                quit->setCallback([](Button& button){
                    if (fadeout) {
                        return;
                    }
                    soundCancel();
                    //auto window = static_cast<Frame*>(button.getParent());
                    //auto frame = static_cast<Frame*>(window->getParent());
                    //frame->removeSelf();

				    pauseGame(2, 0);
				    soundActivate();
				    destroyMainMenu();
				    createDummyMainMenu();
				    tutorial_map_destination = "tutorial_hub";
				    beginFade(MainMenu::FadeDestination::HallOfTrials);
                    });
            } else {
                quit->setCallback([](Button& button){
                    if (fadeout) {
                        return;
                    }
                    soundCancel();
                    //auto window = static_cast<Frame*>(button.getParent());
                    //auto frame = static_cast<Frame*>(window->getParent());
                    //frame->removeSelf();

					deleteSaveGame(multiplayer);
				    pauseGame(2, 0);
				    destroyMainMenu();
				    createDummyMainMenu();
				    beginFade(MainMenu::FadeDestination::Endgame);
                    });
            }
            quit->setWidgetRight("restart");

            auto restart = window->addButton("restart");
            restart->setSize(SDL_Rect{202, 294, 124, 34});
            restart->setColor(makeColor(255, 255, 255, 255));
            restart->setHighlightColor(makeColor(255, 255, 255, 255));
            restart->setBackground("images/ui/GameOver/UI_GameOver_Button_Lobby_02.png");
            restart->setBackgroundHighlighted("images/ui/GameOver/UI_GameOver_Button_LobbyHigh_02.png");
            restart->setBackgroundActivated("images/ui/GameOver/UI_GameOver_Button_LobbyPress_02.png");
            restart->setText("Restart");
            restart->setFont(smallfont_outline);
            restart->setTextColor(makeColor(170, 134, 102, 255));
            restart->setTextHighlightColor(makeColor(170, 134, 102, 255));
            restart->setCallback([](Button& button){
                if (fadeout) {
                    return;
                }
                soundActivate();
                //auto window = static_cast<Frame*>(button.getParent());
                //auto frame = static_cast<Frame*>(window->getParent());
                //frame->removeSelf();

				pauseGame(2, 0);
				destroyMainMenu();
				createDummyMainMenu();
				if (gameModeManager.currentMode == GameModeManager_t::GameModes::GAME_MODE_DEFAULT) {
					deleteSaveGame(multiplayer);
					beginFade(MainMenu::FadeDestination::GameStart);
				} else {
				    tutorial_map_destination = map.filename;
					beginFade(MainMenu::FadeDestination::HallOfTrials);
				}

				// set unique game key
				local_rng.seedTime();
				local_rng.getSeed(&uniqueGameKey, sizeof(uniqueGameKey));
				net_rng.seedBytes(&uniqueGameKey, sizeof(uniqueGameKey));

				if (multiplayer == SERVER) {
                    for (int c = 1; c < MAXPLAYERS; c++) {
	                    if (client_disconnected[c]) {
		                    continue;
	                    }
	                    memcpy((char*)net_packet->data, "RSTR", 4);
	                    SDLNet_Write32(svFlags, &net_packet->data[4]);
	                    SDLNet_Write32(uniqueGameKey, &net_packet->data[8]);
	                    net_packet->data[12] = 0;
	                    net_packet->address.host = net_clients[c - 1].host;
	                    net_packet->address.port = net_clients[c - 1].port;
	                    net_packet->len = 13;
	                    sendPacketSafe(net_sock, -1, net_packet, c - 1);
                    }
				}
                });
            restart->select();
            restart->setWidgetLeft("quit");
            restart->setWidgetRight("dismiss");

            auto dismiss = window->addButton("dismiss");
            dismiss->setSize(SDL_Rect{328, 294, 96, 34});
            dismiss->setColor(makeColor(255, 255, 255, 255));
            dismiss->setHighlightColor(makeColor(255, 255, 255, 255));
            dismiss->setBackground("images/ui/GameOver/UI_GameOver_Button_Restart_02.png");
            dismiss->setBackgroundHighlighted("images/ui/GameOver/UI_GameOver_Button_RestartHigh_02.png");
            dismiss->setBackgroundActivated("images/ui/GameOver/UI_GameOver_Button_RestartPress_02.png");
            dismiss->setText("Dismiss");
            dismiss->setFont(smallfont_outline);
            dismiss->setTextColor(makeColor(170, 134, 102, 255));
            dismiss->setTextHighlightColor(makeColor(170, 134, 102, 255));
            dismiss->setTickCallback(dismiss_tick);
            dismiss->setCallback([](Button& button){
                soundCancel();
                auto window = static_cast<Frame*>(button.getParent());
                auto frame = static_cast<Frame*>(window->getParent());
                frame->removeSelf();
                });
            dismiss->setWidgetLeft("restart");
        }
    }

	void receivedInvite(void* lobby) {
	    if (intro) {
	        if (multiplayer) {
	            disconnectFromLobby();
	            destroyMainMenu();
	            createMainMenu(false);
	        }
#ifdef STEAMWORKS
            if (processLobbyInvite(lobby)) { // load any relevant save data
                connectToServer(nullptr, lobby, LobbyType::LobbyOnline);
            } else {
                const auto error = LobbyHandler_t::EResult_LobbyFailures::LOBBY_USING_SAVEGAME;
                const auto str = LobbyHandler.getLobbyJoinFailedConnectString(error);
                errorPrompt(str.c_str(), "Okay",
                [](Button&){
                soundCancel();
                closeMono();
                });
            }
#else
	        connectToServer(nullptr, lobby, LobbyType::LobbyOnline);
#endif
	    } else {
	        saved_invite_lobby = lobby;
		    disconnectFromLobby();
	        destroyMainMenu();
		    createDummyMainMenu();
	        beginFade(FadeDestination::RootMainMenu);
	        pauseGame(2, 0);
	    }
	}

	bool isPlayerSlotLocked(int index) {
	    if (index < 0 || index >= MAXPLAYERS) {
	        return true;
	    }
		return playerSlotsLocked[index];
	}

	bool isPlayerSignedIn(int index) {
	    if (index < 0 || index >= MAXPLAYERS) {
	        return false;
	    }
	    if (intro) {
	        switch (currentLobbyType) {
	        default:
	        case LobbyType::None: return false;
	        case LobbyType::LobbyLocal: {
	            auto lobby = main_menu_frame->findFrame("lobby");
	            if (!lobby) {
	                return false;
	            }
			    auto card = lobby->findFrame((std::string("card") + std::to_string(index)).c_str());
			    if (!card) {
			        return false;
			    }
			    auto backdrop = card->findImage("backdrop");
			    if (!backdrop) {
			        return false;
			    }
			    return backdrop->path != "*images/ui/Main Menus/Play/PlayerCreation/UI_Invite_Window00.png";
			}
			case LobbyType::LobbyJoined:
			case LobbyType::LobbyLAN:
			case LobbyType::LobbyOnline: {
			    if (index == clientnum) {
	                auto lobby = main_menu_frame->findFrame("lobby");
	                if (!lobby) {
	                    return false;
	                }
			        auto card = lobby->findFrame((std::string("card") + std::to_string(index)).c_str());
			        if (!card) {
			            return false;
			        }
			        auto backdrop = card->findImage("backdrop");
			        if (!backdrop) {
			            return false;
			        }
			        return backdrop->path != "*images/ui/Main Menus/Play/PlayerCreation/UI_Invite_Window00.png";
			    } else {
			        return !client_disconnected[index];
			    }
			}
			}
	    } else {
	        return !client_disconnected[index];
	    }
	}

    void controllerDisconnected(int player) {
        if (!gamePaused) {
            pauseGame(2, 0);
            destroyMainMenu();
            createDummyMainMenu();
        }

		if (!main_menu_frame) {
			return;
		}

		Frame* old_prompt;
        old_prompt = main_menu_frame->findFrame("controller_prompt");
        if (old_prompt) {
			// obviously we don't need disconnect prompts
			// if we're in the reassignment menu...
			return;
        }
        old_prompt = main_menu_frame->findFrame("controller_disconnect_prompt");
        if (old_prompt) {
            if (old_prompt->getOwner() < player) {
                // don't open a new disconnect prompt.
                // the controller would be swallowed by the current player.
                return;
            } else {
                // this player is gonna take over the controller when we plug it in,
                // so give them the prompt instead.
                destroyMainMenu();
                createDummyMainMenu();
	    	}
        }

        static real_t bounce;
        bounce = (real_t)0.0;
        auto prompt_tick_callback = [](Widget& widget){
            auto frame = static_cast<Frame*>(widget.getParent());
            const int player = frame->getOwner();
			const real_t inc = (PI / fpsLimit) * 0.5f;
            for (int c = 0; c < MAXPLAYERS; ++c) {
		        bounce += inc;
		        const real_t bounce_height = fabs(sin(bounce)) * 32.0;
                const std::string name = std::string("player") + std::to_string(player);
                auto image = frame->findImage(name.c_str());
                if (image) {
		            const int h = image->pos.h;
                    const int y = frame->getSize().h - h - 32 - (int)bounce_height;
	                image->pos.y = y;
                }
                auto field = frame->findField(name.c_str());
                if (field) {
                    auto size = field->getSize();
                    size.y = frame->getSize().h - size.h - 32 - (int)bounce_height;
                    field->setSize(size);
                }
            }
            if (inputs.getPlayerIDAllowedKeyboard() == player) {
                destroyMainMenu();
                pauseGame(1, 0); // unpause game
            } else {
                auto controller = inputs.getController(player);
                if (controller && controller->isActive()) {
                    destroyMainMenu();
                    pauseGame(1, 0); // unpause game
                }
            }
            };

		char text[1024];
        if (splitscreen) {
		    snprintf(text, sizeof(text), "Reconnect the controller for Player %d\n\n\n\n", player + 1);
        } else {
		    snprintf(text, sizeof(text), "Please reconnect your controller.\n\n\n\n");
        }

        // at this point the prompt should ALWAYS open
        // because we already handled the case where one exists above.

        auto prompt = textPrompt("controller_disconnect_prompt", text, prompt_tick_callback, false);
        assert(prompt);

        prompt->setOwner(player);

        auto header_size = prompt->getActualSize(); header_size.h = 80;
        auto header = prompt->addField("header", 128);
        header->setSize(header_size);
        header->setFont(bigfont_outline);
        header->setJustify(Field::justify_t::CENTER);
        header->setText("RECONNECT CONTROLLER");

        auto back = createBackWidget(prompt, [](Button& button){
            destroyMainMenu();
            createMainMenu(!intro);
            });
        //back->select();

        auto dimmer = static_cast<Frame*>(prompt->getParent());
        dimmer->setOwner(player);

	    int playercount = 0;
		if (multiplayer == SINGLE) {
			for (int c = 0; c < MAXPLAYERS; ++c) {
				if (isPlayerSignedIn(c)) {
					++playercount;
				}
			}
		} else {
			playercount = 1;
		}

	    int num = 0;
	    for (int c = 0; c < MAXPLAYERS; ++c) {
	        if ((multiplayer == SINGLE && isPlayerSignedIn(c)) || (multiplayer != SINGLE && c == 0)) {
                const char* path = inputs.hasController(c) || inputs.getPlayerIDAllowedKeyboard() != c ?
                    Input::getControllerGlyph(c) : Input::getKeyboardGlyph(c);
                auto image = Image::get(path);
                const int w = image->getWidth();
                const int h = image->getHeight();
                const int space = 100;
                const int x = (prompt->getSize().w - playercount * space + space - w) / 2 + num * space;
                const int y = prompt->getSize().h - h - 32;
	            const std::string name = std::string("player") + std::to_string(c);
                prompt->addImage(
                    SDL_Rect{x, y, w, h},
                    makeColor(255, 255, 255, 255),
                    path, name.c_str());

                auto field = prompt->addField(name.c_str(), 16);
                field->setSize(SDL_Rect{x, y, w, h});
                field->setJustify(Field::justify_t::CENTER);
                field->setText((std::string("P") + std::to_string(c + 1)).c_str());
                field->setFont(bigfont_outline);
				field->setColor(playerColor(c, colorblind_lobby, false));
                ++num;
	        }
	    }

        std::string path = Input::getGlyphPathForInput("ButtonA", false, Input::getControllerType(player));
        auto image = Image::get((std::string("*") + path).c_str());
        const int w = image->getWidth();
        const int h = image->getHeight();
        const int x = (prompt->getSize().w - w) / 2;
        const int y = (prompt->getSize().h - h) / 2;
        prompt->addImage(SDL_Rect{x, y, w, h}, 0xffffffff, path.c_str(), "a_button");

        soundError();
    }

    void crossplayPrompt() {
#ifdef USE_EOS
#if defined(STEAMWORKS)
        const char* prompt =
            "Enabling Crossplay allows you to host and join\n"
            "lobbies via Epic Games, Inc.\n"
            "\n"
            "By clicking Accept, you agree to share public info\n"
            "about your Steam profile with Epic Games, Inc.\n"
            "for the purpose of enabling crossplay.";
#elif defined(NINTENDO)
		const char* prompt =
			"Would you like to link your device to an online\n"
			"account hosted by Epic Games, Inc. to play online?\n"
			"\n"
			"By choosing Accept, you agree to share some info\n"
			"about your Nintendo console with Epic Games, Inc.\n"
			"for the purpose of enabling online play.";
#else
		const char* prompt =
			"Enabling online play allows you to host and join\n"
			"online lobbies hosted via Epic Games, Inc.\n"
			"\n"
			"By clicking Accept, you agree to share some info\n"
			"about your device with Epic Games, Inc. for the\n"
			"purpose of enabling online play.";
#endif
        trinaryPrompt(
            prompt,
            "Accept", "View\nPrivacy Policy", "Deny",
            [](Button&){ // accept
            soundActivate();
			closeTrinary();
            EOS.CrossplayAccountManager.acceptCrossplay();
            },
            [](Button&){ // view privacy policy
            soundActivate();
            EOS.CrossplayAccountManager.viewPrivacyPolicy();
            },
            [](Button&){ // deny
            soundCancel();
			closeTrinary();
            EOS.CrossplayAccountManager.denyCrossplay();

            // turn off button
            auto settings_subwindow = gui->findFrame("settings_subwindow");
            if (settings_subwindow) {
                auto button = settings_subwindow->findButton("setting_crossplay_button");
                if (button) {
                    button->setPressed(false);
                }
            }

            // turn off button
            auto lobby_browser_window = gui->findFrame("lobby_browser_window");
            if (lobby_browser_window) {
                auto button = lobby_browser_window->findButton("crossplay");
                if (button) {
                    button->setPressed(false);
                }
            }
            });
#endif
    }

	static ConsoleCommand ccmd_testCrossplayPrompt(
		"/test_crossplay_prompt", "Test the crossplay prompt",
		[](int argc, const char** argv) {
			crossplayPrompt();
		});
                
    /******************************************************************************/
    
    static std::string username;
    static std::string hostname;
    
    const char* getUsername() {
        return username.c_str();
    }
    
    const char* getHostname() {
        return hostname.c_str();
    }
    
    void setUsername(const char* name) {
        username = name;
    }
    
    void setHostname(const char* name) {
        hostname = name;
    }

	void randomizeUsername() {
		// generate a name like:
		// Filthy Rat #5743
		std::string monster = getMonsterLocalizedName(
			(Monster)RNG.uniform(Monster::HUMAN, Monster::MAX_MONSTER));
		monster[0] = toupper(monster[0]);
		char buf[32];
		snprintf(buf, sizeof(buf), "%s %s #%04d",
			language[4234 + RNG.uniform(0, 16)],
			monster.c_str(), RNG.uniform(1000, 9999));
		setUsername(buf);
	}

	void randomizeHostname() {
		char buf[32];
		snprintf(buf, sizeof(buf), "Room #%04d", RNG.uniform(1000, 9999));
		setHostname(buf);
	}
}
