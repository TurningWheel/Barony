#pragma once

#include <unordered_map>
#include <string>
#include "../main.hpp"
#include "../game.hpp"
#include "../json.hpp"
#include "../ui/Frame.hpp"
#include "../interface/consolecommand.hpp"

#define MAX_SPLITSCREEN 4

namespace MainMenu {
    extern int pause_menu_owner; // which player is driving the pause menu
	extern bool cursor_delete_mode; // if true, mouse cursor displays an extra glyph to denote delete mode (used to delete save games)
	extern Frame* main_menu_frame; // root main menu node

	// Here be new menu options:
	extern std::string current_audio_device; // guid of the audio device currently in use
	extern float master_volume; // range is [0 - 100]
	extern bool arachnophobia_filter; // if true, all spiders are crabs'
	extern ConsoleVariable<bool> vertical_splitscreen; // if true, 2-player splitscreen has a vertical rather than horizontal layout
	extern ConsoleVariable<bool> staggered_splitscreen; // if true, viewport sizes are reduced to preserve aspect ratio
	extern ConsoleVariable<bool> clipped_splitscreen; // if true, viewports rest in a corner rather than centered
    extern ConsoleVariable<bool> cvar_fastRestart;
	extern ConsoleVariable<float> cvar_worldtooltip_scale;
	extern ConsoleVariable<float> cvar_worldtooltip_scale_splitscreen;
	extern ConsoleVariable<bool> cvar_hold_to_activate;
	extern ConsoleVariable<float> cvar_enemybar_scale;
    extern ConsoleVariable<int> cvar_desiredFps;
    extern ConsoleVariable<int> cvar_displayHz;
	extern ConsoleVariable<bool> cvar_hdrEnabled;
	
	static constexpr const char* emptyBinding = "[unbound]"; // string appended to default empty bindings
	static constexpr const char* hiddenBinding = "[hidden]"; // string appended to hidden bindings on the UI

	enum class FadeDestination : Uint8 {
		None,           // don't fade anywhere (???)
		TitleScreen,    // fade to the title screen
		RootMainMenu,   // return to main menu, save no score if ingame
		RootMainMenuNoEndGame,   // return to main menu, doEndGame already done
        Endgame,        // save a highscore and return to main menu
		Victory,        // save a highscore and roll credits

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
        
        // Classic endings:

		ClassicEndingHuman,
		ClassicEndingAutomaton,
		ClassicEndingBeast,
		ClassicEndingEvil,

		ClassicBaphometEndingHuman,
		ClassicBaphometEndingAutomaton,
		ClassicBaphometEndingBeast,
		ClassicBaphometEndingEvil,

        // Game starts:

		GameStart,          // used by servers and local games
		GameStartDummy,     // used by clients to fade without really launching
		HallOfTrials,       // used to launch a hall-of-trials map
	};

    const char* getUsername();              // get local account name
    const char* getHostname();              // get local host name
    void setUsername(const char* name);     // set local account name
    void setHostname(const char* name);     // set local hostname
	void randomizeUsername();				// randomize the username
	void randomizeHostname();				// randomize the hostname
	void logoutOfEpic();					// log out of epic online services

	int getMenuOwner();					// get current pause menu owner
    bool isPlayerSignedIn(int index);   // checks whether a player is signed into a given slot
	bool isPlayerSlotLocked(int index);	// checks whether a player slot has been locked out for joining
    bool isCutsceneActive();            // checks whether we are playing a cutscene
	bool isMenuOpen();					// checks whether the menu is open
	void beginFade(FadeDestination);    // begins a fade transition to a specific destination

	void settingsApply();					// write settings to global variables (true if video mode changed)
	void settingsMount(bool video = true);	// read settings from global variables
	bool settingsSave();					// write settings to disk (true if succeeded)
	bool settingsLoad();					// read settings from disk (true if succeeded)
	void settingsReset();					// default settings

	void doMainMenu(bool ingame);           // call in a loop to update the menu
	void createTitleScreen();               // creates a fresh title screen
	void createMainMenu(bool ingame);       // creates a fresh main menu
	void destroyMainMenu();                 // destroys the main menu tree
	void createDummyMainMenu();             // creates a main menu devoid of widgets
	void closeMainMenu();                   // closes the menu and unpauses the game

	// special events:

    void controllerDisconnected(int player);                        // controller disconnect prompt, eg if a player unplugs a controller
	void tutorialFirstTimeCompleted();								// tutorial first level completed event
    void openGameoverWindow(int player, bool tutorial = false);     // opens gameover window, used when player dies
	void openCompendium();
	void disconnectedFromServer(const char* text);                  // called when the player is disconnected from the server, prompts them to end the game
	void receivedInvite(void*);                                     // called when a player receives an invite to a lobby (EOS or Steam)
	void setupSplitscreen();                                        // used to resize player game views, for example if a player drops or we change the aspect ratio
	void crossplayPrompt();                                         // user chose to activate crossplay
	void timedOut();												// special disconnection event that may display a system error message

	struct ClassDescriptions
	{
		struct DescData_t
		{
			std::string text;
			std::string internal_name;
			std::vector<std::tuple<int, std::string, Uint32>> survivalComplexity;
			std::vector<Uint32> statRatings;
			std::vector<std::string> statRatingsStrings;
			Sint32 hp = DEFAULT_HP;
			Sint32 mp = DEFAULT_MP;
			std::vector<int> linePaddings;
		};
		static std::unordered_map<int, DescData_t> data;
		static void readFromFile();
		static bool init;
		static void update_stat_growths(Frame& card, int classnum, int shapeshiftedType);
	};

	struct RaceDescriptions
	{
		struct DescData_t
		{
			std::string textLeft;
			std::string textRight;
			std::set<int> traitLines;
			std::set<int> proLines;
			std::vector<int> linePaddings;
			std::string title;
			std::string traitsBasedOnPlayerRace;
			std::string traitsBasedOnMonsterType;
			std::string resistances;
			std::string weaknesses;
			std::string friendlyWith;
			std::string racialSpells;
		};
		static std::unordered_map<std::string, DescData_t> data;
		static void readFromFile();
		static std::string getRaceKey(int race)
		{
			switch ( race )
			{
			case RACE_HUMAN:
				return "human";
			case RACE_SKELETON:
				return "skeleton";
			case RACE_VAMPIRE:
				return "vampire";
			case RACE_SUCCUBUS:
				return "succubus";
			case RACE_GOATMAN:
				return "goatman";
			case RACE_AUTOMATON:
				return "automaton";
			case RACE_INCUBUS:
				return "incubus";
			case RACE_GOBLIN:
				return "goblin";
			case RACE_INSECTOID:
				return "insectoid";
			default:
				break;
			}
			return "";
		}
		static DescData_t& getRaceDescriptionData(int race) { return data[getRaceKey(race)]; }
		static DescData_t& getMonsterDescriptionData(int type);
		static bool init;
		static void update_details_text(Frame& card);
		static void update_details_text(Frame& card, void* stats);
		static void update_details_text(Frame& card, int race, int modified_race);
	};

	struct MainMenuBanners_t
	{
		static std::string updateBannerImg;
		static std::string updateBannerImgHighlight;
		static std::string updateBannerURL;
		static void readFromFile();
	};

	enum class DLC {
		Base,
		MythsAndOutcasts,
		LegendsAndPariahs
	};

	struct Class {
		DLC dlc;
		const char* image;
		const char* image_highlighted;
		const char* image_locked;
	};

	const std::unordered_map<std::string, Class> classes = {
		{"barbarian", {
			DLC::Base,
			"ClassSelect_Icon_Barbarian_00.png",
			"ClassSelect_Icon_BarbarianOn_00.png",
			"ClassSelect_Icon_BarbarianLocked_00.png",
			}},
		{"warrior", {
			DLC::Base,
			"ClassSelect_Icon_Warrior_00.png",
			"ClassSelect_Icon_WarriorOn_00.png",
			"ClassSelect_Icon_WarriorLocked_00.png",
			}},
		{"healer", {
			DLC::Base,
			"ClassSelect_Icon_Healer_00.png",
			"ClassSelect_Icon_HealerOn_00.png",
			"ClassSelect_Icon_HealerLocked_00.png",
			}},
		{"rogue", {
			DLC::Base,
			"ClassSelect_Icon_Rogue_00.png",
			"ClassSelect_Icon_RogueOn_00.png",
			"ClassSelect_Icon_RogueLocked_00.png",
			}},
		{"wanderer", {
			DLC::Base,
			"ClassSelect_Icon_Wanderer_00.png",
			"ClassSelect_Icon_WandererOn_00.png",
			"ClassSelect_Icon_WandererLocked_00.png",
			}},
		{"cleric", {
			DLC::Base,
			"ClassSelect_Icon_Cleric_00.png",
			"ClassSelect_Icon_ClericOn_00.png",
			"ClassSelect_Icon_ClericLocked_00.png",
			}},
		{"merchant", {
			DLC::Base,
			"ClassSelect_Icon_Merchant_00.png",
			"ClassSelect_Icon_MerchantOn_00.png",
			"ClassSelect_Icon_MerchantLocked_00.png",
			}},
		{"wizard", {
			DLC::Base,
			"ClassSelect_Icon_Wizard_00.png",
			"ClassSelect_Icon_WizardOn_00.png",
			"ClassSelect_Icon_WizardLocked_00.png",
			}},
		{"arcanist", {
			DLC::Base,
			"ClassSelect_Icon_Arcanist_00.png",
			"ClassSelect_Icon_ArcanistOn_00.png",
			"ClassSelect_Icon_ArcanistLocked_00.png",
			}},
		{"joker", {
			DLC::Base,
			"ClassSelect_Icon_Jester_00.png",
			"ClassSelect_Icon_JesterOn_00.png",
			"ClassSelect_Icon_JesterLocked_00.png",
			}},
		{"sexton", {
			DLC::Base,
			"ClassSelect_Icon_Sexton_00.png",
			"ClassSelect_Icon_SextonOn_00.png",
			"ClassSelect_Icon_SextonLocked_00.png",
			}},
		{"ninja", {
			DLC::Base,
			"ClassSelect_Icon_Ninja_00.png",
			"ClassSelect_Icon_NinjaOn_00.png",
			"ClassSelect_Icon_NinjaLocked_00.png",
			}},
		{"monk", {
			DLC::Base,
			"ClassSelect_Icon_Monk_00.png",
			"ClassSelect_Icon_MonkOn_00.png",
			"ClassSelect_Icon_MonkLocked_00.png",
			}},
		{"conjurer", {
			DLC::MythsAndOutcasts,
			"ClassSelect_Icon_Conjurer_00.png",
			"ClassSelect_Icon_ConjurerOn_00.png",
			"ClassSelect_Icon_ConjurerLocked_00.png",
			}},
		{"accursed", {
			DLC::MythsAndOutcasts,
			"ClassSelect_Icon_Accursed_00.png",
			"ClassSelect_Icon_AccursedOn_00.png",
			"ClassSelect_Icon_AccursedLocked_00.png",
			}},
		{"mesmer", {
			DLC::MythsAndOutcasts,
			"ClassSelect_Icon_Mesmer_00.png",
			"ClassSelect_Icon_MesmerOn_00.png",
			"ClassSelect_Icon_MesmerLocked_00.png",
			}},
		{"brewer", {
			DLC::MythsAndOutcasts,
			"ClassSelect_Icon_Brewer_00.png",
			"ClassSelect_Icon_BrewerOn_00.png",
			"ClassSelect_Icon_BrewerLocked_00.png",
			}},
		{"mechanist", {
			DLC::LegendsAndPariahs,
			"ClassSelect_Icon_Mechanist_00.png",
			"ClassSelect_Icon_MechanistOn_00.png",
			"ClassSelect_Icon_MechanistLocked_00.png",
			}},
		{"punisher", {
			DLC::LegendsAndPariahs,
			"ClassSelect_Icon_Punisher_00.png",
			"ClassSelect_Icon_PunisherOn_00.png",
			"ClassSelect_Icon_PunisherLocked_00.png",
			}},
		{"shaman", {
			DLC::LegendsAndPariahs,
			"ClassSelect_Icon_Shaman_00.png",
			"ClassSelect_Icon_ShamanOn_00.png",
			"ClassSelect_Icon_ShamanLocked_00.png",
			}},
		{"hunter", {
			DLC::LegendsAndPariahs,
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
}
