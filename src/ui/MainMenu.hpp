#pragma once

#include <unordered_map>
#include <string>
#include "../main.hpp"
#include "../json.hpp"
#include "../ui/Frame.hpp"
#include "../interface/consolecommand.hpp"

namespace MainMenu {
    extern int pause_menu_owner; // which player is driving the pause menu
	extern bool cursor_delete_mode; // if true, mouse cursor displays an extra glyph to denote delete mode (used to delete save games)
	extern Frame* main_menu_frame; // root main menu node

	// Here be new menu options:
	extern std::string current_audio_device; // guid of the audio device currently in use
	extern float master_volume; // range is [0 - 100]
	extern bool arachnophobia_filter; // if true, all spiders are crabs'
	extern bool mkb_world_tooltips_enabled; // if true, mouse and keyboard control uses old pixel-picking for interacting
	extern ConsoleVariable<bool> vertical_splitscreen; // if true, 2-player splitscreen has a vertical rather than horizontal layout
	extern ConsoleVariable<bool> staggered_splitscreen; // if true, viewport sizes are reduced to preserve aspect ratio
	extern ConsoleVariable<bool> clipped_splitscreen; // if true, viewports rest in a corner rather than centered
    extern ConsoleVariable<bool> cvar_fastRestart;
	extern ConsoleVariable<bool> cvar_mkb_world_tooltips;
	extern ConsoleVariable<bool> cvar_mkb_facehotbar;
	extern ConsoleVariable<bool> cvar_gamepad_facehotbar;
	extern ConsoleVariable<float> cvar_worldtooltip_scale;
	extern ConsoleVariable<float> cvar_worldtooltip_scale_splitscreen;
	
	static constexpr const char* emptyBinding = "[unbound]"; // string appended to default empty bindings
	static constexpr const char* hiddenBinding = "[hidden]"; // string appended to hidden bindings on the UI

	enum class FadeDestination : Uint8 {
		None,
		TitleScreen,
		RootMainMenu,
		Victory,

		// Story scenes:

		IntroStoryScreen,
		IntroStoryScreenNoMusicFade,

		HerxMidpointHuman,
		HerxMidpointAutomaton,
		HerxMidpointBeast,
		HerxMidpointEvil,

		BaphometMidpointHuman,
		BaphometMidpointAutomaton,
		BaphometMidpointBeast,
		BaphometMidpointEvil,

		EndingHuman,
		EndingAutomaton,
		EndingBeast,
		EndingEvil,

		ClassicEndingHuman,
		ClassicEndingAutomaton,
		ClassicEndingBeast,
		ClassicEndingEvil,

		ClassicBaphometEndingHuman,
		ClassicBaphometEndingAutomaton,
		ClassicBaphometEndingBeast,
		ClassicBaphometEndingEvil,

        // Game starts:

		GameStart,
		GameStartDummy,
		HallOfTrials,
	};

    bool isPlayerSignedIn(int index);   // checks whether a player is signed into a given slot
    bool isCutsceneActive();            // checks whether we are playing a cutscene
	void beginFade(FadeDestination);    // begins a fade transition to a specific destination

	bool settingsApply();	// write settings to global variables (true if video mode changed)
	void settingsMount();	// read settings from global variables
	bool settingsSave();	// write settings to disk (true if succeeded)
	bool settingsLoad();	// read settings from disk (true if succeeded)
	void settingsReset();	// default settings

	void doMainMenu(bool ingame);           // call in a loop to update the menu
	void createTitleScreen();               // creates a fresh title screen
	void createMainMenu(bool ingame);       // creates a fresh main menu
	void destroyMainMenu();                 // destroys the main menu tree
	void createDummyMainMenu();             // creates a main menu devoid of widgets
	void closeMainMenu();                   // closes the menu and unpauses the game

	// special events:

    void controllerDisconnected(int player);                        // controller disconnect prompt, eg if a player unplugs a controller
    void openGameoverWindow(int player, bool tutorial = false);     // opens gameover window, used when player dies
	void disconnectedFromServer(const char* text);                  // called when the player is disconnected from the server, prompts them to end the game
	void receivedInvite(void*);                                     // called when a player receives an invite to a lobby (EOS or Steam)
	void setupSplitscreen();                                        // used to resize player game views, for example if a player drops or we change the aspect ratio
	void crossplayPrompt();                                         // user chose to activate crossplay
}
