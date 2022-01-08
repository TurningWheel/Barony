#pragma once

#include <unordered_map>
#include <string>
#include "../main.hpp"
#include "../json.hpp"
#include "../ui/Frame.hpp"

namespace MainMenu {
	// Here be new menu options:
	extern float master_volume; // range is [0 - 100]
	extern bool arachnophobia_filter; // if true, all spiders are crabs
	extern bool vertical_splitscreen; // if true, 2-player splitscreen has a vertical rather than horizontal layout
	extern bool cursor_delete_mode; // if true, mouse cursor displays an extra glyph to denote delete mode (used to delete save games)
	extern Frame* main_menu_frame;

	enum class FadeDestination : Uint8 {
		None = 0,
		RootMainMenu = 1,
		IntroStoryScreen = 2,
		IntroStoryScreenNoMusicFade = 3,
		HallOfTrials = 4,
		GameStart = 5,
	};

	void beginFade(FadeDestination);

	void settingsApply();	// write settings to global variables
	void settingsMount();	// read settings from global variables
	bool settingsSave();	// write settings to disk
	bool settingsLoad();	// read settings from disk
	void settingsReset();	// default settings

	void doMainMenu(bool ingame);       // call in a loop to update the menu
	void createMainMenu(bool ingame);   // creates a fresh main menu
	void destroyMainMenu();             // destroys the main menu tree
	void createDummyMainMenu();         // creates a main menu devoid of widgets
	void closeMainMenu();               // closes the menu and unpauses the game

	// special events:

	void disconnectedFromServer();
	void receivedInvite();
}
