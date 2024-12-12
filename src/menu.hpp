/*-------------------------------------------------------------------------------

	BARONY
	File: menu.hpp
	Desc: definitions and prototypes for menu.c

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once

#define NUMSUBTITLES 30
extern int subtitleCurrent;
extern bool subtitleVisible;

extern Sint32 gearrot;
extern Sint32 gearsize;
extern Uint16 logoalpha;
extern int credittime;
extern int creditstage;
extern int intromovietime;
extern int intromoviestage;
extern int firstendmovietime;
extern int firstendmoviestage;
extern int secondendmovietime;
extern int secondendmoviestage;
extern int thirdendmoviestage;
extern int thirdendmovietime;
extern int fourthendmoviestage;
extern int fourthendmovietime;
extern int DLCendmovieStageAndTime[8][2];
enum NewMovieStageAndTimeIndex : int
{
	MOVIE_STAGE,
	MOVIE_TIME,
};
enum NewMovieCrawlTypes : int
{
	MOVIE_MIDGAME_HERX_MONSTERS,
	MOVIE_MIDGAME_BAPHOMET_MONSTERS,
	MOVIE_MIDGAME_BAPHOMET_HUMAN_AUTOMATON,
	MOVIE_CLASSIC_WIN_MONSTERS,
	MOVIE_CLASSIC_WIN_BAPHOMET_MONSTERS,
	MOVIE_WIN_AUTOMATON,
	MOVIE_WIN_DEMONS_UNDEAD,
	MOVIE_WIN_BEASTS
};
extern bool losingConnection[MAXPLAYERS];
extern int rebindaction;

// button definitions
void buttonQuitConfirm(button_t* my);
void buttonQuitNoSaveConfirm(button_t* my);
void buttonEndGameConfirm(button_t* my);
void buttonCloseAndEndGameConfirm(button_t* my);
void buttonCloseSubwindow(button_t* my);
void buttonContinue(button_t* my);
void buttonBack(button_t* my);
void buttonVideoTab(button_t* my);
void buttonAudioTab(button_t* my);
void buttonKeyboardTab(button_t* my);
void buttonMouseTab(button_t* my);
void buttonGamepadBindingsTab(button_t* my);
void buttonGamepadSettingsTab(button_t* my);
void buttonMiscTab(button_t* my);
void buttonSettingsAccept(button_t* my);
void buttonSettingsOK(button_t* my);
void buttonStartSingleplayer(button_t* my);
void buttonStartServer(button_t* my);
void buttonHostMultiplayer(button_t* my);
void buttonJoinMultiplayer(button_t* my);
void buttonHostLobby(button_t* my);
void buttonJoinLobby(button_t* my);
void buttonDisconnect(button_t* my);
void buttonScoreNext(button_t* my);
void buttonScorePrev(button_t* my);
void buttonScoreToggle(button_t* my);
void buttonDeleteCurrentScore(button_t* my);
void buttonOpenCharacterCreationWindow(button_t* my);
void buttonDeleteSavedSoloGame(button_t* my);
void buttonDeleteSavedMultiplayerGame(button_t* my);
void buttonConfirmDeleteSoloFile(button_t* my);
void buttonConfirmDeleteMultiplayerFile(button_t* my);
void buttonLoadSingleplayerGame(button_t* my);
void buttonLoadMultiplayerGame(button_t* my);
void buttonRandomCharacter(button_t* my);
bool replayLastCharacter(const int index, int multiplayer);
void buttonOpenScoresWindow(button_t* my);
void buttonRandomName(button_t* my);
void buttonGamemodsOpenDirectory(button_t* my);
void buttonGamemodsPrevDirectory(button_t* my);
void buttonGamemodsBaseDirectory(button_t* my);
void buttonGamemodsSelectDirectoryForUpload(button_t* my);
void buttonGamemodsOpenModifyExistingWindow(button_t* my);
void buttonGamemodsStartModdedGame(button_t* my);
void buttonInviteFriends(button_t* my);

#ifdef STEAMWORKS
void buttonGamemodsPrepareWorkshopItemUpload(button_t* my);
void buttonGamemodsSetWorkshopItemFields(button_t* my);
void buttonGamemodsStartUploadItem(button_t* my);
void buttonGamemodsGetSubscribedItems(button_t* my);
void buttonGamemodsOpenSubscribedWindow(button_t* my);
void buttonGamemodsOpenUploadWindow(button_t* my);
void buttonGamemodsGetMyWorkshopItems(button_t* my);
void buttonGamemodsModifyExistingWorkshopItemFields(button_t* my);
void buttonGamemodsCancelModifyFileContents(button_t* my);
void buttonSteamLobbyBrowserJoinGame(button_t* my);
void buttonSteamLobbyBrowserRefresh(button_t* my);
void buttonGamemodsSubscribeToHostsModFiles(button_t* my);
void buttonGamemodsMountHostsModFiles(button_t* my);
void* cpp_SteamMatchmaking_GetLobbyOwner(void* steamIDLobby);
void* cpp_SteamMatchmaking_GetLobbyMember(void* steamIDLobby, int index);
void openSteamLobbyWaitWindow(button_t* my);
#elif defined USE_EOS
void buttonSteamLobbyBrowserJoinGame(button_t* my);
void buttonSteamLobbyBrowserRefresh(button_t* my);
void openSteamLobbyWaitWindow(button_t* my);
#else
void windowEnterSerialPrompt();
void windowSerialResult(int success);
size_t serialHash(const std::string& input);
extern char serialInputText[64];
#endif

#define SLIDERFONT font12x12_bmp

// achievement window
void openAchievementsWindow();
void closeAchievementsWindow(button_t*);
extern bool achievements_window;
extern int achievements_window_page;
void buttonAchievementsUp(button_t* my);
void buttonAchievementsDown(button_t* my);

// misc functions
void openSettingsWindow();
void openFailedConnectionWindow(int mode);
void openGameoverWindow();
void openSteamLobbyBrowserWindow(button_t* my);
void openLoadGameWindow(button_t* my);
void openNewLoadGameWindow(button_t* my);
void doSlider(int x, int y, int dots, int minvalue, int maxvalue, int increment, int* var, SDL_Surface* slider_font = SLIDERFONT, int slider_font_char_width = 16);
void doSliderF(int x, int y, int dots, real_t minvalue, real_t maxvalue, real_t increment, real_t* var);

// menu variables
extern bool settings_window;
extern int charcreation_step;
extern int loadGameSaveShowRectangle;
extern Uint32 charcreation_ticks;
extern bool playing_random_char;
extern int settings_tab;
extern int connect_window;
extern bool lobby_window;
extern int score_window;
extern Uint32 lobbyWindowSvFlags;

// gamemods window stuff
extern int gamemods_window;
extern int gamemods_window_scroll;
extern int gamemods_window_fileSelect;
extern int gamemods_uploadStatus;
//extern int gamemods_numCurrentModsLoaded;
extern std::list<std::string> currentDirectoryFiles;
extern std::string directoryPath;
void gamemodsWindowClearVariables();
void gamemodsCustomContentInit();
bool gamemodsDrawClickableButton(int padx, int pady, int padw, int padh, Uint32 btnColor, std::string btnText, int action);
bool gamemodsRemovePathFromMountedFiles(std::string findStr);
bool gamemodsIsPathInMountedFiles(std::string findStr);
bool gamemodsClearAllMountedPaths();
bool gamemodsMountAllExistingPaths();
//extern std::vector<std::pair<std::string, std::string>> gamemods_mountedFilepaths;
//extern bool gamemods_modelsListRequiresReload;
//extern bool gamemods_soundListRequiresReload;
//extern bool gamemods_modPreload;
#ifdef STEAMWORKS
void gamemodsWorkshopPreloadMod(int fileID, std::string modTitle);
void gamemodsWindowUploadInit(bool creatingNewItem);
void gamemodsSubscribedItemsInit();
void gamemodsDrawWorkshopItemTagToggle(std::string tagname, int x, int y);
bool gamemodsCheckIfSubscribedAndDownloadedFileID(uint64 fileID);
bool gamemodsCheckFileIDInLoadedPaths(uint64 fileID);
bool gamemodsIsClientLoadOrderMatchingHost(std::vector<std::string> serverModList);
extern std::vector<std::pair<std::string, uint64>> gamemods_workshopLoadedFileIDMap;
extern std::unordered_set<uint64> betaPlayers;
#endif // STEAMWORKS
bool drawClickableButton(int padx, int pady, int padw, int padh, Uint32 btnColor);
extern bool scoreDisplayMultiplayer;
extern std::vector<std::tuple<int, int, int, std::string>> savegamesList; // tuple - last modified, multiplayer type, file entry, and description of save game.

extern Sint32 slidery, slidersize, oslidery;

// settings window
extern Uint32 settings_fov;
extern Uint32 settings_fps;
extern int settings_xres, settings_yres;
extern bool settings_smoothlighting;
extern int settings_fullscreen, settings_shaking, settings_bobbing;
extern bool settings_borderless;
extern real_t settings_gamma;
extern int settings_sfxvolume, settings_musvolume;
extern int settings_sfxAmbientVolume;
extern int settings_sfxEnvironmentVolume;
extern int settings_impulses[NUMIMPULSES];
extern int settings_reversemouse;
extern bool settings_smoothmouse;
extern bool settings_disablemouserotationlimit;
extern real_t settings_mousespeed;
extern bool settings_broadcast;
extern bool settings_nohud;
extern bool settings_colorblind;
extern bool settings_spawn_blood;
extern bool settings_light_flicker;
extern bool settings_vsync;
extern bool settings_status_effect_icons;
extern bool settings_minimap_ping_mute;
extern bool settings_mute_audio_on_focus_lost;
extern bool settings_mute_player_monster_sounds;
extern int settings_minimap_transparency_foreground;
extern int settings_minimap_transparency_background;
extern int settings_minimap_scale;
extern int settings_minimap_object_zoom;
extern char portnumber_char[6];
extern char connectaddress[64];
extern bool usecamerasmoothing;
extern bool disablemouserotationlimit;
extern bool broadcast;
extern bool nohud;
extern int menuselect;
extern bool colorblind;
extern bool right_click_protect;
extern bool settings_auto_hotbar_new_items;
extern bool settings_auto_hotbar_categories[NUM_HOTBAR_CATEGORIES];
extern int settings_autosort_inventory_categories[NUM_AUTOSORT_CATEGORIES];
extern bool settings_hotbar_numkey_quick_add;
extern bool settings_disable_messages;
extern bool settings_right_click_protect;
extern bool settings_auto_appraise_new_items;
extern bool settings_lock_right_sidebar;
extern bool settings_show_game_timer_always;
extern bool settings_uiscale_charactersheet;
extern bool settings_uiscale_skillspage;
extern real_t settings_uiscale_hotbar;
extern real_t settings_uiscale_playerbars;
extern real_t settings_uiscale_chatlog;
extern real_t settings_uiscale_inventory;
extern bool settings_hide_statusbar;
extern bool settings_hide_playertags;
extern bool settings_show_skill_values;
extern bool settings_disableMultithreadedSteamNetworking;
extern bool settings_disableFPSLimitOnNetworkMessages;

static const int NUM_SETTINGS_TABS = 7;

static const int SETTINGS_VIDEO_TAB = 0;
static const int SETTINGS_AUDIO_TAB = 1;
static const int SETTINGS_KEYBOARD_TAB = 2;
static const int SETTINGS_MOUSE_TAB = 3;
static const int SETTINGS_GAMEPAD_BINDINGS_TAB = 4;
static const int SETTINGS_GAMEPAD_SETTINGS_TAB = 5;
static const int SETTINGS_MISC_TAB = 6;


//Confirm resolution window stuff.
extern bool resolutionChanged;
extern bool confirmResolutionWindow;
extern int resolutionConfirmationTimer;
static const int RESOLUTION_CONFIRMATION_TIME = 10000; //Time in milliseconds before resolution reverts.
extern Sint32 oldXres;
extern Sint32 oldYres;
extern button_t* revertResolutionButton;

int getNumDisplays();
struct resolution {
	int x;
	int y;
	int hz;

	bool operator==(const resolution& rhs) const {
		return x == rhs.x && y == rhs.y && hz == rhs.hz;
	}
};
void getResolutionList(int device_id, std::list<resolution>&);
void applySettings();
void openConfirmResolutionWindow();
void buttonAcceptResolution(button_t* my);
void buttonRevertResolution(button_t* my);
void revertResolution();

class Stat;
int isCharacterValidFromDLC(Stat& myStats, int characterClass);
int isCharacterValidFromDLC(int player, int characterClass, int race, int appearance);

// handle intro stage stuff
void doQuitGame();
void doNewGame(bool makeHighscore);
void doCredits();
void doEndgame(bool saveHighscore, bool onServerDisconnect);
void doEndgameOnDisconnect();
void doIntro();
void doEndgameHerx();
void doEndgameDevil();
void doMidgame();
void doEndgameCitadel();
void doEndgameClassicAndExtraMidGame();
void doEndgameExpansion();

enum CharacterDLCValidation : int
{
	INVALID_CHARACTER,
	VALID_OK_CHARACTER,
	INVALID_REQUIREDLC1,
	INVALID_REQUIREDLC2,
	INVALID_REQUIRE_ACHIEVEMENT
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
extern LastCreatedCharacter LastCreatedCharacterSettings;

bool isAchievementUnlockedForClassUnlock(int race);
