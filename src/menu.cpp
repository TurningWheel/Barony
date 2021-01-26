/*-------------------------------------------------------------------------------

	BARONY
	File: menu.cpp
	Desc: contains code for all menu buttons in the game

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include <list>
#include "main.hpp"
#include "draw.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "messages.hpp"
#include "entity.hpp"
#include "files.hpp"
#include "menu.hpp"
#include "classdescriptions.hpp"
#include "interface/interface.hpp"
#include "magic/magic.hpp"
#include "sound.hpp"
#include "items.hpp"
#include "init.hpp"
#include "shops.hpp"
#include "monster.hpp"
#include "scores.hpp"
#include "menu.hpp"
#include "net.hpp"
#ifdef STEAMWORKS
#include <steam/steam_api.h>
#include "steam.hpp"
#endif
#include "prng.hpp"
#include "credits.hpp"
#include "paths.hpp"
#include "collision.hpp"
#include "player.hpp"
#include "cppfuncs.hpp"
#include "colors.hpp"
#include <ctime>
#include "sys/stat.h"
#include "eos.hpp"
#include "mod_tools.hpp"
#include "interface/ui.hpp"
#include "lobbies.hpp"
#include <sstream>

#ifdef STEAMWORKS
//Helper func. //TODO: Bugger.
void* cpp_SteamMatchmaking_GetLobbyOwner(void* steamIDLobby)
{
	CSteamID* id = new CSteamID();
	*id = SteamMatchmaking()->GetLobbyOwner(*static_cast<CSteamID*>(steamIDLobby));
	return id; //Still don't like this method.
}
// get player names in a lobby
void* cpp_SteamMatchmaking_GetLobbyMember(void* steamIDLobby, int index)
{
	CSteamID* id = new CSteamID();
	*id = SteamMatchmaking()->GetLobbyMemberByIndex(*static_cast<CSteamID*>(currentLobby), index);
	return id;
}
uint64 SteamAPICall_NumPlayersOnline = 0;
NumberOfCurrentPlayers_t NumberOfCurrentPlayers;
int steamOnlinePlayers = 0;
#endif

// menu variables
bool lobby_window = false;
bool settings_window = false;
int connect_window = 0;
int charcreation_step = 0;
int loadGameSaveShowRectangle = 0; // stores the current amount of savegames available, to use when drawing load game window boxes.
int singleplayerSavegameFreeSlot = -1; // used on multiplayer/single player select window to store if savefile exists. 
int multiplayerSavegameFreeSlot = -1; // used on multiplayer/single player select window to store if savefile exists.
int raceSelect = 0;
Uint32 lobbyWindowSvFlags = 0;
/*
 * settings_tab
 * valid values:
 *		- 0 = Video settings
 *		- 1 = Audio settings
 *		- 2 = Keyboard/binding settings
 *		- 3 = Mouse settings
 *		- 4 = Gamepad bindings
 *		- 5 = Gamepad settings
 *		- 6 = Misc settings
 */
int settings_tab = 0;
button_t* button_video_tab = nullptr;
button_t* button_audio_tab = nullptr;
button_t* button_keyboard_tab = nullptr;
button_t* button_mouse_tab = nullptr;
button_t* button_gamepad_bindings_tab = nullptr;
button_t* button_gamepad_settings_tab = nullptr;
button_t* button_misc_tab = nullptr;

int score_window = 0;
int score_window_to_delete = 0;
bool score_window_delete_multiplayer = false;
int score_leaderboard_window = 0;

int savegames_window = 0;
int savegames_window_scroll = 0;
std::vector<std::tuple<int, int, int, std::string>> savegamesList; // tuple - last modified, multiplayer type, file entry, and description of save game.

// gamemods window stuff.
int gamemods_window = 0;
std::list<std::string> currentDirectoryFiles;
std::list<std::string> directoryFilesListToUpload;
std::string directoryToUpload;
std::string directoryPath;
int gamemods_window_scroll = 0;
int gamemods_window_fileSelect = 0;
int gamemods_uploadStatus = 0;
char gamemods_uploadTitle[32] = "Title";
char gamemods_uploadDescription[32] = "Description";
int gamemods_currentEditField = 0;
bool gamemods_workshopSetPropertyReturn[3] = { false, false, false };
int gamemods_subscribedItemsStatus = 0;
char gamemods_newBlankDirectory[32] = "";
char gamemods_newBlankDirectoryOldName[32] = "";
int gamemods_newBlankDirectoryStatus = 0;
int gamemods_numCurrentModsLoaded = -1;
const int gamemods_maxTags = 10;
std::vector<std::pair<std::string, std::string>> gamemods_mountedFilepaths;
std::list<std::string> gamemods_localModFoldernames;
bool gamemods_modelsListRequiresReload = false;
bool gamemods_modelsListLastStartedUnmodded = false; // if starting regular game that had to reset model list, use this to reinit custom models.
bool gamemods_soundListRequiresReload = false;
bool gamemods_soundsListLastStartedUnmodded = false; // if starting regular game that had to reset sounds list, use this to reinit custom sounds.
bool gamemods_tileListRequireReloadUnmodded = false;
bool gamemods_spriteImagesRequireReloadUnmodded = false;
bool gamemods_booksRequireReloadUnmodded = false;
bool gamemods_musicRequireReloadUnmodded = false;
bool gamemods_langRequireReloadUnmodded = false;
bool gamemods_itemSpritesRequireReloadUnmodded = false;
bool gamemods_itemsTxtRequireReloadUnmodded = false;
bool gamemods_itemsGlobalTxtRequireReloadUnmodded = false;
bool gamemods_monsterLimbsRequireReloadUnmodded = false;
bool gamemods_systemImagesReloadUnmodded = false;
bool gamemods_customContentLoadedFirstTime = false;
bool gamemods_disableSteamAchievements = false;
bool gamemods_modPreload = false;

sex_t lastSex = MALE;
PlayerRaces lastRace = RACE_HUMAN;
int lastAppearance = 0;
bool enabledDLCPack1 = false;
bool enabledDLCPack2 = false;
bool showRaceInfo = false;
#ifdef STEAMWORKS
std::vector<SteamUGCDetails_t *> workshopSubscribedItemList;
std::vector<std::pair<std::string, uint64>> gamemods_workshopLoadedFileIDMap;
#else
bool serialEnterWindow = false;
char serialInputText[64] = "";
int serialVerifyWindow = 0;
#endif // STEAMWORKS


bool scoreDisplayMultiplayer = false;
int settings_xres, settings_yres;

typedef std::tuple<int, int> resolution;
std::list<resolution> resolutions;
Uint32 settings_fov;
Uint32 settings_fps;
bool settings_smoothlighting;
int settings_fullscreen, settings_shaking, settings_bobbing;
bool settings_borderless = false;
real_t settings_gamma;
int settings_sfxvolume, settings_musvolume;
int settings_sfxAmbientVolume;
int settings_sfxEnvironmentVolume;
int settings_impulses[NUMIMPULSES];
int settings_joyimpulses[NUM_JOY_IMPULSES];
int settings_reversemouse;
real_t settings_mousespeed;
bool settings_broadcast;
bool settings_nohud;
bool settings_colorblind;
bool settings_spawn_blood;
bool settings_light_flicker;
bool settings_vsync;
bool settings_status_effect_icons = true;
bool settings_minimap_ping_mute = false;
bool settings_mute_audio_on_focus_lost = false;
bool settings_mute_player_monster_sounds = false;
int settings_minimap_transparency_foreground = 0;
int settings_minimap_transparency_background = 0;
int settings_minimap_scale = 4;
int settings_minimap_object_zoom = 0;
char portnumber_char[6];
char connectaddress[64];
char classtoquickstart[256] = "";
bool spawn_blood = true;
int multiplayerselect = SINGLE;
int menuselect = 0;
bool settings_auto_hotbar_new_items = true;
bool settings_auto_hotbar_categories[NUM_HOTBAR_CATEGORIES] = { true, true, true, true,
																true, true, true, true,
																true, true, true, true };
int settings_autosort_inventory_categories[NUM_AUTOSORT_CATEGORIES] = {	0, 0, 0, 0,
																		0, 0, 0, 0,
																		0, 0, 0, 0 };
bool settings_hotbar_numkey_quick_add = false;
bool settings_disable_messages = true;
bool settings_right_click_protect = false;
bool settings_auto_appraise_new_items = true;
bool playing_random_char = false;
bool colorblind = false;
bool settings_lock_right_sidebar = false;
bool settings_show_game_timer_always = false;
bool settings_uiscale_charactersheet = false;
bool settings_uiscale_skillspage = false;
real_t settings_uiscale_hotbar = 1.f;
real_t settings_uiscale_playerbars = 1.f;
real_t settings_uiscale_chatlog = 1.f;
real_t settings_uiscale_inventory = 1.f;
bool settings_hide_statusbar = false;
bool settings_hide_playertags = false;
bool settings_show_skill_values = false;
bool settings_disableMultithreadedSteamNetworking = true;
bool settings_disableFPSLimitOnNetworkMessages = false;
Sint32 oslidery = 0;

//Gamepad settings.
bool settings_gamepad_leftx_invert = false;
bool settings_gamepad_lefty_invert = false;
bool settings_gamepad_rightx_invert = false;
bool settings_gamepad_righty_invert = false;
bool settings_gamepad_menux_invert = false;
bool settings_gamepad_menuy_invert = false;

int settings_gamepad_deadzone = 1;
int settings_gamepad_rightx_sensitivity = 1;
int settings_gamepad_righty_sensitivity = 1;
int settings_gamepad_menux_sensitivity = 1;
int settings_gamepad_menuy_sensitivity = 1;

Uint32 colorWhite = 0xFFFFFFFF;

int firstendmoviealpha[30];
int secondendmoviealpha[30];
int thirdendmoviealpha[30];
int fourthendmoviealpha[30];
int intromoviealpha[30];
int rebindkey = -1;
int rebindaction = -1;

Sint32 gearrot = 0;
Sint32 gearsize = 5000;
Uint16 logoalpha = 0;
int credittime = 0;
int creditstage = 0;
int intromovietime = 0;
int intromoviestage = 0;
int firstendmovietime = 0;
int firstendmoviestage = 0;
int secondendmovietime = 0;
int secondendmoviestage = 0;
int thirdendmoviestage = 0;
int thirdendmovietime = 0;
int thirdEndNumLines = 6;
int fourthendmoviestage = 0;
int fourthendmovietime = 0;
int fourthEndNumLines = 13;
real_t drunkextend = 0;
bool losingConnection[MAXPLAYERS] = { false };
bool subtitleVisible = false;
int subtitleCurrent = 0;

// new text crawls...
int DLCendmoviealpha[8][30] = { 0 };
int DLCendmovieStageAndTime[8][2] = { 0 };

int DLCendmovieNumLines[8] = 
{
	7,	// MOVIE_MIDGAME_HERX_MONSTERS,
	8,	// MOVIE_MIDGAME_BAPHOMET_MONSTERS,
	8,	// MOVIE_MIDGAME_BAPHOMET_HUMAN_AUTOMATON,
	5,	// MOVIE_CLASSIC_WIN_MONSTERS,
	7,	// MOVIE_CLASSIC_WIN_BAPHOMET_MONSTERS,
	13,	// MOVIE_WIN_AUTOMATON,
	13,	// MOVIE_WIN_DEMONS_UNDEAD,
	13	// MOVIE_WIN_BEASTS
};
char epilogueHostName[128] = "";
int epilogueHostRace = 0;
int epilogueMultiplayerType = SINGLE;

//Confirm resolution window stuff.
bool resolutionChanged = false;
bool confirmResolutionWindow = false;
int resolutionConfirmationTimer = 0;
Sint32 oldXres;
Sint32 oldYres;
Sint32 oldFullscreen;
bool oldBorderless = false;
real_t oldGamma;
bool oldVerticalSync = false;
button_t* revertResolutionButton = nullptr;

void buttonCloseSettingsSubwindow(button_t* my);

button_t* getSettingsTabButton()
{
	switch ( settings_tab )
	{
		case SETTINGS_VIDEO_TAB:
			return button_video_tab;
		case SETTINGS_AUDIO_TAB:
			return button_audio_tab;
		case SETTINGS_KEYBOARD_TAB:
			return button_keyboard_tab;
		case SETTINGS_MOUSE_TAB:
			return button_mouse_tab;
		case SETTINGS_GAMEPAD_BINDINGS_TAB:
			return button_gamepad_bindings_tab;
		case SETTINGS_GAMEPAD_SETTINGS_TAB:
			return button_gamepad_settings_tab;
		case SETTINGS_MISC_TAB:
			return button_misc_tab;
	}

	return nullptr;
}

void changeSettingsTab(int option)
{
	if ( getSettingsTabButton() )
	{
		getSettingsTabButton()->outline = false;
	}

	settings_tab = option;

	if ( settings_tab < 0 )
	{
		settings_tab = NUM_SETTINGS_TABS - 1;
	}
	if ( settings_tab >= NUM_SETTINGS_TABS )
	{
		settings_tab = 0;
	}

	if ( getSettingsTabButton() )
	{
		button_t* button = getSettingsTabButton();
		button->outline = true;
		int x = button->x + (button->sizex / 2);
		int y = button->y + (button->sizey / 2);
	}
}

std::vector<std::pair<std::string, int>> menuOptions;
void initMenuOptions()
{
	menuOptions.clear();
#if (defined USE_EOS && !defined STEAMWORKS)
	menuOptions.push_back(std::make_pair(language[1303], 1)); // start game
	menuOptions.push_back(std::make_pair(language[1304], 2)); // intro
	menuOptions.push_back(std::make_pair(language[3964], 3)); // hall of trials
	menuOptions.push_back(std::make_pair(language[1305], 4)); // statistics
	menuOptions.push_back(std::make_pair(language[3971], 5)); // achievements
	menuOptions.push_back(std::make_pair(language[1306], 6)); // settings
	menuOptions.push_back(std::make_pair(language[1307], 7)); // credits
	menuOptions.push_back(std::make_pair(language[2978], 8)); // custom content
	menuOptions.push_back(std::make_pair(language[1308], 9)); // quit
#else
	menuOptions.push_back(std::make_pair(language[1303], 1)); // start game
	menuOptions.push_back(std::make_pair(language[1304], 2)); // intro
	menuOptions.push_back(std::make_pair(language[3964], 3)); // hall of trials
	menuOptions.push_back(std::make_pair(language[1305], 4)); // statistics
	menuOptions.push_back(std::make_pair(language[1306], 5)); // settings
	menuOptions.push_back(std::make_pair(language[1307], 6)); // credits
	menuOptions.push_back(std::make_pair(language[2978], 7)); // custom content
#ifdef STEAMWORKS
	menuOptions.push_back(std::make_pair(language[2979], 8)); // workshop
	menuOptions.push_back(std::make_pair(language[1308], 9)); // quit
#else
	menuOptions.push_back(std::make_pair(language[1308], 8)); // quit
#endif
#endif
}

void getPrevMenuOption(int& currentMenuOption)
{
	for ( auto it = menuOptions.begin(); it != menuOptions.end(); )
	{
		if ( (*it).second == currentMenuOption )
		{
			if ( it == menuOptions.begin() )
			{
				currentMenuOption = menuOptions.back().second;
				return;
			}
			--it;
			currentMenuOption = (*it).second;
			return;
		}
		++it;
	}

	currentMenuOption = menuOptions.at(0).second; // default value.
}

void getNextMenuOption(int& currentMenuOption)
{
	for ( auto it = menuOptions.begin(); it != menuOptions.end(); )
	{
		if ( (*it).second == currentMenuOption )
		{
			++it;
			if ( it == menuOptions.end() )
			{
				currentMenuOption = menuOptions.at(0).second;
				return;
			}
			currentMenuOption = (*it).second;
			return;
		}
		++it;
	}

	currentMenuOption = menuOptions.at(0).second; // default value.
}

void navigateMainMenuItems(bool mode)
{
	if ( menuOptions.empty() )
	{
		return;
	}

	int numInGameMenuOptions = 4 + (multiplayer != CLIENT);
	if ( !mode && gameModeManager.getMode() == GameModeManager_t::GAME_MODE_TUTORIAL )
	{
		if ( !strcmp(map.name, "Tutorial Hub") )
		{
			numInGameMenuOptions = 4;
		}
	}

#if defined USE_EOS && !defined STEAMWORKS
	numInGameMenuOptions += 1;
#endif

	int warpx, warpy;
	if (menuselect == 0)
	{
		//No menu item selected.
		if ( keystatus[SDL_SCANCODE_UP] || (inputs.bControllerInputPressed(clientnum, INJOY_DPAD_UP) && rebindaction == -1) )
		{
			keystatus[SDL_SCANCODE_UP] = 0;
			if ( rebindaction == -1 )
			{
				inputs.controllerClearInput(clientnum, INJOY_DPAD_UP);
			}
			inputs.hideMouseCursors();
			menuselect = menuOptions.at(0).second;
			//Warp cursor to menu item, for gamepad convenience.
			warpx = 50 + 18;
			warpy = (yres / 4) + 80 + (18 / 2); //I am a wizard. I hate magic numbers.
			//SDL_WarpMouseInWindow(screen, warpx, warpy);
			Uint32 flags = (Inputs::SET_MOUSE | Inputs::SET_CONTROLLER);
			inputs.warpMouse(clientnum, warpx, warpy, flags);
		}
		else if ( keystatus[SDL_SCANCODE_DOWN] || (inputs.bControllerInputPressed(clientnum, INJOY_DPAD_DOWN) && rebindaction == -1) )
		{
			keystatus[SDL_SCANCODE_DOWN] = 0;
			if ( rebindaction == -1 )
			{
				inputs.controllerClearInput(clientnum, INJOY_DPAD_DOWN);
			}
			inputs.hideMouseCursors();
			menuselect = menuOptions.at(0).second;
			warpx = 50 + 18;
			warpy = (yres / 4) + 80 + (18 / 2);
			//SDL_WarpMouseInWindow(screen, warpx, warpy);
			Uint32 flags = (Inputs::SET_MOUSE | Inputs::SET_CONTROLLER);
			inputs.warpMouse(clientnum, warpx, warpy, flags);
		}
	}
	else
	{
		if ( keystatus[SDL_SCANCODE_UP] || (inputs.bControllerInputPressed(clientnum, INJOY_DPAD_UP) && rebindaction == -1) )
		{
			keystatus[SDL_SCANCODE_UP] = 0;
			if ( rebindaction == -1 )
			{
				inputs.controllerClearInput(clientnum, INJOY_DPAD_UP);
			}
			inputs.hideMouseCursors();
			if ( mode )
			{
				getPrevMenuOption(menuselect);
			}
			else
			{
				menuselect--;
				if ( menuselect == 0 )
				{
					menuselect = numInGameMenuOptions;
				}
			}

			warpx = 50 + 18;
			warpy = (((yres / 4) + 80 + (18 / 2)) + ((menuselect - 1) * 24));
			//SDL_WarpMouseInWindow(screen, warpx, warpy);
			Uint32 flags = (Inputs::SET_MOUSE | Inputs::SET_CONTROLLER);
			inputs.warpMouse(clientnum, warpx, warpy, flags);
		}
		else if (keystatus[SDL_SCANCODE_DOWN] || (inputs.bControllerInputPressed(clientnum, INJOY_DPAD_DOWN) && rebindaction == -1) )
		{
			keystatus[SDL_SCANCODE_DOWN] = 0;
			if ( rebindaction == -1 )
			{
				inputs.controllerClearInput(clientnum, INJOY_DPAD_DOWN);
			}
			inputs.hideMouseCursors();
			if ( mode )
			{
				getNextMenuOption(menuselect);
			}
			else
			{
				menuselect++;
				if ( menuselect > numInGameMenuOptions )
				{
					menuselect = 1;
				}
			}
			warpx = 50 + 18;
			warpy = (((yres / 4) + 80 + (18 / 2)) + ((menuselect - 1) * 24));
			//SDL_WarpMouseInWindow(screen, warpx, warpy);
			Uint32 flags = (Inputs::SET_MOUSE | Inputs::SET_CONTROLLER);
			inputs.warpMouse(clientnum, warpx, warpy, flags);
		}
	}
}

void inline printJoybindingNames(const SDL_Rect& currentPos, int c, bool &rebindingaction)
{
	Sint32 omousex = inputs.getMouse(clientnum, Inputs::MouseInputs::OX);
	Sint32 omousey = inputs.getMouse(clientnum, Inputs::MouseInputs::OY);
	ttfPrintText(ttf8, currentPos.x, currentPos.y, language[1948 + c]);
	if ( inputs.bMouseLeft(clientnum) && !rebindingaction )
	{
		if ( omousex >= currentPos.x && omousex < subx2 - 24 )
		{
			if ( omousey >= currentPos.y && omousey < currentPos.y + 12 )
			{
				inputs.mouseClearLeft(clientnum);
				if ( settings_joyimpulses[c] != UNBOUND_JOYBINDING )
				{
					settings_joyimpulses[c] = UNBOUND_JOYBINDING; //Unbind the joybinding if clicked on.
				}
				else
				{
					lastkeypressed = 0;
					rebindingaction = true;
					rebindaction = c;
				}
			}
		}
	}

	if ( c != rebindaction )
	{
		if ( !strcmp(getInputName(settings_joyimpulses[c]), "Unassigned key" ))
		{
			ttfPrintTextColor(ttf8, currentPos.x + 232, currentPos.y, uint32ColorBaronyBlue(*mainsurface), true, getInputName(settings_joyimpulses[c]));
		}
		else if ( !strcmp(getInputName(settings_joyimpulses[c]), "Unknown key") || !strcmp(getInputName(settings_joyimpulses[c]), "Unknown trigger") )
		{
			ttfPrintTextColor(ttf8, currentPos.x + 232, currentPos.y, uint32ColorRed(*mainsurface), true, getInputName(settings_joyimpulses[c]));
		}
		else
		{
			ttfPrintText(ttf8, currentPos.x + 232, currentPos.y, getInputName(settings_joyimpulses[c]));
		}
	}
	else
	{
		ttfPrintTextColor(ttf8, currentPos.x + 232, currentPos.y, uint32ColorGreen(*mainsurface), true, "...");
	}
}

enum CharacterDLCValidation : int
{
	INVALID_CHARACTER,
	VALID_OK_CHARACTER,
	INVALID_REQUIREDLC1,
	INVALID_REQUIREDLC2,
	INVALID_REQUIRE_ACHIEVEMENT
};

bool isAchievementUnlockedForClassUnlock(PlayerRaces race)
{
#ifdef STEAMWORKS
	bool unlocked = false;
	if ( enabledDLCPack1 && race == RACE_SKELETON && SteamUserStats()->GetAchievement("BARONY_ACH_BONY_BARON", &unlocked) )
	{
		return unlocked;
	}
	else if ( enabledDLCPack1 && race == RACE_VAMPIRE && SteamUserStats()->GetAchievement("BARONY_ACH_BUCKTOOTH_BARON", &unlocked) )
	{
		return unlocked;
	}
	else if ( enabledDLCPack1 && race == RACE_SUCCUBUS && SteamUserStats()->GetAchievement("BARONY_ACH_BOMBSHELL_BARON", &unlocked) )
	{
		return unlocked;
	}
	else if ( enabledDLCPack1 && race == RACE_GOATMAN && SteamUserStats()->GetAchievement("BARONY_ACH_BLEATING_BARON", &unlocked) )
	{
		return unlocked;
	}
	else if ( enabledDLCPack2 && race == RACE_AUTOMATON && SteamUserStats()->GetAchievement("BARONY_ACH_BOILERPLATE_BARON", &unlocked) )
	{
		return unlocked;
	}
	else if ( enabledDLCPack2 && race == RACE_INCUBUS && SteamUserStats()->GetAchievement("BARONY_ACH_BAD_BOY_BARON", &unlocked) )
	{
		return unlocked;
	}
	else if ( enabledDLCPack2 && race == RACE_GOBLIN && SteamUserStats()->GetAchievement("BARONY_ACH_BAYOU_BARON", &unlocked) )
	{
		return unlocked;
	}
	else if ( enabledDLCPack2 && race == RACE_INSECTOID && SteamUserStats()->GetAchievement("BARONY_ACH_BUGGAR_BARON", &unlocked) )
	{
		return unlocked;
	}
#elif defined USE_EOS
	if ( enabledDLCPack1 && race == RACE_SKELETON && achievementUnlocked("BARONY_ACH_BONY_BARON") )
	{
		return true;
	}
	else if ( enabledDLCPack1 && race == RACE_VAMPIRE && achievementUnlocked("BARONY_ACH_BUCKTOOTH_BARON") )
	{
		return true;
	}
	else if ( enabledDLCPack1 && race == RACE_SUCCUBUS && achievementUnlocked("BARONY_ACH_BOMBSHELL_BARON") )
	{
		return true;
	}
	else if ( enabledDLCPack1 && race == RACE_GOATMAN && achievementUnlocked("BARONY_ACH_BLEATING_BARON") )
	{
		return true;
	}
	else if ( enabledDLCPack2 && race == RACE_AUTOMATON && achievementUnlocked("BARONY_ACH_BOILERPLATE_BARON") )
	{
		return true;
	}
	else if ( enabledDLCPack2 && race == RACE_INCUBUS && achievementUnlocked("BARONY_ACH_BAD_BOY_BARON") )
	{
		return true;
	}
	else if ( enabledDLCPack2 && race == RACE_GOBLIN && achievementUnlocked("BARONY_ACH_BAYOU_BARON") )
	{
		return true;
	}
	else if ( enabledDLCPack2 && race == RACE_INSECTOID && achievementUnlocked("BARONY_ACH_BUGGAR_BARON") )
	{
		return true;
	}
#else
	return false;
#endif // STEAMWORKS
	return false;
}

int isCharacterValidFromDLC(Stat& myStats, int characterClass)
{
	switch ( characterClass )
	{
		case CLASS_CONJURER:
		case CLASS_ACCURSED:
		case CLASS_MESMER:
		case CLASS_BREWER:
			if ( !enabledDLCPack1 )
			{
				return INVALID_REQUIREDLC1;
			}
			break;
		case CLASS_MACHINIST:
		case CLASS_PUNISHER:
		case CLASS_SHAMAN:
		case CLASS_HUNTER:
			if ( !enabledDLCPack2 )
			{
				return INVALID_REQUIREDLC2;
			}
			break;
		default:
			break;
	}

	switch ( myStats.playerRace )
	{
		case RACE_SKELETON:
		case RACE_VAMPIRE:
		case RACE_SUCCUBUS:
		case RACE_GOATMAN:
			if ( !enabledDLCPack1 )
			{
				return INVALID_REQUIREDLC1;
			}
			break;
		case RACE_AUTOMATON:
		case RACE_INCUBUS:
		case RACE_GOBLIN:
		case RACE_INSECTOID:
			if ( !enabledDLCPack2 )
			{
				return INVALID_REQUIREDLC2;
			}
			break;
		default:
			break;
	}

	if ( myStats.playerRace == RACE_HUMAN )
	{
		return VALID_OK_CHARACTER;
	}
	else if ( myStats.playerRace > RACE_HUMAN && myStats.appearance == 1 )
	{
		return VALID_OK_CHARACTER; // aesthetic only option.
	}
	if ( characterClass <= CLASS_MONK )
	{
		return VALID_OK_CHARACTER;
	}

	switch ( characterClass )
	{
		case CLASS_CONJURER:
			if ( myStats.playerRace == RACE_SKELETON )
			{
				return VALID_OK_CHARACTER;
			}
			return isAchievementUnlockedForClassUnlock(RACE_SKELETON) ? VALID_OK_CHARACTER : INVALID_REQUIRE_ACHIEVEMENT;
			break;
		case CLASS_ACCURSED:
			if ( myStats.playerRace == RACE_VAMPIRE )
			{
				return VALID_OK_CHARACTER;
			}
			return isAchievementUnlockedForClassUnlock(RACE_VAMPIRE) ? VALID_OK_CHARACTER : INVALID_REQUIRE_ACHIEVEMENT;
			break;
		case CLASS_MESMER:
			if ( myStats.playerRace == RACE_SUCCUBUS )
			{
				return VALID_OK_CHARACTER;
			}
			return isAchievementUnlockedForClassUnlock(RACE_SUCCUBUS) ? VALID_OK_CHARACTER : INVALID_REQUIRE_ACHIEVEMENT;
			break;
		case CLASS_BREWER:
			if ( myStats.playerRace == RACE_GOATMAN )
			{
				return VALID_OK_CHARACTER;
			}
			return isAchievementUnlockedForClassUnlock(RACE_GOATMAN) ? VALID_OK_CHARACTER : INVALID_REQUIRE_ACHIEVEMENT;
			break;
		case CLASS_MACHINIST:
			if ( myStats.playerRace == RACE_AUTOMATON )
			{
				return VALID_OK_CHARACTER;
			}
			return isAchievementUnlockedForClassUnlock(RACE_AUTOMATON) ? VALID_OK_CHARACTER : INVALID_REQUIRE_ACHIEVEMENT;
			break;
		case CLASS_PUNISHER:
			if ( myStats.playerRace == RACE_INCUBUS )
			{
				return VALID_OK_CHARACTER;
			}
			return isAchievementUnlockedForClassUnlock(RACE_INCUBUS) ? VALID_OK_CHARACTER : INVALID_REQUIRE_ACHIEVEMENT;
			break;
		case CLASS_SHAMAN:
			if ( myStats.playerRace == RACE_GOBLIN )
			{
				return VALID_OK_CHARACTER;
			}
			return isAchievementUnlockedForClassUnlock(RACE_GOBLIN) ? VALID_OK_CHARACTER : INVALID_REQUIRE_ACHIEVEMENT;
			break;
		case CLASS_HUNTER:
			if ( myStats.playerRace == RACE_INSECTOID )
			{
				return VALID_OK_CHARACTER;
			}
			return isAchievementUnlockedForClassUnlock(RACE_INSECTOID) ? VALID_OK_CHARACTER : INVALID_REQUIRE_ACHIEVEMENT;
			break;
		default:
			break;
	}

	return INVALID_CHARACTER;
}

void inline pauseMenuOnInputPressed()
{
	inputs.mouseClearLeft(clientnum);
	keystatus[SDL_SCANCODE_RETURN] = 0;
	playSound(139, 64);
	if ( rebindaction == -1 )
	{
		inputs.controllerClearInput(clientnum, INJOY_MENU_NEXT);
	}
}

void handleInGamePauseMenu()
{
	Uint32 colorGray = uint32ColorGray(*mainsurface);

	Sint32 mousex = inputs.getMouse(clientnum, Inputs::MouseInputs::X);
	Sint32 mousey = inputs.getMouse(clientnum, Inputs::MouseInputs::Y);
	Sint32 omousex = inputs.getMouse(clientnum, Inputs::MouseInputs::OX);
	Sint32 omousey = inputs.getMouse(clientnum, Inputs::MouseInputs::OY);
	const bool inputIsPressed = (inputs.bMouseLeft(clientnum) || keystatus[SDL_SCANCODE_RETURN] || (inputs.bControllerInputPressed(clientnum, INJOY_MENU_NEXT) && rebindaction == -1));
	SDL_Rect text;
	text.x = 50;
	text.h = 18;
	text.w = 18;

	int numOption = 1;

	text.y = yres / 4 + 80;
	if ( ((omousex >= text.x && omousex < text.x + strlen(language[1309]) * text.w && omousey >= text.y && omousey < text.y + text.h) || (menuselect == numOption)) && subwindow == 0 && introstage == 1 )
	{
		// resume game
		menuselect = 1;
		ttfPrintTextFormattedColor(ttf16, text.x, text.y, colorGray, language[1309]);
		if ( inputIsPressed )
		{
			pauseMenuOnInputPressed();
			pauseGame(1, MAXPLAYERS);
		}
	}
	else
	{
		ttfPrintText(ttf16, text.x, text.y, language[1309]);
	}

	bool achievementsMenu = false;
#if !defined STEAMWORKS
#ifdef USE_EOS
	achievementsMenu = true;
#endif
#endif
	text.y += 24;
	++numOption;
	if ( achievementsMenu )
	{
		if ( ((omousex >= text.x && omousex < text.x + strlen(language[3971]) * text.w && omousey >= text.y && omousey < text.y + text.h) || (menuselect == numOption)) && subwindow == 0 && introstage == 1 )
		{
			// settings menu
			menuselect = numOption;
			ttfPrintTextFormattedColor(ttf16, text.x, text.y, colorGray, language[3971]);
			if ( inputIsPressed )
			{
				pauseMenuOnInputPressed();
				openAchievementsWindow();
			}
		}
		else
		{
			ttfPrintText(ttf16, text.x, text.y, language[3971]);
		}
		text.y += 24;
		++numOption;
	}

	if ( ((omousex >= text.x && omousex < text.x + strlen(language[1306]) * text.w && omousey >= text.y && omousey < text.y + text.h) || (menuselect == numOption)) && subwindow == 0 && introstage == 1 )
	{
		// settings menu
		menuselect = numOption;
		ttfPrintTextFormattedColor(ttf16, text.x, text.y, colorGray, language[1306]);
		if ( inputIsPressed )
		{
			pauseMenuOnInputPressed();
			openSettingsWindow();
		}
	}
	else
	{
		ttfPrintText(ttf16, text.x, text.y, language[1306]);
	}
	char* endgameText = NULL;
	char* quitgameText = language[1313];
	bool singleplayerAliveEndGameAndSave = false;
	bool singleplayerBossLevelDisableSaveOnExit = false;
	bool multiplayerAliveEndGameAndSave = false;
	if ( multiplayer == SINGLE )
	{
		if ( stats[clientnum] && stats[clientnum]->HP > 0 )
		{
			endgameText = language[3919];
			singleplayerAliveEndGameAndSave = true;
			if ( !strncmp(map.name, "Boss", 4)
				|| !strncmp(map.name, "Hell Boss", 9)
				|| !strncmp(map.name, "Sanctum", 7) )
			{
				// boss floor, no save scumming easily!
				singleplayerAliveEndGameAndSave = false;
				singleplayerBossLevelDisableSaveOnExit = true;
				endgameText = language[1310];
				quitgameText = language[3987];
			}
		}
		else
		{
			endgameText = language[1310];
			quitgameText = language[3987];
			singleplayerAliveEndGameAndSave = false;
		}
	}
	else if ( multiplayer == SERVER )
	{
		endgameText = language[1310];
		quitgameText = language[3987];
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			if ( !client_disconnected[i] && stats[i] && stats[i]->HP > 0 )
			{
				multiplayerAliveEndGameAndSave = true;
				quitgameText = language[1313];
				endgameText = language[3019];
				break;
			}
		}
	}
	else
	{
		endgameText = language[3019];
	}

	text.y += 24;
	++numOption;
	if ( ((omousex >= text.x && omousex < text.x + strlen(endgameText) * text.w && omousey >= text.y && omousey < text.y + text.h) || (menuselect == numOption)) && subwindow == 0 && introstage == 1 )
	{
		// end game / return to main menu
		menuselect = numOption;
		ttfPrintTextFormattedColor(ttf16, text.x, text.y, colorGray, endgameText);
		if ( inputIsPressed )
		{
			pauseMenuOnInputPressed();

			// create confirmation window
			subwindow = 1;
			if ( multiplayer == SINGLE )
			{
				subx1 = xres / 2 - 144;
				subx2 = xres / 2 + 144;
				suby1 = yres / 2 - 64;
				suby2 = yres / 2 + 64;
				if ( singleplayerAliveEndGameAndSave )
				{
					strcpy(subtext, language[3920]);
					subx1 = xres / 2 - 188;
					subx2 = xres / 2 + 188;
					suby1 = yres / 2 - 64;
					suby2 = yres / 2 + 64;

					// add a cancel button
					button_t* button = newButton();
					strcpy(button->label, language[1316]);
					button->x = subx2 - strlen(language[1316]) * 12 - 16;
					button->y = suby2 - 28;
					button->sizex = strlen(language[1316]) * 12 + 8;
					button->sizey = 20;
					button->action = &buttonCloseSubwindow;
					button->visible = 1;
					button->focused = 1;
				}
				else
				{
					subx1 = xres / 2 - 188;
					subx2 = xres / 2 + 188;
					if ( singleplayerBossLevelDisableSaveOnExit )
					{
						strcpy(subtext, language[3990]);
					}
					else
					{
						strcpy(subtext, language[1129]);
					}
				}
			}
			else
			{
				subx1 = xres / 2 - 224;
				subx2 = xres / 2 + 224;
				if ( multiplayer == SERVER )
				{
					if ( multiplayerAliveEndGameAndSave )
					{
						suby1 = yres / 2 - 100;
						suby2 = yres / 2 + 100;
						strcpy(subtext, language[3021]);
					}
					else
					{
						suby1 = yres / 2 - 90;
						suby2 = yres / 2 + 90;
						strcpy(subtext, language[3986]);
					}
				}
				else if ( multiplayer == CLIENT )
				{
					suby1 = yres / 2 - 112;
					suby2 = yres / 2 + 112;
					strcpy(subtext, language[3020]);
				}
			}

			// close button
			button_t* button = newButton();
			strcpy(button->label, "x");
			button->x = subx2 - 20;
			button->y = suby1;
			button->sizex = 20;
			button->sizey = 20;
			button->action = &buttonCloseSubwindow;
			button->visible = 1;
			button->focused = 1;
			button->key = SDL_SCANCODE_ESCAPE;
			button->joykey = joyimpulses[INJOY_MENU_CANCEL];

			// yes button
			button = newButton();
			strcpy(button->label, language[1314]);
			button->x = subx1 + 8;
			button->y = suby2 - 28;
			button->sizex = strlen(language[1314]) * 12 + 8;
			button->sizey = 20;
			if ( multiplayer == SINGLE )
			{
				if ( singleplayerAliveEndGameAndSave )
				{
					button->action = &buttonCloseAndEndGameConfirm;
				}
				else
				{
					button->action = &buttonEndGameConfirm;
				}
			}
			else
			{
				button->action = &buttonCloseAndEndGameConfirm;
			}
			button->visible = 1;
			button->focused = 1;
			button->key = SDL_SCANCODE_RETURN;
			button->joykey = joyimpulses[INJOY_MENU_NEXT];

			if ( multiplayer == SINGLE && singleplayerAliveEndGameAndSave )
			{
				// noop - button created earlier.
			}
			else
			{
				// cancel button
				button = newButton();
				strcpy(button->label, language[1316]);
				button->x = subx2 - strlen(language[1316]) * 12 - 16;
				button->y = suby2 - 28;
				button->sizex = strlen(language[1316]) * 12 + 8;
				button->sizey = 20;
				button->action = &buttonCloseSubwindow;
				button->visible = 1;
				button->focused = 1;
			}
		}
	}
	else
	{
		ttfPrintText(ttf16, text.x, text.y, endgameText);
	}

	if ( multiplayer != CLIENT )
	{
		text.y += 24;
		++numOption;
		if ( ((omousex >= text.x && omousex < text.x + strlen(language[1312]) * text.w && omousey >= text.y && omousey < text.y + text.h) || (menuselect == numOption)) && subwindow == 0 && introstage == 1 )
		{
			//restart game
			menuselect = numOption;
			ttfPrintTextFormattedColor(ttf16, text.x, text.y, colorGray, language[1312]);
			if ( inputIsPressed )
			{
				pauseMenuOnInputPressed();

				// create confirmation window
				subwindow = 1;
				subx1 = xres / 2 - 164;
				subx2 = xres / 2 + 164;
				suby1 = yres / 2 - 48;
				suby2 = yres / 2 + 48;
				strcpy(subtext, language[1130]);

				// close button
				button_t* button = newButton();
				strcpy(button->label, "x");
				button->x = subx2 - 20;
				button->y = suby1;
				button->sizex = 20;
				button->sizey = 20;
				button->action = &buttonCloseSubwindow;
				button->visible = 1;
				button->focused = 1;
				button->key = SDL_SCANCODE_ESCAPE;
				button->joykey = joyimpulses[INJOY_MENU_CANCEL];

				// yes button
				button = newButton();
				strcpy(button->label, language[1314]);
				button->x = subx1 + 8;
				button->y = suby2 - 28;
				button->sizex = strlen(language[1314]) * 12 + 8;
				button->sizey = 20;
				if ( multiplayer == SINGLE )
				{
					button->action = &buttonStartSingleplayer;
				}
				else
				{
					button->action = &buttonStartServer;
				}
				button->visible = 1;
				button->focused = 1;
				button->key = SDL_SCANCODE_RETURN;
				button->joykey = joyimpulses[INJOY_MENU_NEXT];

				// no button
				button = newButton();
				strcpy(button->label, language[1315]);
				button->x = subx2 - strlen(language[1315]) * 12 - 16;
				button->y = suby2 - 28;
				button->sizex = strlen(language[1315]) * 12 + 8;
				button->sizey = 20;
				button->action = &buttonCloseSubwindow;
				button->visible = 1;
				button->focused = 1;
			}
		}
		else
		{
			ttfPrintText(ttf16, text.x, text.y, language[1312]);
		}
	}
	
	text.y += 24;
	++numOption;
	if ( ((omousex >= text.x && omousex < text.x + strlen(quitgameText) * text.w && omousey >= text.y && omousey < text.y + text.h) || (menuselect == numOption)) && subwindow == 0 && introstage == 1 )
	{
		menuselect = numOption;
		// save & quit
		ttfPrintTextFormattedColor(ttf16, text.x, text.y, colorGray, quitgameText);
		if ( inputIsPressed )
		{
			pauseMenuOnInputPressed();

			// create confirmation window
			subwindow = 1;
			subx1 = xres / 2 - 188;
			subx2 = xres / 2 + 188;
			suby1 = yres / 2 - 64;
			suby2 = yres / 2 + 64;
			strcpy(subtext, language[1131]);
			if ( multiplayer == SINGLE )
			{
				if ( !singleplayerAliveEndGameAndSave )
				{
					if ( singleplayerBossLevelDisableSaveOnExit )
					{
						strcpy(subtext, language[3991]);
					}
					else
					{
						strcpy(subtext, language[3988]);
					}
				}
			}
			else if ( multiplayer == SERVER )
			{
				if ( !multiplayerAliveEndGameAndSave )
				{
					subx1 = xres / 2 - 224;
					subx2 = xres / 2 + 224;
					suby1 = yres / 2 - 64;
					suby2 = yres / 2 + 64;
					strcpy(subtext, language[3989]);
				}
			}

			// yes button
			button_t* button = newButton();
			strcpy(button->label, language[1314]);
			button->x = subx1 + 8;
			button->y = suby2 - 28;
			button->sizex = strlen(language[1314]) * 12 + 8;
			button->sizey = 20;
			button->action = &buttonQuitConfirm;
			button->visible = 1;
			button->focused = 1;
			button->key = SDL_SCANCODE_RETURN;
			button->joykey = joyimpulses[INJOY_MENU_NEXT]; //TODO: Select which button to activate via dpad.

			// no button
			// button = newButton();
			// strcpy(button->label, language[1315]);
			// button->sizex = strlen(language[1315]) * 12 + 8;
			// button->sizey = 20;
			// button->x = subx1 + (subx2 - subx1) / 2 - button->sizex / 2;
			// button->y = suby2 - 28;
			// button->action = &buttonQuitNoSaveConfirm;
			// button->visible = 1;
			// button->focused = 1;

			// cancel button
			button = newButton();
			strcpy(button->label, language[1316]);
			button->x = subx2 - strlen(language[1316]) * 12 - 16;
			button->y = suby2 - 28;
			button->sizex = strlen(language[1316]) * 12 + 8;
			button->sizey = 20;
			button->action = &buttonCloseSubwindow;
			button->visible = 1;
			button->focused = 1;

			// close button
			button = newButton();
			strcpy(button->label, "x");
			button->x = subx2 - 20;
			button->y = suby1;
			button->sizex = 20;
			button->sizey = 20;
			button->action = &buttonCloseSubwindow;
			button->visible = 1;
			button->focused = 1;
			button->key = SDL_SCANCODE_ESCAPE;
			button->joykey = joyimpulses[INJOY_MENU_CANCEL];
		}
	}
	else
	{
		ttfPrintText(ttf16, text.x, text.y, quitgameText);
	}
}

void handleTutorialPauseMenu()
{
	Sint32 mousex = inputs.getMouse(clientnum, Inputs::MouseInputs::X);
	Sint32 mousey = inputs.getMouse(clientnum, Inputs::MouseInputs::Y);
	Sint32 omousex = inputs.getMouse(clientnum, Inputs::MouseInputs::OX);
	Sint32 omousey = inputs.getMouse(clientnum, Inputs::MouseInputs::OY);

	const Uint32 colorGray = uint32ColorGray(*mainsurface);
	const bool inputIsPressed = (inputs.bMouseLeft(clientnum) || keystatus[SDL_SCANCODE_RETURN] || (inputs.bControllerInputPressed(clientnum, INJOY_MENU_NEXT) && rebindaction == -1));
	SDL_Rect text;
	text.x = 50;
	text.h = 18;
	text.w = 18;

	bool mapIsTutorialHub = false;
	if ( !strcmp(map.name, "Tutorial Hub") )
	{
		mapIsTutorialHub = true;
	}

	int numOption = 1;
	bool achievementsMenu = false;
#if !defined STEAMWORKS
#ifdef USE_EOS
	achievementsMenu = true;
#endif
#endif

	text.y = yres / 4 + 80;
	if ( ((omousex >= text.x && omousex < text.x + strlen(language[1309]) * text.w && omousey >= text.y && omousey < text.y + text.h) || (menuselect == numOption)) && subwindow == 0 && introstage == 1 )
	{
		// resume game
		menuselect = numOption;
		ttfPrintTextFormattedColor(ttf16, text.x, text.y, colorGray, language[1309]);
		if ( inputIsPressed )
		{
			pauseMenuOnInputPressed();
			pauseGame(1, MAXPLAYERS);
		}
	}
	else
	{
		ttfPrintText(ttf16, text.x, text.y, language[1309]);
	}

	//++numOption;
	//text.y += 24;
	//if ( ((omousex >= text.x && omousex < text.x + strlen(language[1306]) * text.w && omousey >= text.y && omousey < text.y + text.h) || (menuselect == numOption)) && subwindow == 0 && introstage == 1 )
	//{
	//	// settings menu
	//	menuselect = numOption;
	//	ttfPrintTextFormattedColor(ttf16, text.x, text.y, colorGray, language[1306]);
	//	if ( inputIsPressed )
	//	{
	//		pauseMenuOnInputPressed();
	//		gameModeManager.Tutorial.Menu.open();
	//	}
	//}
	//else
	//{
	//	ttfPrintText(ttf16, text.x, text.y, language[1306]);
	//}

	if ( achievementsMenu )
	{
		++numOption;
		text.y += 24;
		if ( ((omousex >= text.x && omousex < text.x + strlen(language[3971]) * text.w && omousey >= text.y && omousey < text.y + text.h) || (menuselect == numOption)) && subwindow == 0 && introstage == 1 )
		{
			// achievements menu
			menuselect = numOption;
			ttfPrintTextFormattedColor(ttf16, text.x, text.y, colorGray, language[3971]);
			if ( inputIsPressed )
			{
				pauseMenuOnInputPressed();
				openAchievementsWindow();
			}
		}
		else
		{
			ttfPrintText(ttf16, text.x, text.y, language[3971]);
		}
	}

	++numOption;
	text.y += 24;
	if ( ((omousex >= text.x && omousex < text.x + strlen(language[1306]) * text.w && omousey >= text.y && omousey < text.y + text.h) || (menuselect == numOption)) && subwindow == 0 && introstage == 1 )
	{
		// settings menu
		menuselect = numOption;
		ttfPrintTextFormattedColor(ttf16, text.x, text.y, colorGray, language[1306]);
		if ( inputIsPressed )
		{
			pauseMenuOnInputPressed();
			openSettingsWindow();
		}
	}
	else
	{
		ttfPrintText(ttf16, text.x, text.y, language[1306]);
	}

	++numOption;
	text.y += 24;
	char* returnToHubOptionText = language[3958];
	if ( mapIsTutorialHub )
	{
		returnToHubOptionText = language[3969];
	}

	if ( ((omousex >= text.x && omousex < text.x + strlen(returnToHubOptionText) * text.w && omousey >= text.y && omousey < text.y + text.h) || (menuselect == numOption)) && subwindow == 0 && introstage == 1 )
	{
		// return to hub
		menuselect = numOption;
		ttfPrintTextFormattedColor(ttf16, text.x, text.y, colorGray, returnToHubOptionText);
		if ( inputIsPressed )
		{
			pauseMenuOnInputPressed();

			// create confirmation window
			subwindow = 1;
			subx1 = xres / 2 - 144;
			subx2 = xres / 2 + 144;
			suby1 = yres / 2 - 48;
			suby2 = yres / 2 + 48;

			if ( mapIsTutorialHub )
			{
				strcpy(subtext, language[3970]);
			}
			else
			{
				strcpy(subtext, language[3960]);
			}

			// add a cancel button
			button_t* button = newButton();
			strcpy(button->label, language[1316]);
			button->x = subx2 - strlen(language[1316]) * 12 - 16;
			button->y = suby2 - 28;
			button->sizex = strlen(language[1316]) * 12 + 8;
			button->sizey = 20;
			button->action = &buttonCloseSubwindow;
			button->visible = 1;
			button->focused = 1;

			// close button
			button = newButton();
			strcpy(button->label, "x");
			button->x = subx2 - 20;
			button->y = suby1;
			button->sizex = 20;
			button->sizey = 20;
			button->action = &buttonCloseSubwindow;
			button->visible = 1;
			button->focused = 1;
			button->key = SDL_SCANCODE_ESCAPE;
			button->joykey = joyimpulses[INJOY_MENU_CANCEL];

			// yes button
			button = newButton();
			strcpy(button->label, language[1314]);
			button->x = subx1 + 8;
			button->y = suby2 - 28;
			button->sizex = strlen(language[1314]) * 12 + 8;
			button->sizey = 20;
			button->action = &gameModeManager.Tutorial.buttonReturnToTutorialHub;
			button->visible = 1;
			button->focused = 1;
			button->key = SDL_SCANCODE_RETURN;
			button->joykey = joyimpulses[INJOY_MENU_NEXT];
		}
	}
	else
	{
		ttfPrintText(ttf16, text.x, text.y, returnToHubOptionText);
	}

	if ( !mapIsTutorialHub )
	{
		++numOption;
		text.y += 24;
		if ( ((omousex >= text.x && omousex < text.x + strlen(language[3957]) * text.w && omousey >= text.y && omousey < text.y + text.h) || (menuselect == numOption)) && subwindow == 0 && introstage == 1 )
		{
			//restart game
			menuselect = numOption;
			ttfPrintTextFormattedColor(ttf16, text.x, text.y, colorGray, language[3957]);
			if ( inputIsPressed )
			{
				pauseMenuOnInputPressed();

				// create confirmation window
				subwindow = 1;
				subx1 = xres / 2 - 144;
				subx2 = xres / 2 + 144;
				suby1 = yres / 2 - 48;
				suby2 = yres / 2 + 48;
				strcpy(subtext, language[3956]);

				// add a cancel button
				button_t* button = newButton();
				strcpy(button->label, language[1316]);
				button->x = subx2 - strlen(language[1316]) * 12 - 16;
				button->y = suby2 - 28;
				button->sizex = strlen(language[1316]) * 12 + 8;
				button->sizey = 20;
				button->action = &buttonCloseSubwindow;
				button->visible = 1;
				button->focused = 1;

				// close button
				button = newButton();
				strcpy(button->label, "x");
				button->x = subx2 - 20;
				button->y = suby1;
				button->sizex = 20;
				button->sizey = 20;
				button->action = &buttonCloseSubwindow;
				button->visible = 1;
				button->focused = 1;
				button->key = SDL_SCANCODE_ESCAPE;
				button->joykey = joyimpulses[INJOY_MENU_CANCEL];

				// yes button
				button = newButton();
				strcpy(button->label, language[1314]);
				button->x = subx1 + 8;
				button->y = suby2 - 28;
				button->sizex = strlen(language[1314]) * 12 + 8;
				button->sizey = 20;
				button->action = &gameModeManager.Tutorial.buttonRestartTrial;
				button->visible = 1;
				button->focused = 1;
				button->key = SDL_SCANCODE_RETURN;
				button->joykey = joyimpulses[INJOY_MENU_NEXT];
			}
		}
		else
		{
			ttfPrintText(ttf16, text.x, text.y, language[3957]);
		}
	}

	text.y += 24;
	++numOption;

	if ( ((omousex >= 50 && omousex < 50 + strlen(language[3959]) * 18 && omousey >= text.y && omousey < text.y + text.h) || (menuselect == numOption)) && subwindow == 0 && introstage == 1 )
	{
		menuselect = numOption;
		// return to main menu
		ttfPrintTextFormattedColor(ttf16, text.x, text.y, colorGray, language[3959]);
		if ( inputIsPressed )
		{
			pauseMenuOnInputPressed();

			// create confirmation window
			subwindow = 1;
			subx1 = xres / 2 - 144;
			subx2 = xres / 2 + 144;
			suby1 = yres / 2 - 48;
			suby2 = yres / 2 + 48;
			strcpy(subtext, language[3961]);

			// yes button
			button_t* button = newButton();
			strcpy(button->label, language[1314]);
			button->x = subx1 + 8;
			button->y = suby2 - 28;
			button->sizex = strlen(language[1314]) * 12 + 8;
			button->sizey = 20;
			button->action = &buttonEndGameConfirm;
			button->visible = 1;
			button->focused = 1;
			button->key = SDL_SCANCODE_RETURN;
			button->joykey = joyimpulses[INJOY_MENU_NEXT]; //TODO: Select which button to activate via dpad.

			// cancel button
			button = newButton();
			strcpy(button->label, language[1316]);
			button->x = subx2 - strlen(language[1316]) * 12 - 16;
			button->y = suby2 - 28;
			button->sizex = strlen(language[1316]) * 12 + 8;
			button->sizey = 20;
			button->action = &buttonCloseSubwindow;
			button->visible = 1;
			button->focused = 1;

			// close button
			button = newButton();
			strcpy(button->label, "x");
			button->x = subx2 - 20;
			button->y = suby1;
			button->sizex = 20;
			button->sizey = 20;
			button->action = &buttonCloseSubwindow;
			button->visible = 1;
			button->focused = 1;
			button->key = SDL_SCANCODE_ESCAPE;
			button->joykey = joyimpulses[INJOY_MENU_CANCEL];
		}
	}
	else
	{
		ttfPrintText(ttf16, text.x, text.y, language[3959]);
	}
}

/*-------------------------------------------------------------------------------

	handleMainMenu

	draws & processes the game menu; if passed true, does the whole menu,
	otherwise just handles the reduced ingame menu

-------------------------------------------------------------------------------*/

void handleMainMenu(bool mode)
{
	int x, c;
	//int y;
	bool b;
	//int tilesreceived=0;
	//Mix_Music **music, *intromusic, *splashmusic, *creditsmusic;
	node_t* node, *nextnode;
	Entity* entity;
	//SDL_Surface *sky_bmp;
	button_t* button;
	Sint32 mousex = inputs.getMouse(clientnum, Inputs::MouseInputs::X);
	Sint32 mousey = inputs.getMouse(clientnum, Inputs::MouseInputs::Y);
	Sint32 omousex = inputs.getMouse(clientnum, Inputs::MouseInputs::OX);
	Sint32 omousey = inputs.getMouse(clientnum, Inputs::MouseInputs::OY);

#ifdef STEAMWORKS
	if ( SteamApps()->BIsDlcInstalled(1010820) )
	{
		enabledDLCPack1 = true;
	}
	if ( SteamApps()->BIsDlcInstalled(1010821) )
	{
		enabledDLCPack2 = true;
	}
#else
#endif // STEAMWORKS
	if ( menuOptions.empty() )
	{
		initMenuOptions();
	}

	if ( !movie )
	{
		// title pic
		SDL_Rect src;
		src.x = 20;
		src.y = 20;
		src.w = title_bmp->w * (230.0 / 240.0); // new banner scaled to old size.
		src.h = title_bmp->h * (230.0 / 240.0);
		if ( mode || introstage != 5 )
		{
			drawImageScaled(title_bmp, nullptr, &src);
		}
		if ( mode && subtitleVisible )
		{
			Uint32 colorYellow = SDL_MapRGBA(mainsurface->format, 255, 255, 0, 255);
			Uint32 len = strlen(language[1910 + subtitleCurrent]);
			ttfPrintTextColor(ttf16, src.x + src.w / 2 - (len * TTF16_WIDTH) / 2, src.y + src.h - 32, colorYellow, true, language[1910 + subtitleCurrent]);
		}
#ifdef STEAMWORKS
		if ( mode )
		{
			// print community links
			if ( SteamUser()->BLoggedOn() )
			{
				if ( ticks % 50 == 0 )
				{
					UIToastNotificationManager.createCommunityNotification();
				}

				// upgrade steam achievement for existing hunters
				if ( ticks % 250 == 0 )
				{
					bool unlocked = false;
					if ( SteamUserStats()->GetAchievement("BARONY_ACH_GUDIPARIAN_BAZI", &unlocked) )
					{
						if ( unlocked )
						{
							steamAchievement("BARONY_ACH_RANGER_DANGER");
						}
					}
				}
			}
		}
#elif defined USE_EOS
		if ( mode )
		{
			if ( ticks % 50 == 0 )
			{
				UIToastNotificationManager.createCommunityNotification();
			}
		}
#endif

#ifdef USE_EOS
		if ( mode && EOS.StatGlobalManager.bPromoEnabled )
		{
			if ( ticks % 100 == 0 )
			{
				UIToastNotificationManager.createPromoNotification();
			}
		}
#endif

		// gray text color
		Uint32 colorGray = SDL_MapRGBA(mainsurface->format, 128, 128, 128, 255);

		// print game version
		if ( mode || introstage != 5 )
		{
			char version[64];
			strcpy(version, __DATE__ + 7);
			strcat(version, ".");
			if ( !strncmp(__DATE__, "Jan", 3) )
			{
				strcat(version, "01");
			}
			else if ( !strncmp(__DATE__, "Feb", 3) )
			{
				strcat(version, "02");
			}
			else if ( !strncmp(__DATE__, "Mar", 3) )
			{
				strcat(version, "03");
			}
			else if ( !strncmp(__DATE__, "Apr", 3) )
			{
				strcat(version, "04");
			}
			else if ( !strncmp(__DATE__, "May", 3) )
			{
				strcat(version, "05");
			}
			else if ( !strncmp(__DATE__, "Jun", 3) )
			{
				strcat(version, "06");
			}
			else if ( !strncmp(__DATE__, "Jul", 3) )
			{
				strcat(version, "07");
			}
			else if ( !strncmp(__DATE__, "Aug", 3) )
			{
				strcat(version, "08");
			}
			else if ( !strncmp(__DATE__, "Sep", 3) )
			{
				strcat(version, "09");
			}
			else if ( !strncmp(__DATE__, "Oct", 3) )
			{
				strcat(version, "10");
			}
			else if ( !strncmp(__DATE__, "Nov", 3) )
			{
				strcat(version, "11");
			}
			else if ( !strncmp(__DATE__, "Dec", 3) )
			{
				strcat(version, "12");
			}
			strcat(version, ".");
			int day = atoi(__DATE__ + 4);
			if (day >= 10)
			{
				strncat(version, __DATE__ + 4, 2);
			}
			else
			{
				strcat(version, "0");
				strncat(version, __DATE__ + 5, 1);
			}
			int w, h;
			getSizeOfText(ttf8, version, &w, &h);
			ttfPrintTextFormatted(ttf8, xres - 8 - w, yres - 4 - h, "%s", version);
			int h2 = h;
			getSizeOfText(ttf8, VERSION, &w, &h);
			ttfPrintTextFormatted(ttf8, xres - 8 - w, yres - 8 - h - h2, VERSION);
			if ( gamemods_numCurrentModsLoaded >= 0 || conductGameChallenges[CONDUCT_MODDED] )
			{
				if ( gamemods_numCurrentModsLoaded >= 0 )
				{
					ttfPrintTextFormatted(ttf8, xres - 8 - TTF8_WIDTH * 16, yres - 12 - h - h2 * 2, "%2d mod(s) loaded", gamemods_numCurrentModsLoaded);
				}
				else if ( !mode )
				{
					ttfPrintTextFormatted(ttf8, xres - 8 - TTF8_WIDTH * 24, yres - 12 - h - h2 * 2, "Using modified map files");
				}
			}
#if (defined STEAMWORKS || defined USE_EOS)
			if ( gamemods_disableSteamAchievements
				|| (intro == false && 
					(conductGameChallenges[CONDUCT_CHEATS_ENABLED]
					|| conductGameChallenges[CONDUCT_LIFESAVING])) )
			{
				getSizeOfText(ttf8, language[3003], &w, &h);
				if ( gamemods_numCurrentModsLoaded < 0 && !conductGameChallenges[CONDUCT_MODDED] )
				{
					h = -4;
				}
				if ( gameModeManager.getMode() != GameModeManager_t::GAME_MODE_DEFAULT )
				{
					// achievements are disabled
					ttfPrintTextFormatted(ttf8, xres - 8 - w, yres - 16 - h - h2 * 3, language[3003]);
				}
				else
				{
					// achievements are disabled
					ttfPrintTextFormatted(ttf8, xres - 8 - w, yres - 16 - h - h2 * 3, language[3003]);
				}
			}
#endif

#ifdef STEAMWORKS
			getSizeOfText(ttf8, language[2549], &w, &h);
			if ( (omousex >= xres - 8 - w && omousex < xres && omousey >= 8 && omousey < 8 + h)
				&& subwindow == 0
				&& introstage == 1
				&& SteamUser()->BLoggedOn() )
			{
				if ( inputs.bMouseLeft(clientnum) )
				{
					inputs.mouseClearLeft(clientnum);
					playSound(139, 64);
					SteamAPICall_NumPlayersOnline = SteamUserStats()->GetNumberOfCurrentPlayers();
				}
				ttfPrintTextFormattedColor(ttf8, xres - 8 - w, 8, colorGray, language[2549], steamOnlinePlayers);
			}
			else if ( SteamUser()->BLoggedOn() )
			{
				ttfPrintTextFormatted(ttf8, xres - 8 - w, 8, language[2549], steamOnlinePlayers);
			}
			if ( intro == false )
			{
				if ( conductGameChallenges[CONDUCT_CHEATS_ENABLED] )
				{
					getSizeOfText(ttf8, language[2986], &w, &h);
					ttfPrintTextFormatted(ttf8, xres - 8 - w, 8 + h, language[2986]);
				}
			}
			if ( SteamUser()->BLoggedOn() && SteamAPICall_NumPlayersOnline == 0 )
			{
				SteamAPICall_NumPlayersOnline = SteamUserStats()->GetNumberOfCurrentPlayers();
			}
			bool bFailed = false;
			if ( SteamUser()->BLoggedOn() )
			{
				SteamUtils()->GetAPICallResult(SteamAPICall_NumPlayersOnline, &NumberOfCurrentPlayers, sizeof(NumberOfCurrentPlayers_t), 1107, &bFailed);
				if ( NumberOfCurrentPlayers.m_bSuccess )
				{
					steamOnlinePlayers = NumberOfCurrentPlayers.m_cPlayers;
				}
				uint64 id = SteamUser()->GetSteamID().ConvertToUint64();
			}
#elif defined USE_EOS
#else
			if ( intro && introstage == 1 )
			{
				getSizeOfText(ttf8, language[3402], &w, &h);
				if ( (omousex >= xres - 8 - w && omousex < xres && omousey >= 8 && omousey < 8 + h)
					&& subwindow == 0 )
				{
					if ( inputs.bMouseLeft(clientnum) )
					{
						inputs.mouseClearLeft(clientnum);
						playSound(139, 64);
						windowEnterSerialPrompt();
						
					}
					ttfPrintTextFormattedColor(ttf8, xres - 8 - w, 8, colorGray, language[3402]);
				}
				else
				{
					ttfPrintTextFormatted(ttf8, xres - 8 - w, 8, language[3402]);
				}
			}
#endif // STEAMWORKS
		}
		// navigate with arrow keys
		if (!subwindow)
		{
			navigateMainMenuItems(mode);
		}

		// draw menu
		if ( mode )
		{
			/*
			 * Mouse menu item select/highlight implicitly handled here.
			 */
			if ( keystatus[SDL_SCANCODE_L] && (keystatus[SDL_SCANCODE_LCTRL] || keystatus[SDL_SCANCODE_RCTRL]) )
			{
				buttonOpenCharacterCreationWindow(nullptr);
				client_classes[clientnum] = CLASS_BARBARIAN;
				stats[0]->appearance = 0;
				stats[0]->playerRace = RACE_HUMAN;
				initClass(0);
				strcpy(stats[0]->name, "The Server");
				keystatus[SDL_SCANCODE_L] = 0;
				keystatus[SDL_SCANCODE_LCTRL] = 0;
				keystatus[SDL_SCANCODE_RCTRL] = 0;
				multiplayerselect = SERVER;
				charcreation_step = 6;
				camera_charsheet_offsetyaw = (330) * PI / 180;
				directConnect = true;
				strcpy(portnumber_char, "12345");
				buttonHostLobby(nullptr);
			}

			if ( keystatus[SDL_SCANCODE_M] && (keystatus[SDL_SCANCODE_LCTRL] || keystatus[SDL_SCANCODE_RCTRL]) )
			{
				buttonOpenCharacterCreationWindow(nullptr);
				client_classes[clientnum] = CLASS_BARBARIAN;
				stats[0]->appearance = 0;
				stats[0]->playerRace = RACE_HUMAN;
				initClass(0);
				strcpy(stats[0]->name, "The Client");
				keystatus[SDL_SCANCODE_M] = 0;
				keystatus[SDL_SCANCODE_LCTRL] = 0;
				keystatus[SDL_SCANCODE_RCTRL] = 0;
				multiplayerselect = CLIENT;
				charcreation_step = 6;
				camera_charsheet_offsetyaw = (330) * PI / 180;
				directConnect = true;
				strcpy(connectaddress, "localhost:12345");
				buttonJoinLobby(nullptr);
			}

			bool mainMenuSelectInputIsPressed = (inputs.bMouseLeft(clientnum) || keystatus[SDL_SCANCODE_RETURN] || (inputs.bControllerInputPressed(clientnum, INJOY_MENU_NEXT) && rebindaction == -1));

			//"Start Game" button.
			SDL_Rect text;
			text.x = 50;
			text.h = 18;
			text.w = 18;

			const Uint32 numMenuOptions = menuOptions.size();
			Uint32 menuIndex = 0;
			text.y = yres / 4 + 80 + (menuOptions.at(menuIndex).second - 1) * 24;
			Uint32 menuOptionSize = std::max(static_cast<Uint32>(menuOptions.at(menuIndex).first.size()), static_cast<Uint32>(4));

			if ( ((omousex >= text.x && omousex < text.x + menuOptionSize * text.w && omousey >= text.y && omousey < text.y + text.h) || (menuselect == menuOptions.at(menuIndex).second)) && subwindow == 0 && introstage == 1 )
			{
				menuselect = menuOptions.at(menuIndex).second;
				ttfPrintTextFormattedColor(ttf16, text.x, text.y, colorGray, "%s", menuOptions.at(menuIndex).first.c_str());
				if ( mainMenuSelectInputIsPressed )
				{
					pauseMenuOnInputPressed();

					// look for a save game
					bool reloadModels = false;
					int modelsIndexUpdateStart = 1;
					int modelsIndexUpdateEnd = nummodels;

					bool reloadSounds = false;
					if ( gamemods_customContentLoadedFirstTime )
					{
						if ( physfsSearchModelsToUpdate() || !gamemods_modelsListModifiedIndexes.empty() )
						{
							reloadModels = true; // we had some models already loaded which should be reset
						}
						if ( physfsSearchSoundsToUpdate() )
						{
							reloadSounds = true; // we had some sounds already loaded which should be reset
						}
					}

					gamemodsClearAllMountedPaths();

					if ( reloadModels )
					{
						// print a loading message
						drawClearBuffers();
						int w, h;
						getSizeOfText(ttf16, language[2990], &w, &h);
						ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, language[2990]);
						GO_SwapBuffers(screen);

						physfsModelIndexUpdate(modelsIndexUpdateStart, modelsIndexUpdateEnd, true);
						generatePolyModels(modelsIndexUpdateStart, modelsIndexUpdateEnd, false);
						gamemods_modelsListLastStartedUnmodded = true;
					}
					if ( reloadSounds )
					{
						// print a loading message
						drawClearBuffers();
						int w, h;
						getSizeOfText(ttf16, language[2988], &w, &h);
						ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, language[2988]);
						GO_SwapBuffers(screen);
						physfsReloadSounds(true);
						gamemods_soundsListLastStartedUnmodded = true;
					}

					if ( gamemods_tileListRequireReloadUnmodded )
					{
						drawClearBuffers();
						int w, h;
						getSizeOfText(ttf16, language[3018], &w, &h);
						ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, language[3018]);
						GO_SwapBuffers(screen);
						physfsReloadTiles(true);
						gamemods_tileListRequireReloadUnmodded = false;
					}

					if ( gamemods_booksRequireReloadUnmodded )
					{
						drawClearBuffers();
						int w, h;
						getSizeOfText(ttf16, language[2992], &w, &h);
						ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, language[2992]);
						GO_SwapBuffers(screen);
						physfsReloadBooks();
						gamemods_booksRequireReloadUnmodded = false;
					}

					if ( gamemods_musicRequireReloadUnmodded )
					{
						gamemodsUnloadCustomThemeMusic();
						drawClearBuffers();
						int w, h;
						getSizeOfText(ttf16, language[2994], &w, &h);
						ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, language[2994]);
						GO_SwapBuffers(screen);
						bool reloadIntroMusic = false;
						physfsReloadMusic(reloadIntroMusic, true);
						if ( reloadIntroMusic )
						{
#ifdef SOUND
							playmusic(intromusic[rand() % (NUMINTROMUSIC - 1)], false, true, true);
#endif			
						}
						gamemods_musicRequireReloadUnmodded = false;
					}

					if ( gamemods_langRequireReloadUnmodded )
					{
						drawClearBuffers();
						int w, h;
						getSizeOfText(ttf16, language[3005], &w, &h);
						ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, language[3005]);
						GO_SwapBuffers(screen);
						reloadLanguage();
						gamemods_langRequireReloadUnmodded = false;
					}

					if ( gamemods_itemsTxtRequireReloadUnmodded )
					{
						drawClearBuffers();
						int w, h;
						getSizeOfText(ttf16, language[3009], &w, &h);
						ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, language[3009]);
						GO_SwapBuffers(screen);
						physfsReloadItemsTxt();
						gamemods_itemsTxtRequireReloadUnmodded = false;
					}

					if ( gamemods_itemSpritesRequireReloadUnmodded )
					{
						drawClearBuffers();
						int w, h;
						getSizeOfText(ttf16, language[3007], &w, &h);
						ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, language[3007]);
						GO_SwapBuffers(screen);
						physfsReloadItemSprites(true);
						gamemods_itemSpritesRequireReloadUnmodded = false;
					}

					if ( gamemods_spriteImagesRequireReloadUnmodded )
					{
						drawClearBuffers();
						int w, h;
						getSizeOfText(ttf16, language[3016], &w, &h);
						ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, language[3016]);
						GO_SwapBuffers(screen);
						physfsReloadSprites(true);
						gamemods_spriteImagesRequireReloadUnmodded = false;
					}

					if ( gamemods_itemsGlobalTxtRequireReloadUnmodded )
					{
						gamemods_itemsGlobalTxtRequireReloadUnmodded = false;
						printlog("[PhysFS]: Unloaded modified items/items_global.txt file, reloading item spawn levels...");
						loadItemLists();
					}

					if ( gamemods_monsterLimbsRequireReloadUnmodded )
					{
						drawClearBuffers();
						int w, h;
						getSizeOfText(ttf16, language[3014], &w, &h);
						ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, language[3014]);
						GO_SwapBuffers(screen);
						physfsReloadMonsterLimbFiles();
						gamemods_monsterLimbsRequireReloadUnmodded = false;
					}

					if ( gamemods_systemImagesReloadUnmodded )
					{
						drawClearBuffers();
						int w, h;
						getSizeOfText(ttf16, language[3016], &w, &h);
						ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, language[3016]);
						GO_SwapBuffers(screen);
						physfsReloadSystemImages();
						gamemods_systemImagesReloadUnmodded = false;
						systemResourceImagesToReload.clear();

						// tidy up some other resource files.
						rightsidebar_titlebar_img = spell_list_titlebar_bmp;
						rightsidebar_slot_img = spell_list_gui_slot_bmp;
						rightsidebar_slot_highlighted_img = spell_list_gui_slot_highlighted_bmp;
					}

					gamemods_disableSteamAchievements = false;

					if ( gameModeManager.Tutorial.FirstTimePrompt.showFirstTimePrompt )
					{
						gameModeManager.Tutorial.FirstTimePrompt.createPrompt();
					}
					else
					{
						if ( anySaveFileExists() )
						{
							//openLoadGameWindow(NULL);
							openNewLoadGameWindow(nullptr);
						}
						else
						{
							buttonOpenCharacterCreationWindow(NULL);
						}
					}
				}
			}
			else
			{
				ttfPrintText(ttf16, text.x, text.y, menuOptions.at(menuIndex).first.c_str());
			}

			++menuIndex;
			text.y = yres / 4 + 80 + (menuOptions.at(menuIndex).second - 1) * 24;
			menuOptionSize = std::max(static_cast<Uint32>(menuOptions.at(menuIndex).first.size()), static_cast<Uint32>(4));

			//"Introduction" button.
			if ( ((omousex >= text.x && omousex < text.x + menuOptionSize * text.w && omousey >= text.y && omousey < text.y + text.h) || (menuselect == menuOptions.at(menuIndex).second)) && subwindow == 0 && introstage == 1 )
			{
				menuselect = menuOptions.at(menuIndex).second;
				ttfPrintTextFormattedColor(ttf16, text.x, text.y, colorGray, "%s", menuOptions.at(menuIndex).first.c_str());
				if ( mainMenuSelectInputIsPressed )
				{
					pauseMenuOnInputPressed();

					introstage = 6; // goes to intro movie
					fadeout = true;
#ifdef MUSIC
					playmusic(introductionmusic, true, true, false);
#endif
				}
			}
			else
			{
				ttfPrintText(ttf16, text.x, text.y, menuOptions.at(menuIndex).first.c_str());
			}

			++menuIndex;
			text.y = yres / 4 + 80 + (menuOptions.at(menuIndex).second - 1) * 24;
			menuOptionSize = std::max(static_cast<Uint32>(menuOptions.at(menuIndex).first.size()), static_cast<Uint32>(4));

			//"Hall of Trials" Button.
			if ( ((omousex >= text.x && omousex < text.x + menuOptionSize * text.w && omousey >= text.y && omousey < text.y + text.h) || (menuselect == menuOptions.at(menuIndex).second)) && subwindow == 0 && introstage == 1 )
			{
				menuselect = menuOptions.at(menuIndex).second;
				ttfPrintTextFormattedColor(ttf16, text.x, text.y, colorGray, "%s", menuOptions.at(menuIndex).first.c_str());
				if ( mainMenuSelectInputIsPressed )
				{
					pauseMenuOnInputPressed();

					gameModeManager.Tutorial.readFromFile();
					gameModeManager.Tutorial.Menu.open();
				}
			}
			else
			{
				ttfPrintText(ttf16, text.x, text.y, menuOptions.at(menuIndex).first.c_str());
			}

			++menuIndex;
			text.y = yres / 4 + 80 + (menuOptions.at(menuIndex).second - 1) * 24;
			menuOptionSize = std::max(static_cast<Uint32>(menuOptions.at(menuIndex).first.size()), static_cast<Uint32>(4));

			//"Statistics" Button.
			if ( ((omousex >= text.x && omousex < text.x + menuOptionSize * text.w && omousey >= text.y && omousey < text.y + text.h) || (menuselect == menuOptions.at(menuIndex).second)) && subwindow == 0 && introstage == 1 )
			{
				menuselect = menuOptions.at(menuIndex).second;
				ttfPrintTextFormattedColor(ttf16, text.x, text.y, colorGray, "%s", menuOptions.at(menuIndex).first.c_str());
				if ( mainMenuSelectInputIsPressed )
				{
					pauseMenuOnInputPressed();

					buttonOpenScoresWindow(nullptr);
				}
			}
			else
			{
				ttfPrintText(ttf16, text.x, text.y, menuOptions.at(menuIndex).first.c_str());
			}

#if (defined USE_EOS && !defined STEAMWORKS)
			++menuIndex;
			text.y = yres / 4 + 80 + (menuOptions.at(menuIndex).second - 1) * 24;
			menuOptionSize = std::max(static_cast<Uint32>(menuOptions.at(menuIndex).first.size()), static_cast<Uint32>(4));

			//"Achievements" Button.
			if ( ((omousex >= text.x && omousex < text.x + menuOptionSize * text.w && omousey >= text.y && omousey < text.y + text.h) || (menuselect == menuOptions.at(menuIndex).second)) && subwindow == 0 && introstage == 1 )
			{
				menuselect = menuOptions.at(menuIndex).second;
				ttfPrintTextFormattedColor(ttf16, text.x, text.y, colorGray, "%s", menuOptions.at(menuIndex).first.c_str());
				if ( mainMenuSelectInputIsPressed )
				{
					pauseMenuOnInputPressed();

					openAchievementsWindow();
				}
			}
			else
			{
				ttfPrintText(ttf16, text.x, text.y, menuOptions.at(menuIndex).first.c_str());
			}
#endif 

			++menuIndex;
			text.y = yres / 4 + 80 + (menuOptions.at(menuIndex).second - 1) * 24;
			menuOptionSize = std::max(static_cast<Uint32>(menuOptions.at(menuIndex).first.size()), static_cast<Uint32>(4));

			//"Settings" button.
			if ( ((omousex >= text.x && omousex < text.x + menuOptionSize * text.w && omousey >= text.y && omousey < text.y + text.h) || (menuselect == menuOptions.at(menuIndex).second)) && subwindow == 0 && introstage == 1 )
			{
				menuselect = menuOptions.at(menuIndex).second;
				ttfPrintTextFormattedColor(ttf16, text.x, text.y, colorGray, "%s", menuOptions.at(menuIndex).first.c_str());
				if ( mainMenuSelectInputIsPressed )
				{
					pauseMenuOnInputPressed();
					openSettingsWindow();
				}
			}
			else
			{
				ttfPrintText(ttf16, text.x, text.y, menuOptions.at(menuIndex).first.c_str());
			}

			++menuIndex;
			text.y = yres / 4 + 80 + (menuOptions.at(menuIndex).second - 1) * 24;
			menuOptionSize = std::max(static_cast<Uint32>(menuOptions.at(menuIndex).first.size()), static_cast<Uint32>(4));

			//"Credits" button
			if ( ((omousex >= text.x && omousex < text.x + menuOptionSize * text.w && omousey >= text.y && omousey < text.y + text.h) || (menuselect == menuOptions.at(menuIndex).second)) && subwindow == 0 && introstage == 1 )
			{
				menuselect = menuOptions.at(menuIndex).second;
				ttfPrintTextFormattedColor(ttf16, text.x, text.y, colorGray, "%s", menuOptions.at(menuIndex).first.c_str());
				if ( mainMenuSelectInputIsPressed )
				{
					pauseMenuOnInputPressed();
					introstage = 4; // goes to credits
					fadeout = true;
				}
			}
			else
			{
				ttfPrintText(ttf16, text.x, text.y, menuOptions.at(menuIndex).first.c_str());
			}

			++menuIndex;
			text.y = yres / 4 + 80 + (menuOptions.at(menuIndex).second - 1) * 24;
			menuOptionSize = std::max(static_cast<Uint32>(menuOptions.at(menuIndex).first.size()), static_cast<Uint32>(4));
			
			//"Custom content" button.
			if ( ((omousex >= text.x && omousex < text.x + menuOptionSize * text.w && omousey >= text.y && omousey < text.y + text.h) || (menuselect == menuOptions.at(menuIndex).second)) && subwindow == 0 && introstage == 1 )
			{
				menuselect = menuOptions.at(menuIndex).second;
				ttfPrintTextFormattedColor(ttf16, text.x, text.y, colorGray, "%s", menuOptions.at(menuIndex).first.c_str());
				if ( mainMenuSelectInputIsPressed )
				{
					pauseMenuOnInputPressed();
					gamemodsCustomContentInit();
				}
			}
			else
			{
				ttfPrintText(ttf16, text.x, text.y, menuOptions.at(menuIndex).first.c_str());
			}
#ifdef STEAMWORKS
			++menuIndex;
			text.y = yres / 4 + 80 + (menuOptions.at(menuIndex).second - 1) * 24;
			menuOptionSize = std::max(static_cast<Uint32>(menuOptions.at(menuIndex).first.size()), static_cast<Uint32>(4));

			if ( ((omousex >= text.x && omousex < text.x + menuOptionSize * text.w && omousey >= text.y && omousey < text.y + text.h) || (menuselect == menuOptions.at(menuIndex).second)) && subwindow == 0 && introstage == 1 )
			{
				menuselect = menuOptions.at(menuIndex).second;
				ttfPrintTextFormattedColor(ttf16, text.x, text.y, colorGray, "%s", menuOptions.at(menuIndex).first.c_str());
				if ( mainMenuSelectInputIsPressed )
				{
					pauseMenuOnInputPressed();
					gamemodsSubscribedItemsInit();
				}
			}
			else
			{
				ttfPrintText(ttf16, text.x, text.y, menuOptions.at(menuIndex).first.c_str());
			}
#endif
			++menuIndex;
			text.y = yres / 4 + 80 + (menuOptions.at(menuIndex).second - 1) * 24;
			menuOptionSize = std::max(static_cast<Uint32>(menuOptions.at(menuIndex).first.size()), static_cast<Uint32>(4));

			//"Quit" button.
			if ( ((omousex >= text.x && omousex < text.x + menuOptionSize * text.w && omousey >= text.y && omousey < text.y + text.h) || (menuselect == menuOptions.at(menuIndex).second)) && subwindow == 0 && introstage == 1 )
			{
				menuselect = menuOptions.at(menuIndex).second;
				ttfPrintTextFormattedColor(ttf16, text.x, text.y, colorGray, "%s", menuOptions.at(menuIndex).first.c_str());
				if ( mainMenuSelectInputIsPressed )
				{
					pauseMenuOnInputPressed();

					// create confirmation window
					subwindow = 1;
					subx1 = xres / 2 - 128;
					subx2 = xres / 2 + 128;
					suby1 = yres / 2 - 40;
					suby2 = yres / 2 + 40;
					strcpy(subtext, language[1128]);

					// close button
					button = newButton();
					strcpy(button->label, "x");
					button->x = subx2 - 20;
					button->y = suby1;
					button->sizex = 20;
					button->sizey = 20;
					button->action = &buttonCloseSubwindow;
					button->visible = 1;
					button->focused = 1;
					button->key = SDL_SCANCODE_ESCAPE;
					button->joykey = joyimpulses[INJOY_MENU_CANCEL];

					// yes button
					button = newButton();
					strcpy(button->label, language[1314]);
					button->x = subx1 + 8;
					button->y = suby2 - 28;
					button->sizex = strlen(language[1314]) * 12 + 8;
					button->sizey = 20;
					button->action = &buttonQuitConfirm;
					button->visible = 1;
					button->focused = 1;
					button->key = SDL_SCANCODE_RETURN;
					button->joykey = joyimpulses[INJOY_MENU_NEXT];

					// no button
					button = newButton();
					strcpy(button->label, language[1315]);
					button->x = subx2 - strlen(language[1315]) * 12 - 16;
					button->y = suby2 - 28;
					button->sizex = strlen(language[1315]) * 12 + 8;
					button->sizey = 20;
					button->action = &buttonCloseSubwindow;
					button->visible = 1;
					button->focused = 1;
				}
			}
			else
			{
				ttfPrintText(ttf16, text.x, text.y, menuOptions.at(menuIndex).first.c_str());
			}
		}
		else
		{
			if ( introstage != 5 )
			{
				if ( gameModeManager.getMode() == GameModeManager_t::GAME_MODE_DEFAULT )
				{
					handleInGamePauseMenu();
				}
				else if ( gameModeManager.getMode() == GameModeManager_t::GAME_MODE_TUTORIAL )
				{
					handleTutorialPauseMenu();
				}
			}
		}

		LobbyHandler.handleLobbyListRequests();

		//Confirm Resolution Change Window
		if ( confirmResolutionWindow )
		{
			subx1 = xres / 2 - 128;
			subx2 = xres / 2 + 128;
			suby1 = yres / 2 - 40;
			suby2 = yres / 2 + 40;
			drawWindowFancy(subx1, suby1, subx2, suby2);

			if ( SDL_GetTicks() >= resolutionConfirmationTimer + RESOLUTION_CONFIRMATION_TIME )
			{
				//Automatically revert.
				buttonRevertResolution(revertResolutionButton);
			}
		}

		// draw subwindow
		if ( subwindow )
		{
			drawWindowFancy(subx1, suby1, subx2, suby2);
			if ( loadGameSaveShowRectangle > 0 )
			{
				SDL_Rect saveBox;
				saveBox.x = subx1 + 4;
				saveBox.y = suby1 + TTF12_HEIGHT * 2;
				saveBox.w = subx2 - subx1 - 8;
				saveBox.h = TTF12_HEIGHT * 3;
				drawWindowFancy(saveBox.x, saveBox.y, saveBox.x + saveBox.w, saveBox.y + saveBox.h);
				if ( gamemods_numCurrentModsLoaded >= 0 )
				{
					drawRect(&saveBox, uint32ColorGreen(*mainsurface), 32);
				}
				else
				{
					drawRect(&saveBox, uint32ColorBaronyBlue(*mainsurface), 32);
				}
				if ( loadGameSaveShowRectangle == 2 )
				{
					saveBox.y = suby1 + TTF12_HEIGHT * 5 + 2;
					//drawTooltip(&saveBox);
					drawWindowFancy(saveBox.x, saveBox.y, saveBox.x + saveBox.w, saveBox.y + saveBox.h);
					if ( gamemods_numCurrentModsLoaded >= 0 )
					{
						drawRect(&saveBox, uint32ColorGreen(*mainsurface), 32);
					}
					else
					{
						drawRect(&saveBox, uint32ColorBaronyBlue(*mainsurface), 32);
					}
				}
			}
			if ( gamemods_window == 1 || gamemods_window == 2 || gamemods_window == 5 )
			{
				drawWindowFancy(subx1 + 4, suby1 + 44 + 10 * TTF12_HEIGHT,
					subx2 - 4, suby2 - 4);
			}
			if ( subtext != NULL )
			{
				if ( strncmp(subtext, language[740], 12) )
				{
					ttfPrintTextFormatted(ttf12, subx1 + 8, suby1 + 8, subtext);
				}
				else
				{
					ttfPrintTextFormatted(ttf16, subx1 + 8, suby1 + 8, subtext);
				}
			}
			if ( loadGameSaveShowRectangle > 0 && gamemods_numCurrentModsLoaded >= 0 )
			{
				ttfPrintTextFormattedColor(ttf12, subx1 + 8, suby2 - TTF12_HEIGHT * 5, uint32ColorBaronyBlue(*mainsurface), "%s", language[2982]);
			}
		}
		else
		{
			loadGameSaveShowRectangle = 0;
		}

		LobbyHandler.drawLobbyFilters();

		// process button actions
		handleButtons();

		LobbyHandler.handleLobbyBrowser();
	}

	// character creation screen
	if ( charcreation_step >= 1 && charcreation_step < 6 )
	{
		if ( gamemods_numCurrentModsLoaded >= 0 )
		{
			ttfPrintText(ttf16, subx1 + 8, suby1 + 8, language[2980]);
		}
		else
		{
			ttfPrintText(ttf16, subx1 + 8, suby1 + 8, language[1318]);
		}

		// draw character window
		if (players[clientnum] != nullptr && players[clientnum]->entity != nullptr)
		{
			camera_charsheet.x = players[clientnum]->entity->x / 16.0 + 1.118 * cos(camera_charsheet_offsetyaw); // + 1
			camera_charsheet.y = players[clientnum]->entity->y / 16.0 + 1.118 * sin(camera_charsheet_offsetyaw); // -.5
			if ( !stats[clientnum]->EFFECTS[EFF_ASLEEP] )
			{
				camera_charsheet.z = players[clientnum]->entity->z * 2;
			}
			else
			{
				camera_charsheet.z = 1.5;
			}
			camera_charsheet.ang = atan2(players[clientnum]->entity->y / 16.0 - camera_charsheet.y, players[clientnum]->entity->x / 16.0 - camera_charsheet.x);
			camera_charsheet.vang = PI / 24;
			camera_charsheet.winw = 360;
			camera_charsheet.winy = suby1 + 32;
			camera_charsheet.winh = suby2 - 96 - camera_charsheet.winy;
			camera_charsheet.winx = subx2 - camera_charsheet.winw - 32;
			SDL_Rect pos;
			pos.x = camera_charsheet.winx;
			pos.y = camera_charsheet.winy;
			pos.w = camera_charsheet.winw;
			pos.h = camera_charsheet.winh;
			drawRect(&pos, 0, 255);
			b = players[clientnum]->entity->flags[BRIGHT];
			players[clientnum]->entity->flags[BRIGHT] = true;
			if (!playing_random_char)
			{
				if ( !players[clientnum]->entity->flags[INVISIBLE] )
				{
					real_t ofov = fov;
					fov = 50;
					glDrawVoxel(&camera_charsheet, players[clientnum]->entity, REALCOLORS);
					fov = ofov;
				}
				players[clientnum]->entity->flags[BRIGHT] = b;
				c = 0;
				for ( node = players[clientnum]->entity->children.first; node != NULL; node = node->next )
				{
					if ( c == 0 )
					{
						c++;
					}
					entity = (Entity*) node->element;
					if ( !entity->flags[INVISIBLE] )
					{
						b = entity->flags[BRIGHT];
						entity->flags[BRIGHT] = true;
						real_t ofov = fov;
						fov = 50;
						glDrawVoxel(&camera_charsheet, entity, REALCOLORS);
						fov = ofov;
						entity->flags[BRIGHT] = b;
					}
					c++;
				}
			}
			SDL_Rect rotateBtn;
			rotateBtn.w = 24;
			rotateBtn.h = 24;
			rotateBtn.x = camera_charsheet.winx + camera_charsheet.winw - rotateBtn.w;
			rotateBtn.y = camera_charsheet.winy + camera_charsheet.winh - rotateBtn.h;
			drawWindow(rotateBtn.x, rotateBtn.y, rotateBtn.x + rotateBtn.w, rotateBtn.y + rotateBtn.h);
			if ( mouseInBounds(clientnum, rotateBtn.x, rotateBtn.x + rotateBtn.w, rotateBtn.y, rotateBtn.y + rotateBtn.h) )
			{
				if ( inputs.bMouseLeft(clientnum) )
				{
					camera_charsheet_offsetyaw += 0.05;
					if ( camera_charsheet_offsetyaw > 2 * PI )
					{
						camera_charsheet_offsetyaw -= 2 * PI;
					}
					drawDepressed(rotateBtn.x, rotateBtn.y, rotateBtn.x + rotateBtn.w, rotateBtn.y + rotateBtn.h);
				}
			}
			ttfPrintText(ttf12, rotateBtn.x + 4, rotateBtn.y + 6, ">");

			rotateBtn.x = camera_charsheet.winx + camera_charsheet.winw - rotateBtn.w * 2 - 4;
			rotateBtn.y = camera_charsheet.winy + camera_charsheet.winh - rotateBtn.h;
			drawWindow(rotateBtn.x, rotateBtn.y, rotateBtn.x + rotateBtn.w, rotateBtn.y + rotateBtn.h);
			if ( mouseInBounds(clientnum, rotateBtn.x, rotateBtn.x + rotateBtn.w, rotateBtn.y, rotateBtn.y + rotateBtn.h) )
			{
				if ( inputs.bMouseLeft(clientnum) )
				{
					camera_charsheet_offsetyaw -= 0.05;
					if ( camera_charsheet_offsetyaw < 0.f )
					{
						camera_charsheet_offsetyaw += 2 * PI;
					}
					drawDepressed(rotateBtn.x, rotateBtn.y, rotateBtn.x + rotateBtn.w, rotateBtn.y + rotateBtn.h);
				}
			}
			ttfPrintText(ttf12, rotateBtn.x + 4, rotateBtn.y + 6, "<");

			SDL_Rect raceInfoBtn;
			raceInfoBtn.y = rotateBtn.y;
			raceInfoBtn.w = longestline(language[3373]) * TTF12_WIDTH + 8 + 4;
			raceInfoBtn.x = rotateBtn.x - raceInfoBtn.w - 4;
			raceInfoBtn.h = rotateBtn.h;
			drawWindow(raceInfoBtn.x, raceInfoBtn.y, raceInfoBtn.x + raceInfoBtn.w, raceInfoBtn.y + raceInfoBtn.h);
			if ( mouseInBounds(clientnum, raceInfoBtn.x, raceInfoBtn.x + raceInfoBtn.w, raceInfoBtn.y, raceInfoBtn.y + raceInfoBtn.h) )
			{
				if ( inputs.bControllerInputPressed(clientnum, INJOY_MENU_LEFT_CLICK) || inputs.bMouseLeft(clientnum) )
				{
					//drawDepressed(raceInfoBtn.x, raceInfoBtn.y, raceInfoBtn.x + raceInfoBtn.w, raceInfoBtn.y + raceInfoBtn.h);
					inputs.mouseClearLeft(clientnum);
					inputs.controllerClearInput(clientnum, INJOY_MENU_LEFT_CLICK);
					showRaceInfo = !showRaceInfo;
					playSound(139, 64);
				}
			}
			if ( showRaceInfo )
			{
				pos.y += 2;
				pos.h -= raceInfoBtn.h + 6;
				pos.x += 2;
				pos.w -= 6;
				drawRect(&pos, 0, 168);
				drawLine(pos.x, pos.y, pos.x + pos.w, pos.y, SDL_MapRGB(mainsurface->format, 0, 192, 255), 255);
				drawLine(pos.x, pos.y + pos.h, pos.x + pos.w, pos.y + pos.h, SDL_MapRGB(mainsurface->format, 0, 192, 255), 255);
				drawLine(pos.x, pos.y, pos.x, pos.y + pos.h, SDL_MapRGB(mainsurface->format, 0, 192, 255), 255);
				drawLine(pos.x + pos.w, pos.y, pos.x + pos.w, pos.y + pos.h, SDL_MapRGB(mainsurface->format, 0, 192, 255), 255);
				if ( stats[0]->playerRace >= RACE_HUMAN )
				{
					ttfPrintText(ttf12, pos.x + 12, pos.y + 6, language[3375 + stats[0]->playerRace]);
				}
				ttfPrintText(ttf12, raceInfoBtn.x + 4, raceInfoBtn.y + 6, language[3374]);
			}
			else
			{
				ttfPrintText(ttf12, raceInfoBtn.x + 4, raceInfoBtn.y + 6, language[3373]);
			}
		}

		// skin DLC check flags.
		bool skipFirstDLC = false;
		bool skipSecondDLC = false;
		if ( enabledDLCPack2 && !enabledDLCPack1 )
		{
			skipFirstDLC = true;
			if ( stats[0]->playerRace > 0 && stats[0]->playerRace <= RACE_GOATMAN )
			{
				stats[0]->playerRace = RACE_HUMAN;
			}
		}
		else if ( enabledDLCPack1 && !enabledDLCPack2 )
		{
			skipSecondDLC = true;
			if ( stats[0]->playerRace > RACE_GOATMAN )
			{
				stats[0]->playerRace = RACE_HUMAN;
			}
		}
		else if ( !enabledDLCPack1 && !enabledDLCPack2 )
		{
			stats[0]->playerRace = RACE_HUMAN;
		}

		// sexes/race
		if ( charcreation_step == 1 )
		{
			ttfPrintText(ttf16, subx1 + 24, suby1 + 32, language[1319]);
			Uint32 colorStep1 = uint32ColorWhite(*mainsurface);
			if ( raceSelect != 0 )
			{
				colorStep1 = uint32ColorGray(*mainsurface);
			}
			if ( stats[0]->sex == 0 )
			{
				ttfPrintTextFormattedColor(ttf16, subx1 + 32, suby1 + 56, colorStep1, "[o] %s", language[1321]);
				ttfPrintTextFormattedColor(ttf16, subx1 + 32, suby1 + 73, colorStep1, "[ ] %s", language[1322]);

				ttfPrintTextFormattedColor(ttf12, subx1 + 8, suby2 - 80, uint32ColorWhite(*mainsurface), language[1320], language[1321]);
			}
			else
			{
				ttfPrintTextFormattedColor(ttf16, subx1 + 32, suby1 + 56, colorStep1, "[ ] %s", language[1321]);
				ttfPrintTextFormattedColor(ttf16, subx1 + 32, suby1 + 73, colorStep1, "[o] %s", language[1322]);

				ttfPrintTextFormattedColor(ttf12, subx1 + 8, suby2 - 80, uint32ColorWhite(*mainsurface), language[1320], language[1322]);
			}
			ttfPrintTextFormattedColor(ttf12, subx1 + 8, suby2 - 56, uint32ColorWhite(*mainsurface), language[3175]);

			// race
			if ( raceSelect != 1 )
			{
				colorStep1 = uint32ColorGray(*mainsurface);
			}
			else if ( raceSelect == 1 )
			{
				colorStep1 = uint32ColorWhite(*mainsurface);
			}
			ttfPrintText(ttf16, subx1 + 24, suby1 + 108, language[3160]);
			int pady = suby1 + 108 + 24;
			bool isLocked = false;
			for ( int c = 0; c < NUMPLAYABLERACES; )
			{
				if ( raceSelect == 1 )
				{
					if ( skipSecondDLC )
					{
						if ( c > RACE_GOATMAN )
						{
							colorStep1 = uint32ColorGray(*mainsurface);
						}
						else
						{
							colorStep1 = uint32ColorWhite(*mainsurface);
						}
					}
					else if ( skipFirstDLC )
					{
						if ( c > RACE_HUMAN && c <= RACE_GOATMAN )
						{
							colorStep1 = uint32ColorGray(*mainsurface);
						}
						else
						{
							colorStep1 = uint32ColorWhite(*mainsurface);
						}
					}
					else if ( !(enabledDLCPack2 && enabledDLCPack1) )
					{
						if ( c > RACE_HUMAN )
						{
							colorStep1 = uint32ColorGray(*mainsurface);
						}
						else
						{
							colorStep1 = uint32ColorWhite(*mainsurface);
						}
					}
					else if ( enabledDLCPack2 && enabledDLCPack1 )
					{
						colorStep1 = uint32ColorWhite(*mainsurface);
					}
				}
				if ( stats[0]->playerRace == c )
				{
					ttfPrintTextFormattedColor(ttf16, subx1 + 32, pady, colorStep1, "[o] %s", language[3161 + c]);
				}
				else
				{
					if ( skipSecondDLC )
					{
						if ( c > RACE_GOATMAN )
						{
							isLocked = true;
						}
					}
					else if ( skipFirstDLC )
					{
						if ( c >= RACE_SKELETON && c < RACE_AUTOMATON )
						{
							isLocked = true;
						}
					}
					else if ( !enabledDLCPack1 && !enabledDLCPack2 )
					{
						isLocked = true;
					}
					if ( isLocked )
					{
						SDL_Rect img;
						img.x = subx1 + 32 + 10;
						img.y = pady - 2;
						img.w = 22;
						img.h = 20;
						drawImageScaled(sidebar_unlock_bmp, nullptr, &img);
						ttfPrintTextFormattedColor(ttf16, subx1 + 32, pady, colorStep1, "[ ] %s", language[3161 + c]);
					}
					else
					{
						ttfPrintTextFormattedColor(ttf16, subx1 + 32, pady, colorStep1, "[ ] %s", language[3161 + c]);
					}
				}

				if ( skipFirstDLC )
				{
					if ( c == RACE_HUMAN )
					{
						c = RACE_AUTOMATON;
					}
					else if ( c == RACE_INSECTOID )
					{
						c = RACE_SKELETON;
						pady += 8;
					}
					else if ( c == RACE_GOATMAN )
					{
						c = NUMPLAYABLERACES;
					}
					else
					{
						++c;
					}
				}
				else
				{
					if ( skipSecondDLC && c == RACE_GOATMAN )
					{
						pady += 8;
					}
					else if ( !enabledDLCPack1 && !enabledDLCPack2 && c == RACE_HUMAN )
					{
						pady += 8;
					}
					++c;
				}
				pady += 17;
			}

			pady += 24;
			if ( isLocked )
			{
				pady -= 8;
			}
			bool displayRaceOptions = false;
			if ( raceSelect != 2 )
			{
				colorStep1 = uint32ColorGray(*mainsurface);
			}
			else
			{
				colorStep1 = uint32ColorWhite(*mainsurface);
			}
			if ( stats[0]->playerRace > 0 )
			{
				displayRaceOptions = true;
				ttfPrintText(ttf16, subx1 + 24, pady, language[3176]);
				pady += 24;
				char raceOptionBuffer[128];
				snprintf(raceOptionBuffer, 63, language[3177], language[3161 + stats[0]->playerRace]);
				if ( stats[0]->appearance > 1 )
				{
					stats[0]->appearance = lastAppearance;
				}
				if ( stats[0]->appearance == 0 )
				{
					ttfPrintTextFormattedColor(ttf16, subx1 + 32, pady, colorStep1, "[o] %s", raceOptionBuffer);
					ttfPrintTextFormattedColor(ttf16, subx1 + 32, pady + 17, colorStep1, "[ ] %s", language[3178]);
				}
				else if ( stats[0]->appearance == 1 )
				{
					ttfPrintTextFormattedColor(ttf16, subx1 + 32, pady, colorStep1, "[ ] %s", raceOptionBuffer);
					ttfPrintTextFormattedColor(ttf16, subx1 + 32, pady + 17, colorStep1, "[o] %s", language[3178]);
				}

			}

			pady = suby1 + 108 + 24;
			lastRace = static_cast<PlayerRaces>(stats[0]->playerRace);
			if ( omousex >= subx1 + 40 && omousex < subx1 + 72 )
			{
				if ( omousey >= suby1 + 56 && omousey < suby1 + 72 )
				{
					if ( inputs.bMouseLeft(clientnum) )
					{
						raceSelect = 0;
						inputs.mouseClearLeft(clientnum);
						stats[0]->sex = MALE;
						lastSex = MALE;
						if ( stats[0]->playerRace == RACE_SUCCUBUS )
						{
							if ( enabledDLCPack2 )
							{
								stats[0]->playerRace = RACE_INCUBUS;
								if ( client_classes[0] == CLASS_MESMER && stats[0]->appearance == 0 )
								{
									if ( isCharacterValidFromDLC(*stats[0], client_classes[0]) != VALID_OK_CHARACTER )
									{
										client_classes[0] = CLASS_PUNISHER;
									}
									stats[0]->clearStats();
									initClass(0);
								}
							}
							else
							{
								stats[0]->playerRace = RACE_HUMAN;
							}
						}
					}
				}
				else if ( omousey >= suby1 + 72 && omousey < suby1 + 90 )
				{
					if ( inputs.bMouseLeft(clientnum) )
					{
						raceSelect = 0;
						inputs.mouseClearLeft(clientnum);
						stats[0]->sex = FEMALE;
						lastSex = FEMALE;
						if ( stats[0]->playerRace == RACE_INCUBUS )
						{
							if ( enabledDLCPack1 )
							{
								stats[0]->playerRace = RACE_SUCCUBUS;
								if ( client_classes[0] == CLASS_PUNISHER && stats[0]->appearance == 0 )
								{
									if ( isCharacterValidFromDLC(*stats[0], client_classes[0]) != VALID_OK_CHARACTER )
									{
										client_classes[0] = CLASS_MESMER;
									}
									stats[0]->clearStats();
									initClass(0);
								}
							}
							else
							{
								stats[0]->playerRace = RACE_HUMAN;
							}
						}
					}
				}
				else if ( omousey >= pady && omousey < pady + NUMPLAYABLERACES * 17 + (isLocked ? 8 : 0) )
				{
					for ( c = 0; c < NUMPLAYABLERACES; ++c )
					{
						if ( omousey >= pady && omousey < pady + 17 )
						{
							bool disableSelect = false;
							if ( skipSecondDLC )
							{
								if ( c > RACE_GOATMAN )
								{
									disableSelect = true;
								}
							}
							else if ( skipFirstDLC )
							{
								if ( c > RACE_GOATMAN && c <= RACE_INSECTOID ) // this is weird cause we're reordering the menu above...
								{
									disableSelect = true;
								}
							}
							else if ( !enabledDLCPack1 && !enabledDLCPack2 )
							{
								if ( c != RACE_HUMAN )
								{
									disableSelect = true;
								}
							}
							if ( !disableSelect && inputs.bMouseLeft(clientnum) )
							{
								raceSelect = 1;
								inputs.mouseClearLeft(clientnum);
								if ( !disableSelect )
								{
									PlayerRaces lastRace = static_cast<PlayerRaces>(stats[0]->playerRace);
									if ( skipFirstDLC )
									{
										// this is weird cause we're reordering the menu above...
										if ( c > RACE_GOATMAN )
										{
											stats[0]->playerRace = c - 4;
										}
										else if ( c > RACE_HUMAN )
										{
											stats[0]->playerRace = c + 4;
										}
										else
										{
											stats[0]->playerRace = c;
										}
									}
									else
									{
										stats[0]->playerRace = c;
									}
									inputs.mouseClearLeft(clientnum);
									if ( stats[0]->playerRace == RACE_INCUBUS )
									{
										stats[0]->sex = MALE;
									}
									else if ( stats[0]->playerRace == RACE_SUCCUBUS )
									{
										stats[0]->sex = FEMALE;
									}
									else if ( lastRace == RACE_SUCCUBUS || lastRace == RACE_INCUBUS )
									{
										stats[0]->sex = lastSex;
									}
									// convert human class to monster special classes on reselect.
									if ( stats[0]->playerRace != RACE_HUMAN && lastRace != RACE_HUMAN && client_classes[0] > CLASS_MONK
										&& stats[0]->appearance == 0 )
									{
										if ( isCharacterValidFromDLC(*stats[0], client_classes[0]) != VALID_OK_CHARACTER )
										{
											client_classes[0] = CLASS_MONK + stats[0]->playerRace;
										}
										stats[0]->clearStats();
										initClass(0);
									}
									else if ( stats[0]->playerRace != RACE_HUMAN && lastRace == RACE_HUMAN && client_classes[0] > CLASS_MONK
										&& lastAppearance == 0 )
									{
										if ( isCharacterValidFromDLC(*stats[0], client_classes[0]) != VALID_OK_CHARACTER )
										{
											client_classes[0] = CLASS_MONK + stats[0]->playerRace;
										}
										stats[0]->clearStats();
										initClass(0);
									}
									else if ( stats[0]->playerRace != RACE_GOATMAN && lastRace == RACE_GOATMAN )
									{
										stats[0]->clearStats();
										initClass(0);
									}
									// appearance reset.
									if ( stats[0]->playerRace == RACE_HUMAN && lastRace != RACE_HUMAN )
									{
										stats[0]->appearance = rand() % NUMAPPEARANCES;
									}
									else if ( stats[0]->playerRace != RACE_HUMAN && lastRace == RACE_HUMAN )
									{
										stats[0]->appearance = lastAppearance;
									}
								}
								break;
							}
							else if ( disableSelect )
							{
								SDL_Rect tooltip;
								tooltip.x = omousex + 16;
								tooltip.y = omousey + 16;
								tooltip.h = TTF12_HEIGHT + 8;
#if (defined STEAMWORKS || defined USE_EOS)
								if ( c > RACE_GOATMAN && c <= RACE_INSECTOID && !skipFirstDLC )
								{
									tooltip.h = TTF12_HEIGHT * 2 + 8;
									tooltip.w = longestline(language[3917]) * TTF12_WIDTH + 8;
									drawTooltip(&tooltip);
									ttfPrintTextFormattedColor(ttf12, tooltip.x + 4, tooltip.y + 6, uint32ColorOrange(*mainsurface), language[3917]);
								}
								else
								{
									tooltip.h = TTF12_HEIGHT * 2 + 8;
									tooltip.w = longestline(language[3200]) * TTF12_WIDTH + 8;
									drawTooltip(&tooltip);
									ttfPrintTextFormattedColor(ttf12, tooltip.x + 4, tooltip.y + 6, uint32ColorOrange(*mainsurface), language[3200]);
								}
#ifdef STEAMWORKS
								if ( SteamUser()->BLoggedOn() )
								{
									if ( inputs.bMouseLeft(clientnum) )
									{
										if ( SteamUtils()->IsOverlayEnabled() )
										{
											SteamFriends()->ActivateGameOverlayToStore(STEAM_APPID, k_EOverlayToStoreFlag_None);
										}
										else
										{
											if ( c > RACE_GOATMAN && c <= RACE_INSECTOID && !skipFirstDLC )
											{
												openURLTryWithOverlay(language[3993]);
											}
											else
											{
												openURLTryWithOverlay(language[3992]);
											}
										}
										inputs.mouseClearLeft(clientnum);
									}
								}
#elif defined USE_EOS
								if ( c > RACE_GOATMAN && c <= RACE_INSECTOID && !skipFirstDLC )
								{
									if ( inputs.bMouseLeft(clientnum) )
									{
										openURLTryWithOverlay(language[3985]);
										inputs.mouseClearLeft(clientnum);
									}
								}
								else
								{
									if ( inputs.bMouseLeft(clientnum) )
									{
										openURLTryWithOverlay(language[3984]);
										inputs.mouseClearLeft(clientnum);
									}
								}
#endif
#else
								if ( c > RACE_GOATMAN && c <= RACE_INSECTOID )
								{
									tooltip.w = longestline(language[3372]) * TTF12_WIDTH + 8;
									drawTooltip(&tooltip);
									ttfPrintTextFormattedColor(ttf12, tooltip.x + 4, tooltip.y + 6, uint32ColorOrange(*mainsurface), language[3372]);
								}
								else
								{
									tooltip.w = longestline(language[3199]) * TTF12_WIDTH + 8;
									drawTooltip(&tooltip);
									ttfPrintTextFormattedColor(ttf12, tooltip.x + 4, tooltip.y + 6, uint32ColorOrange(*mainsurface), language[3199]);
								}
#endif // STEAMWORKS
							}
						}
						pady += 17;
						if ( isLocked )
						{
							if ( skipFirstDLC && c == RACE_INSECTOID )
							{
								pady += 8;
							}
							else
							{
								if ( skipSecondDLC && c == RACE_GOATMAN )
								{
									pady += 8;
								}
								else if ( !enabledDLCPack1 && !enabledDLCPack2 && c == RACE_HUMAN )
								{
									pady += 8;
								}
							}
						}
					}
				}
				else if ( omousey >= pady + (NUMPLAYABLERACES * 17) + 48 && omousey < pady + (NUMPLAYABLERACES * 17) + 82 )
				{
					if ( inputs.bMouseLeft(clientnum) )
					{
						inputs.mouseClearLeft(clientnum);
						if ( stats[0]->playerRace > 0 )
						{
							if ( omousey < pady + (NUMPLAYABLERACES * 17) + 64 ) // first option
							{
								if ( stats[0]->appearance != 0 )
								{
									stats[0]->appearance = 0; // use racial passives
									// convert human class to monster special classes on reselect.
									if ( client_classes[0] > CLASS_MONK )
									{
										if ( isCharacterValidFromDLC(*stats[0], client_classes[0]) != VALID_OK_CHARACTER )
										{
											client_classes[0] = CLASS_MONK + stats[0]->playerRace;
										}
									}
								}
							}
							else
							{
								stats[0]->appearance = 1; // act as human
							}
							lastAppearance = stats[0]->appearance;
							stats[0]->clearStats();
							initClass(0);
							raceSelect = 2;
							inputs.mouseClearLeft(clientnum);
						}
					}
				}
			}
			if ( keystatus[SDL_SCANCODE_UP] || (inputs.bControllerInputPressed(clientnum, INJOY_DPAD_UP) && rebindaction == -1) )
			{
				keystatus[SDL_SCANCODE_UP] = 0;
				if ( rebindaction == -1 )
				{
					inputs.controllerClearInput(clientnum, INJOY_DPAD_UP);
				}
				inputs.hideMouseCursors();
				if ( raceSelect == 1 )
				{
					PlayerRaces lastRace = static_cast<PlayerRaces>(stats[0]->playerRace);
					if ( !enabledDLCPack1 && !enabledDLCPack2 )
					{
						// do nothing.
						stats[0]->playerRace = RACE_HUMAN;
					}
					else if ( stats[0]->playerRace <= 0 )
					{
						if ( skipSecondDLC )
						{
							stats[0]->playerRace = NUMPLAYABLERACES - 5;
						}
						else if ( enabledDLCPack2 )
						{
							stats[0]->playerRace = NUMPLAYABLERACES - 1;
						}
					}
					else
					{
						if ( skipFirstDLC && stats[0]->playerRace == RACE_AUTOMATON )
						{
							stats[0]->playerRace = RACE_HUMAN;
						}
						else
						{
							--stats[0]->playerRace;
						}
					}
					if ( stats[0]->playerRace == RACE_INCUBUS )
					{
						stats[0]->sex = MALE;
					}
					else if ( stats[0]->playerRace == RACE_SUCCUBUS )
					{
						stats[0]->sex = FEMALE;
					}
					else if ( lastRace == RACE_SUCCUBUS || lastRace == RACE_INCUBUS )
					{
						stats[0]->sex = lastSex;
					}
					// convert human class to monster special classes on reselect.
					if ( stats[0]->playerRace != RACE_HUMAN && lastRace != RACE_HUMAN && client_classes[0] > CLASS_MONK
						&& stats[0]->appearance == 0 )
					{
						if ( isCharacterValidFromDLC(*stats[0], client_classes[0]) != VALID_OK_CHARACTER )
						{
							client_classes[0] = CLASS_MONK + stats[0]->playerRace;
						}
						stats[0]->clearStats();
						initClass(0);
					}
					else if ( stats[0]->playerRace != RACE_HUMAN && lastRace == RACE_HUMAN && client_classes[0] > CLASS_MONK
						&& lastAppearance == 0 )
					{
						if ( isCharacterValidFromDLC(*stats[0], client_classes[0]) != VALID_OK_CHARACTER )
						{
							client_classes[0] = CLASS_MONK + stats[0]->playerRace;
						}
						stats[0]->clearStats();
						initClass(0);
					}
					else if ( stats[0]->playerRace != RACE_GOATMAN && lastRace == RACE_GOATMAN )
					{
						stats[0]->clearStats();
						initClass(0);
					}
					// appearance reset.
					if ( stats[0]->playerRace == RACE_HUMAN && lastRace != RACE_HUMAN )
					{
						stats[0]->appearance = rand() % NUMAPPEARANCES;
					}
					else if ( stats[0]->playerRace != RACE_HUMAN && lastRace == RACE_HUMAN )
					{
						stats[0]->appearance = lastAppearance;
					}
				}
				else if ( raceSelect == 0 )
				{
					stats[0]->sex = static_cast<sex_t>((stats[0]->sex == MALE));
					lastSex = stats[0]->sex;
					if ( stats[0]->playerRace == RACE_INCUBUS )
					{
						stats[0]->sex = MALE;
					}
					else if ( stats[0]->playerRace == RACE_SUCCUBUS )
					{
						stats[0]->sex = FEMALE;
					}
				}
				else if ( raceSelect == 2 )
				{
					if ( stats[0]->appearance != 0 )
					{
						stats[0]->appearance = 0;
						// convert human class to monster special classes on reselect.
						if ( stats[0]->playerRace != RACE_HUMAN && client_classes[0] > CLASS_MONK )
						{
							if ( isCharacterValidFromDLC(*stats[0], client_classes[0]) != VALID_OK_CHARACTER )
							{
								client_classes[0] = CLASS_MONK + stats[0]->playerRace;
							}
							stats[0]->clearStats();
							initClass(0);
						}
					}
					else
					{
						stats[0]->appearance = 1;
					}
					lastAppearance = stats[0]->appearance;
					stats[0]->clearStats();
					initClass(0);
				}
			}
			if ( keystatus[SDL_SCANCODE_DOWN] || (inputs.bControllerInputPressed(clientnum, INJOY_DPAD_DOWN) && rebindaction == -1) )
			{
				keystatus[SDL_SCANCODE_DOWN] = 0;
				if ( rebindaction == -1 )
				{
					inputs.controllerClearInput(clientnum, INJOY_DPAD_DOWN);
				}
				inputs.hideMouseCursors();
				if ( raceSelect == 1 )
				{
					PlayerRaces lastRace = static_cast<PlayerRaces>(stats[0]->playerRace);
					if ( !enabledDLCPack1 && !enabledDLCPack2 )
					{
						// do nothing.
						stats[0]->playerRace = RACE_HUMAN;
					}
					else if ( skipSecondDLC )
					{
						if ( stats[0]->playerRace >= NUMPLAYABLERACES - 5 )
						{
							stats[0]->playerRace = RACE_HUMAN;
						}
						else
						{
							++stats[0]->playerRace;
						}
					}
					else if ( skipFirstDLC )
					{
						if ( stats[0]->playerRace >= RACE_GOATMAN && stats[0]->playerRace < NUMPLAYABLERACES - 1 )
						{
							++stats[0]->playerRace;
						}
						else if ( stats[0]->playerRace == RACE_HUMAN )
						{
							stats[0]->playerRace = RACE_AUTOMATON;
						}
						else if ( stats[0]->playerRace == NUMPLAYABLERACES - 1 )
						{
							stats[0]->playerRace = RACE_HUMAN;
						}
					}
					else
					{
						if ( stats[0]->playerRace >= NUMPLAYABLERACES - 1 )
						{
							stats[0]->playerRace = RACE_HUMAN;
						}
						else
						{
							++stats[0]->playerRace;
						}
					}
					if ( stats[0]->playerRace == RACE_INCUBUS )
					{
						stats[0]->sex = MALE;
					}
					else if ( stats[0]->playerRace == RACE_SUCCUBUS )
					{
						stats[0]->sex = FEMALE;
					}
					else if ( lastRace == RACE_SUCCUBUS || lastRace == RACE_INCUBUS )
					{
						stats[0]->sex = lastSex;
					}
					// convert human class to monster special classes on reselect.
					if ( stats[0]->playerRace != RACE_HUMAN && lastRace != RACE_HUMAN && client_classes[0] > CLASS_MONK
						&& stats[0]->appearance == 0 )
					{
						if ( isCharacterValidFromDLC(*stats[0], client_classes[0]) != VALID_OK_CHARACTER )
						{
							client_classes[0] = CLASS_MONK + stats[0]->playerRace;
						}
						stats[0]->clearStats();
						initClass(0);
					}
					else if ( stats[0]->playerRace != RACE_HUMAN && lastRace == RACE_HUMAN && client_classes[0] > CLASS_MONK 
						&& lastAppearance == 0 )
					{
						if ( isCharacterValidFromDLC(*stats[0], client_classes[0]) != VALID_OK_CHARACTER )
						{
							client_classes[0] = CLASS_MONK + stats[0]->playerRace;
						}
						stats[0]->clearStats();
						initClass(0);
					}
					else if ( stats[0]->playerRace != RACE_GOATMAN && lastRace == RACE_GOATMAN )
					{
						stats[0]->clearStats();
						initClass(0);
					}
					// appearance reset.
					if ( stats[0]->playerRace == RACE_HUMAN && lastRace != RACE_HUMAN )
					{
						stats[0]->appearance = rand() % NUMAPPEARANCES;
					}
					else if ( stats[0]->playerRace != RACE_HUMAN && lastRace == RACE_HUMAN )
					{
						stats[0]->appearance = lastAppearance;
					}
				}
				else if ( raceSelect == 0 )
				{
					stats[0]->sex = static_cast<sex_t>((stats[0]->sex == MALE));
					lastSex = stats[0]->sex;
					if ( stats[0]->playerRace == RACE_INCUBUS )
					{
						stats[0]->sex = MALE;
					}
					else if ( stats[0]->playerRace == RACE_SUCCUBUS )
					{
						stats[0]->sex = FEMALE;
					}
				}
				else if ( raceSelect == 2 )
				{
					if ( stats[0]->appearance != 0 )
					{
						stats[0]->appearance = 0;
						// convert human class to monster special classes on reselect.
						if ( stats[0]->playerRace != RACE_HUMAN && client_classes[0] > CLASS_MONK )
						{
							if ( isCharacterValidFromDLC(*stats[0], client_classes[0]) != VALID_OK_CHARACTER )
							{
								client_classes[0] = CLASS_MONK + stats[0]->playerRace;
							}
							stats[0]->clearStats();
							initClass(0);
						}
					}
					else
					{
						stats[0]->appearance = 1;
					}
					lastAppearance = stats[0]->appearance;
					stats[0]->clearStats();
					initClass(0);
				}
			}
			if ( keystatus[SDL_SCANCODE_RIGHT] || (inputs.bControllerInputPressed(clientnum, INJOY_DPAD_RIGHT) && rebindaction == -1) )
			{
				keystatus[SDL_SCANCODE_RIGHT] = 0;
				if ( rebindaction == -1 )
				{
					inputs.controllerClearInput(clientnum, INJOY_DPAD_RIGHT);
				}
				inputs.hideMouseCursors();
				++raceSelect;
				if ( stats[0]->playerRace == RACE_HUMAN )
				{
					if ( raceSelect > 1 )
					{
						raceSelect = 0;
					}
				}
				else if ( raceSelect > 2 )
				{
					raceSelect = 0;
				}
			}
			if ( keystatus[SDL_SCANCODE_LEFT] || (inputs.bControllerInputPressed(clientnum, INJOY_DPAD_LEFT) && rebindaction == -1) )
			{
				keystatus[SDL_SCANCODE_LEFT] = 0;
				if ( rebindaction == -1 )
				{
					inputs.controllerClearInput(clientnum, INJOY_DPAD_LEFT);
				}
				inputs.hideMouseCursors();
				--raceSelect;
				if ( stats[0]->playerRace == RACE_HUMAN )
				{
					if ( raceSelect < 0 )
					{
						raceSelect = 1;
					}
				}
				else if ( raceSelect < 0 )
				{
					raceSelect = 2;
				}
			}
			if ( lastRace != RACE_GOATMAN && stats[0]->playerRace == RACE_GOATMAN && stats[0]->appearance == 0 )
			{
				stats[0]->clearStats();
				initClass(0);
			}
		}

		// classes
		else if ( charcreation_step == 2 )
		{
			ttfPrintText(ttf16, subx1 + 24, suby1 + 32, language[1323]);
			int entriesToDisplay = NUMCLASSES;
			if ( enabledDLCPack1 && enabledDLCPack2 )
			{
				entriesToDisplay = NUMCLASSES;
			}
			else if ( enabledDLCPack1 || enabledDLCPack2 )
			{
				entriesToDisplay = NUMCLASSES - 4;
			}
			else
			{
				entriesToDisplay = CLASS_MONK + 1;
			}

			std::set<int> availableClasses;
			std::set<int> lockedClasses;
			std::vector<int> displayedClasses;
			for ( c = 0; c < NUMCLASSES; ++c )
			{
				int result = isCharacterValidFromDLC(*stats[0], c);
				if ( result == VALID_OK_CHARACTER )
				{
					availableClasses.insert(c);
					displayedClasses.push_back(c);
				}
				else if ( result == INVALID_REQUIRE_ACHIEVEMENT )
				{
					lockedClasses.insert(c);
				}
			}

			for ( auto it = lockedClasses.begin(); it != lockedClasses.end(); ++it )
			{
				displayedClasses.push_back(*it);
			}

			int drawLockedTooltip = 0;
			SDL_Rect tooltip;
			for ( c = 0; c < entriesToDisplay; c++ )
			{
				int classToPick = displayedClasses.at(c);
				int pady = suby1 + 56 + 16 * c;
				if ( lockedClasses.find(classToPick) != lockedClasses.end() )
				{
					pady += 8;
				}

				if ( inputs.bMouseLeft(clientnum) )
				{
					if ( omousex >= subx1 + 40 && omousex < subx1 + 72 )
					{
						if ( omousey >= pady && omousey < pady + 16 )
						{
							inputs.mouseClearLeft(clientnum);
							if ( isCharacterValidFromDLC(*stats[0], classToPick) == VALID_OK_CHARACTER )
							{
								int previousClassPicked = client_classes[0];
								client_classes[0] = classToPick;
								if ( previousClassPicked != client_classes[0] )
								{
									// reset class loadout
									stats[0]->clearStats();
									initClass(0);
								}
							}
						}
					}
				}

				bool classLocked = false;

				if ( classToPick == client_classes[0] )
				{
					ttfPrintTextFormatted(ttf16, subx1 + 32, pady, "[o] %s", playerClassLangEntry(classToPick, 0));
				}
				else
				{
					if ( lockedClasses.find(classToPick) != lockedClasses.end() )
					{
						classLocked = true;
						SDL_Rect img;
						img.x = subx1 + 32 + 10;
						img.y = pady - 2;
						img.w = 22;
						img.h = 20;
						drawImageScaled(sidebar_unlock_bmp, nullptr, &img);
						ttfPrintTextFormattedColor(ttf16, subx1 + 32, pady, uint32ColorGray(*mainsurface), "[ ] %s", playerClassLangEntry(classToPick, 0));

						if ( mouseInBounds(clientnum, subx1 + 40, subx1 + 72, pady, pady + 16) )
						{
#if (defined STEAMWORKS || defined USE_EOS)
							tooltip.x = omousex + 16;
							tooltip.y = omousey + 16;
							tooltip.h = TTF12_HEIGHT + 8;
							if ( classToPick > CLASS_MONK )
							{
								int langline = 3927 + classToPick - CLASS_CONJURER;
								tooltip.w = longestline(language[langline]) * TTF12_WIDTH + 8;
								drawLockedTooltip = langline;
							}
#endif
						}
					}
					else
					{
						ttfPrintTextFormatted(ttf16, subx1 + 32, pady, "[ ] %s", playerClassLangEntry(classToPick, 0));
					}
				}

				if ( keystatus[SDL_SCANCODE_UP] || (inputs.bControllerInputPressed(clientnum, INJOY_DPAD_UP) && rebindaction == -1) )
				{
					keystatus[SDL_SCANCODE_UP] = 0;
					if ( rebindaction == -1 )
					{
						inputs.controllerClearInput(clientnum, INJOY_DPAD_UP);
					}
					inputs.hideMouseCursors();

					int previousClassPicked = client_classes[0];
					if ( client_classes[0] == 0 )
					{
						client_classes[0] = *(availableClasses.rbegin());;
					}
					else
					{
						auto it = availableClasses.find(client_classes[0]);
						if ( it != availableClasses.end() )
						{
							client_classes[0] = *(--it); // get previous element
						}
						else
						{
							client_classes[0] = 0;
						}
					}

					if ( previousClassPicked != client_classes[0] )
					{
						// reset class loadout
						stats[0]->clearStats();
						initClass(0);
					}
				}
				if ( keystatus[SDL_SCANCODE_DOWN] || (inputs.bControllerInputPressed(clientnum, INJOY_DPAD_DOWN) && rebindaction == -1) )
				{
					keystatus[SDL_SCANCODE_DOWN] = 0;
					if ( rebindaction == -1 )
					{
						inputs.controllerClearInput(clientnum, INJOY_DPAD_DOWN);
					}
					inputs.hideMouseCursors();

					auto it = availableClasses.find(client_classes[0]);
					int previousClassPicked = client_classes[0];
					if ( it != availableClasses.end() )
					{
						auto nextIt = std::next(it, 1);
						if ( nextIt == availableClasses.end() )
						{
							client_classes[0] = 0;
						}
						else
						{
							client_classes[0] = *(++it); // get next element
						}
					}
					else
					{
						client_classes[0] = 0;
					}

					if ( previousClassPicked != client_classes[0] )
					{
						// reset class loadout
						stats[0]->clearStats();
						initClass(0);
					}
				}
			}

			// class description
			ttfPrintText(ttf12, subx1 + 8, suby2 - 80, playerClassDescription(client_classes[0], 0));

			if ( drawLockedTooltip > 0 )
			{
				drawTooltip(&tooltip);
				ttfPrintTextFormattedColor(ttf12, tooltip.x + 4, tooltip.y + 6,
					uint32ColorOrange(*mainsurface), language[drawLockedTooltip]);
			}
		}

		// faces
		else if ( charcreation_step == 3 )
		{
			ttfPrintText(ttf16, subx1 + 24, suby1 + 32, language[1324]);
			for ( c = 0; c < NUMAPPEARANCES; c++ )
			{
				if ( stats[0]->appearance == c )
				{
					ttfPrintTextFormatted(ttf16, subx1 + 32, suby1 + 56 + c * 16, "[o] %s", language[20 + c]);
					ttfPrintText(ttf12, subx1 + 8, suby2 - 80, language[38 + c]);
				}
				else
				{
					ttfPrintTextFormatted(ttf16, subx1 + 32, suby1 + 56 + c * 16, "[ ] %s", language[20 + c]);
				}
				if ( inputs.bMouseLeft(clientnum) )
				{
					if ( omousex >= subx1 + 40 && omousex < subx1 + 72 )
					{
						if ( omousey >= suby1 + 56 + 16 * c && omousey < suby1 + 72 + 16 * c )
						{
							inputs.mouseClearLeft(clientnum);
							stats[0]->appearance = c;
						}
					}
				}
				if ( keystatus[SDL_SCANCODE_UP] || (inputs.bControllerInputPressed(clientnum, INJOY_DPAD_UP) && rebindaction == -1) )
				{
					keystatus[SDL_SCANCODE_UP] = 0;
					if ( rebindaction == -1 )
					{
						inputs.controllerClearInput(clientnum, INJOY_DPAD_UP);
					}
					inputs.hideMouseCursors();
					stats[0]->appearance--;
					if (stats[0]->appearance >= NUMAPPEARANCES)
					{
						stats[0]->appearance = NUMAPPEARANCES - 1;
					}
				}
				if ( keystatus[SDL_SCANCODE_DOWN] || (inputs.bControllerInputPressed(clientnum, INJOY_DPAD_DOWN) && rebindaction == -1) )
				{
					keystatus[SDL_SCANCODE_DOWN] = 0;
					if ( rebindaction == -1 )
					{
						inputs.controllerClearInput(clientnum, INJOY_DPAD_DOWN);
					}
					inputs.hideMouseCursors();
					stats[0]->appearance++;
					if (stats[0]->appearance >= NUMAPPEARANCES)
					{
						stats[0]->appearance = 0;
					}
				}
			}
		}

		// name
		else if ( charcreation_step == 4 )
		{
			ttfPrintText(ttf16, subx1 + 24, suby1 + 32, language[1325]);
			drawDepressed(subx1 + 40, suby1 + 56, subx1 + 364, suby1 + 88);
			ttfPrintText(ttf16, subx1 + 48, suby1 + 64, stats[0]->name);
			ttfPrintText(ttf12, subx1 + 8, suby2 - 80, language[1326]);

			// enter character name
			if ( !SDL_IsTextInputActive() )
			{
				if (inputstr != stats[0]->name) //TODO: NX PORT: Not sure if this portion is correct...the PC version of this chunk has changed significantly in the interleaving time.
				{
					inputstr = stats[0]->name;
#ifdef NINTENDO
					auto result = nxKeyboard("Enter your character's name");
					if (result.success)
					{
						strncpy(inputstr, result.str.c_str(), 21);
						inputstr[21] = '\0';
					}
#endif
				}
				SDL_StartTextInput();
			}
			//strncpy(stats[0]->name,inputstr,16);
			inputlen = 22;
			if (lastname != "" && strlen(inputstr) == 0)
			{
				strncat(inputstr, lastname.c_str(), std::max<size_t>(0, inputlen - strlen(inputstr)));
				lastname = ""; // Set this to nothing while we're currently editing so it doesn't keep filling it.  We'll save it again if we leave this tab.
			}

			if ( (ticks - cursorflash) % TICKS_PER_SECOND < TICKS_PER_SECOND / 2 )
			{
				int x;
				getSizeOfText(ttf16, stats[0]->name, &x, NULL);
				ttfPrintText(ttf16, subx1 + 48 + x, suby1 + 64, "_");
			}
		}

		// gamemode
		else if ( charcreation_step == 5 )
		{
			ttfPrintText(ttf16, subx1 + 24, suby1 + 32, language[1327]);

			std::vector<Sint32> optionY;
			std::vector<char*> optionTexts;
			std::vector<char*> optionSubtexts;
			std::vector<char*> optionDescriptions;
			std::vector<Uint32> displayedOptionToGamemode;
			Uint32 optionHeight = TTF12_HEIGHT + 2;
			int nummodes = 3;
#if (defined USE_EOS && defined STEAMWORKS)
			nummodes += 2;
			if ( LobbyHandler.crossplayEnabled )
			{
				nummodes += 1;
				optionY.insert(optionY.end(), { suby1 + 56, suby1 + 86, suby1 + 128, suby1 + 178, suby1 + 216, suby1 + 256 });
				optionTexts.insert(optionTexts.end(), { language[1328], language[1330], language[1330], language[1332], language[1330], language[1332] });
				optionDescriptions.insert(optionDescriptions.end(), { language[1329], language[3946], language[3947], language[3945], language[1538], language[1539] });
				optionSubtexts.insert(optionSubtexts.end(), { nullptr, language[3943], language[3944], nullptr, language[1537], language[1537] });
				displayedOptionToGamemode.insert(displayedOptionToGamemode.end(), { SINGLE, SERVER, SERVERCROSSPLAY, CLIENT, DIRECTSERVER, DIRECTCLIENT});
			}
			else
			{
				optionY.insert(optionY.end(), { suby1 + 56, suby1 + 76, suby1 + 96, suby1 + 136, suby1 + 176, 0 });
				optionTexts.insert(optionTexts.end(), { language[1328], language[1330], language[1332], language[1330], language[1332], nullptr });
				optionDescriptions.insert(optionDescriptions.end(), { language[1329], language[1331], language[1333], language[1538], language[1539], nullptr });
				optionSubtexts.insert(optionSubtexts.end(), { nullptr, nullptr, nullptr, language[1537], language[1537], nullptr });
				displayedOptionToGamemode.insert(displayedOptionToGamemode.end(), { SINGLE, SERVER, CLIENT, DIRECTSERVER, DIRECTCLIENT, 0 });
			}
#elif (defined(USE_EOS) || defined(STEAMWORKS))
			nummodes += 2;
			optionY.insert(optionY.end(), { suby1 + 56, suby1 + 76, suby1 + 96, suby1 + 136, suby1 + 176, 0 });
			optionTexts.insert(optionTexts.end(), { language[1328], language[1330], language[1332], language[1330], language[1332], nullptr });
			optionDescriptions.insert(optionDescriptions.end(), { language[1329], language[1331], language[1333], language[1538], language[1539], nullptr });
			optionSubtexts.insert(optionSubtexts.end(), { nullptr, nullptr, nullptr, language[1537], language[1537] });
			displayedOptionToGamemode.insert(displayedOptionToGamemode.end(), { SINGLE, SERVER, CLIENT, DIRECTSERVER, DIRECTCLIENT, 0 });
#else
			optionY.insert(optionY.end(), { suby1 + 56, suby1 + 76, suby1 + 96, suby1 + 136, suby1 + 176, 0 });
			optionTexts.insert(optionTexts.end(), { language[1328], language[1330], language[1332], language[1330], language[1332], nullptr });
			optionDescriptions.insert(optionDescriptions.end(), { language[1329], language[1331], language[1333], language[1538], language[1539], nullptr });
			optionSubtexts.insert(optionSubtexts.end(), { nullptr, nullptr, nullptr, language[1537], language[1537] });
			displayedOptionToGamemode.insert(displayedOptionToGamemode.end(), { SINGLE, SERVER, CLIENT, DIRECTSERVER, DIRECTCLIENT, 0 });
#endif
			for ( int mode = 0; mode < nummodes; mode++ )
			{
				char selected = ' ';
				if ( multiplayerselect == displayedOptionToGamemode.at(mode) )
				{
					selected = 'o';
				}
				if ( optionSubtexts.at(mode) == nullptr )
				{
					ttfPrintTextFormatted(ttf16, subx1 + 32, optionY.at(mode), "[%c] %s", selected, optionTexts.at(mode));
				}
				else
				{
					ttfPrintTextFormatted(ttf16, subx1 + 32, optionY.at(mode), "[%c] %s\n     %s", selected, optionTexts.at(mode), optionSubtexts.at(mode));
				}
				if ( selected == 'o' ) // draw description
				{
					ttfPrintText(ttf12, subx1 + 8, suby2 - 80, optionDescriptions.at(mode));
				}

				if ( multiplayerselect == SINGLE )
				{
					if ( singleplayerSavegameFreeSlot == -1 )
					{
						ttfPrintTextColor(ttf12, subx1 + 8, suby2 - 60, uint32ColorOrange(*mainsurface), true, language[2965]);
					}
				}
				else if ( multiplayerselect > SINGLE )
				{
					if ( multiplayerSavegameFreeSlot == -1 )
					{
						ttfPrintTextColor(ttf12, subx1 + 8, suby2 - 60, uint32ColorOrange(*mainsurface), true, language[2966]);
					}
					if ( gamemods_numCurrentModsLoaded >= 0 )
					{
						ttfPrintTextColor(ttf12, subx1 + 8, suby2 - 60 - TTF12_HEIGHT * 6, uint32ColorOrange(*mainsurface), true, language[2981]);
					}
				}
				if ( gamemods_numCurrentModsLoaded >= 0 )
				{
					ttfPrintTextColor(ttf12, subx1 + 8, suby2 - 60 + TTF12_HEIGHT, uint32ColorBaronyBlue(*mainsurface), true, language[2982]);
				}
				if ( inputs.bMouseLeft(clientnum) )
				{
					if ( omousex >= subx1 + 40 && omousex < subx1 + 72 )
					{
						if ( omousey >= optionY.at(mode) && omousey < (optionY.at(mode) + optionHeight) )
						{
							inputs.mouseClearLeft(clientnum);
							multiplayerselect = displayedOptionToGamemode.at(mode);
						}
					}
				}
			}
			if (keystatus[SDL_SCANCODE_UP] || (inputs.bControllerInputPressed(clientnum, INJOY_DPAD_UP) && rebindaction == -1) )
			{
				keystatus[SDL_SCANCODE_UP] = 0;
				if ( rebindaction == -1 )
				{
					inputs.controllerClearInput(clientnum, INJOY_DPAD_UP);
				}
				inputs.hideMouseCursors();

				Uint32 vIndex = 0;
				for ( auto& option : displayedOptionToGamemode )
				{
					if ( option == multiplayerselect )
					{
						break;
					}
					++vIndex;
				}
				if ( vIndex > 0 )
				{
					multiplayerselect = displayedOptionToGamemode.at(vIndex - 1);
				}
				else
				{
					multiplayerselect = displayedOptionToGamemode.at(nummodes - 1);
				}
			}
			if ( keystatus[SDL_SCANCODE_DOWN] || (inputs.bControllerInputPressed(clientnum, INJOY_DPAD_DOWN) && rebindaction == -1) )
			{
				keystatus[SDL_SCANCODE_DOWN] = 0;
				if ( rebindaction == -1 )
				{
					inputs.controllerClearInput(clientnum, INJOY_DPAD_DOWN);
				}
				inputs.hideMouseCursors();

				Uint32 vIndex = 0;
				for ( auto& option : displayedOptionToGamemode )
				{
					if ( option == multiplayerselect )
					{
						break;
					}
					++vIndex;
				}
				if ( vIndex >= nummodes - 1 )
				{
					multiplayerselect = displayedOptionToGamemode.at(0);
				}
				else
				{
					multiplayerselect = displayedOptionToGamemode.at(vIndex + 1);
				}
			}
		}
	}

	// serial window.
#if (!defined STEAMWORKS && !defined USE_EOS)
	if ( intro && introstage == 1 && subwindow && !strcmp(subtext, language[3403]) && serialEnterWindow )
	{
		drawDepressed(subx1 + 8, suby1 + 32, subx2 - 8, suby1 + 56);
		ttfPrintText(ttf12, subx1 + 16, suby1 + 40, serialInputText);

		// enter character name
		if ( serialVerifyWindow == 0 && !SDL_IsTextInputActive() )
		{
			if (inputstr != serialInputText)
			{
				inputstr = serialInputText;
#ifdef NINTENDO
				auto result = nxKeyboard("Enter your character's name");
				if (result.success)
				{
					strncpy(inputstr, result.str.c_str(), 21);
					inputstr[21] = '\0'; //TODO: NX Port: The inputlen below names 63...? Should this be modified to match? Everywhere else, we operate on inputstr[21] when inputlen = 22.
				}
#endif
			}
			SDL_StartTextInput();
		}
		//strncpy(stats[0]->name,inputstr,16);
		inputlen = 63;

		if ( serialVerifyWindow > 0 )
		{
			if ( serialVerifyWindow % 10 < 3 )
			{
				ttfPrintTextFormattedColor(ttf12, subx1 + 16, suby2 - 20, uint32ColorOrange(*mainsurface), "Verifying");
			}
			else if ( serialVerifyWindow % 10 < 5 )
			{
				ttfPrintTextFormattedColor(ttf12, subx1 + 16, suby2 - 20, uint32ColorOrange(*mainsurface), "Verifying.");
			}
			else if ( serialVerifyWindow % 10 < 7 )
			{
				ttfPrintTextFormattedColor(ttf12, subx1 + 16, suby2 - 20, uint32ColorOrange(*mainsurface), "Verifying..");
			}
			else if ( serialVerifyWindow % 10 < 10 )
			{
				ttfPrintTextFormattedColor(ttf12, subx1 + 16, suby2 - 20, uint32ColorOrange(*mainsurface), "Verifying...");
			}
			if ( ticks % (TICKS_PER_SECOND / 2) == 0 )
			{
				++serialVerifyWindow;
				if ( serialVerifyWindow >= 25 )
				{
					std::string serial = serialInputText;

					// compute hash
					if ( !serial.empty() )
					{
						std::size_t DLCHash = serialHash(serial);
						if ( DLCHash == 144425 )
						{
							printlog("[LICENSE]: Myths and Outcasts DLC license key found.");
							enabledDLCPack1 = true;
							windowSerialResult(1);
						}
						else if ( DLCHash == 135398 )
						{
							printlog("[LICENSE]: Legends and Pariahs DLC license key found.");
							enabledDLCPack2 = true;
							windowSerialResult(2);
						}
						else
						{
							printlog("[LICENSE]: DLC license key invalid.");
							windowSerialResult(0);
						}
					}
					else
					{
						windowSerialResult(0);
					}
				}
			}
		}
		else if ( (ticks - cursorflash) % TICKS_PER_SECOND < TICKS_PER_SECOND / 2 )
		{
			int x;
			getSizeOfText(ttf12, serialInputText, &x, NULL);
			ttfPrintText(ttf12, subx1 + 16 + x, suby1 + 40, "_");
		}
	}
#endif

	// settings window
	if ( settings_window == true )
	{
		drawWindowFancy(subx1 + 16, suby1 + 44, subx2 - 16, suby2 - 32);

		int hovering_selection = -1; //0 to NUM_SERVER_FLAGS used for the game flags settings, e.g. are traps enabled, are cheats enabled, is minotaur enabled, etc.
		SDL_Rect tooltip_box;

		if ( inputs.bControllerInputPressed(clientnum, INJOY_MENU_SETTINGS_NEXT) && rebindaction == -1 )
		{
			inputs.controllerClearInput(clientnum, INJOY_MENU_SETTINGS_NEXT);
			changeSettingsTab(settings_tab + 1);
		}
		if ( inputs.bControllerInputPressed(clientnum, INJOY_MENU_SETTINGS_PREV) && rebindaction == -1 )
		{
			inputs.controllerClearInput(clientnum, INJOY_MENU_SETTINGS_PREV);
			changeSettingsTab(settings_tab - 1);
		}

		// video tab
		if ( settings_tab == SETTINGS_VIDEO_TAB )
		{
			// resolution
			ttfPrintText(ttf12, subx1 + 24, suby1 + 60, language[1338]);
			c=0;
			for ( auto cur : resolutions )
			{
				int width, height;
				std::tie (width, height) = cur;
				if ( settings_xres == width && settings_yres == height )
				{
					ttfPrintTextFormatted(ttf12, subx1 + 32, suby1 + 84 + c * 16, "[o] %dx%d", width, height);
				}
				else
				{
					ttfPrintTextFormatted(ttf12, subx1 + 32, suby1 + 84 + c * 16, "[ ] %dx%d", width, height);
				}
				if ( inputs.bMouseLeft(clientnum) )
				{
					if ( omousex >= subx1 + 38 && omousex < subx1 + 62 )
					{
						if ( omousey >= suby1 + 84 + c * 16 && omousey < suby1 + 96 + c * 16 )
						{
							inputs.mouseClearLeft(clientnum);
							settings_xres = width;
							settings_yres = height;
							resolutionChanged = true;
						}
					}
				}
				c++;
			}

			// extra options
			ttfPrintText(ttf12, subx1 + 224, suby1 + 60, language[1339]);
			if ( settings_smoothlighting )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 236, suby1 + 84, "[x] %s", language[1340]);
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 236, suby1 + 84, "[ ] %s", language[1340]);
			}
			if ( settings_fullscreen )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 236, suby1 + 108, "[x] %s", language[1341]);
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 236, suby1 + 108, "[ ] %s", language[1341]);
			}
			if ( settings_shaking )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 236, suby1 + 132, "[x] %s", language[1342]);
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 236, suby1 + 132, "[ ] %s", language[1342]);
			}
			if ( settings_bobbing )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 236, suby1 + 156, "[x] %s", language[1343]);
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 236, suby1 + 156, "[ ] %s", language[1343]);
			}
			if ( settings_spawn_blood )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 236, suby1 + 180, "[x] %s", language[1344]);
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 236, suby1 + 180, "[ ] %s", language[1344]);
			}
			if ( settings_colorblind )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 236, suby1 + 204, "[x] %s", language[1345]);
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 236, suby1 + 204, "[ ] %s", language[1345]);
			}
			if ( settings_light_flicker )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 236, suby1 + 228, "[x] %s", language[2967]);
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 236, suby1 + 228, "[ ] %s", language[2967]);
			}
			if ( settings_vsync )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 236, suby1 + 252, "[x] %s", language[3011]);
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 236, suby1 + 252, "[ ] %s", language[3011]);
			}
			if ( settings_status_effect_icons )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 236, suby1 + 276, "[x] %s", language[3357]);
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 236, suby1 + 276, "[ ] %s", language[3357]);
			}
			if ( settings_borderless )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 236, suby1 + 300, "[x] %s", language[3935]);
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 236, suby1 + 300, "[ ] %s", language[3935]);
			}

			if ( inputs.bMouseLeft(clientnum) )
			{
				if ( omousex >= subx1 + 242 && omousex < subx1 + 266 )
				{
					if ( omousey >= suby1 + 84 && omousey < suby1 + 84 + 12 )
					{
						inputs.mouseClearLeft(clientnum);
						settings_smoothlighting = (settings_smoothlighting == 0);
					}
					else if ( omousey >= suby1 + 108 && omousey < suby1 + 108 + 12 )
					{
						inputs.mouseClearLeft(clientnum);
						settings_fullscreen = (settings_fullscreen == 0);
					}
					else if ( omousey >= suby1 + 132 && omousey < suby1 + 132 + 12 )
					{
						inputs.mouseClearLeft(clientnum);
						settings_shaking = (settings_shaking == 0);
					}
					else if ( omousey >= suby1 + 156 && omousey < suby1 + 156 + 12 )
					{
						inputs.mouseClearLeft(clientnum);
						settings_bobbing = (settings_bobbing == 0);
					}
					else if ( omousey >= suby1 + 180 && omousey < suby1 + 180 + 12 )
					{
						inputs.mouseClearLeft(clientnum);
						settings_spawn_blood = (settings_spawn_blood == 0);
					}
					else if ( omousey >= suby1 + 204 && omousey < suby1 + 204 + 12 )
					{
						inputs.mouseClearLeft(clientnum);
						settings_colorblind = (settings_colorblind == false);
					}
					else if ( omousey >= suby1 + 228 && omousey < suby1 + 228 + 12 )
					{
						inputs.mouseClearLeft(clientnum);
						settings_light_flicker = (settings_light_flicker == false);
					}
					else if ( omousey >= suby1 + 252 && omousey < suby1 + 252 + 12 )
					{
						inputs.mouseClearLeft(clientnum);
						settings_vsync = (settings_vsync == false);
					}
					else if ( omousey >= suby1 + 276 && omousey < suby1 + 276 + 12 )
					{
						inputs.mouseClearLeft(clientnum);
						settings_status_effect_icons = (settings_status_effect_icons == false);
					}
					else if ( omousey >= suby1 + 300 && omousey < suby1 + 300 + 12 )
					{
						inputs.mouseClearLeft(clientnum);
						settings_borderless = (settings_borderless == false);
					}
				}
			}

			// minimap options
			ttfPrintText(ttf12, subx1 + 498, suby1 + 60, language[3022]);

			// total scale
			ttfPrintText(ttf12, subx1 + 498, suby1 + 84, language[3025]);
			doSlider(subx1 + 498, suby1 + 84 + 24, 10, 2, 16, 1, (int*)(&settings_minimap_scale));

			// objects (players/allies/minotaur) scale
			ttfPrintText(ttf12, subx1 + 498, suby1 + 130, language[3026]);
			doSlider(subx1 + 498, suby1 + 130 + 24, 10, 0, 4, 1, (int*)(&settings_minimap_object_zoom));

			// foreground transparency
			ttfPrintText(ttf12, subx1 + 498, suby1 + 176, language[3023]);
			doSlider(subx1 + 498, suby1 + 176 + 24, 10, 0, 100, 1, (int*)(&settings_minimap_transparency_foreground));

			// background transparency
			ttfPrintText(ttf12, subx1 + 498, suby1 + 222, language[3024]);
			doSlider(subx1 + 498, suby1 + 222 + 24, 10, 0, 100, 1, (int*)(&settings_minimap_transparency_background));

			// UI options
			ttfPrintText(ttf12, subx1 + 498, suby1 + 276, language[3034]);

			if ( settings_uiscale_charactersheet )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 498, suby1 + 300, "[x] %s", language[3027]);
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 498, suby1 + 300, "[ ] %s", language[3027]);
			}
			if ( settings_uiscale_skillspage )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 498, suby1 + 324, "[x] %s", language[3028]);
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 498, suby1 + 324, "[ ] %s", language[3028]);
			}
			if ( settings_hide_statusbar )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 498, suby1 + 348, "[x] %s", language[3033]);
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 498, suby1 + 348, "[ ] %s", language[3033]);
			}
			if ( settings_hide_playertags )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 498, suby1 + 372, "[x] %s", language[3136]);
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 498, suby1 + 372, "[ ] %s", language[3136]);
			}
			if ( settings_show_skill_values )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 498, suby1 + 396, "[x] %s", language[3159]);
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 498, suby1 + 396, "[ ] %s", language[3159]);
			}

			if ( inputs.bMouseLeft(clientnum) )
			{
				if ( omousex >= subx1 + 498 && omousex < subx1 + 522 )
				{
					if ( omousey >= suby1 + 300 && omousey < suby1 + 300 + 12 )
					{
						inputs.mouseClearLeft(clientnum);
						settings_uiscale_charactersheet = (settings_uiscale_charactersheet == 0);
					}
					else if ( omousey >= suby1 + 324 && omousey < suby1 + 324 + 12 )
					{
						inputs.mouseClearLeft(clientnum);
						settings_uiscale_skillspage = (settings_uiscale_skillspage == 0);
					}
					else if ( omousey >= suby1 + 348 && omousey < suby1 + 348 + 12 )
					{
						inputs.mouseClearLeft(clientnum);
						settings_hide_statusbar = (settings_hide_statusbar == 0);
					}
					else if ( omousey >= suby1 + 372 && omousey < suby1 + 372 + 12 )
					{
						inputs.mouseClearLeft(clientnum);
						settings_hide_playertags = (settings_hide_playertags == 0);
					}
					else if ( omousey >= suby1 + 396 && omousey < suby1 + 396 + 12 )
					{
						inputs.mouseClearLeft(clientnum);
						settings_show_skill_values = (settings_show_skill_values == 0);
					}
				}
			}
			// UI scale sliders
			ttfPrintText(ttf12, subx1 + 498, suby2 - 220, language[3029]);
			doSliderF(subx1 + 498, suby2 - 220 + 24, 10, 1.f, 2.f, 0.25, &settings_uiscale_hotbar);
			ttfPrintText(ttf12, subx1 + 498, suby2 - 174, language[3030]);
			doSliderF(subx1 + 498, suby2 - 174 + 24, 10, 1.f, 2.f, 0.25, &settings_uiscale_chatlog);
			ttfPrintText(ttf12, subx1 + 498, suby2 - 128, language[3031]);
			doSliderF(subx1 + 498, suby2 - 128 + 24, 10, 1.f, 2.f, 0.25, &settings_uiscale_playerbars);
			ttfPrintText(ttf12, subx1 + 498, suby2 - 80, language[3032]);
			doSliderF(subx1 + 498, suby2 - 80 + 24, 10, 1.f, 2.f, 0.25, &settings_uiscale_inventory);

			// fov slider
			ttfPrintText(ttf12, subx1 + 24, suby2 - 174, language[1346]);
			doSlider(subx1 + 24, suby2 - 148, 14, 40, 100, 1, (int*)(&settings_fov));

			// gamma slider
			ttfPrintText(ttf12, subx1 + 24, suby2 - 128, language[1347]);
			doSliderF(subx1 + 24, suby2 - 104, 14, 0.25, 2.f, 0.25, &settings_gamma);

			// fps slider
			ttfPrintText(ttf12, subx1 + 24, suby2 - 80, language[2411]);
			doSlider(subx1 + 24, suby2 - 56, 14, 60, 144, 1, (int*)(&settings_fps));
		}

		// audio tab
		if ( settings_tab == SETTINGS_AUDIO_TAB )
		{
			ttfPrintText(ttf12, subx1 + 24, suby1 + 60, language[1348]);
			doSlider(subx1 + 24, suby1 + 84, 15, 0, 128, 0, &settings_sfxvolume);

			ttfPrintText(ttf12, subx1 + 24, suby1 + 108, language[3972]);
			doSlider(subx1 + 24, suby1 + 132, 15, 0, 128, 0, &settings_sfxAmbientVolume);

			ttfPrintText(ttf12, subx1 + 24, suby1 + 156, language[3973]);
			doSlider(subx1 + 24, suby1 + 180, 15, 0, 128, 0, &settings_sfxEnvironmentVolume);

			ttfPrintText(ttf12, subx1 + 24, suby1 + 204, language[1349]);
			doSlider(subx1 + 24, suby1 + 228, 15, 0, 128, 0, &settings_musvolume);

			if ( settings_minimap_ping_mute )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 24, suby1 + 264, "[x] %s", language[3012]);
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 24, suby1 + 264, "[ ] %s", language[3012]);
			}
			if ( settings_mute_audio_on_focus_lost )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 24, suby1 + 288, "[x] %s", language[3158]);
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 24, suby1 + 288, "[ ] %s", language[3158]);
			}
			if ( settings_mute_player_monster_sounds )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 24, suby1 + 312, "[x] %s", language[3371]);
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 24, suby1 + 312, "[ ] %s", language[3371]);
			}
			if ( inputs.bMouseLeft(clientnum) )
			{
				if ( omousex >= subx1 + 30 && omousex < subx1 + 54 )
				{
					if ( omousey >= suby1 + 168 && omousey < suby1 + 264 + 12 )
					{
						inputs.mouseClearLeft(clientnum);
						settings_minimap_ping_mute = (settings_minimap_ping_mute == false);
					}
					else if ( omousey >= suby1 + 192 && omousey < suby1 + 288 + 12 )
					{
						inputs.mouseClearLeft(clientnum);
						settings_mute_audio_on_focus_lost = (settings_mute_audio_on_focus_lost == false);
					}
					else if ( omousey >= suby1 + 216 && omousey < suby1 + 312 + 12 )
					{
						inputs.mouseClearLeft(clientnum);
						settings_mute_player_monster_sounds = (settings_mute_player_monster_sounds == false);
					}
				}
			}
		}

		// keyboard tab
		if ( settings_tab == SETTINGS_KEYBOARD_TAB )
		{
			ttfPrintText(ttf12, subx1 + 24, suby1 + 60, language[1350]);

			bool rebindingkey = false;
			if ( rebindkey != -1 )
			{
				rebindingkey = true;
			}

			for ( int c = 0; c < NUMIMPULSES; c++ )
			{
				if ( c < 14 )
				{
					ttfPrintText(ttf12, subx1 + 24, suby1 + 84 + 16 * c, language[1351 + c]);
				}
				else if ( c < 16 )
				{
					ttfPrintText(ttf12, subx1 + 24, suby1 + 84 + 16 * c, language[1940 + (c - 14)]);
				}
				else if ( c < 22 )
				{
					ttfPrintText(ttf12, subx1 + 24, suby1 + 84 + 16 * c, language[1986 + (c - 16)]);
				}
				else if ( c < 25 )
				{
					ttfPrintText(ttf12, subx1 + 24, suby1 + 84 + 16 * c, language[3901 + (c - 22)]);
				}
				if ( inputs.bMouseLeft(clientnum) && !rebindingkey )
				{
					if ( omousex >= subx1 + 24 && omousex < subx2 - 24 )
					{
						if ( omousey >= suby1 + 84 + c * 16 && omousey < suby1 + 96 + c * 16 )
						{
							inputs.mouseClearLeft(clientnum);
							lastkeypressed = 0;
							rebindingkey = true;
							rebindkey = c;
						}
					}
				}
				if ( c != rebindkey )
				{
					if ( !strcmp(getInputName(settings_impulses[c]), "Unassigned key" ))
					{
						ttfPrintTextColor(ttf12, subx1 + 256, suby1 + 84 + c * 16, uint32ColorBaronyBlue(*mainsurface), true, getInputName(settings_impulses[c]));
					}
					else if ( !strcmp(getInputName(settings_impulses[c]), "Unknown key") || !strcmp(getInputName(settings_impulses[c]), "Unknown trigger") )
					{
						ttfPrintTextColor(ttf12, subx1 + 256, suby1 + 84 + c * 16, uint32ColorRed(*mainsurface), true, getInputName(settings_impulses[c]));
					}
					else
					{
						ttfPrintText(ttf12, subx1 + 256, suby1 + 84 + c * 16, getInputName(settings_impulses[c]));
					}
				}
				else
				{
					ttfPrintTextColor(ttf12, subx1 + 256, suby1 + 84 + c * 16, uint32ColorGreen(*mainsurface), true, "...");
				}
			}

			if ( rebindkey != -1 && lastkeypressed )
			{
				if ( lastkeypressed == SDL_SCANCODE_ESCAPE )
				{
					keystatus[SDL_SCANCODE_ESCAPE] = 0;
					lastkeypressed = 0;
					rebindkey = -1;
				}
				else
				{
					settings_impulses[rebindkey] = lastkeypressed;
					if ( lastkeypressed == 283 )
					{
						inputs.mouseClearLeft(clientnum);  // fixes mouse-left not registering bug
					}
					rebindkey = -1;
				}
			}
		}

		// mouse tab
		if ( settings_tab == SETTINGS_MOUSE_TAB )
		{
			ttfPrintText(ttf12, subx1 + 24, suby1 + 60, language[1365]);
			doSliderF(subx1 + 24, suby1 + 84, 11, 0, 128, 1, &settings_mousespeed);

			// checkboxes
			if ( settings_reversemouse )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 24, suby1 + 108, "[x] %s", language[1366]);
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 24, suby1 + 108, "[ ] %s", language[1366]);
			}
			if ( settings_smoothmouse )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 24, suby1 + 132, "[x] %s", language[1367]);
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 24, suby1 + 132, "[ ] %s", language[1367]);
			}
			if ( settings_disablemouserotationlimit )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 24, suby1 + 156, "[x] %s", language[3918]);
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 24, suby1 + 156, "[ ] %s", language[3918]);
			}
			if ( inputs.bMouseLeft(clientnum) )
			{
				if ( omousex >= subx1 + 30 && omousex < subx1 + 54 )
				{
					if ( omousey >= suby1 + 108 && omousey < suby1 + 120 )
					{
						inputs.mouseClearLeft(clientnum);
						settings_reversemouse = (settings_reversemouse == 0);
					}
					if ( omousey >= suby1 + 132 && omousey < suby1 + 144 )
					{
						inputs.mouseClearLeft(clientnum);
						settings_smoothmouse = (settings_smoothmouse == 0);
					}
					if ( omousey >= suby1 + 156 && omousey < suby1 + 168 )
					{
						inputs.mouseClearLeft(clientnum);
						settings_disablemouserotationlimit = (settings_disablemouserotationlimit == 0);
					}
				}
			}
		}

		//Gamepad tab
		if (settings_tab == SETTINGS_GAMEPAD_BINDINGS_TAB)
		{
			SDL_Rect startPos;
			startPos.x = subx1 + 24;
			startPos.y = suby1 + 60;
			SDL_Rect currentPos = startPos;
			ttfPrintText(ttf8, currentPos.x, currentPos.y, language[1996]);
			currentPos.y += 24;

			bool rebindingaction = false;
			if (rebindaction != -1)
			{
				rebindingaction = true;
			}

			//Print out the bi-functional bindings.
			for ( int c = 0; c < INDEX_JOYBINDINGS_START_MENU; ++c, currentPos.y += 12 )
			{
				printJoybindingNames(currentPos, c, rebindingaction);
			}

			//Print out the menu-exclusive bindings.
			currentPos.y += 12;
			drawLine(subx1 + 24, currentPos.y - 6, subx2 - 24, currentPos.y - 6, uint32ColorGray(*mainsurface), 255);
			ttfPrintText(ttf8, currentPos.x, currentPos.y, language[1994]);
			currentPos.y += 18;
			for ( c = INDEX_JOYBINDINGS_START_MENU; c < INDEX_JOYBINDINGS_START_GAME; ++c, currentPos.y += 12 )
			{
				printJoybindingNames(currentPos, c, rebindingaction);
			}

			//Print out the game-exclusive bindings.
			currentPos.y += 12;
			drawLine(subx1 + 24, currentPos.y - 6, subx2 - 24, currentPos.y - 6, uint32ColorGray(*mainsurface), 255);
			ttfPrintText(ttf8, currentPos.x, currentPos.y, language[1995]);
			currentPos.y += 18;
			for ( c = INDEX_JOYBINDINGS_START_GAME; c < NUM_JOY_IMPULSES; ++c, currentPos.y += 12 )
			{
				printJoybindingNames(currentPos, c, rebindingaction);
			}

			if (rebindaction != -1 && lastkeypressed)
			{

				if (lastkeypressed >= 299)   /* Is a joybutton. */
				{
					settings_joyimpulses[rebindaction] = lastkeypressed;
					*inputPressed(lastkeypressed) = 0; //To prevent bugs where the button will still be treated as pressed after assigning it, potentially doing wonky things.
					rebindaction = -1;
				}
				else
				{
					if (lastkeypressed == SDL_SCANCODE_ESCAPE)
					{
						keystatus[SDL_SCANCODE_ESCAPE] = 0;
					}
					lastkeypressed = 0;
					rebindaction = -1;
				}
			}
		}

		//General gamepad settings
		if (settings_tab == SETTINGS_GAMEPAD_SETTINGS_TAB)
		{
			int current_option_x = subx1 + 24;
			int current_option_y = suby1 + 60;

			//Checkboxes.
			if (settings_gamepad_leftx_invert)
			{
				ttfPrintTextFormatted(ttf12, current_option_x, current_option_y, "[x] %s", language[2401]);
			}
			else
			{
				ttfPrintTextFormatted(ttf12, current_option_x, current_option_y, "[ ] %s", language[2401]);
			}

			if (inputs.bMouseLeft(clientnum) && mouseInBounds(clientnum, current_option_x, current_option_x + strlen("[x]")*TTF12_WIDTH, current_option_y, current_option_y + TTF12_HEIGHT))
			{
				inputs.mouseClearLeft(clientnum);
				settings_gamepad_leftx_invert = !settings_gamepad_leftx_invert;
			}

			current_option_y += 24;

			if (settings_gamepad_lefty_invert)
			{
				ttfPrintTextFormatted(ttf12, current_option_x, current_option_y, "[x] %s", language[2402]);
			}
			else
			{
				ttfPrintTextFormatted(ttf12, current_option_x, current_option_y, "[ ] %s", language[2402]);
			}

			if (inputs.bMouseLeft(clientnum) && mouseInBounds(clientnum, current_option_x, current_option_x + strlen("[x]")*TTF12_WIDTH, current_option_y, current_option_y + TTF12_HEIGHT))
			{
				inputs.mouseClearLeft(clientnum);
				settings_gamepad_lefty_invert = !settings_gamepad_lefty_invert;
			}

			current_option_y += 24;

			if (settings_gamepad_rightx_invert)
			{
				ttfPrintTextFormatted(ttf12, current_option_x, current_option_y, "[x] %s", language[2403]);
			}
			else
			{
				ttfPrintTextFormatted(ttf12, current_option_x, current_option_y, "[ ] %s", language[2403]);
			}

			if (inputs.bMouseLeft(clientnum) && mouseInBounds(clientnum, current_option_x, current_option_x + strlen("[x]")*TTF12_WIDTH, current_option_y, current_option_y + TTF12_HEIGHT))
			{
				inputs.mouseClearLeft(clientnum);
				settings_gamepad_rightx_invert = !settings_gamepad_rightx_invert;
			}

			current_option_y += 24;

			if (settings_gamepad_righty_invert)
			{
				ttfPrintTextFormatted(ttf12, current_option_x, current_option_y, "[x] %s", language[2404]);
			}
			else
			{
				ttfPrintTextFormatted(ttf12, current_option_x, current_option_y, "[ ] %s", language[2404]);
			}

			if (inputs.bMouseLeft(clientnum) && mouseInBounds(clientnum, current_option_x, current_option_x + strlen("[x]")*TTF12_WIDTH, current_option_y, current_option_y + TTF12_HEIGHT))
			{
				inputs.mouseClearLeft(clientnum);
				settings_gamepad_righty_invert = !settings_gamepad_righty_invert;
			}

			current_option_y += 24;

			if (settings_gamepad_menux_invert)
			{
				ttfPrintTextFormatted(ttf12, current_option_x, current_option_y, "[x] %s", language[2405]);
			}
			else
			{
				ttfPrintTextFormatted(ttf12, current_option_x, current_option_y, "[ ] %s", language[2405]);
			}

			if (inputs.bMouseLeft(clientnum) && mouseInBounds(clientnum, current_option_x, current_option_x + strlen("[x]")*TTF12_WIDTH, current_option_y, current_option_y + TTF12_HEIGHT))
			{
				inputs.mouseClearLeft(clientnum);
				settings_gamepad_menux_invert = !settings_gamepad_menux_invert;
			}

			current_option_y += 24;

			if (settings_gamepad_menuy_invert)
			{
				ttfPrintTextFormatted(ttf12, current_option_x, current_option_y, "[x] %s", language[2406]);
			}
			else
			{
				ttfPrintTextFormatted(ttf12, current_option_x, current_option_y, "[ ] %s", language[2406]);
			}

			if (inputs.bMouseLeft(clientnum) && mouseInBounds(clientnum, current_option_x, current_option_x + strlen("[x]")*TTF12_WIDTH, current_option_y, current_option_y + TTF12_HEIGHT))
			{
				inputs.mouseClearLeft(clientnum);
				settings_gamepad_menuy_invert = !settings_gamepad_menuy_invert;
			}

			current_option_y += 24;

			ttfPrintText(ttf12, current_option_x, current_option_y, language[2407]);
			current_option_y += 24;
			//doSlider(current_option_x, current_option_y, 11, 1, 2000, 200, &settings_gamepad_rightx_sensitivity, font8x8_bmp, 12); //Doesn't like any fonts besides the default.
			doSlider(current_option_x, current_option_y, 11, 1, 4096, 100, &settings_gamepad_rightx_sensitivity);

			current_option_y += 24;

			ttfPrintText(ttf12, current_option_x, current_option_y, language[2408]);
			current_option_y += 24;
			//doSlider(current_option_x, current_option_y, 11, 1, 2000, 200, &settings_gamepad_righty_sensitivity, font8x8_bmp, 12);
			doSlider(current_option_x, current_option_y, 11, 1, 4096, 100, &settings_gamepad_righty_sensitivity);

			current_option_y += 24;

			ttfPrintText(ttf12, current_option_x, current_option_y, language[2409]);
			current_option_y += 24;
			//doSlider(current_option_x, current_option_y, 11, 1, 2000, 200, &settings_gamepad_menux_sensitivity, font8x8_bmp, 12);
			doSlider(current_option_x, current_option_y, 11, 1, 4096, 100, &settings_gamepad_menux_sensitivity);

			current_option_y += 24;

			ttfPrintText(ttf12, current_option_x, current_option_y, language[2410]);
			current_option_y += 24;
			//doSlider(current_option_x, current_option_y, 11, 1, 2000, 200, &settings_gamepad_menuy_sensitivity, font8x8_bmp, 12);
			doSlider(current_option_x, current_option_y, 11, 1, 4096, 100, &settings_gamepad_menuy_sensitivity);
		}

		// miscellaneous options
		if (settings_tab == SETTINGS_MISC_TAB)
		{
			int current_x = subx1;
			int current_y = suby1 + 60;

			ttfPrintText(ttf12, subx1 + 24, current_y, language[1371]);
			current_y += 24;

			int options_start_y = current_y;
			if ( settings_broadcast )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 36, current_y, "[x] %s", language[1372]);
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 36, current_y, "[ ] %s", language[1372]);
			}
			current_y += 16;
			if ( settings_nohud )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 36, current_y, "[x] %s", language[1373]);
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 36, current_y, "[ ] %s", language[1373]);
			}
			current_y += 16;
			int hotbar_options_x = subx1 + 72 + 256;
			int hotbar_options_y = current_y;
			if ( settings_auto_hotbar_new_items )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 36, current_y, "[x] %s", language[1374]);
				int pad_x = hotbar_options_x;
				int pad_y = hotbar_options_y;
				drawWindowFancy(pad_x - 16, pad_y - 32, pad_x + 4 * 128 + 16, pad_y + 48 + 16);
				ttfPrintTextFormatted(ttf12, pad_x, current_y - 16, "%s", language[2583]);
				for ( int i = 0; i < (NUM_HOTBAR_CATEGORIES); ++i )
				{
					if ( settings_auto_hotbar_categories[i] == true )
					{
						ttfPrintTextFormatted(ttf12, pad_x, pad_y, "[x] %s", language[2571 + i]);
					}
					else
					{
						ttfPrintTextFormatted(ttf12, pad_x, pad_y, "[ ] %s", language[2571 + i]);
					}
					pad_x += 128;
					if ( i == 3 || i == 7 )
					{
						pad_x = hotbar_options_x;
						pad_y += 16;
					}
				}
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 36, current_y, "[ ] %s", language[1374]);
			}

			// autosort inventory categories
			int autosort_options_x = subx1 + 72 + 256;
			int autosort_options_y = current_y + 112;
			int pad_x = autosort_options_x;
			int pad_y = autosort_options_y;
			drawWindowFancy(pad_x - 16, pad_y - 32, pad_x + 4 * 128 + 16, pad_y + 48 + 16);
			ttfPrintTextFormatted(ttf12, pad_x, current_y - 16 + 112, "%s", language[2912]);

			// draw the values for autosort
			for ( int i = 0; i < (NUM_AUTOSORT_CATEGORIES); ++i )
			{
				ttfPrintTextFormatted(ttf12, pad_x, pad_y, "<");
				Uint32 autosortColor = uint32ColorGreen(*mainsurface);
				int padValue_x = pad_x;
				if ( settings_autosort_inventory_categories[i] < 0 )
				{
					autosortColor = uint32ColorRed(*mainsurface);
					padValue_x += 4; // centre the negative numbers.
				}
				else if ( settings_autosort_inventory_categories[i] == 0 )
				{
					autosortColor = uint32ColorWhite(*mainsurface);
				}
				ttfPrintTextFormattedColor(ttf12, padValue_x, pad_y, autosortColor, " %2d", settings_autosort_inventory_categories[i]);
				if ( i == NUM_AUTOSORT_CATEGORIES - 1 )
				{
					ttfPrintTextFormatted(ttf12, pad_x, pad_y, "    > %s", language[2916]);
				}
				else
				{
					ttfPrintTextFormatted(ttf12, pad_x, pad_y, "    > %s", language[2571 + i]);
				}
				pad_x += 128;
				if ( i == 3 || i == 7 )
				{
					pad_x = autosort_options_x;
					pad_y += 16;
				}
			}

			pad_x = autosort_options_x + (strlen(language[2912]) - 3) * (TTF12_WIDTH) + 8; // 3 chars from the end of string.
			pad_y = autosort_options_y;
			// hover text for autosort title text
			if ( mouseInBounds(clientnum, pad_x - 4, pad_x + 3 * TTF12_WIDTH + 8, current_y - 16 + 112, current_y - 16 + 124) )
			{
				tooltip_box.x = omousex - TTF12_WIDTH * 32;
				tooltip_box.y = omousey - (TTF12_HEIGHT * 3 + 16);
				tooltip_box.w = strlen(language[2914]) * TTF12_WIDTH + 8;
				tooltip_box.h = TTF12_HEIGHT * 3 + 8;
				drawTooltip(&tooltip_box);
				ttfPrintTextFormatted(ttf12, tooltip_box.x + 4, tooltip_box.y + 4, language[2913]);
				ttfPrintTextFormatted(ttf12, tooltip_box.x + 4, tooltip_box.y + 4 + TTF12_HEIGHT, language[2914]);
				ttfPrintTextFormatted(ttf12, tooltip_box.x + 4, tooltip_box.y + 4 + TTF12_HEIGHT * 2, language[2915]);
			}

			current_y += 16;
			if ( settings_auto_appraise_new_items )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 36, current_y, "[x] %s", language[1997]);
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 36, current_y, "[ ] %s", language[1997]);
			}
			current_y += 16;
			if ( settings_disable_messages )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 36, current_y, "[x] %s", language[1536]);
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 36, current_y, "[ ] %s", language[1536]);
			}
			current_y += 16;
			if ( settings_right_click_protect )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 36, current_y, "[x] %s", language[1998]);
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 36, current_y, "[ ] %s", language[1998]);
			}
			current_y += 16;
			if ( settings_hotbar_numkey_quick_add )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 36, current_y, "[x] %s", language[2590]);
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 36, current_y, "[ ] %s", language[2590]);
			}
			current_y += 16;
			if ( settings_lock_right_sidebar )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 36, current_y, "[x] %s", language[2598]);
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 36, current_y, "[ ] %s", language[2598]);
			}
			current_y += 16;
			if ( settings_show_game_timer_always )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 36, current_y, "[x] %s", language[2983]);
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 36, current_y, "[ ] %s", language[2983]);
			}
			current_y += 32;

			// server flag elements
			ttfPrintText(ttf12, subx1 + 24, current_y, language[1375]);
			current_y += 24;


			int server_flags_start_y = current_y;
			for ( int i = 0; i < NUM_SERVER_FLAGS; i++, current_y += 16 )
			{
				char flagStringBuffer[512] = "";
				if ( i < 5 )
				{
					strncpy(flagStringBuffer, language[153 + i], 255);
				}
				else
				{
					strncpy(flagStringBuffer, language[2917 - 5 + i], 255);
				}

				bool flagEnabled = settings_svFlags & power(2, i);
				if ( multiplayer == CLIENT )
				{
					flagEnabled = svFlags & power(2, i); // clients get the data from the server and don't cache it
				}

				if ( flagEnabled )
				{
					ttfPrintTextFormatted(ttf12, subx1 + 36, current_y, "[x] %s", flagStringBuffer);
				}
				else
				{
					ttfPrintTextFormatted(ttf12, subx1 + 36, current_y, "[ ] %s", flagStringBuffer);
				}
				if (mouseInBounds(clientnum, subx1 + 36 + 6, subx1 + 36 + 24 + 6, current_y, current_y + 12))   //So many gosh dang magic numbers ._.
				{
					if ( i < 5 )
					{
						strncpy(flagStringBuffer, language[1942 + i], 255);
					}
					else
					{
						strncpy(flagStringBuffer, language[2921 - 5 + i], 255);
					}
					if (strlen(flagStringBuffer) > 0)   //Don't bother drawing a tooltip if the file doesn't say anything.
					{
						hovering_selection = i;
#if (!defined STEAMWORKS && !defined USE_EOS)
						if ( hovering_selection == 0 )
						{
							hovering_selection = -1; // don't show cheats tooltip about disabling achievements.
						}
#endif // STEAMWORKS
						tooltip_box.x = omousex + 16;
						tooltip_box.y = omousey + 8; //I hate magic numbers :|. These should probably be replaced with omousex + mousecursorsprite->width, omousey + mousecursorsprite->height, respectively.
						if ( i == 2 || i == 3 || i == 5 || i == 6 || i == 7 )
						{
							tooltip_box.h = TTF12_HEIGHT * 2 + 8;
						}
						else if ( i == 4 || i == 8 )
						{
							tooltip_box.h = TTF12_HEIGHT * 3 + 8;
						}
						else
						{
							tooltip_box.h = TTF12_HEIGHT + 8;
						}
						if ( gameModeManager.isServerflagDisabledForCurrentMode(i) )
						{
							strcat(flagStringBuffer, language[3962]); // changing flags disabled.
							tooltip_box.h += TTF12_HEIGHT;
						}
						tooltip_box.w = longestline(flagStringBuffer) * TTF12_WIDTH + 8; //MORE MAGIC NUMBERS. HNNGH. I can guess what they all do, but dang.
					}
				}
			}

			// network options
			current_y += 32;
			ttfPrintText(ttf12, subx1 + 24, current_y, language[3146]);
			current_y += 24;
			int networking_options_start_y = current_y;
			if ( settings_disableFPSLimitOnNetworkMessages )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 36, current_y, "[x] %s", "disable netcode FPS optimization");
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 36, current_y, "[ ] %s", "disable netcode FPS optimization");
			}
			current_y += 16;
#ifdef STEAMWORKS
			if ( settings_disableMultithreadedSteamNetworking )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 36, current_y, "[x] %s", language[3147]);
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 36, current_y, "[ ] %s", language[3147]);
			}
#ifdef USE_EOS
			current_y += 16;
			ttfPrintTextFormatted(ttf12, subx1 + 36, current_y, "[%c] %s", LobbyHandler.settings_crossplayEnabled ? 'x' : ' ', language[3948]);
#endif
#endif // STEAMWORKS

			if (hovering_selection > -1)
			{
				drawTooltip(&tooltip_box);
				if (hovering_selection < NUM_SERVER_FLAGS)
				{
					char flagStringBuffer[512] = "";
					if ( hovering_selection < 5 )
					{
						strncpy(flagStringBuffer, language[1942 + hovering_selection], 255);
					}
					else
					{
						strncpy(flagStringBuffer, language[2921 - 5 + hovering_selection], 255);
					}
					if ( gameModeManager.isServerflagDisabledForCurrentMode(hovering_selection) )
					{
						strcat(flagStringBuffer, language[3962]); // changing flags disabled.
					}
					ttfPrintTextFormatted(ttf12, tooltip_box.x + 4, tooltip_box.y + 4, flagStringBuffer);
				}
			}

			current_y = options_start_y;

			if ( inputs.bMouseLeft(clientnum) )
			{
				if ( omousex >= subx1 + 42 && omousex < subx1 + 66 )
				{
					if ( omousey >= current_y && omousey < current_y + 12 )
					{
						inputs.mouseClearLeft(clientnum);
						settings_broadcast = (settings_broadcast == false);
					}
					else if ( omousey >= (current_y += 16) && omousey < current_y + 12 )
					{
						inputs.mouseClearLeft(clientnum);
						settings_nohud = (settings_nohud == false);
					}
					else if ( omousey >= (current_y += 16) && omousey < current_y + 12 )
					{
						inputs.mouseClearLeft(clientnum);
						settings_auto_hotbar_new_items = (settings_auto_hotbar_new_items == false);
					}
					else if ( omousey >= (current_y += 16) && omousey < current_y + 12 )
					{
						inputs.mouseClearLeft(clientnum);
						settings_auto_appraise_new_items = (settings_auto_appraise_new_items == false);
					}
					else if ( omousey >= (current_y += 16) && omousey < current_y + 12 )
					{
						inputs.mouseClearLeft(clientnum);
						settings_disable_messages = (settings_disable_messages == false);
					}
					else if ( omousey >= (current_y += 16) && omousey < current_y + 12 )
					{
						inputs.mouseClearLeft(clientnum);
						settings_right_click_protect = (settings_right_click_protect == false);
					}
					else if ( omousey >= (current_y += 16) && omousey < current_y + 12 )
					{
						inputs.mouseClearLeft(clientnum);
						settings_hotbar_numkey_quick_add = (settings_hotbar_numkey_quick_add == false);
					}
					else if ( omousey >= (current_y += 16) && omousey < current_y + 12 )
					{
						inputs.mouseClearLeft(clientnum);
						settings_lock_right_sidebar = (settings_lock_right_sidebar == false);
					}
					else if ( omousey >= (current_y += 16) && omousey < current_y + 12 )
					{
						inputs.mouseClearLeft(clientnum);
						settings_show_game_timer_always = (settings_show_game_timer_always == false);
					}
				}
				else
				{
					if ( settings_auto_hotbar_new_items )
					{
						if ( inputs.bMouseLeft(clientnum) )
						{
							for ( int i = 0; i < NUM_HOTBAR_CATEGORIES; ++i )
							{
								if ( mouseInBounds(clientnum, hotbar_options_x, hotbar_options_x + 24, hotbar_options_y, hotbar_options_y + 12) )
								{
									settings_auto_hotbar_categories[i] = !settings_auto_hotbar_categories[i];
									inputs.mouseClearLeft(clientnum);
								}
								hotbar_options_x += 128;
								if ( i == 3 || i == 7 )
								{
									hotbar_options_x -= (128 * 4);
									hotbar_options_y += 16;
								}
							}
						}
					}

					// autosort category toggles
					if ( inputs.bMouseLeft(clientnum) )
					{
						for ( int i = 0; i < NUM_AUTOSORT_CATEGORIES; ++i )
						{
							if ( mouseInBounds(clientnum, autosort_options_x, autosort_options_x + 16, autosort_options_y, autosort_options_y + 12) )
							{
								--settings_autosort_inventory_categories[i];
								if ( settings_autosort_inventory_categories[i] < -9 )
								{
									settings_autosort_inventory_categories[i] = 9;
								}
								inputs.mouseClearLeft(clientnum);
							}
							else if ( mouseInBounds(clientnum, autosort_options_x + 36, autosort_options_x + 52, autosort_options_y, autosort_options_y + 12) )
							{
								++settings_autosort_inventory_categories[i];
								if ( settings_autosort_inventory_categories[i] > 9 )
								{
									settings_autosort_inventory_categories[i] = -9;
								}
								inputs.mouseClearLeft(clientnum);
							}
							autosort_options_x += 128;
							if ( i == 3 || i == 7 )
							{
								autosort_options_x -= (128 * 4);
								autosort_options_y += 16;
							}
						}
					}
				}

				if ( multiplayer != CLIENT )
				{
					current_y = server_flags_start_y;
					for ( int i = 0; i < NUM_SERVER_FLAGS; i++, current_y += 16 )
					{
						if ( !gameModeManager.isServerflagDisabledForCurrentMode(i)
							&& mouseInBounds(clientnum, subx1 + 36 + 6, subx1 + 36 + 24 + 6, current_y, current_y + 12) )
						{
							inputs.mouseClearLeft(clientnum);

							// toggle flag
							settings_svFlags ^= power(2, i);
						}
					}
				}
			}


			if ( omousex >= subx1 + 42 && omousex < subx1 + 66 )
			{
				tooltip_box.x = omousex + 16;
				tooltip_box.y = omousey + 8;
				tooltip_box.h = TTF12_HEIGHT + 8;

				// networking hover text and mouse selection
				current_y = networking_options_start_y;
				if ( omousey >= current_y && omousey < current_y + 12 )
				{
					//tooltip_box.w = longestline(language[3148]) * TTF12_WIDTH + 8;
					//tooltip_box.h = TTF12_HEIGHT * 2 + 8;
					//drawTooltip(&tooltip_box);
					//ttfPrintTextFormatted(ttf12, tooltip_box.x + 4, tooltip_box.y + 4, language[3148]);
					if ( inputs.bMouseLeft(clientnum) )
					{
						inputs.mouseClearLeft(clientnum);
						settings_disableFPSLimitOnNetworkMessages = (settings_disableFPSLimitOnNetworkMessages == false);
					}
				}
				current_y += 16;
#ifdef STEAMWORKS
				if ( omousey >= current_y && omousey < current_y + 12 )
				{
					tooltip_box.w = longestline(language[3148]) * TTF12_WIDTH + 8;
					tooltip_box.h = TTF12_HEIGHT * 2 + 8;
					drawTooltip(&tooltip_box);
					ttfPrintTextFormatted(ttf12, tooltip_box.x + 4, tooltip_box.y + 4, language[3148]);
					if ( inputs.bMouseLeft(clientnum) )
					{
						inputs.mouseClearLeft(clientnum);
						settings_disableMultithreadedSteamNetworking = true;// (settings_disableMultithreadedSteamNetworking == false);
					}
				}
#ifdef USE_EOS
				current_y += 16;
				if ( omousey >= current_y && omousey < current_y + 12 )
				{
					/*tooltip_box.w = longestline(language[3148]) * TTF12_WIDTH + 8;
					tooltip_box.h = TTF12_HEIGHT * 2 + 8;
					drawTooltip(&tooltip_box);
					ttfPrintTextFormatted(ttf12, tooltip_box.x + 4, tooltip_box.y + 4, language[3148]);*/
					if ( inputs.bMouseLeft(clientnum) )
					{
						inputs.mouseClearLeft(clientnum);
						LobbyHandler.settings_crossplayEnabled = !LobbyHandler.settings_crossplayEnabled;
					}
				}
#endif
#endif // STEAMWORKS


				current_y = options_start_y;

				if ( omousey >= current_y && omousey < current_y + 12 ) // ip broadcast
				{
					tooltip_box.w = longestline(language[3149]) * TTF12_WIDTH + 8;
					tooltip_box.h = TTF12_HEIGHT + 8;
					drawTooltip(&tooltip_box);
					ttfPrintTextFormatted(ttf12, tooltip_box.x + 4, tooltip_box.y + 4, language[3149]);
				}
				else if ( omousey >= (current_y += 16) && omousey < current_y + 12 ) // no hud
				{
					tooltip_box.w = longestline(language[3150]) * TTF12_WIDTH + 8;
					tooltip_box.h = TTF12_HEIGHT + 8;
					drawTooltip(&tooltip_box);
					ttfPrintTextFormatted(ttf12, tooltip_box.x + 4, tooltip_box.y + 4, language[3150]);
				}
				else if ( omousey >= (current_y += 16) && omousey < current_y + 12 ) // auto add to hotbar
				{
					tooltip_box.w = longestline(language[3151]) * TTF12_WIDTH + 8;
					tooltip_box.h = TTF12_HEIGHT * 2 + 8;
					drawTooltip(&tooltip_box);
					ttfPrintTextFormatted(ttf12, tooltip_box.x + 4, tooltip_box.y + 4, language[3151]);
				}
				else if ( omousey >= (current_y += 16) && omousey < current_y + 12 ) // auto appraisal
				{
					tooltip_box.w = longestline(language[3152]) * TTF12_WIDTH + 8;
					tooltip_box.h = TTF12_HEIGHT * 2 + 8;
					drawTooltip(&tooltip_box);
					ttfPrintTextFormatted(ttf12, tooltip_box.x + 4, tooltip_box.y + 4, language[3152]);
				}
				else if ( omousey >= (current_y += 16) && omousey < current_y + 12 ) // no messages
				{
					tooltip_box.w = longestline(language[3153]) * TTF12_WIDTH + 8;
					tooltip_box.h = TTF12_HEIGHT * 2 + 8;
					drawTooltip(&tooltip_box);
					ttfPrintTextFormatted(ttf12, tooltip_box.x + 4, tooltip_box.y + 4, language[3153]);
				}
				else if ( omousey >= (current_y += 16) && omousey < current_y + 12 ) // right click protect
				{
					tooltip_box.w = longestline(language[3154]) * TTF12_WIDTH + 8;
					tooltip_box.h = TTF12_HEIGHT * 2 + 8;
					drawTooltip(&tooltip_box);
					ttfPrintTextFormatted(ttf12, tooltip_box.x + 4, tooltip_box.y + 4, language[3154]);
				}
				else if ( omousey >= (current_y += 16) && omousey < current_y + 12 ) // numkey hotbar
				{
					tooltip_box.w = longestline(language[3155]) * TTF12_WIDTH + 8;
					tooltip_box.h = TTF12_HEIGHT * 3 + 8;
					drawTooltip(&tooltip_box);
					ttfPrintTextFormatted(ttf12, tooltip_box.x + 4, tooltip_box.y + 4, language[3155]);
				}
				else if ( omousey >= (current_y += 16) && omousey < current_y + 12 ) // lock ride sidebar
				{
					tooltip_box.w = longestline(language[3156]) * TTF12_WIDTH + 8;
					tooltip_box.h = TTF12_HEIGHT * 3 + 8;
					drawTooltip(&tooltip_box);
					ttfPrintTextFormatted(ttf12, tooltip_box.x + 4, tooltip_box.y + 4, language[3156]);
				}
				else if ( omousey >= (current_y += 16) && omousey < current_y + 12 ) // show timer always
				{
					tooltip_box.w = longestline(language[3157]) * TTF12_WIDTH + 8;
					tooltip_box.h = TTF12_HEIGHT * 3 + 8;
					drawTooltip(&tooltip_box);
					ttfPrintTextFormatted(ttf12, tooltip_box.x + 4, tooltip_box.y + 4, language[3157]);
				}
			}
		}
	}

	// achievements window
	if ( subwindow && achievements_window )
	{
		if ( mousestatus[SDL_BUTTON_WHEELDOWN] )
		{
			mousestatus[SDL_BUTTON_WHEELDOWN] = 0;
			buttonAchievementsDown(nullptr);
		}
		if ( mousestatus[SDL_BUTTON_WHEELUP] )
		{
			mousestatus[SDL_BUTTON_WHEELUP] = 0;
			buttonAchievementsUp(nullptr);
		}

		char page_str[128];
		int num_achievements = achievementNames.size();
		if ( num_achievements > 0 )
		{
			int num_unlocked = 0;
			int num_locked_and_hidden = 0;
			int num_displayed = num_achievements;
			for (auto& item : achievementNames)
			{
				if (achievementUnlocked(item.first.c_str()))
				{
					++num_unlocked;
				}
				else if ( achievementHidden.find(item.first.c_str()) != achievementHidden.end() )
				{
					++num_locked_and_hidden;
					--num_displayed;
				}
			}

			if ( num_locked_and_hidden > 0 )
			{
				++num_displayed; // add 1 hidden entry.
			}

			int max_pages = num_displayed / 6 + ((num_displayed % 6) ? 1 : 0);
			achievements_window_page = std::min(achievements_window_page, max_pages);

			snprintf(page_str, sizeof(page_str), "Page %d / %d\n\nUnlocked %d / %d achievements (%d%%)",
				achievements_window_page, max_pages, num_unlocked, num_achievements, (num_unlocked * 100) / num_achievements);
			ttfPrintText(ttf12, subx1 + 8, suby1 + 30, page_str);

			int first_ach = (achievements_window_page - 1) * 6;
			// list achievement images
			int index = 0;
			bool bFirstHiddenAchievement = true;
			for (auto& item : achievementNamesSorted)
			{
				bool hiddenAchievement = (achievementHidden.find(item.first) != achievementHidden.end());
				bool unlocked = achievementUnlocked(item.first.c_str());
				if ( index < first_ach )
				{
					++index; continue;
				}
				if ( hiddenAchievement )
				{
					if ( !bFirstHiddenAchievement )
					{
						continue;
					}
					bFirstHiddenAchievement = false;
				}

				const int iconAreaWidth = 12 + 64 + 12;

				SDL_Rect bodyBox;
				bodyBox.x = subx1 + 4 + iconAreaWidth;
				bodyBox.y = suby1 + 80 + 4 + (index - first_ach) * 80;
				bodyBox.w = subx2 - subx1 - 30 - 8 - iconAreaWidth;
				bodyBox.h = 80 - 8;
				drawWindowFancy(bodyBox.x, bodyBox.y, bodyBox.x + bodyBox.w, bodyBox.y + bodyBox.h);

				SDL_Rect iconBox;
				iconBox.x = subx1 + 4;
				iconBox.y = bodyBox.y;
				iconBox.w = iconAreaWidth;
				iconBox.h = bodyBox.h;
				drawWindowFancy(iconBox.x, iconBox.y, iconBox.x + iconBox.w, iconBox.y + iconBox.h);

				SDL_Rect bodyHighlight;
				bodyHighlight.x = bodyBox.x + 2;
				bodyHighlight.y = bodyBox.y + 2;
				bodyHighlight.w = bodyBox.w - 4;
				bodyHighlight.h = bodyBox.h - 4;

				SDL_Rect iconHighlight;
				iconHighlight.x = iconBox.x + 2;
				iconHighlight.y = iconBox.y + 2;
				iconHighlight.w = iconBox.w - 4;
				iconHighlight.h = iconBox.h - 4;

				if ( achievementUnlocked(item.first.c_str()) )
				{
					drawRect(&bodyHighlight, uint32ColorBaronyBlue(*mainsurface), 64);
					drawRect(&iconHighlight, SDL_MapRGB(mainsurface->format, 1, 0, 16), 255);
				}
				else
				{
					drawRect(&bodyHighlight, SDL_MapRGB(mainsurface->format, 128, 128, 128), 64);
					drawRect(&iconHighlight, SDL_MapRGB(mainsurface->format, 36, 36, 36), 255);
				}

				// draw name
				Uint32 nameColor = unlocked ? SDL_MapRGB(mainsurface->format, 0, 255, 255) : SDL_MapRGB(mainsurface->format, 128, 128, 128);
				if ( hiddenAchievement && !unlocked )
				{
					ttfPrintTextColor(ttf12, subx1 + 100, suby1 + 92 + (index - first_ach) * 80, nameColor, true, "Hidden Achievements");
				}
				else
				{
					ttfPrintTextColor(ttf12, subx1 + 100, suby1 + 92 + (index - first_ach) * 80, nameColor, true, item.second.c_str());
				}

				// draw description
				if ( !(hiddenAchievement && !unlocked) )
				{
					auto it = achievementDesc.find(item.first);
					if ( it != achievementDesc.end() )
					{
						auto item = *it;
						std::string sub = item.second.length() > 140 ? item.second.substr(0, 140) + "..." : item.second;
						for ( size_t c, offset = 0;;)
						{
							size_t lastoffset = offset;
							for ( c = lastoffset + 1; c < sub.size(); ++c )
							{
								if ( sub[c] == ' ' )
								{
									break;
								}
							}
							offset = c;
							if ( offset > 70 && lastoffset )
							{
								sub[lastoffset] = '\n';
								break;
							}
							if ( offset >= sub.size() )
							{
								break;
							}
						}
						ttfPrintText(ttf12, subx1 + 100, suby1 + 120 + (index - first_ach) * 80, sub.c_str());
					}
				}
				else if ( hiddenAchievement && !unlocked )
				{
					ttfPrintTextFormatted(ttf12, subx1 + 100, suby1 + 120 + (index - first_ach) * 80, 
						"+%d hidden achievements remain...", num_locked_and_hidden);
				}

				// draw progress
				if ( !unlocked )
				{
					auto it = achievementProgress.find(item.first);
					if ( it != achievementProgress.end() )
					{
						int maxValue = steamStatAchStringsAndMaxVals[it->second].second;
						int currentValue = g_SteamStats[it->second].m_iValue;
						int percent = (int)floor(currentValue * 100 / static_cast<double>(maxValue));
						//char percent_str[32] = { 0 };
						// snprintf(percent_str, sizeof(percent_str), "%3d%% complete", percent);
						// ttfPrintTextColor(ttf12, subx2 - 330, suby1 + 92 + (index - first_ach) * 80, uint32ColorWhite(*mainsurface), true, percent_str);

						SDL_Rect progressbar;
						progressbar.x = subx2 - 330 + (4 * TTF12_WIDTH) + TTF12_WIDTH;
						progressbar.y = suby1 + 92 + (index - first_ach) * 80 - 4;
						progressbar.h = TTF12_HEIGHT + 2;
						progressbar.w = (bodyBox.x + bodyBox.w) - progressbar.x - 4;
						drawWindowFancy(progressbar.x - 2, progressbar.y - 2, progressbar.x + progressbar.w + 2, progressbar.y + progressbar.h + 2);

						drawRect(&progressbar, SDL_MapRGB(mainsurface->format, 36, 36, 36), 255);
						progressbar.w = std::min((bodyBox.x + bodyBox.w) - progressbar.x - 4, static_cast<int>(progressbar.w * percent / 100.0));
						drawRect(&progressbar, uint32ColorBaronyBlue(*mainsurface), 92);
						progressbar.w = (bodyBox.x + bodyBox.w) - progressbar.x - TTF12_WIDTH;

						char progress_str[32] = { 0 };
						snprintf(progress_str, sizeof(progress_str), "%d / %d", currentValue, maxValue);
						ttfPrintTextColor(ttf12, progressbar.x + progressbar.w / 2 - (strlen(progress_str) * TTF12_WIDTH) / 2,
							suby1 + 92 + (index - first_ach) * 80, uint32ColorWhite(*mainsurface), true, progress_str);
					}
				}

				// draw unlock time
				if ( unlocked )
				{
					auto it = achievementUnlockTime.find(item.first);
					if ( it != achievementUnlockTime.end() )
					{
						char buffer[64];
						time_t t = (time_t)it->second;
						struct tm* tm_info = localtime(&t);
						strftime(buffer, sizeof(buffer), "Unlocked %Y/%m/%d at %H:%M:%S", tm_info);

						char text[64];
						snprintf(text, sizeof(text), "%32s", buffer);
						ttfPrintTextColor(ttf12, subx2 - 330, suby1 + 92 + (index - first_ach) * 80, uint32ColorYellow(*mainsurface), true, text);
					}
				}

				// draw image
				std::string img = unlocked ? item.first + ".png" : item.first + "_l.png";
				if ( !unlocked && hiddenAchievement )
				{
					img = "LOCKED_ACHIEVEMENT.png";
				}
				auto it = achievementImages.find(img);
				if ( it != achievementImages.end() )
				{
					SDL_Rect rect;
					rect.x = subx1 + 16;
					rect.y = suby1 + 88 + (index - first_ach) * 80;
					rect.w = 64;
					rect.h = 64;
					drawImage((*it).second, NULL, &rect);
				}

				++index;
				if (index >= first_ach + 6)
				{
					break;
				}
			}
		}
		else
		{
			ttfPrintText(ttf12, subx1 + 8, suby1 + 100, language[709]);
		}
	}

	// connect window
	if ( connect_window )
	{
		if ( connect_window == SERVER )
		{
			drawDepressed(subx1 + 8, suby1 + 40, subx2 - 8, suby1 + 64);
			ttfPrintText(ttf12, subx1 + 12, suby1 + 46, portnumber_char);

			// enter port number
			if ( !SDL_IsTextInputActive() )
			{
				SDL_StartTextInput(); //TODO: NX PORT: Why is the order different in some of these chunks? For example, some of them start text input first, others start it after the below chunk of code.
				if (inputstr != portnumber_char)
				{
					inputstr = portnumber_char;
#ifdef NINTENDO
					auto result = nxKeyboard("Enter port number");
					if (result.success)
					{
						strncpy(inputstr, result.str.c_str(), 21);
						inputstr[21] = '\0'; //TODO: NX PORT: Why 21? inputlen = 5 down there, shouldn't this be inputstr[4]?
					}
#endif
				}
			}
			//strncpy(portnumber_char,inputstr,5);
			inputlen = 5;
			if ( (ticks - cursorflash) % TICKS_PER_SECOND < TICKS_PER_SECOND / 2 )
			{
				int x;
				getSizeOfText(ttf12, portnumber_char, &x, NULL);
				ttfPrintText(ttf12, subx1 + 12 + x, suby1 + 46, "_");
			}
		}
		else if ( connect_window == CLIENT )
		{
			drawDepressed(subx1 + 8, suby1 + 40, subx2 - 8, suby1 + 64);
			if ( !broadcast )
			{
				ttfPrintText(ttf12, subx1 + 12, suby1 + 46, connectaddress);
			}
			else
			{
				int i;
				for ( i = 0; i < strlen(connectaddress); i++ )
				{
					ttfPrintText(ttf12, subx1 + 12 + 12 * i, suby1 + 46, "*");
				}
			}

			// enter address
			if ( !SDL_IsTextInputActive() )
			{
				SDL_StartTextInput();

				if (inputstr != connectaddress)
				{
					inputstr = connectaddress;
#ifdef NINTENDO
					auto result = nxKeyboard("Enter address");
					if (result.success)
					{
						strncpy(inputstr, result.str.c_str(), 21);
						inputstr[21] = '\0'; //TODO: NX PORT: Why not inputstr[30], since inputlen = 31?
					}
#endif
				}
			}
			//strncpy(connectaddress,inputstr,31);
			inputlen = 31;
			if ( (ticks - cursorflash) % TICKS_PER_SECOND < TICKS_PER_SECOND / 2 )
			{
				int x;
				getSizeOfText(ttf12, connectaddress, &x, NULL);
				ttfPrintText(ttf12, subx1 + 12 + x, suby1 + 46, "_");
			}
		}
	}

	// communicating with clients
	if ( multiplayer == SERVER && mode )
	{
		//void *newSteamID = NULL; //TODO: Bugger void pointers!
#ifdef STEAMWORKS
		CSteamID newSteamID;
#endif
#if defined USE_EOS
		EOS_ProductUserId newRemoteProductId = nullptr;
#endif

		// hosting the lobby
		int numpacket;
		for ( numpacket = 0; numpacket < PACKET_LIMIT; numpacket++ )
		{
			if ( directConnect )
			{
				if ( !SDLNet_UDP_Recv(net_sock, net_packet) )
				{
					break;
				}
			}
			else
			{
				if ( LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM )
				{
#ifdef STEAMWORKS
					uint32_t packetlen = 0;
					if ( !SteamNetworking()->IsP2PPacketAvailable(&packetlen, 0) )
					{
						break;
					}
					packetlen = std::min<int>(packetlen, NET_PACKET_SIZE - 1);
					/*if ( newSteamID ) {
						cpp_Free_CSteamID( newSteamID );
						newSteamID = NULL;
					}*/
					//newSteamID = c_AllocateNew_CSteamID();
					Uint32 bytesRead = 0;
					if ( !SteamNetworking()->ReadP2PPacket(net_packet->data, packetlen, &bytesRead, &newSteamID, 0) )
					{
						continue;
					}
					net_packet->len = packetlen;
					if ( packetlen < sizeof(DWORD) )
					{
						continue;    // junk packet, skip //TODO: Investigate the cause of this. During earlier testing, we were getting bombarded with untold numbers of these malformed packets, as if the entire steam network were being routed through this game.
					}

					CSteamID mySteamID = SteamUser()->GetSteamID();
					if ( mySteamID.ConvertToUint64() == newSteamID.ConvertToUint64() )
					{
						continue;
					}
#endif
				}
				else if ( LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY )
				{
#if defined USE_EOS
					if ( !EOS.HandleReceivedMessages(&newRemoteProductId) )
					{
						continue;
					}
					EOS.P2PConnectionInfo.insertProductIdIntoPeers(newRemoteProductId);
#endif // USE_EOS
				}
			}

			if ( handleSafePacket() )
			{
				continue;
			}
			if (!strncmp((char*)net_packet->data, "BARONY_JOIN_REQUEST", 19))
			{
				int playerNum = MAXPLAYERS;
				if ( !directConnect )
				{
					if ( LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM )
					{
#ifdef STEAMWORKS
						bool skipJoin = false;
						for ( int c = 0; c < MAXPLAYERS; c++ )
						{
							if ( client_disconnected[c] || !steamIDRemote[c] )
							{
								continue;
							}
							if ( newSteamID.ConvertToUint64() == (static_cast<CSteamID*>(steamIDRemote[c]))->ConvertToUint64() )
							{
								// we've already accepted this player. NEXT!
								skipJoin = true;
								break;
							}
						}
						if ( skipJoin )
						{
							continue;
						}
#endif
					}
					else if ( LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY )
					{
#if defined USE_EOS
						bool skipJoin = false;
						/*for ( c = 0; c < MAXPLAYERS; c++ )
						{
							if ( client_disconnected[c] )
							{
								continue;
							}
						}*/
						EOSFuncs::logInfo("newRemoteProductId: %s", EOSFuncs::Helpers_t::productIdToString(newRemoteProductId));
						if ( newRemoteProductId && EOS.P2PConnectionInfo.isPeerIndexed(newRemoteProductId) )
						{
							if ( EOS.P2PConnectionInfo.getIndexFromPeerId(newRemoteProductId) >= 0 )
							{
								// we've already accepted this player. NEXT!
								skipJoin = true;
							}
						}
						if ( skipJoin )
						{
							continue;
						}
#endif // USE_EOS
					}
				}
				NetworkingLobbyJoinRequestResult result = lobbyPlayerJoinRequest(playerNum);
				if ( result == NetworkingLobbyJoinRequestResult::NET_LOBBY_JOIN_P2P_FAILURE )
				{
					if ( LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM )
					{
#ifdef STEAMWORKS
						for ( int responses = 0; responses < 5; ++responses )
						{
							SteamNetworking()->SendP2PPacket(newSteamID, net_packet->data, net_packet->len, k_EP2PSendReliable, 0);
							SDL_Delay(5);
						}
#endif
					}
					else if ( LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY )
					{
#if defined USE_EOS
						for ( int responses = 0; responses < 5; ++responses )
						{
							EOS.SendMessageP2P(newRemoteProductId, net_packet->data, net_packet->len);
							SDL_Delay(5);
						}
#endif
					}
				}
				else if ( result == NetworkingLobbyJoinRequestResult::NET_LOBBY_JOIN_P2P_SUCCESS )
				{
					if ( LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM )
					{
#ifdef STEAMWORKS
						if ( steamIDRemote[playerNum - 1] )
						{
							cpp_Free_CSteamID(steamIDRemote[playerNum - 1]);
						}
						steamIDRemote[playerNum - 1] = new CSteamID();
						*static_cast<CSteamID*>(steamIDRemote[playerNum - 1]) = newSteamID;
						for ( int responses = 0; responses < 5; ++responses )
						{
							SteamNetworking()->SendP2PPacket(*static_cast<CSteamID* >(steamIDRemote[playerNum - 1]), net_packet->data, net_packet->len, k_EP2PSendReliable, 0);
							SDL_Delay(5);
						}
#endif
					}
					else if ( LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY )
					{
#if defined USE_EOS
						EOS.P2PConnectionInfo.assignPeerIndex(newRemoteProductId, playerNum - 1);
						for ( int responses = 0; responses < 5; ++responses )
						{
							EOS.SendMessageP2P(EOS.P2PConnectionInfo.getPeerIdFromIndex(playerNum - 1), net_packet->data, net_packet->len);
							SDL_Delay(5);
						}
#endif
					}
				}
				continue;
			}

			// got a chat message
			else if (!strncmp((char*)net_packet->data, "CMSG", 4))
			{
				int i;
				for ( i = 1; i < MAXPLAYERS; i++ )
				{
					if ( client_disconnected[i] )
					{
						continue;
					}
					net_packet->address.host = net_clients[i - 1].host;
					net_packet->address.port = net_clients[i - 1].port;
					sendPacketSafe(net_sock, -1, net_packet, i - 1);
				}
				newString(&lobbyChatboxMessages, 0xFFFFFFFF, (char*)(&net_packet->data[4]));
				playSound(238, 64);
				continue;
			}

			// player disconnected
			else if (!strncmp((char*)net_packet->data, "PLAYERDISCONNECT", 16))
			{
				client_disconnected[net_packet->data[16]] = true;
				for ( int c = 1; c < MAXPLAYERS; c++ )
				{
					if ( client_disconnected[c] )
					{
						continue;
					}
					net_packet->address.host = net_clients[c - 1].host;
					net_packet->address.port = net_clients[c - 1].port;
					net_packet->len = 17;
					sendPacketSafe(net_sock, -1, net_packet, c - 1);
				}
				char shortname[32] = { 0 };
				strncpy(shortname, stats[net_packet->data[16]]->name, 22);
				newString(&lobbyChatboxMessages, 0xFFFFFFFF, language[1376], shortname);
				continue;
			}

			// client requesting new svFlags
			else if (!strncmp((char*)net_packet->data, "SVFL", 4))
			{
				// update svFlags for everyone
				SDLNet_Write32(svFlags, &net_packet->data[4]);
				net_packet->len = 8;

				for ( int c = 1; c < MAXPLAYERS; c++ )
				{
					if ( client_disconnected[c] )
					{
						continue;
					}
					net_packet->address.host = net_clients[c - 1].host;
					net_packet->address.port = net_clients[c - 1].port;
					sendPacketSafe(net_sock, -1, net_packet, c - 1);
				}
				continue;
			}

			// keepalive
			else if (!strncmp((char*)net_packet->data, "KEEPALIVE", 9))
			{
				client_keepalive[net_packet->data[9]] = ticks;
				continue; // just a keep alive
			}
		}
	}

	// communicating with server
	if ( multiplayer == CLIENT && mode )
	{
		if ( receivedclientnum == false )
		{
#ifdef STEAMWORKS
			CSteamID newSteamID;
			if ( !directConnect && LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM )
			{
				// waiting on host response.
				if ( joinLobbyWaitingForHostResponse )
				{
					// waiting on host response.
					if ( ticks - client_keepalive[0] >= 15 * TICKS_PER_SECOND ) // 15 second timeout
					{
						buttonDisconnect(nullptr);
						openFailedConnectionWindow(CLIENT);
						strcpy(subtext, LobbyHandler_t::getLobbyJoinFailedConnectString(static_cast<int>(LobbyHandler_t::LOBBY_JOIN_TIMEOUT)).c_str());
						connectingToLobbyStatus = EResult::k_EResultOK;
					}
				}
			}
#endif
#if defined USE_EOS
			EOS_ProductUserId newRemoteProductId = nullptr;
			if ( !directConnect && LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY )
			{
				if ( EOS.bJoinLobbyWaitingForHostResponse )
				{
					// waiting on host response.
					if ( ticks - client_keepalive[0] >= 15 * TICKS_PER_SECOND ) // 15 second timeout
					{
						buttonDisconnect(nullptr);
						openFailedConnectionWindow(CLIENT);
						strcpy(subtext, LobbyHandler_t::getLobbyJoinFailedConnectString(static_cast<int>(LobbyHandler_t::LOBBY_JOIN_TIMEOUT)).c_str());
						EOS.ConnectingToLobbyStatus = static_cast<int>(EOS_EResult::EOS_Success);
					}
				}
			}
#endif

			// trying to connect to the server and get a player number
			// receive the packet:
			bool gotPacket = false;
			if ( directConnect )
			{
				if ( SDLNet_TCP_Recv(net_tcpsock, net_packet->data, 4 + MAXPLAYERS * (5 + 23)) )
				{
					gotPacket = true;
				}
			}
			else if ( LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM )
			{
#ifdef STEAMWORKS
				for ( Uint32 numpacket = 0; numpacket < PACKET_LIMIT && net_packet; numpacket++ )
				{
					Uint32 packetlen = 0;
					if ( !SteamNetworking()->IsP2PPacketAvailable(&packetlen, 0) )
					{
						break;
					}
					packetlen = std::min<int>(packetlen, NET_PACKET_SIZE - 1);
					Uint32 bytesRead = 0;
					if ( !SteamNetworking()->ReadP2PPacket(net_packet->data, packetlen, &bytesRead, &newSteamID, 0) || bytesRead != 4 + MAXPLAYERS * (5 + 23) )
					{
						continue;
					}
					net_packet->len = packetlen;
					if ( packetlen < sizeof(DWORD) )
					{
						continue;
					}

					CSteamID mySteamID = SteamUser()->GetSteamID();
					if ( mySteamID.ConvertToUint64() == newSteamID.ConvertToUint64() )
					{
						continue;
					}
					if ( (int)net_packet->data[3] < '0'
						&& (int)net_packet->data[0] == 0
						&& (int)net_packet->data[1] == 0
						&& (int)net_packet->data[2] == 0 )
					{
						// data encoded with [0 0 0 clientnum] - directly sends an INT, if the character is < '0', then it is non-alphanumeric character.
						// likely not some other form of data - like an old "GOTP" from a recently closed session.
						gotPacket = true;
					}
					else
					{
						continue;
					}
					break;
				}
#endif
			}
			else if ( LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY )
			{
#ifdef USE_EOS
				for ( Uint32 numpacket = 0; numpacket < PACKET_LIMIT; numpacket++ )
				{
					if ( !EOS.HandleReceivedMessages(&newRemoteProductId) )
					{
						continue;
					}
					if ( (int)net_packet->data[3] < '0' 
						&& (int)net_packet->data[0] == 0 
						&& (int)net_packet->data[1] == 0
						&& (int)net_packet->data[2] == 0 )
					{
						// data encoded with [0 0 0 clientnum] - directly sends an INT, if the character is < '0', then it is non-alphanumeric character.
						// likely not some other form of data - like an old "GOTP" from a recently closed session.
						gotPacket = true;
					}
					else
					{
						continue;
					}
					break;
				}
#endif // USE_EOS
			}


			// parse the packet:
			if ( gotPacket )
			{
				list_FreeAll(&button_l);
				deleteallbuttons = true;

				//clientnum = (int)SDLNet_Read32(&net_packet->data[0]);
				clientnum = (int)net_packet->data[3];
				//printlog("%d %d %d %d", (int)net_packet->data[0], (int)net_packet->data[1], (int)net_packet->data[2], (int)net_packet->data[3]);

				if ( clientnum >= MAXPLAYERS || clientnum <= 0 )
				{
					printlog("connection attempt denied by server, error code: %d.\n", clientnum);
					multiplayer = SINGLE;
					if ( !directConnect )
					{
						if ( LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM )
						{
#ifdef STEAMWORKS
							// if we got a packet, flush any remaining packets from the queue.
							Uint32 startTicks = SDL_GetTicks();
							Uint32 checkTicks = startTicks;
							while ( (checkTicks - startTicks) < 2000 )
							{
								SteamAPI_RunCallbacks();
								Uint32 packetlen = 0;
								if ( SteamNetworking()->IsP2PPacketAvailable(&packetlen, 0) )
								{
									packetlen = std::min<int>(packetlen, NET_PACKET_SIZE - 1);
									Uint32 bytesRead = 0;
									char buffer[NET_PACKET_SIZE];
									if ( SteamNetworking()->ReadP2PPacket(buffer, packetlen, &bytesRead, &newSteamID, 0) )
									{
										checkTicks = SDL_GetTicks(); // found a packet, extend the wait time.
									}
									buffer[4] = '\0';
									if ( (int)buffer[3] < '0'
										&& (int)buffer[0] == 0
										&& (int)buffer[1] == 0
										&& (int)buffer[2] == 0 )
									{
										printlog("[Steam Lobby]: Clearing P2P packet queue: received: %d", (int)buffer[3]);
									}
									else
									{
										printlog("[Steam Lobby]: Clearing P2P packet queue: received: %s", buffer);
									}
								}
								SDL_Delay(10);
								if ( (SDL_GetTicks() - startTicks) > 5000 )
								{
									// hard break at 3 seconds.
									break;
								}
							}
#endif
						}
						else if ( LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY )
						{
#if defined USE_EOS
							// if we got a packet, flush any remaining packets from the queue.
							Uint32 startTicks = SDL_GetTicks();
							Uint32 checkTicks = startTicks;
							while ( (checkTicks - startTicks) < 2000 )
							{
								EOS_Platform_Tick(EOS.PlatformHandle);
								if ( EOS.HandleReceivedMessagesAndIgnore(&newRemoteProductId) )
								{
									checkTicks = SDL_GetTicks(); // found a packet, extend the wait time.
								}
								SDL_Delay(10);
								if ( (SDL_GetTicks() - startTicks) > 5000 )
								{
									// hard break at 3 seconds.
									break;
								}
							}
#endif // USE_EOS
						}
					}

					// close current window
					buttonCloseSubwindow(NULL);
					for ( node = button_l.first; node != NULL; node = nextnode )
					{
						nextnode = node->next;
						button = (button_t*)node->element;
						if ( button->focused )
						{
							list_RemoveNode(button->node);
						}
					}

#ifdef STEAMWORKS
					if ( !directConnect )
					{
						if ( currentLobby )
						{
							SteamMatchmaking()->LeaveLobby(*static_cast<CSteamID*>(currentLobby));
							cpp_Free_CSteamID( currentLobby ); //TODO: Bugger this.
							currentLobby = NULL;
						}
					}
#endif
#if defined USE_EOS
					if ( !directConnect )
					{
						if ( EOS.CurrentLobbyData.currentLobbyIsValid() )
						{
							EOS.leaveLobby();
						}
					}
#endif

					// create new window
					subwindow = 1;
					subx1 = xres / 2 - 256;
					subx2 = xres / 2 + 256;
					suby1 = yres / 2 - 48;
					suby2 = yres / 2 + 48;
					strcpy(subtext, language[1377]);
					if ( clientnum == MAXPLAYERS )
					{
						strcat(subtext, language[1378]);
					}
					else if ( clientnum == MAXPLAYERS + 1 )
					{
						strcat(subtext, language[1379]);
					}
					else if ( clientnum == MAXPLAYERS + 2 )
					{
						strcat(subtext, language[1380]);
					}
					else if ( clientnum == MAXPLAYERS + 3 )
					{
						strcat(subtext, language[1381]);
					}
					else if ( clientnum == MAXPLAYERS + 4 )
					{
						strcat(subtext, language[1382]);
					}
					else if ( clientnum == MAXPLAYERS + 5 )
					{
						strcat(subtext, language[1383]);
					}
					else
					{
						strcat(subtext, language[1384]);
					}
					clientnum = 0;

					// close button
					button = newButton();
					strcpy(button->label, "x");
					button->x = subx2 - 20;
					button->y = suby1;
					button->sizex = 20;
					button->sizey = 20;
					button->action = &buttonCloseSubwindow;
					button->visible = 1;
					button->focused = 1;
					button->key = SDL_SCANCODE_ESCAPE;
					button->joykey = joyimpulses[INJOY_MENU_CANCEL];

					// okay button
					button = newButton();
					strcpy(button->label, language[732]);
					button->x = subx2 - (subx2 - subx1) / 2 - 28;
					button->y = suby2 - 28;
					button->sizex = 56;
					button->sizey = 20;
					button->action = &buttonCloseSubwindow;
					button->visible = 1;
					button->focused = 1;
					button->key = SDL_SCANCODE_RETURN;
					button->joykey = joyimpulses[INJOY_MENU_NEXT];
				}
				else
				{
					// join game succeeded, advance to lobby
					client_keepalive[0] = ticks;
					receivedclientnum = true;
					printlog("connected to server.\n");
					client_disconnected[clientnum] = false;
					if ( !loadingsavegame )
					{
						stats[clientnum]->appearance = stats[0]->appearance;
					}

					// now set up everybody else
					for ( int c = 0; c < MAXPLAYERS; c++ )
					{
						client_disconnected[c] = false;
						client_classes[c] = net_packet->data[4 + c * (5 + 23)]; // class
						stats[c]->sex = static_cast<sex_t>(net_packet->data[5 + c * (5 + 23)]); // sex
						client_disconnected[c] = net_packet->data[6 + c * (5 + 23)]; // connectedness :p
						stats[c]->appearance = net_packet->data[7 + c * (5 + 23)]; // appearance
						stats[c]->playerRace = net_packet->data[8 + c * (5 + 23)]; // player race
						strcpy(stats[c]->name, (char*)(&net_packet->data[9 + c * (5 + 23)]));  // name
					}

					// request svFlags
					strcpy((char*)net_packet->data, "SVFL");
					net_packet->len = 4;
					net_packet->address.host = net_server.host;
					net_packet->address.port = net_server.port;
					sendPacketSafe(net_sock, -1, net_packet, 0);

					// open lobby window
					lobby_window = true;
					subwindow = 1;
					subx1 = xres / 2 - 400;
					subx2 = xres / 2 + 400;
#ifdef PANDORA
					suby1 = yres / 2 - ((yres==480)?230:290);
					suby2 = yres / 2 + ((yres==480)?230:290);
#else
					suby1 = yres / 2 - 300;
					suby2 = yres / 2 + 300;
#endif

					if ( directConnect )
					{
						strcpy(subtext, language[1385]);
						if ( !broadcast )
						{
							strcat(subtext, last_ip);
						}
						else
						{
							strcat(subtext, "HIDDEN FOR BROADCAST");
						}
					}
					else
					{
						strcpy(subtext, language[1386]);
					}
					strcat(subtext, language[1387]);

					// disconnect button
					button = newButton();
					strcpy(button->label, language[1311]);
					button->sizex = strlen(language[1311]) * 12 + 8;
					button->sizey = 20;
					button->x = subx1 + 4;
					button->y = suby2 - 24;
					button->action = &buttonDisconnect;
					button->visible = 1;
					button->focused = 1;
					button->joykey = joyimpulses[INJOY_MENU_CANCEL];
#ifdef STEAMWORKS
					if ( !directConnect && LobbyHandler.getJoiningType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM )
					{
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
				}
			}
		}
		else if ( multiplayer == CLIENT )
		{
#ifdef STEAMWORKS
			CSteamID newSteamID;
			joinLobbyWaitingForHostResponse = false;
#endif
#ifdef USE_EOS
			EOS.bJoinLobbyWaitingForHostResponse = false;
#endif
			int numpacket;
			for ( numpacket = 0; numpacket < PACKET_LIMIT && net_packet; numpacket++ )
			{
				if ( directConnect )
				{
					if ( !SDLNet_UDP_Recv(net_sock, net_packet) )
					{
						break;
					}
				}
				else
				{
					if ( LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM )
					{
#ifdef STEAMWORKS
						uint32_t packetlen = 0;
						if ( !SteamNetworking()->IsP2PPacketAvailable(&packetlen, 0) )
						{
							break;
						}
						packetlen = std::min<int>(packetlen, NET_PACKET_SIZE - 1);
						Uint32 bytesRead = 0;
						if ( !SteamNetworking()->ReadP2PPacket(net_packet->data, packetlen, &bytesRead, &newSteamID, 0) ) //TODO: Sometimes if a host closes a lobby, it can crash here for a client.
						{
							continue;
						}
						net_packet->len = packetlen;
						if ( packetlen < sizeof(DWORD) )
						{
							continue;    //TODO: Again, figure out why this is happening.
						}

						CSteamID mySteamID = SteamUser()->GetSteamID();
						if (mySteamID.ConvertToUint64() == newSteamID.ConvertToUint64())
						{
							continue;
						}
#endif
					}
					else if ( LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY )
					{
#if defined USE_EOS
						EOS_ProductUserId remoteId = nullptr;
						if ( !EOS.HandleReceivedMessages(&remoteId) )
						{
							continue;
						}
#endif // USE_EOS
					}
				}

				if ( handleSafePacket() )
				{
					continue;
				}

				// game start
				if (!strncmp((char*)net_packet->data, "BARONY_GAME_START", 17))
				{
					lobbyWindowSvFlags = SDLNet_Read32(&net_packet->data[17]);
					uniqueGameKey = SDLNet_Read32(&net_packet->data[21]);
					buttonCloseSubwindow(NULL);
					numplayers = MAXPLAYERS;
					introstage = 3;
					fadeout = true;
					if ( net_packet->data[25] == 0 )
					{
						loadingsavegame = 0;
					}
					continue;
				}

				// new player
				else if (!strncmp((char*)net_packet->data, "NEWPLAYER", 9))
				{
					client_disconnected[net_packet->data[9]] = false;
					client_classes[net_packet->data[9]] = net_packet->data[10];
					stats[net_packet->data[9]]->sex = static_cast<sex_t>(net_packet->data[11]);
					stats[net_packet->data[9]]->appearance = net_packet->data[12];
					stats[net_packet->data[9]]->playerRace = net_packet->data[13];
					strcpy(stats[net_packet->data[9]]->name, (char*)(&net_packet->data[14]));

					char shortname[32] = { 0 };
					strncpy(shortname, stats[net_packet->data[9]]->name, 22);
					newString(&lobbyChatboxMessages, 0xFFFFFFFF, language[1388], shortname);
					continue;
				}

				// player disconnect
				else if (!strncmp((char*)net_packet->data, "PLAYERDISCONNECT", 16) || !strncmp((char*)net_packet->data, "KICK", 4) )
				{
					int playerDisconnected = 0;
					if ( !strncmp((char*)net_packet->data, "KICK", 4) )
					{
						playerDisconnected = clientnum;
					}
					else
					{
						playerDisconnected = net_packet->data[16];
						client_disconnected[playerDisconnected] = true;
					}
					if ( playerDisconnected == clientnum || net_packet->data[16] == 0 )
					{
						// close lobby window
						buttonCloseSubwindow(NULL);
						for ( node = button_l.first; node != NULL; node = nextnode )
						{
							nextnode = node->next;
							button = (button_t*)node->element;
							if ( button->focused )
							{
								list_RemoveNode(button->node);
							}
						}

						// create new window
						subwindow = 1;
						subx1 = xres / 2 - 256;
						subx2 = xres / 2 + 256;
						suby1 = yres / 2 - 40;
						suby2 = yres / 2 + 40;

						if ( playerDisconnected == clientnum )
						{
							strcpy(subtext, language[1127]);
						}
						else
						{
							strcpy(subtext, language[1126]);
						}

						// close button
						button = newButton();
						strcpy(button->label, "x");
						button->x = subx2 - 20;
						button->y = suby1;
						button->sizex = 20;
						button->sizey = 20;
						button->action = &buttonCloseSubwindow;
						button->visible = 1;
						button->focused = 1;
						button->joykey = joyimpulses[INJOY_MENU_CANCEL];

						// okay button
						button = newButton();
						strcpy(button->label, language[732]);
						button->x = subx2 - (subx2 - subx1) / 2 - 20;
						button->y = suby2 - 24;
						button->sizex = 56;
						button->sizey = 20;
						button->action = &buttonCloseSubwindow;
						button->visible = 1;
						button->focused = 1;
						button->joykey = joyimpulses[INJOY_MENU_NEXT];

						// reset multiplayer status
						multiplayer = SINGLE;
						stats[0]->sex = stats[clientnum]->sex;
						client_classes[0] = client_classes[clientnum];
						strcpy(stats[0]->name, stats[clientnum]->name);
						clientnum = 0;
						client_disconnected[0] = false;
						for ( int c = 1; c < MAXPLAYERS; c++ )
						{
							client_disconnected[c] = true;
						}

						// close any existing net interfaces
						closeNetworkInterfaces();

#ifdef STEAMWORKS
						if ( !directConnect )
						{
							if ( currentLobby )
							{
								SteamMatchmaking()->LeaveLobby(*static_cast<CSteamID*>(currentLobby));
								cpp_Free_CSteamID(currentLobby); //TODO: Bugger this.
								currentLobby = NULL;
							}
						}
#endif
#if defined USE_EOS
						if ( !directConnect )
						{
							if ( EOS.CurrentLobbyData.currentLobbyIsValid() )
							{
								EOS.leaveLobby();
							}
						}
#endif
					}
					else
					{
						char shortname[32] = { 0 };
						strncpy(shortname, stats[net_packet->data[16]]->name, 22);
						newString(&lobbyChatboxMessages, 0xFFFFFFFF, language[1376], shortname);
					}
					continue;
				}

				// got a chat message
				else if (!strncmp((char*)net_packet->data, "CMSG", 4))
				{
					newString(&lobbyChatboxMessages, 0xFFFFFFFF, (char*)(&net_packet->data[4]));
					playSound(238, 64);
					continue;
				}

				// update svFlags
				else if (!strncmp((char*)net_packet->data, "SVFL", 4))
				{
					lobbyWindowSvFlags = SDLNet_Read32(&net_packet->data[4]);
					continue;
				}

				// keepalive
				else if (!strncmp((char*)net_packet->data, "KEEPALIVE", 9))
				{
					client_keepalive[0] = ticks;
					continue; // just a keep alive
				}
			}
		}
	}
	if ( multiplayer == SINGLE )
	{
		receivedclientnum = false;
	}

	// lobby window
	if ( lobby_window )
	{

		int hovering_selection = -1; //0 to NUM_SERVER_FLAGS used for the server flags settings, e.g. are traps, cheats, minotaur, etc enabled.
		SDL_Rect tooltip_box;

		// player info text
		for ( int c = 0; c < MAXPLAYERS; ++c )
		{
			if ( client_disconnected[c] )
			{
				continue;
			}
			string charDisplayName = "";
			charDisplayName = stats[c]->name;

			if ( !directConnect && LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM )
			{
#ifdef STEAMWORKS
				if ( c != clientnum )
				{
					for ( int remoteIDIndex = 0; remoteIDIndex < MAXPLAYERS; ++remoteIDIndex )
					{
						if ( steamIDRemote[remoteIDIndex] )
						{
							const char* memberNumChar = SteamMatchmaking()->GetLobbyMemberData(*static_cast<CSteamID*>(currentLobby), *static_cast<CSteamID*>(steamIDRemote[remoteIDIndex]), "clientnum");
							if ( memberNumChar )
							{
								std::string str = memberNumChar;
								if ( str.compare("") != 0 )
								{
									int memberNum = std::stoi(str);
									if ( memberNum >= 0 && memberNum < MAXPLAYERS && memberNum == c )
									{
										charDisplayName += " (";
										charDisplayName += SteamFriends()->GetFriendPersonaName(*static_cast<CSteamID*>(steamIDRemote[remoteIDIndex]));
										charDisplayName += ")";
									}
								}
							}
						}
					}
				}
				else
				{
					charDisplayName += " (";
					charDisplayName += SteamFriends()->GetPersonaName();
					charDisplayName += ")";

					if ( currentLobby )
					{
						if ( ticks % 10 == 0 )
						{
							const char* memberNumChar = SteamMatchmaking()->GetLobbyMemberData(*static_cast<CSteamID*>(currentLobby), SteamUser()->GetSteamID(), "clientnum");
							if ( memberNumChar )
							{
								std::string str = memberNumChar;
								if ( str.compare("") == 0 || str.compare(std::to_string(clientnum)) != 0 )
								{
									SteamMatchmaking()->SetLobbyMemberData(*static_cast<CSteamID*>(currentLobby), "clientnum", std::to_string(clientnum).c_str());
									printlog("[STEAM Lobbies]: Updating clientnum %d to lobby member data", clientnum);
								}
							}
						}
					}
				}
#endif
			}
			else if ( !directConnect && LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY )
			{
#if defined USE_EOS
				if ( c == clientnum )
				{
					charDisplayName += " (";
					charDisplayName += EOS.CurrentUserInfo.Name;
					charDisplayName += ")";

					if ( EOS.CurrentLobbyData.currentLobbyIsValid()
						&& EOS.CurrentLobbyData.getClientnumMemberAttribute(EOS.CurrentUserInfo.getProductUserIdHandle()) < 0 )
					{
						if ( EOS.CurrentLobbyData.assignClientnumMemberAttribute(EOS.CurrentUserInfo.getProductUserIdHandle(), clientnum) )
						{
							EOS.CurrentLobbyData.modifyLobbyMemberAttributeForCurrentUser();
						}
					}
				}
				else
				{
					for ( auto& player : EOS.CurrentLobbyData.playersInLobby )
					{
						if ( player.clientNumber == c )
						{
							charDisplayName += " (";
							charDisplayName += player.name;
							charDisplayName += ")";
						}
					}
				}
#endif
			}

			std::string raceAndClass = language[3161 + stats[c]->playerRace];
			raceAndClass += " ";
			if ( stats[c]->playerRace > RACE_HUMAN && stats[c]->appearance != 0 )
			{
				raceAndClass += "(aesthetic) ";
			}
			raceAndClass += playerClassLangEntry(client_classes[c], c);

			if ( charDisplayName.size() > 39 )
			{
				charDisplayName = charDisplayName.substr(0, 37);
				charDisplayName += "..";
			}

			if ( stats[c]->sex )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 8, suby1 + 80 + 60 * c, "%d:  %s\n    %s\n    %s", c + 1, charDisplayName.c_str(), language[1322], raceAndClass.c_str());
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 8, suby1 + 80 + 60 * c, "%d:  %s\n    %s\n    %s", c + 1, charDisplayName.c_str(), language[1321], raceAndClass.c_str());
			}
		}

		// select gui element w/ mouse
		if ( inputs.bMouseLeft(clientnum) )
		{
			if ( mouseInBounds(clientnum, subx1 + 16, subx2 - 16, suby2 - 48, suby2 - 32) )
			{
				inputs.mouseClearLeft(clientnum);

				// chatbox
				inputstr = lobbyChatbox;
				inputlen = LOBBY_CHATBOX_LENGTH - 1;
				cursorflash = ticks;
			}
			else if ( mouseInBounds(clientnum, xres / 2, subx2 - 32, suby1 + 56, suby1 + 68) && !directConnect )
			{
				inputs.mouseClearLeft(clientnum);

				// lobby name
				if ( LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM )
				{
#ifdef STEAMWORKS
					inputstr = currentLobbyName;
					inputlen = 31;
#endif
				}
				else if ( LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY )
				{
#if defined USE_EOS
					inputstr = EOS.currentLobbyName;
					inputlen = 31;
#endif
				}
				cursorflash = ticks;
			}

			// server flags
			int i;
			if ( multiplayer == SERVER )
			{
				for ( i = 0; i < NUM_SERVER_FLAGS; i++ )
				{
					if ( mouseInBounds(clientnum, xres / 2 + 8 + 6, xres / 2 + 8 + 30, suby1 + 80 + i * 16, suby1 + 92 + i * 16) )
					{
						inputs.mouseClearLeft(clientnum);

						// toggle flag
						svFlags ^= power(2, i);

						// update client flags
						strcpy((char*)net_packet->data, "SVFL");
						SDLNet_Write32(svFlags, &net_packet->data[4]);
						net_packet->len = 8;

						int c;
						for ( c = 1; c < MAXPLAYERS; c++ )
						{
							if ( client_disconnected[c] )
							{
								continue;
							}
							net_packet->address.host = net_clients[c - 1].host;
							net_packet->address.port = net_clients[c - 1].port;
							sendPacketSafe(net_sock, -1, net_packet, c - 1);
						}

						// update lobby data
						if ( !directConnect && LobbyHandler.getHostingType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM )
						{
#ifdef STEAMWORKS
							char svFlagsChar[16];
							snprintf(svFlagsChar, 15, "%d", svFlags);
							SteamMatchmaking()->SetLobbyData(*static_cast<CSteamID*>(currentLobby), "svFlags", svFlagsChar);
#endif
						}
					}
				}
			}

			// switch lobby type
			if ( multiplayer == SERVER && !directConnect )
			{
				if ( LobbyHandler.getHostingType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM )
				{
#ifdef STEAMWORKS
					for ( Uint32 i = 0; i < 2; i++ )
					{
						if ( mouseInBounds(clientnum, xres / 2 + 8 + 6, xres / 2 + 8 + 30, suby1 + 256 + i * 16, suby1 + 268 + i * 16) )
						{
							inputs.mouseClearLeft(clientnum);
							switch ( i )
							{
								default:
									currentLobbyType = k_ELobbyTypePrivate;
									break;
								case 1:
									currentLobbyType = k_ELobbyTypePublic;
									break;
								/*case 2:
									currentLobbyType = k_ELobbyTypeFriendsOnly;
									// deprecated by steam, doesn't return in getLobbyList.
									break;*/
							}
							SteamMatchmaking()->SetLobbyType(*static_cast<CSteamID*>(currentLobby), currentLobbyType);
						}
					}
#endif
				}
				else if ( LobbyHandler.getHostingType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY )
				{
#ifdef USE_EOS
					for ( Uint32 i = 0; i < 2; i++ )
					{
						if ( mouseInBounds(clientnum, xres / 2 + 8 + 6, xres / 2 + 8 + 30, suby1 + 256 + i * 16, suby1 + 268 + i * 16) )
						{
							inputs.mouseClearLeft(clientnum);
							switch ( i )
							{
								default:
									EOS.currentPermissionLevel = EOS_ELobbyPermissionLevel::EOS_LPL_JOINVIAPRESENCE;
									break;
								case 1:
									EOS.currentPermissionLevel = EOS_ELobbyPermissionLevel::EOS_LPL_PUBLICADVERTISED;
									break;
							}
						}
					}
#endif
				}
			}
		}

		// switch textboxes with TAB
		if ( multiplayer == SERVER )
		{
			if ( keystatus[SDL_SCANCODE_TAB] )
			{
				keystatus[SDL_SCANCODE_TAB] = 0;
				if ( !directConnect && LobbyHandler.getHostingType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM )
				{
#ifdef STEAMWORKS
					if ( inputstr == currentLobbyName )
					{
						inputstr = lobbyChatbox;
						inputlen = LOBBY_CHATBOX_LENGTH - 1;
					}
					else
					{
						inputstr = currentLobbyName;
						inputlen = 31;
					}
#endif
				}
				else if ( !directConnect && LobbyHandler.getHostingType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY )
				{
#if defined USE_EOS
					if ( inputstr == EOS.currentLobbyName )
					{
						inputstr = lobbyChatbox;
						inputlen = LOBBY_CHATBOX_LENGTH - 1;
					}
					else
					{
						inputstr = EOS.currentLobbyName;
						inputlen = 31;
					}
#endif
				}
			}
		}

		// server flag elements
		int i;
		for ( i = 0; i < NUM_SERVER_FLAGS; i++ )
		{
			char flagStringBuffer[256] = "";
			if ( i < 5 )
			{
				strncpy(flagStringBuffer, language[153 + i], 255);
			}
			else
			{
				strncpy(flagStringBuffer, language[2917 - 5 + i], 255);
			}

			Uint32 displayedSvFlags = svFlags;
			if ( multiplayer == CLIENT )
			{
				displayedSvFlags = lobbyWindowSvFlags;
			}

			if ( displayedSvFlags & power(2, i) )
			{
				ttfPrintTextFormatted(ttf12, xres / 2 + 8, suby1 + 80 + 16 * i, "[x] %s", flagStringBuffer);
			}
			else
			{
				ttfPrintTextFormatted(ttf12, xres / 2 + 8, suby1 + 80 + 16 * i, "[ ] %s", flagStringBuffer);
			}
			if (mouseInBounds(clientnum, (xres / 2) + 8 + 6, (xres / 2) + 8 + 30, suby1 + 80 + (i * 16), suby1 + 92 + (i * 16)))   //So many gosh dang magic numbers ._.
			{
				if ( i < 5 )
				{
					strncpy(flagStringBuffer, language[1942 + i], 255);
				}
				else
				{
					strncpy(flagStringBuffer, language[2921 - 5 + i], 255);
				}
				if (strlen(flagStringBuffer) > 0)   //Don't bother drawing a tooltip if the file doesn't say anything.
				{
					hovering_selection = i;
#if !defined STEAMWORKS && !defined USE_EOS
					if ( hovering_selection == 0 )
					{
						hovering_selection = -1; // don't show cheats tooltip about disabling achievements.
					}
#endif // STEAMWORKS
					tooltip_box.x = mousex - 256;
					tooltip_box.y = mousey + 8;
					tooltip_box.w = longestline(flagStringBuffer) * TTF12_WIDTH + 8; //MORE MAGIC NUMBERS. HNNGH. I can guess what they all do, but dang.
					tooltip_box.h = TTF12_HEIGHT + 8;
					if ( i == 2 || i == 3 || i == 5 || i == 6 || i == 7 )
					{
						tooltip_box.h = TTF12_HEIGHT * 2 + 8;
					}
					else if ( i == 4 || i == 8)
					{
						tooltip_box.h = TTF12_HEIGHT * 3 + 8;
					}
					else
					{
						tooltip_box.h = TTF12_HEIGHT + 8;
					}
				}
			}
		}

		// lobby type elements
		if ( !directConnect && LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM )
		{
#ifdef STEAMWORKS
			if ( multiplayer == SERVER )
			{
				for ( Uint32 i = 0; i < 2; i++ )
				{
					if ( (i == 0 && currentLobbyType == k_ELobbyTypePrivate)
						|| (i == 1 && currentLobbyType == k_ELobbyTypePublic) )
					{
						ttfPrintTextFormatted(ttf12, xres / 2 + 8, suby1 + 256 + 16 * i, "[o] %s", language[250 + i]);
					}
					else
					{
						ttfPrintTextFormatted(ttf12, xres / 2 + 8, suby1 + 256 + 16 * i, "[ ] %s", language[250 + i]);
					}
				}
			}
#endif
		}
		else if ( !directConnect && LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY )
		{
#ifdef USE_EOS
			for ( Uint32 i = 0; i < 2; i++ )
			{
				if ( (i == 0 && EOS.CurrentLobbyData.LobbyAttributes.PermissionLevel == static_cast<Uint32>(EOS_ELobbyPermissionLevel::EOS_LPL_JOINVIAPRESENCE))
					|| (i == 1 && EOS.CurrentLobbyData.LobbyAttributes.PermissionLevel == static_cast<Uint32>(EOS_ELobbyPermissionLevel::EOS_LPL_PUBLICADVERTISED)) )
				{
					ttfPrintTextFormatted(ttf12, xres / 2 + 8, suby1 + 256 + 16 * i, "[o] %s", language[250 + i]);
				}
				else
				{
					ttfPrintTextFormatted(ttf12, xres / 2 + 8, suby1 + 256 + 16 * i, "[ ] %s", language[250 + i]);
				}
			}

			if ( EOS.CurrentLobbyData.LobbyAttributes.gameJoinKey.compare("") != 0 )
			{
				ttfPrintTextFormatted(ttf12, xres / 2 + 8, suby1 + 256 + 16 * 3, "Lobby invite code: %s", EOS.CurrentLobbyData.LobbyAttributes.gameJoinKey.c_str());
			}
#endif
		}

		if ( !directConnect && LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM )
		{
#ifdef STEAMWORKS
			// server name
			drawDepressed(xres / 2, suby1 + 56, xres / 2 + 388, suby1 + 72);
			ttfPrintTextFormatted(ttf12, xres / 2 + 2, suby1 + 58, "%s", currentLobbyName);
			if ( inputstr == currentLobbyName )
			{
				if ( (ticks - cursorflash) % TICKS_PER_SECOND < TICKS_PER_SECOND / 2 )
				{
					int x;
					getSizeOfText(ttf12, currentLobbyName, &x, NULL);
					ttfPrintTextFormatted(ttf12, xres / 2 + 2 + x, suby1 + 58, "_");
				}
			}

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
			// server name
			drawDepressed(xres / 2, suby1 + 56, xres / 2 + 388, suby1 + 72);
			ttfPrintTextFormatted(ttf12, xres / 2 + 2, suby1 + 58, "%s", EOS.currentLobbyName);
			if ( inputstr == EOS.currentLobbyName )
			{
				if ( (ticks - cursorflash) % TICKS_PER_SECOND < TICKS_PER_SECOND / 2 )
				{
					int x;
					getSizeOfText(ttf12, EOS.currentLobbyName, &x, NULL);
					ttfPrintTextFormatted(ttf12, xres / 2 + 2 + x, suby1 + 58, "_");
				}
			}

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
				}
			}
#endif
		}

		// chatbox gui elements
		drawDepressed(subx1 + 16, suby2 - 256, subx2 - 16, suby2 - 48);
		drawDepressed(subx1 + 16, suby2 - 48, subx2 - 16, suby2 - 32);

		// draw chatbox main text
		int y = suby2 - 50;
		for ( c = 0; c < 20; c++ )
		{
			node_t* node = list_Node(&lobbyChatboxMessages, list_Size(&lobbyChatboxMessages) - c - 1);
			if ( node )
			{
				string_t* str = (string_t*)node->element;
				y -= str->lines * TTF12_HEIGHT;
				if ( y < suby2 - 254 )   // there were some tall messages and we're out of space
				{
					break;
				}
				ttfPrintTextFormatted(ttf12, subx1 + 18, y, str->data);
			}
			else
			{
				break;
			}
		}
		while ( list_Size(&lobbyChatboxMessages) > 20 )
		{
			// if there are too many messages to fit the chatbox, just cull them
			list_RemoveNode(lobbyChatboxMessages.first);
		}

		// handle chatbox text entry
		if ( !SDL_IsTextInputActive() )
		{
			//TODO: NX PORT: Does this need to be updated for the Switch? Even if only to just disable this feature entirely?
			// this is the default text entry box in this window.
			inputstr = lobbyChatbox;
			inputlen = LOBBY_CHATBOX_LENGTH - 1;
			SDL_StartTextInput();
		}
		if ( keystatus[SDL_SCANCODE_RETURN] && strlen(lobbyChatbox) > 0 )
		{
			keystatus[SDL_SCANCODE_RETURN] = 0;
			if ( multiplayer != CLIENT )
			{
				playSound(238, 64);
			}

			char shortname[32] = {0};
			strncpy(shortname, stats[clientnum]->name, 22);

			char msg[LOBBY_CHATBOX_LENGTH + 32] = { 0 };
			snprintf(msg, LOBBY_CHATBOX_LENGTH, "%s: %s", shortname, lobbyChatbox);
			if ( strlen(lobbyChatbox) > LOBBY_CHATBOX_LENGTH - strlen(shortname) - 2 )
			{
				msg[strlen(msg)] = '\n';
				int i;
				for ( i = 0; i < strlen(shortname) + 2; i++ )
				{
					snprintf((char*)(msg + strlen(msg)), (LOBBY_CHATBOX_LENGTH + 31) - strlen(msg), " ");
				}
				snprintf((char*)(msg + strlen(msg)), (LOBBY_CHATBOX_LENGTH + 31) - strlen(msg), "%s", (char*)(lobbyChatbox + LOBBY_CHATBOX_LENGTH - strlen(shortname) - 2));
			}
			if ( multiplayer != CLIENT )
			{
				newString(&lobbyChatboxMessages, 0xFFFFFFFF, msg);  // servers print their messages right away
			}

			int playerToKick = -1;
			bool skipMessageRelayToClients = false;
			if ( multiplayer == SERVER )
			{
				std::string chatboxStr = lobbyChatbox;
				if ( (chatboxStr.size() > strlen("/kick ")) 
					&& chatboxStr.find("/kick ") != std::string::npos && chatboxStr.find("/kick ") == size_t(0U) )
				{
					// find player num to kick
					chatboxStr = chatboxStr.substr(chatboxStr.find("/kick ") + strlen("/kick "));
					std::istringstream(chatboxStr) >> playerToKick;
					playerToKick -= 1;
					if ( playerToKick > 0 && playerToKick < MAXPLAYERS )
					{
						char kickMsg[LOBBY_CHATBOX_LENGTH + 32] = { 0 };
						if ( !client_disconnected[playerToKick] )
						{
							char clientShortName[32] = { 0 };
							strncpy(clientShortName, stats[playerToKick]->name, 22);
							snprintf(kickMsg, LOBBY_CHATBOX_LENGTH, language[279], playerToKick + 1, clientShortName);
							newString(&lobbyChatboxMessages, 0xFFFFFFFF, kickMsg);  // servers print their messages right away

							strcpy(msg, kickMsg);
						}
						else
						{
							newString(&lobbyChatboxMessages, 0xFFFFFFFF, "***   Invalid player to kick   ***");
							skipMessageRelayToClients = true;
							playerToKick = -1;
						}
					}
					else
					{
						newString(&lobbyChatboxMessages, 0xFFFFFFFF, "***   Invalid player to kick   ***");
						skipMessageRelayToClients = true;
						playerToKick = -1;
					}
				}
			}

			strcpy(lobbyChatbox, "");

			// send the message
			strcpy((char*)net_packet->data, "CMSG");
			strcat((char*)(net_packet->data), msg);
			net_packet->len = 4 + strlen(msg) + 1;
			net_packet->data[net_packet->len - 1] = 0;
			if ( multiplayer == CLIENT )
			{
				net_packet->address.host = net_server.host;
				net_packet->address.port = net_server.port;
				sendPacketSafe(net_sock, -1, net_packet, 0);
			}
			else if ( multiplayer == SERVER )
			{
				for ( int i = 1; i < MAXPLAYERS; i++ )
				{
					if ( client_disconnected[i] )
					{
						continue;
					}
					if ( playerToKick == i )
					{
						continue;
					}
					if ( skipMessageRelayToClients )
					{
						continue;
					}
					net_packet->address.host = net_clients[i - 1].host;
					net_packet->address.port = net_clients[i - 1].port;
					sendPacketSafe(net_sock, -1, net_packet, i - 1);
				}

				if ( playerToKick > 0 )
				{
					// send message to kicked player
					strcpy((char*)net_packet->data, "KICK");
					net_packet->address.host = net_clients[playerToKick - 1].host;
					net_packet->address.port = net_clients[playerToKick - 1].port;
					net_packet->len = 4;
					sendPacketSafe(net_sock, -1, net_packet, playerToKick - 1);

					client_disconnected[playerToKick] = true;
					// notify everyone else.
					for ( int i = 1; i < MAXPLAYERS; i++ )
					{
						if ( client_disconnected[i] )
						{
							continue;
						}
						strncpy((char*)(net_packet->data), "PLAYERDISCONNECT", 16);
						net_packet->data[16] = playerToKick;
						net_packet->address.host = net_clients[i - 1].host;
						net_packet->address.port = net_clients[i - 1].port;
						net_packet->len = 17;
						sendPacketSafe(net_sock, -1, net_packet, i - 1);
					}
					char shortname[32] = { 0 };
					strncpy(shortname, stats[playerToKick]->name, 22);
					newString(&lobbyChatboxMessages, 0xFFFFFFFF, language[1376], shortname);
				}
			}
		}

		// draw chatbox entry text and cursor
		ttfPrintTextFormatted(ttf12, subx1 + 18, suby2 - 46, ">%s", lobbyChatbox);
		if ( inputstr == lobbyChatbox )
		{
			if ( (ticks - cursorflash) % TICKS_PER_SECOND < TICKS_PER_SECOND / 2 )
			{
				int x;
				getSizeOfText(ttf12, lobbyChatbox, &x, NULL);
				ttfPrintTextFormatted(ttf12, subx1 + 18 + x + TTF12_WIDTH, suby2 - 46, "_");
			}
		}

		if (hovering_selection > -1)
		{
			drawTooltip(&tooltip_box);
			char flagStringBuffer[256] = "";
			if ( hovering_selection < 5 )
			{
				strncpy(flagStringBuffer, language[1942 + hovering_selection], 255);
			}
			else
			{
				strncpy(flagStringBuffer, language[2921 - 5 + hovering_selection], 255);
			}
			if (hovering_selection < NUM_SERVER_FLAGS)
			{
				ttfPrintTextFormatted(ttf12, tooltip_box.x + 4, tooltip_box.y + 4, flagStringBuffer);
			}
		}
#ifdef STEAMWORKS
		// draw server workshop mod list
		if ( !directConnect && currentLobby )
		{
			const char* serverNumModsChar = SteamMatchmaking()->GetLobbyData(*static_cast<CSteamID*>(currentLobby), "svNumMods");
			int serverNumModsLoaded = atoi(serverNumModsChar);
			if ( serverNumModsLoaded > 0 )
			{
				char tagName[32];
				std::vector<std::string> serverFileIdsLoaded;
				std::string modList = "Clients can click '";
				modList.append(language[2984]).append("' and \n'").append(language[2985]);
				modList.append("' to automatically subscribe and \n");
				modList.append("mount workshop items loaded in the host's lobby.\n\n");
				modList.append("All clients should be running the same mod load order\n");
				modList.append("to prevent any crashes or undefined behavior.\n\n");
				modList.append("Game client may need to be closed to install and detect\nnew subscriptions due to Workshop limitations.\n");
				int numToolboxLines = 9;
				bool itemNeedsSubscribing = false;
				bool itemNeedsMounting = false;
				Uint32 modsStatusColor = uint32ColorBaronyBlue(*mainsurface);
				bool modListOutOfOrder = false;
				for ( int lines = 0; lines < serverNumModsLoaded; ++lines )
				{
					snprintf(tagName, 32, "svMod%d", lines);
					const char* serverModFileID = SteamMatchmaking()->GetLobbyData(*static_cast<CSteamID*>(currentLobby), tagName);
					if ( strcmp(serverModFileID, "") )
					{
						if ( gamemodsCheckIfSubscribedAndDownloadedFileID(atoi(serverModFileID)) == false )
						{
							modList.append("Workshop item not subscribed or installed: ");
							modList.append(serverModFileID);
							modList.append("\n");
							itemNeedsSubscribing = true;
							++numToolboxLines;
						}
						else if ( gamemodsCheckFileIDInLoadedPaths(atoi(serverModFileID)) == false )
						{
							modList.append("Workshop item downloaded but not loaded: ");
							modList.append(serverModFileID);
							modList.append("\n");
							itemNeedsMounting = true;
							++numToolboxLines;
						}
						serverFileIdsLoaded.push_back(serverModFileID);
					}
				}
				if ( itemNeedsSubscribing )
				{
					for ( node = button_l.first; node != NULL; node = nextnode )
					{
						nextnode = node->next;
						button = (button_t*)node->element;
						if ( button->action == &buttonGamemodsMountHostsModFiles )
						{
							button->visible = 0;
						}
						if ( button->action == &buttonGamemodsSubscribeToHostsModFiles )
						{
							button->visible = 1;
						}
					}
				}
				else
				{
					if ( itemNeedsMounting )
					{
						modsStatusColor = uint32ColorOrange(*mainsurface);
						for ( node = button_l.first; node != NULL; node = nextnode )
						{
							nextnode = node->next;
							button = (button_t*)node->element;
							if ( button->action == &buttonGamemodsMountHostsModFiles )
							{
								button->visible = 1;
							}
							if ( button->action == &buttonGamemodsSubscribeToHostsModFiles )
							{
								button->visible = 0;
							}
						}
					}
					else if ( gamemodsIsClientLoadOrderMatchingHost(serverFileIdsLoaded)
						&& serverFileIdsLoaded.size() == gamemods_workshopLoadedFileIDMap.size() )
					{
						modsStatusColor = uint32ColorGreen(*mainsurface);
						for ( node = button_l.first; node != NULL; node = nextnode )
						{
							nextnode = node->next;
							button = (button_t*)node->element;
							if ( button->action == &buttonGamemodsMountHostsModFiles )
							{
								button->visible = 0;
							}
							if ( button->action == &buttonGamemodsSubscribeToHostsModFiles )
							{
								button->visible = 0;
							}
						}
					}
					else
					{
						modsStatusColor = uint32ColorOrange(*mainsurface);
						modListOutOfOrder = true;
						for ( node = button_l.first; node != NULL; node = nextnode )
						{
							nextnode = node->next;
							button = (button_t*)node->element;
							if ( button->action == &buttonGamemodsMountHostsModFiles )
							{
								button->visible = 1;
							}
							if ( button->action == &buttonGamemodsSubscribeToHostsModFiles )
							{
								button->visible = 0;
							}
						}
					}
				}
				ttfPrintTextFormattedColor(ttf12, xres / 2 + 8, suby1 + 304, modsStatusColor, "%2d mod(s) loaded by host (?)", serverNumModsLoaded);
				std::string modStatusString;
				if ( itemNeedsSubscribing )
				{
					modStatusString = "Your client is missing mods in subscriptions";
				}
				else if ( itemNeedsMounting )
				{
					modStatusString = "Your client is missing mods in load order";
				}
				else if ( modListOutOfOrder )
				{
					modStatusString = "Your client mod list is out of order";
				}
				else
				{
					modStatusString = "Your client has complete mod list";
				}

				int lineStartListLoadedMods = numToolboxLines;
				numToolboxLines += serverNumModsLoaded + 3;
				std::string clientModString = "Your client mod list:\n";
				for ( std::vector<std::pair<std::string, uint64>>::iterator it = gamemods_workshopLoadedFileIDMap.begin(); 
					it != gamemods_workshopLoadedFileIDMap.end(); ++it )
				{
					clientModString.append(std::to_string(it->second));
					clientModString.append("\n");
				}
				std::string serverModString = "Server mod list:\n";
				for ( std::vector<std::string>::iterator it = serverFileIdsLoaded.begin(); it != serverFileIdsLoaded.end(); ++it )
				{
					serverModString.append(*it);
					serverModString.append("\n");
				}

				ttfPrintTextFormattedColor(ttf12, xres / 2 + 8, suby1 + 320, modsStatusColor, "%s", modStatusString.c_str());
				if ( mouseInBounds(clientnum, xres / 2 + 8, xres / 2 + 8 + 31 * TTF12_WIDTH, suby1 + 304, suby1 + 320 + TTF12_HEIGHT) )
				{
					tooltip_box.w = 60 * TTF12_WIDTH + 8;
					tooltip_box.x = mousex - 16 - tooltip_box.w;
					tooltip_box.y = mousey + 8;
					tooltip_box.h = numToolboxLines * TTF12_HEIGHT + 8;
					drawTooltip(&tooltip_box);
					ttfPrintTextFormatted(ttf12, tooltip_box.x + 4, tooltip_box.y + 8, "%s", modList.c_str());
					ttfPrintTextFormattedColor(ttf12, tooltip_box.x + 4, tooltip_box.y + lineStartListLoadedMods * TTF12_HEIGHT + 16,
						modsStatusColor, "%s", clientModString.c_str());
					ttfPrintTextFormattedColor(ttf12, tooltip_box.x + 4 + 24 * TTF12_WIDTH, tooltip_box.y + lineStartListLoadedMods * TTF12_HEIGHT + 16, 
						modsStatusColor, "%s", serverModString.c_str());
				}
				if ( multiplayer == CLIENT && itemNeedsMounting )
				{
					if ( g_SteamWorkshop->subscribedCallStatus == 1 )
					{
						ttfPrintTextFormattedColor(ttf12, subx2 - 64 * TTF12_WIDTH, suby2 - 4 - TTF12_HEIGHT, uint32ColorOrange(*mainsurface), 
							"retrieving data...");
					}
					else if ( g_SteamWorkshop->subscribedCallStatus == 2 )
					{
						ttfPrintTextFormattedColor(ttf12, subx2 - 64 * TTF12_WIDTH, suby2 - 4 - TTF12_HEIGHT, uint32ColorOrange(*mainsurface),
							"please retry mount operation.");
					}
					else
					{
						ttfPrintTextFormattedColor(ttf12, subx2 - 64 * TTF12_WIDTH, suby2 - 4 - TTF12_HEIGHT, uint32ColorOrange(*mainsurface),
							"press mount button.");
					}
				}
			}
		}
#endif // STEAMWORKS

		// handle keepalive timeouts (lobby)
		if ( multiplayer == SERVER )
		{
			for ( int i = 1; i < MAXPLAYERS; i++ )
			{
				if ( client_disconnected[i] )
				{
					continue;
				}
				bool clientHasLostP2P = false;
				if ( !directConnect && LobbyHandler.getHostingType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM )
				{
#ifdef STEAMWORKS
					if ( !steamIDRemote[i - 1] )
					{
						clientHasLostP2P = true;
					}
#endif
				}
				else if ( !directConnect && LobbyHandler.getHostingType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY )
				{
#ifdef USE_EOS
					if ( !EOS.P2PConnectionInfo.isPeerStillValid(i - 1) )
					{
						clientHasLostP2P = true;
					}
#endif
				}
				if ( clientHasLostP2P || (ticks - client_keepalive[i] > TICKS_PER_SECOND * 30) )
				{
					client_disconnected[i] = true;
					strncpy((char*)(net_packet->data), "PLAYERDISCONNECT", 16);
					net_packet->data[16] = i;
					net_packet->len = 17;
					for ( int c = 1; c < MAXPLAYERS; c++ )
					{
						if ( client_disconnected[c] )
						{
							continue;
						}
						net_packet->address.host = net_clients[c - 1].host;
						net_packet->address.port = net_clients[c - 1].port;
						sendPacketSafe(net_sock, -1, net_packet, c - 1);
					}
					char shortname[32] = { 0 };
					strncpy(shortname, stats[i]->name, 22);
					newString(&lobbyChatboxMessages, 0xFFFFFFFF, language[1376], shortname);
					continue;
				}
			}
		}
		else if ( multiplayer == CLIENT )
		{
			bool hostHasLostP2P = false;
			if ( !directConnect && LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM )
			{
#ifdef STEAMWORKS
				if ( !steamIDRemote[0] )
				{
					hostHasLostP2P = true;
				}
#endif
			}
			else if ( !directConnect && LobbyHandler.getP2PType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY )
			{
#ifdef USE_EOS
				if ( !EOS.P2PConnectionInfo.isPeerStillValid(0) )
				{
					hostHasLostP2P = true;
				}
#endif
			}

			if ( hostHasLostP2P || (ticks - client_keepalive[0] > TICKS_PER_SECOND * 30) )
			{
				buttonDisconnect(NULL);
				openFailedConnectionWindow(3); // lost connection to server box
			}
		}

		// send keepalive messages every second
		if ( ticks % (TICKS_PER_SECOND * 1) == 0 && multiplayer != SINGLE )
		{
			strcpy((char*)net_packet->data, "KEEPALIVE");
			net_packet->data[9] = clientnum;
			net_packet->len = 10;
			if ( multiplayer == CLIENT )
			{
				net_packet->address.host = net_server.host;
				net_packet->address.port = net_server.port;
				sendPacketSafe(net_sock, -1, net_packet, 0);
			}
			else if ( multiplayer == SERVER )
			{
				int i;
				for ( i = 1; i < MAXPLAYERS; i++ )
				{
					if ( client_disconnected[i] )
					{
						continue;
					}
					net_packet->address.host = net_clients[i - 1].host;
					net_packet->address.port = net_clients[i - 1].port;
					sendPacketSafe(net_sock, -1, net_packet, i - 1);
				}
			}
		}
	}

	// leaderboards window
#ifdef STEAMWORKS
	if ( score_leaderboard_window != 0 && g_SteamLeaderboards )
	{
		int numEntriesToShow = 15;
		int filenameMaxLength = 48;
		int filename_padx = subx1 + 16;
		int filename_pady = suby1 + 32;
		int filename_padx2 = subx2 - 16 - 40;
		int filename_pady2 = filename_pady + numEntriesToShow * TTF12_HEIGHT + 8;
		int filename_rowHeight = TTF12_HEIGHT + 4;
		int numEntriesTotal = 0;

		ttfPrintTextFormattedColor(ttf16, filename_padx, filename_pady, uint32ColorWhite(*mainsurface), "%s", 
			g_SteamLeaderboards->leaderboardNames[g_SteamLeaderboards->LeaderboardView.boardToDownload].c_str());

		filename_pady += 3 * TTF12_HEIGHT;
		if ( !g_SteamLeaderboards->b_LeaderboardInit )
		{
			// waiting for leaderboard to be found...
		}
		else if ( g_SteamLeaderboards->b_LeaderboardInit && !g_SteamLeaderboards->b_ScoresDownloaded )
		{
			// wait for leaderboard to be downloaded...
			if ( score_leaderboard_window == 1 )
			{
				g_SteamLeaderboards->DownloadScores(g_SteamLeaderboards->LeaderboardView.requestType, g_SteamLeaderboards->LeaderboardView.rangeStart,
					g_SteamLeaderboards->LeaderboardView.rangeEnd);
				score_leaderboard_window = 2;
			}
			ttfPrintTextFormattedColor(ttf12, filename_padx, filename_pady + 2 * TTF12_HEIGHT, uint32ColorOrange(*mainsurface), "Downloading entries...");
		}
		else 
		{

			if ( g_SteamLeaderboards->b_ScoresDownloaded )
			{
				numEntriesTotal = g_SteamLeaderboards->m_nLeaderboardEntries;
				if ( numEntriesTotal <= 0 )
				{
					ttfPrintTextFormattedColor(ttf12, filename_padx, filename_pady + 2 * TTF12_HEIGHT, uint32ColorGreen(*mainsurface), "No Leaderboard entries for this category");
				}
			}

			SDL_Rect tooltip; // we will draw the tooltip after drawing the other elements of the display window.

			tooltip.x = omousex + 8;
			tooltip.y = omousey + 8;
			tooltip.w = 32 + TTF12_WIDTH * 14;
			tooltip.h = TTF12_HEIGHT + 8;

			filename_pady += 2 * TTF12_HEIGHT;

			// do slider
			SDL_Rect slider;
			slider.x = filename_padx2 + 8;
			slider.y = filename_pady - 8;
			slider.h = suby2 - (filename_pady + 20);
			slider.w = 32;

			int entriesToScroll = std::max(static_cast<int>((numEntriesTotal / numEntriesToShow) - 1), 0);
			entriesToScroll = entriesToScroll * numEntriesToShow + (numEntriesTotal % numEntriesToShow);

			bool drawScrollTooltip = false;

			// handle slider movement.
			if ( numEntriesTotal > numEntriesToShow )
			{
				drawRect(&slider, SDL_MapRGB(mainsurface->format, 64, 64, 64), 255);
				if ( mouseInBounds(clientnum, filename_padx, slider.x + slider.w,
					slider.y, slider.y + slider.h) )
				{
					if ( mouseInBounds(clientnum, slider.x, slider.x + slider.w,
						slider.y, slider.y + slider.h) )
					{
						drawScrollTooltip = true;
					}
					if ( mousestatus[SDL_BUTTON_WHEELUP] )
					{
						g_SteamLeaderboards->LeaderboardView.scrollIndex = std::max(g_SteamLeaderboards->LeaderboardView.scrollIndex - 1, 0);
						mousestatus[SDL_BUTTON_WHEELUP] = 0;
					}
					if ( mousestatus[SDL_BUTTON_WHEELDOWN] )
					{
						g_SteamLeaderboards->LeaderboardView.scrollIndex = std::min(g_SteamLeaderboards->LeaderboardView.scrollIndex + 1, entriesToScroll);
						mousestatus[SDL_BUTTON_WHEELDOWN] = 0;
					}
				}

				if ( keystatus[SDL_SCANCODE_UP] )
				{
					g_SteamLeaderboards->LeaderboardView.scrollIndex = std::max(g_SteamLeaderboards->LeaderboardView.scrollIndex - 1, 0);
					keystatus[SDL_SCANCODE_UP] = 0;
				}
				if ( keystatus[SDL_SCANCODE_DOWN] )
				{
					g_SteamLeaderboards->LeaderboardView.scrollIndex = std::min(g_SteamLeaderboards->LeaderboardView.scrollIndex + 1, entriesToScroll);
					keystatus[SDL_SCANCODE_DOWN] = 0;
				}
				slider.h *= (1 / static_cast<real_t>(entriesToScroll + 1));
				slider.y += slider.h * savegames_window_scroll;
				if ( g_SteamLeaderboards->LeaderboardView.scrollIndex == entriesToScroll ) // reached end.
				{
					slider.y += (suby2 - 28) - (slider.y + slider.h); // bottom of slider is (suby2 - 28), so move the y level to imitate hitting the bottom in case of rounding error.
				}
				drawWindowFancy(slider.x, slider.y, slider.x + slider.w, slider.y + slider.h); // draw shortened list relative slider.
			}
			else
			{
				//drawRect(&slider, SDL_MapRGB(mainsurface->format, 64, 64, 64), 255);
				drawWindowFancy(slider.x, slider.y, slider.x + slider.w, slider.y + slider.h);
			}

			// draw the content
			for ( int i = 0; i < numEntriesTotal; ++i )
			{
				filename_padx = subx1 + 16;
				if ( i >= g_SteamLeaderboards->LeaderboardView.scrollIndex && i < numEntriesToShow + g_SteamLeaderboards->LeaderboardView.scrollIndex )
				{
					drawWindowFancy(filename_padx, filename_pady - 8, filename_padx2, filename_pady + filename_rowHeight);
					SDL_Rect highlightEntry;
					highlightEntry.x = filename_padx;
					highlightEntry.y = filename_pady - 8;
					highlightEntry.w = filename_padx2 - filename_padx;
					highlightEntry.h = filename_rowHeight + 8;
					drawRect(&highlightEntry, uint32ColorBaronyBlue(*mainsurface), 64);

					char steamID[32] = "";
					if ( strlen(SteamFriends()->GetFriendPersonaName(g_SteamLeaderboards->m_leaderboardEntries[i].m_steamIDUser)) > 18 )
					{
						strncpy(steamID, SteamFriends()->GetFriendPersonaName(g_SteamLeaderboards->m_leaderboardEntries[i].m_steamIDUser), 16);
						strcat(steamID, "..");
					}
					else
					{
						strncpy(steamID, SteamFriends()->GetFriendPersonaName(g_SteamLeaderboards->m_leaderboardEntries[i].m_steamIDUser), 18);
					}
					if ( g_SteamLeaderboards->LeaderboardView.boardToDownload != LEADERBOARD_NONE
						&& g_SteamLeaderboards->LeaderboardView.boardToDownload % 2 == 1 )
					{
						Uint32 sec = (g_SteamLeaderboards->m_leaderboardEntries[i].m_nScore) % 60;
						Uint32 min = ((g_SteamLeaderboards->m_leaderboardEntries[i].m_nScore) / 60) % 60;
						Uint32 hour = ((g_SteamLeaderboards->m_leaderboardEntries[i].m_nScore) / 60) / 60;

						ttfPrintTextFormatted(ttf12, filename_padx + 8, filename_pady, "#%2d [%18s]:   Time: %02d:%02d:%02d  Score: %6d",
							g_SteamLeaderboards->m_leaderboardEntries[i].m_nGlobalRank,
							steamID, hour, min, sec, g_SteamLeaderboards->downloadedTags[i][TAG_TOTAL_SCORE]);
					}
					else
					{
						Uint32 sec = (g_SteamLeaderboards->downloadedTags[i][TAG_COMPLETION_TIME]) % 60;
						Uint32 min = ((g_SteamLeaderboards->downloadedTags[i][TAG_COMPLETION_TIME]) / 60) % 60;
						Uint32 hour = ((g_SteamLeaderboards->downloadedTags[i][TAG_COMPLETION_TIME]) / 60) / 60;
						ttfPrintTextFormatted(ttf12, filename_padx + 8, filename_pady, "#%2d [%18s]:   Score: %6d  Time: %02d:%02d:%02d",
							g_SteamLeaderboards->m_leaderboardEntries[i].m_nGlobalRank,
							steamID, g_SteamLeaderboards->m_leaderboardEntries[i].m_nScore, hour, min, sec);
					}

					filename_padx = filename_padx2 - (15 * TTF12_WIDTH + 16);
					int text_x = filename_padx;
					int text_y = filename_pady;
					if ( drawClickableButton(filename_padx, filename_pady, 15 * TTF12_WIDTH + 8, TTF12_HEIGHT, 0) && score_leaderboard_window != 3 )
					{
						score_leaderboard_window = 3;
						g_SteamLeaderboards->currentLeaderBoardIndex = i;
						steamLeaderboardReadScore(g_SteamLeaderboards->downloadedTags[g_SteamLeaderboards->currentLeaderBoardIndex]);
						for ( node = button_l.first; node != NULL; node = node->next )
						{
							button = (button_t*)node->element;
							button->visible = 0;
						}
					}
					ttfPrintTextFormatted(ttf12, text_x + 8, text_y, "%s", "View character");

					filename_padx = filename_padx2 - (2 * TTF12_WIDTH + 14);
					text_x = filename_padx;

					filename_pady += 3 * filename_rowHeight / 2;
				}
			}
		}
	}
#endif
	// statistics window
	if ( score_window || score_leaderboard_window == 3 )
	{
		if ( score_leaderboard_window == 3 )
		{
			drawWindowFancy(subx1 + 10, suby1 + 20, subx2 - 10, suby2 - 10);
			if ( drawClickableButton(subx2 - 10 * TTF12_WIDTH - 3, suby1 + 20 + 8, 8 * TTF12_WIDTH, TTF12_HEIGHT, 0) )
			{
				score_leaderboard_window = 2;
				for ( node = button_l.first; node != NULL; node = node->next )
				{
					button = (button_t*)node->element;
					button->visible = 1;
				}
			}
			ttfPrintTextFormatted(ttf12, subx2 - 10 * TTF12_WIDTH + 9, suby1 + 20 + 8, "close");
		}
		// draw button label... shamelessly hacked together from "multiplayer scores toggle button" initialisation...
		int toggleText_x = subx2 - 44 - strlen("show multiplayer") * 12;
		int toggleText_y = suby1 + 4 ;
		int w = 0;
		int h = 0;
		list_t* scoresPtr = &topscores;

		if ( !score_leaderboard_window )
		{
			if ( !scoreDisplayMultiplayer )
			{
				getSizeOfText(ttf12, "show multiplayer", &w, &h);
				ttfPrintText(ttf12, toggleText_x + (strlen("show multiplayer") * 12 + 8 - w) / 2, toggleText_y + (20 - h) / 2 + 3, "show multiplayer");
			}
			else
			{
				scoresPtr = &topscoresMultiplayer;
				getSizeOfText(ttf12, "show solo", &w, &h);
				ttfPrintText(ttf12, toggleText_x + (strlen("show multiplayer") * 12 + 8 - w) / 2, toggleText_y + (20 - h) / 2 + 3, "show solo");
			}
		}
		else
		{
			scoresPtr = nullptr;
		}

		if ( !list_Size(scoresPtr) && !score_leaderboard_window )
		{
#define NOSCORESSTR language[1389]
			ttfPrintTextFormatted(ttf16, xres / 2 - strlen(NOSCORESSTR) * 9, yres / 2 - 9, NOSCORESSTR);
		}
		else
		{
			if ( !score_leaderboard_window )
			{
				if ( scoreDisplayMultiplayer )
				{
					ttfPrintTextFormatted(ttf16, subx1 + 8, suby1 + 8, "%s - %d / %d", language[2958], score_window, list_Size(&topscoresMultiplayer));
				}
				else
				{
					ttfPrintTextFormatted(ttf16, subx1 + 8, suby1 + 8, "%s - %d / %d", language[1390], score_window, list_Size(&topscores));
				}
			}

			// draw character window
			if (players[clientnum] != nullptr && players[clientnum]->entity != nullptr)
			{
				camera_charsheet.x = players[clientnum]->entity->x / 16.0 + 1.118 * cos(camera_charsheet_offsetyaw); // + 1
				camera_charsheet.y = players[clientnum]->entity->y / 16.0 + 1.118 * sin(camera_charsheet_offsetyaw); // -.5
				camera_charsheet.z = players[clientnum]->entity->z * 2;
				camera_charsheet.ang = atan2(players[clientnum]->entity->y / 16.0 - camera_charsheet.y, players[clientnum]->entity->x / 16.0 - camera_charsheet.x);
				camera_charsheet.vang = PI / 24;
				camera_charsheet.winw = 400;
				camera_charsheet.winy = suby1 + 32;
				camera_charsheet.winh = suby2 - 96 - camera_charsheet.winy;
				camera_charsheet.winx = subx1 + 32;
				SDL_Rect pos;
				pos.x = camera_charsheet.winx;
				pos.y = camera_charsheet.winy;
				pos.w = camera_charsheet.winw;
				pos.h = camera_charsheet.winh;
				drawRect(&pos, 0, 255);
				b = players[clientnum]->entity->flags[BRIGHT];
				players[clientnum]->entity->flags[BRIGHT] = true;
				if ( !players[clientnum]->entity->flags[INVISIBLE] )
				{
					real_t ofov = fov;
					fov = 50;
					glDrawVoxel(&camera_charsheet, players[clientnum]->entity, REALCOLORS);
					fov = ofov;
				}
				players[clientnum]->entity->flags[BRIGHT] = b;
				c = 0;
				for ( node = players[clientnum]->entity->children.first; node != NULL; node = node->next )
				{
					if ( c == 0 )
					{
						c++;
					}
					entity = (Entity*) node->element;
					if ( !entity->flags[INVISIBLE] )
					{
						b = entity->flags[BRIGHT];
						entity->flags[BRIGHT] = true;
						real_t ofov = fov;
						fov = 50;
						glDrawVoxel(&camera_charsheet, entity, REALCOLORS);
						fov = ofov;
						entity->flags[BRIGHT] = b;
					}
					c++;
				}
				SDL_Rect rotateBtn;
				rotateBtn.w = 24;
				rotateBtn.h = 24;
				rotateBtn.x = camera_charsheet.winx + camera_charsheet.winw - rotateBtn.w;
				rotateBtn.y = camera_charsheet.winy + camera_charsheet.winh - rotateBtn.h;
				drawWindow(rotateBtn.x, rotateBtn.y, rotateBtn.x + rotateBtn.w, rotateBtn.y + rotateBtn.h);
				if ( mouseInBounds(clientnum, rotateBtn.x, rotateBtn.x + rotateBtn.w, rotateBtn.y, rotateBtn.y + rotateBtn.h) )
				{
					if ( inputs.bMouseLeft(clientnum) )
					{
						camera_charsheet_offsetyaw += 0.05;
						if ( camera_charsheet_offsetyaw > 2 * PI )
						{
							camera_charsheet_offsetyaw -= 2 * PI;
						}
						drawDepressed(rotateBtn.x, rotateBtn.y, rotateBtn.x + rotateBtn.w, rotateBtn.y + rotateBtn.h);
					}
				}
				ttfPrintText(ttf12, rotateBtn.x + 6, rotateBtn.y + 6, ">");

				rotateBtn.x = camera_charsheet.winx + camera_charsheet.winw - rotateBtn.w * 2 - 4;
				rotateBtn.y = camera_charsheet.winy + camera_charsheet.winh - rotateBtn.h;
				drawWindow(rotateBtn.x, rotateBtn.y, rotateBtn.x + rotateBtn.w, rotateBtn.y + rotateBtn.h);
				if ( mouseInBounds(clientnum, rotateBtn.x, rotateBtn.x + rotateBtn.w, rotateBtn.y, rotateBtn.y + rotateBtn.h) )
				{
					if ( inputs.bMouseLeft(clientnum) )
					{
						camera_charsheet_offsetyaw -= 0.05;
						if ( camera_charsheet_offsetyaw < 0.f )
						{
							camera_charsheet_offsetyaw += 2 * PI;
						}
						drawDepressed(rotateBtn.x, rotateBtn.y, rotateBtn.x + rotateBtn.w, rotateBtn.y + rotateBtn.h);
					}
				}
				ttfPrintText(ttf12, rotateBtn.x + 4, rotateBtn.y + 6, "<");
			}

			// print name and class
			if ( victory )
			{
				ttfPrintTextFormatted(ttf16, subx1 + 448, suby1 + 40, language[1391]);
				ttfPrintTextFormatted(ttf16, subx1 + 448, suby1 + 56, "%s", stats[clientnum]->name);
				int creature = HUMAN;
				if ( players[clientnum] && players[clientnum]->entity )
				{
					creature = static_cast<int>(players[clientnum]->entity->getMonsterFromPlayerRace(stats[clientnum]->playerRace));
				}
				if ( victory == 1 )
				{
					if ( creature != HUMAN )
					{
						ttfPrintTextFormatted(ttf16, subx1 + 448, suby1 + 72, language[3359], monstertypenamecapitalized[creature]);
					}
					else
					{
						ttfPrintTextFormatted(ttf16, subx1 + 448, suby1 + 72, language[1392]);
					}
				}
				else if ( victory == 2 )
				{
					if ( creature != HUMAN )
					{
						ttfPrintTextFormatted(ttf16, subx1 + 448, suby1 + 72, language[3360], monstertypenamecapitalized[creature]);
					}
					else
					{
						ttfPrintTextFormatted(ttf16, subx1 + 448, suby1 + 72, language[1393]);
					}
				}
				else if ( victory == 3 )
				{
					if ( creature != HUMAN )
					{
						ttfPrintTextFormatted(ttf16, subx1 + 448, suby1 + 72, language[3361], 
							language[3363 - 1 + stats[clientnum]->playerRace]);
					}
					else
					{
						ttfPrintTextFormatted(ttf16, subx1 + 448, suby1 + 72, language[2911]);
					}
				}
			}
			else
			{
				ttfPrintTextFormatted(ttf16, subx1 + 448, suby1 + 40, language[1394]);
				ttfPrintTextFormatted(ttf16, subx1 + 448, suby1 + 56, "%s", stats[clientnum]->name);

				char classname[64];
				strcpy(classname, playerClassLangEntry(client_classes[0], clientnum));
				classname[0] -= 32;
				int creature = HUMAN;
				if ( players[clientnum] && players[clientnum]->entity )
				{
					creature = static_cast<int>(players[clientnum]->entity->getMonsterFromPlayerRace(stats[clientnum]->playerRace));
				}
				if ( creature != HUMAN )
				{
					ttfPrintTextFormatted(ttf16, subx1 + 448, suby1 + 72, language[3362], monstertypenamecapitalized[creature], classname);
				}
				else
				{
					ttfPrintTextFormatted(ttf16, subx1 + 448, suby1 + 72, language[1395], classname);
				}
			}

			// print total score
			if ( score_leaderboard_window != 3 )
			{
				node = list_Node(scoresPtr, score_window - 1);
				if ( node )
				{
					score_t* score = (score_t*)node->element;
					ttfPrintTextFormatted(ttf16, subx1 + 448, suby1 + 104, language[1404], totalScore(score));
				}
			}
			else
			{
#ifdef STEAMWORKS
				ttfPrintTextFormatted(ttf16, subx1 + 448, suby1 + 104, language[1404], g_SteamLeaderboards->downloadedTags[g_SteamLeaderboards->currentLeaderBoardIndex][TAG_TOTAL_SCORE]);
#endif // STEAMWORKS
			}

			Entity* playerEntity = nullptr;
			if ( players[clientnum] )
			{
				playerEntity = players[clientnum]->entity;
			}

			// print character stats
			ttfPrintTextFormatted(ttf12, subx1 + 456, suby1 + 128, language[359], stats[clientnum]->LVL, playerClassLangEntry(client_classes[clientnum], clientnum));
			ttfPrintTextFormatted(ttf12, subx1 + 456, suby1 + 140, language[1396], stats[clientnum]->EXP);
			ttfPrintTextFormatted(ttf12, subx1 + 456, suby1 + 152, language[1397], stats[clientnum]->GOLD);
			ttfPrintTextFormatted(ttf12, subx1 + 456, suby1 + 164, language[361], currentlevel);

			ttfPrintTextFormatted(ttf12, subx1 + 456, suby1 + 188, language[1398], statGetSTR(stats[clientnum], playerEntity), stats[clientnum]->STR);
			ttfPrintTextFormatted(ttf12, subx1 + 456, suby1 + 200, language[1399], statGetDEX(stats[clientnum], playerEntity), stats[clientnum]->DEX);
			ttfPrintTextFormatted(ttf12, subx1 + 456, suby1 + 212, language[1400], statGetCON(stats[clientnum], playerEntity), stats[clientnum]->CON);
			ttfPrintTextFormatted(ttf12, subx1 + 456, suby1 + 224, language[1401], statGetINT(stats[clientnum], playerEntity), stats[clientnum]->INT);
			ttfPrintTextFormatted(ttf12, subx1 + 456, suby1 + 236, language[1402], statGetPER(stats[clientnum], playerEntity), stats[clientnum]->PER);
			ttfPrintTextFormatted(ttf12, subx1 + 456, suby1 + 248, language[1403], statGetCHR(stats[clientnum], playerEntity), stats[clientnum]->CHR);

			// time
			Uint32 sec = (completionTime / TICKS_PER_SECOND) % 60;
			Uint32 min = ((completionTime / TICKS_PER_SECOND) / 60) % 60;
			Uint32 hour = ((completionTime / TICKS_PER_SECOND) / 60) / 60;
			ttfPrintTextFormatted(ttf12, subx1 + 32, suby2 - 80, "%s: %02d:%02d:%02d. %s:", language[1405], hour, min, sec, language[1406]);
			if ( !conductPenniless && !conductFoodless && !conductVegetarian && !conductIlliterate && !conductGameChallenges[CONDUCT_HARDCORE]
				&& !conductGameChallenges[CONDUCT_CHEATS_ENABLED]
				&& !conductGameChallenges[CONDUCT_MODDED]
				&& !conductGameChallenges[CONDUCT_LIFESAVING]
				&& !conductGameChallenges[CONDUCT_KEEPINVENTORY]
				&& !conductGameChallenges[CONDUCT_BRAWLER]
				&& !conductGameChallenges[CONDUCT_RANGED_ONLY]
				&& !conductGameChallenges[CONDUCT_BLESSED_BOOTS_SPEED]
				&& !conductGameChallenges[CONDUCT_BOOTS_SPEED]
				&& !conductGameChallenges[CONDUCT_ACCURSED]
				&& !conductGameChallenges[CONDUCT_MULTIPLAYER])
			{
				ttfPrintText(ttf12, subx1 + 32, suby2 - 64, language[1407]);
			}
			else
			{
				int b = 0;
				strcpy(tempstr, " ");
				if ( conductPenniless )
				{
					strcat(tempstr, language[1408]);
					++b;
				}
				if ( conductFoodless )
				{
					if ( b > 0 )
					{
						strcat(tempstr, ", ");
					}
					strcat(tempstr, language[1409]);
					++b;
				}
				if ( conductVegetarian )
				{
					if ( b > 0 )
					{
						strcat(tempstr, ", ");
					}
					strcat(tempstr, language[1410]);
					++b;
				}
				if ( conductIlliterate )
				{
					if ( b > 0 )
					{
						strcat(tempstr, ", ");
					}
					strcat(tempstr, language[1411]);
					++b;
				}
				for ( int c = 0; c < NUM_CONDUCT_CHALLENGES; ++c )
				{
					if ( conductGameChallenges[c] != 0 )
					{
						if ( b > 0 )
						{
							strcat(tempstr, ", ");
						}
						if ( b > 0 && b % 4 == 0 )
						{
							strcat(tempstr, "\n ");
						}
						strcat(tempstr, language[2925 + c]);
						++b;
					}
				}
				/*	if ( b > 0 )
				{
					tempstr[strlen(tempstr) - 2] = 0;
				}*/
				ttfPrintTextFormatted(ttf12, subx1 + 20, suby2 - 64, tempstr);
			}

			// kills
			int x = 0, y = 0;
			ttfPrintText(ttf12, subx1 + 456, suby1 + 272, language[1412]);
			bool nokills = true;
			for ( x = 0; x < NUMMONSTERS; x++ )
			{
				if ( kills[x] )
				{
					nokills = false;
					if ( kills[x] > 1 )
					{
						if ( x < KOBOLD )
						{
							ttfPrintTextFormatted(ttf12, subx1 + 456 + (y / 14) * 156, suby1 + 296 + (y % 14) * 12, "%d %s", kills[x], language[111 + x]);
						}
						else
						{
							ttfPrintTextFormatted(ttf12, subx1 + 456 + (y / 14) * 156, suby1 + 296 + (y % 14) * 12, "%d %s", kills[x], language[2050 + (x - KOBOLD)]);
						}
					}
					else
					{
						if ( x < KOBOLD )
						{
							ttfPrintTextFormatted(ttf12, subx1 + 456 + (y / 14) * 156, suby1 + 296 + (y % 14) * 12, "%d %s", kills[x], language[90 + x]);
						}
						else
						{
							ttfPrintTextFormatted(ttf12, subx1 + 456 + (y / 14) * 156, suby1 + 296 + (y % 14) * 12, "%d %s", kills[x], language[2000 + (x - KOBOLD)]);
						}
					}
					y++;
				}
			}
			if ( nokills )
			{
				ttfPrintText(ttf12, subx1 + 456, suby1 + 296, language[1413]);
			}
		}
	}
	else
	{
		scoreDisplayMultiplayer = false;
	}

	if ( savegames_window != 0 )
	{
		int numSavesToShow = 5;
		int filenameMaxLength = 48;
		int filename_padx = subx1 + 16;
		int filename_pady = suby1 + 32;
		int filename_padx2 = subx2 - 16 - 40;
		int filename_pady2 = filename_pady + numSavesToShow * TTF12_HEIGHT + 8;
		int filename_rowHeight = 2 * TTF12_HEIGHT + 8;
		filename_pady += 3 * TTF12_HEIGHT;
		int numSaves = savegamesList.size();
		if ( numSaves > 0 )
		{
			//ttfPrintTextFormattedColor(ttf12, filename_padx, filename_pady, uint32ColorGreen(*mainsurface), language[3066]);
		}

		SDL_Rect tooltip; // we will draw the tooltip after drawing the other elements of the display window.

		tooltip.x = omousex + 8;
		tooltip.y = omousey + 8;
		tooltip.w = 32 + TTF12_WIDTH * 14;
		tooltip.h = TTF12_HEIGHT + 8;

		filename_pady += 2 * TTF12_HEIGHT;

		// do slider
		SDL_Rect slider;
		slider.x = filename_padx2 + 8;
		slider.y = filename_pady - 8;
		slider.h = suby2 - (filename_pady + 20);
		slider.w = 32;

		int entriesToScroll = std::max(static_cast<int>((numSaves / numSavesToShow) - 1), 0);
		entriesToScroll = entriesToScroll * numSavesToShow + (numSaves % numSavesToShow);

		bool drawScrollTooltip = false;

		// handle slider movement.
		if ( numSaves > numSavesToShow )
		{
			drawRect(&slider, SDL_MapRGB(mainsurface->format, 64, 64, 64), 255);
			if ( mouseInBounds(clientnum, filename_padx, slider.x + slider.w,
				slider.y, slider.y + slider.h) )
			{
				if ( mouseInBounds(clientnum, slider.x, slider.x + slider.w,
					slider.y, slider.y + slider.h) )
				{
					drawScrollTooltip = true;
				}
				if ( mousestatus[SDL_BUTTON_WHEELUP] )
				{
					savegames_window_scroll = std::max(savegames_window_scroll - 1, 0);
					mousestatus[SDL_BUTTON_WHEELUP] = 0;
				}
				if ( mousestatus[SDL_BUTTON_WHEELDOWN] )
				{
					savegames_window_scroll = std::min(savegames_window_scroll + 1, entriesToScroll);
					mousestatus[SDL_BUTTON_WHEELDOWN] = 0;
				}
			}

			if ( keystatus[SDL_SCANCODE_UP] )
			{
				savegames_window_scroll = std::max(savegames_window_scroll - 1, 0);
				keystatus[SDL_SCANCODE_UP] = 0;
			}
			if ( keystatus[SDL_SCANCODE_DOWN] )
			{
				savegames_window_scroll = std::min(savegames_window_scroll + 1, entriesToScroll);
				keystatus[SDL_SCANCODE_DOWN] = 0;
			}
			slider.h *= (1 / static_cast<real_t>(entriesToScroll + 1));
			slider.y += slider.h * savegames_window_scroll;
			if ( savegames_window_scroll == entriesToScroll ) // reached end.
			{
				slider.y += (suby2 - 28) - (slider.y + slider.h); // bottom of slider is (suby2 - 28), so move the y level to imitate hitting the bottom in case of rounding error.
			}
			drawWindowFancy(slider.x, slider.y, slider.x + slider.w, slider.y + slider.h); // draw shortened list relative slider.
		}
		else
		{
			//drawRect(&slider, SDL_MapRGB(mainsurface->format, 64, 64, 64), 255);
			drawWindowFancy(slider.x, slider.y, slider.x + slider.w, slider.y + slider.h);
		}

		bool drawDeleteTooltip = false;
		int numSingleplayerSaves = 0;
		int numMultiplayerSaves = 0;

		// draw the content
		for ( int i = 0; i < numSaves && !savegamesList.empty(); ++i ) //TODO: Why did this line add `&& !savegamesList.empty()` ?
		{
			filename_padx = subx1 + 16;

			std::vector<std::tuple<int, int, int, std::string>>::iterator it = savegamesList.begin();
			std::advance(it, i);
			std::tuple<int, int, int, std::string> entry = *it;

			if ( std::get<1>(entry) != SINGLE )
			{
				++numMultiplayerSaves;
			}
			else
			{
				++numSingleplayerSaves;
			}

			if ( i >= savegames_window_scroll && i < numSavesToShow + savegames_window_scroll )
			{
				drawWindowFancy(filename_padx, filename_pady - 8, filename_padx2, filename_pady + filename_rowHeight);
				SDL_Rect highlightEntry;
				highlightEntry.x = filename_padx;
				highlightEntry.y = filename_pady - 8;
				highlightEntry.w = filename_padx2 - filename_padx;
				highlightEntry.h = filename_rowHeight + 8;
				if ( gamemods_numCurrentModsLoaded >= 0 )
				{
					if ( std::get<1>(entry) == SINGLE ) // single player.
					{
						drawRect(&highlightEntry, uint32ColorGreen(*mainsurface), 64);
					}
					else
					{
						drawRect(&highlightEntry, uint32ColorGreen(*mainsurface), 32);
					}
				}
				else
				{
					if ( std::get<1>(entry) == SINGLE ) // single player.
					{
						drawRect(&highlightEntry, SDL_MapRGB(mainsurface->format, 128, 128, 128), 48);
						//drawRect(&highlightEntry, uint32ColorBaronyBlue(*mainsurface), 16);
					}
					else
					{
						drawRect(&highlightEntry, uint32ColorBaronyBlue(*mainsurface), 32);
					}
				}

				ttfPrintTextFormatted(ttf12, filename_padx + 8, filename_pady, "[%d]: %s", i + 1, std::get<3>(entry).c_str());

				filename_padx = filename_padx2 - (13 * TTF12_WIDTH + 16);
				int text_x = filename_padx;
				int text_y = filename_pady + 10;
				if ( drawClickableButton(filename_padx, filename_pady, 10 * TTF12_WIDTH + 8, TTF12_HEIGHT * 2 + 4, 0) )
				{
					if ( std::get<1>(entry) == SINGLE )
					{
						savegameCurrentFileIndex = std::get<2>(entry);
						buttonLoadSingleplayerGame(nullptr);
					}
					else
					{
						savegameCurrentFileIndex = std::get<2>(entry);
						buttonLoadMultiplayerGame(nullptr);
					}
				}
				ttfPrintTextFormatted(ttf12, text_x + 8, text_y, "%s", "Load Game");

				filename_padx = filename_padx2 - (2 * TTF12_WIDTH + 14);
				text_x = filename_padx;
				if ( drawClickableButton(filename_padx, filename_pady, 2 * TTF12_WIDTH + 8, TTF12_HEIGHT * 2 + 4, uint32ColorRed(*mainsurface)) )
				{
					if ( std::get<1>(entry) == SINGLE )
					{
						savegameCurrentFileIndex = std::get<2>(entry);
						buttonDeleteSavedSoloGame(nullptr);
					}
					else
					{
						savegameCurrentFileIndex = std::get<2>(entry);
						buttonDeleteSavedMultiplayerGame(nullptr);
					}
				}
				ttfPrintTextFormatted(ttf12, text_x + 6, text_y, "%s", "X");
				if ( mouseInBounds(clientnum, filename_padx, filename_padx + 2 * TTF12_WIDTH + 8, filename_pady, filename_pady + TTF12_HEIGHT * 2 + 4) )
				{
					drawDeleteTooltip = true;
				}

				filename_pady += 3 * filename_rowHeight / 2;
			}
		}

		Uint32 saveNumColor = uint32ColorGreen(*mainsurface);
		if ( numSingleplayerSaves == SAVE_GAMES_MAX )
		{
			saveNumColor = uint32ColorOrange(*mainsurface);
		}
		ttfPrintTextFormattedColor(ttf12, subx2 - (longestline(language[3067]) * TTF12_WIDTH), suby1 + 44, saveNumColor,
			language[3067], numSingleplayerSaves, SAVE_GAMES_MAX);

		saveNumColor = uint32ColorGreen(*mainsurface);
		if ( numMultiplayerSaves == SAVE_GAMES_MAX )
		{
			saveNumColor = uint32ColorOrange(*mainsurface);
		}
		ttfPrintTextFormattedColor(ttf12, subx2 - (longestline(language[3068]) * TTF12_WIDTH), suby1 + 44 + TTF12_HEIGHT + 4, saveNumColor,
			language[3068], numMultiplayerSaves, SAVE_GAMES_MAX);

		// draw the tooltip we initialised earlier.
		if ( drawDeleteTooltip )
		{
			tooltip.w = longestline(language[3064]) * TTF12_WIDTH + 16;
			drawTooltip(&tooltip);
			ttfPrintTextFormatted(ttf12, tooltip.x + 6, tooltip.y + 6, language[3064]);
		}
		else if ( drawScrollTooltip )
		{
			tooltip.w = longestline(language[3066]) * TTF12_WIDTH + 16;
			drawTooltip(&tooltip);
			ttfPrintTextFormatted(ttf12, tooltip.x + 6, tooltip.y + 6, language[3066]);
		}
	}
	else if ( gamemods_window != 0 )
	{
		int filenameMaxLength = 24;
		int filename_padx = subx1 + 16;
		int filename_pady = suby1 + 32;
		int numFileEntries = 10;

		int filename_padx2 = filename_padx + filenameMaxLength * TTF12_WIDTH + 8;
		int filename_pady2 = filename_pady + numFileEntries * TTF12_HEIGHT + 8;
#ifdef STEAMWORKS
		if ( gamemods_window == 1 || gamemods_window == 2 || gamemods_window == 5 )
		{
			if ( !currentDirectoryFiles.empty() )
			{
				int lineNumber = 0;
				std::string line;
				std::list<std::string>::const_iterator it = currentDirectoryFiles.begin();
				std::advance(it, gamemods_window_scroll);

				drawWindow(filename_padx, filename_pady - 2,
					filename_padx2, filename_pady2);

				SDL_Rect pos;
				pos.x = filename_padx;
				pos.y = filename_pady - 2 + std::max(gamemods_window_fileSelect - 1, 0) * TTF12_HEIGHT;
				pos.w = filenameMaxLength * TTF12_WIDTH + 8;
				pos.h = TTF12_HEIGHT;
				if ( gamemods_window_fileSelect != 0 )
				{
					drawRect(&pos, SDL_MapRGB(mainsurface->format, 64, 64, 64), 255);
				}

				for ( ; it != currentDirectoryFiles.end() && lineNumber < numFileEntries; ++it )
				{
					line = *it;
					line = line.substr(0, filenameMaxLength);
					ttfPrintTextFormatted(ttf12, filename_padx, filename_pady + lineNumber * TTF12_HEIGHT, "%s", line.c_str());
					++lineNumber;
				}
				int entriesToScroll = std::max(static_cast<int>((currentDirectoryFiles.size() / numFileEntries) - 1), 0);
				entriesToScroll = entriesToScroll * numFileEntries + (currentDirectoryFiles.size() % numFileEntries);

				if ( mouseInBounds(clientnum, filename_padx - 4, filename_padx2,
					filename_pady, filename_pady2) && currentDirectoryFiles.size() > numFileEntries )
				{
					if ( mousestatus[SDL_BUTTON_WHEELUP] )
					{
						gamemods_window_scroll = std::max(gamemods_window_scroll - 1, 0);
						mousestatus[SDL_BUTTON_WHEELUP] = 0;
					}
					if ( mousestatus[SDL_BUTTON_WHEELDOWN] )
					{
						gamemods_window_scroll = std::min(gamemods_window_scroll + 1, entriesToScroll);
						mousestatus[SDL_BUTTON_WHEELDOWN] = 0;
					}
				}
				for ( int i = 1; i <= numFileEntries; ++i )
				{
					if ( mouseInBounds(clientnum, filename_padx - 4, filename_padx2,
						filename_pady - 2, filename_pady - 2 + i * TTF12_HEIGHT) )
					{
						if ( inputs.bMouseLeft(clientnum) )
						{
							gamemods_window_fileSelect = i;
							inputs.mouseClearLeft(clientnum);
						}
					}
				}
				if ( !directoryToUpload.empty() )
				{
					ttfPrintTextFormatted(ttf12, filename_padx, filename_pady2 + TTF12_HEIGHT, "folder to upload: %s", directoryToUpload.c_str());
				}
			}
		}
		if ( gamemods_window == 2 || gamemods_window == 5 )
		{
			numFileEntries = 20;
			filename_padx = subx2 - (filenameMaxLength * TTF12_WIDTH + 16);
			filename_padx2 = subx2 - 16;
			filename_pady = filename_pady2 + 2 * TTF12_HEIGHT;
			filename_pady2 = filename_pady + numFileEntries * TTF12_HEIGHT + 2;
			if ( !directoryFilesListToUpload.empty() )
			{
				ttfPrintTextFormatted(ttf12, filename_padx, filename_pady - TTF12_HEIGHT, "file preview in folder:");
				drawWindow(filename_padx, filename_pady - 2,
					filename_padx2, filename_pady2);
				int lineNumber = 0;
				std::string line;
				for ( std::list<std::string>::const_iterator it = directoryFilesListToUpload.begin(); it != directoryFilesListToUpload.end() && lineNumber < 20; ++it )
				{
					line = *it;
					if ( line.size() >= filenameMaxLength )
					{
						line = line.substr(0, filenameMaxLength - 3);
						line.append("..");
					}
					else
					{
						line = line.substr(0, filenameMaxLength);
					}
					ttfPrintTextFormatted(ttf12, filename_padx, filename_pady + lineNumber * TTF12_HEIGHT, "%s", line.c_str());
					++lineNumber;
				}
			}

			int status_padx = subx1 + 16;
			int status_pady = filename_pady;
			if ( gamemods_uploadStatus != 0 && g_SteamWorkshop )
			{
				status_pady += 3 * TTF12_HEIGHT;
				if ( gamemods_window == 2 )
				{
					if ( g_SteamWorkshop->SubmitItemUpdateResult.m_eResult == 0
						&& gamemods_uploadStatus < 5 )
					{
						switch ( g_SteamWorkshop->createItemResult.m_eResult )
						{
							case 0:
								ttfPrintTextFormatted(ttf12, status_padx, status_pady, "creating item...");
								break;
							case k_EResultOK:
								if ( gamemods_uploadStatus < 2 )
								{
									for ( node = button_l.first; node != NULL; node = nextnode )
									{
										nextnode = node->next;
										button = (button_t*)node->element;
										if ( button->action == &buttonGamemodsPrepareWorkshopItemUpload )
										{
											button->visible = false;
										}
									}
									gamemods_uploadStatus = 2;
									g_SteamWorkshop->StartItemUpdate();
								}
								else
								{
									if ( g_SteamWorkshop->UGCUpdateHandle == 0 )
									{
										ttfPrintTextFormattedColor(ttf12, status_padx, status_pady, uint32ColorOrange(*mainsurface), "item created! awaiting file handle...");
									}
									else
									{
										if ( gamemods_uploadStatus == 2 )
										{
											gamemods_uploadStatus = 3;
											// set item fields button
											button = newButton();
											strcpy(button->label, "set item fields");
											button->x = subx1 + 16;
											button->y = suby1 + TTF12_HEIGHT * 34;
											button->sizex = 16 * TTF12_WIDTH + 8;
											button->sizey = 32;
											button->action = &buttonGamemodsSetWorkshopItemFields;
											button->visible = 1;
											button->focused = 1;
											gamemods_currentEditField = 0;
										}
										ttfPrintTextFormattedColor(ttf12, status_padx, status_pady, uint32ColorGreen(*mainsurface), "item and file handle create success!");
									}
								}
								break;
							default:
								// error in createItem!
								ttfPrintTextFormatted(ttf12, status_padx, status_pady, "error in creating item!");
								break;
						}
						status_pady += 2 * TTF12_HEIGHT;
						if ( gamemods_uploadStatus >= 3 && g_SteamWorkshop->SubmitItemUpdateResult.m_eResult == 0 )
						{
							for ( int fields = 0; fields < 2; ++fields )
							{
								status_pady += TTF12_HEIGHT;
								drawDepressed(status_padx, status_pady - 4, status_padx + 32 * TTF12_WIDTH, status_pady + TTF12_HEIGHT);
								switch ( fields )
								{
									case 0:
										ttfPrintText(ttf12, status_padx + 8, status_pady - TTF12_HEIGHT, "Enter a title:");
										if ( gamemods_uploadStatus == 3 && gamemods_workshopSetPropertyReturn[0] )
										{
											ttfPrintTextColor(ttf12, status_padx + 20 * TTF12_WIDTH, status_pady - TTF12_HEIGHT, uint32ColorGreen(*mainsurface), true, "success set");
										}
										ttfPrintText(ttf12, status_padx + 8, status_pady, gamemods_uploadTitle);
										break;
									case 1:
										ttfPrintText(ttf12, status_padx + 8, status_pady - TTF12_HEIGHT, "Enter description:");
										if ( gamemods_uploadStatus == 3 && gamemods_workshopSetPropertyReturn[1] )
										{

										}
										ttfPrintText(ttf12, status_padx + 8, status_pady, gamemods_uploadDescription);
										break;
									default:
										break;
								}
								if ( gamemods_uploadStatus == 4 )
								{
									if ( gamemods_workshopSetPropertyReturn[fields] )
									{
										ttfPrintTextColor(ttf12, status_padx + 20 * TTF12_WIDTH, status_pady - TTF12_HEIGHT, uint32ColorGreen(*mainsurface), true, "success set");
									}
									else
									{
										ttfPrintTextColor(ttf12, status_padx + 20 * TTF12_WIDTH, status_pady - TTF12_HEIGHT, uint32ColorRed(*mainsurface), true, "error!");
									}
								}

								if ( mouseInBounds(clientnum, status_padx, status_padx + 32 * TTF12_WIDTH, status_pady - 4, status_pady + TTF12_HEIGHT) )
								{
									if ( inputs.bMouseLeft(clientnum) )
									{
										switch ( fields )
										{
											case 0:
												inputstr = gamemods_uploadTitle;
												gamemods_currentEditField = 0;
												break;
											case 1:
												inputstr = gamemods_uploadDescription;
												gamemods_currentEditField = 1;
												break;
											default:
												break;
										}
										inputs.mouseClearLeft(clientnum);
									}
								}

								if ( gamemods_uploadStatus == 3 && !SDL_IsTextInputActive() )
								{
									inputstr = gamemods_uploadTitle;
									SDL_StartTextInput();
								}
								inputlen = 30;
								if ( SDL_IsTextInputActive() && gamemods_currentEditField == fields
									&& (ticks - cursorflash) % TICKS_PER_SECOND < TICKS_PER_SECOND / 2 )
								{
									int x;
									getSizeOfText(ttf12, inputstr, &x, NULL);
									ttfPrintText(ttf12, status_padx + x + 8, status_pady, "_");
								}
								status_pady += 2 * TTF12_HEIGHT;
							}
							if ( gamemods_uploadStatus >= 4 )
							{
								if ( gamemods_workshopSetPropertyReturn[2] )
								{
									ttfPrintTextColor(ttf12, status_padx, status_pady, uint32ColorGreen(*mainsurface), true, "folder path success set");
								}
								else
								{
									ttfPrintTextColor(ttf12, status_padx, status_pady, uint32ColorRed(*mainsurface), true, "error in folder path!");
								}
							}

							status_pady += 2 * TTF12_HEIGHT;
							// set some workshop item tags
							ttfPrintText(ttf12, status_padx, status_pady, "Select workshop tags");
							int tag_padx = status_padx;
							int tag_pady = status_pady + TTF12_HEIGHT;
							int tag_padx1 = tag_padx + 20 * TTF12_WIDTH;
							gamemodsDrawWorkshopItemTagToggle("dungeons", tag_padx, tag_pady);
							gamemodsDrawWorkshopItemTagToggle("textures", tag_padx1, tag_pady);
							tag_pady += TTF12_HEIGHT + 4;
							gamemodsDrawWorkshopItemTagToggle("models", tag_padx, tag_pady);
							gamemodsDrawWorkshopItemTagToggle("gameplay", tag_padx1, tag_pady);
							tag_pady += TTF12_HEIGHT + 4;
							gamemodsDrawWorkshopItemTagToggle("audio", tag_padx, tag_pady);
							gamemodsDrawWorkshopItemTagToggle("translations", tag_padx1, tag_pady);
							tag_pady += TTF12_HEIGHT + 4;
							gamemodsDrawWorkshopItemTagToggle("misc", tag_padx, tag_pady);
						}
					}
				}
				else if ( gamemods_window == 5 && g_SteamWorkshop->m_myWorkshopItemToModify.m_nPublishedFileId != 0 && gamemods_uploadStatus < 5 )
				{
					std::string line = g_SteamWorkshop->m_myWorkshopItemToModify.m_rgchTitle;
					if ( line.size() > filenameMaxLength )
					{
						line = line.substr(0, filenameMaxLength - 2);
						line.append("..");
					}
					status_pady += 2 * TTF12_HEIGHT;
					ttfPrintTextFormattedColor(ttf12, status_padx + 8, status_pady, uint32ColorBaronyBlue(*mainsurface), "Title:");
					status_pady += TTF12_HEIGHT;
					ttfPrintTextFormatted(ttf12, status_padx + 8, status_pady, "%s", line.c_str());

					line = g_SteamWorkshop->m_myWorkshopItemToModify.m_rgchDescription;
					if ( line.size() > filenameMaxLength )
					{
						line = line.substr(0, filenameMaxLength - 2);
						line.append("..");
					}
					status_pady += TTF12_HEIGHT;
					ttfPrintTextFormattedColor(ttf12, status_padx + 8, status_pady, uint32ColorBaronyBlue(*mainsurface), "Description:");
					status_pady += TTF12_HEIGHT;
					ttfPrintTextFormatted(ttf12, status_padx + 8, status_pady, "%s", line.c_str());

					status_pady += 2 * TTF12_HEIGHT;
					// set some workshop item tags
					ttfPrintText(ttf12, status_padx, status_pady, "Modify workshop tags");
					int tag_padx = status_padx;
					int tag_pady = status_pady + TTF12_HEIGHT;
					int tag_padx1 = tag_padx + 20 * TTF12_WIDTH;
					gamemodsDrawWorkshopItemTagToggle("dungeons", tag_padx, tag_pady);
					gamemodsDrawWorkshopItemTagToggle("textures", tag_padx1, tag_pady);
					tag_pady += TTF12_HEIGHT + 4;
					gamemodsDrawWorkshopItemTagToggle("models", tag_padx, tag_pady);
					gamemodsDrawWorkshopItemTagToggle("gameplay", tag_padx1, tag_pady);
					tag_pady += TTF12_HEIGHT + 4;
					gamemodsDrawWorkshopItemTagToggle("audio", tag_padx, tag_pady);
					gamemodsDrawWorkshopItemTagToggle("translations", tag_padx1, tag_pady);
					tag_pady += TTF12_HEIGHT + 4;
					gamemodsDrawWorkshopItemTagToggle("misc", tag_padx, tag_pady);

					status_pady += 6 * TTF12_HEIGHT;
					if ( directoryFilesListToUpload.empty() )
					{
						ttfPrintTextFormattedColor(ttf12, status_padx, status_pady, uint32ColorGreen(*mainsurface), "Only Workshop tags will be updated.");
					}
					else
					{
						ttfPrintTextFormattedColor(ttf12, status_padx, status_pady, uint32ColorOrange(*mainsurface), "Workshop file contents will be updated.");
					}
				}
				status_pady += 5 * TTF12_HEIGHT;
				if ( gamemods_uploadStatus >= 5 )
				{
					uint64 bytesProc;
					uint64 bytesTotal;
					int status = SteamUGC()->GetItemUpdateProgress(g_SteamWorkshop->UGCUpdateHandle, &bytesProc, &bytesTotal);
					if ( g_SteamWorkshop->SubmitItemUpdateResult.m_eResult != 0 )
					{
						for ( node = button_l.first; node != NULL; node = nextnode )
						{
							nextnode = node->next;
							button = (button_t*)node->element;
							if ( button->action == &buttonGamemodsPrepareWorkshopItemUpload
								|| button->action == &buttonGamemodsStartUploadItem
								|| button->action == &buttonGamemodsSetWorkshopItemFields
								|| button->action == &buttonGamemodsSelectDirectoryForUpload
								|| button->action == &buttonGamemodsModifyExistingWorkshopItemFields
								|| button->action == &buttonGamemodsCancelModifyFileContents )
							{
								button->visible = false;
							}
						}
						if ( g_SteamWorkshop->SubmitItemUpdateResult.m_eResult == k_EResultOK )
						{
							if ( g_SteamWorkshop->uploadSuccessTicks == 0 )
							{
								g_SteamWorkshop->uploadSuccessTicks = ticks;
							}
							else
							{
								if ( ticks - g_SteamWorkshop->uploadSuccessTicks > TICKS_PER_SECOND * 5 )
								{
									//cleanup the interface.
									buttonCloseSubwindow(NULL);
									list_FreeAll(&button_l);
									deleteallbuttons = true;
									gamemodsSubscribedItemsInit();
								}
							}
							ttfPrintTextFormattedColor(ttf12, status_padx, status_pady, uint32ColorGreen(*mainsurface), "successfully uploaded!");
							ttfPrintTextFormattedColor(ttf12, status_padx, status_pady + TTF12_HEIGHT, uint32ColorGreen(*mainsurface), "reloading window in %d...!", 5 - ((ticks - g_SteamWorkshop->uploadSuccessTicks) / TICKS_PER_SECOND));
						}
						else
						{
							ttfPrintTextFormattedColor(ttf12, status_padx, status_pady, uint32ColorOrange(*mainsurface), "error! %d", g_SteamWorkshop->SubmitItemUpdateResult.m_eResult);
							ttfPrintTextFormattedColor(ttf12, status_padx, status_pady + TTF12_HEIGHT, uint32ColorOrange(*mainsurface), "close the window and try again.");
						}
					}
					else
					{
						ttfPrintTextFormattedColor(ttf12, status_padx, status_pady, uint32ColorOrange(*mainsurface), "uploading... status %d", status);
						ttfPrintTextFormattedColor(ttf12, status_padx, status_pady + TTF12_HEIGHT, uint32ColorOrange(*mainsurface), "bytes processed: %d", bytesProc);
					}
				}
			}
		}
		if ( gamemods_window == 3 || gamemods_window == 4 )
		{
			numFileEntries = 8;
			filenameMaxLength = 48;
			filename_padx = subx1 + 16;
			filename_pady = suby1 + 32;
			filename_padx2 = subx2 - 16 - 40;
			filename_pady2 = filename_pady + numFileEntries * TTF12_HEIGHT + 8;
			int filename_rowHeight = 2 * TTF12_HEIGHT + 8;


			if ( gamemods_subscribedItemsStatus == 0 )
			{
				if ( g_SteamWorkshop->SteamUGCQueryCompleted.m_eResult == k_EResultOK )
				{
					gamemods_subscribedItemsStatus = 1;
				}
			}
			else
			{
				filename_pady += 1 * TTF12_HEIGHT;


				filename_pady += 2 * TTF12_HEIGHT;
				if ( gamemods_window == 3 )
				{
					ttfPrintTextFormattedColor(ttf12, filename_padx, filename_pady, uint32ColorGreen(*mainsurface), "successfully retrieved subscribed items!");
				}
				else
				{
					ttfPrintTextFormattedColor(ttf12, filename_padx, filename_pady, uint32ColorGreen(*mainsurface), "successfully retrieved my workshop items!");
				}

				std::string modInfoStr = "current loaded mods (hover for info): ";
				SDL_Rect tooltip; // we will draw the tooltip after drawing the other elements of the display window.
				bool drawModLoadOrder = false;
				int drawExtendedInformationForMod = -1; // value of 0 or greater will draw.
				int maxDescriptionLines = 10;

				tooltip.x = omousex - 256;
				tooltip.y = omousey + 16;
				tooltip.w = 32 + TTF12_WIDTH * 64;
				tooltip.h = (gamemods_mountedFilepaths.size() + 1) * TTF12_HEIGHT + 8;

				if ( gamemods_mountedFilepaths.size() > 0 && gamemods_window == 3 )
				{
					ttfPrintTextFormatted(ttf12, filename_padx2 - modInfoStr.length() * TTF12_WIDTH - 16, filename_pady, "%s %2d", modInfoStr.c_str(), gamemods_mountedFilepaths.size());
					if ( mouseInBounds(clientnum, filename_padx2 - modInfoStr.length() * TTF12_WIDTH - 16, filename_padx2, filename_pady, filename_pady + TTF12_HEIGHT) )
					{
						drawModLoadOrder = true;
					}
					else
					{
						drawModLoadOrder = false;
					}
				}

				filename_pady += 2 * TTF12_HEIGHT;

				// do slider
				SDL_Rect slider;
				slider.x = filename_padx2 + 8;
				slider.y = filename_pady - 8;
				slider.h = suby2 - (filename_pady + 20);
				slider.w = 32;

				int numSubscribedItemsReturned = g_SteamWorkshop->SteamUGCQueryCompleted.m_unNumResultsReturned;
				int entriesToScroll = std::max(static_cast<int>((numSubscribedItemsReturned / numFileEntries) - 1), 0);
				entriesToScroll = entriesToScroll * numFileEntries + (numSubscribedItemsReturned % numFileEntries);

				// handle slider movement.
				if ( numSubscribedItemsReturned > numFileEntries )
				{
					drawRect(&slider, SDL_MapRGB(mainsurface->format, 64, 64, 64), 255);
					if ( mouseInBounds(clientnum, filename_padx, slider.x + slider.w,
						slider.y, slider.y + slider.h) )
					{
						if ( mousestatus[SDL_BUTTON_WHEELUP] )
						{
							gamemods_window_scroll = std::max(gamemods_window_scroll - 1, 0);
							mousestatus[SDL_BUTTON_WHEELUP] = 0;
						}
						if ( mousestatus[SDL_BUTTON_WHEELDOWN] )
						{
							gamemods_window_scroll = std::min(gamemods_window_scroll + 1, entriesToScroll);
							mousestatus[SDL_BUTTON_WHEELDOWN] = 0;
						}
					}
				
					if ( keystatus[SDL_SCANCODE_UP] )
					{
						gamemods_window_scroll = std::max(gamemods_window_scroll - 1, 0);
						keystatus[SDL_SCANCODE_UP] = 0;
					}
					if ( keystatus[SDL_SCANCODE_DOWN] )
					{
						gamemods_window_scroll = std::min(gamemods_window_scroll + 1, entriesToScroll);
						keystatus[SDL_SCANCODE_DOWN] = 0;
					}
					slider.h *= (1 / static_cast<real_t>(entriesToScroll + 1));
					slider.y += slider.h * gamemods_window_scroll;
					if ( gamemods_window_scroll == entriesToScroll ) // reached end.
					{
						slider.y += (suby2 - 28) - (slider.y + slider.h); // bottom of slider is (suby2 - 28), so move the y level to imitate hitting the bottom in case of rounding error.
					}
					drawWindowFancy(slider.x, slider.y, slider.x + slider.w, slider.y + slider.h); // draw shortened list relative slider.
				}

				// draw last message results
				if ( ticks - g_SteamWorkshop->LastActionResult.creationTick < TICKS_PER_SECOND * 5 )
				{
					ttfPrintTextFormattedColor(ttf12, filename_padx + 8, suby2 - TTF12_HEIGHT - 4, uint32ColorOrange(*mainsurface), "%s returned status %d", 
						g_SteamWorkshop->LastActionResult.actionMsg.c_str(), static_cast<int>(g_SteamWorkshop->LastActionResult.lastResult));
				}

				// draw the content
				for ( int i = gamemods_window_scroll; i < numSubscribedItemsReturned && i < numFileEntries + gamemods_window_scroll; ++i )
				{
					filename_padx = subx1 + 16;
					SteamUGCDetails_t itemDetails = g_SteamWorkshop->m_subscribedItemListDetails[i];
					char fullpath[PATH_MAX];
					if ( itemDetails.m_eResult == k_EResultOK )
					{
						drawWindowFancy(filename_padx, filename_pady - 8, filename_padx2, filename_pady + filename_rowHeight);
						SDL_Rect highlightEntry;
						highlightEntry.x = filename_padx;
						highlightEntry.y = filename_pady - 8;
						highlightEntry.w = filename_padx2 - filename_padx;
						highlightEntry.h = filename_rowHeight + 8;
						drawRect(&highlightEntry, SDL_MapRGB(mainsurface->format, 128, 128, 128), 64);

						bool itemDownloaded = SteamUGC()->GetItemInstallInfo(itemDetails.m_nPublishedFileId, NULL, fullpath, PATH_MAX, NULL);
						bool pathIsMounted = gamemodsIsPathInMountedFiles(fullpath);

						if ( pathIsMounted && gamemods_window == 3 )
						{
							SDL_Rect pos;
							pos.x = filename_padx + 2;
							pos.y = filename_pady - 6;
							pos.w = filename_padx2 - filename_padx - 4;
							pos.h = filename_rowHeight + 4;
							drawRect(&pos, uint32ColorGreen(*mainsurface), 64);
						}

						// draw preview title
						std::string line = itemDetails.m_rgchTitle;
						std::size_t found = line.find_first_of('\n');
						if ( found != std::string::npos && found < filenameMaxLength - 2 )
						{
							line = line.substr(0, found); // don't print out newlines.
							line.append("..");
						}
						else if ( line.length() >= filenameMaxLength )
						{
							line = line.substr(0, filenameMaxLength - 2);
							line.append("..");
						}
						ttfPrintTextFormatted(ttf12, filename_padx + 8, filename_pady, "Title: %s", line.c_str());

						// draw preview description
						line = itemDetails.m_rgchDescription;
						found = line.find_first_of('\n');
						if ( found != std::string::npos && found < filenameMaxLength - 2 )
						{
							line = line.substr(0, found);
							line.append("..");
						}
						else if ( line.length() >= filenameMaxLength )
						{
							line = line.substr(0, filenameMaxLength - 2);
							line.append("..");
						}
						ttfPrintTextFormatted(ttf12, filename_padx + 8, filename_pady + TTF12_HEIGHT, "Desc: %s", line.c_str());

						// if hovering over title or description, provide more info...
						if ( mouseInBounds(clientnum, filename_padx + 8, filename_padx + 8 + 52 * TTF12_WIDTH, filename_pady + TTF12_HEIGHT, filename_pady + 2 * TTF12_HEIGHT) )
						{
							drawExtendedInformationForMod = i;
						}

						filename_padx = filename_padx2 - (12 * TTF12_WIDTH + 16) * 2;
						// download button
						if ( gamemods_window == 3 )
						{
							if ( !itemDownloaded )
							{
								if ( gamemodsDrawClickableButton(filename_padx, filename_pady, 12 * TTF12_WIDTH + 8, TTF12_HEIGHT, uint32ColorBaronyBlue(*mainsurface), " Download ", 0) )
								{
									SteamUGC()->DownloadItem(itemDetails.m_nPublishedFileId, true);
								}
							}
							filename_padx += (12 * TTF12_WIDTH + 16);
							// mount button
							if ( itemDownloaded && !pathIsMounted )
							{
								if ( gamemodsDrawClickableButton(filename_padx, filename_pady, 12 * TTF12_WIDTH + 8, TTF12_HEIGHT, 0, " Load Item ", 0) )
								{
									if ( PHYSFS_mount(fullpath, NULL, 0) )
									{
										gamemods_mountedFilepaths.push_back(std::make_pair(fullpath, itemDetails.m_rgchTitle));
										gamemods_workshopLoadedFileIDMap.push_back(std::make_pair(itemDetails.m_rgchTitle, itemDetails.m_nPublishedFileId));
										gamemods_modelsListRequiresReload = true;
										gamemods_soundListRequiresReload = true;
									}
								}
							}
							filename_padx -= (12 * TTF12_WIDTH + 16);
						}
						if ( gamemods_window == 4 )
						{
							filename_padx += (12 * TTF12_WIDTH + 16);
							// edit content button
							if ( gamemodsDrawClickableButton(filename_padx, filename_pady + filename_rowHeight / 4, 12 * TTF12_WIDTH + 8, TTF12_HEIGHT, uint32ColorBaronyBlue(*mainsurface), "  Update  ", 0) )
							{
								buttonGamemodsOpenModifyExistingWindow(nullptr);
								gamemods_window = 5;
								gamemods_uploadStatus = 1;
								g_SteamWorkshop->m_myWorkshopItemToModify = itemDetails;

								// grab the current item tags and store them for modification.
								std::string workshopItemTagString = g_SteamWorkshop->m_myWorkshopItemToModify.m_rgchTags;
								std::size_t found = workshopItemTagString.find(",");
								g_SteamWorkshop->workshopItemTags.clear();
								while ( found != std::string::npos )
								{
									std::string line = workshopItemTagString.substr(0, found);
									workshopItemTagString = workshopItemTagString.substr(found + 1); // skip the "," character.
									g_SteamWorkshop->workshopItemTags.push_back(line); // store the comma separated value.
									found = workshopItemTagString.find(",");
								}
								// add the final string.
								g_SteamWorkshop->workshopItemTags.push_back(workshopItemTagString); 
							}
						}
						filename_pady += filename_rowHeight / 2;
						if ( gamemods_window == 3 )
						{
							// unsubscribe button
							if ( gamemodsDrawClickableButton(filename_padx, filename_pady, 12 * TTF12_WIDTH + 8, TTF12_HEIGHT, uint32ColorRed(*mainsurface), "Unsubscribe", 0) )
							{
								if ( pathIsMounted )
								{
									if ( PHYSFS_unmount(fullpath) )
									{
										if ( gamemodsRemovePathFromMountedFiles(fullpath) )
										{
											printlog("[%s] is removed from the search path.\n", fullpath);
											gamemods_modelsListRequiresReload = true;
											gamemods_soundListRequiresReload = true;
										}
									}
								}
								g_SteamWorkshop->UnsubscribeItemFileID(itemDetails.m_nPublishedFileId);
								gamemods_window_scroll = 0;
							}
							filename_padx += (12 * TTF12_WIDTH + 16);
							// unmount button
							if ( itemDownloaded && pathIsMounted )
							{
								if ( gamemodsDrawClickableButton(filename_padx, filename_pady, 12 * TTF12_WIDTH + 8, TTF12_HEIGHT, 0, "Unload Item", 0) )
								{
									if ( PHYSFS_unmount(fullpath) )
									{
										if ( gamemodsRemovePathFromMountedFiles(fullpath) )
										{
											printlog("[%s] is removed from the search path.\n", fullpath);
											gamemods_modelsListRequiresReload = true;
											gamemods_soundListRequiresReload = true;
										}
									}
								}
							}
						}
					}
					filename_pady += filename_rowHeight;
				}

				// draw the tooltip we initialised earlier.
				if ( drawModLoadOrder )
				{
					drawTooltip(&tooltip);
					int numLoadedModLine = 1;
					ttfPrintTextFormattedColor(ttf12, tooltip.x + 4, tooltip.y + 4, uint32ColorBaronyBlue(*mainsurface), 
						"Current load list: (first is lowest priority)");
					for ( std::vector<std::pair<std::string, std::string>>::iterator it = gamemods_mountedFilepaths.begin(); it != gamemods_mountedFilepaths.end(); ++it )
					{
						std::pair<std::string, std::string> line = *it;
						modInfoStr = line.second;
						if ( modInfoStr.length() > 64 )
						{
							modInfoStr = modInfoStr.substr(0, 64 - 2).append("..");
						}
						ttfPrintTextFormatted(ttf12, tooltip.x + 4, tooltip.y + 4 + numLoadedModLine * TTF12_HEIGHT, "%2d) %s", numLoadedModLine, modInfoStr.c_str());
						++numLoadedModLine;
					}
				}
				else if ( drawExtendedInformationForMod >= 0 )
				{
					int tooltip_pady = 8;
					SteamUGCDetails_t itemDetails = g_SteamWorkshop->m_subscribedItemListDetails[drawExtendedInformationForMod];
					// draw the information.
					std::string line;

					line = itemDetails.m_rgchDescription;
					line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
					//line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());
					std::string subString;
					std::string outputStr;
					std::size_t found = line.find('\n');
					int numlines = 0;
					while ( line.length() >= 62 || (found != std::string::npos && found < 62) )
					{
						if ( numlines >= maxDescriptionLines )
						{
							break;
						}
						if ( found != std::string::npos && found < 62 )
						{
							// found newline.
							subString = line.substr(0, found);
							line = line.substr(found + 1);
						}
						else
						{
							subString = line.substr(0, 62);
							if ( subString.at(subString.length() - 1) != ' ' || line.at(62) != ' ' )
							{
								// handle word wrapping.
								std::size_t lastSpace = subString.find_last_of(' ');
								if ( lastSpace != std::string::npos )
								{
									subString = subString.substr(0, lastSpace);
									line = line.substr(lastSpace, line.length());
								}
								else
								{
									line = line.substr(62);
								}
							}
							else
							{
								line = line.substr(62);
							}
						}
						outputStr.append(subString);
						outputStr += '\n';
						outputStr.append("  ");
						found = line.find('\n');
						++numlines;
					}

					subString = line;
					outputStr.append(subString);

					tooltip.h = (6 + std::min(maxDescriptionLines, numlines)) * TTF12_HEIGHT + 12;
					drawTooltip(&tooltip);

					// draw description title.
					line = itemDetails.m_rgchTitle;
					found = line.find_first_of('\n');
					if ( found != std::string::npos && found < 62 )
					{
						line = line.substr(0, found);
						line.append("..");
					}
					else if ( line.length() >= 64 )
					{
						line = line.substr(0, 62);
						line.append("..");
					}
					ttfPrintTextFormattedColor(ttf12, tooltip.x + 8, tooltip.y + tooltip_pady, uint32ColorBaronyBlue(*mainsurface), "%s", line.c_str());
					tooltip_pady += TTF12_HEIGHT * 2;

					// draw description body.
					ttfPrintTextFormatted(ttf12, tooltip.x + 8, tooltip.y + tooltip_pady, "  %s", outputStr.c_str());

					tooltip_pady += TTF12_HEIGHT * (numlines + 2);
					
					// draw tags.
					ttfPrintTextFormattedColor(ttf12, tooltip.x + 8, tooltip.y + tooltip_pady, uint32ColorBaronyBlue(*mainsurface), "tags:");
					tooltip_pady += TTF12_HEIGHT;
					line = itemDetails.m_rgchTags;

					int tooltip_padx = 0;
					if ( !line.empty() )
					{
						subString = line;
						ttfPrintTextFormatted(ttf12, tooltip.x + 8, tooltip.y + tooltip_pady, "  %s", subString.c_str());
					}
				}
			}
		}
#endif //STEAMWORKS

		if ( gamemods_window == 6 ) //TODO: NX PORT: Does this need any changes for the switch? Even if only to entirely disable the feature?
		{
			// create blank file structure for a mod.
			filename_padx = subx1 + 16;
			filename_pady = suby1 + 32;
			ttfPrintTextFormatted(ttf12, filename_padx, filename_pady, "Enter base folder name for template directory");
			filename_pady += TTF12_HEIGHT + 8;
			drawDepressed(filename_padx - 4, filename_pady - 4, filename_padx + 32 * TTF12_WIDTH + 8, filename_pady + TTF12_HEIGHT);
			ttfPrintTextFormatted(ttf12, filename_padx, filename_pady, "%s", gamemods_newBlankDirectory);
			if ( !SDL_IsTextInputActive() )
			{
				inputstr = gamemods_newBlankDirectory;
				SDL_StartTextInput();
			}
			inputlen = 30;
			if ( SDL_IsTextInputActive()
				&& (ticks - cursorflash) % TICKS_PER_SECOND < TICKS_PER_SECOND / 2 )
			{
				int x;
				getSizeOfText(ttf12, inputstr, &x, NULL);
				ttfPrintText(ttf12, filename_padx + x, filename_pady, "_");
			}
			filename_pady += TTF12_HEIGHT + 8;
			if ( strcmp(gamemods_newBlankDirectoryOldName, gamemods_newBlankDirectory) == 0 )
			{
				if ( gamemods_newBlankDirectoryStatus == -1 )
				{
					ttfPrintTextFormattedColor(ttf12, filename_padx, filename_pady, uint32ColorRed(*mainsurface), "Error: could not create directory %s/, already exists in mods/ folder", gamemods_newBlankDirectory);
				}
				else if ( gamemods_newBlankDirectoryStatus == 1 )
				{
					ttfPrintTextFormattedColor(ttf12, filename_padx, filename_pady, uint32ColorGreen(*mainsurface), "Successfully created directory %s/ in mods/ folder", gamemods_newBlankDirectory);
				}
			}
		}
		if ( gamemods_window == 7 )
		{
			numFileEntries = 8;
			filenameMaxLength = 48;
			filename_padx = subx1 + 16;
			filename_pady = suby1 + 32;
			filename_padx2 = subx2 - 16 - 40;
			filename_pady2 = filename_pady + numFileEntries * TTF12_HEIGHT + 8;
			int filename_rowHeight = 2 * TTF12_HEIGHT + 8;
			filename_pady += 3 * TTF12_HEIGHT;
			int numLocalFolders = std::max(static_cast<int>(gamemods_localModFoldernames.size() - 2), 0);
			if ( numLocalFolders > 0 )
			{
				ttfPrintTextFormattedColor(ttf12, filename_padx, filename_pady, uint32ColorGreen(*mainsurface), "successfully retrieved local items!");
			}
			else
			{
				ttfPrintTextFormattedColor(ttf12, filename_padx, filename_pady, uint32ColorOrange(*mainsurface), "no folders found!");
				ttfPrintTextFormattedColor(ttf12, filename_padx, filename_pady + TTF12_HEIGHT + 8, uint32ColorOrange(*mainsurface), "to get started create a new folder, or copy shared custom content to the mods/ folder");
			}

			std::string modInfoStr = "current loaded mods (hover for info): ";
			SDL_Rect tooltip; // we will draw the tooltip after drawing the other elements of the display window.
			bool drawModLoadOrder = false;
			int drawExtendedInformationForMod = -1; // value of 0 or greater will draw.
			int maxDescriptionLines = 10;

			tooltip.x = omousex - 256;
			tooltip.y = omousey + 16;
			tooltip.w = 32 + TTF12_WIDTH * 64;
			tooltip.h = (gamemods_mountedFilepaths.size() + 1) * TTF12_HEIGHT + 8;

			if ( gamemods_mountedFilepaths.size() > 0 )
			{
				ttfPrintTextFormatted(ttf12, filename_padx2 - modInfoStr.length() * TTF12_WIDTH - 16, filename_pady, "%s %2d", modInfoStr.c_str(), gamemods_mountedFilepaths.size());
				if ( mouseInBounds(clientnum, filename_padx2 - modInfoStr.length() * TTF12_WIDTH - 16, filename_padx2, filename_pady, filename_pady + TTF12_HEIGHT) )
				{
					drawModLoadOrder = true;
				}
				else
				{
					drawModLoadOrder = false;
				}
			}

			filename_pady += 2 * TTF12_HEIGHT;

			// do slider
			SDL_Rect slider;
			slider.x = filename_padx2 + 8;
			slider.y = filename_pady - 8;
			slider.h = suby2 - (filename_pady + 20);
			slider.w = 32;

			int entriesToScroll = std::max(static_cast<int>((numLocalFolders / numFileEntries) - 1), 0);
			entriesToScroll = entriesToScroll * numFileEntries + (numLocalFolders % numFileEntries);

			// handle slider movement.
			if ( numLocalFolders > numFileEntries )
			{
				drawRect(&slider, SDL_MapRGB(mainsurface->format, 64, 64, 64), 255);
				if ( mouseInBounds(clientnum, filename_padx, slider.x + slider.w,
					slider.y, slider.y + slider.h) )
				{
					if ( mousestatus[SDL_BUTTON_WHEELUP] )
					{
						gamemods_window_scroll = std::max(gamemods_window_scroll - 1, 0);
						mousestatus[SDL_BUTTON_WHEELUP] = 0;
					}
					if ( mousestatus[SDL_BUTTON_WHEELDOWN] )
					{
						gamemods_window_scroll = std::min(gamemods_window_scroll + 1, entriesToScroll);
						mousestatus[SDL_BUTTON_WHEELDOWN] = 0;
					}
				}

				if ( keystatus[SDL_SCANCODE_UP] )
				{
					gamemods_window_scroll = std::max(gamemods_window_scroll - 1, 0);
					keystatus[SDL_SCANCODE_UP] = 0;
				}
				if ( keystatus[SDL_SCANCODE_DOWN] )
				{
					gamemods_window_scroll = std::min(gamemods_window_scroll + 1, entriesToScroll);
					keystatus[SDL_SCANCODE_DOWN] = 0;
				}
				slider.h *= (1 / static_cast<real_t>(entriesToScroll + 1));
				slider.y += slider.h * gamemods_window_scroll;
				if ( gamemods_window_scroll == entriesToScroll ) // reached end.
				{
					slider.y += (suby2 - 28) - (slider.y + slider.h); // bottom of slider is (suby2 - 28), so move the y level to imitate hitting the bottom in case of rounding error.
				}
				drawWindowFancy(slider.x, slider.y, slider.x + slider.w, slider.y + slider.h); // draw shortened list relative slider.
			}

			// draw the content
			for ( int i = gamemods_window_scroll; i < numLocalFolders && i < numFileEntries + gamemods_window_scroll; ++i )
			{
				filename_padx = subx1 + 16;

				std::list<std::string>::iterator it = gamemods_localModFoldernames.begin();
				std::advance(it, 2); // skip the "." and ".." directories.
				std::advance(it, i);
				std::string folderName = *it;

				drawWindowFancy(filename_padx, filename_pady - 8, filename_padx2, filename_pady + filename_rowHeight);
				SDL_Rect highlightEntry;
				highlightEntry.x = filename_padx;
				highlightEntry.y = filename_pady - 8;
				highlightEntry.w = filename_padx2 - filename_padx;
				highlightEntry.h = filename_rowHeight + 8;
				drawRect(&highlightEntry, SDL_MapRGB(mainsurface->format, 128, 128, 128), 64);

				std::string path = outputdir;
				path.append(PHYSFS_getDirSeparator()).append("mods").append(PHYSFS_getDirSeparator()).append(folderName);
				bool pathIsMounted = gamemodsIsPathInMountedFiles(path);

				if ( pathIsMounted )
				{
					SDL_Rect pos;
					pos.x = filename_padx + 2;
					pos.y = filename_pady - 6;
					pos.w = filename_padx2 - filename_padx - 4;
					pos.h = filename_rowHeight + 4;
					drawRect(&pos, uint32ColorGreen(*mainsurface), 64);
				}

				if ( folderName.length() >= filenameMaxLength )
				{
					folderName = folderName.substr(0, 46);
					folderName.append("..");
				}
				ttfPrintTextFormatted(ttf12, filename_padx + 8, filename_pady + TTF12_HEIGHT / 2, "Folder Name: %s", folderName.c_str());

				filename_padx = filename_padx2 - (12 * TTF12_WIDTH + 16);
				// mount button
				if ( !pathIsMounted )
				{
					if ( gamemodsDrawClickableButton(filename_padx, filename_pady, 12 * TTF12_WIDTH + 8, TTF12_HEIGHT, 0, " Load Item ", 0) )
					{
						if ( PHYSFS_mount(path.c_str(), NULL, 0) )
						{
							gamemods_mountedFilepaths.push_back(std::make_pair(path, folderName));
							printlog("[PhysFS]: [%s] is in the search path.\n", path.c_str());
							gamemods_modelsListRequiresReload = true;
							gamemods_soundListRequiresReload = true;
						}
					}
				}
				filename_pady += filename_rowHeight / 2;
				// unmount button
				if ( pathIsMounted )
				{
					if ( gamemodsDrawClickableButton(filename_padx, filename_pady, 12 * TTF12_WIDTH + 8, TTF12_HEIGHT, 0, "Unload Item", 0) )
					{
						if ( PHYSFS_unmount(path.c_str()) )
						{
							if ( gamemodsRemovePathFromMountedFiles(path) )
							{
								printlog("[PhysFS]: [%s] is removed from the search path.\n", path.c_str());
								gamemods_modelsListRequiresReload = true;
								gamemods_soundListRequiresReload = true;
							}
						}
					}
				}
				filename_pady += filename_rowHeight;
			}

			// draw the tooltip we initialised earlier.
			if ( drawModLoadOrder )
			{
				drawTooltip(&tooltip);
				int numLoadedModLine = 1;
				ttfPrintTextFormattedColor(ttf12, tooltip.x + 4, tooltip.y + 4, uint32ColorBaronyBlue(*mainsurface),
					"Current load list: (first is lowest priority)");
				for ( std::vector<std::pair<std::string, std::string>>::iterator it = gamemods_mountedFilepaths.begin(); it != gamemods_mountedFilepaths.end(); ++it )
				{
					std::pair<std::string, std::string> line = *it;
					modInfoStr = line.second;
					if ( modInfoStr.length() > 64 )
					{
						modInfoStr = modInfoStr.substr(0, 64 - 2).append("..");
					}
					ttfPrintTextFormatted(ttf12, tooltip.x + 4, tooltip.y + 4 + numLoadedModLine * TTF12_HEIGHT, "%2d) %s", numLoadedModLine, modInfoStr.c_str());
					++numLoadedModLine;
				}
			}
		}
	}
	else if ( gameModeManager.Tutorial.Menu.isOpen() )
	{
		int numFileEntries = 11;

		int filenameMaxLength = 48;
		int filename_padx = subx1 + 16;
		int filename_pady = suby1 + 32;
		int filename_padx2 = subx2 - 16;
		int filename_rowHeight = 1.25 * TTF12_HEIGHT + 8;

		auto& menu = gameModeManager.Tutorial.Menu;
		ttfPrintTextFormattedColor(ttf12, filename_padx + 8, filename_pady + 8, uint32ColorWhite(*mainsurface), "%s", menu.windowTitle.c_str());

		filename_pady += 4 * TTF12_HEIGHT;

		// do slider
		SDL_Rect slider;
		slider.x = filename_padx2 + 8;
		slider.y = filename_pady - 8;
		slider.h = suby2 - (filename_pady + 20);
		slider.w = 32;

		int entriesToScroll = static_cast<Uint32>(std::max(((static_cast<int>(gameModeManager.Tutorial.levels.size()) / numFileEntries) - 1), 0));
		entriesToScroll = entriesToScroll * numFileEntries + (gameModeManager.Tutorial.levels.size() % numFileEntries);

		menu.selectedMenuItem = -1;

		// handle slider movement.
		if ( gameModeManager.Tutorial.levels.size() > numFileEntries )
		{
			drawRect(&slider, SDL_MapRGB(mainsurface->format, 64, 64, 64), 255);
			if ( mouseInBounds(clientnum, filename_padx, slider.x + slider.w,
				slider.y, slider.y + slider.h) )
			{
				if ( mousestatus[SDL_BUTTON_WHEELUP] )
				{
					menu.windowScroll = std::max(menu.windowScroll - 1, 0);
					mousestatus[SDL_BUTTON_WHEELUP] = 0;
				}
				if ( mousestatus[SDL_BUTTON_WHEELDOWN] )
				{
					menu.windowScroll = std::min(menu.windowScroll + 1, entriesToScroll);
					mousestatus[SDL_BUTTON_WHEELDOWN] = 0;
				}
			}

			if ( keystatus[SDL_SCANCODE_UP] )
			{
				menu.windowScroll = std::max(menu.windowScroll - 1, 0);
				keystatus[SDL_SCANCODE_UP] = 0;
			}
			if ( keystatus[SDL_SCANCODE_DOWN] )
			{
				menu.windowScroll = std::min(menu.windowScroll + 1, entriesToScroll);
				keystatus[SDL_SCANCODE_DOWN] = 0;
			}
			slider.h *= (1 / static_cast<real_t>(entriesToScroll + 1));
			slider.y += slider.h * menu.windowScroll;
			if ( menu.windowScroll == entriesToScroll ) // reached end.
			{
				slider.y += (suby2 - 28) - (slider.y + slider.h); // bottom of slider is (suby2 - 28), so move the y level to imitate hitting the bottom in case of rounding error.
			}
			drawWindowFancy(slider.x, slider.y, slider.x + slider.w, slider.y + slider.h); // draw shortened list relative slider.
		}

		// draw the content
		for ( int i = menu.windowScroll; i < gameModeManager.Tutorial.levels.size() && i < numFileEntries + menu.windowScroll; ++i )
		{
			filename_padx = subx1 + 16;

			auto it = gameModeManager.Tutorial.levels.begin();
			std::advance(it, i);
			std::string folderName = (*it).title;

			drawWindowFancy(filename_padx, filename_pady - 8, filename_padx2, filename_pady + filename_rowHeight);

			SDL_Rect highlightEntry;
			highlightEntry.x = filename_padx;
			highlightEntry.y = filename_pady - 8;
			highlightEntry.w = filename_padx2 - filename_padx;
			highlightEntry.h = filename_rowHeight + 8;
			drawRect(&highlightEntry, SDL_MapRGB(mainsurface->format, 128, 128, 128), 64);

			if ( mouseInBounds(clientnum, highlightEntry.x, highlightEntry.x + highlightEntry.w, highlightEntry.y - 2, highlightEntry.y + highlightEntry.h + 4) )
			{
				menu.selectedMenuItem = i;
			}

			if ( menu.selectedMenuItem == i )
			{
				SDL_Rect pos;
				pos.x = filename_padx + 2;
				pos.y = filename_pady - 6;
				pos.w = filename_padx2 - filename_padx - 4;
				pos.h = filename_rowHeight + 4;
				drawRect(&pos, uint32ColorBaronyBlue(*mainsurface), 64);
				ttfPrintTextFormattedColor(ttf12, filename_padx + 8, suby2 - 3 * TTF12_HEIGHT, uint32ColorYellow(*mainsurface), "%s", (*it).description.c_str());
			}

			SDL_Rect btn;
			btn.w = 6 * TTF12_WIDTH + 8;
			btn.h = highlightEntry.h - 12;
			btn.x = highlightEntry.x + highlightEntry.w - btn.w - 4;
			btn.y = highlightEntry.y + 8;
			if ( i == 0 )
			{
				btn.w = 11 * TTF12_WIDTH + 8;
				btn.x = highlightEntry.x + highlightEntry.w - btn.w - 4;
				if ( drawClickableButton(btn.x, btn.y, btn.w, btn.h, 0) )
				{
					menu.onClickEntry();
				}
				ttfPrintTextFormatted(ttf12, btn.x + 11, filename_pady + 4, "%s", "Enter Hub");
			}
			else
			{
				if ( drawClickableButton(btn.x, btn.y, btn.w, btn.h, 0) )
				{
					menu.onClickEntry();
				}
				ttfPrintTextFormatted(ttf12, btn.x + 7, filename_pady + 4, "%s", "Begin");
			}

			ttfPrintTextFormatted(ttf12, filename_padx + 8, filename_pady + 4, "%s", (*it).title.c_str());

			if ( i != 0 )
			{
				const Uint32 sec = ((*it).completionTime / TICKS_PER_SECOND) % 60;
				const Uint32 min = (((*it).completionTime / TICKS_PER_SECOND) / 60) % 60;
				const Uint32 hour = (((*it).completionTime / TICKS_PER_SECOND) / 60) / 60;
				const Uint32 timeTextWidth = 28 * TTF12_WIDTH + 8;
				if ( (*it).completionTime > 0 )
				{
					ttfPrintTextFormatted(ttf12, btn.x - timeTextWidth, filename_pady + 4, "Best time: %02d:%02d:%02d", hour, min, sec);
				}
				else
				{
					ttfPrintTextFormatted(ttf12, btn.x - timeTextWidth, filename_pady + 4, "Not completed");
				}
			}

			filename_padx = filename_padx2 - (12 * TTF12_WIDTH + 16);
			filename_pady += 1.5 * filename_rowHeight;
		}

		if ( menu.selectedMenuItem == -1 )
		{
			ttfPrintTextFormattedColor(ttf12, subx1 + 16 + 8, suby2 - 3 * TTF12_HEIGHT, uint32ColorYellow(*mainsurface), "%s", menu.defaultHoverText.c_str());
		}
	}
	else if ( gameModeManager.Tutorial.FirstTimePrompt.isOpen() )
	{
		gameModeManager.Tutorial.FirstTimePrompt.drawDialogue();
		if ( gameModeManager.Tutorial.FirstTimePrompt.doButtonSkipPrompt )
		{
			gameModeManager.Tutorial.FirstTimePrompt.doButtonSkipPrompt = false;
			if ( anySaveFileExists() )
			{
				openNewLoadGameWindow(nullptr);
			}
			else
			{
				buttonOpenCharacterCreationWindow(NULL);
			}
		}
	}

	// handle fade actions
	if ( fadefinished )
	{
		if ( introstage == 2 )   // quit game
		{
			introstage = 0;
			mainloop = 0;
		}
		else if ( introstage == 3 )     // new game
		{
			bool bWasOnMainMenu = intro;
			introstage = 1;
			fadefinished = false;
			fadeout = false;
			gamePaused = false;
			multiplayerselect = SINGLE;
			intro = true; //Fix items auto-adding to the hotbar on game restart.

			if ( gameModeManager.getMode() == GameModeManager_t::GAME_MODE_DEFAULT )
			{
				if ( !mode )
				{
					// restarting game, make a highscore
					saveScore();
					deleteSaveGame(multiplayer);
					loadingsavegame = 0;
				}
			}
			camera_charsheet_offsetyaw = (330) * PI / 180; // reset player camera view.

			// undo shopkeeper grudge
			swornenemies[SHOPKEEPER][HUMAN] = false;
			monsterally[SHOPKEEPER][HUMAN] = true;
			swornenemies[SHOPKEEPER][AUTOMATON] = false;
			monsterally[SHOPKEEPER][AUTOMATON] = true;

			// setup game //TODO: Move into a function startGameStuff() or something.
			entity_uids = 1;
			loading = true;
			darkmap = false;

			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				players[i]->init();
				players[i]->hud.reset();
				deinitShapeshiftHotbar(i);
				for ( c = 0; c < NUM_HOTBAR_ALTERNATES; ++c )
				{
					players[i]->hotbar.hotbarShapeshiftInit[c] = false;
				}
				players[i]->shootmode = true;
				players[i]->magic.clearSelectedSpells();
				enemyHPDamageBarHandler[i].HPBars.clear();
			}
			currentlevel = startfloor;
			secretlevel = false;
			victory = 0;
			completionTime = 0;

			setDefaultPlayerConducts(); // penniless, foodless etc.
			if ( startfloor != 0 )
			{
				conductGameChallenges[CONDUCT_CHEATS_ENABLED] = 1;
			}

			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				minimapPings[i].clear(); // clear minimap pings
			}
			globalLightModifierActive = GLOBAL_LIGHT_MODIFIER_STOPPED;
			gameplayCustomManager.readFromFile();
			textSourceScript.scriptVariables.clear();

			if ( multiplayer == CLIENT )
			{
				gameModeManager.currentSession.saveServerFlags();
				svFlags = lobbyWindowSvFlags;
			}
			else if ( !loadingsavegame && bWasOnMainMenu )
			{
				gameModeManager.currentSession.saveServerFlags();
			}

			if ( gameModeManager.getMode() == GameModeManager_t::GAME_MODE_TUTORIAL )
			{
				svFlags &= ~(SV_FLAG_HARDCORE);
				svFlags &= ~(SV_FLAG_CHEATS);
				svFlags &= ~(SV_FLAG_LIFESAVING);
				svFlags &= ~(SV_FLAG_CLASSIC);
				svFlags &= ~(SV_FLAG_KEEPINVENTORY);
				svFlags |= SV_FLAG_HUNGER;
				svFlags |= SV_FLAG_FRIENDLYFIRE;
				svFlags |= SV_FLAG_MINOTAURS;
				svFlags |= SV_FLAG_TRAPS;

				if ( gameModeManager.Tutorial.dungeonLevel >= 0 )
				{
					currentlevel = gameModeManager.Tutorial.dungeonLevel;
					gameModeManager.Tutorial.dungeonLevel = -1;
				}
			}

			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				// clear follower menu entities.
				FollowerMenu[i].closeFollowerMenuGUI(true);
				list_FreeAll(&damageIndicators[i]);
			}
			for ( c = 0; c < NUMMONSTERS; c++ )
			{
				kills[c] = 0;
			}

			// close chests
			for ( c = 0; c < MAXPLAYERS; ++c )
			{
				if ( players[c]->isLocalPlayer() )
				{
					if ( openedChest[c] )
					{
						openedChest[c]->closeChest();
					}
				}
				else if ( c > 0 && !client_disconnected[c] )
				{
					if ( openedChest[c] )
					{
						openedChest[c]->closeChestServer();
					}
				}
			}

			// disable cheats
			noclip = false;
			godmode = false;
			buddhamode = false;
			everybodyfriendly = false;
			gameloopFreezeEntities = false;

#ifdef STEAMWORKS
			if ( !directConnect )
			{
				if ( currentLobby )
				{
					// once the game is started, the lobby is no longer needed.
					// when all steam users have left the lobby,
					// the lobby is destroyed automatically on the backend.

					SteamMatchmaking()->LeaveLobby(*static_cast<CSteamID*>(currentLobby));
					cpp_Free_CSteamID(currentLobby); //TODO: Bugger this.
					currentLobby = NULL;
				}
			}
#elif defined USE_EOS
			if ( !directConnect )
			{
				/*if ( EOS.CurrentLobbyData.currentLobbyIsValid() )
				{
					EOS.leaveLobby();
				}*/
			}
#endif

			// load dungeon
			if ( multiplayer != CLIENT )
			{
				// stop all sounds
#ifdef USE_FMOD
				if ( sound_group )
				{
					FMOD_ChannelGroup_Stop(sound_group);
				}
				if ( soundAmbient_group )
				{
					FMOD_ChannelGroup_Stop(soundAmbient_group);
				}
				if ( soundEnvironment_group )
				{
					FMOD_ChannelGroup_Stop(soundEnvironment_group);
				}
#elif defined USE_OPENAL
				if ( sound_group )
				{
					OPENAL_ChannelGroup_Stop(sound_group);
				}
				if ( soundAmbient_group )
				{
					OPENAL_ChannelGroup_Stop(soundAmbient_group);
				}
				if ( soundEnvironment_group )
				{
					OPENAL_ChannelGroup_Stop(soundEnvironment_group);
				}
#endif

				// generate a unique game key (used to identify compatible save games)
				prng_seed_time();
				if ( multiplayer == SINGLE )
				{
					uniqueGameKey = prng_get_uint();
					if ( !uniqueGameKey )
					{
						uniqueGameKey++;
					}
				}

				// reset class loadout
				if ( !loadingsavegame )
				{
					stats[0]->clearStats();
					initClass(0);
					mapseed = 0;
				}
				else
				{
					loadGame(0);
				}

				// hack to fix these things from breaking everything...
				for ( int i = 0; i < MAXPLAYERS; ++i )
				{
					players[i]->hud.arm = nullptr;
					players[i]->hud.weapon = nullptr;
					players[i]->hud.magicLeftHand = nullptr;
					players[i]->hud.magicRightHand = nullptr;
				}

				for ( node = map.entities->first; node != nullptr; node = node->next )
				{
					entity = (Entity*)node->element;
					entity->flags[NOUPDATE] = true;
				}
				lastEntityUIDs = entity_uids;
				numplayers = 0;
				int checkMapHash = -1;
				if ( loadingmap == false )
				{
					physfsLoadMapFile(currentlevel, mapseed, false, &checkMapHash);
					if ( checkMapHash == 0 )
					{
						conductGameChallenges[CONDUCT_MODDED] = 1;
					}
				}
				else
				{
					if ( genmap == false )
					{
						std::string fullMapName = physfsFormatMapName(maptoload);
						loadMap(fullMapName.c_str(), &map, map.entities, map.creatures, &checkMapHash);
						if ( checkMapHash == 0 )
						{
							conductGameChallenges[CONDUCT_MODDED] = 1;
						}
					}
					else
					{
						generateDungeon(maptoload, mapseed);
					}
				}
				assignActions(&map);
				generatePathMaps();

				achievementObserver.updateData();

				if ( loadingsavegame )
				{
					for ( c = 0; c < MAXPLAYERS; c++ )
					{
						if ( players[c] && players[c]->entity && !client_disconnected[c] )
						{
							if ( stats[c] && stats[c]->EFFECTS[EFF_POLYMORPH] && stats[c]->playerPolymorphStorage != NOTHING )
							{
								players[c]->entity->effectPolymorph = stats[c]->playerPolymorphStorage;
								serverUpdateEntitySkill(players[c]->entity, 50); // update visual polymorph effect for clients.
								serverUpdateEffects(c);
							}
							if ( stats[c] && stats[c]->EFFECTS[EFF_SHAPESHIFT] && stats[c]->playerShapeshiftStorage != NOTHING )
							{
								players[c]->entity->effectShapeshift = stats[c]->playerShapeshiftStorage;
								serverUpdateEntitySkill(players[c]->entity, 53); // update visual shapeshift effect for clients.
								serverUpdateEffects(c);
							}
							if ( stats[c] && stats[c]->EFFECTS[EFF_VAMPIRICAURA] && stats[c]->EFFECTS_TIMERS[EFF_VAMPIRICAURA] == -2 )
							{
								players[c]->entity->playerVampireCurse = 1;
								serverUpdateEntitySkill(players[c]->entity, 51); // update curse progression
							}
						}
					}

					list_t* followers = loadGameFollowers();
					if ( followers )
					{
						int c;
						for ( c = 0; c < MAXPLAYERS; c++ )
						{
							node_t* tempNode = list_Node(followers, c);
							if ( tempNode )
							{
								list_t* tempFollowers = (list_t*)tempNode->element;
								if (players[c] && players[c]->entity && !client_disconnected[c])
								{
									node_t* node;
									node_t* gyrobotNode = nullptr;
									Entity* gyrobotEntity = nullptr;
									std::vector<node_t*> allyRobotNodes;
									for ( node = tempFollowers->first; node != NULL; node = node->next )
									{
										Stat* tempStats = (Stat*)node->element;
										if ( tempStats && tempStats->type == GYROBOT )
										{
											gyrobotNode = node;
											break;
										}
									}
									for ( node = tempFollowers->first; node != NULL; node = node->next )
									{
										Stat* tempStats = (Stat*)node->element;
										if ( tempStats && (tempStats->type == DUMMYBOT
											|| tempStats->type == SENTRYBOT
											|| tempStats->type == SPELLBOT) )
										{
											// gyrobot will pick up these guys into it's inventory, otherwise leave them behind.
											if ( gyrobotNode )
											{
												allyRobotNodes.push_back(node);
											}
											continue;
										}
										Entity* monster = summonMonster(tempStats->type, players[c]->entity->x, players[c]->entity->y);
										if ( monster )
										{
											if ( node == gyrobotNode )
											{
												gyrobotEntity = monster;
											}
											monster->skill[3] = 1; // to mark this monster partially initialized
											list_RemoveNode(monster->children.last);

											node_t* newNode = list_AddNodeLast(&monster->children);
											newNode->element = tempStats->copyStats();
											newNode->deconstructor = &statDeconstructor;
											newNode->size = sizeof(tempStats);

											Stat* monsterStats = (Stat*)newNode->element;
											monsterStats->leader_uid = players[c]->entity->getUID();
											monster->flags[USERFLAG2] = true;
											/*if ( !monsterally[HUMAN][monsterStats->type] )
											{
											}*/
											monster->monsterAllyIndex = c;
											if ( multiplayer == SERVER )
											{
												serverUpdateEntitySkill(monster, 42); // update monsterAllyIndex for clients.
											}

											if ( multiplayer != CLIENT )
											{
												monster->monsterAllyClass = monsterStats->allyClass;
												monster->monsterAllyPickupItems = monsterStats->allyItemPickup;
												if ( stats[c]->playerSummonPERCHR != 0 && !strcmp(monsterStats->name, "skeleton knight") )
												{
													monster->monsterAllySummonRank = (stats[c]->playerSummonPERCHR & 0x0000FF00) >> 8;
												}
												else if ( stats[c]->playerSummon2PERCHR != 0 && !strcmp(monsterStats->name, "skeleton sentinel") )
												{
													monster->monsterAllySummonRank = (stats[c]->playerSummon2PERCHR & 0x0000FF00) >> 8;
												}
												serverUpdateEntitySkill(monster, 46); // update monsterAllyClass
												serverUpdateEntitySkill(monster, 44); // update monsterAllyPickupItems
												serverUpdateEntitySkill(monster, 50); // update monsterAllySummonRank
											}

											newNode = list_AddNodeLast(&stats[c]->FOLLOWERS);
											newNode->deconstructor = &defaultDeconstructor;
											Uint32* myuid = (Uint32*) malloc(sizeof(Uint32));
											newNode->element = myuid;
											*myuid = monster->getUID();

											if ( c > 0 && multiplayer == SERVER )
											{
												strcpy((char*)net_packet->data, "LEAD");
												SDLNet_Write32((Uint32)monster->getUID(), &net_packet->data[4]);
												strcpy((char*)(&net_packet->data[8]), monsterStats->name);
												net_packet->data[8 + strlen(monsterStats->name)] = 0;
												net_packet->address.host = net_clients[c - 1].host;
												net_packet->address.port = net_clients[c - 1].port;
												net_packet->len = 8 + strlen(monsterStats->name) + 1;
												sendPacketSafe(net_sock, -1, net_packet, c - 1);

												serverUpdateAllyStat(c, monster->getUID(), monsterStats->LVL, monsterStats->HP, monsterStats->MAXHP, monsterStats->type);
											}

											if ( !FollowerMenu[c].recentEntity && players[c]->isLocalPlayer() )
											{
												FollowerMenu[c].recentEntity = monster;
											}
										}
									}
									if ( gyrobotEntity && !allyRobotNodes.empty() )
									{
										Stat* gyroStats = gyrobotEntity->getStats();
										for ( auto it = allyRobotNodes.begin(); gyroStats && it != allyRobotNodes.end(); ++it )
										{
											node_t* botNode = *it;
											if ( botNode )
											{
												Stat* tempStats = (Stat*)botNode->element;
												if ( tempStats )
												{
													ItemType type = WOODEN_SHIELD;
													if ( tempStats->type == SENTRYBOT )
													{
														type = TOOL_SENTRYBOT;
													}
													else if ( tempStats->type == SPELLBOT )
													{
														type = TOOL_SPELLBOT;
													}
													else if ( tempStats->type == DUMMYBOT )
													{
														type = TOOL_DUMMYBOT;
													}
													int appearance = monsterTinkeringConvertHPToAppearance(tempStats);
													if ( type != WOODEN_SHIELD )
													{
														Item* item = newItem(type, static_cast<Status>(tempStats->monsterTinkeringStatus), 
															0, 1, appearance, true, &gyroStats->inventory);
													}
												}
											}
										}
									}
								}
							}
						}
						list_FreeAll(followers);
						free(followers);
					}
				}

				if ( multiplayer == SINGLE )
				{
					saveGame();
				}
			}
			else
			{
				// hack to fix these things from breaking everything...
				for ( int i = 0; i < MAXPLAYERS; ++i )
				{
					players[i]->hud.arm = nullptr;
					players[i]->hud.weapon = nullptr;
					players[i]->hud.magicLeftHand = nullptr;
					players[i]->hud.magicRightHand = nullptr;
				}

				client_disconnected[0] = false;

				// initialize class
				if ( !loadingsavegame )
				{
					stats[clientnum]->clearStats();
					initClass(clientnum);
					mapseed = 0;
				}
				else
				{
					loadGame(clientnum);
				}

				// stop all sounds
#ifdef USE_FMOD
				if ( sound_group )
				{
					FMOD_ChannelGroup_Stop(sound_group);
				}
				if ( soundAmbient_group )
				{
					FMOD_ChannelGroup_Stop(soundAmbient_group);
				}
				if ( soundEnvironment_group )
				{
					FMOD_ChannelGroup_Stop(soundEnvironment_group);
				}
#elif defined USE_OPENAL
				if ( sound_group )
				{
					OPENAL_ChannelGroup_Stop(sound_group);
				}
				if ( soundAmbient_group )
				{
					OPENAL_ChannelGroup_Stop(soundAmbient_group);
				}
				if ( soundEnvironment_group )
				{
					OPENAL_ChannelGroup_Stop(soundEnvironment_group);
				}
#endif
				// load next level
				entity_uids = 1;
				lastEntityUIDs = entity_uids;
				numplayers = 0;

				int checkMapHash = -1;
				if ( loadingmap == false )
				{
					physfsLoadMapFile(currentlevel, mapseed, false, &checkMapHash);
					if ( checkMapHash == 0 )
					{
						conductGameChallenges[CONDUCT_MODDED] = 1;
					}
				}
				else
				{
					if ( genmap == false )
					{
						std::string fullMapName = physfsFormatMapName(maptoload);
						loadMap(fullMapName.c_str(), &map, map.entities, map.creatures, &checkMapHash);
						if ( checkMapHash == 0 )
						{
							conductGameChallenges[CONDUCT_MODDED] = 1;
						}
					}
					else
					{
						generateDungeon(maptoload, rand());
					}
				}
				assignActions(&map);
				generatePathMaps();
				for ( node = map.entities->first; node != nullptr; node = nextnode )
				{
					nextnode = node->next;
					Entity* entity = (Entity*)node->element;
					if ( entity->flags[NOUPDATE] )
					{
						list_RemoveNode(entity->mynode);    // we're anticipating this entity data from server
					}
				}

				printlog("Done.\n");
			}

			// spice of life achievement
			usedClass[client_classes[clientnum]] = true;
			bool usedAllClasses = true;
			for ( c = 0; c <= CLASS_MONK; c++ )
			{
				if ( !usedClass[c] )
				{
					usedAllClasses = false;
				}
			}
			if ( usedAllClasses )
			{
				steamAchievement("BARONY_ACH_SPICE_OF_LIFE");
			}

			if ( stats[clientnum]->playerRace >= 0 && stats[clientnum]->playerRace <= RACE_INSECTOID )
			{
				usedRace[stats[clientnum]->playerRace] = true;
			}
			// new achievement
			usedAllClasses = true;
			for ( c = 0; c <= CLASS_HUNTER; ++c )
			{
				if ( !usedClass[c] )
				{
					usedAllClasses = false;
				}
			}
			bool usedAllRaces = true;
			for ( c = RACE_HUMAN; c <= RACE_INSECTOID; ++c )
			{
				if ( !usedRace[c] )
				{
					usedAllRaces = false;
				}
			}
			if ( usedAllClasses && usedAllRaces )
			{
				steamAchievement("BARONY_ACH_I_WANT_IT_ALL");
			}

			if ( gameModeManager.getMode() == GameModeManager_t::GAME_MODE_DEFAULT && !loadingsavegame )
			{
				steamStatisticUpdate(STEAM_STAT_GAMES_STARTED, STEAM_STAT_INT, 1);
				achievementObserver.updateGlobalStat(STEAM_GSTAT_GAMES_STARTED);
			}

			// delete game data clutter
			list_FreeAll(&messages);
			list_FreeAll(&command_history);
			list_FreeAll(&safePacketsSent);
			for ( c = 0; c < MAXPLAYERS; c++ )
			{
				safePacketsReceivedMap[c].clear();
				players[c]->messageZone.deleteAllNotificationMessages();
			}
			if ( !loadingsavegame ) // don't delete the followers we just created!
			{
				for (c = 0; c < MAXPLAYERS; c++)
				{
					list_FreeAll(&stats[c]->FOLLOWERS);
				}
			}

			if ( loadingsavegame && multiplayer != CLIENT )
			{
				loadingsavegame = 0;
			}

			enchantedFeatherScrollSeed.seed(uniqueGameKey);
			enchantedFeatherScrollsShuffled.clear();
			enchantedFeatherScrollsShuffled = enchantedFeatherScrollsFixedList;
			std::shuffle(enchantedFeatherScrollsShuffled.begin(), enchantedFeatherScrollsShuffled.end(), enchantedFeatherScrollSeed);
			for ( auto it = enchantedFeatherScrollsShuffled.begin(); it != enchantedFeatherScrollsShuffled.end(); ++it )
			{
				//printlog("Sequence: %d", *it);
			}

			list_FreeAll(&removedEntities);

			for ( c = 0; c < MAXPLAYERS; c++ )
			{
				list_FreeAll(&chestInv[c]);
			}

			// make some messages
			startMessages();

			// kick off the main loop!
			pauseGame(1, 0);
			loading = false;
			intro = false;
		}
		else if ( introstage == 4 )     // credits
		{
			fadefinished = false;
			fadeout = false;
			if ( creditstage == 0 && victory == 3 )
			{
#ifdef MUSIC
			playmusic(citadelmusic[0], true, false, false);
#endif
			}
			creditstage++;
			if ( creditstage >= 15 )
			{
#ifdef MUSIC
				if ( victory == 3 )
				{
					playmusic(intromusic[2], true, false, false);
				}
				else
				{
					playmusic(intromusic[rand() % 2], true, false, false);
				}
#endif
				introstage = 1;
				credittime = 0;
				creditstage = 0;
				movie = false;
			}
			else
			{
				credittime = 0;
				movie = true;
			}
		}
		else if ( introstage == 5 )     // end game
		{
			bool endTutorial = false;
			if ( gameModeManager.getMode() != GameModeManager_t::GAME_MODE_DEFAULT )
			{
				victory = 0;
				gameModeManager.setMode(GameModeManager_t::GAME_MODE_DEFAULT);
				endTutorial = true;
			}

			// in greater numbers achievement
			if ( victory )
			{
				int k = 0;
				for ( c = 0; c < MAXPLAYERS; c++ )
				{
					if (players[c] && players[c]->entity)
					{
						k++;
					}
				}
				if ( k >= 2 )
				{
					steamAchievement("BARONY_ACH_IN_GREATER_NUMBERS");
				}

				if ( (victory == 1 && currentlevel >= 20)
					|| (victory == 2 && currentlevel >= 24)
					|| (victory == 3 && currentlevel >= 35) )
				{
					if ( client_classes[clientnum] == CLASS_ACCURSED )
					{
						if ( stats[clientnum]->EFFECTS[EFF_VAMPIRICAURA] && stats[clientnum]->EFFECTS_TIMERS[EFF_VAMPIRICAURA] == -2 )
						{
							conductGameChallenges[CONDUCT_ACCURSED] = 1;
						}
					}
					if ( completionTime < 20 * 60 * TICKS_PER_SECOND )
					{
						conductGameChallenges[CONDUCT_BOOTS_SPEED] = 1;
					}
					achievementObserver.updateGlobalStat(STEAM_GSTAT_GAMES_WON);
				}
			}

			// figure out the victory crawl texts...
			int movieCrawlType = -1;
			if ( victory )
			{
				if ( stats[0] )
				{
					strcpy(epilogueHostName, stats[0]->name);
					epilogueHostRace = RACE_HUMAN;
					if ( stats[0]->playerRace > 0 && stats[0]->appearance == 0 )
					{
						epilogueHostRace = stats[0]->playerRace;
					}
					epilogueMultiplayerType = multiplayer;
					if ( victory == 1 && epilogueHostRace > 0 && epilogueHostRace != RACE_AUTOMATON )
					{
						// herx defeat by monsters.
						movieCrawlType = MOVIE_CLASSIC_WIN_MONSTERS;
					}
					else if ( victory == 2 && epilogueHostRace > 0 && epilogueHostRace != RACE_AUTOMATON )
					{
						// baphomet defeat by monsters.
						movieCrawlType = MOVIE_CLASSIC_WIN_BAPHOMET_MONSTERS;
					}
					else if ( victory == 3 )
					{
						switch ( epilogueHostRace )
						{
							case RACE_AUTOMATON:
								movieCrawlType = MOVIE_WIN_AUTOMATON;
								break;
							case RACE_SKELETON:
							case RACE_VAMPIRE:
							case RACE_SUCCUBUS:
							case RACE_INCUBUS:
								movieCrawlType = MOVIE_WIN_DEMONS_UNDEAD;
								break;
							case RACE_GOATMAN:
							case RACE_GOBLIN:
							case RACE_INSECTOID:
								movieCrawlType = MOVIE_WIN_BEASTS;
								break;
							case RACE_HUMAN:
								break;
							default:
								break;
						}
					}
				}
			}

			// make a highscore!
			if ( !endTutorial )
			{
				int saveScoreResult = saveScore();
			}

			// pick a new subtitle :)
			subtitleCurrent = rand() % NUMSUBTITLES;
			subtitleVisible = true;

			for ( c = 0; c < NUMMONSTERS; c++ )
			{
				kills[c] = 0;
			}

			// stop all sounds
#ifdef USE_FMOD
			if ( sound_group )
			{
				FMOD_ChannelGroup_Stop(sound_group);
			}
			if ( soundAmbient_group )
			{
				FMOD_ChannelGroup_Stop(soundAmbient_group);
			}
			if ( soundEnvironment_group )
			{
				FMOD_ChannelGroup_Stop(soundEnvironment_group);
			}
#elif defined USE_OPENAL
			if ( sound_group )
			{
				OPENAL_ChannelGroup_Stop(sound_group);
			}
			if ( soundAmbient_group )
			{
				OPENAL_ChannelGroup_Stop(soundAmbient_group);
			}
			if ( soundEnvironment_group )
			{
				OPENAL_ChannelGroup_Stop(soundEnvironment_group);
			}
#endif

			// send disconnect messages
			if (multiplayer == CLIENT)
			{
				strcpy((char*)net_packet->data, "DISCONNECT");
				net_packet->data[10] = clientnum;
				net_packet->address.host = net_server.host;
				net_packet->address.port = net_server.port;
				net_packet->len = 11;
				sendPacketSafe(net_sock, -1, net_packet, 0);
				printlog("disconnected from server.\n");
			}
			else if (multiplayer == SERVER)
			{
				for (x = 1; x < MAXPLAYERS; x++)
				{
					if ( client_disconnected[x] == true )
					{
						continue;
					}
					strcpy((char*)net_packet->data, "DISCONNECT");
					net_packet->data[10] = clientnum;
					net_packet->address.host = net_clients[x - 1].host;
					net_packet->address.port = net_clients[x - 1].port;
					net_packet->len = 11;
					sendPacketSafe(net_sock, -1, net_packet, x - 1);
					client_disconnected[x] = true;
				}
			}

			// clean up shopInv
			if ( multiplayer == CLIENT )
			{
				for ( x = 0; x < MAXPLAYERS; x++ )
				{
					if ( shopInv[x] )
					{
						list_FreeAll(shopInv[x]);
						free(shopInv[x]);
						shopInv[x] = nullptr;
					}
				}
			}

			// delete save game
			if ( !savethisgame && !endTutorial )
			{
				deleteSaveGame(multiplayer);
			}
			else
			{
				savethisgame = false;
			}

			if ( victory )
			{
				// conduct achievements
				if ( (victory == 1 && currentlevel >= 20)
					|| (victory == 2 && currentlevel >= 24)
					|| (victory == 3 && currentlevel >= 35) )
				{
					if ( conductPenniless )
					{
						steamAchievement("BARONY_ACH_PENNILESS_CONDUCT");
					}
					if ( conductFoodless )
					{
						steamAchievement("BARONY_ACH_FOODLESS_CONDUCT");
					}
					if ( conductVegetarian )
					{
						steamAchievement("BARONY_ACH_VEGETARIAN_CONDUCT");
					}
					if ( conductIlliterate )
					{
						steamAchievement("BARONY_ACH_ILLITERATE_CONDUCT");
					}

					if ( completionTime < 20 * 60 * TICKS_PER_SECOND )
					{
						steamAchievement("BARONY_ACH_BOOTS_OF_SPEED");
					}
				}

				if ( victory == 1 )
				{
					if ( currentlevel >= 20 )
					{
						if ( conductGameChallenges[CONDUCT_HARDCORE] )
						{
							steamAchievement("BARONY_ACH_HARDCORE");
						}
					}
				}
				else if ( victory == 2 )
				{
					if ( currentlevel >= 24 )
					{
						if ( conductGameChallenges[CONDUCT_HARDCORE] )
						{
							steamAchievement("BARONY_ACH_HARDCORE");
						}
					}
				}
				else if ( victory == 3 )
				{
					if ( currentlevel >= 35 )
					{
						if ( conductGameChallenges[CONDUCT_BRAWLER] )
						{
							steamAchievement("BARONY_ACH_BRAWLER");
						}
						if ( conductGameChallenges[CONDUCT_BLESSED_BOOTS_SPEED] )
						{
							steamAchievement("BARONY_ACH_PLUS_BOOTS_OF_SPEED");
						}
						if ( conductGameChallenges[CONDUCT_HARDCORE] )
						{
							steamAchievement("BARONY_ACH_POST_HARDCORE");
						}

						if ( client_classes[clientnum] == CLASS_MESMER )
						{
							steamAchievement("BARONY_ACH_COMMANDER_CHIEF");
						}
						else if ( client_classes[clientnum] == CLASS_BREWER )
						{
							steamAchievement("BARONY_ACH_DRUNK_POWER");
						}
						else if ( client_classes[clientnum] == CLASS_ACCURSED )
						{
							steamAchievement("BARONY_ACH_POWER_HUNGRY");
							if ( stats[clientnum]->EFFECTS[EFF_VAMPIRICAURA] && stats[clientnum]->EFFECTS_TIMERS[EFF_VAMPIRICAURA] == -2 )
							{
								if ( stats[clientnum] && (svFlags & SV_FLAG_HUNGER) )
								{
									steamAchievement("BARONY_ACH_BLOOD_IS_THE_LIFE");
								}
							}
						}
						else if ( client_classes[clientnum] == CLASS_HUNTER )
						{
							steamAchievement("BARONY_ACH_RANGER_DANGER");
							if ( conductGameChallenges[CONDUCT_RANGED_ONLY] )
							{
								steamAchievement("BARONY_ACH_GUDIPARIAN_BAZI");
							}
						}
						else if ( client_classes[clientnum] == CLASS_CONJURER )
						{
							steamAchievement("BARONY_ACH_TURN_UNDEAD");
						}
						else if ( client_classes[clientnum] == CLASS_SHAMAN )
						{
							steamAchievement("BARONY_ACH_MY_FINAL_FORM");
						}
						else if ( client_classes[clientnum] == CLASS_PUNISHER )
						{
							steamAchievement("BARONY_ACH_TIME_TO_SUFFER");
						}
						else if ( client_classes[clientnum] == CLASS_MACHINIST )
						{
							steamAchievement("BARONY_ACH_LIKE_CLOCKWORK");
						}

						if ( stats[clientnum] && stats[clientnum]->appearance == 0 )
						{
							switch ( stats[clientnum]->playerRace )
							{
								case RACE_SKELETON:
									steamAchievement("BARONY_ACH_BONY_BARON");
									break;
								case RACE_SUCCUBUS:
									steamAchievement("BARONY_ACH_BOMBSHELL_BARON");
									break;
								case RACE_GOATMAN:
									steamAchievement("BARONY_ACH_BLEATING_BARON");
									break;
								case RACE_VAMPIRE:
									steamAchievement("BARONY_ACH_BUCKTOOTH_BARON");
									break;
								case RACE_INCUBUS:
									steamAchievement("BARONY_ACH_BAD_BOY_BARON");
									break;
								case RACE_INSECTOID:
									steamAchievement("BARONY_ACH_BUGGAR_BARON");
									break;
								case RACE_AUTOMATON:
									steamAchievement("BARONY_ACH_BOILERPLATE_BARON");
									break;
								case RACE_GOBLIN:
									steamAchievement("BARONY_ACH_BAYOU_BARON");
									break;
								default:
									break;
							}
						}
					}
				}
			}

			// reset game
			darkmap = false;
			multiplayer = 0;
			currentlevel = 0;
			secretlevel = false;
			clientnum = 0;
			introstage = 1;
			intro = true;

			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				players[i]->inventoryUI.appraisal.timer = 0;
				players[i]->inventoryUI.appraisal.current_item = 0;
				players[i]->hud.reset();
				deinitShapeshiftHotbar(i);
				for ( c = 0; c < NUM_HOTBAR_ALTERNATES; ++c )
				{
					players[i]->hotbar.hotbarShapeshiftInit[c] = false;
				}
				players[i]->shootmode = true;
				players[i]->magic.clearSelectedSpells();
			}
			gameModeManager.currentSession.restoreSavedServerFlags();
			client_classes[0] = 0;
			for ( c = 0; c < MAXPLAYERS; c++ )
			{
				spellcastingAnimationManager_deactivate(&cast_animation[c]);
			}
			SDL_StopTextInput();

			// delete game data clutter
			list_FreeAll(&messages);
			list_FreeAll(&command_history);
			list_FreeAll(&safePacketsSent);
			for ( c = 0; c < MAXPLAYERS; c++ )
			{
				safePacketsReceivedMap[c].clear();
				players[c]->messageZone.deleteAllNotificationMessages();
			}
			for (c = 0; c < MAXPLAYERS; c++)
			{
				stats[c]->freePlayerEquipment();
				list_FreeAll(&stats[c]->inventory);
				list_FreeAll(&stats[c]->FOLLOWERS);
			}
			list_FreeAll(&removedEntities);
			for ( c = 0; c < MAXPLAYERS; c++ )
			{
				list_FreeAll(&chestInv[c]);
			}

			// default player stats
			for ( c = 0; c < MAXPLAYERS; c++ )
			{
				if ( c > 0 )
				{
					client_disconnected[c] = true;
				}
				else
				{
					client_disconnected[c] = false;
				}
				players[c]->entity = nullptr; //TODO: PLAYERSWAP VERIFY. Need to do anything else?
				players[c]->cleanUpOnEntityRemoval();
				stats[c]->sex = static_cast<sex_t>(0);
				stats[c]->appearance = 0;
				strcpy(stats[c]->name, "");
				stats[c]->type = HUMAN;
				stats[c]->playerRace = RACE_HUMAN;
				stats[c]->clearStats();
				entitiesToDelete[c].first = NULL;
				entitiesToDelete[c].last = NULL;
				if ( c == 0 )
				{
					initClass(c);
				}
			}

			// hack to fix these things from breaking everything...
			for ( int i = 0; i < MAXPLAYERS; ++i )
			{
				players[i]->hud.arm = nullptr;
				players[i]->hud.weapon = nullptr;
				players[i]->hud.magicLeftHand = nullptr;
				players[i]->hud.magicRightHand = nullptr;
			}

			// load menu level
			int menuMapType = 0;
			if ( victory == 3 )
			{
				menuMapType = loadMainMenuMap(true, true);
			}
			else
			{
				switch ( rand() % 2 )
				{
					case 0:
						menuMapType = loadMainMenuMap(true, false);
						break;
					case 1:
						menuMapType = loadMainMenuMap(false, false);
						break;
					default:
						break;
				}
			}
			for (int c = 0; c < MAXPLAYERS; ++c) {
				cameras[c].vang = 0;
			}
			numplayers = 0;
			assignActions(&map);
			generatePathMaps();
			gamePaused = false;
			if ( !victory )
			{
				fadefinished = false;
				fadeout = false;
#ifdef MUSIC
				if ( menuMapType )
				{
					playmusic(intromusic[2], true, false, false);
				}
				else
				{
					playmusic(intromusic[rand() % 2], true, false, false);
				}
#endif
			}
			else
			{
				if ( victory == 1 )
				{
					introstage = 7;
				}
				else if ( victory == 2 )
				{
					introstage = 8;
				}
				else if ( victory == 3 )
				{
					introstage = 10;
				}

				if ( movieCrawlType >= 0 ) // overrides the introstage 7,8,10 sequences for DLC monsters.
				{
					introstage = 11 + movieCrawlType;
				}
			}

			// finish handling invite
#ifdef STEAMWORKS
			if ( stillConnectingToLobby )
			{
				processLobbyInvite();
			}
#endif
#if defined USE_EOS
			if ( !directConnect )
			{
				if ( EOS.CurrentLobbyData.currentLobbyIsValid() )
				{
					EOS.leaveLobby();
				}
			}
#endif
		}
		else if ( introstage == 6 )     // introduction cutscene
		{
			fadefinished = false;
			fadeout = false;
			intromoviestage++;
			if ( intromoviestage >= 9 )
			{
#ifdef MUSIC
				playmusic(intromusic[1], true, false, false);
#endif
				introstage = 1;
				intromovietime = 0;
				intromoviestage = 0;
				int c;
				for ( c = 0; c < 30; c++ )
				{
					intromoviealpha[c] = 0;
				}
				movie = false;
			}
			else
			{
				intromovietime = 0;
				movie = true;
			}
		}
		else if ( introstage == 7 )     // win game sequence (herx)
		{
#ifdef MUSIC
			if ( firstendmoviestage == 0 )
			{
				playmusic(endgamemusic, true, true, false);
			}
#endif
			firstendmoviestage++;
			if ( firstendmoviestage >= 5 )
			{
				introstage = 4;
				firstendmovietime = 0;
				firstendmoviestage = 0;
				int c;
				for ( c = 0; c < 30; c++ )
				{
					firstendmoviealpha[c] = 0;
				}
				fadeout = true;
			}
			else
			{
				fadefinished = false;
				fadeout = false;
				firstendmovietime = 0;
				movie = true;
			}
		}
		else if ( introstage == 8 )     // win game sequence (devil)
		{
#ifdef MUSIC
			if ( secondendmoviestage == 0 )
			{
				playmusic(endgamemusic, true, true, false);
			}
#endif
			secondendmoviestage++;
			if ( secondendmoviestage >= 5 )
			{
				introstage = 4;
				secondendmovietime = 0;
				secondendmoviestage = 0;
				int c;
				for ( c = 0; c < 30; c++ )
				{
					secondendmoviealpha[c] = 0;
				}
				fadeout = true;
			}
			else
			{
				fadefinished = false;
				fadeout = false;
				secondendmovietime = 0;
				movie = true;
			}
		}
		else if ( introstage == 9 )     // mid game sequence
		{
#ifdef MUSIC
			if ( thirdendmoviestage == 0 )
			{
				playmusic(endgamemusic, true, true, false);
			}
#endif
			thirdendmoviestage++;
			if ( thirdendmoviestage >= thirdEndNumLines )
			{
				int c;
				for ( c = 0; c < 30; c++ )
				{
					thirdendmoviealpha[c] = 0;
				}
				fadefinished = false;
				fadeout = false;
				if ( multiplayer != CLIENT )
				{
					movie = false; // allow normal pause screen.
					thirdendmoviestage = 0;
					thirdendmovietime = 0;
					introstage = 1; // return to normal game functionality
					skipLevelsOnLoad = 5;
					loadnextlevel = true; // load the next level.
					pauseGame(1, false); // unpause game
				}
			}
			else
			{
				fadefinished = false;
				fadeout = false;
				thirdendmovietime = 0;
				movie = true;
			}
		}
		else if ( introstage == 10 )     // expansion end game sequence
		{
#ifdef MUSIC
			if ( fourthendmoviestage == 0 )
			{
				playmusic(endgamemusic, true, true, false);
			}
#endif
			fourthendmoviestage++;
			if ( fourthendmoviestage >= fourthEndNumLines )
			{
				int c;
				for ( c = 0; c < 30; c++ )
				{
					fourthendmoviealpha[c] = 0;
				}
				introstage = 4;
				fourthendmovietime = 0;
				fourthendmoviestage = 0;
				fadeout = true;
			}
			else
			{
				fadefinished = false;
				fadeout = false;
				fourthendmovietime = 0;
				movie = true;
			}
		}
		else if ( introstage >= 11 && introstage <= 15 )     // new mid and classic end sequences.
		{
			int movieType = introstage - 11;
			for ( int i = 0; i < 8; ++i )
			{
				if ( i != movieType )
				{
					// clean the other end stage credits.
					DLCendmovieStageAndTime[i][MOVIE_STAGE] = 0;
					DLCendmovieStageAndTime[i][MOVIE_TIME] = 0;
				}
			}
#ifdef MUSIC
			if ( DLCendmovieStageAndTime[movieType][MOVIE_STAGE] == 0 )
			{
				playmusic(endgamemusic, true, true, false);
			}
#endif
			DLCendmovieStageAndTime[movieType][MOVIE_STAGE]++;
			if ( movieType == MOVIE_CLASSIC_WIN_BAPHOMET_MONSTERS || movieType == MOVIE_CLASSIC_WIN_MONSTERS )
			{
				// win crawls.
				if ( DLCendmovieStageAndTime[movieType][MOVIE_STAGE] >= DLCendmovieNumLines[movieType] )
				{
					introstage = 4;
					DLCendmovieStageAndTime[movieType][MOVIE_TIME] = 0;
					DLCendmovieStageAndTime[movieType][MOVIE_STAGE] = 0;
					int c;
					for ( c = 0; c < 30; c++ )
					{
						DLCendmoviealpha[movieType][c] = 0;
					}
					fadeout = true;
				}
				else
				{
					fadefinished = false;
					fadeout = false;
					DLCendmovieStageAndTime[movieType][MOVIE_TIME] = 0;
					movie = true;
				}
			}
			else
			{
				// mid-game sequences
				if ( DLCendmovieStageAndTime[movieType][MOVIE_STAGE] >= DLCendmovieNumLines[movieType] )
				{
					int c;
					for ( c = 0; c < 30; c++ )
					{
						DLCendmoviealpha[movieType][c] = 0;
					}
					fadefinished = false;
					fadeout = false;
					if ( multiplayer != CLIENT )
					{
						movie = false; // allow normal pause screen.
						DLCendmovieStageAndTime[movieType][MOVIE_STAGE] = 0;
						DLCendmovieStageAndTime[movieType][MOVIE_TIME] = 0;
						introstage = 1; // return to normal game functionality
						if ( movieType == MOVIE_MIDGAME_HERX_MONSTERS )
						{
							skipLevelsOnLoad = 5;
						}
						else
						{
							skipLevelsOnLoad = 0;
						}
						loadnextlevel = true; // load the next level.
						pauseGame(1, false); // unpause game
					}
				}
				else
				{
					fadefinished = false;
					fadeout = false;
					DLCendmovieStageAndTime[movieType][MOVIE_TIME] = 0;
					movie = true;
				}
			}
		}
		else if ( introstage >= 16 && introstage <= 18 )     // expansion end game sequence DLC
		{
			int movieType = introstage - 11;
			for ( int i = 0; i < 8; ++i )
			{
				if ( i != movieType )
				{
					// clean the other end stage credits.
					DLCendmovieStageAndTime[i][MOVIE_STAGE] = 0;
					DLCendmovieStageAndTime[i][MOVIE_TIME] = 0;
				}
			}
#ifdef MUSIC
			if ( DLCendmovieStageAndTime[movieType][MOVIE_STAGE] == 0 )
			{
				playmusic(endgamemusic, true, true, false);
			}
#endif
			DLCendmovieStageAndTime[movieType][MOVIE_STAGE]++;
			if ( DLCendmovieStageAndTime[movieType][MOVIE_STAGE] >= DLCendmovieNumLines[movieType] )
			{
				int c;
				for ( c = 0; c < 30; c++ )
				{
					DLCendmoviealpha[movieType][c] = 0;
				}
				introstage = 4;
				DLCendmovieStageAndTime[movieType][MOVIE_TIME] = 0;
				DLCendmovieStageAndTime[movieType][MOVIE_STAGE] = 0;
				fadeout = true;
			}
			else
			{
				fadefinished = false;
				fadeout = false;
				DLCendmovieStageAndTime[movieType][MOVIE_TIME] = 0;
				movie = true;
			}
		}
	}

	// credits sequence
	if ( creditstage > 0 )
	{
		if ( (credittime >= 300 && (creditstage <= 11 || creditstage > 13)) || (credittime >= 180 && creditstage == 12) ||
		        (credittime >= 480 && creditstage == 13) || inputs.bMouseLeft(clientnum) || (inputs.bControllerInputPressed(clientnum, INJOY_MENU_NEXT) && rebindaction == -1) )
		{
			inputs.mouseClearLeft(clientnum);
			if ( rebindaction == -1 )
			{
				inputs.controllerClearInput(clientnum, INJOY_MENU_NEXT);
			}
			introstage = 4;
			fadeout = true;
		}

		// stages
		Uint32 colorBlue = SDL_MapRGBA(mainsurface->format, 0, 92, 255, 255);
		if ( creditstage == 1 )
		{
			ttfPrintTextFormattedColor(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(language[56]), yres / 2 - 9 - 18, colorBlue, language[56]);
			ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE02), yres / 2 - 9 + 18, CREDITSLINE02);
		}
		else if ( creditstage == 2 )
		{
			ttfPrintTextFormattedColor(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(language[57]), yres / 2 - 9 - 18, colorBlue, language[57]);
			ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE04), yres / 2 - 9 + 18, CREDITSLINE04);
		}
		else if ( creditstage == 3 )
		{
			ttfPrintTextFormattedColor(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(language[58]), yres / 2 - 9 - 18, colorBlue, language[58]);
			ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE06), yres / 2 - 9, CREDITSLINE06);
			ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE40), yres / 2 - 9 + 18, CREDITSLINE40);
		}
		else if ( creditstage == 4 )
		{
			ttfPrintTextFormattedColor(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(language[59]), yres / 2 - 9 - 18, colorBlue, language[59]);
			ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE39), yres / 2 + 9, CREDITSLINE39);
		}
		else if ( creditstage == 5 )
		{
			ttfPrintTextFormattedColor(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(language[60]), yres / 2 - 9 - 18, colorBlue, language[60]);
			ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE11), yres / 2 - 9, CREDITSLINE11);
			ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE08), yres / 2 - 9 + 18, CREDITSLINE08);
			ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE09), yres / 2 + 9 + 18 * 1, CREDITSLINE09);
		}
		else if ( creditstage == 6 )
		{
			ttfPrintTextFormattedColor(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(language[61]), yres / 2 - 9 - 18, colorBlue, language[61]);
			ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE13), yres / 2 - 9 + 18, CREDITSLINE13);
		}
		else if ( creditstage == 7 )
		{
			ttfPrintTextFormattedColor(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(language[62]), yres / 2 - 9 - 18 * 4, colorBlue, language[62]);
			ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE15), yres / 2 - 9 - 18 * 2, CREDITSLINE15);
			ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE16), yres / 2 - 9 - 18 * 1, CREDITSLINE16);
			ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE17), yres / 2 - 9, CREDITSLINE17);
			ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE18), yres / 2 + 9, CREDITSLINE18);
			ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE19), yres / 2 + 9 + 18 * 1, CREDITSLINE19);
			ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE20), yres / 2 + 9 + 18 * 2, CREDITSLINE20);
			ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE21), yres / 2 + 9 + 18 * 3, CREDITSLINE21);
		}
		else if ( creditstage == 8 )
		{
			ttfPrintTextFormattedColor(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(language[63]), yres / 2 - 9 - 18 * 4, colorBlue, language[63]);
			ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE23), yres / 2 - 9 - 18 * 2, CREDITSLINE23);
			ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE24), yres / 2 - 9 - 18 * 1, CREDITSLINE24);
			ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE25), yres / 2 - 9, CREDITSLINE25);
			ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE26), yres / 2 + 9, CREDITSLINE26);
			ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE27), yres / 2 + 9 + 18 * 1, CREDITSLINE27);
			ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE28), yres / 2 + 9 + 18 * 2, CREDITSLINE28);
			ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE29), yres / 2 + 9 + 18 * 3, CREDITSLINE29);
			ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE30), yres / 2 + 9 + 18 * 4, CREDITSLINE30);
		}
		else if ( creditstage == 9 )
		{
			ttfPrintTextFormattedColor(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(language[2585]), yres / 2 - 9 - 18, colorBlue, language[2585]);
			ttfPrintTextFormattedColor(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(language[2586]), yres / 2 - 9, colorBlue, language[2586]);
			ttfPrintTextFormattedColor(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(language[2587]), yres / 2 - 9 + 18, colorBlue, language[2587]);
			ttfPrintTextFormattedColor(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(language[2588]), yres / 2 + 9 + 18, colorBlue, language[2588]);
		}
		else if ( creditstage == 10 )
		{
			ttfPrintTextFormattedColor(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(language[64]), yres / 2 - 9 - 18, colorBlue, language[64]);
			ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(language[65]), yres / 2 - 9 + 18, language[65]);
		}
		else if ( creditstage == 11 )
		{

			// title
			SDL_Rect src;
			src.x = 0;
			src.y = 0;
			src.w = title_bmp->w;
			src.h = title_bmp->h;
			SDL_Rect dest;
			dest.x = xres / 2 - (title_bmp->w) / 2;
			dest.y = yres / 2 - title_bmp->h / 2 - 96;
			dest.w = xres;
			dest.h = yres;
			drawImage(title_bmp, &src, &dest);
			// text
			ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2) * strlen(language[66]), yres / 2, language[66]);
			ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2) * strlen(language[67]), yres / 2 + 20, language[67]);
			ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2) * strlen(language[68]), yres / 2 + 40, language[68]);
			ttfPrintTextFormattedColor(ttf16, xres / 2 - (TTF16_WIDTH / 2) * strlen(language[69]), yres / 2 + 60, colorBlue, language[69]);

			// logo
			src.x = 0;
			src.y = 0;
			src.w = logo_bmp->w;
			src.h = logo_bmp->h;
			dest.x = xres / 2 - (logo_bmp->w) / 2;
			dest.y = yres / 2 + 80;
			dest.w = xres;
			dest.h = yres;
			drawImage(logo_bmp, &src, &dest);
		}
		else if ( creditstage == 13 )
		{
			ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE37), yres / 2 - 9, CREDITSLINE37);
			//ttfPrintTextFormattedColor(ttf16,xres/2-(TTF16_WIDTH/2)*strlen(CREDITSLINE37),yres/2+9,colorBlue,CREDITSLINE38);
		}
	}

	// intro sequence
	if ( intromoviestage > 0 )
	{
		SDL_Rect pos;
		pos.x = 0;
		pos.y = 0;
		pos.w = xres;
		pos.h = (((real_t)xres) / backdrop_cursed_bmp->w) * backdrop_cursed_bmp->h;
		drawImageScaled(backdrop_cursed_bmp, NULL, &pos);

		if ( intromovietime >= 600 || inputs.bMouseLeft(clientnum) || keystatus[SDL_SCANCODE_ESCAPE] ||
		        keystatus[SDL_SCANCODE_SPACE] || keystatus[SDL_SCANCODE_RETURN] || (intromovietime >= 120 && intromoviestage == 1) || (inputs.bControllerInputPressed(clientnum, INJOY_MENU_NEXT) && rebindaction == -1) )
		{
			intromovietime = 0;
			inputs.mouseClearLeft(clientnum);
			if ( rebindaction == -1 )
			{
				inputs.controllerClearInput(clientnum, INJOY_MENU_NEXT);
			}
			if ( intromoviestage != 9 )
			{
				intromoviestage++;
			}
			else
			{
				introstage = 6;
				fadeout = true;
			}
		}

		if ( intromoviestage >= 1 )
		{
			intromoviealpha[8] = std::min(intromoviealpha[8] + 2, 255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0, intromoviealpha[8]), 255) << 24;
			ttfPrintTextColor(ttf16, 16, yres - 32, color, true, language[1414]);
		}
		if ( intromoviestage >= 2 )
		{
			intromoviealpha[0] = std::min(intromoviealpha[0] + 2, 255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0, intromoviealpha[0]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, language[1415]);
		}
		if ( intromoviestage >= 3 )
		{
			intromoviealpha[1] = std::min(intromoviealpha[1] + 2, 255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0, intromoviealpha[1]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, language[1416]);
		}
		if ( intromoviestage >= 4 )
		{
			intromoviealpha[2] = std::min(intromoviealpha[2] + 2, 255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0, intromoviealpha[2]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, language[1417]);
		}
		if ( intromoviestage >= 5 )
		{
			intromoviealpha[3] = std::min(intromoviealpha[3] + 2, 255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0, intromoviealpha[3]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, language[1418]);
		}
		if ( intromoviestage >= 6 )
		{
			intromoviealpha[4] = std::min(intromoviealpha[4] + 2, 255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0, intromoviealpha[4]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, language[1419]);
		}
		if ( intromoviestage >= 7 )
		{
			intromoviealpha[5] = std::min(intromoviealpha[5] + 2, 255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0, intromoviealpha[5]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, language[1420]);
		}
		if ( intromoviestage >= 8 )
		{
			intromoviealpha[6] = std::min(intromoviealpha[6] + 2, 255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0, intromoviealpha[6]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, language[1421]);
		}
		if ( intromoviestage == 9 )
		{
			intromoviealpha[7] = std::min(intromoviealpha[7] + 2, 255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0, intromoviealpha[7]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, language[1422]);
		}
	}

	// first end sequence (defeating herx)
	if ( firstendmoviestage > 0 )
	{
		SDL_Rect pos;
		pos.x = 0;
		pos.y = 0;
		pos.w = xres;
		pos.h = (((real_t)xres) / backdrop_minotaur_bmp->w) * backdrop_minotaur_bmp->h;
		drawImageScaled(backdrop_minotaur_bmp, NULL, &pos);

		if ( firstendmovietime >= 600 || inputs.bMouseLeft(clientnum) || keystatus[SDL_SCANCODE_ESCAPE] ||
		        keystatus[SDL_SCANCODE_SPACE] || keystatus[SDL_SCANCODE_RETURN] || (firstendmovietime >= 120 && firstendmoviestage == 1) )
		{
			firstendmovietime = 0;
			inputs.mouseClearLeft(clientnum);
			if ( firstendmoviestage != 5 )
			{
				firstendmoviestage++;
			}
			else
			{
				introstage = 7;
				fadeout = true;
			}
		}

		if ( firstendmoviestage >= 1 )
		{
			firstendmoviealpha[8] = std::min(firstendmoviealpha[8] + 2, 255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0, firstendmoviealpha[8]), 255) << 24;
			ttfPrintTextColor(ttf16, 16, yres - 32, color, true, language[1414]);

			int titlex = 16;
			int titley = 12;
			// epilogues
			if ( epilogueMultiplayerType == CLIENT )
			{
				if ( strcmp(epilogueHostName, "") )
				{
					ttfPrintTextFormattedColor(ttf16, titlex, titley, color, language[3819], epilogueHostName, language[3821 + epilogueHostRace]); // says who's story type it is.
				}
			}
			else
			{
				ttfPrintTextFormattedColor(ttf16, titlex, titley, color, language[3830], language[3821 + epilogueHostRace]); // says who's story type it is
			}
		}
		if ( firstendmoviestage >= 2 )
		{
			firstendmoviealpha[0] = std::min(firstendmoviealpha[0] + 2, 255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0, firstendmoviealpha[0]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, language[1423]);
		}
		if ( firstendmoviestage >= 3 )
		{
			firstendmoviealpha[1] = std::min(firstendmoviealpha[1] + 2, 255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0, firstendmoviealpha[1]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, language[1424]);
		}
		if ( firstendmoviestage >= 4 )
		{
			firstendmoviealpha[2] = std::min(firstendmoviealpha[2] + 2, 255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0, firstendmoviealpha[2]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, language[1425]);
		}
		if ( firstendmoviestage == 5 )
		{
			firstendmoviealpha[3] = std::min(firstendmoviealpha[3] + 2, 255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0, firstendmoviealpha[3]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, language[1426]);
		}
	}

	// second end sequence (defeating the devil)
	if ( secondendmoviestage > 0 )
	{
		SDL_Rect pos;
		pos.x = 0;
		pos.y = 0;
		pos.w = xres;
		pos.h = (((real_t)xres) / backdrop_minotaur_bmp->w) * backdrop_minotaur_bmp->h;
		drawImageScaled(backdrop_minotaur_bmp, NULL, &pos);

		if ( secondendmovietime >= 600 || inputs.bMouseLeft(clientnum) || keystatus[SDL_SCANCODE_ESCAPE] ||
		        keystatus[SDL_SCANCODE_SPACE] || keystatus[SDL_SCANCODE_RETURN] || (secondendmovietime >= 120 && secondendmoviestage == 1) )
		{
			secondendmovietime = 0;
			inputs.mouseClearLeft(clientnum);
			if ( secondendmoviestage != 7 )
			{
				secondendmoviestage++;
			}
			else
			{
				introstage = 8;
				fadeout = true;
			}
		}

		if ( secondendmoviestage >= 1 )
		{
			secondendmoviealpha[8] = std::min(secondendmoviealpha[8] + 2, 255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0, secondendmoviealpha[8]), 255) << 24;
			ttfPrintTextColor(ttf16, 16, yres - 32, color, true, language[1414]);

			int titlex = 16;
			int titley = 12;
			// epilogues
			if ( epilogueMultiplayerType == CLIENT )
			{
				if ( strcmp(epilogueHostName, "") )
				{
					ttfPrintTextFormattedColor(ttf16, titlex, titley, color, language[3819], epilogueHostName, language[3821 + epilogueHostRace]); // says who's story type it is.
				}
			}
			else
			{
				ttfPrintTextFormattedColor(ttf16, titlex, titley, color, language[3830], language[3821 + epilogueHostRace]); // says who's story type it is
			}
		}
		if ( secondendmoviestage >= 2 )
		{
			secondendmoviealpha[0] = std::min(secondendmoviealpha[0] + 2, 255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0, secondendmoviealpha[0]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 22, color, true, language[1427]);
		}
		if ( secondendmoviestage >= 3 )
		{
			secondendmoviealpha[1] = std::min(secondendmoviealpha[1] + 2, 255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0, secondendmoviealpha[1]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, language[1428]);
		}
		if ( secondendmoviestage >= 4 )
		{
			secondendmoviealpha[2] = std::min(secondendmoviealpha[2] + 2, 255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0, secondendmoviealpha[2]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, language[1429]);
		}
		if ( secondendmoviestage >= 5 )
		{
			secondendmoviealpha[3] = std::min(secondendmoviealpha[3] + 2, 255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0, secondendmoviealpha[3]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, language[1430]);
		}
		if ( secondendmoviestage >= 6 )
		{
			secondendmoviealpha[4] = std::min(secondendmoviealpha[4] + 2, 255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0, secondendmoviealpha[4]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, language[1431]);
		}
		if ( secondendmoviestage == 7 )
		{
			secondendmoviealpha[5] = std::min(secondendmoviealpha[5] + 2, 255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0, secondendmoviealpha[5]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, language[1432]);
		}
	}

	// third end movie stage
	if ( thirdendmoviestage > 0 )
	{
		SDL_Rect pos;
		pos.x = 0;
		pos.y = 0;
		pos.w = xres;
		pos.h = (((real_t)xres) / backdrop_minotaur_bmp->w) * backdrop_minotaur_bmp->h;
		drawRect(&pos, 0, 255);
		drawImageScaled(backdrop_minotaur_bmp, NULL, &pos);

		if ( thirdendmovietime >= 600 || inputs.bMouseLeft(clientnum) || keystatus[SDL_SCANCODE_ESCAPE] ||
			keystatus[SDL_SCANCODE_SPACE] || keystatus[SDL_SCANCODE_RETURN] || (thirdendmovietime >= 120 && thirdendmoviestage == 1) )
		{
			thirdendmovietime = 0;
			inputs.mouseClearLeft(clientnum);
			if ( thirdendmoviestage < thirdEndNumLines )
			{
				thirdendmoviestage++;
			}
			else if ( thirdendmoviestage == thirdEndNumLines )
			{
				if ( multiplayer != CLIENT )
				{
					fadeout = true;
					++thirdendmoviestage;
				}
			}
		}
		Uint32 color = 0x00FFFFFF;
		if ( thirdendmoviestage >= 1 )
		{
			if ( thirdendmoviestage >= 6 )
			{
				thirdendmoviealpha[8] = std::max(thirdendmoviealpha[8] - 4, 0); // click to continue decrease alpha
				if ( thirdendmoviealpha[8] == 0 )
				{
					thirdendmoviealpha[10] = std::min(thirdendmoviealpha[10] + 4, 255);
					color = 0x00FFFFFF;
					color += std::min(std::max(0, thirdendmoviealpha[10]), 255) << 24;
					if ( multiplayer == CLIENT )
					{
						ttfPrintTextColor(ttf16, 16, yres - 32, color, true, language[3833]);
					}
					else
					{
						ttfPrintTextColor(ttf16, 16, yres - 32, color, true, language[3832]);
					}
				}
			}
			else
			{
				thirdendmoviealpha[8] = std::min(thirdendmoviealpha[8] + 2, 255); // click to continue increase alpha
			}
			color = 0x00FFFFFF;
			color += std::min(std::max(0, thirdendmoviealpha[8]), 255) << 24;
			ttfPrintTextColor(ttf16, 16, yres - 32, color, true, language[2606]);

			if ( stats[0] )
			{
				int titlex = 16;
				int titley = 12;
				int race = RACE_HUMAN;
				if ( stats[0]->playerRace > 0 && stats[0]->appearance == 0 )
				{
					race = stats[0]->playerRace;
				}
				// interludes
				if ( multiplayer == CLIENT )
				{
					if ( strcmp(stats[0]->name, "") )
					{
						ttfPrintTextFormattedColor(ttf16, titlex, titley, color, language[3820], stats[0]->name, language[3821 + race]); // says who's story type it is.
					}
				}
				else
				{
					ttfPrintTextFormattedColor(ttf16, titlex, titley, color, language[3831], language[3821 + race]); // says who's story type it is.
				}
			}
		}
		if ( thirdendmoviestage >= 2 )
		{
			thirdendmoviealpha[0] = std::min(thirdendmoviealpha[0] + 2, 255);
			color = 0x00FFFFFF;
			color += std::min(std::max(0, thirdendmoviealpha[0]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, language[2600]);
		}
		if ( thirdendmoviestage >= 3 )
		{
			thirdendmoviealpha[1] = std::min(thirdendmoviealpha[1] + 2, 255);
			color = 0x00FFFFFF;
			color += std::min(std::max(0, thirdendmoviealpha[1]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, language[2601]);
		}
		if ( thirdendmoviestage >= 4 )
		{
			thirdendmoviealpha[2] = std::min(thirdendmoviealpha[2] + 2, 255);
			color = 0x00FFFFFF;
			color += std::min(std::max(0, thirdendmoviealpha[2]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, language[2602]);
		}
		if ( thirdendmoviestage >= 5 )
		{
			thirdendmoviealpha[3] = std::min(thirdendmoviealpha[3] + 2, 255);
			color = 0x00FFFFFF;
			color += std::min(std::max(0, thirdendmoviealpha[3]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, language[2603]);
		}
	}
	// fourth (expansion) end movie stage
	if ( fourthendmoviestage > 0 )
	{
		SDL_Rect pos;
		pos.x = 0;
		pos.y = 0;
		pos.w = xres;
		pos.h = (((real_t)xres) / backdrop_blessed_bmp->w) * backdrop_blessed_bmp->h;
		drawRect(&pos, 0, 255);
		drawImageScaled(backdrop_blessed_bmp, NULL, &pos);

		if ( fourthendmovietime >= 600 
			|| (inputs.bMouseLeft(clientnum) 
				&& fourthendmoviestage < 10 
				&& fourthendmoviestage != 10 
				&& fourthendmoviestage != 5
				&& fourthendmoviestage != 1)
			|| (fourthendmovietime >= 120 && fourthendmoviestage == 1)
			|| (fourthendmovietime >= 60 && fourthendmoviestage == 5)
			|| (fourthendmovietime >= 240 && fourthendmoviestage == 10)
			|| (fourthendmovietime >= 200 && fourthendmoviestage == 11)
			|| (fourthendmovietime >= 60 && fourthendmoviestage == 12)
			|| (fourthendmovietime >= 400 && fourthendmoviestage == 13)
			)
		{
			fourthendmovietime = 0;
			inputs.mouseClearLeft(clientnum);
			if ( fourthendmoviestage < fourthEndNumLines )
			{
				fourthendmoviestage++;
			}
			else if ( fourthendmoviestage == fourthEndNumLines )
			{
				fadeout = true;
				introstage = 10;
			}
		}
		Uint32 color = 0x00FFFFFF;
		if ( fourthendmoviestage >= 1 )
		{
			if ( fourthendmoviestage >= 10 )
			{
				fourthendmoviealpha[8] = std::max(fourthendmoviealpha[8] - 2, 0);
			}
			else
			{
				fourthendmoviealpha[8] = std::min(fourthendmoviealpha[8] + 2, 255);
			}
			color = 0x00FFFFFF;
			color += std::min(std::max(0, fourthendmoviealpha[8]), 255) << 24;
			ttfPrintTextColor(ttf16, 16, yres - 32, color, true, language[2606]);

			if ( stats[0] )
			{
				color = 0x00FFFFFF;
				if ( fourthendmoviestage >= 10 )
				{
					fourthendmoviealpha[9] = std::max(fourthendmoviealpha[9] - 2, 0);
				}
				else
				{
					fourthendmoviealpha[9] = std::min(fourthendmoviealpha[9] + 2, 255);
				}
				color += std::min(std::max(0, fourthendmoviealpha[9]), 255) << 24;
				int titlex = 16;
				int titley = 12;
				if ( epilogueMultiplayerType == CLIENT )
				{
					if ( strcmp(epilogueHostName, "") )
					{
						ttfPrintTextFormattedColor(ttf16, titlex, titley, color, language[3819], epilogueHostName, language[3821 + epilogueHostRace]); // says who's story type it is.
					}
				}
				else
				{
					ttfPrintTextFormattedColor(ttf16, titlex, titley, color, language[3830], language[3821 + epilogueHostRace]); // singleplayer story
				}
			}
		}
		if ( fourthendmoviestage >= 2 )
		{
			if ( fourthendmoviestage < 5 )
			{
				fourthendmoviealpha[0] = std::min(fourthendmoviealpha[0] + 2, 255);
			}
			color = 0x00FFFFFF;
			color += std::min(std::max(0, fourthendmoviealpha[0]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, language[2607]);
		}
		if ( fourthendmoviestage >= 3 )
		{
			if ( fourthendmoviestage < 5 )
			{
				fourthendmoviealpha[1] = std::min(fourthendmoviealpha[1] + 2, 255);
			}
			color = 0x00FFFFFF;
			color += std::min(std::max(0, fourthendmoviealpha[1]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, language[2608]);
		}
		if ( fourthendmoviestage >= 4 )
		{
			if ( fourthendmoviestage < 5 )
			{
				fourthendmoviealpha[2] = std::min(fourthendmoviealpha[2] + 2, 255);
			}
			color = 0x00FFFFFF;
			color += std::min(std::max(0, fourthendmoviealpha[2]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, language[2609]);
		}
		if ( fourthendmoviestage >= 5 )
		{
			fourthendmoviealpha[0] = std::max(fourthendmoviealpha[2] - 2, 0);
			fourthendmoviealpha[1] = std::max(fourthendmoviealpha[2] - 2, 0);
			fourthendmoviealpha[2] = std::max(fourthendmoviealpha[2] - 2, 0);
		}
		if ( fourthendmoviestage >= 6 )
		{
			if ( fourthendmoviestage < 10 )
			{
				fourthendmoviealpha[3] = std::min(fourthendmoviealpha[3] + 2, 255);
			}
			color = 0x00FFFFFF;
			color += std::min(std::max(0, fourthendmoviealpha[3]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, language[2610]);
		}
		if ( fourthendmoviestage >= 7 )
		{
			if ( fourthendmoviestage < 10 )
			{
				fourthendmoviealpha[4] = std::min(fourthendmoviealpha[4] + 2, 255);
			}
			color = 0x00FFFFFF;
			color += std::min(std::max(0, fourthendmoviealpha[4]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, language[2611]);
		}
		if ( fourthendmoviestage >= 8 )
		{
			if ( fourthendmoviestage < 10 )
			{
				fourthendmoviealpha[5] = std::min(fourthendmoviealpha[5] + 2, 255);
			}
			color = 0x00FFFFFF;
			color += std::min(std::max(0, fourthendmoviealpha[5]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, language[2612]);
		}
		if ( fourthendmoviestage >= 9 )
		{
			if ( fourthendmoviestage < 10 )
			{
				fourthendmoviealpha[6] = std::min(fourthendmoviealpha[6] + 2, 255);
			}
			color = 0x00FFFFFF;
			color += std::min(std::max(0, fourthendmoviealpha[6]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, language[2613]);
		}
		if ( fourthendmoviestage >= 10 )
		{
			fourthendmoviealpha[3] = std::max(fourthendmoviealpha[3] - 2, 0);
			fourthendmoviealpha[4] = std::max(fourthendmoviealpha[4] - 2, 0);
			fourthendmoviealpha[5] = std::max(fourthendmoviealpha[5] - 2, 0);
			fourthendmoviealpha[6] = std::max(fourthendmoviealpha[6] - 2, 0);
		}
		if ( fourthendmoviestage >= 11 )
		{
			fourthendmoviealpha[7] = std::min(fourthendmoviealpha[7] + 2, 255);
			color = 0x00FFFFFF;
			color += std::min(std::max(0, fourthendmoviealpha[7]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres/ 2) - 256, (yres / 2) - 64, color, true, language[2614]);
			if ( fourthendmovietime % 50 == 0 )
			{
				steamAchievement("BARONY_ACH_ALWAYS_WAITING");
			}
		}
		if ( fourthendmoviestage >= 13 )
		{
			fadealpha = std::min(fadealpha + 2, 255);
		}
	}

	// new end movie stage
	int movieType = -1;
	for ( int i = 0; i < 8; ++i )
	{
		if ( DLCendmovieStageAndTime[i][MOVIE_STAGE] > 0 )
		{
			movieType = i;
			break;
		}
	}
	if ( movieType >= MOVIE_MIDGAME_HERX_MONSTERS && movieType <= MOVIE_CLASSIC_WIN_BAPHOMET_MONSTERS )
	{
		SDL_Rect pos;
		pos.x = 0;
		pos.y = 0;
		pos.w = xres;
		pos.h = (((real_t)xres) / backdrop_minotaur_bmp->w) * backdrop_minotaur_bmp->h;
		drawRect(&pos, 0, 255);
		drawImageScaled(backdrop_minotaur_bmp, NULL, &pos);

		if ( DLCendmovieStageAndTime[movieType][MOVIE_TIME] >= 600 || inputs.bMouseLeft(clientnum) || keystatus[SDL_SCANCODE_ESCAPE] ||
			keystatus[SDL_SCANCODE_SPACE] || keystatus[SDL_SCANCODE_RETURN] || (DLCendmovieStageAndTime[movieType][MOVIE_TIME] >= 120 && DLCendmovieStageAndTime[movieType][MOVIE_STAGE] == 1) )
		{
			DLCendmovieStageAndTime[movieType][MOVIE_TIME] = 0;
			inputs.mouseClearLeft(clientnum);
			if ( DLCendmovieStageAndTime[movieType][MOVIE_STAGE] < DLCendmovieNumLines[movieType] )
			{
				DLCendmovieStageAndTime[movieType][MOVIE_STAGE]++;
			}
			else if ( DLCendmovieStageAndTime[movieType][MOVIE_STAGE] == DLCendmovieNumLines[movieType] )
			{
				if ( movieType == MOVIE_MIDGAME_BAPHOMET_HUMAN_AUTOMATON
					|| movieType == MOVIE_MIDGAME_BAPHOMET_MONSTERS
					|| movieType == MOVIE_MIDGAME_HERX_MONSTERS )
				{
					// midgame - clients pause until host continues.
					if ( multiplayer != CLIENT )
					{
						fadeout = true;
						++DLCendmovieStageAndTime[movieType][MOVIE_STAGE];
					}
				}
				else
				{
					// endgame - fade and return to credits.
					if ( movieType == MOVIE_CLASSIC_WIN_BAPHOMET_MONSTERS
						|| movieType == MOVIE_CLASSIC_WIN_MONSTERS )
					{
						// classic mode end.
						introstage = 11 + movieType;
					}
					fadeout = true;
				}
			}
		}

		std::vector<char*> langEntries;
		switch ( movieType )
		{
			case MOVIE_MIDGAME_HERX_MONSTERS:
				langEntries.push_back(language[3771]);
				langEntries.push_back(language[3772]);
				langEntries.push_back(language[3773]);
				langEntries.push_back(language[3774]);
				langEntries.push_back(language[3775]);
				break;
			case MOVIE_MIDGAME_BAPHOMET_MONSTERS:
				langEntries.push_back(language[3776]);
				langEntries.push_back(language[3777]);
				langEntries.push_back(language[3778]);
				langEntries.push_back(language[3779]);
				langEntries.push_back(language[3780]);
				langEntries.push_back(language[3781]);
				break;
			case MOVIE_MIDGAME_BAPHOMET_HUMAN_AUTOMATON:
				langEntries.push_back(language[3782]);
				langEntries.push_back(language[3783]);
				langEntries.push_back(language[3784]);
				langEntries.push_back(language[3785]);
				langEntries.push_back(language[3786]);
				langEntries.push_back(language[3787]);
				break;
			case MOVIE_CLASSIC_WIN_MONSTERS:
				langEntries.push_back(language[3788]);
				langEntries.push_back(language[3789]);
				langEntries.push_back(language[3790]);
				langEntries.push_back(language[3791]);
				break;
			case MOVIE_CLASSIC_WIN_BAPHOMET_MONSTERS:
				langEntries.push_back(language[3792]);
				langEntries.push_back(language[3793]);
				langEntries.push_back(language[3794]);
				langEntries.push_back(language[3795]);
				langEntries.push_back(language[3796]);
				langEntries.push_back(language[3797]);
				break;
			default:
				break;
		}

		Uint32 color = 0x00FFFFFF;
		if ( DLCendmovieStageAndTime[movieType][MOVIE_STAGE] >= 1 )
		{
			if ( !(movieType == MOVIE_CLASSIC_WIN_BAPHOMET_MONSTERS
				|| movieType == MOVIE_CLASSIC_WIN_MONSTERS) )
			{
				// only do this on interludes, not intermission.
				if ( DLCendmovieStageAndTime[movieType][MOVIE_STAGE] >= DLCendmovieNumLines[movieType] )
				{
					DLCendmoviealpha[movieType][8] = std::max(DLCendmoviealpha[movieType][8] - 4, 0); // click to continue decrease alpha
					if ( DLCendmoviealpha[movieType][8] == 0 )
					{
						DLCendmoviealpha[movieType][10] = std::min(DLCendmoviealpha[movieType][10] + 4, 255);
						color = 0x00FFFFFF;
						color += std::min(std::max(0, DLCendmoviealpha[movieType][10]), 255) << 24;
						if ( multiplayer == CLIENT )
						{
							ttfPrintTextColor(ttf16, 16, yres - 32, color, true, language[3833]);
						}
						else
						{
							ttfPrintTextColor(ttf16, 16, yres - 32, color, true, language[3832]);
						}
					}
				}
				else
				{
					DLCendmoviealpha[movieType][8] = std::min(DLCendmoviealpha[movieType][8] + 2, 255); // click to continue increase alpha
				}
			}
			else
			{
				DLCendmoviealpha[movieType][8] = std::min(DLCendmoviealpha[movieType][8] + 2, 255); // click to continue increase alpha
			}

			color = 0x00FFFFFF;
			color += std::min(std::max(0, DLCendmoviealpha[movieType][8]), 255) << 24;
			ttfPrintTextColor(ttf16, 16, yres - 32, color, true, language[2606]); // click to continue

			if ( stats[0] )
			{
				int titlex = 16;
				int titley = 12;

				if ( movieType == MOVIE_CLASSIC_WIN_BAPHOMET_MONSTERS
					|| movieType == MOVIE_CLASSIC_WIN_MONSTERS )
				{
					// epilogues
					if ( epilogueMultiplayerType == CLIENT )
					{
						if ( strcmp(epilogueHostName, "") )
						{
							ttfPrintTextFormattedColor(ttf16, titlex, titley, color, language[3819], epilogueHostName, language[3821 + epilogueHostRace]); // says who's story type it is.
						}
					}
					else
					{
						ttfPrintTextFormattedColor(ttf16, titlex, titley, color, language[3830], language[3821 + epilogueHostRace]); // says who's story type it is
					}
				}
				else
				{
					int race = RACE_HUMAN;
					if ( stats[0]->playerRace > 0 && stats[0]->appearance == 0 )
					{
						race = stats[0]->playerRace;
					}
					// interludes
					if ( multiplayer == CLIENT )
					{
						if ( strcmp(stats[0]->name, "") )
						{
							ttfPrintTextFormattedColor(ttf16, titlex, titley, color, language[3820], stats[0]->name, language[3821 + race]); // says who's story type it is.
						}
					}
					else
					{
						ttfPrintTextFormattedColor(ttf16, titlex, titley, color, language[3831], language[3821 + race]); // says who's story type it is.
					}
				}
			}
		}

		int index = 0;
		for ( index = 0; index < DLCendmovieNumLines[movieType]; ++index )
		{
			if ( DLCendmovieStageAndTime[movieType][MOVIE_STAGE] >= index + 2 && langEntries.size() > index )
			{
				DLCendmoviealpha[movieType][index] = std::min(DLCendmoviealpha[movieType][index] + 2, 255);
				color = 0x00FFFFFF;
				color += std::min(std::max(0, DLCendmoviealpha[movieType][index]), 255) << 24;
				ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, langEntries.at(index));
			}
		}
	}
	else if ( movieType > MOVIE_CLASSIC_WIN_BAPHOMET_MONSTERS )
	{
		SDL_Rect pos;
		pos.x = 0;
		pos.y = 0;
		pos.w = xres;
		pos.h = (((real_t)xres) / backdrop_blessed_bmp->w) * backdrop_blessed_bmp->h;
		drawRect(&pos, 0, 255);
		drawImageScaled(backdrop_blessed_bmp, NULL, &pos);

		if ( DLCendmovieStageAndTime[movieType][MOVIE_TIME] >= 600
			|| (inputs.bMouseLeft(clientnum)
				&& DLCendmovieStageAndTime[movieType][MOVIE_STAGE] < 10
				&& DLCendmovieStageAndTime[movieType][MOVIE_STAGE] != 10
				&& DLCendmovieStageAndTime[movieType][MOVIE_STAGE] != 5
				&& DLCendmovieStageAndTime[movieType][MOVIE_STAGE] != 1)
			|| (DLCendmovieStageAndTime[movieType][MOVIE_TIME] >= 120 && DLCendmovieStageAndTime[movieType][MOVIE_STAGE] == 1)
			|| (DLCendmovieStageAndTime[movieType][MOVIE_TIME] >= 60 && DLCendmovieStageAndTime[movieType][MOVIE_STAGE] == 5)
			|| (DLCendmovieStageAndTime[movieType][MOVIE_TIME] >= 240 && DLCendmovieStageAndTime[movieType][MOVIE_STAGE] == 10)
			|| (DLCendmovieStageAndTime[movieType][MOVIE_TIME] >= 200 && DLCendmovieStageAndTime[movieType][MOVIE_STAGE] == 11)
			|| (DLCendmovieStageAndTime[movieType][MOVIE_TIME] >= 60 && DLCendmovieStageAndTime[movieType][MOVIE_STAGE] == 12)
			|| (DLCendmovieStageAndTime[movieType][MOVIE_TIME] >= 400 && DLCendmovieStageAndTime[movieType][MOVIE_STAGE] == 13)
			)
		{
			DLCendmovieStageAndTime[movieType][MOVIE_TIME] = 0;
			inputs.mouseClearLeft(clientnum);
			if ( DLCendmovieStageAndTime[movieType][MOVIE_STAGE] < DLCendmovieNumLines[movieType] )
			{
				DLCendmovieStageAndTime[movieType][MOVIE_STAGE]++;
			}
			else if ( DLCendmovieStageAndTime[movieType][MOVIE_STAGE] == DLCendmovieNumLines[movieType] )
			{
				fadeout = true;
				introstage = 11 + movieType;
			}
		}

		std::vector<char*> langEntries;
		switch ( movieType )
		{
			case MOVIE_WIN_AUTOMATON:
				langEntries.push_back(language[3798]);
				langEntries.push_back(language[3799]);
				langEntries.push_back(language[3800]);
				langEntries.push_back(language[3801]);
				langEntries.push_back(language[3802]);
				langEntries.push_back(language[3803]);
				langEntries.push_back(language[3804]);
				break;
			case MOVIE_WIN_DEMONS_UNDEAD:
				langEntries.push_back(language[3805]);
				langEntries.push_back(language[3806]);
				langEntries.push_back(language[3807]);
				langEntries.push_back(language[3808]);
				langEntries.push_back(language[3809]);
				langEntries.push_back(language[3810]);
				langEntries.push_back(language[3811]);
				break;
			case MOVIE_WIN_BEASTS:
				langEntries.push_back(language[3812]);
				langEntries.push_back(language[3813]);
				langEntries.push_back(language[3814]);
				langEntries.push_back(language[3815]);
				langEntries.push_back(language[3816]);
				langEntries.push_back(language[3817]);
				langEntries.push_back(language[3818]);
				break;
			default:
				break;
		}

		Uint32 color = 0x00FFFFFF;
		if ( DLCendmovieStageAndTime[movieType][MOVIE_STAGE] >= 1 )
		{
			if ( DLCendmovieStageAndTime[movieType][MOVIE_STAGE] >= 10 )
			{
				DLCendmoviealpha[movieType][8] = std::max(DLCendmoviealpha[movieType][8] - 2, 0);
			}
			else
			{
				DLCendmoviealpha[movieType][8] = std::min(DLCendmoviealpha[movieType][8] + 2, 255);
			}
			color = 0x00FFFFFF;
			color += std::min(std::max(0, DLCendmoviealpha[movieType][8]), 255) << 24;
			ttfPrintTextColor(ttf16, 16, yres - 32, color, true, language[2606]); // click to continue.

			if ( stats[0] )
			{
				color = 0x00FFFFFF;
				if ( DLCendmovieStageAndTime[movieType][MOVIE_STAGE] >= 10 )
				{
					DLCendmoviealpha[movieType][9] = std::max(DLCendmoviealpha[movieType][9] - 2, 0);
				}
				else
				{
					DLCendmoviealpha[movieType][9] = std::min(DLCendmoviealpha[movieType][9] + 2, 255);
				}
				color += std::min(std::max(0, DLCendmoviealpha[movieType][9]), 255) << 24;
				int titlex = 16;
				int titley = 12;
				if ( epilogueMultiplayerType == CLIENT )
				{
					if ( strcmp(epilogueHostName, "") )
					{
						ttfPrintTextFormattedColor(ttf16, titlex, titley, color, language[3819], epilogueHostName, language[3821 + epilogueHostRace]); // says who's story type it is.
					}
				}
				else
				{
					ttfPrintTextFormattedColor(ttf16, titlex, titley, color, language[3830], language[3821 + epilogueHostRace]); // singleplayer story
				}
			}
		}
		if ( DLCendmovieStageAndTime[movieType][MOVIE_STAGE] >= 2 )
		{
			if ( DLCendmovieStageAndTime[movieType][MOVIE_STAGE] < 5 )
			{
				DLCendmoviealpha[movieType][0] = std::min(DLCendmoviealpha[movieType][0] + 2, 255);
			}
			color = 0x00FFFFFF;
			color += std::min(std::max(0, DLCendmoviealpha[movieType][0]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, langEntries.at(0));
		}
		if ( DLCendmovieStageAndTime[movieType][MOVIE_STAGE] >= 3 )
		{
			if ( DLCendmovieStageAndTime[movieType][MOVIE_STAGE] < 5 )
			{
				DLCendmoviealpha[movieType][1] = std::min(DLCendmoviealpha[movieType][1] + 2, 255);
			}
			color = 0x00FFFFFF;
			color += std::min(std::max(0, DLCendmoviealpha[movieType][1]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, langEntries.at(1));
		}
		if ( DLCendmovieStageAndTime[movieType][MOVIE_STAGE] >= 4 )
		{
			if ( DLCendmovieStageAndTime[movieType][MOVIE_STAGE] < 5 )
			{
				DLCendmoviealpha[movieType][2] = std::min(DLCendmoviealpha[movieType][2] + 2, 255);
			}
			color = 0x00FFFFFF;
			color += std::min(std::max(0, DLCendmoviealpha[movieType][2]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, langEntries.at(2));
		}
		if ( DLCendmovieStageAndTime[movieType][MOVIE_STAGE] >= 5 )
		{
			DLCendmoviealpha[movieType][0] = std::max(DLCendmoviealpha[movieType][2] - 2, 0);
			DLCendmoviealpha[movieType][1] = std::max(DLCendmoviealpha[movieType][2] - 2, 0);
			DLCendmoviealpha[movieType][2] = std::max(DLCendmoviealpha[movieType][2] - 2, 0);
		}
		if ( DLCendmovieStageAndTime[movieType][MOVIE_STAGE] >= 6 )
		{
			if ( DLCendmovieStageAndTime[movieType][MOVIE_STAGE] < 10 )
			{
				DLCendmoviealpha[movieType][3] = std::min(DLCendmoviealpha[movieType][3] + 2, 255);
			}
			color = 0x00FFFFFF;
			color += std::min(std::max(0, DLCendmoviealpha[movieType][3]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, langEntries.at(3));
		}
		if ( DLCendmovieStageAndTime[movieType][MOVIE_STAGE] >= 7 )
		{
			if ( DLCendmovieStageAndTime[movieType][MOVIE_STAGE] < 10 )
			{
				DLCendmoviealpha[movieType][4] = std::min(DLCendmoviealpha[movieType][4] + 2, 255);
			}
			color = 0x00FFFFFF;
			color += std::min(std::max(0, DLCendmoviealpha[movieType][4]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, langEntries.at(4));
		}
		if ( DLCendmovieStageAndTime[movieType][MOVIE_STAGE] >= 8 )
		{
			if ( DLCendmovieStageAndTime[movieType][MOVIE_STAGE] < 10 )
			{
				DLCendmoviealpha[movieType][5] = std::min(DLCendmoviealpha[movieType][5] + 2, 255);
			}
			color = 0x00FFFFFF;
			color += std::min(std::max(0, DLCendmoviealpha[movieType][5]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, langEntries.at(5));
		}
		if ( DLCendmovieStageAndTime[movieType][MOVIE_STAGE] >= 9 )
		{
			if ( DLCendmovieStageAndTime[movieType][MOVIE_STAGE] < 10 )
			{
				DLCendmoviealpha[movieType][6] = std::min(DLCendmoviealpha[movieType][6] + 2, 255);
			}
			color = 0x00FFFFFF;
			color += std::min(std::max(0, DLCendmoviealpha[movieType][6]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, langEntries.at(6));
		}
		if ( DLCendmovieStageAndTime[movieType][MOVIE_STAGE] >= 10 )
		{
			DLCendmoviealpha[movieType][3] = std::max(DLCendmoviealpha[movieType][3] - 2, 0);
			DLCendmoviealpha[movieType][4] = std::max(DLCendmoviealpha[movieType][4] - 2, 0);
			DLCendmoviealpha[movieType][5] = std::max(DLCendmoviealpha[movieType][5] - 2, 0);
			DLCendmoviealpha[movieType][6] = std::max(DLCendmoviealpha[movieType][6] - 2, 0);
		}
		if ( DLCendmovieStageAndTime[movieType][MOVIE_STAGE] >= 11 )
		{
			DLCendmoviealpha[movieType][7] = std::min(DLCendmoviealpha[movieType][7] + 2, 255);
			color = 0x00FFFFFF;
			color += std::min(std::max(0, DLCendmoviealpha[movieType][7]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres / 2) - 256, (yres / 2) - 64, color, true, language[2614]);
			if ( DLCendmovieStageAndTime[movieType][MOVIE_TIME] % 50 == 0 )
			{
				steamAchievement("BARONY_ACH_ALWAYS_WAITING");
			}
		}
		if ( DLCendmovieStageAndTime[movieType][MOVIE_STAGE] >= 13 )
		{
			fadealpha = std::min(fadealpha + 2, 255);
		}
	}
}

/*-------------------------------------------------------------------------------

	button functions

	this section contains numerous button functions for the game

-------------------------------------------------------------------------------*/

// opens the gameover window
void openGameoverWindow()
{
	node_t* node;
	buttonCloseSubwindow(nullptr);
	list_FreeAll(&button_l);
	deleteallbuttons = true;

	subwindow = 1;
	subx1 = xres / 2 - 288;
	subx2 = xres / 2 + 288;
	suby1 = yres / 2 - 160;
	suby2 = yres / 2 + 160;
	button_t* button;

	// calculate player score
	char scorenum[16];
	score_t* score = scoreConstructor();
	Uint32 total = totalScore(score);
	snprintf(scorenum, 16, "%d\n\n", total);

	bool madetop = false;
	list_t* scoresPtr = &topscores;
	if ( score->conductGameChallenges[CONDUCT_MULTIPLAYER] )
	{
		scoresPtr = &topscoresMultiplayer;
	}
	if ( !list_Size(scoresPtr) )
	{
		madetop = true;
	}
	else if ( list_Size(scoresPtr) < MAXTOPSCORES )
	{
		madetop = true;
	}
	else if ( totalScore((score_t*)scoresPtr->last->element) < total )
	{
		madetop = true;
	}

	scoreDeconstructor((void*)score);

	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		players[i]->shootmode = false;
	}

	if ( multiplayer == SINGLE )
	{
		strcpy(subtext, language[1133]);

		strcat(subtext, language[1134]);

		strcat(subtext, language[1135]);
		strcat(subtext, scorenum);

		if ( madetop )
		{
			strcat(subtext, language[1136]);
		}
		else
		{
			strcat(subtext, language[1137]);
		}

		// identify all inventory items
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			if ( !players[i]->isLocalPlayer() )
			{
				continue;
			}
			for ( node = stats[i]->inventory.first; node != NULL; node = node->next )
			{
				Item* item = (Item*)node->element;
				item->identified = true;
			}
		}

		// Restart
		button = newButton();
		strcpy(button->label, language[1138]);
		button->x = subx2 - strlen(language[1138]) * 12 - 16;
		button->y = suby2 - 28;
		button->sizex = strlen(language[1138]) * 12 + 8;
		button->sizey = 20;
		button->action = &buttonStartSingleplayer;
		button->visible = 1;
		button->focused = 1;
		button->joykey = joyimpulses[INJOY_MENU_NEXT];

		// Return to Main Menu
		button = newButton();
		strcpy(button->label, language[1139]);
		button->x = subx1 + 8;
		button->y = suby2 - 28;
		button->sizex = strlen(language[1139]) * 12 + 8;
		button->sizey = 20;
		button->action = &buttonEndGameConfirm;
		button->visible = 1;
		button->focused = 1;
		button->joykey = joyimpulses[INJOY_MENU_CANCEL];
	}
	else
	{
		strcpy(subtext, language[1140]);

		bool survivingPlayer = false;
		int c;
		for (c = 0; c < MAXPLAYERS; c++)
		{
			if (!client_disconnected[c] && players[c]->entity)
			{
				survivingPlayer = true;
				break;
			}
		}
		if ( survivingPlayer )
		{
			strcat(subtext, language[1141]);
		}
		else
		{
			strcat(subtext, language[1142]);
		}

		strcat(subtext, language[1143]);
		strcat(subtext, scorenum);

		strcat(subtext, "\n\n");

		// Okay
		button = newButton();
		strcpy(button->label, language[1144]);
		button->sizex = strlen(language[1144]) * 12 + 8;
		button->sizey = 20;
		button->x = subx1 + (subx2 - subx1) / 2 - button->sizex / 2;
		button->y = suby2 - 28;
		button->action = &buttonCloseSubwindow;
		button->visible = 1;
		button->focused = 1;
		button->joykey = joyimpulses[INJOY_MENU_NEXT];
	}

	// death hints
	if ( currentlevel / LENGTH_OF_LEVEL_REGION < 1 )
	{
		strcat(subtext, language[1145 + rand() % 15]);
	}

	// close button
	button = newButton();
	strcpy(button->label, "x");
	button->x = subx2 - 20;
	button->y = suby1;
	button->sizex = 20;
	button->sizey = 20;
	button->action = &buttonCloseSubwindow;
	button->visible = 1;
	button->focused = 1;
	button->key = SDL_SCANCODE_ESCAPE;
	button->joykey = joyimpulses[INJOY_MENU_CANCEL];
}

// get
void getResolutionList()
{
	// for now just use the resolution modes on the first
	// display.
	int numdisplays = SDL_GetNumVideoDisplays();
	int nummodes = SDL_GetNumDisplayModes(0);
	int im;
	printlog("display count: %d.\n", numdisplays);
	printlog("display mode count: %d.\n", nummodes);

	for (im = 0; im < nummodes; im++)
	{
		SDL_DisplayMode mode;
		SDL_GetDisplayMode(0, im, &mode);
		// resolutions below 960x600 are not supported
		if ( mode.w >= 960 && mode.h >= 600 )
		{
			resolution res(mode.w, mode.h);
			resolutions.push_back(res);
		}
	}

	// Sort by total number of pixels
	resolutions.sort([](resolution a, resolution b) {
		return std::get<0>(a) * std::get<1>(a) > std::get<0>(b) * std::get<1>(b);
	});
	resolutions.unique();

	const Uint32 kMaxResolutionsToDisplay = 22;
	for ( auto it = resolutions.end(); it != resolutions.begin() && resolutions.size() >= kMaxResolutionsToDisplay; )
	{
		// cull some resolutions in smallest -> largest order.
		--it;

		int w = std::get<0>(*it);
		int h = std::get<1>(*it);
		int ratio = (w * 100) / h;
		if ( ratio == 125 || ratio > 200 ) // 5x4, 32x15
		{
			it = resolutions.erase(it);
		}
		else if ( (w == 1152 && h == 864) || (w == 2048 && h == 1536) ) // explicit exclude for these odd 4x3 resolutions
		{
			it = resolutions.erase(it);
		}
	}
}

bool achievements_window = false;
int achievements_window_page = 1;

void buttonAchievementsUp(button_t* my)
{
	achievements_window_page =
		std::max(1, achievements_window_page - 1);
}

void buttonAchievementsDown(button_t* my)
{
	int num_achievements = achievementNames.size();
	if ( num_achievements == 0 )
	{
		return;
	}
	int max_pages = num_achievements / 6 + ((num_achievements % 6) ? 1 : 0);
	achievements_window_page = std::min(max_pages, achievements_window_page + 1);
}

// sets up the achievements window
void openAchievementsWindow()
{
	achievements_window = true;
	achievements_window_page = 1;
	subwindow = 1;
	subx1 = xres / 2 - 400;
	subx2 = xres / 2 + 400;
	suby1 = yres / 2 - 280;
	suby2 = yres / 2 + 280;
	strcpy(subtext, language[3971]);
#ifdef USE_EOS
	EOS.loadAchievementData();
#endif
	// close button
	{
		button_t* button = newButton();
		strcpy(button->label, "x");
		button->x = subx2 - 20;
		button->y = suby1;
		button->sizex = 20;
		button->sizey = 20;
		button->action = &closeAchievementsWindow;
		button->visible = 1;
		button->focused = 1;
		button->key = SDL_SCANCODE_ESCAPE;
		button->joykey = joyimpulses[INJOY_MENU_CANCEL];
	}

	// up / prev page button
	{
		button_t* button = newButton();
		strcpy(button->label, u8"\u25B2");
		button->x = subx2 - 33;
		button->y = suby1 + 84;
		button->sizex = 30;
		button->sizey = 30;
		button->action = &buttonAchievementsUp;
		button->visible = 1;
		button->focused = 1;
		button->key = SDL_SCANCODE_UP;
		button->joykey = joyimpulses[INJOY_MENU_SETTINGS_PREV];
	}

	// down / next page button
	{
		button_t* button = newButton();
		strcpy(button->label, u8"\u25BC");
		button->x = subx2 - 33;
		button->y = suby2 - 34;
		button->sizex = 30;
		button->sizey = 30;
		button->action = &buttonAchievementsDown;
		button->visible = 1;
		button->focused = 1;
		button->key = SDL_SCANCODE_DOWN;
		button->joykey = joyimpulses[INJOY_MENU_SETTINGS_NEXT];
	}
}

void closeAchievementsWindow(button_t* my)
{
	buttonCloseSubwindow(my);
}

// sets up the settings window
void openSettingsWindow()
{
	button_t* button;
	int c;

	getResolutionList();

	// set the "settings" variables
	settings_xres = xres;
	settings_yres = yres;
	settings_fov = fov;
	settings_svFlags = svFlags;
	settings_smoothlighting = smoothlighting;
	settings_fullscreen = fullscreen;
	settings_borderless = borderless;
	settings_shaking = shaking;
	settings_bobbing = bobbing;
	settings_spawn_blood = spawn_blood;
	settings_light_flicker = flickerLights;
	settings_vsync = verticalSync;
	settings_status_effect_icons = showStatusEffectIcons;
	settings_colorblind = colorblind;
	settings_gamma = vidgamma;
	settings_fps = fpsLimit;
	settings_sfxvolume = sfxvolume;
	settings_sfxAmbientVolume = sfxAmbientVolume;
	settings_sfxEnvironmentVolume = sfxEnvironmentVolume;
	settings_musvolume = musvolume;
	settings_minimap_ping_mute = minimapPingMute;
	settings_mute_audio_on_focus_lost = mute_audio_on_focus_lost;
	settings_mute_player_monster_sounds = mute_player_monster_sounds;
	settings_minimap_transparency_foreground = minimapTransparencyForeground;
	settings_minimap_transparency_background = minimapTransparencyBackground;
	settings_minimap_scale = minimapScale;
	settings_minimap_object_zoom = minimapObjectZoom;
	settings_uiscale_charactersheet = uiscale_charactersheet;
	settings_uiscale_skillspage = uiscale_skillspage;
	settings_uiscale_inventory = uiscale_inventory;
	settings_uiscale_hotbar = uiscale_hotbar;
	settings_uiscale_chatlog = uiscale_chatlog;
	settings_uiscale_playerbars = uiscale_playerbars;
	settings_hide_statusbar = hide_statusbar;
	settings_hide_playertags = hide_playertags;
	settings_show_skill_values = show_skill_values;
	settings_disableMultithreadedSteamNetworking = disableMultithreadedSteamNetworking;
	settings_disableFPSLimitOnNetworkMessages = disableFPSLimitOnNetworkMessages;
	LobbyHandler.settings_crossplayEnabled = LobbyHandler.crossplayEnabled;
	for (c = 0; c < NUMIMPULSES; c++)
	{
		settings_impulses[c] = impulses[c];
	}
	for (c = 0; c < NUM_JOY_IMPULSES; c++)
	{
		settings_joyimpulses[c] = joyimpulses[c];
	}
	settings_reversemouse = reversemouse;
	settings_smoothmouse = smoothmouse;
	settings_disablemouserotationlimit = disablemouserotationlimit;
	settings_mousespeed = mousespeed;
	settings_broadcast = broadcast;
	settings_nohud = nohud;
	settings_auto_hotbar_new_items = auto_hotbar_new_items;
	for ( c = 0; c < NUM_HOTBAR_CATEGORIES; ++c )
	{
		settings_auto_hotbar_categories[c] = auto_hotbar_categories[c];
	}
	for ( c = 0; c < NUM_AUTOSORT_CATEGORIES; ++c )
	{
		settings_autosort_inventory_categories[c] = autosort_inventory_categories[c];
	}
	settings_hotbar_numkey_quick_add = hotbar_numkey_quick_add;
	settings_disable_messages = disable_messages;
	settings_right_click_protect = right_click_protect;
	settings_auto_appraise_new_items = auto_appraise_new_items;
	settings_lock_right_sidebar = players[clientnum]->characterSheet.lock_right_sidebar;
	settings_show_game_timer_always = show_game_timer_always;

	settings_gamepad_leftx_invert = gamepad_leftx_invert;
	settings_gamepad_lefty_invert = gamepad_lefty_invert;
	settings_gamepad_rightx_invert = gamepad_rightx_invert;
	settings_gamepad_righty_invert = gamepad_righty_invert;
	settings_gamepad_menux_invert = gamepad_menux_invert;
	settings_gamepad_menuy_invert = gamepad_menuy_invert;

	settings_gamepad_deadzone = gamepad_deadzone;
	settings_gamepad_rightx_sensitivity = gamepad_rightx_sensitivity;
	settings_gamepad_righty_sensitivity = gamepad_righty_sensitivity;
	settings_gamepad_menux_sensitivity = gamepad_menux_sensitivity;
	settings_gamepad_menuy_sensitivity = gamepad_menuy_sensitivity;

	// create settings window
	settings_window = true;
	subwindow = 1;
	//subx1 = xres/2-256;
	subx1 = xres / 2 - 448;
	//subx2 = xres/2+256;
	subx2 = xres / 2 + 448;
	//suby1 = yres/2-192;
	//suby2 = yres/2+192;
#ifdef PANDORA
	suby1 = yres / 2 - ((yres==480)?210:278);
	suby2 = yres / 2 + ((yres==480)?210:278);
#else
	suby1 = yres / 2 - 320;
	suby2 = yres / 2 + 320;
#endif
	strcpy(subtext, language[1306]);

	// close button
	button = newButton();
	strcpy(button->label, "x");
	button->x = subx2 - 20;
	button->y = suby1;
	button->sizex = 20;
	button->sizey = 20;
	button->action = &buttonCloseSettingsSubwindow;
	button->visible = 1;
	button->focused = 1;
	button->key = SDL_SCANCODE_ESCAPE;
	button->joykey = joyimpulses[INJOY_MENU_CANCEL];

	// cancel button
	button = newButton();
	strcpy(button->label, language[1316]);
	button->x = subx1 + 8;
	button->y = suby2 - 28;
	button->sizex = strlen(language[1316]) * 12 + 8;
	button->sizey = 20;
	button->action = &buttonCloseSubwindow;
	button->visible = 1;
	button->focused = 1;

	// ok button
	button = newButton();
	strcpy(button->label, language[1433]);
	button->x = subx2 - strlen(language[1433]) * 12 - 16;
	button->y = suby2 - 28;
	button->sizex = strlen(language[1433]) * 12 + 8;
	button->sizey = 20;
	button->action = &buttonSettingsOK;
	button->visible = 1;
	button->focused = 1;
	button->key = SDL_SCANCODE_RETURN;
	button->joykey = joyimpulses[INJOY_MENU_NEXT];

	// accept button
	button = newButton();
	strcpy(button->label, language[1317]);
	button->x = subx2 - strlen(language[1317]) * 12 - 16 - strlen(language[1317]) * 12 - 16;
	button->y = suby2 - 28;
	button->sizex = strlen(language[1317]) * 12 + 8;
	button->sizey = 20;
	button->action = &buttonSettingsAccept;
	button->visible = 1;
	button->focused = 1;

	int tabx_so_far = subx1 + 16;

	//TODO: Select tab based off of dpad left & right.
	//TODO: Maybe golden highlighting & stuff.

	// video tab
	button = newButton();
	strcpy(button->label, language[1434]);
	button->x = tabx_so_far;
	button->y = suby1 + 24;
	button->sizex = strlen(language[1434]) * 12 + 8;
	button->sizey = 20;
	button->action = &buttonVideoTab;
	button->visible = 1;
	button->focused = 1;
	button_video_tab = button;

	tabx_so_far += strlen(language[1434]) * 12 + 8;

	// audio tab
	button = newButton();
	strcpy(button->label, language[1435]);
	button->x = tabx_so_far;
	button->y = suby1 + 24;
	button->sizex = strlen(language[1435]) * 12 + 8;
	button->sizey = 20;
	button->action = &buttonAudioTab;
	button->visible = 1;
	button->focused = 1;
	button_audio_tab = button;

	tabx_so_far += strlen(language[1435]) * 12 + 8;

	// keyboard tab
	button = newButton();
	strcpy(button->label, language[1436]);
	button->x = tabx_so_far;
	button->y = suby1 + 24;
	button->sizex = strlen(language[1436]) * 12 + 8;
	button->sizey = 20;
	button->action = &buttonKeyboardTab;
	button->visible = 1;
	button->focused = 1;
	button_keyboard_tab = button;

	tabx_so_far += strlen(language[1436]) * 12 + 8;

	// mouse tab
	button = newButton();
	strcpy(button->label, language[1437]);
	button->x = tabx_so_far;
	button->y = suby1 + 24;
	button->sizex = strlen(language[1437]) * 12 + 8;
	button->sizey = 20;
	button->action = &buttonMouseTab;
	button->visible = 1;
	button->focused = 1;
	button_mouse_tab = button;

	tabx_so_far += strlen(language[1437]) * 12 + 8;

	//Gamepad bindings tab.
	button = newButton();
	strcpy(button->label, language[1947]);
	button->x = tabx_so_far;
	button->y = suby1 + 24;
	button->sizex = strlen(language[1947]) * 12 + 8;
	button->sizey = 20;
	button->action = &buttonGamepadBindingsTab;
	button->visible = 1;
	button->focused = 1;
	button_gamepad_bindings_tab = button;

	tabx_so_far += strlen(language[1947]) * 12 + 8;

	//Gamepad settings tab.
	button = newButton();
	strcpy(button->label, language[2400]);
	button->x = tabx_so_far;
	button->y = suby1 + 24;
	button->sizex = strlen(language[2400]) * 12 + 8;
	button->sizey = 20;
	button->action = &buttonGamepadSettingsTab;
	button->visible = 1;
	button->focused = 1;
	button_gamepad_settings_tab = button;

	tabx_so_far += strlen(language[2400]) * 12 + 8;

	// misc tab
	button = newButton();
	strcpy(button->label, language[1438]);
	button->x =  tabx_so_far;
	button->y = suby1 + 24;
	button->sizex = strlen(language[1438]) * 12 + 8;
	button->sizey = 20;
	button->action = &buttonMiscTab;
	button->visible = 1;
	button->focused = 1;
	button_misc_tab = button;

	//Initialize resolution confirmation window related variables.
	resolutionChanged = false;
	resolutionConfirmationTimer = 0;

	changeSettingsTab(settings_tab);
}


// opens the wait window for steam lobby (getting lobby list, etc.)
void openSteamLobbyWaitWindow(button_t* my)
{
	button_t* button;

	// close current window
#ifdef STEAMWORKS
	bool prevConnectingToLobbyWindow = connectingToLobbyWindow;
	if ( connectingToLobbyWindow )
	{
		// we quit the connection window before joining lobby, but invite was mid-flight.
		denyLobbyJoinEvent = true;
	}
	else if ( joinLobbyWaitingForHostResponse )
	{
		// we quit the connection window after lobby join, but before host has accepted us.
		joinLobbyWaitingForHostResponse = false;
		buttonDisconnect(nullptr);
		openFailedConnectionWindow(CLIENT);
		strcpy(subtext, LobbyHandler_t::getLobbyJoinFailedConnectString(static_cast<int>(LobbyHandler_t::LOBBY_JOIN_CANCELLED)).c_str());
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
		buttonDisconnect(nullptr);
		openFailedConnectionWindow(CLIENT);
		strcpy(subtext, LobbyHandler_t::getLobbyJoinFailedConnectString(static_cast<int>(LobbyHandler_t::LOBBY_JOIN_CANCELLED)).c_str());
		return;
	}
#endif


	buttonCloseSubwindow(NULL);
	list_FreeAll(&button_l);
	deleteallbuttons = true;

	// create new window
	subwindow = 1;
#ifdef STEAMWORKS
	requestingLobbies = true;
#endif
#if defined USE_EOS
	EOS.bRequestingLobbies = true;
#endif // USE_EOS

	subx1 = xres / 2 - 256;
	subx2 = xres / 2 + 256;
	suby1 = yres / 2 - 64;
	suby2 = yres / 2 + 64;
	strcpy(subtext, language[1444]);
#ifdef STEAMWORKS
	//c_SteamMatchmaking_RequestLobbyList();
	//SteamMatchmaking()->RequestLobbyList(); //TODO: Is this sufficient for it to work?
	cpp_SteamMatchmaking_RequestLobbyList();
#endif

	LobbyHandler.selectedLobbyInList = 0;

#if defined USE_EOS
#ifdef STEAMWORKS
	if ( EOS.CurrentUserInfo.bUserLoggedIn )
	{
		EOS.searchLobbies(EOSFuncs::LobbyParameters_t::LobbySearchOptions::LOBBY_SEARCH_ALL,
			EOSFuncs::LobbyParameters_t::LobbyJoinOptions::LOBBY_DONT_JOIN, "");
	}
	else
	{
		EOS.bRequestingLobbies = false; // don't attempt search if not logged in
	}
#else
	EOS.searchLobbies(EOSFuncs::LobbyParameters_t::LobbySearchOptions::LOBBY_SEARCH_ALL,
		EOSFuncs::LobbyParameters_t::LobbyJoinOptions::LOBBY_DONT_JOIN, "");
#endif
#endif // USE_EOS


	// close button
	button = newButton();
	strcpy(button->label, "x");
	button->x = subx2 - 20;
	button->y = suby1;
	button->sizex = 20;
	button->sizey = 20;
	button->action = &buttonCloseSubwindow;
	button->visible = 1;
	button->focused = 1;
	button->key = SDL_SCANCODE_ESCAPE;
	button->joykey = joyimpulses[INJOY_MENU_CANCEL];

	// cancel button
	button = newButton();
	strcpy(button->label, language[1316]);
	button->sizex = strlen(language[1316]) * 12 + 8;
	button->sizey = 20;
	button->x = subx1 + (subx2 - subx1) / 2 - button->sizex / 2;
	button->y = suby2 - 28;
	button->action = &buttonCloseSubwindow;
	button->visible = 1;
	button->focused = 1;
}

// "failed to connect" message
void openFailedConnectionWindow(int mode)
{
	button_t* button;

	// close current window
	buttonCloseSubwindow(NULL);
	list_FreeAll(&button_l);
	deleteallbuttons = true;

	// create new window
	subwindow = 1;
	subx1 = xres / 2 - 256;
	subx2 = xres / 2 + 256;
	suby1 = yres / 2 - 64;
	suby2 = yres / 2 + 64;
	if ( directConnect )
	{
		if ( mode == CLIENT )
		{
			strcpy(subtext, language[1439]);
			strcat(subtext, SDLNet_GetError());
		}
		else if ( mode == SERVER )
		{
			strcpy(subtext, language[1440]);
			strcat(subtext, SDLNet_GetError());
		}
		else
		{
			strcpy(subtext, language[1443]);
		}
	}
	else
	{
		if ( mode == CLIENT )
		{
			strcpy(subtext, language[1441]);
		}
		else if ( mode == SERVER )
		{
			strcpy(subtext, language[1442]);
		}
		else
		{
			strcpy(subtext, language[1443]);
		}
	}

	// close button
	button = newButton();
	strcpy(button->label, "x");
	button->x = subx2 - 20;
	button->y = suby1;
	button->sizex = 20;
	button->sizey = 20;
	button->action = &buttonCloseSubwindow;
	button->visible = 1;
	button->focused = 1;
	button->key = SDL_SCANCODE_ESCAPE;
	button->joykey = joyimpulses[INJOY_MENU_CANCEL];

	// okay button
	button = newButton();
	strcpy(button->label, language[732]);
	button->x = subx2 - (subx2 - subx1) / 2 - strlen(language[732]) * 6;
	button->y = suby2 - 24;
	button->sizex = strlen(language[732]) * 12 + 8;
	button->sizey = 20;
	button->visible = 1;
	button->focused = 1;
	button->key = SDL_SCANCODE_RETURN;
	button->joykey = joyimpulses[INJOY_MENU_NEXT];

	if ( directConnect )
	{
		if ( mode == CLIENT )
		{
			button->action = &buttonJoinMultiplayer;
		}
		else if ( mode == SERVER )
		{
			button->action = &buttonHostMultiplayer;
		}
		else
		{
			button->action = &buttonCloseSubwindow;
		}
	}
	else
	{
		if ( mode == CLIENT )
		{
			button->action = &openSteamLobbyWaitWindow;
		}
		else if ( mode == SERVER )
		{
			button->action = &buttonCloseSubwindow;
		}
		else
		{
			button->action = &buttonCloseSubwindow;
		}
	}

	multiplayer = SINGLE;
	clientnum = 0;
}

// opens the lobby browser window (steam client only)
void openSteamLobbyBrowserWindow(button_t* my)
{
	button_t* button;

	// close current window
	buttonCloseSubwindow(NULL);
	list_FreeAll(&button_l);
	deleteallbuttons = true;

	// create new window
	subwindow = 1;
	subx1 = xres / 2 - 280;
	subx2 = xres / 2 + 280;
	suby1 = yres / 2 - 198;
	suby2 = yres / 2 + 198;
	strcpy(subtext, language[1334]);

	bool showCrossplayLobbyFilters = false;

	// setup lobby browser
#ifdef STEAMWORKS //TODO: Should this whole function be ifdeffed?
	selectedSteamLobby = 0;
#endif
#if defined USE_EOS
	EOS.LobbySearchResults.selectedLobby = 0;
	showCrossplayLobbyFilters = true;
#ifdef STEAMWORKS
	if ( !LobbyHandler.crossplayEnabled )
	{
		showCrossplayLobbyFilters = false;
	}
#endif
#endif
	slidery = 0;
	oslidery = 0;

	// close button
	button = newButton();
	strcpy(button->label, "x");
	button->x = subx2 - 20;
	button->y = suby1;
	button->sizex = 20;
	button->sizey = 20;
	button->action = &buttonCloseSubwindow;
	button->visible = 1;
	button->focused = 1;
	button->key = SDL_SCANCODE_ESCAPE;
	button->joykey = joyimpulses[INJOY_MENU_CANCEL];

	// join button
	button = newButton();
	strcpy(button->label, language[1445]);
	button->x = subx1 + 8;
	button->y = suby2 - 56;
	button->sizex = strlen(language[1445]) * 12 + 8;
	button->sizey = 20;
#if defined STEAMWORKS || defined USE_EOS
	button->action = &buttonSteamLobbyBrowserJoinGame;
#endif 
	button->visible = 1;
	button->focused = 1;
	button->key = SDL_SCANCODE_RETURN;
	button->joykey = joyimpulses[INJOY_MENU_NEXT];

	// refresh button
	button = newButton();
	strcpy(button->label, language[1446]);
	button->x = subx1 + 8;
	button->y = suby2 - 28;
	button->sizex = strlen(language[1446]) * 12 + 8;
	button->sizey = 20;
#if defined STEAMWORKS || defined USE_EOS
	button->action = &buttonSteamLobbyBrowserRefresh;
#endif
	button->visible = 1;
	button->focused = 1;
	button->joykey = joyimpulses[INJOY_MENU_REFRESH_LOBBY]; //"y" refreshes

	if ( showCrossplayLobbyFilters )
	{
		// filter button
		button = newButton();
		strcpy(button->label, language[3950]);
		button->sizex = strlen(language[3950]) * 12 + 8;
		button->sizey = 20;
		button->x = subx2 - 8 - button->sizex;
		button->y = suby2 - 56;
#if defined STEAMWORKS || defined USE_EOS
		button->action = &LobbyHandler_t::filterLobbyButton;
#endif
		button->visible = 1;
		button->focused = 1;

		button = newButton();
		button->x = 0;
		button->y = 0;
		button->sizex = 0;
		button->sizey = 0;
		button->visible = 0;
		button->focused = 0;
		strcpy(button->label, language[3953]);
		button->action = &LobbyHandler.searchLobbyWithFilter;
	}
}

// steam lobby browser join game
void buttonSteamLobbyBrowserJoinGame(button_t* my)
{
	LobbyHandler.setLobbyJoinTypeOfCurrentSelection();
	LobbyHandler.setP2PType(LobbyHandler.getJoiningType());
	if ( LobbyHandler.getJoiningType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM )
	{
#ifdef STEAMWORKS
		button_t* button;
		int lobbyIndex = std::min(std::max(0, selectedSteamLobby), MAX_STEAM_LOBBIES - 1);
		if ( lobbyIDs[lobbyIndex] )
		{
			// clear buttons
			list_FreeAll(&button_l);
			deleteallbuttons = true;

			// create new window
			subwindow = 1;
			subx1 = xres / 2 - 256;
			subx2 = xres / 2 + 256;
			suby1 = yres / 2 - 64;
			suby2 = yres / 2 + 64;
			strcpy(subtext, language[1447]);

			// close button
			button = newButton();
			strcpy(button->label, "x");
			button->x = subx2 - 20;
			button->y = suby1;
			button->sizex = 20;
			button->sizey = 20;
			button->action = &openSteamLobbyWaitWindow;
			button->visible = 1;
			button->focused = 1;
			button->key = SDL_SCANCODE_ESCAPE;
			button->joykey = joyimpulses[INJOY_MENU_CANCEL];

			// cancel button
			button = newButton();
			strcpy(button->label, language[1316]);
			button->sizex = strlen(language[1316]) * 12 + 8;
			button->sizey = 20;
			button->x = subx1 + (subx2 - subx1) / 2 - button->sizex / 2;
			button->y = suby2 - 28;
			button->action = &openSteamLobbyWaitWindow;
			button->visible = 1;
			button->focused = 1;

			connectingToLobby = true;
			connectingToLobbyWindow = true;
			strncpy(currentLobbyName, lobbyText[lobbyIndex], 31);
			LobbyHandler.steamValidateAndJoinLobby(*static_cast<CSteamID*>(lobbyIDs[lobbyIndex]));
		}
#endif
	}
	else if ( LobbyHandler.getJoiningType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY )
	{
#if defined USE_EOS
		button_t* button;
		int lobbyIndex = std::min(std::max(0, EOS.LobbySearchResults.selectedLobby), EOS.kMaxLobbiesToSearch - 1);
		if ( !EOS.LobbySearchResults.results.empty() )
		{
			// clear buttons
			list_FreeAll(&button_l);
			deleteallbuttons = true;

			// create new window
			subwindow = 1;
			subx1 = xres / 2 - 256;
			subx2 = xres / 2 + 256;
			suby1 = yres / 2 - 64;
			suby2 = yres / 2 + 64;
			strcpy(subtext, language[1447]);

			// close button
			button = newButton();
			strcpy(button->label, "x");
			button->x = subx2 - 20;
			button->y = suby1;
			button->sizex = 20;
			button->sizey = 20;
			button->action = &openSteamLobbyWaitWindow;
			button->visible = 1;
			button->focused = 1;
			button->key = SDL_SCANCODE_ESCAPE;
			button->joykey = joyimpulses[INJOY_MENU_CANCEL];

			// cancel button
			button = newButton();
			strcpy(button->label, language[1316]);
			button->sizex = strlen(language[1316]) * 12 + 8;
			button->sizey = 20;
			button->x = subx1 + (subx2 - subx1) / 2 - button->sizex / 2;
			button->y = suby2 - 28;
			button->action = &openSteamLobbyWaitWindow;
			button->visible = 1;
			button->focused = 1;

			EOS.bConnectingToLobby = true;
			EOS.bConnectingToLobbyWindow = true;
			strncpy(EOS.currentLobbyName, EOS.LobbySearchResults.getResultFromDisplayedIndex(lobbyIndex)->LobbyAttributes.lobbyName.c_str(), 31);
			std::string lobbyToJoin = EOS.LobbySearchResults.getResultFromDisplayedIndex(lobbyIndex)->LobbyId;
			EOS.searchLobbies(EOSFuncs::LobbyParameters_t::LobbySearchOptions::LOBBY_SEARCH_BY_LOBBYID,
				EOSFuncs::LobbyParameters_t::LobbyJoinOptions::LOBBY_JOIN_FIRST_SEARCH_RESULT,
				lobbyToJoin.c_str());
		}
#endif // USE_EOS
	}
	else if ( LobbyHandler.getJoiningType() == LobbyHandler_t::LobbyServiceType::LOBBY_DISABLE )
	{
		LobbyHandler_t::logError("Invalid lobby join type from current browser selection: %d, list size %d", LobbyHandler.selectedLobbyInList, LobbyHandler.lobbyDisplayedSearchResults.size());
	}
	return;
}

// steam lobby browser refresh
void buttonSteamLobbyBrowserRefresh(button_t* my)
{
#if defined STEAMWORKS || defined USE_EOS
	LobbyHandler.showLobbyFilters = false;
	openSteamLobbyWaitWindow(my);
#endif
}

// quit game button
void buttonQuitConfirm(button_t* my)
{
	subwindow = 0;
	introstage = 2; // prepares to quit the whole game
	fadeout = true;
}

// quit game button (no save)
void buttonQuitNoSaveConfirm(button_t* my)
{
	buttonQuitConfirm(my);
	deleteSaveGame(multiplayer);

	// make a highscore!
	saveScore();
}

// end game button
bool savethisgame = false;
void buttonEndGameConfirm(button_t* my)
{
	savethisgame = false;
	subwindow = 0;
	introstage = 5; // prepares to end the current game (throws to main menu)
	fadeout = true;
	//Edge case for freeing channeled spells on a client.
	if (multiplayer == CLIENT)
	{
		list_FreeAll(&channeledSpells[clientnum]);
	}
	if ( !intro )
	{
		pauseGame(2, false);
	}
}

void buttonEndGameConfirmSave(button_t* my)
{
	subwindow = 0;
	introstage = 5; // prepares to end the current game (throws to main menu)
	fadeout = true;
	savethisgame = true;
	if ( !intro )
	{
		pauseGame(2, false);
	}
}

// generic close window button
void buttonCloseSubwindow(button_t* my)
{
	int c;
	for ( c = 0; c < 512; c++ )
	{
		keystatus[c] = 0;
	}
	if ( !subwindow )
	{
		return;
	}

	loadGameSaveShowRectangle = 0;
	singleplayerSavegameFreeSlot = -1; // clear this value when closing window
	multiplayerSavegameFreeSlot = -1;  // clear this value when closing window
	directoryPath = "";
	gamemodsWindowClearVariables();
	if ( score_window || score_leaderboard_window )
	{
		// reset class loadout
		stats[0]->sex = static_cast<sex_t>(0);
		stats[0]->appearance = 0;
		stats[0]->playerRace = RACE_HUMAN;
		strcpy(stats[0]->name, "");
		stats[0]->type = HUMAN;
		client_classes[0] = 0;
		stats[0]->clearStats();
		initClass(0);
	}
	rebindkey = -1;
#ifdef STEAMWORKS
	requestingLobbies = false;
	connectingToLobbyWindow = false;
	connectingToLobby = false;
	joinLobbyWaitingForHostResponse = false;
#endif
#if defined USE_EOS
	EOS.bRequestingLobbies = false;
	EOS.bJoinLobbyWaitingForHostResponse = false;
	EOS.bConnectingToLobby = false;
	EOS.bConnectingToLobbyWindow = false;
#endif

#if !defined STEAMWORKS && !defined USE_EOS
	serialEnterWindow = false;
#endif

	score_window = 0;
	score_leaderboard_window = 0;
	gamemods_window = 0;
	gameModeManager.Tutorial.Menu.close();
	gameModeManager.Tutorial.FirstTimePrompt.close();
	savegames_window = 0;
	savegames_window_scroll = 0;
	achievements_window = false;
	achievements_window_page = 1;
	lobby_window = false;
	settings_window = false;
	connect_window = 0;
#ifdef STEAMWORKS
	if ( charcreation_step )
	{
		if ( lobbyToConnectTo )
		{
			// cancel lobby invitation acceptance
			cpp_Free_CSteamID(lobbyToConnectTo); //TODO: Bugger this.
			lobbyToConnectTo = NULL;
		}
	}
#endif

	charcreation_step = 0;
	subwindow = 0;
	if ( SDL_IsTextInputActive() )
	{
		SDL_StopTextInput();
	}
	playSound(138, 64);
}

void buttonCloseSettingsSubwindow(button_t* my)
{
	if ( rebindkey != -1 || rebindaction != -1 )
	{
		//Do not close settings subwindow if rebinding a key/gamepad button/whatever.
		return;
	}

	buttonCloseSubwindow(my);
}

void buttonCloseAndEndGameConfirm(button_t* my)
{
	//Edge case for freeing channeled spells on a client.
	if (multiplayer == CLIENT)
	{
		list_FreeAll(&channeledSpells[clientnum]);
	}
	buttonCloseSubwindow(my);
	buttonEndGameConfirmSave(my);
}

Uint32 charcreation_ticks = 0;

// move player forward through creation dialogue
void buttonContinue(button_t* my)
{
	if ( ticks - charcreation_ticks < TICKS_PER_SECOND / 10 )
	{
		return;
	}
	charcreation_ticks = ticks;
	if ( charcreation_step == 4 && !strcmp(stats[0]->name, "") )
	{
		return;
	}

	charcreation_step++;
	if ( charcreation_step == 3 && stats[0]->playerRace != RACE_HUMAN )
	{
		charcreation_step = 4; // skip appearance window
	}
	if ( charcreation_step == 4 )
	{
		if (inputstr != stats[0]->name)
		{
			inputstr = stats[0]->name;
#ifdef NINTENDO
			auto result = nxKeyboard("Enter your character's name");
			if (result.success)
			{
				strncpy(inputstr, result.str.c_str(), 21);
				inputstr[21] = '\0';
			}
#endif
		}
		SDL_StartTextInput();
		inputlen = 22;
	}
	else if ( charcreation_step == 5 )
	{
		// look for a gap in save game numbering
		savegameCurrentFileIndex = 0;
		for ( int fileNumber = 0; fileNumber < SAVE_GAMES_MAX; ++fileNumber )
		{
			if ( !saveGameExists(true, fileNumber) )
			{
				singleplayerSavegameFreeSlot = fileNumber;
				break;
			}
		}
		for ( int fileNumber = 0; fileNumber < SAVE_GAMES_MAX; ++fileNumber )
		{
			if ( !saveGameExists(false, fileNumber) )
			{
				multiplayerSavegameFreeSlot = fileNumber;
				break;
			}
		}
		if ( SDL_IsTextInputActive() )
		{
			lastname = (string)stats[0]->name;
			SDL_StopTextInput();
		}
#ifdef STEAMWORKS
		if ( lobbyToConnectTo )
		{
			charcreation_step = 0;

			// since we skip step 6 we never set the correct free slot.
			reloadSavegamesList(false);
			if ( multiplayerSavegameFreeSlot == -1 )
			{
				savegameCurrentFileIndex = 0;
				std::vector<std::tuple<int, int, int, std::string>>::reverse_iterator it = savegamesList.rbegin();
				for ( ; it != savegamesList.rend(); ++it )
				{
					std::tuple<int, int, int, std::string> entry = *it;
					if ( std::get<1>(entry) != SINGLE )
					{
						savegameCurrentFileIndex = std::get<2>(entry);
						break;
					}
				}
			}
			else
			{
				savegameCurrentFileIndex = multiplayerSavegameFreeSlot;
			}

			// clear buttons
			list_FreeAll(&button_l);
			deleteallbuttons = true;

			// create new window
			subwindow = 1;
			subx1 = xres / 2 - 256;
			subx2 = xres / 2 + 256;
			suby1 = yres / 2 - 64;
			suby2 = yres / 2 + 64;
			strcpy(subtext, language[1447]);

			// close button
			button_t* button = nullptr;
			button = newButton();
			strcpy(button->label, "x");
			button->x = subx2 - 20;
			button->y = suby1;
			button->sizex = 20;
			button->sizey = 20;
			button->action = &openSteamLobbyWaitWindow;
			button->visible = 1;
			button->focused = 1;
			button->key = SDL_SCANCODE_ESCAPE;
			button->joykey = joyimpulses[INJOY_MENU_CANCEL];

			// cancel button
			button = newButton();
			strcpy(button->label, language[1316]);
			button->sizex = strlen(language[1316]) * 12 + 8;
			button->sizey = 20;
			button->x = subx1 + (subx2 - subx1) / 2 - button->sizex / 2;
			button->y = suby2 - 28;
			button->action = &openSteamLobbyWaitWindow;
			button->visible = 1;
			button->focused = 1;

			connectingToLobby = true;
			connectingToLobbyWindow = true;
			strncpy( currentLobbyName, "", 31 );
			LobbyHandler.steamValidateAndJoinLobby(*static_cast<CSteamID*>(lobbyToConnectTo));
			cpp_Free_CSteamID(lobbyToConnectTo); //TODO: Bugger this.
			lobbyToConnectTo = NULL;
		}
#endif
	}
	else if ( charcreation_step == 6 )
	{
		// store this character into previous character.
		lastCreatedCharacterSex = stats[0]->sex;
		lastCreatedCharacterClass = client_classes[0];
		lastCreatedCharacterAppearance = stats[0]->appearance;
		lastCreatedCharacterRace = stats[0]->playerRace;

		if ( multiplayerselect != SINGLE )
		{
			if ( multiplayerSavegameFreeSlot == -1 )
			{
				savegameCurrentFileIndex = 0;
				std::vector<std::tuple<int, int, int, std::string>>::reverse_iterator it = savegamesList.rbegin();
				for ( ; it != savegamesList.rend(); ++it )
				{
					std::tuple<int, int, int, std::string> entry = *it;
					if ( std::get<1>(entry) != SINGLE )
					{
						savegameCurrentFileIndex = std::get<2>(entry);
						break;
					}
				}
			}
			else
			{
				savegameCurrentFileIndex = multiplayerSavegameFreeSlot;
			}
			multiplayerSavegameFreeSlot = -1;
		}

		if ( multiplayerselect == SINGLE )
		{
			if ( singleplayerSavegameFreeSlot == -1 )
			{
				savegameCurrentFileIndex = 0;
				std::vector<std::tuple<int, int, int, std::string>>::reverse_iterator it = savegamesList.rbegin();
				for ( ; it != savegamesList.rend(); ++it )
				{
					std::tuple<int, int, int, std::string> entry = *it;
					if ( std::get<1>(entry) == SINGLE )
					{
						savegameCurrentFileIndex = std::get<2>(entry);
						break;
					}
				}
			}
			else
			{
				savegameCurrentFileIndex = singleplayerSavegameFreeSlot;
			}
			buttonStartSingleplayer(my);
			singleplayerSavegameFreeSlot = -1;
		}
		else if ( multiplayerselect == SERVER || multiplayerselect == SERVERCROSSPLAY )
		{
#if (defined STEAMWORKS || defined USE_EOS)
			directConnect = false;
#else
			directConnect = true;
#endif
#ifdef STEAMWORKS
			if ( multiplayerselect == SERVERCROSSPLAY )
			{
				LobbyHandler.hostingType = LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY;
				LobbyHandler.setP2PType(LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY);
			}
			else
			{
				LobbyHandler.hostingType = LobbyHandler_t::LobbyServiceType::LOBBY_STEAM;
				LobbyHandler.setP2PType(LobbyHandler_t::LobbyServiceType::LOBBY_STEAM);
			}
#endif
			buttonHostMultiplayer(my);
		}
		else if ( multiplayerselect == CLIENT )
		{
#if (defined STEAMWORKS || defined USE_EOS)
			directConnect = false;
			openSteamLobbyWaitWindow(my);
#else
			directConnect = true;
			buttonJoinMultiplayer(my);
#endif
		}
		else if ( multiplayerselect == DIRECTSERVER )
		{
			directConnect = true;
			buttonHostMultiplayer(my);
		}
		else if ( multiplayerselect == DIRECTCLIENT )
		{
			directConnect = true;
			buttonJoinMultiplayer(my);
		}
	}
}

// move player backward through creation dialogue
void buttonBack(button_t* my)
{
	charcreation_step--;
	if (charcreation_step < 4)
	{
		playing_random_char = false;
	}
	

	if (charcreation_step == 3)
	{
		// If we've backed out, save what name was input for later
		lastname = (string)inputstr;
		SDL_StopTextInput();
		if ( stats[0]->playerRace != RACE_HUMAN )
		{
			charcreation_step = 2; // skip appearance window for non-human races.
		}
	}
	else if ( charcreation_step == 1 )
	{
		raceSelect = 0; // reset the race selection menu to select sex
		if ( stats[0]->playerRace != RACE_HUMAN )
		{
			stats[0]->clearStats();
			initClass(0);
		}
	}
	else if ( charcreation_step == 0 )
	{
		buttonCloseSubwindow(my);
	}
}

// start a singleplayer game
void buttonStartSingleplayer(button_t* my)
{
	buttonCloseSubwindow(my);
	multiplayer = SINGLE;
	numplayers = 0;
	introstage = 3;
	fadeout = true;
	if ( !intro )
	{
		// intro is true if starting from main menu, otherwise we're restarting the game.
		pauseGame(2, false);
	}
}

// host a multiplayer game
void buttonHostMultiplayer(button_t* my)
{
	button_t* button;

	// refresh keepalive
	int c;
	for ( c = 0; c < MAXPLAYERS; c++ )
	{
		client_keepalive[c] = ticks;
	}

	if ( !directConnect )
	{
		snprintf(portnumber_char, 6, "%d", DEFAULT_PORT);
		buttonHostLobby(my);
	}
	else
	{
		// close current window
		buttonCloseSubwindow(my);
		list_FreeAll(&button_l);
		deleteallbuttons = true;

		// open port window
		connect_window = SERVER;
		subwindow = 1;
		subx1 = xres / 2 - 128;
		subx2 = xres / 2 + 128;
		suby1 = yres / 2 - 56;
		suby2 = yres / 2 + 56;
		strcpy(subtext, language[1448]);

		// close button
		button = newButton();
		strcpy(button->label, "x");
		button->x = subx2 - 20;
		button->y = suby1;
		button->sizex = 20;
		button->sizey = 20;
		button->action = &buttonCloseSubwindow;
		button->visible = 1;
		button->focused = 1;
		button->key = SDL_SCANCODE_ESCAPE;
		button->joykey = joyimpulses[INJOY_MENU_CANCEL];

		// host button
		button = newButton();
		strcpy(button->label, language[1449]);
		button->sizex = strlen(language[1449]) * 12 + 8;
		button->sizey = 20;
		button->x = subx2 - button->sizex - 4;
		button->y = suby2 - 24;
		button->action = &buttonHostLobby;
		button->visible = 1;
		button->focused = 1;
		button->key = SDL_SCANCODE_RETURN;
		button->joykey = joyimpulses[INJOY_MENU_NEXT];

		// cancel button
		button = newButton();
		strcpy(button->label, language[1316]);
		button->sizex = strlen(language[1316]) * 12 + 8;
		button->sizey = 20;
		button->x = subx1 + 4;
		button->y = suby2 - 24;
		button->action = &buttonCloseSubwindow;
		button->visible = 1;
		button->focused = 1;
		strcpy(portnumber_char, last_port); //Copy the last used port.
	}
}

// join a multiplayer game
void buttonJoinMultiplayer(button_t* my)
{
	button_t* button;

	// close current window
	buttonCloseSubwindow(my);
	list_FreeAll(&button_l);
	deleteallbuttons = true;

	// open port window
	connect_window = CLIENT;
	subwindow = 1;
	subx1 = xres / 2 - 210;
	subx2 = xres / 2 + 210;
	suby1 = yres / 2 - 56;
	suby2 = yres / 2 + 56;
	strcpy(subtext, language[1450]);

	// close button
	button = newButton();
	strcpy(button->label, "x");
	button->x = subx2 - 20;
	button->y = suby1;
	button->sizex = 20;
	button->sizey = 20;
	button->action = &buttonCloseSubwindow;
	button->visible = 1;
	button->focused = 1;
	button->key = SDL_SCANCODE_ESCAPE;
	button->joykey = joyimpulses[INJOY_MENU_CANCEL];

	// join button
	button = newButton();
	strcpy(button->label, language[1451]);
	button->sizex = strlen(language[1451]) * 12 + 8;
	button->sizey = 20;
	button->x = subx2 - button->sizex - 4;
	button->y = suby2 - 24;
	button->action = &buttonJoinLobby;
	button->visible = 1;
	button->focused = 1;
	button->key = SDL_SCANCODE_RETURN;
	button->joykey = joyimpulses[INJOY_MENU_NEXT];

	// cancel button
	button = newButton();
	strcpy(button->label, language[1316]);
	button->x = subx1 + 4;
	button->y = suby2 - 24;
	button->sizex = strlen(language[1316]) * 12 + 8;
	button->sizey = 20;
	button->action = &buttonCloseSubwindow;
	button->visible = 1;
	button->focused = 1;

	strcpy(connectaddress, last_ip); //Copy the last used IP.
}

// starts a lobby as host
void buttonHostLobby(button_t* my)
{
	button_t* button;
	char *portnumbererr;
	int c;

	// close current window
	buttonCloseSubwindow(my);
	list_FreeAll(&button_l);
	deleteallbuttons = true;
	portnumber = (Uint16)strtol(portnumber_char, &portnumbererr, 10); // get the port number from the text field
	list_FreeAll(&lobbyChatboxMessages);

	if ( *portnumbererr != '\0' || portnumber < 1024 )
	{
		printlog("warning: invalid port number: %d\n", portnumber);
		openFailedConnectionWindow(SERVER);
		return;
	}

	newString(&lobbyChatboxMessages, 0xFFFFFFFF, language[1452]);
	if ( loadingsavegame )
	{
		newString(&lobbyChatboxMessages, 0xFFFFFFFF, language[1453]);
	}

	// close any existing net interfaces
	closeNetworkInterfaces();

	if ( !directConnect )
	{
		if ( LobbyHandler.getHostingType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM )
		{
#if defined STEAMWORKS
			for ( c = 0; c < MAXPLAYERS; c++ )
			{
				if ( steamIDRemote[c] )
				{
					cpp_Free_CSteamID(steamIDRemote[c]); //TODO: Bugger this.
					steamIDRemote[c] = NULL;
				}
			}
			currentLobbyType = k_ELobbyTypePrivate;
			cpp_SteamMatchmaking_CreateLobby(currentLobbyType, MAXPLAYERS);
#endif
		}
		else if ( LobbyHandler.getHostingType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY )
		{
#ifdef USE_EOS
			EOS.createLobby();
#endif
		}
	}
	else
	{
		// resolve host's address
		if (SDLNet_ResolveHost(&net_server, NULL, portnumber) == -1)
		{
			printlog( "warning: resolving host at localhost:%d has failed.\n", portnumber);
			openFailedConnectionWindow(SERVER);
			return;
		}

		// open sockets
		if (!(net_sock = SDLNet_UDP_Open(portnumber)))
		{
			printlog( "warning: SDLNet_UDP_open has failed: %s\n", SDLNet_GetError());
			openFailedConnectionWindow(SERVER);
			return;
		}
		if (!(net_tcpsock = SDLNet_TCP_Open(&net_server)))
		{
			printlog( "warning: SDLNet_TCP_open has failed: %s\n", SDLNet_GetError());
			openFailedConnectionWindow(SERVER);
			return;
		}
		tcpset = SDLNet_AllocSocketSet(MAXPLAYERS);
		SDLNet_TCP_AddSocket(tcpset, net_tcpsock);
	}

	// allocate data for client connections
	net_clients = (IPaddress*) malloc(sizeof(IPaddress) * MAXPLAYERS);
	net_tcpclients = (TCPsocket*) malloc(sizeof(TCPsocket) * MAXPLAYERS);
	for ( c = 0; c < MAXPLAYERS; c++ )
	{
		net_tcpclients[c] = NULL;
	}

	// allocate packet data
	if (!(net_packet = SDLNet_AllocPacket(NET_PACKET_SIZE)))
	{
		printlog( "warning: packet allocation failed: %s\n", SDLNet_GetError());
		openFailedConnectionWindow(SERVER);
		return;
	}

	if ( directConnect )
	{
		printlog( "server initialized successfully.\n");
	}
	else
	{
		printlog( "steam lobby opened successfully.\n");
	}

	// open lobby window
	multiplayer = SERVER;
	lobby_window = true;
	subwindow = 1;
	subx1 = xres / 2 - 400;
	subx2 = xres / 2 + 400;
#ifdef PANDORA
	suby1 = yres / 2 - ((yres==480)?230:290);
	suby2 = yres / 2 + ((yres==480)?230:290);
#else
	suby1 = yres / 2 - 300;
	suby2 = yres / 2 + 300;
#endif
	if ( directConnect )
	{
		strcpy(subtext, language[1454]);
		strcat(subtext, portnumber_char);
		strcat(subtext, language[1456]);
	}
	else
	{
		strcpy(subtext, language[1455]);
		strcat(subtext, language[1456]);
	}

	// start game button
	button = newButton();
	strcpy(button->label, language[1457]);
	button->sizex = strlen(language[1457]) * 12 + 8;
	button->sizey = 20;
	button->x = subx2 - button->sizex - 4;
	button->y = suby2 - 24;
	button->action = &buttonStartServer;
	button->visible = 1;
	button->focused = 1;
	button->joykey = joyimpulses[INJOY_MENU_NEXT];

	// disconnect button
	button = newButton();
	strcpy(button->label, language[1311]);
	button->sizex = strlen(language[1311]) * 12 + 8;
	button->sizey = 20;
	button->x = subx1 + 4;
	button->y = suby2 - 24;
	button->action = &buttonDisconnect;
	button->visible = 1;
	button->focused = 1;
	button->key = SDL_SCANCODE_ESCAPE;
	button->joykey = joyimpulses[INJOY_MENU_CANCEL];
	c = button->x + button->sizex + 4;

	// invite friends button
	if ( !directConnect )
	{
#ifdef STEAMWORKS
		if ( LobbyHandler.getHostingType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM )
		{
			button = newButton();
			strcpy(button->label, language[1458]);
			button->sizex = strlen(language[1458]) * 12 + 8;
			button->sizey = 20;
			button->x = c;
			button->y = suby2 - 24;
			button->action = &buttonInviteFriends;
			button->visible = 1;
			button->focused = 1;
		}
#endif
	}

	if ( loadingsavegame )
	{
		loadGame(clientnum);
	}

	strcpy(last_port, portnumber_char);
	saveConfig("default.cfg");
}

// joins a lobby as client
// if direct-ip, this is called directly after pressing join
// otherwise for matchmaking, this is called asynchronously after a matchmaking lobby has been joined
void buttonJoinLobby(button_t* my)
{
	button_t* button;
	int c;

	// refresh keepalive
	client_keepalive[0] = ticks;

	// close current window
	bool temp1 = false;
	bool temp2 = false;
	bool temp3 = false;
	bool temp4 = false;
#ifdef STEAMWORKS
	temp1 = connectingToLobby;
	temp2 = connectingToLobbyWindow;
	if ( !directConnect && LobbyHandler.getJoiningType() == LobbyHandler_t::LobbyServiceType::LOBBY_STEAM )
	{
		joinLobbyWaitingForHostResponse = true;
	}
#endif
#if defined USE_EOS
	temp3 = EOS.bConnectingToLobby;
	temp4 = EOS.bConnectingToLobbyWindow;
	if ( !directConnect && LobbyHandler.getJoiningType() == LobbyHandler_t::LobbyServiceType::LOBBY_CROSSPLAY)
	{
		EOS.bJoinLobbyWaitingForHostResponse = true;
	}
#endif
	if ( directConnect )
	{
		buttonCloseSubwindow(my);
	}
	list_FreeAll(&button_l);
	deleteallbuttons = true;
#ifdef STEAMWORKS
	connectingToLobby = temp1;
	connectingToLobbyWindow = temp2;
#endif
#if defined USE_EOS
	EOS.bConnectingToLobby = temp3;
	EOS.bConnectingToLobbyWindow = temp4;
#endif

	multiplayer = CLIENT;
	if ( loadingsavegame )
	{
		loadGame(getSaveGameClientnum(false));
	}

	// open wait window
	list_FreeAll(&lobbyChatboxMessages);
	newString(&lobbyChatboxMessages, 0xFFFFFFFF, language[1452]);
	subwindow = 1;
	subx1 = xres / 2 - 256;
	subx2 = xres / 2 + 256;
	suby1 = yres / 2 - 64;
	suby2 = yres / 2 + 64;
	strcpy(subtext, language[1459]);

	// close button
	button = newButton();
	strcpy(button->label, "x");
	button->x = subx2 - 20;
	button->y = suby1;
	button->sizex = 20;
	button->sizey = 20;
	button->action = &openSteamLobbyWaitWindow;
	button->visible = 1;
	button->focused = 1;
	button->key = SDL_SCANCODE_ESCAPE;
	button->joykey = joyimpulses[INJOY_MENU_CANCEL];

	// cancel button
	button = newButton();
	strcpy(button->label, language[1316]);
	button->sizex = strlen(language[1316]) * 12 + 8;
	button->sizey = 20;
	button->x = subx1 + (subx2 - subx1) / 2 - button->sizex / 2;
	button->y = suby2 - 28;
	button->action = &openSteamLobbyWaitWindow;
	button->visible = 1;
	button->focused = 1;

	if ( directConnect )
	{
		for ( c = 0; c < sizeof(connectaddress); c++ )
		{
			if ( connectaddress[c] == ':' )
			{
				break;
			}
		}		
		char *portnumbererr;
		strncpy(address, connectaddress, c); // get the address from the text field
		portnumber = (Uint16)strtol(&connectaddress[c + 1], &portnumbererr, 10); // get the port number from the text field
		if ( *portnumbererr != '\0' || portnumber < 1024 )
		{
			printlog("warning: invalid port number %d.\n", portnumber);
			SDLNet_SetError("Invalid address %s.\nExample: 192.168.0.100:12345", connectaddress);
			openFailedConnectionWindow(CLIENT);
			return;
		}
		strcpy(last_ip, connectaddress);
		saveConfig("default.cfg");
	}

	// close any existing net interfaces
	closeNetworkInterfaces();

	if ( directConnect )
	{
		// resolve host's address
		printlog("resolving host's address at %s:%d...\n", address, portnumber);
		if (SDLNet_ResolveHost(&net_server, address, portnumber) == -1)
		{
			printlog( "warning: resolving host at %s:%d has failed.\n", address, portnumber);
			openFailedConnectionWindow(CLIENT);
			return;
		}

		// open sockets
		printlog("opening TCP and UDP sockets...\n");
		if (!(net_sock = SDLNet_UDP_Open(0)))
		{
			printlog( "warning: SDLNet_UDP_open has failed.\n");
			openFailedConnectionWindow(CLIENT);
			return;
		}
		if (!(net_tcpsock = SDLNet_TCP_Open(&net_server)))
		{
			printlog( "warning: SDLNet_TCP_open has failed.\n");
			openFailedConnectionWindow(CLIENT);
			return;
		}
		tcpset = SDLNet_AllocSocketSet(MAXPLAYERS);
		SDLNet_TCP_AddSocket(tcpset, net_tcpsock);
	}

	// allocate packet data
	if (!(net_packet = SDLNet_AllocPacket(NET_PACKET_SIZE)))
	{
		printlog( "warning: packet allocation failed.\n");
		openFailedConnectionWindow(CLIENT);
		return;
	}

	if ( directConnect )
	{
		printlog( "successfully contacted server at %s:%d.\n", address, portnumber);
	}

	printlog( "submitting join request...\n");

	// send join request
	strcpy((char*)net_packet->data, "BARONY_JOIN_REQUEST");
	if ( loadingsavegame )
	{
		strncpy((char*)net_packet->data + 19, stats[getSaveGameClientnum(false)]->name, 22);
		SDLNet_Write32((Uint32)client_classes[getSaveGameClientnum(false)], &net_packet->data[42]);
		SDLNet_Write32((Uint32)stats[getSaveGameClientnum(false)]->sex, &net_packet->data[46]);
		Uint32 appearanceAndRace = ((Uint8)stats[getSaveGameClientnum(false)]->appearance << 8); // store in bits 8 - 15
		appearanceAndRace |= (Uint8)stats[getSaveGameClientnum(false)]->playerRace; // store in bits 0 - 7
		SDLNet_Write32(appearanceAndRace, &net_packet->data[50]);
		strcpy((char*)net_packet->data + 54, VERSION);
		net_packet->data[62] = 0;
		net_packet->data[63] = getSaveGameClientnum(false);
	}
	else
	{
		strncpy((char*)net_packet->data + 19, stats[0]->name, 22);
		SDLNet_Write32((Uint32)client_classes[0], &net_packet->data[42]);
		SDLNet_Write32((Uint32)stats[0]->sex, &net_packet->data[46]);
		Uint32 appearanceAndRace = ((Uint8)stats[0]->appearance << 8);
		appearanceAndRace |= ((Uint8)stats[0]->playerRace);
		SDLNet_Write32(appearanceAndRace, &net_packet->data[50]);
		strcpy((char*)net_packet->data + 54, VERSION);
		net_packet->data[62] = 0;
		net_packet->data[63] = 0;
	}
	if ( loadingsavegame )
	{
		// send over the map seed being used
		SDLNet_Write32(getSaveGameMapSeed(false), &net_packet->data[64]);
	}
	else
	{
		SDLNet_Write32(0, &net_packet->data[64]);
	}
	SDLNet_Write32(loadingsavegame, &net_packet->data[68]); // send unique game key
	net_packet->address.host = net_server.host;
	net_packet->address.port = net_server.port;
	net_packet->len = 72;
	if ( !directConnect )
	{
#if (defined STEAMWORKS || defined USE_EOS)
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
#endif
	}
	else
	{
		sendPacket(net_sock, -1, net_packet, 0);
	}
}

// starts the game as server
void buttonStartServer(button_t* my)
{
	int c;

	// close window
	buttonCloseSubwindow(my);

	multiplayer = SERVER;

	if ( !intro )
	{
		// intro is true if starting from main menu, otherwise we're restarting the game.
		// set the main menu camera to the player camera coordinates if restarting midgame.
		menucam.x = cameras[clientnum].x;
		menucam.y = cameras[clientnum].y;
		menucam.z = cameras[clientnum].z;
		menucam.ang = cameras[clientnum].ang;
		menucam.vang = cameras[clientnum].vang;
	}

	intro = true;
	introstage = 3;
	numplayers = 0;
	fadeout = true;

	// send the ok to start
	for ( c = 1; c < MAXPLAYERS; c++ )
	{
		if ( !client_disconnected[c] )
		{
			if ( !loadingsavegame || !intro )
			{
				stats[c]->clearStats();
				initClass(c);
			}
			else
			{
				loadGame(c);
			}
		}
	}
	uniqueGameKey = prng_get_uint();
	if ( !uniqueGameKey )
	{
		uniqueGameKey++;
	}
	for ( c = 1; c < MAXPLAYERS; c++ )
	{
		if ( client_disconnected[c] )
		{
			continue;
		}
		strcpy((char*)net_packet->data, "BARONY_GAME_START");
		SDLNet_Write32(svFlags, &net_packet->data[17]);
		SDLNet_Write32(uniqueGameKey, &net_packet->data[21]);
		if ( loadingsavegame == 0 )
		{
			net_packet->data[25] = 0;
		}
		else
		{
			net_packet->data[25] = 1;
		}
		net_packet->address.host = net_clients[c - 1].host;
		net_packet->address.port = net_clients[c - 1].port;
		net_packet->len = 26;
		sendPacketSafe(net_sock, -1, net_packet, c - 1);
	}
}

// opens the steam dialog to invite friends
void buttonInviteFriends(button_t* my)
{
#ifdef STEAMWORKS
	if (SteamUser()->BLoggedOn() && currentLobby)
	{
		SteamFriends()->ActivateGameOverlayInviteDialog(*static_cast<CSteamID*>(currentLobby));
	}
#else
#ifdef USE_EOS
	EOS.showFriendsOverlay();
#endif
#endif
}

// disconnects from whatever lobby the game is connected to
void buttonDisconnect(button_t* my)
{
	int c;

	if ( multiplayer == SERVER )
	{
		// send disconnect message to clients
		for ( c = 1; c < MAXPLAYERS; c++ )
		{
			if ( client_disconnected[c] )
			{
				continue;
			}
			strcpy((char*)net_packet->data, "PLAYERDISCONNECT");
			net_packet->data[16] = clientnum;
			net_packet->address.host = net_clients[c - 1].host;
			net_packet->address.port = net_clients[c - 1].port;
			net_packet->len = 17;
			sendPacketSafe(net_sock, -1, net_packet, c - 1);
		}
	}
	else
	{
		// send disconnect message to server
		strcpy((char*)net_packet->data, "PLAYERDISCONNECT");
		net_packet->data[16] = clientnum;
		net_packet->address.host = net_server.host;
		net_packet->address.port = net_server.port;
		net_packet->len = 17;
		sendPacketSafe(net_sock, -1, net_packet, 0);
	}

	// reset multiplayer status
	multiplayer = SINGLE;
	stats[0]->sex = stats[clientnum]->sex;
	client_classes[0] = client_classes[clientnum];
	strcpy(stats[0]->name, stats[clientnum]->name);
	clientnum = 0;
	client_disconnected[0] = false;
	for ( c = 1; c < MAXPLAYERS; c++ )
	{
		client_disconnected[c] = true;
	}

	// close any existing net interfaces
	closeNetworkInterfaces();
#ifdef STEAMWORKS
	if ( currentLobby )
	{
		SteamMatchmaking()->LeaveLobby(*static_cast<CSteamID*>(currentLobby));
		cpp_Free_CSteamID(currentLobby); //TODO: Bugger this.
		currentLobby = NULL;
	}
#endif
#if defined USE_EOS
	if ( EOS.CurrentLobbyData.currentLobbyIsValid() )
	{
		EOS.leaveLobby();
	}
#endif

	// close lobby window
	buttonCloseSubwindow(my);
}

// open the video tab in the settings window
void buttonVideoTab(button_t* my)
{
	changeSettingsTab(SETTINGS_VIDEO_TAB);
}

// open the audio tab in the settings window
void buttonAudioTab(button_t* my)
{
	changeSettingsTab(SETTINGS_AUDIO_TAB);
}

// open the keyboard tab in the settings window
void buttonKeyboardTab(button_t* my)
{
	changeSettingsTab(SETTINGS_KEYBOARD_TAB);
}

// open the mouse tab in the settings window
void buttonMouseTab(button_t* my)
{
	changeSettingsTab(SETTINGS_MOUSE_TAB);
}

//Open the gamepad bindings tab in the settings window
void buttonGamepadBindingsTab(button_t* my)
{
	changeSettingsTab(SETTINGS_GAMEPAD_BINDINGS_TAB);
}

//Open the general gamepad settings tab in the settings window
void buttonGamepadSettingsTab(button_t* my)
{
	changeSettingsTab(SETTINGS_GAMEPAD_SETTINGS_TAB);
}

// open the misc tab in the settings window
void buttonMiscTab(button_t* my)
{
	changeSettingsTab(SETTINGS_MISC_TAB);
}

void applySettings()
{
	int c;

	// set video options
	fov = settings_fov;
	smoothlighting = settings_smoothlighting;
	oldFullscreen = fullscreen;
	oldBorderless = borderless;
	fullscreen = settings_fullscreen;
	borderless = settings_borderless;
	shaking = settings_shaking;
	bobbing = settings_bobbing;
	spawn_blood = settings_spawn_blood;
	flickerLights = settings_light_flicker;
	oldVerticalSync = verticalSync;
	verticalSync = settings_vsync;
	showStatusEffectIcons = settings_status_effect_icons;
	colorblind = settings_colorblind;
	oldGamma = vidgamma;
	vidgamma = settings_gamma;
	fpsLimit = settings_fps;
	oldXres = xres;
	oldYres = yres;
	xres = settings_xres;
	yres = settings_yres;

	if ( svFlags != settings_svFlags && multiplayer != CLIENT )
	{
		svFlags = settings_svFlags;
		if ( !intro && multiplayer == SERVER )
		{
			// update client flags
			strcpy((char*)net_packet->data, "SVFL");
			SDLNet_Write32(svFlags, &net_packet->data[4]);
			net_packet->len = 8;

			for ( int c = 1; c < MAXPLAYERS; ++c )
			{
				if ( client_disconnected[c] )
				{
					continue;
				}
				net_packet->address.host = net_clients[c - 1].host;
				net_packet->address.port = net_clients[c - 1].port;
				sendPacketSafe(net_sock, -1, net_packet, c - 1);
				messagePlayer(c, language[276]);
			}
			messagePlayer(clientnum, language[276]);
		}
	}

	minimapTransparencyForeground = settings_minimap_transparency_foreground;
	minimapTransparencyBackground = settings_minimap_transparency_background;
	minimapScale = settings_minimap_scale;
	minimapObjectZoom = settings_minimap_object_zoom;

	cameras[0].winx = 0;
	cameras[0].winy = 0;
	cameras[0].winw = std::min(cameras[0].winw, xres);
	cameras[0].winh = std::min(cameras[0].winh, yres);
	if(xres!=oldXres || yres!=oldYres || oldFullscreen!=fullscreen || oldGamma!=vidgamma
		|| oldVerticalSync != verticalSync || oldBorderless != borderless )
	{
		if ( !changeVideoMode() )
		{
			printlog("critical error! Attempting to abort safely...\n");
			mainloop = 0;
		}
		if ( zbuffer != NULL )
		{
			free(zbuffer);
		}
		zbuffer = (real_t*) malloc(sizeof(real_t) * xres * yres);
		if ( clickmap != NULL )
		{
			free(clickmap);
		}
		clickmap = (Entity**) malloc(sizeof(Entity*)*xres * yres);
	}
	// set audio options
	sfxvolume = settings_sfxvolume;
	sfxAmbientVolume = settings_sfxAmbientVolume;
	sfxEnvironmentVolume = settings_sfxEnvironmentVolume;
	musvolume = settings_musvolume;
	minimapPingMute = settings_minimap_ping_mute;
	mute_audio_on_focus_lost = settings_mute_audio_on_focus_lost;
	mute_player_monster_sounds = settings_mute_player_monster_sounds;

#ifdef USE_FMOD
	FMOD_ChannelGroup_SetVolume(music_group, musvolume / 128.f);
	FMOD_ChannelGroup_SetVolume(sound_group, sfxvolume / 128.f);
	FMOD_ChannelGroup_SetVolume(soundAmbient_group, sfxAmbientVolume / 128.f);
	FMOD_ChannelGroup_SetVolume(soundEnvironment_group, sfxEnvironmentVolume / 128.f);
#elif defined USE_OPENAL
	OPENAL_ChannelGroup_SetVolume(music_group, musvolume / 128.f);
	OPENAL_ChannelGroup_SetVolume(sound_group, sfxvolume / 128.f);
	OPENAL_ChannelGroup_SetVolume(soundAmbient_group, sfxAmbientVolume / 128.f);
	OPENAL_ChannelGroup_SetVolume(soundEnvironment_group, sfxEnvironmentVolume / 128.f);
#endif

	// set keyboard options
	for (c = 0; c < NUMIMPULSES; c++)
	{
		impulses[c] = settings_impulses[c];
	}
	for (c = 0; c < NUM_JOY_IMPULSES; c++)
	{
		joyimpulses[c] = settings_joyimpulses[c];
	}

	// set mouse options
	reversemouse = settings_reversemouse;
	smoothmouse = settings_smoothmouse;
	disablemouserotationlimit = settings_disablemouserotationlimit;
	mousespeed = settings_mousespeed;

	// set misc options
	broadcast = settings_broadcast;
	nohud = settings_nohud;

	auto_hotbar_new_items = settings_auto_hotbar_new_items;
	for ( c = 0; c < NUM_HOTBAR_CATEGORIES; ++c )
	{
		auto_hotbar_categories[c] = settings_auto_hotbar_categories[c];
	}
	for ( c = 0; c < NUM_AUTOSORT_CATEGORIES; ++c )
	{
		autosort_inventory_categories[c] = settings_autosort_inventory_categories[c];
	}
	hotbar_numkey_quick_add = settings_hotbar_numkey_quick_add;
	disable_messages = settings_disable_messages;
	right_click_protect = settings_right_click_protect;
	auto_appraise_new_items = settings_auto_appraise_new_items;
	players[clientnum]->characterSheet.lock_right_sidebar = settings_lock_right_sidebar;
	show_game_timer_always = settings_show_game_timer_always;

	gamepad_leftx_invert = settings_gamepad_leftx_invert;
	gamepad_lefty_invert = settings_gamepad_lefty_invert;
	gamepad_rightx_invert = settings_gamepad_rightx_invert;
	gamepad_righty_invert = settings_gamepad_righty_invert;
	gamepad_menux_invert = settings_gamepad_menux_invert;
	gamepad_menuy_invert = settings_gamepad_menuy_invert;


	gamepad_deadzone = settings_gamepad_deadzone;
	gamepad_rightx_sensitivity = settings_gamepad_rightx_sensitivity;
	gamepad_righty_sensitivity = settings_gamepad_righty_sensitivity;
	gamepad_menux_sensitivity = settings_gamepad_menux_sensitivity;
	gamepad_menuy_sensitivity = settings_gamepad_menuy_sensitivity;

	uiscale_charactersheet = settings_uiscale_charactersheet;
	uiscale_skillspage = settings_uiscale_skillspage;
	uiscale_inventory = settings_uiscale_inventory;
	uiscale_hotbar = settings_uiscale_hotbar;
	uiscale_chatlog = settings_uiscale_chatlog;
	uiscale_playerbars = settings_uiscale_playerbars;
	hide_statusbar = settings_hide_statusbar;
	hide_playertags = settings_hide_playertags;
	show_skill_values = settings_show_skill_values;
	if ( net_handler && disableMultithreadedSteamNetworking != settings_disableMultithreadedSteamNetworking )
	{
		net_handler->toggleMultithreading(settings_disableMultithreadedSteamNetworking);
	}
	disableMultithreadedSteamNetworking = settings_disableMultithreadedSteamNetworking;
	disableFPSLimitOnNetworkMessages = settings_disableFPSLimitOnNetworkMessages;
#ifdef USE_EOS
	if ( LobbyHandler.settings_crossplayEnabled )
	{
		if ( !LobbyHandler.crossplayEnabled )
		{
			LobbyHandler.settings_crossplayEnabled = false;
			EOS.CrossplayAccountManager.trySetupFromSettingsMenu = true;
		}
	}
	else
	{
		if ( LobbyHandler.crossplayEnabled )
		{
			LobbyHandler.crossplayEnabled = false;
			EOS.CrossplayAccountManager.logOut = true;
		}
	}
#endif
	saveConfig("default.cfg");
}

void openConfirmResolutionWindow()
{
	inputs.mouseClearLeft(clientnum);
	keystatus[SDL_SCANCODE_RETURN] = 0;
	inputs.controllerClearInput(clientnum, INJOY_MENU_NEXT);
	playSound(139, 64);

	//Create confirmation window
	subwindow = 1;
	subx1 = xres / 2 - 128;
	subx2 = xres / 2 + 128;
	suby1 = yres / 2 - 40;
	suby2 = yres / 2 + 40;
	strcpy(subtext, "Testing resolution.\nWill revert in 10 seconds.");

	//Accept button
	button_t* button = newButton();
	strcpy(button->label, "Accept");
	button->x = subx1 + 8;
	button->y = suby2 - 28;
	button->sizex = strlen("Accept") * 12 + 8;
	button->sizey = 20;
	button->action = &buttonAcceptResolution;
	button->visible = 1;
	button->focused = 1;
	button->key = SDL_SCANCODE_RETURN;
	button->joykey = joyimpulses[INJOY_MENU_NEXT];

	//Revert button
	button = newButton();
	strcpy(button->label, "Revert");
	button->x = subx2 - strlen("Revert") * 12 - 16;
	button->y = suby2 - 28;
	button->sizex = strlen("Revert") * 12 + 8;
	button->sizey = 20;
	button->action = &buttonRevertResolution;
	button->visible = 1;
	button->focused = 1;
	button->key = SDL_SCANCODE_ESCAPE;
	button->joykey = joyimpulses[INJOY_MENU_CANCEL];
	revertResolutionButton = button;

	resolutionConfirmationTimer = SDL_GetTicks();
	confirmResolutionWindow = true;
}

void buttonAcceptResolution(button_t* my)
{
	confirmResolutionWindow = false;
	buttonCloseSubwindow(my);
	list_FreeAll(&button_l);
	deleteallbuttons = true;
	revertResolutionButton = nullptr;

	applySettings();
}

void buttonRevertResolution(button_t* my)
{
	revertResolution();

	confirmResolutionWindow = false;
	buttonCloseSubwindow(my);
	list_FreeAll(&button_l);
	deleteallbuttons = true;
	revertResolutionButton = nullptr;
}

void revertResolution()
{
	settings_xres = oldXres;
	settings_yres = oldYres;

	applySettings();
}

// settings accept button
void buttonSettingsAccept(button_t* my)
{
	applySettings();

	if ( resolutionChanged )
	{
		buttonCloseSettingsSubwindow(my);
		resolutionChanged = false;
		list_FreeAll(&button_l);
		deleteallbuttons = true;
		openConfirmResolutionWindow();
	}
	else
	{
		// we need to reposition the settings window now.
		buttonCloseSubwindow(my);
		list_FreeAll(&button_l);
		deleteallbuttons = true;
		openSettingsWindow();
	}
}

// settings okay button
void buttonSettingsOK(button_t* my)
{
	buttonSettingsAccept(my);
	if ( !confirmResolutionWindow )
	{
		buttonCloseSubwindow(my);
	}
}

// next score button (statistics window)
void buttonScoreNext(button_t* my)
{
	if ( scoreDisplayMultiplayer )
	{
		score_window = std::min<int>(score_window + 1, std::max<Uint32>(1, list_Size(&topscoresMultiplayer)));
	}
	else
	{
		score_window = std::min<int>(score_window + 1, std::max<Uint32>(1, list_Size(&topscores)));
	}
	loadScore(score_window - 1);
	camera_charsheet_offsetyaw = (330) * PI / 180;
}

// previous score button (statistics window)
void buttonScorePrev(button_t* my)
{
	score_window = std::max(score_window - 1, 1);
	loadScore(score_window - 1);
	camera_charsheet_offsetyaw = (330) * PI / 180;
}

void buttonScoreToggle(button_t* my)
{
	score_window = 1;
	camera_charsheet_offsetyaw = (330) * PI / 180;
	scoreDisplayMultiplayer = !scoreDisplayMultiplayer;
	loadScore(score_window - 1);
}

#ifdef STEAMWORKS

void buttonLeaderboardFetch(button_t* my)
{
	if ( g_SteamLeaderboards )
	{
		g_SteamLeaderboards->DownloadScores(g_SteamLeaderboards->LeaderboardView.requestType,
			g_SteamLeaderboards->LeaderboardView.rangeStart, g_SteamLeaderboards->LeaderboardView.rangeEnd);
	}
}

void buttonLeaderboardNextCategory(button_t* my)
{
	if ( g_SteamLeaderboards )
	{
		int offset = (g_SteamLeaderboards->b_ShowDLCScores ? 16 : 0);
		g_SteamLeaderboards->LeaderboardView.boardToDownload = 
			std::min(g_SteamLeaderboards->LeaderboardView.boardToDownload + 1, (int)LEADERBOARD_MULTIPLAYER_HELL_SCORE + offset);
		g_SteamLeaderboards->b_ScoresDownloaded = false;
		score_leaderboard_window = 1;
		g_SteamLeaderboards->FindLeaderboard(g_SteamLeaderboards->leaderboardNames[g_SteamLeaderboards->LeaderboardView.boardToDownload].c_str());
	}
}

void buttonLeaderboardPrevCategory(button_t* my)
{
	if ( g_SteamLeaderboards )
	{
		int offset = (g_SteamLeaderboards->b_ShowDLCScores ? 16 : 0);
		g_SteamLeaderboards->LeaderboardView.boardToDownload = 
			std::max(g_SteamLeaderboards->LeaderboardView.boardToDownload - 1, (int)LEADERBOARD_NORMAL_TIME + offset);
		g_SteamLeaderboards->b_ScoresDownloaded = false;
		score_leaderboard_window = 1;
		g_SteamLeaderboards->FindLeaderboard(g_SteamLeaderboards->leaderboardNames[g_SteamLeaderboards->LeaderboardView.boardToDownload].c_str());
	}
}

void buttonDLCLeaderboardFetch(button_t* my)
{
	if ( g_SteamLeaderboards )
	{
		if ( g_SteamLeaderboards->b_ShowDLCScores )
		{
			if ( g_SteamLeaderboards->LeaderboardView.boardToDownload > LEADERBOARD_MULTIPLAYER_HELL_SCORE )
			{
				g_SteamLeaderboards->LeaderboardView.boardToDownload -= 16;
			}
		}
		else
		{
			if ( g_SteamLeaderboards->LeaderboardView.boardToDownload <= LEADERBOARD_MULTIPLAYER_HELL_SCORE )
			{
				g_SteamLeaderboards->LeaderboardView.boardToDownload += 16;
			}
		}
		g_SteamLeaderboards->b_ShowDLCScores = !g_SteamLeaderboards->b_ShowDLCScores;
		g_SteamLeaderboards->b_ScoresDownloaded = false;
		score_leaderboard_window = 1;
		g_SteamLeaderboards->FindLeaderboard(g_SteamLeaderboards->leaderboardNames[g_SteamLeaderboards->LeaderboardView.boardToDownload].c_str());
	}

}

void buttonOpenSteamLeaderboards(button_t* my)
{
	if ( g_SteamLeaderboards )
	{
		// close current window
		buttonCloseSubwindow(nullptr);
		list_FreeAll(&button_l);
		deleteallbuttons = true;

		// create confirmation window
		subwindow = 1;
		subx1 = xres / 2 - 390;
		subx2 = xres / 2 + 390;
		suby1 = yres / 2 - 300;
		suby2 = yres / 2 + 300;
		score_leaderboard_window = 1;
		g_SteamLeaderboards->LeaderboardView.boardToDownload = LEADERBOARD_NORMAL_TIME;
		g_SteamLeaderboards->b_ScoresDownloaded = false;
		g_SteamLeaderboards->FindLeaderboard(g_SteamLeaderboards->leaderboardNames[g_SteamLeaderboards->LeaderboardView.boardToDownload].c_str());

		strcpy(subtext, "Steam Leaderboards");

		// close button
		button_t* button = newButton();
		strcpy(button->label, "x");
		button->x = subx2 - 20;
		button->y = suby1 + 4;
		button->sizex = 20;
		button->sizey = 20;
		button->action = &buttonCloseSubwindow;
		button->visible = 1;
		button->focused = 1;
		button->key = SDL_SCANCODE_ESCAPE;
		button->joykey = joyimpulses[INJOY_MENU_CANCEL];

		// next button
		button = newButton();
		strcpy(button->label, ">");
		button->sizex = strlen(">") * 12 + 8;
		button->sizey = 20;
		button->x = subx2 - button->sizex - 4;
		button->y = suby2 - 24;
		button->action = &buttonLeaderboardNextCategory;
		button->visible = 1;
		button->focused = 1;
		button->key = SDL_SCANCODE_RIGHT;
		button->joykey = joyimpulses[INJOY_DPAD_RIGHT];

		// previous button
		button = newButton();
		strcpy(button->label, "<");
		button->sizex = strlen("<") * 12 + 8;
		button->sizey = 20;
		button->x = subx1 + 4;
		button->y = suby2 - 24;
		button->action = &buttonLeaderboardPrevCategory;
		button->visible = 1;
		button->focused = 1;
		button->key = SDL_SCANCODE_LEFT;
		button->joykey = joyimpulses[INJOY_DPAD_LEFT];

		// fetch leaderboards
		button = newButton();
		strcpy(button->label, "Fetch Leaderboard");
		button->y = suby1 + 3 * TTF12_HEIGHT + 8;
		button->sizex = 25 * TTF12_WIDTH + 8;
		button->x = subx2 - button->sizex - 8;
		button->sizey = 32;
		button->action = &buttonLeaderboardFetch;
		button->visible = 1;
		button->focused = 1;

		// fetch DLC leaderboards
		button_t* dlcScoreButton = newButton();
		strcpy(dlcScoreButton->label, "Toggle DLC Scores");
		dlcScoreButton->y = suby1 + 3 * TTF12_HEIGHT + 8;
		dlcScoreButton->sizex = 25 * TTF12_WIDTH + 8;
		dlcScoreButton->x = button->x - dlcScoreButton->sizex - 8;
		dlcScoreButton->sizey = 32;
		dlcScoreButton->action = &buttonDLCLeaderboardFetch;
		dlcScoreButton->visible = 1;
		dlcScoreButton->focused = 1;
	}
}
#endif

void buttonOpenScoresWindow(button_t* my)
{
	// create statistics window
	clientnum = 0;
	subwindow = 1;
	score_window = 1;
	camera_charsheet_offsetyaw = (330) * PI / 180;
	loadScore(0);
	subx1 = xres / 2 - 400;
	subx2 = xres / 2 + 400;
#ifdef PANDORA
	suby1 = yres / 2 - ((yres == 480) ? 200 : 240);
	suby2 = yres / 2 + ((yres == 480) ? 200 : 240);
#else
	suby1 = yres / 2 - 260;
	suby2 = yres / 2 + 260;
#endif
	strcpy(subtext, "");

	// close button
	button_t* button = newButton();
	strcpy(button->label, "x");
	button->x = subx2 - 20;
	button->y = suby1 + 4;
	button->sizex = 20;
	button->sizey = 20;
	button->action = &buttonCloseSubwindow;
	button->visible = 1;
	button->focused = 1;
	button->key = SDL_SCANCODE_ESCAPE;
	button->joykey = joyimpulses[INJOY_MENU_CANCEL];

	// next button
	button = newButton();
	strcpy(button->label, ">");
	button->sizex = strlen(">") * 12 + 8;
	button->sizey = 20;
	button->x = subx2 - button->sizex - 4;
	button->y = suby2 - 24;
	button->action = &buttonScoreNext;
	button->visible = 1;
	button->focused = 1;
	button->key = SDL_SCANCODE_RIGHT;
	button->joykey = joyimpulses[INJOY_DPAD_RIGHT];

	// previous button
	button = newButton();
	strcpy(button->label, "<");
	button->sizex = strlen("<") * 12 + 8;
	button->sizey = 20;
	button->x = subx1 + 4;
	button->y = suby2 - 24;
	button->action = &buttonScorePrev;
	button->visible = 1;
	button->focused = 1;
	button->key = SDL_SCANCODE_LEFT;
	button->joykey = joyimpulses[INJOY_DPAD_LEFT];

	// multiplayer scores toggle button
	button = newButton();
	strcpy(button->label, "");
	button->sizex = strlen("show multiplayer") * 12 + 8;
	button->sizey = 20;
	button->x = subx2 - 44 - strlen("show multiplayer") * 12;
	button->y = suby1 + 4;
	button->action = &buttonScoreToggle;
	button->visible = 1;
	button->focused = 1;

	// delete single score button
	button = newButton();
	strcpy(button->label, "delete score");
	button->sizex = strlen("delete score") * 12 + 8;
	button->sizey = 20;
	button->x = subx2 - 44 - (strlen("delete score") + strlen("show multiplayer") + 1) * 12;
	button->y = suby1 + 4;
	button->action = &buttonDeleteScoreWindow;
	button->visible = 1;
	button->focused = 1;

	// open steam leaderboards button
#ifdef STEAMWORKS
	button = newButton();
	strcpy(button->label, language[3095]);
	button->sizex = strlen(language[3095]) * 12 + 8;
	button->sizey = 20;
	button->x = subx2 - 44 - strlen(language[3095]) * 12;
	button->y = suby2 - 8 - TTF12_HEIGHT;
	button->action = &buttonOpenSteamLeaderboards;
	button->visible = 1;
	button->focused = 1;
#endif // STEAMWORKS
}

void buttonDeleteCurrentScore(button_t* my)
{
	node_t* node = nullptr;
	if ( score_window_delete_multiplayer )
	{
		node = list_Node(&topscoresMultiplayer, score_window_to_delete - 1);
		if ( node )
		{
			list_RemoveNode(node);
			score_window_to_delete = std::max(score_window_to_delete - 1, 1);
		}
	}
	else
	{
		node = list_Node(&topscores, score_window_to_delete - 1);
		if ( node )
		{
			list_RemoveNode(node);
			score_window_to_delete = std::max(score_window_to_delete - 1, 1);
		}
	}
}

// handles slider
void doSlider(int x, int y, int dots, int minvalue, int maxvalue, int increment, int* var, SDL_Surface* slider_font, int slider_font_char_width)
{
	int c;
	Sint32 mousex = inputs.getMouse(clientnum, Inputs::MouseInputs::X);
	Sint32 mousey = inputs.getMouse(clientnum, Inputs::MouseInputs::Y);
	Sint32 omousex = inputs.getMouse(clientnum, Inputs::MouseInputs::OX);
	Sint32 omousey = inputs.getMouse(clientnum, Inputs::MouseInputs::OY);

	// build bar
	strcpy(tempstr, "| ");
	for ( c = 0; c < dots; c++ )
	{
		strcat(tempstr, ". ");
	}
	strcat(tempstr, "| %d");
	printTextFormatted(slider_font, x, y, tempstr, *var);

	// control
	int range = maxvalue - minvalue;
	int sliderLength = ((strlen(tempstr) - 4) * (slider_font->w / slider_font_char_width));
	if ( inputs.bMouseLeft(clientnum) )
	{
		if ( omousex >= x && omousex < x + sliderLength + (slider_font->w / slider_font_char_width) )
		{
			if ( omousey >= y - (slider_font->h / slider_font_char_width) / 2 && omousey < y + ((slider_font->h / slider_font_char_width) / 2) * 3 )
			{
				*var = ((real_t)(mousex - x - (slider_font->w / slider_font_char_width) / 2) / sliderLength) * range + minvalue;
				if ( increment )
				{
					*var += increment / 2;
					*var /= increment;
					*var *= increment;
				}
				*var = std::min(std::max(minvalue, *var), maxvalue);
			}
		}
	}

	// draw slider
	int sliderx = x + (slider_font->w / slider_font_char_width) / 2;
	sliderx += (((real_t)(*var) - minvalue) / range) * sliderLength;
	drawWindowFancy( sliderx - (slider_font->w / slider_font_char_width) / 2, y - (slider_font->h / slider_font_char_width) / 2, sliderx + (slider_font->w / slider_font_char_width) / 2, y + ((slider_font->h / slider_font_char_width) / 2) * 3);
}

// handles slider (float)
void doSliderF(int x, int y, int dots, real_t minvalue, real_t maxvalue, real_t increment, real_t* var)
{
	int c;
	Sint32 mousex = inputs.getMouse(clientnum, Inputs::MouseInputs::X);
	Sint32 mousey = inputs.getMouse(clientnum, Inputs::MouseInputs::Y);
	Sint32 omousex = inputs.getMouse(clientnum, Inputs::MouseInputs::OX);
	Sint32 omousey = inputs.getMouse(clientnum, Inputs::MouseInputs::OY);

	// build bar
	strcpy(tempstr, "| ");
	for ( c = 0; c < dots; c++ )
	{
		strcat(tempstr, ". ");
	}
	strcat(tempstr, "| %.3f");
	printTextFormatted(SLIDERFONT, x, y, tempstr, *var);

	// control
	real_t range = maxvalue - minvalue;
	int sliderLength = ((strlen(tempstr) - 6) * (SLIDERFONT->w / 16));
	if ( inputs.bMouseLeft(clientnum) )
	{
		if ( omousex >= x && omousex < x + sliderLength + (SLIDERFONT->w / 16) )
		{
			if ( omousey >= y - (SLIDERFONT->h / 16) / 2 && omousey < y + ((SLIDERFONT->h / 16) / 2) * 3 )
			{
				*var = ((real_t)(mousex - x - (SLIDERFONT->w / 16) / 2) / sliderLength) * range + minvalue;
				if ( increment )
				{
					*var += increment / 2;
					*var /= increment;
					*var = floor(*var);
					*var *= increment;
				}
				*var = fmin(fmax(minvalue, *var), maxvalue);
			}
		}
	}

	// draw slider
	int sliderx = x + (SLIDERFONT->w / 16) / 2;
	sliderx += (((*var) - minvalue) / range) * sliderLength;
	drawWindowFancy( sliderx - (SLIDERFONT->w / 16) / 2, y - (SLIDERFONT->h / 16) / 2, sliderx + (SLIDERFONT->w / 16) / 2, y + ((SLIDERFONT->h / 16) / 2) * 3);
}

void openLoadGameWindow(button_t* my)
{
	button_t* button;

	// close current window
	buttonCloseSubwindow(NULL);
	list_FreeAll(&button_l);
	deleteallbuttons = true;

	// create confirmation window
	subwindow = 1;
	subx1 = xres / 2 - 256;
	subx2 = xres / 2 + 256;
	suby1 = yres / 2 - 128;
	suby2 = yres / 2 + 128;
	strcpy(subtext, language[1460]);
	bool singleplayerSave = saveGameExists(true);
	bool multiplayerSave = saveGameExists(false);

	char saveGameName[1024] = "";
	if ( singleplayerSave && multiplayerSave )
	{
		strncpy(saveGameName, getSaveGameName(true), 1024);
		strcat(subtext, saveGameName);
		strcat(subtext, "\n\n");
		strncpy(saveGameName, getSaveGameName(false), 1024);
		loadGameSaveShowRectangle = 2;

		suby1 = yres / 2 - 152;
		suby2 = yres / 2 + 152;
	}
	else if ( singleplayerSave )
	{
		strncpy(saveGameName, getSaveGameName(true), 1024);
		loadGameSaveShowRectangle = 1;
	}
	else if ( multiplayerSave )
	{
		strncpy(saveGameName, getSaveGameName(false), 1024);
		loadGameSaveShowRectangle = 1;
	}
	strcat(subtext, saveGameName);
	strcat(subtext, language[1461]);

	if ( gamemods_numCurrentModsLoaded >= 0 )
	{
		suby1 -= 24;
		suby2 += 24;
	}

	// close button
	button = newButton();
	strcpy(button->label, "x");
	button->x = subx2 - 20;
	button->y = suby1;
	button->sizex = 20;
	button->sizey = 20;
	button->action = &buttonCloseSubwindow;
	button->visible = 1;
	button->focused = 1;
	button->key = SDL_SCANCODE_ESCAPE;
	button->joykey = joyimpulses[INJOY_MENU_CANCEL];

	// yes solo button
	button = newButton();
	if ( multiplayerSave && !singleplayerSave )
	{
		strcpy(button->label, language[2959]);
		button->action = &buttonLoadMultiplayerGame;
	}
	else
	{
		strcpy(button->label, language[1462]);
		button->action = &buttonLoadSingleplayerGame;
	}
	button->sizex = strlen(language[2959]) * 9 + 16;
	button->sizey = 20;
	button->x = subx1 + (subx2 - subx1) / 2 - button->sizex / 2;
	button->y = suby2 - 52;
	if ( singleplayerSave && multiplayerSave )
	{
		button->x -= 124;
	}
	else
	{
		button->sizex = strlen(language[1463]) * 12 + 8; // resize to be wider
		button->x = subx1 + (subx2 - subx1) / 2 - button->sizex / 2; // resize to match new width
	}
	button->visible = 1;
	button->focused = 1;
	button->joykey = joyimpulses[INJOY_MENU_NEXT]; //load save game yes => "a" button

	// yes multiplayer button
	if ( singleplayerSave && multiplayerSave )
	{
		button = newButton();
		strcpy(button->label, language[2959]);
		button->sizex = strlen(language[2959]) * 9 + 16;
		button->sizey = 20;
		button->x = subx1 + (subx2 - subx1) / 2 - button->sizex / 2 + 124;
		button->y = suby2 - 52;
		button->action = &buttonLoadMultiplayerGame;
		button->visible = 1;
		button->focused = 1;
		//button->joykey = joyimpulses[INJOY_MENU_NEXT]; //load save game yes => "a" button
	}

	// no button
	button = newButton();
	strcpy(button->label, language[1463]);
	button->sizex = strlen(language[1463]) * 10 + 8;
	button->sizey = 20;
	button->x = subx1 + (subx2 - subx1) / 2 - button->sizex / 2;
	button->y = suby2 - 28;
	button->action = &buttonOpenCharacterCreationWindow;
	button->visible = 1;
	button->focused = 1;
	button->key = SDL_SCANCODE_RETURN;
	button->joykey = joyimpulses[INJOY_MENU_DONT_LOAD_SAVE]; //load save game no => "y" button

	// delete savegame button
	if ( singleplayerSave || multiplayerSave )
	{
		button = newButton();
		strcpy(button->label, language[2961]);
		button->sizex = strlen(language[2961]) * 12 + 8;
		button->sizey = 20;
		button->x = subx2 - button->sizex - 8;
		button->y = suby1 + TTF12_HEIGHT * 2 + 4;
		if ( singleplayerSave && multiplayerSave)
		{
			button->action = &buttonDeleteSavedSoloGame; // showing 2 entries, single player delete
		}
		if ( singleplayerSave && !multiplayerSave ) // showing 1 entry, single player delete
		{
			button->action = &buttonDeleteSavedSoloGame;
		}
		if ( !singleplayerSave && multiplayerSave ) // showing 1 entry, multi player delete
		{
			button->action = &buttonDeleteSavedMultiplayerGame;
		}
		button->visible = 1;
		button->focused = 1;
	}
	if ( singleplayerSave && multiplayerSave )
	{
		button = newButton();
		strcpy(button->label, language[2961]);
		button->sizex = strlen(language[2961]) * 12 + 8;
		button->sizey = 20;
		button->x = subx2 - button->sizex - 8;
		button->y = suby1 + TTF12_HEIGHT * 5 + 6;
		button->action = &buttonDeleteSavedMultiplayerGame;
		button->visible = 1;
		button->focused = 1;
	}
}

void reloadSavegamesList(bool showWindow)
{
	savegamesList.clear();

	// load single player files
	for ( int fileNumber = 0; fileNumber < SAVE_GAMES_MAX; ++fileNumber )
	{
		if ( saveGameExists(true, fileNumber) )
		{
			time_t timeNow = std::time(nullptr);
			struct tm *tm = nullptr;
			char path[PATH_MAX] = "";
			char savefile[PATH_MAX] = "";
			strncpy(savefile, setSaveGameFileName(true, false, fileNumber).c_str(), PATH_MAX - 1);
			completePath(path, savefile, outputdir);
#ifdef WINDOWS
			struct _stat result;
			if ( _stat(path, &result) == 0 )
			{
				tm = localtime(&result.st_mtime);
			}
#else
			struct stat result;
			if ( stat(path, &result) == 0 )
			{
				tm = localtime(&result.st_mtime);
			}
#endif
			if ( tm )
			{
				int timeDifference = std::difftime(timeNow, mktime(tm));
				char* saveGameName = getSaveGameName(true, fileNumber);
				if ( saveGameName )
				{
					savegamesList.push_back(std::make_tuple(timeDifference, getSaveGameType(true, fileNumber), fileNumber, saveGameName));
					free(saveGameName);
				}
			}
		}
	}
	// load multiplayer files
	for ( int fileNumber = 0; fileNumber < SAVE_GAMES_MAX; ++fileNumber )
	{
		if ( saveGameExists(false, fileNumber) )
		{
			time_t timeNow = std::time(nullptr);
			struct tm *tm = nullptr;
			char path[PATH_MAX] = "";
			char savefile[PATH_MAX] = "";
			strncpy(savefile, setSaveGameFileName(false, false, fileNumber).c_str(), PATH_MAX - 1);
			completePath(path, savefile, outputdir);
#ifdef WINDOWS
			struct _stat result;
			if ( _stat(path, &result) == 0 )
			{
				tm = localtime(&result.st_mtime);
			}
#else
			struct stat result;
			if ( stat(path, &result) == 0 )
			{
				tm = localtime(&result.st_mtime);
			}
#endif
			if ( tm )
			{
				int timeDifference = std::difftime(timeNow, mktime(tm));
				char* saveGameName = getSaveGameName(false, fileNumber);
				if ( saveGameName )
				{
					savegamesList.push_back(std::make_tuple(timeDifference, getSaveGameType(false, fileNumber), fileNumber, saveGameName));
					free(saveGameName);
				}
			}
		}
	}
	if ( showWindow )
	{
		savegames_window = 1;
	}
	std::sort(savegamesList.begin(), savegamesList.end());
}

void openNewLoadGameWindow(button_t* my)
{
	// close current window
	buttonCloseSubwindow(nullptr);
	list_FreeAll(&button_l);
	deleteallbuttons = true;

	// create confirmation window
	subwindow = 1;
	subx1 = xres / 2 - 380;
	subx2 = xres / 2 + 380;
	suby1 = yres / 2 - 210;
	suby2 = yres / 2 + 210;
	strcpy(subtext, language[3065]);

	// close button
	button_t* button = newButton();
	strcpy(button->label, "x");
	button->x = subx2 - 20;
	button->y = suby1;
	button->sizex = 20;
	button->sizey = 20;
	button->action = &buttonCloseSubwindow;
	button->visible = 1;
	button->focused = 1;
	button->key = SDL_SCANCODE_ESCAPE;
	button->joykey = joyimpulses[INJOY_MENU_CANCEL];

	button = newButton();
	strcpy(button->label, language[1463]);
	button->sizex = strlen(language[1463]) * 10 + 8;
	button->sizey = 36;
	button->x = subx1 + 16;
	button->y = suby1 + 42;
	button->action = &buttonOpenCharacterCreationWindow;
	button->visible = 1;
	button->focused = 1;
	button->key = SDL_SCANCODE_RETURN;
	button->joykey = joyimpulses[INJOY_MENU_DONT_LOAD_SAVE]; //load save game no => "y" button

	reloadSavegamesList();
}

void buttonDeleteSavedSoloGame(button_t* my)
{
	// close current window
	buttonCloseSubwindow(nullptr);
	list_FreeAll(&button_l);
	deleteallbuttons = true;

	loadGameSaveShowRectangle = 1;

	// create confirmation window
	subwindow = 1;
	subx1 = xres / 2 - 288;
	subx2 = xres / 2 + 288;
	suby1 = yres / 2 - 80;
	suby2 = yres / 2 + 80;
	char saveGameName[1024];
	strcpy(subtext, language[2963]);
	strncpy(saveGameName, getSaveGameName(true), 1024);
	strcat(subtext, saveGameName);
	// close button
	button_t* button = newButton();
	strcpy(button->label, "x");
	button->x = subx2 - 20;
	button->y = suby1;
	button->sizex = 20;
	button->sizey = 20;
	button->action = &buttonCloseSubwindow;
	button->visible = 1;
	button->focused = 1;
	button->key = SDL_SCANCODE_ESCAPE;
	button->joykey = joyimpulses[INJOY_MENU_CANCEL];

	// delete button
	button = newButton();
	strcpy(button->label, language[2961]);
	button->sizex = strlen(language[2961]) * 12 + 8;
	button->sizey = 20;
	button->x = subx1 + (subx2 - subx1) / 2 - button->sizex / 2;
	button->y = suby2 - 56;
	button->action = &buttonConfirmDeleteSoloFile;
	button->visible = 1;
	button->focused = 1;
	button->key = SDL_SCANCODE_RETURN;
	//button->joykey = joyimpulses[INJOY_MENU_DONT_LOAD_SAVE]; //load save game no => "y" button

	// close button
	button = newButton();
	strcpy(button->label, language[2962]);
	button->sizex = strlen(language[2962]) * 12 + 8;
	button->sizey = 20;
	button->x = subx1 + (subx2 - subx1) / 2 - button->sizex / 2;
	button->y = suby2 - 28;
	button->action = &buttonCloseSubwindow;
	button->visible = 1;
	button->focused = 1;
}

void buttonDeleteSavedMultiplayerGame(button_t* my)
{
	// close current window
	buttonCloseSubwindow(nullptr);
	list_FreeAll(&button_l);
	deleteallbuttons = true;

	loadGameSaveShowRectangle = 1;

	// create confirmation window
	subwindow = 1;
	subx1 = xres / 2 - 288;
	subx2 = xres / 2 + 288;
	suby1 = yres / 2 - 80;
	suby2 = yres / 2 + 80;
	char saveGameName[1024];
	strcpy(subtext, language[2964]);
	strncpy(saveGameName, getSaveGameName(false), 1024);
	strcat(subtext, saveGameName);
	// close button
	button_t* button = newButton();
	strcpy(button->label, "x");
	button->x = subx2 - 20;
	button->y = suby1;
	button->sizex = 20;
	button->sizey = 20;
	button->action = &buttonCloseSubwindow;
	button->visible = 1;
	button->focused = 1;
	button->key = SDL_SCANCODE_ESCAPE;
	button->joykey = joyimpulses[INJOY_MENU_CANCEL];

	// delete button
	button = newButton();
	strcpy(button->label, language[2961]);
	button->sizex = strlen(language[2961]) * 12 + 8;
	button->sizey = 20;
	button->x = subx1 + (subx2 - subx1) / 2 - button->sizex / 2;
	button->y = suby2 - 56;
	button->action = &buttonConfirmDeleteMultiplayerFile;
	button->visible = 1;
	button->focused = 1;
	button->key = SDL_SCANCODE_RETURN;
	//button->joykey = joyimpulses[INJOY_MENU_DONT_LOAD_SAVE]; //load save game no => "y" button

	// close button
	button = newButton();
	strcpy(button->label, language[2962]);
	button->sizex = strlen(language[2962]) * 12 + 8;
	button->sizey = 20;
	button->x = subx1 + (subx2 - subx1) / 2 - button->sizex / 2;
	button->y = suby2 - 28;
	button->action = &buttonCloseSubwindow;
	button->visible = 1;
	button->focused = 1;
}

void buttonConfirmDeleteSoloFile(button_t* my)
{
	// close current window
	buttonCloseSubwindow(nullptr);
	list_FreeAll(&button_l);
	deleteallbuttons = true;
	loadGameSaveShowRectangle = 0;
	deleteSaveGame(SINGLE);
	if ( anySaveFileExists() ) // check for saved game to load up
	{
		openNewLoadGameWindow(nullptr);
	}
	playSound(153, 96);
}

void buttonConfirmDeleteMultiplayerFile(button_t* my)
{
	// close current window
	buttonCloseSubwindow(nullptr);
	list_FreeAll(&button_l);
	deleteallbuttons = true;
	loadGameSaveShowRectangle = 0;
	deleteSaveGame(CLIENT);
	if ( anySaveFileExists() ) // check for saved game to load up
	{
		openNewLoadGameWindow(nullptr);
	}
	playSound(153, 96);
}


void buttonDeleteScoreCancel(button_t* my)
{
	// close current window
	buttonCloseSubwindow(nullptr);
	list_FreeAll(&button_l);
	deleteallbuttons = true;

	buttonOpenScoresWindow(nullptr);
	score_window = score_window_to_delete;
	scoreDisplayMultiplayer = score_window_delete_multiplayer;
	score_window_to_delete = 0;
	score_window_delete_multiplayer = false;

	loadScore(score_window - 1);
}

void buttonDeleteScoreConfirm(button_t* my)
{
	buttonDeleteCurrentScore(nullptr);
	buttonDeleteScoreCancel(nullptr);
}

void buttonDeleteScoreWindow(button_t* my)
{
	score_window_to_delete = score_window;
	score_window_delete_multiplayer = scoreDisplayMultiplayer;

	// close current window
	buttonCloseSubwindow(nullptr);
	list_FreeAll(&button_l);
	deleteallbuttons = true;

	// create confirmation window
	subwindow = 1;
	subx1 = xres / 2 - 244;
	subx2 = xres / 2 + 244;
	suby1 = yres / 2 - 60;
	suby2 = yres / 2 + 60;
	strcpy(subtext, language[3002]);

	// close button
	button_t* button = newButton();
	strcpy(button->label, "x");
	button->x = subx2 - 20;
	button->y = suby1;
	button->sizex = 20;
	button->sizey = 20;
	button->action = &buttonDeleteScoreCancel;
	button->visible = 1;
	button->focused = 1;
	button->key = SDL_SCANCODE_ESCAPE;
	button->joykey = joyimpulses[INJOY_MENU_CANCEL];

	// delete button
	button = newButton();
	strcpy(button->label, language[3001]);
	button->sizex = strlen(language[3001]) * 12 + 8;
	button->sizey = 20;
	button->x = subx1 + (subx2 - subx1) / 2 - button->sizex / 2;
	button->y = suby2 - 56;
	button->action = &buttonDeleteScoreConfirm;
	button->visible = 1;
	button->focused = 1;
	button->key = SDL_SCANCODE_RETURN;

	// close button
	button = newButton();
	strcpy(button->label, language[2962]);
	button->sizex = strlen(language[2962]) * 12 + 8;
	button->sizey = 20;
	button->x = subx1 + (subx2 - subx1) / 2 - button->sizex / 2;
	button->y = suby2 - 28;
	button->action = &buttonDeleteScoreCancel;
	button->visible = 1;
	button->focused = 1;
}

void buttonOpenCharacterCreationWindow(button_t* my)
{
	button_t* button;

	playing_random_char = false;
	loadingsavegame = 0;
	loadGameSaveShowRectangle = 0;
	// reset class loadout
	clientnum = 0;
	stats[0]->sex = static_cast<sex_t>(0 + rand() % 2);
	stats[0]->appearance = 0 + rand() % NUMAPPEARANCES;
	stats[0]->playerRace = RACE_HUMAN;
	strcpy(stats[0]->name, "");
	stats[0]->type = HUMAN;
	client_classes[0] = 0;
	stats[0]->clearStats();
	initClass(0);

	// close current window
	if ( subwindow )
	{
		buttonCloseSubwindow(NULL);
		list_FreeAll(&button_l);
		deleteallbuttons = true;
	}

	// create character creation window
	charcreation_step = 1;
	raceSelect = 0;
	camera_charsheet_offsetyaw = (330) * PI / 180;
	subwindow = 1;
	subx1 = xres / 2 - 400;
	subx2 = xres / 2 + 400;
	suby1 = yres / 2 - 260;
	suby2 = yres / 2 + 260;
	strcpy(subtext, "");

	// close button
	button = newButton();
	strcpy(button->label, "x");
	button->x = subx2 - 20;
	button->y = suby1;
	button->sizex = 20;
	button->sizey = 20;
	button->action = &buttonCloseSubwindow;
	button->visible = 1;
	button->focused = 1;
	button->joykey = joyimpulses[INJOY_PAUSE_MENU];

	if ( lastCreatedCharacterClass >= 0
		&& lastCreatedCharacterAppearance >= 0 
		&& lastCreatedCharacterSex >= 0 )
	{
		button_t* replayCharacterBtn = newButton();
		strcpy(replayCharacterBtn->label, language[3000]);
		replayCharacterBtn->sizex = strlen(language[3000]) * 12 + 8;
		replayCharacterBtn->sizey = 20;
		replayCharacterBtn->x = button->x - (replayCharacterBtn->sizex + 4); // take position of button attributes above.
		replayCharacterBtn->y = button->y;
		replayCharacterBtn->action = &buttonReplayLastCharacter;
		replayCharacterBtn->visible = 1;
		replayCharacterBtn->focused = 1;
	}

	// Continue ...
	button = newButton();
	strcpy(button->label, language[1464]);
	button->sizex = strlen(language[1464]) * 12 + 8;
	button->sizey = 20;
	button->x = subx2 - button->sizex - 4;
	button->y = suby2 - 24;
	button->action = &buttonContinue;
	button->visible = 1;
	button->focused = 1;
	button->key = SDL_SCANCODE_RETURN;
	button->joykey = joyimpulses[INJOY_MENU_NEXT];

	// Back ...
	button = newButton();
	strcpy(button->label, language[1465]);
	button->x = subx1 + 4;
	button->y = suby2 - 24;
	button->sizex = strlen(language[1465]) * 12 + 8;
	button->sizey = 20;
	button->action = &buttonBack;
	button->visible = 1;
	button->focused = 1;
	button->key = SDL_SCANCODE_ESCAPE;
	button->joykey = joyimpulses[INJOY_MENU_CANCEL];
	int button_back_x = button->x;
	int button_back_width = button->sizex;

	// Random Character ...
	button = newButton();
	strcpy(button->label, language[1466]);
	button->x = button_back_x + button_back_width + 4;
	button->y = suby2 - 24;
	button->sizex = strlen(language[1466]) * 12 + 8;
	button->sizey = 20;
	button->action = &buttonRandomCharacter;
	button->visible = 1;
	button->focused = 1;
	button->key = SDL_SCANCODE_R; //NOTE: This might cause the character to randomly R when you're typing a name. So far, exactly one user has reported something like this happening exactly once in the entirety of existence.
	button->joykey = joyimpulses[INJOY_MENU_RANDOM_CHAR]; //random character => "y" button

	//Random Name.
	button = newButton();
	strcpy(button->label, language[2498]);
	button->x = button_back_x + button_back_width + 4;
	button->y = suby2 - 24;
	button->sizex = strlen(language[2498]) * 12 + 8;
	button->sizey = 20;
	button->action = &buttonRandomName;
	button->visible = 1;
	button->focused = 1;
	button->joykey = joyimpulses[INJOY_MENU_RANDOM_NAME];
}

void buttonLoadSingleplayerGame(button_t* button)
{
	loadGameSaveShowRectangle = 0;
	savegamesList.clear();
	loadingsavegame = getSaveGameUniqueGameKey(true);
	int mul = getSaveGameType(true);

	if ( mul == DIRECTSERVER )
	{
		directConnect = true;
		buttonHostMultiplayer(button);
	}
	else if ( mul == DIRECTCLIENT )
	{
		directConnect = true;
		buttonJoinMultiplayer(button);
	}
	else if ( mul == SINGLE )
	{
		buttonStartSingleplayer(button);
	}
	else
	{
		directConnect = false;
#if defined(USE_EOS) || defined(STEAMWORKS)
		if ( mul == SERVERCROSSPLAY )
		{
			// if steamworks, the hosting type can change if crossplay is enabled or not.
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
			buttonHostMultiplayer(button);
		}
		else if ( mul == SERVER )
		{
			if ( getSaveGameVersionNum(true) <= 335 )
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
			buttonHostMultiplayer(button);
		}
		else if ( mul == CLIENT )
		{
#ifdef STEAMWORKS
			if ( !lobbyToConnectTo )
			{
				openSteamLobbyWaitWindow(button);
			}
			else
			{
				// close current window
				list_FreeAll(&button_l);
				deleteallbuttons = true;

				// create new window
				subwindow = 1;
				subx1 = xres / 2 - 256;
				subx2 = xres / 2 + 256;
				suby1 = yres / 2 - 64;
				suby2 = yres / 2 + 64;
				strcpy(subtext, language[1447]);

				// close button
				button = newButton();
				strcpy(button->label, "x");
				button->x = subx2 - 20;
				button->y = suby1;
				button->sizex = 20;
				button->sizey = 20;
				button->action = &openSteamLobbyWaitWindow;
				button->visible = 1;
				button->focused = 1;
				button->key = SDL_SCANCODE_ESCAPE;
				button->joykey = joyimpulses[INJOY_MENU_CANCEL];

				// cancel button
				button = newButton();
				strcpy(button->label, language[1316]);
				button->sizex = strlen(language[1316]) * 12 + 8;
				button->sizey = 20;
				button->x = subx1 + (subx2 - subx1) / 2 - button->sizex / 2;
				button->y = suby2 - 28;
				button->action = &openSteamLobbyWaitWindow;
				button->visible = 1;
				button->focused = 1;

				connectingToLobby = true;
				connectingToLobbyWindow = true;
				strncpy( currentLobbyName, "", 31 );
				LobbyHandler.steamValidateAndJoinLobby(*static_cast<CSteamID*>(lobbyToConnectTo));
				cpp_Free_CSteamID(lobbyToConnectTo);
				lobbyToConnectTo = NULL;
			}
#elif defined USE_EOS
			openSteamLobbyWaitWindow(button);
#endif
		}
		else
		{
			buttonStartSingleplayer(button);
		}
#endif
	}
}

void buttonLoadMultiplayerGame(button_t* button)
{
	loadGameSaveShowRectangle = 0;
	savegamesList.clear();
	loadingsavegame = getSaveGameUniqueGameKey(false);
	int mul = getSaveGameType(false);

	if ( mul == DIRECTSERVER )
	{
		directConnect = true;
		buttonHostMultiplayer(button);
	}
	else if ( mul == DIRECTCLIENT )
	{
		directConnect = true;
		buttonJoinMultiplayer(button);
	}
	else if ( mul == SINGLE )
	{
		buttonStartSingleplayer(button);
	}
	else
	{
		directConnect = false;
#if defined(USE_EOS) || defined(STEAMWORKS)
		if ( mul == SERVERCROSSPLAY )
		{
			// if steamworks, the hosting type can change if crossplay is enabled or not.
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
			buttonHostMultiplayer(button);
		}
		else if ( mul == SERVER )
		{
			if ( getSaveGameVersionNum(false) <= 335 )
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
			buttonHostMultiplayer(button);
		}
		else if ( mul == CLIENT )
		{
#ifdef STEAMWORKS
			if ( !lobbyToConnectTo )
			{
				openSteamLobbyWaitWindow(button);
			}
			else
			{
				list_FreeAll(&button_l);
				deleteallbuttons = true;

				// create new window
				subwindow = 1;
				subx1 = xres / 2 - 256;
				subx2 = xres / 2 + 256;
				suby1 = yres / 2 - 64;
				suby2 = yres / 2 + 64;
				strcpy(subtext, language[1447]);

				// close button
				button = newButton();
				strcpy(button->label, "x");
				button->x = subx2 - 20;
				button->y = suby1;
				button->sizex = 20;
				button->sizey = 20;
				button->action = &openSteamLobbyWaitWindow;
				button->visible = 1;
				button->focused = 1;
				button->key = SDL_SCANCODE_ESCAPE;
				button->joykey = joyimpulses[INJOY_MENU_CANCEL];

				// cancel button
				button = newButton();
				strcpy(button->label, language[1316]);
				button->sizex = strlen(language[1316]) * 12 + 8;
				button->sizey = 20;
				button->x = subx1 + (subx2 - subx1) / 2 - button->sizex / 2;
				button->y = suby2 - 28;
				button->action = &openSteamLobbyWaitWindow;
				button->visible = 1;
				button->focused = 1;

				connectingToLobby = true;
				connectingToLobbyWindow = true;
				strncpy(currentLobbyName, "", 31);
				LobbyHandler.steamValidateAndJoinLobby(*static_cast<CSteamID*>(lobbyToConnectTo));
				cpp_Free_CSteamID(lobbyToConnectTo);
				lobbyToConnectTo = NULL;
			}
#elif defined USE_EOS
			openSteamLobbyWaitWindow(button);
#endif
		}
		else
		{
			buttonStartSingleplayer(button);
		}
#endif
	}
}

void buttonRandomCharacter(button_t* my)
{
	playing_random_char = true;
	charcreation_step = 4;
	camera_charsheet_offsetyaw = (330) * PI / 180;
	stats[0]->sex = static_cast<sex_t>(rand() % 2);
	client_classes[0] = rand() % (CLASS_MONK + 1);//NUMCLASSES;
	stats[0]->clearStats();
	if ( enabledDLCPack1 || enabledDLCPack2 )
	{
		stats[0]->playerRace = rand() % NUMPLAYABLERACES;
		if ( !enabledDLCPack1 )
		{
			while ( stats[0]->playerRace == RACE_SKELETON || stats[0]->playerRace == RACE_VAMPIRE
				|| stats[0]->playerRace == RACE_SUCCUBUS || stats[0]->playerRace == RACE_GOATMAN )
			{
				stats[0]->playerRace = rand() % NUMPLAYABLERACES;
			}
		}
		else if ( !enabledDLCPack2 )
		{
			while ( stats[0]->playerRace == RACE_AUTOMATON || stats[0]->playerRace == RACE_GOBLIN
				|| stats[0]->playerRace == RACE_INCUBUS || stats[0]->playerRace == RACE_INSECTOID )
			{
				stats[0]->playerRace = rand() % NUMPLAYABLERACES;
			}
		}
		if ( stats[0]->playerRace == RACE_INCUBUS )
		{
			stats[0]->sex = MALE;
		}
		else if ( stats[0]->playerRace == RACE_SUCCUBUS )
		{
			stats[0]->sex = FEMALE;
		}

		if ( stats[0]->playerRace == RACE_HUMAN )
		{
			client_classes[0] = rand() % (NUMCLASSES);
			if ( !enabledDLCPack1 )
			{
				while ( client_classes[0] == CLASS_CONJURER || client_classes[0] == CLASS_ACCURSED
					|| client_classes[0] == CLASS_MESMER || client_classes[0] == CLASS_BREWER )
				{
					client_classes[0] = rand() % (NUMCLASSES);
				}
			}
			else if ( !enabledDLCPack2 )
			{
				while ( client_classes[0] == CLASS_HUNTER || client_classes[0] == CLASS_SHAMAN
					|| client_classes[0] == CLASS_PUNISHER || client_classes[0] == CLASS_MACHINIST )
				{
					client_classes[0] = rand() % (NUMCLASSES);
				}
			}
			stats[0]->appearance = rand() % NUMAPPEARANCES;
		}
		else
		{
			client_classes[0] = rand() % (CLASS_MONK + 2);
			if ( client_classes[0] > CLASS_MONK )
			{
				client_classes[0] = CLASS_MONK + stats[0]->playerRace; // monster specific classes.
			}
			stats[0]->appearance = 0;
		}
	}
	else
	{
		stats[0]->playerRace = RACE_HUMAN;
		stats[0]->appearance = rand() % NUMAPPEARANCES;
	}
	initClass(0);
}

void buttonReplayLastCharacter(button_t* my)
{
	if ( lastCreatedCharacterClass >= 0 )
	{
		playing_random_char = false;
		camera_charsheet_offsetyaw = (330) * PI / 180;
		stats[0]->sex = static_cast<sex_t>(lastCreatedCharacterSex);
		stats[0]->playerRace = std::min(std::max(static_cast<int>(RACE_HUMAN), lastCreatedCharacterRace), static_cast<int>(RACE_INSECTOID));
		client_classes[0] = std::min(std::max(0, lastCreatedCharacterClass), static_cast<int>(CLASS_HUNTER));

		switch ( isCharacterValidFromDLC(*stats[0], lastCreatedCharacterClass) )
		{
			case VALID_OK_CHARACTER:
				// do nothing.
				break;
			case INVALID_REQUIREDLC1:
			case INVALID_REQUIREDLC2:
				// class or race invalid.
				if ( stats[0]->playerRace > RACE_HUMAN )
				{
					stats[0]->playerRace = RACE_HUMAN;
				}
				if ( client_classes[0] > CLASS_MONK )
				{
					client_classes[0] = CLASS_BARBARIAN;
				}
				break;
			case INVALID_CHARACTER:
				// invalid for whatever reason, reset.
				stats[0]->playerRace = RACE_HUMAN;
				client_classes[0] = CLASS_BARBARIAN;
				break;
			case INVALID_REQUIRE_ACHIEVEMENT:
				// required achievement for class mixing among races, so race is valid.
				client_classes[0] = CLASS_BARBARIAN;
				break;
			default:
				// invalid for whatever reason, reset.
				stats[0]->playerRace = RACE_HUMAN;
				client_classes[0] = CLASS_BARBARIAN;
				break;
		}
		
		stats[0]->clearStats();
		initClass(0);
		stats[0]->appearance = lastCreatedCharacterAppearance;
		strcpy(stats[0]->name, lastname.c_str());
		charcreation_step = 4; // set the step to 4, so clicking continue advances to 5 (single/multiplayer select)
		buttonContinue(nullptr);
	}
}

void buttonRandomName(button_t* my)
{
	if ( !SDL_IsTextInputActive() || charcreation_step != 4 )
	{
		return;
	}

	std::vector<std::string> *names;

	if ( stats[0]->sex == MALE )
	{
		names = &randomPlayerNamesMale;
	}
	else
	{
		names = &randomPlayerNamesFemale;
	}

	if ( !names->size() )
	{
		printlog("Warning: Random Name: Need names to pick from!");
		return;
	}
	std::string name;
#ifndef NINTENDO
	try
	{
		name = randomEntryFromVector(*names);
	}
	catch ( const char* e )
	{
		printlog("Error: Random Name: \"%s\"", e);
		return;
	}
	catch ( ... )
	{
		printlog("Error: Failed to choose random name.");
		return;
	}
#else
	name = randomEntryFromVector(*names);
#endif

	strncpy(inputstr, name.c_str(), std::min<size_t>(name.length(), inputlen));
	inputstr[std::min<size_t>(name.length(), inputlen)] = '\0';
}

void buttonGamemodsOpenDirectory(button_t* my)
{
	if ( gamemods_window_fileSelect != 0 && !currentDirectoryFiles.empty() )
	{
		std::list<std::string>::const_iterator it = currentDirectoryFiles.begin();
		std::advance(it, std::max(gamemods_window_scroll + gamemods_window_fileSelect - 1, 0));
		std::string directoryName = *it;

		if ( directoryName.compare("..") == 0 || directoryName.compare(".") == 0 )
		{
			if ( !strcmp(outputdir, "./") )
			{
				directoryPath.append(directoryName);
				directoryPath.append(PHYSFS_getDirSeparator());
			}
			else
			{
				directoryPath = outputdir;
				directoryPath.append(PHYSFS_getDirSeparator()).append(directoryName).append(PHYSFS_getDirSeparator());
			}
		}
		else
		{
			if ( !strcmp(outputdir, "./") )
			{
				directoryPath.append(directoryName);
				directoryPath.append(PHYSFS_getDirSeparator());
			}
			else
			{
				directoryPath = outputdir;
				directoryPath.append(PHYSFS_getDirSeparator()).append(directoryName).append(PHYSFS_getDirSeparator());
			}
		}
		gamemods_window_fileSelect = 0;
		gamemods_window_scroll = 0;
		currentDirectoryFiles = directoryContents(directoryPath.c_str(), true, false);
	}
}

void buttonGamemodsPrevDirectory(button_t* my)
{
	gamemods_window_fileSelect = 0;
	gamemods_window_scroll = 0;
	directoryPath.append("..");
	directoryPath.append(PHYSFS_getDirSeparator());
	currentDirectoryFiles = directoryContents(directoryPath.c_str(), true, false);
}


void writeLevelsTxt(std::string modFolder)
{
	std::string path = BASE_DATA_DIR;
	path.append("mods/").append(modFolder);
	if ( access(path.c_str(), F_OK) == 0 )
	{
		std::string writeFile = modFolder + "/maps/levels.txt";
		PHYSFS_File *physfp = PHYSFS_openWrite(writeFile.c_str());
		if ( physfp != NULL )
		{
			PHYSFS_writeBytes(physfp, "map: start\n", 11);
			PHYSFS_writeBytes(physfp, "gen: mine\n", 10);
			PHYSFS_writeBytes(physfp, "gen: mine\n", 10);
			PHYSFS_writeBytes(physfp, "gen: mine\n", 10);
			PHYSFS_writeBytes(physfp, "gen: mine\n", 10);
			PHYSFS_writeBytes(physfp, "map: minetoswamp\n", 17);
			PHYSFS_writeBytes(physfp, "gen: swamp\n", 11);
			PHYSFS_writeBytes(physfp, "gen: swamp\n", 11);
			PHYSFS_writeBytes(physfp, "gen: swamp\n", 11);
			PHYSFS_writeBytes(physfp, "gen: swamp\n", 11);
			PHYSFS_writeBytes(physfp, "map: swamptolabyrinth\n", 22);
			PHYSFS_writeBytes(physfp, "gen: labyrinth\n", 15);
			PHYSFS_writeBytes(physfp, "gen: labyrinth\n", 15);
			PHYSFS_writeBytes(physfp, "gen: labyrinth\n", 15);
			PHYSFS_writeBytes(physfp, "gen: labyrinth\n", 15);
			PHYSFS_writeBytes(physfp, "map: labyrinthtoruins\n", 22);
			PHYSFS_writeBytes(physfp, "gen: ruins\n", 11);
			PHYSFS_writeBytes(physfp, "gen: ruins\n", 11);
			PHYSFS_writeBytes(physfp, "gen: ruins\n", 11);
			PHYSFS_writeBytes(physfp, "gen: ruins\n", 11);
			PHYSFS_writeBytes(physfp, "map: boss\n", 10);
			PHYSFS_writeBytes(physfp, "gen: hell\n", 10);
			PHYSFS_writeBytes(physfp, "gen: hell\n", 10);
			PHYSFS_writeBytes(physfp, "gen: hell\n", 10);
			PHYSFS_writeBytes(physfp, "map: hellboss\n", 14);
			PHYSFS_writeBytes(physfp, "map: hamlet\n", 12);
			PHYSFS_writeBytes(physfp, "gen: caves\n", 11);
			PHYSFS_writeBytes(physfp, "gen: caves\n", 11);
			PHYSFS_writeBytes(physfp, "gen: caves\n", 11);
			PHYSFS_writeBytes(physfp, "gen: caves\n", 11);
			PHYSFS_writeBytes(physfp, "map: cavestocitadel\n", 20);
			PHYSFS_writeBytes(physfp, "gen: citadel\n", 13);
			PHYSFS_writeBytes(physfp, "gen: citadel\n", 13);
			PHYSFS_writeBytes(physfp, "gen: citadel\n", 13);
			PHYSFS_writeBytes(physfp, "gen: citadel\n", 13);
			PHYSFS_writeBytes(physfp, "map: sanctum", 12);
			PHYSFS_close(physfp);
		}
		else
		{
			printlog("[PhysFS]: Failed to open %s/maps/levels.txt for writing.", path.c_str());
		}
	}
	else
	{
		printlog("[PhysFS]: Failed to write levels.txt in %s", path.c_str());
	}
}

void buttonGamemodsCreateModDirectory(button_t* my)
{
	std::string baseDir = outputdir;
	baseDir.append(PHYSFS_getDirSeparator()).append("mods").append(PHYSFS_getDirSeparator()).append(gamemods_newBlankDirectory);

	if ( access(baseDir.c_str(), F_OK) == 0 )
	{
		// folder already exists!
		gamemods_newBlankDirectoryStatus = -1;
	}
	else
	{
		if ( PHYSFS_mkdir(gamemods_newBlankDirectory) )
		{
			gamemods_newBlankDirectoryStatus = 1;
			std::string dir = gamemods_newBlankDirectory;
			std::string folder = "/books";
			PHYSFS_mkdir((dir + folder).c_str());
			folder = "/editor";
			PHYSFS_mkdir((dir + folder).c_str());

			folder = "/images";
			PHYSFS_mkdir((dir + folder).c_str());
			std::string subfolder = "/sprites";
			PHYSFS_mkdir((dir + folder + subfolder).c_str());
			subfolder = "/system";
			PHYSFS_mkdir((dir + folder + subfolder).c_str());
			subfolder = "/tiles";
			PHYSFS_mkdir((dir + folder + subfolder).c_str());

			folder = "/items";
			PHYSFS_mkdir((dir + folder).c_str());
			subfolder = "/images";
			PHYSFS_mkdir((dir + folder + subfolder).c_str());

			folder = "/lang";
			PHYSFS_mkdir((dir + folder).c_str());
			folder = "/maps";
			PHYSFS_mkdir((dir + folder).c_str());
			writeLevelsTxt(gamemods_newBlankDirectory);

			folder = "/models";
			PHYSFS_mkdir((dir + folder).c_str());
			subfolder = "/creatures";
			PHYSFS_mkdir((dir + folder + subfolder).c_str());
			subfolder = "/decorations";
			PHYSFS_mkdir((dir + folder + subfolder).c_str());
			subfolder = "/doors";
			PHYSFS_mkdir((dir + folder + subfolder).c_str());
			subfolder = "/items";
			PHYSFS_mkdir((dir + folder + subfolder).c_str());
			subfolder = "/particles";
			PHYSFS_mkdir((dir + folder + subfolder).c_str());

			folder = "/music";
			PHYSFS_mkdir((dir + folder).c_str());
			folder = "/sound";
			PHYSFS_mkdir((dir + folder).c_str());
		}
	}
	strcpy(gamemods_newBlankDirectoryOldName, gamemods_newBlankDirectory);
}

void buttonGamemodsCreateNewModTemplate(button_t* my)
{
	buttonCloseSubwindow(nullptr);
	list_FreeAll(&button_l);
	deleteallbuttons = true;

	gamemods_window = 6;

	// create window
	subwindow = 1;
	subx1 = xres / 2 - 400;
	subx2 = xres / 2 + 400;
	suby1 = yres / 2 - 70;
	suby2 = yres / 2 + 70;
	strcpy(subtext, "Create new blank mod template");

	// close button
	button_t* button = newButton();
	strcpy(button->label, "x");
	button->x = subx2 - 20;
	button->y = suby1;
	button->sizex = 20;
	button->sizey = 20;
	button->action = &buttonCloseSubwindow;
	button->visible = 1;
	button->focused = 1;
	button->key = SDL_SCANCODE_ESCAPE;
	button->joykey = joyimpulses[INJOY_MENU_CANCEL];

	// create button
	button = newButton();
	strcpy(button->label, "create");
	button->x = subx2 - (strlen(button->label) * TTF12_WIDTH + 8);
	button->y = suby2 - TTF12_HEIGHT - 8;
	button->sizex = strlen(button->label) * TTF12_WIDTH + 8;
	button->sizey = 20;
	button->action = &buttonGamemodsCreateModDirectory;
	button->visible = 1;
	button->focused = 1;
}


void buttonGamemodsBaseDirectory(button_t* my)
{
	gamemods_window_fileSelect = 0;
	gamemods_window_scroll = 0;
	directoryPath = outputdir;
	directoryToUpload = directoryPath;
	currentDirectoryFiles = directoryContents(directoryPath.c_str(), true, false);
}

#ifdef STEAMWORKS
void buttonGamemodsSelectDirectoryForUpload(button_t* my)
{
	if ( !currentDirectoryFiles.empty() )
	{
		std::list<std::string>::const_iterator it = currentDirectoryFiles.begin();
		std::advance(it, std::max(gamemods_window_scroll + gamemods_window_fileSelect - 1, 0));
		std::string directoryName = *it;

		if ( directoryName.compare("..") == 0 || directoryName.compare(".") == 0 )
		{
			directoryToUpload = directoryName;
			directoryToUpload.append(PHYSFS_getDirSeparator());
		}
		else
		{
			directoryToUpload = directoryPath;
			directoryToUpload.append(directoryName);
			directoryToUpload.append(PHYSFS_getDirSeparator());
		}
	}
	if ( gamemods_window != 5 )
	{
		if ( g_SteamWorkshop )
		{
			g_SteamWorkshop->createItemResult = {};
		}
		gamemods_uploadStatus = 0;
		gamemods_window = 2;
	}
	directoryFilesListToUpload = directoryContents(directoryToUpload.c_str(), true, true);
}

void buttonGamemodsPrepareWorkshopItemUpload(button_t* my)
{
	if ( SteamUser()->BLoggedOn() && g_SteamWorkshop )
	{
		g_SteamWorkshop->CreateItem();
		gamemods_uploadStatus = 1;
	}
}

void buttonGamemodsCancelModifyFileContents(button_t* my)
{
	directoryFilesListToUpload.clear();
}

void buttonGamemodsPrepareWorkshopItemUpdate(button_t* my)
{
	if ( SteamUser()->BLoggedOn() && g_SteamWorkshop )
	{
		g_SteamWorkshop->CreateItem();
		gamemods_uploadStatus = 1;
	}
}

void buttonGamemodsSetWorkshopItemFields(button_t* my)
{
	if ( SteamUser()->BLoggedOn() && g_SteamWorkshop )
	{
		bool itemTagSetSuccess = false;
		if ( g_SteamWorkshop->UGCUpdateHandle != 0 )
		{
			if ( !strcmp(gamemods_uploadTitle, "") )
			{
				strcpy(gamemods_uploadTitle, "Title");
			}
			gamemods_workshopSetPropertyReturn[0] = SteamUGC()->SetItemTitle(g_SteamWorkshop->UGCUpdateHandle, gamemods_uploadTitle);
			if ( !strcmp(gamemods_uploadDescription, "") )
			{
				strcpy(gamemods_uploadDescription, "Description");
			}
			gamemods_workshopSetPropertyReturn[1] = SteamUGC()->SetItemDescription(g_SteamWorkshop->UGCUpdateHandle, gamemods_uploadDescription);
#ifdef WINDOWS
			char pathbuffer[PATH_MAX];
			GetFullPathName(directoryToUpload.c_str(), PATH_MAX, pathbuffer, NULL);
			std::string fullpath = pathbuffer;
#else
			char pathbuffer[PATH_MAX];
			realpath(directoryToUpload.c_str(), pathbuffer);
			std::string fullpath = pathbuffer;
#endif
			if ( access(fullpath.c_str(), F_OK) == 0 )
			{
				gamemods_workshopSetPropertyReturn[2] = SteamUGC()->SetItemContent(g_SteamWorkshop->UGCUpdateHandle, fullpath.c_str());
				// set preview image.
				bool imagePreviewFound = false;
				std::string imgPath = fullpath;
				imgPath.append("preview.jpg");
				if ( !imagePreviewFound && access((imgPath).c_str(), F_OK) == 0 )
				{
					imagePreviewFound = SteamUGC()->SetItemPreview(g_SteamWorkshop->UGCUpdateHandle, imgPath.c_str());
				}
				imgPath = fullpath;
				imgPath.append("preview.png");
				if ( !imagePreviewFound && access((imgPath).c_str(), F_OK) == 0 )
				{
					imagePreviewFound = SteamUGC()->SetItemPreview(g_SteamWorkshop->UGCUpdateHandle, imgPath.c_str());
				}
				imgPath = fullpath;
				imgPath.append("preview.jpg");
				if ( !imagePreviewFound && access((imgPath).c_str(), F_OK) == 0 )
				{
					imagePreviewFound = SteamUGC()->SetItemPreview(g_SteamWorkshop->UGCUpdateHandle, imgPath.c_str());
				}
				if ( !imagePreviewFound )
				{
					printlog("Failed to upload image for workshop item!");
				}
			}

			// some mumbo jumbo to work with the steam API needing const char[][]
			SteamParamStringArray_t SteamParamStringArray;
			SteamParamStringArray.m_nNumStrings = g_SteamWorkshop->workshopItemTags.size() + 1;

			// construct new char[][]
			char **tagArray = new char*[gamemods_maxTags];
			int i = 0;
			for ( i = 0; i < gamemods_maxTags; ++i )
			{
				tagArray[i] = new char[32];
			}

			// copy all the items into this new char[][].
			std::string line;
			i = 0;
			for ( std::list<std::string>::iterator it = g_SteamWorkshop->workshopItemTags.begin(); it != g_SteamWorkshop->workshopItemTags.end(); ++it )
			{
				line = *it;
				strcpy(tagArray[i], line.c_str());
				++i;
			}
			strcpy(tagArray[i], VERSION); // copy the version number as a tag.

			// set the tags in the API call.
			SteamParamStringArray.m_ppStrings = const_cast<const char**>(tagArray);
			itemTagSetSuccess = SteamUGC()->SetItemTags(g_SteamWorkshop->UGCUpdateHandle, &SteamParamStringArray);

			// delete the allocated char[][]
			for ( i = 0; i < gamemods_maxTags; ++i )
			{
				delete[] tagArray[i];
			}
			delete[] tagArray;
		}
		gamemods_uploadStatus = 4;
		if ( itemTagSetSuccess && gamemods_workshopSetPropertyReturn[0] && gamemods_workshopSetPropertyReturn[1] && gamemods_workshopSetPropertyReturn[2] )
		{
			my->visible = false;
			// set item fields button
			button_t* button = newButton();
			strcpy(button->label, "upload!");
			button->x = subx1 + 16;
			button->y = suby1 + TTF12_HEIGHT * 34;
			button->sizex = 16 * TTF12_WIDTH + 8;
			button->sizey = 32;
			button->action = &buttonGamemodsStartUploadItem;
			button->visible = 1;
			button->focused = 1;
			gamemods_currentEditField = 0;
		}
	}
}

void buttonGamemodsModifyExistingWorkshopItemFields(button_t* my)
{
	if ( SteamUser()->BLoggedOn() && g_SteamWorkshop && g_SteamWorkshop->m_myWorkshopItemToModify.m_nPublishedFileId != 0 )
	{
		g_SteamWorkshop->StartItemExistingUpdate(g_SteamWorkshop->m_myWorkshopItemToModify.m_nPublishedFileId);
		if ( g_SteamWorkshop->UGCUpdateHandle != 0 )
		{
			bool itemTagSetSuccess = false;
			bool itemContentSetSuccess = false;
			if ( !directoryFilesListToUpload.empty() )
			{
#ifdef WINDOWS
				char pathbuffer[PATH_MAX];
				GetFullPathName(directoryToUpload.c_str(), PATH_MAX, pathbuffer, NULL);
				std::string fullpath = pathbuffer;
#else
				char pathbuffer[PATH_MAX];
				realpath(directoryToUpload.c_str(), pathbuffer);
				std::string fullpath = pathbuffer;
#endif
				if ( access(fullpath.c_str(), F_OK) == 0 )
				{
					itemContentSetSuccess = SteamUGC()->SetItemContent(g_SteamWorkshop->UGCUpdateHandle, fullpath.c_str());
					// set preview image.
					bool imagePreviewFound = false;
					std::string imgPath = fullpath;
					imgPath.append("preview.jpg");
					if ( !imagePreviewFound && access((imgPath).c_str(), F_OK) == 0 )
					{
						imagePreviewFound = SteamUGC()->SetItemPreview(g_SteamWorkshop->UGCUpdateHandle, imgPath.c_str());
					}
					imgPath = fullpath;
					imgPath.append("preview.png");
					if ( !imagePreviewFound && access((imgPath).c_str(), F_OK) == 0 )
					{
						imagePreviewFound = SteamUGC()->SetItemPreview(g_SteamWorkshop->UGCUpdateHandle, imgPath.c_str());
					}
					imgPath = fullpath;
					imgPath.append("preview.jpg");
					if ( !imagePreviewFound && access((imgPath).c_str(), F_OK) == 0 )
					{
						imagePreviewFound = SteamUGC()->SetItemPreview(g_SteamWorkshop->UGCUpdateHandle, imgPath.c_str());
					}
					if ( !imagePreviewFound )
					{
						printlog("Failed to upload image for workshop item!");
					}
				}
			}

			// some mumbo jumbo to work with the steam API needing const char[][]
			SteamParamStringArray_t SteamParamStringArray;
			SteamParamStringArray.m_nNumStrings = g_SteamWorkshop->workshopItemTags.size() + 1;

			// construct new char[][]
			char **tagArray = new char*[gamemods_maxTags];
			int i = 0;
			for ( i = 0; i < gamemods_maxTags; ++i )
			{
				tagArray[i] = new char[32];
			}

			// copy all the items into this new char[][].
			std::string line;
			i = 0;
			for ( std::list<std::string>::iterator it = g_SteamWorkshop->workshopItemTags.begin(); it != g_SteamWorkshop->workshopItemTags.end(); ++it )
			{
				line = *it;
				strcpy(tagArray[i], line.c_str());
				++i;
			}
			strcpy(tagArray[i], VERSION); // copy the version number as a tag.

			// set the tags in the API call.
			SteamParamStringArray.m_ppStrings = const_cast<const char**>(tagArray);
			itemTagSetSuccess = SteamUGC()->SetItemTags(g_SteamWorkshop->UGCUpdateHandle, &SteamParamStringArray);

			// delete the allocated char[][]
			for ( i = 0; i < gamemods_maxTags; ++i )
			{
				delete[] tagArray[i];
			}
			delete[] tagArray;

			if ( itemTagSetSuccess && (directoryFilesListToUpload.empty() || (!directoryFilesListToUpload.empty() && itemContentSetSuccess)) )
			{
				my->visible = false;
				// set item fields button
				button_t* button = newButton();
				strcpy(button->label, "upload!");
				button->x = subx1 + 16;
				button->y = suby1 + TTF12_HEIGHT * 34;
				button->sizex = 16 * TTF12_WIDTH + 8;
				button->sizey = 32;
				button->action = &buttonGamemodsStartUploadItem;
				button->visible = 1;
				button->focused = 1;
				gamemods_currentEditField = 0;
			}
		}
	}
}

void buttonGamemodsStartUploadItem(button_t* my)
{
	if ( SteamUser()->BLoggedOn() && g_SteamWorkshop && g_SteamWorkshop->UGCUpdateHandle != 0 )
	{
		if ( gamemods_window == 5 )
		{
			g_SteamWorkshop->SubmitItemUpdate("Item updated.");
		}
		else
		{
			g_SteamWorkshop->SubmitItemUpdate("First upload.");
		}
		gamemods_uploadStatus = 5;
		my->visible = false;
	}
}

void gamemodsWindowUploadInit(bool creatingNewItem)
{
	gamemods_window = 1;
	currentDirectoryFiles = directoryContents(outputdir, true, false);
	directoryToUpload = outputdir;

	// create window
	subwindow = 1;
	subx1 = xres / 2 - 320;
	subx2 = xres / 2 + 320;
	suby1 = yres / 2 - 300;
	suby2 = yres / 2 + 300;
	strcpy(subtext, "Upload to workshop");

	// close button
	button_t* button = newButton();
	strcpy(button->label, "x");
	button->x = subx2 - 20;
	button->y = suby1;
	button->sizex = 20;
	button->sizey = 20;
	button->action = &buttonCloseSubwindow;
	button->visible = 1;
	button->focused = 1;
	button->key = SDL_SCANCODE_ESCAPE;
	button->joykey = joyimpulses[INJOY_MENU_CANCEL];

	// subscribed items window button
	button = newButton();
	strcpy(button->label, "view workshop items");
	button->x = subx2 - 40 - strlen(button->label) * TTF12_WIDTH;
	button->y = suby1;
	button->sizex = strlen(button->label) * TTF12_WIDTH + 16;
	button->sizey = 20;
	button->action = &buttonGamemodsOpenSubscribedWindow;
	button->visible = 1;
	button->focused = 1;

	// previous directory button
	button = newButton();
	strcpy(button->label, "home directory");
	button->x = subx1 + 250;
	button->y = suby1 + 32;
	button->sizex = strlen("home directory") * 12 + 8;
	button->sizey = 20;
	button->action = &buttonGamemodsBaseDirectory;
	button->visible = 1;
	button->focused = 1;


	// open directory button
	button = newButton();
	strcpy(button->label, "open");
	button->x = subx1 + 250;
	button->y = suby1 + 56;
	button->sizex = strlen("home directory") * 12 + 8;
	button->sizey = 20;
	button->action = &buttonGamemodsOpenDirectory;
	button->visible = 1;
	button->focused = 1;

	// previous directory button
	button = newButton();
	strcpy(button->label, "previous folder");
	button->x = subx1 + 250;
	button->y = suby1 + 80;
	button->sizex = strlen("home directory") * 12 + 8;
	button->sizey = 20;
	button->action = &buttonGamemodsPrevDirectory;
	button->visible = 1;
	button->focused = 1;

	// previous directory button
	button = newButton();
	strcpy(button->label, "new mod folder");
	button->x = subx1 + 250;
	button->y = suby1 + 128;
	button->sizex = strlen("new mod folder") * 12 + 8;
	button->sizey = 20;
	button->action = &buttonGamemodsCreateNewModTemplate;
	button->visible = 1;
	button->focused = 1;

	// select directory button
	button = newButton();
	strcpy(button->label, "select folder to upload");
	button->x = subx1 + 16;
	button->y = suby1 + 14 * TTF12_HEIGHT + 8;
	button->sizex = 24 * TTF12_WIDTH + 8;
	button->sizey = 32;
	button->action = &buttonGamemodsSelectDirectoryForUpload;
	button->visible = 1;
	button->focused = 1;

	// prepare directory button
	button_t* button2 = newButton();
	if ( creatingNewItem )
	{
		strcpy(button2->label, "prepare");
		button2->action = &buttonGamemodsPrepareWorkshopItemUpload;
	}
	else
	{
		strcpy(button2->label, "deselect folder");
		button2->action = &buttonGamemodsCancelModifyFileContents;
	}
	button2->x = button->x + button->sizex + 4;
	button2->y = button->y;
	button2->sizex = 16 * TTF12_WIDTH + 8;
	button2->sizey = 32;
	button2->visible = 1;
	button2->focused = 1;

	if ( !creatingNewItem )
	{
		// modify item fields button
		button = newButton();
		strcpy(button->label, "modify tags/content");
		button->x = subx1 + 16;
		button->y = suby1 + TTF12_HEIGHT * 34;
		button->sizex = 22 * TTF12_WIDTH + 8;
		button->sizey = 32;
		button->action = &buttonGamemodsModifyExistingWorkshopItemFields;
		button->visible = 1;
		button->focused = 1;
	}
}

void gamemodsSubscribedItemsInit()
{
	gamemods_window = 3;
	currentDirectoryFiles = directoryContents(outputdir, true, false);
	directoryToUpload = outputdir;

	gamemodsMountAllExistingPaths();

	// create confirmation window
	subwindow = 1;
	subx1 = xres / 2 - 420;
	subx2 = xres / 2 + 420;
	suby1 = yres / 2 - 300;
	suby2 = yres / 2 + 300;
	strcpy(subtext, "Workshop items");

	// close button
	button_t* button = newButton();
	strcpy(button->label, "x");
	button->x = subx2 - 20;
	button->y = suby1;
	button->sizex = 20;
	button->sizey = 20;
	button->action = &buttonCloseSubwindow;
	button->visible = 1;
	button->focused = 1;
	button->key = SDL_SCANCODE_ESCAPE;
	button->joykey = joyimpulses[INJOY_MENU_CANCEL];

	// upload window button
	button = newButton();
	strcpy(button->label, "upload workshop content");
	button->x = subx2 - 40 - strlen(button->label) * TTF12_WIDTH;
	button->y = suby1;
	button->sizex = strlen(button->label) * TTF12_WIDTH + 16;
	button->sizey = 20;
	button->action = &buttonGamemodsOpenUploadWindow;
	button->visible = 1;
	button->focused = 1;

	// fetch subscribed items button
	button = newButton();
	strcpy(button->label, "get subscribed item list");
	button->x = subx1 + 16;
	button->y = suby1 + 2 * TTF12_HEIGHT + 8;
	button->sizex = 25 * TTF12_WIDTH + 8;
	button->sizey = 32;
	button->action = &buttonGamemodsGetSubscribedItems;
	button->visible = 1;
	button->focused = 1;

	// fetch my workshop items
	button_t* button2 = newButton();
	strcpy(button2->label, "my workshop items");
	button2->x = button->x + button->sizex + 16;
	button2->y = suby1 + 2 * TTF12_HEIGHT + 8;
	button2->sizex = 25 * TTF12_WIDTH + 8;
	button2->sizey = 32;
	button2->action = &buttonGamemodsGetMyWorkshopItems;
	button2->visible = 1;
	button2->focused = 1;

	// start modded game
	button = newButton();
	strcpy(button->label, "start modded game");
	button->sizex = 25 * TTF12_WIDTH + 8;
	button->sizey = 32;
	button->x = subx2 - (button->sizex + 16);
	button->y = suby1 + 2 * TTF12_HEIGHT + 8;
	button->action = &buttonGamemodsStartModdedGame;
	button->visible = 1;
	button->focused = 1;
}

void buttonGamemodsOpenUploadWindow(button_t* my)
{
	buttonCloseSubwindow(nullptr);
	list_FreeAll(&button_l);
	deleteallbuttons = true;
	gamemodsWindowUploadInit(true);
}

void buttonGamemodsOpenModifyExistingWindow(button_t* my)
{
	buttonCloseSubwindow(nullptr);
	list_FreeAll(&button_l);
	deleteallbuttons = true;
	gamemodsWindowUploadInit(false);
}

void buttonGamemodsOpenSubscribedWindow(button_t* my)
{
	buttonCloseSubwindow(nullptr);
	list_FreeAll(&button_l);
	deleteallbuttons = true;
	gamemodsSubscribedItemsInit();
}

void buttonGamemodsGetSubscribedItems(button_t* my)
{
	if ( g_SteamWorkshop )
	{
		g_SteamWorkshop->CreateQuerySubscribedItems(k_EUserUGCList_Subscribed, k_EUGCMatchingUGCType_All, k_EUserUGCListSortOrder_LastUpdatedDesc);
		gamemods_window_scroll = 0;
		gamemods_window = 3;
	}
}

void buttonGamemodsGetMyWorkshopItems(button_t* my)
{
	if ( g_SteamWorkshop )
	{
		g_SteamWorkshop->CreateQuerySubscribedItems(k_EUserUGCList_Published, k_EUGCMatchingUGCType_All, k_EUserUGCListSortOrder_LastUpdatedDesc);
		gamemods_window_scroll = 0;
		gamemods_window = 4;
	}
}

void gamemodsDrawWorkshopItemTagToggle(std::string tagname, int x, int y)
{
	if ( !g_SteamWorkshop )
	{
		return;
	}
	std::string printText = tagname;
	std::string line;
	bool foundTag = false;
	std::list<std::string>::iterator it;
	if ( !g_SteamWorkshop->workshopItemTags.empty() )
	{
		for ( it = g_SteamWorkshop->workshopItemTags.begin(); it != g_SteamWorkshop->workshopItemTags.end(); ++it )
		{
			line = *it;
			std::size_t found = line.find_first_of(' '); // trim any trailing spaces.
			if ( found != std::string::npos )
			{
				line = line.substr(0, found);
			}
			if ( line.compare(tagname) == 0 )
			{
				foundTag = true;
				break;
			}
		}
	}
	while ( printText.length() < 12 )
	{
		printText.append(" ");
	}
	if ( foundTag )
	{
		printText.append(": [x]");
	}
	else
	{
		printText.append(": [ ]");
	}
	if ( mouseInBounds(clientnum, x, x + printText.size() * TTF12_WIDTH, y, y + TTF12_HEIGHT) )
	{
		ttfPrintTextColor(ttf12, x, y, SDL_MapRGBA(mainsurface->format, 128, 128, 128, 255), true, printText.c_str());
		if ( inputs.bMouseLeft(clientnum) )
		{
			playSound(139, 64);
			if ( foundTag )
			{
				g_SteamWorkshop->workshopItemTags.erase(it);
			}
			else
			{
				g_SteamWorkshop->workshopItemTags.push_back(tagname);
			}
			inputs.mouseClearLeft(clientnum);
		}
	}
	else
	{
		ttfPrintText(ttf12, x, y, printText.c_str());
	}
}

bool gamemodsCheckIfSubscribedAndDownloadedFileID(uint64 fileID)
{
	if ( directConnect || !currentLobby )
	{
		return false;
	}

	uint64 itemState = SteamUGC()->GetItemState(fileID);
	if ( (itemState & k_EItemStateSubscribed) && (itemState & k_EItemStateInstalled) )
	{
		return true; // client has downloaded and subscribed to content.
	}

	return false; // client does not have item subscribed or downloaded.
}

bool gamemodsCheckFileIDInLoadedPaths(uint64 fileID)
{
	if ( directConnect || !currentLobby )
	{
		return false;
	}

	bool found = false;
	for ( std::vector<std::pair<std::string, uint64>>::iterator it = gamemods_workshopLoadedFileIDMap.begin();
		it != gamemods_workshopLoadedFileIDMap.end(); ++it )
	{
		if ( it->second == fileID )
		{
			return true; // client has fileID in mod load path.
		}
	}

	return false; // client does not have fileID in mod load path.
}

void buttonGamemodsSubscribeToHostsModFiles(button_t* my)
{
	if ( !directConnect && currentLobby && g_SteamWorkshop )
	{
		const char* serverNumModsChar = SteamMatchmaking()->GetLobbyData(*static_cast<CSteamID*>(currentLobby), "svNumMods");
		int serverNumModsLoaded = atoi(serverNumModsChar);
		if ( serverNumModsLoaded > 0 )
		{
			char tagName[32];
			std::vector<uint64> fileIdsToDownload;
			for ( int lines = 0; lines < serverNumModsLoaded; ++lines )
			{
				snprintf(tagName, 32, "svMod%d", lines);
				const char* serverModFileID = SteamMatchmaking()->GetLobbyData(*static_cast<CSteamID*>(currentLobby), tagName);
				if ( strcmp(serverModFileID, "") )
				{
					if ( gamemodsCheckIfSubscribedAndDownloadedFileID(atoi(serverModFileID)) == false )
					{
						SteamUGC()->SubscribeItem(atoi(serverModFileID));
					}
					fileIdsToDownload.push_back(atoi(serverModFileID));
				}
			}
			for ( std::vector<uint64>::iterator it = fileIdsToDownload.begin(); it != fileIdsToDownload.end(); ++it )
			{
				SteamUGC()->DownloadItem(*it, true); // download all the newly subscribed items.
				// hopefully enough time elapses for this to complete
			}
			g_SteamWorkshop->CreateQuerySubscribedItems(k_EUserUGCList_Subscribed, k_EUGCMatchingUGCType_All, k_EUserUGCListSortOrder_LastUpdatedDesc);
		}
	}
}

void buttonGamemodsMountHostsModFiles(button_t* my)
{
	if ( !directConnect && currentLobby && g_SteamWorkshop )
	{
		const char* serverNumModsChar = SteamMatchmaking()->GetLobbyData(*static_cast<CSteamID*>(currentLobby), "svNumMods");
		int serverNumModsLoaded = atoi(serverNumModsChar);
		if ( serverNumModsLoaded > 0 )
		{
			char tagName[32];
			char fullpath[PATH_MAX];
			// prepare to mount only the hosts workshop files.
			gamemodsClearAllMountedPaths();
			gamemods_mountedFilepaths.clear();
			gamemods_workshopLoadedFileIDMap.clear();
			for ( int lines = 0; lines < serverNumModsLoaded; ++lines )
			{
				snprintf(tagName, 32, "svMod%d", lines);
				const char* serverModFileID = SteamMatchmaking()->GetLobbyData(*static_cast<CSteamID*>(currentLobby), tagName);
				if ( strcmp(serverModFileID, "") )
				{
					if ( gamemodsCheckFileIDInLoadedPaths(atoi(serverModFileID)) == false )
					{
						if ( SteamUGC()->GetItemInstallInfo(atoi(serverModFileID), NULL, fullpath, PATH_MAX, NULL) )
						{
							for ( int i = 0; i < g_SteamWorkshop->numSubcribedItemResults; ++i )
							{
								if ( g_SteamWorkshop->m_subscribedItemListDetails[i].m_nPublishedFileId == atoi(serverModFileID) )
								{
									gamemods_mountedFilepaths.push_back(std::make_pair(fullpath, g_SteamWorkshop->m_subscribedItemListDetails[i].m_rgchTitle));
									gamemods_workshopLoadedFileIDMap.push_back(std::make_pair(g_SteamWorkshop->m_subscribedItemListDetails[i].m_rgchTitle, 
										g_SteamWorkshop->m_subscribedItemListDetails[i].m_nPublishedFileId));
									break;
								}
							}
						}
					}
				}
			}
			g_SteamWorkshop->CreateQuerySubscribedItems(k_EUserUGCList_Subscribed, k_EUGCMatchingUGCType_All, k_EUserUGCListSortOrder_LastUpdatedDesc);
			gamemodsMountAllExistingPaths(); // mount all the new filepaths, update gamemods_numCurrentModsLoaded.
		}
	}
}

bool gamemodsIsClientLoadOrderMatchingHost(std::vector<std::string> serverModList)
{
	std::vector<std::pair<std::string, uint64>>::iterator found = gamemods_workshopLoadedFileIDMap.begin();
	std::vector<std::pair<std::string, uint64>>::iterator previousFound = gamemods_workshopLoadedFileIDMap.begin();
	std::vector<std::string>::iterator itServerList;
	if ( serverModList.empty() || (serverModList.size() > gamemods_mountedFilepaths.size()) )
	{
		return false;
	}

	for ( itServerList = serverModList.begin(); itServerList != serverModList.end(); ++itServerList )
	{
		for ( found = previousFound; found != gamemods_workshopLoadedFileIDMap.end(); ++found )
		{
			if ( std::to_string(found->second) == *itServerList )
			{
				break;
			}
		}
		if ( found != gamemods_workshopLoadedFileIDMap.end() )
		{
			// look for the server's modID in my loaded paths.
			// check the distance along the vector our found result is.
			// if the distance is negative, then our mod order is out of sync with the server's mod list
			// and requires rearranging.
			if ( std::distance(previousFound, found) < 0 )
			{
				return false;
			}
			previousFound = found;
		}
		else
		{
			// server's mod doesn't exist in our filepath, so our mod lists are not in sync.
			return false;
		}
	}
	return true;
}

#endif //STEAMWORKS

bool gamemodsDrawClickableButton(int padx, int pady, int padw, int padh, Uint32 btnColor, std::string btnText, int action)
{
	bool clicked = false;
	if ( mouseInBounds(clientnum, padx, padx + padw, pady - 4, pady + padh) )
	{
		drawDepressed(padx, pady - 4, padx + padw, pady + padh);
		if ( inputs.bMouseLeft(clientnum) )
		{
			playSound(139, 64);
			inputs.mouseClearLeft(clientnum);
			clicked = true;
		}
	}
	else
	{
		drawWindow(padx, pady - 4, padx + padw, pady + padh);
	}
	SDL_Rect pos;
	pos.x = padx;
	pos.y = pady - 4;
	pos.w = padw;
	pos.h = padh + 4;
	drawRect(&pos, btnColor, 64);
	ttfPrintTextFormatted(ttf12, padx + 8, pady, "%s", btnText.c_str());
	return clicked;
}

bool gamemodsRemovePathFromMountedFiles(std::string findStr)
{
	std::vector<std::pair<std::string, std::string>>::iterator it;
	std::pair<std::string, std::string> line;
	for ( it = gamemods_mountedFilepaths.begin(); it != gamemods_mountedFilepaths.end(); ++it )
	{
		line = *it;
		if ( line.first.compare(findStr) == 0 )
		{
			// found entry, remove from list.
#ifdef STEAMWORKS
			for ( std::vector<std::pair<std::string, uint64>>::iterator itId = gamemods_workshopLoadedFileIDMap.begin();
				itId != gamemods_workshopLoadedFileIDMap.end(); ++itId )
			{
				if ( itId->first.compare(line.second) == 0 )
				{
					gamemods_workshopLoadedFileIDMap.erase(itId);
					break;
				}
			}
#endif // STEAMWORKS
			gamemods_mountedFilepaths.erase(it);
			return true;
		}
	}
	return false;
}

bool gamemodsIsPathInMountedFiles(std::string findStr)
{
	std::vector<std::pair<std::string, std::string>>::iterator it;
	std::pair<std::string, std::string> line;
	for ( it = gamemods_mountedFilepaths.begin(); it != gamemods_mountedFilepaths.end(); ++it )
	{
		line = *it;
		if ( line.first.compare(findStr) == 0 )
		{
			// found entry
			return true;
		}
	}
	return false;
}

void buttonGamemodsGetLocalMods(button_t* my)
{
	gamemods_window_scroll = 0;
	gamemods_window = 7;
	gamemods_localModFoldernames.clear();
	std::string path = outputdir;
	path.append(PHYSFS_getDirSeparator()).append("mods").append(PHYSFS_getDirSeparator());
	gamemods_localModFoldernames = directoryContents(path.c_str(), true, false);
}

void buttonGamemodsStartModdedGame(button_t* my)
{
	if ( gamemods_modPreload )
	{
		// look for a save game
		if ( anySaveFileExists() )
		{
			openNewLoadGameWindow(nullptr);
		}
		else
		{
			buttonOpenCharacterCreationWindow(NULL);
		}
		return;
	}

	gamemods_numCurrentModsLoaded = gamemods_mountedFilepaths.size();
	if ( gamemods_numCurrentModsLoaded > 0 )
	{
		steamAchievement("BARONY_ACH_LOCAL_CUSTOMS");
	}

	if ( physfsIsMapLevelListModded() )
	{
		gamemods_disableSteamAchievements = true;
	}
	else
	{
		gamemods_disableSteamAchievements = false;
	}

	int w, h;

	if ( !gamemods_modelsListRequiresReload && gamemods_modelsListLastStartedUnmodded )
	{
		if ( physfsSearchModelsToUpdate() || !gamemods_modelsListModifiedIndexes.empty() )
		{
			gamemods_modelsListRequiresReload = true;
		}
		gamemods_modelsListLastStartedUnmodded = false;
	}
	if ( !gamemods_soundListRequiresReload && gamemods_soundsListLastStartedUnmodded )
	{
		if ( physfsSearchSoundsToUpdate() )
		{
			gamemods_soundListRequiresReload = true;
		}
		gamemods_soundsListLastStartedUnmodded = false;
	}

	// process any new model files encountered in the mod load list.
	int modelsIndexUpdateStart = 1;
	int modelsIndexUpdateEnd = nummodels;
	if ( gamemods_modelsListRequiresReload )
	{
		if ( physfsSearchModelsToUpdate() || !gamemods_modelsListModifiedIndexes.empty() )
		{
			// print a loading message
			drawClearBuffers();
			getSizeOfText(ttf16, language[2989], &w, &h);
			ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, language[2989]);
			GO_SwapBuffers(screen);
			physfsModelIndexUpdate(modelsIndexUpdateStart, modelsIndexUpdateEnd, true);
			generatePolyModels(modelsIndexUpdateStart, modelsIndexUpdateEnd, false);
		}
		gamemods_modelsListRequiresReload = false;
	}
	if ( gamemods_soundListRequiresReload )
	{
		if ( physfsSearchSoundsToUpdate() )
		{
			// print a loading message
			drawClearBuffers();
			getSizeOfText(ttf16, language[2987], &w, &h);
			ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, language[2987]);
			GO_SwapBuffers(screen);
			physfsReloadSounds(true);
		}
		gamemods_soundListRequiresReload = false;
	}

	if ( physfsSearchTilesToUpdate() )
	{
		// print a loading message
		drawClearBuffers();
		getSizeOfText(ttf16, language[3017], &w, &h);
		ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, language[3017]);
		GO_SwapBuffers(screen);
		physfsReloadTiles(false);
		gamemods_tileListRequireReloadUnmodded = true;
	}

	if ( physfsSearchSpritesToUpdate() )
	{
		// print a loading message
		drawClearBuffers();
		getSizeOfText(ttf16, language[3015], &w, &h);
		ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, language[3015]);
		GO_SwapBuffers(screen);
		physfsReloadSprites(false);
		gamemods_spriteImagesRequireReloadUnmodded = true;
	}

	if ( physfsSearchBooksToUpdate() )
	{
		// print a loading message
		drawClearBuffers();
		getSizeOfText(ttf16, language[2991], &w, &h);
		ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, language[2991]);
		GO_SwapBuffers(screen);
		physfsReloadBooks();
		gamemods_booksRequireReloadUnmodded = true;
	}

	gamemodsUnloadCustomThemeMusic();

	if ( physfsSearchMusicToUpdate() )
	{
		// print a loading message
		drawClearBuffers();
		getSizeOfText(ttf16, language[2993], &w, &h);
		ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, language[2993]);
		GO_SwapBuffers(screen);
		bool reloadIntroMusic = false;
		physfsReloadMusic(reloadIntroMusic, false);
		if ( reloadIntroMusic )
		{
#ifdef SOUND
			playmusic(intromusic[rand() % (NUMINTROMUSIC - 1)], false, true, true);
#endif			
		}
		gamemods_musicRequireReloadUnmodded = true;
	}

	std::string langDirectory = PHYSFS_getRealDir("lang/en.txt");
	if ( langDirectory.compare("./") != 0 )
	{
		// print a loading message
		drawClearBuffers();
		getSizeOfText(ttf16, language[3004], &w, &h);
		ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, language[3004]);
		GO_SwapBuffers(screen);
		if ( reloadLanguage() != 0 )
		{
			printlog("[PhysFS]: Error reloading modified language file in lang/ directory!");
		}
		else
		{
			printlog("[PhysFS]: Found modified language file in lang/ directory, reloading en.txt...");
		}
		gamemods_langRequireReloadUnmodded = true;
	}

	if ( physfsSearchItemsTxtToUpdate() )
	{
		// print a loading message
		drawClearBuffers();
		getSizeOfText(ttf16, language[3008], &w, &h);
		ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, language[3008]);
		GO_SwapBuffers(screen);
		physfsReloadItemsTxt();
		gamemods_itemsTxtRequireReloadUnmodded = true;
	}

	if ( physfsSearchItemSpritesToUpdate() )
	{
		// print a loading message
		drawClearBuffers();
		getSizeOfText(ttf16, language[3006], &w, &h);
		ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, language[3006]);
		GO_SwapBuffers(screen);
		physfsReloadItemSprites(false);
		gamemods_itemSpritesRequireReloadUnmodded = true;
	}

	if ( physfsSearchItemsGlobalTxtToUpdate() )
	{
		gamemods_itemsGlobalTxtRequireReloadUnmodded = true;
		loadItemLists();
	}

	if ( physfsSearchMonsterLimbFilesToUpdate() )
	{
		// print a loading message
		drawClearBuffers();
		getSizeOfText(ttf16, language[3013], &w, &h);
		ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, language[3013]);
		GO_SwapBuffers(screen);
		physfsReloadMonsterLimbFiles();
		gamemods_monsterLimbsRequireReloadUnmodded = true;
	}

	if ( physfsSearchSystemImagesToUpdate() )
	{
		// print a loading message
		drawClearBuffers();
		getSizeOfText(ttf16, language[3015], &w, &h);
		ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, language[3015]);
		GO_SwapBuffers(screen);
		physfsReloadSystemImages();
		gamemods_systemImagesReloadUnmodded = true;

		// tidy up some other resource files.
		rightsidebar_titlebar_img = spell_list_titlebar_bmp;
		rightsidebar_slot_img = spell_list_gui_slot_bmp;
		rightsidebar_slot_highlighted_img = spell_list_gui_slot_highlighted_bmp;
	}

	// look for a save game
	if ( anySaveFileExists() )
	{
		//openLoadGameWindow(NULL);
		openNewLoadGameWindow(nullptr);
	}
	else
	{
		buttonOpenCharacterCreationWindow(NULL);
	}
}

void gamemodsCustomContentInit()
{

	gamemods_window = 3;
	currentDirectoryFiles = directoryContents(outputdir, true, false);
	directoryToUpload = outputdir;

	gamemodsMountAllExistingPaths();

	// create confirmation window
	subwindow = 1;
	subx1 = xres / 2 - 420;
	subx2 = xres / 2 + 420;
	suby1 = yres / 2 - 300;
	suby2 = yres / 2 + 300;
	strcpy(subtext, "Custom Content");

	// close button
	button_t* button = newButton();
	strcpy(button->label, "x");
	button->x = subx2 - 20;
	button->y = suby1;
	button->sizex = 20;
	button->sizey = 20;
	button->action = &buttonCloseSubwindow;
	button->visible = 1;
	button->focused = 1;
	button->key = SDL_SCANCODE_ESCAPE;
	button->joykey = joyimpulses[INJOY_MENU_CANCEL];

	// fetch local mods button
	button = newButton();
	strcpy(button->label, "local mods");
	button->x = subx1 + 16;
	button->y = suby1 + 2 * TTF12_HEIGHT + 8;
	button->sizex = 25 * TTF12_WIDTH + 8;
	button->sizey = 32;
	button->action = &buttonGamemodsGetLocalMods;
	button->visible = 1;
	button->focused = 1;

	// fetch my workshop items
	button_t* button2 = newButton();
	strcpy(button2->label, "new mod folder");
	button2->x = button->x + button->sizex + 16;
	button2->y = suby1 + 2 * TTF12_HEIGHT + 8;
	button2->sizex = 25 * TTF12_WIDTH + 8;
	button2->sizey = 32;
	button2->action = &buttonGamemodsCreateNewModTemplate;
	button2->visible = 1;
	button2->focused = 1;

	// start modded game
	button = newButton();
	strcpy(button->label, "start modded game");
	button->sizex = 25 * TTF12_WIDTH + 8;
	button->sizey = 32;
	button->x = subx2 - (button->sizex + 16);
	button->y = suby1 + 2 * TTF12_HEIGHT + 8;
	button->action = &buttonGamemodsStartModdedGame;
	button->visible = 1;
	button->focused = 1;
}

bool gamemodsClearAllMountedPaths()
{
	bool success = true;
	char **i;
	for ( i = PHYSFS_getSearchPath(); *i != NULL; i++ )
	{
		std::string line = *i;
		if ( line.compare(outputdir) != 0 && line.compare(datadir) != 0 && line.compare("./") != 0 ) // don't unmount the base ./ directory
		{
			if ( PHYSFS_unmount(*i) == 0 )
			{
				success = false;
				printlog("[%s] unsuccessfully removed from the search path.\n", line.c_str());
			}
			else
			{
				printlog("[%s] is removed from the search path.\n", line.c_str());
			}
		}
	}
	gamemods_numCurrentModsLoaded = -1;
	PHYSFS_freeList(*i);
	return success;
}

bool gamemodsMountAllExistingPaths()
{
	bool success = true;
	std::vector<std::pair<std::string, std::string>>::iterator it;
	for ( it = gamemods_mountedFilepaths.begin(); it != gamemods_mountedFilepaths.end(); ++it )
	{
		std::pair<std::string, std::string> itpair = *it;
		if ( PHYSFS_mount(itpair.first.c_str(), NULL, 0) )
		{
			printlog("[%s] is in the search path.\n", itpair.first.c_str());
		}
		else
		{
			printlog("[%s] unsuccessfully added to search path.\n", itpair.first.c_str());
			success = false;
		}
	}
	gamemods_numCurrentModsLoaded = gamemods_mountedFilepaths.size();
	gamemods_customContentLoadedFirstTime = true;
	return success;
}

void gamemodsWindowClearVariables()
{
#ifdef STEAMWORKS
	if ( g_SteamWorkshop )
	{
		g_SteamWorkshop->createItemResult = {};
		g_SteamWorkshop->UGCUpdateHandle = {};
		g_SteamWorkshop->SubmitItemUpdateResult = {};
		SteamUGC()->ReleaseQueryUGCRequest(g_SteamWorkshop->UGCQueryHandle);
		g_SteamWorkshop->UGCQueryHandle = {};
		g_SteamWorkshop->SteamUGCQueryCompleted = {};
		g_SteamWorkshop->UnsubscribePublishedFileResult = {};
		g_SteamWorkshop->LastActionResult.creationTick = 0;
		g_SteamWorkshop->LastActionResult.actionMsg = "";
		g_SteamWorkshop->LastActionResult.lastResult = static_cast<EResult>(0);
		g_SteamWorkshop->workshopItemTags.clear();
		for ( int i = 0; i < 50; ++i )
		{
			g_SteamWorkshop->m_subscribedItemListDetails[i] = {};
		}
		g_SteamWorkshop->uploadSuccessTicks = 0;
		g_SteamWorkshop->m_myWorkshopItemToModify = {};
	}
#endif // STEAMWORKS
	directoryToUpload.clear();
	directoryPath.clear();
	directoryFilesListToUpload.clear();
	gamemods_window_scroll = 0;
	gamemods_uploadStatus = 0;
	strcpy(gamemods_uploadTitle, "Title");
	strcpy(gamemods_uploadDescription, "Description");
	strcpy(gamemods_newBlankDirectory, "");
	strcpy(gamemods_newBlankDirectoryOldName, "");
	gamemods_newBlankDirectoryStatus = 0;
	gamemods_currentEditField = 0;
	gamemods_workshopSetPropertyReturn[0] = false;
	gamemods_workshopSetPropertyReturn[1] = false;
	gamemods_workshopSetPropertyReturn[2] = false;
	gamemods_subscribedItemsStatus = 0;
}

bool drawClickableButton(int padx, int pady, int padw, int padh, Uint32 btnColor)
{
	bool clicked = false;
	if ( mouseInBounds(clientnum, padx, padx + padw, pady - 4, pady + padh) )
	{
		drawDepressed(padx, pady - 4, padx + padw, pady + padh);
		if ( inputs.bMouseLeft(clientnum) )
		{
			playSound(139, 64);
			inputs.mouseClearLeft(clientnum);
			clicked = true;
		}
	}
	else
	{
		drawWindow(padx, pady - 4, padx + padw, pady + padh);
	}
	SDL_Rect pos;
	pos.x = padx;
	pos.y = pady - 4;
	pos.w = padw;
	pos.h = padh + 4;
	if ( btnColor != 0 )
	{
		drawRect(&pos, btnColor, 64);
	}
	return clicked;
}
#ifdef STEAMWORKS
void gamemodsWorkshopPreloadMod(int fileID, std::string modTitle)
{
	char fullpath[PATH_MAX] = "";
	useModelCache = false;
	if ( SteamUGC()->GetItemInstallInfo(fileID, NULL, fullpath, PATH_MAX, NULL) )
	{
		gamemods_modPreload = true;
		bool addToPath = !gamemodsIsPathInMountedFiles(fullpath);
		if ( PHYSFS_mount(fullpath, NULL, 0) )
		{
			reloadLanguage();
			if ( addToPath )
			{
				gamemods_mountedFilepaths.push_back(std::make_pair(fullpath, modTitle)); // change string to your mod name here.
				gamemods_workshopLoadedFileIDMap.push_back(std::make_pair(modTitle, fileID));
			}
		}
	}
}
#else
size_t serialHash(std::string input)
{
	if ( input.empty() || input.size() != 19 )
	{
		return 0;
	}
	int i = 0;
	size_t hash = 0;
	for ( std::string::iterator it = input.begin(); it != input.end(); ++it )
	{
		char c = *it;
		if ( c == '\0' || c == '\n' )
		{
			break;
		}
		hash += static_cast<size_t>(c) * (i * i);
		++i;
	}
	return hash;
}

void buttonConfirmSerial(button_t* my)
{
	serialVerifyWindow = 1;
	if ( SDL_IsTextInputActive() )
	{
		SDL_StopTextInput();
	}
	list_FreeAll(&button_l);
	deleteallbuttons = true;
}

void windowSerialResult(int success)
{
	// close current window
	if ( success > 0 )
	{
		char path[PATH_MAX] = "";
		if ( success == 2 )
		{
			completePath(path, "legendsandpariahs.key", outputdir);
		}
		else if ( success == 1 )
		{
			completePath(path, "mythsandoutcasts.key", outputdir);
		}

		// open the serial file
		File* fp = nullptr;
		if ( (fp = FileIO::open(path, "wb")) == NULL )
		{
			printlog("ERROR: failed to save license file!\n");
		}
		else
		{
			fp->write(serialInputText, sizeof(char), strlen(serialInputText));
			FileIO::close(fp);
		}
	}
	buttonCloseSubwindow(nullptr);
	list_FreeAll(&button_l);
	deleteallbuttons = true;

	subwindow = 1;
	subx1 = xres / 2 - 250;
	subx2 = xres / 2 + 250;
	suby1 = yres / 2 - 32;
	suby2 = yres / 2 + 32;

	// ok button
	button_t* button = newButton();
	strcpy(button->label, language[1317]);
	button->x = subx2 - strlen(language[1317]) * 12 - 16;
	button->y = suby2 - 28;
	button->sizex = strlen(language[1317]) * 12 + 8;
	button->sizey = 20;
	button->action = &buttonCloseSubwindow;
	button->visible = 1;
	button->focused = 1;

	// close button
	button = newButton();
	strcpy(button->label, "x");
	button->x = subx2 - 20;
	button->y = suby1;
	button->sizex = 20;
	button->sizey = 20;
	button->action = &buttonCloseSubwindow;
	button->visible = 1;
	button->focused = 1;
	button->key = SDL_SCANCODE_ESCAPE;
	button->joykey = joyimpulses[INJOY_MENU_CANCEL];

	if ( success > 0 )
	{
		if ( success == 2 )
		{
			strcpy(subtext, language[3405]);
		}
		else if ( success == 1 )
		{
			strcpy(subtext, language[3404]);
		}
		playSound(402, 92);
	}
	else
	{
		strcpy(subtext, language[3406]);
	}
}

void windowEnterSerialPrompt()
{
	// create confirmation window
	subwindow = 1;
	subx1 = xres / 2 - 300;
	subx2 = xres / 2 + 300;
	suby1 = yres / 2 - 64;
	suby2 = yres / 2 + 64;
	strcpy(subtext, language[3403]);
	strcpy(serialInputText, "");
	serialEnterWindow = true;
	serialVerifyWindow = 0;

	// yes button
	button_t* button = newButton();
	strcpy(button->label, "Submit");
	button->x = subx2 - strlen("Submit") * 12 - 16;
	button->y = suby2 - 28 * 2;
	button->sizex = strlen("Submit") * 12 + 8;
	button->sizey = 20;
	button->action = &buttonConfirmSerial;
	button->visible = 1;
	button->focused = 1;
	button->key = SDL_SCANCODE_RETURN;
	button->joykey = joyimpulses[INJOY_MENU_NEXT]; //TODO: Select which button to activate via dpad.

	// cancel button
	button = newButton();
	strcpy(button->label, language[1316]);
	button->x = subx2 - strlen(language[1316]) * 12 - 16;
	button->y = suby2 - 28;
	button->sizex = strlen(language[1316]) * 12 + 8;
	button->sizey = 20;
	button->action = &buttonCloseSubwindow;
	button->visible = 1;
	button->focused = 1;

	// close button
	button = newButton();
	strcpy(button->label, "x");
	button->x = subx2 - 20;
	button->y = suby1;
	button->sizex = 20;
	button->sizey = 20;
	button->action = &buttonCloseSubwindow;
	button->visible = 1;
	button->focused = 1;
	button->key = SDL_SCANCODE_ESCAPE;
	button->joykey = joyimpulses[INJOY_MENU_CANCEL];
}
#endif // STEAMWORKS