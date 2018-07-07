/*-------------------------------------------------------------------------------

	BARONY
	File: menu.hpp
	Desc: definitions and prototypes for menu.c

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once

extern bool savethisgame;

// main menu code
void handleMainMenu(bool mode);

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
extern real_t drunkextend;
extern bool losingConnection[4];
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
void buttonReplayLastCharacter(button_t* my);
void buttonDeleteScoreWindow(button_t* my);
void buttonOpenScoresWindow(button_t* my);
void buttonRandomName(button_t* my);
void buttonGamemodsOpenDirectory(button_t* my);
void buttonGamemodsPrevDirectory(button_t* my);
void buttonGamemodsBaseDirectory(button_t* my);
void buttonGamemodsSelectDirectoryForUpload(button_t* my);
void buttonGamemodsOpenModifyExistingWindow(button_t* my);
void buttonGamemodsStartModdedGame(button_t* my);

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
void buttonInviteFriends(button_t* my);
void buttonSteamLobbyBrowserJoinGame(button_t* my);
void buttonSteamLobbyBrowserRefresh(button_t* my);
void buttonGamemodsSubscribeToHostsModFiles(button_t* my);
void buttonGamemodsMountHostsModFiles(button_t* my);
#endif

#define SLIDERFONT font12x12_bmp

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

// gamemods window stuff
extern int gamemods_window;
extern int gamemods_window_scroll;
extern int gamemods_window_fileSelect;
extern int gamemods_uploadStatus;
extern int gamemods_numCurrentModsLoaded;
extern std::list<std::string> currentDirectoryFiles;
extern std::string directoryPath;
void gamemodsWindowClearVariables();
void gamemodsCustomContentInit();
bool gamemodsDrawClickableButton(int padx, int pady, int padw, int padh, Uint32 btnColor, std::string btnText, int action);
bool gamemodsRemovePathFromMountedFiles(std::string findStr);
bool gamemodsIsPathInMountedFiles(std::string findStr);
bool gamemodsClearAllMountedPaths();
bool gamemodsMountAllExistingPaths();
extern bool gamemods_disableSteamAchievements;
extern std::vector<std::pair<std::string, std::string>> gamemods_mountedFilepaths;
extern bool gamemods_modelsListRequiresReload;
extern bool gamemods_soundListRequiresReload;
#ifdef STEAMWORKS
void gamemodsWindowUploadInit(bool creatingNewItem);
void gamemodsSubscribedItemsInit();
void gamemodsDrawWorkshopItemTagToggle(std::string tagname, int x, int y);
bool gamemodsCheckIfSubscribedAndDownloadedFileID(uint64 fileID);
bool gamemodsCheckFileIDInLoadedPaths(uint64 fileID);
bool gamemodsIsClientLoadOrderMatchingHost(std::vector<std::string> serverModList);
extern std::vector<std::pair<std::string, uint64>> gamemods_workshopLoadedFileIDMap;
#endif // STEAMWORKS

extern bool scoreDisplayMultiplayer;

extern Sint32 slidery, slidersize, oslidery;

// settings window
extern Uint32 settings_fov;
extern Uint32 settings_fps;
extern int settings_xres, settings_yres;
extern bool settings_smoothlighting;
extern int settings_fullscreen, settings_shaking, settings_bobbing;
extern real_t settings_gamma;
extern int settings_sfxvolume, settings_musvolume;
extern int settings_impulses[NUMIMPULSES];
extern int settings_reversemouse;
extern bool settings_smoothmouse;
extern real_t settings_mousespeed;
extern bool settings_broadcast;
extern bool settings_nohud;
extern bool settings_colorblind;
extern bool settings_spawn_blood;
extern bool settings_light_flicker;
extern bool settings_vsync;
extern bool settings_minimap_ping_mute;
extern int settings_minimap_transparency_foreground;
extern int settings_minimap_transparency_background;
extern int settings_minimap_scale;
extern int settings_minimap_object_zoom;
extern char portnumber_char[6];
extern char connectaddress[64];
extern int multiplayerselect;
extern bool smoothmouse;
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

void applySettings();
void openConfirmResolutionWindow();
void buttonAcceptResolution(button_t* my);
void buttonRevertResolution(button_t* my);
void revertResolution();
