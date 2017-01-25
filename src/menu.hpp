/*-------------------------------------------------------------------------------

	BARONY
	File: menu.hpp
	Desc: definitions and prototypes for menu.c

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once

#ifdef PANDORA
#define NUMRESOLUTIONS 2
#else
#define NUMRESOLUTIONS 10
#endif

extern int resolutions[NUMRESOLUTIONS][2];

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
extern DOUBLE drunkextend;
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
void buttonOpenCharacterCreationWindow(button_t* my);
void buttonLoadGame(button_t* my);
void buttonRandomCharacter(button_t* my);

#ifdef STEAMWORKS
void buttonInviteFriends(button_t* my);
void buttonSteamLobbyBrowserJoinGame(button_t* my);
void buttonSteamLobbyBrowserRefresh(button_t* my);
#endif

#define SLIDERFONT font12x12_bmp

// misc functions
void openSettingsWindow();
void openFailedConnectionWindow(int mode);
void openGameoverWindow();
void openSteamLobbyBrowserWindow(button_t* my);
void openLoadGameWindow(button_t* my);
void doSlider(int x, int y, int dots, int minvalue, int maxvalue, int increment, int* var, SDL_Surface* slider_font = SLIDERFONT, int slider_font_char_width = 16);
void doSliderF(int x, int y, int dots, DOUBLE minvalue, DOUBLE maxvalue, DOUBLE increment, DOUBLE* var);

// menu variables
extern bool settings_window;
extern int charcreation_step;
extern Uint32 charcreation_ticks;
extern bool playing_random_char;
extern int settings_tab;
extern int connect_window;
extern bool lobby_window;
extern int score_window;

extern Sint32 slidery, slidersize, oslidery;

// settings window
extern Uint32 settings_fov;
extern int settings_xres, settings_yres;
extern bool settings_smoothlighting;
extern int settings_fullscreen, settings_shaking, settings_bobbing;
extern DOUBLE settings_gamma;
extern int settings_sfxvolume, settings_musvolume;
extern int settings_impulses[NUMIMPULSES];
extern int settings_reversemouse;
extern bool settings_smoothmouse;
extern DOUBLE settings_mousespeed;
extern bool settings_broadcast;
extern bool settings_nohud;
extern bool settings_colorblind;
extern bool settings_spawn_blood;
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
extern bool settings_disable_messages;
extern bool settings_right_click_protect;
extern bool settings_auto_appraise_new_items;

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
