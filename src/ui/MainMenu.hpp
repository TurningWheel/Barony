#pragma once

#include <unordered_map>
#include <string>
#include "../main.hpp"
#include "../json.hpp"
#include "../ui/Frame.hpp"

namespace MainMenu {
    extern int pause_menu_owner; // which player is driving the pause menu
	extern bool cursor_delete_mode; // if true, mouse cursor displays an extra glyph to denote delete mode (used to delete save games)
	extern Frame* main_menu_frame; // root main menu node

	// Here be new menu options:
	extern float master_volume; // range is [0 - 100]
	extern bool arachnophobia_filter; // if true, all spiders are crabs
	extern bool vertical_splitscreen; // if true, 2-player splitscreen has a vertical rather than horizontal layout
	
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

    bool isPlayerSignedIn(int index);
    bool isCutsceneActive();
	void beginFade(FadeDestination);

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

	// sounds:

	void soundToggleMenu();

	// special events:

    void controllerDisconnected(int player);
    void openGameoverWindow(int player, bool tutorial = false);
    void connectionErrorPrompt(const char* str);
	void disconnectedFromServer(const char* text);
	void receivedInvite();
	void handleScanPacket();
	void setupSplitscreen(); // resizes player game views
}
