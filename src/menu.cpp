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
#include "engine/audio/sound.hpp"
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
#ifdef USE_PLAYFAB
#include "playfab.hpp"
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

#include "ui/Text.hpp"
#include "ui/Font.hpp"
#include "ui/MainMenu.hpp"
#include "ui/Image.hpp"

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
//int gamemods_numCurrentModsLoaded = -1;
const int gamemods_maxTags = 10;
std::vector<std::pair<std::string, std::string>> gamemods_mountedFilepaths;
std::list<std::string> gamemods_localModFoldernames;
//bool gamemods_modelsListRequiresReload = false;
//bool gamemods_soundListRequiresReload = false;
bool gamemods_modelsListLastStartedUnmodded = false; // if starting regular game that had to reset model list, use this to reinit custom models.
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
Sint32 gearsize = 0;
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
bool losingConnection[MAXPLAYERS] = { false };

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

bool isAchievementUnlockedForClassUnlock(int race)
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
#elif (defined USE_EOS || defined LOCAL_ACHIEVEMENTS)
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

int isCharacterValidFromDLC(int player, int characterClass, int race, int appearance)
{
	if ( player < 0 || player >= MAXPLAYERS )
	{
		return INVALID_CHARACTER;
	}
	auto oldAppearance = stats[player]->stat_appearance;
	auto oldRace = stats[player]->playerRace;
	stats[player]->stat_appearance = appearance;
	stats[player]->playerRace = race;
	auto result = isCharacterValidFromDLC(*stats[player], characterClass);
	stats[player]->stat_appearance = oldAppearance;
	stats[player]->playerRace = oldRace;
	return result;
}

int isCharacterValidFromDLC(Stat& myStats, int characterClass)
{
	bool challengeClass = false;
	bool challengeRace = false;
	if ( gameModeManager.currentSession.challengeRun.isActive() )
	{
		if ( gameModeManager.currentSession.challengeRun.classnum >= 0
			&& gameModeManager.currentSession.challengeRun.classnum < NUMCLASSES )
		{
			challengeClass = (characterClass == gameModeManager.currentSession.challengeRun.classnum);
		}
		if ( gameModeManager.currentSession.challengeRun.race >= 0
			&& gameModeManager.currentSession.challengeRun.race <= RACE_INSECTOID )
		{
			challengeRace = (myStats.playerRace == gameModeManager.currentSession.challengeRun.race);
		}
	}

	switch ( characterClass )
	{
		case CLASS_CONJURER:
		case CLASS_ACCURSED:
		case CLASS_MESMER:
		case CLASS_BREWER:
			if ( !enabledDLCPack1 )
			{
				if ( !challengeClass )
				{
					return INVALID_REQUIREDLC1;
				}
			}
			break;
		case CLASS_MACHINIST:
		case CLASS_PUNISHER:
		case CLASS_SHAMAN:
		case CLASS_HUNTER:
			if ( !enabledDLCPack2 )
			{
				if ( !challengeClass )
				{
					return INVALID_REQUIREDLC2;
				}
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
				if ( !challengeRace )
				{
					return INVALID_REQUIREDLC1;
				}
			}
			break;
		case RACE_AUTOMATON:
		case RACE_INCUBUS:
		case RACE_GOBLIN:
		case RACE_INSECTOID:
			if ( !enabledDLCPack2 )
			{
				if ( !challengeRace )
				{
					return INVALID_REQUIREDLC2;
				}
			}
			break;
		default:
			break;
	}

	if ( myStats.playerRace == RACE_HUMAN )
	{
		return VALID_OK_CHARACTER;
	}
	else if ( myStats.playerRace > RACE_HUMAN && myStats.stat_appearance == 1 )
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

/*-------------------------------------------------------------------------------

	handleMainMenu

	draws & processes the game menu; if passed true, does the whole menu,
	otherwise just handles the reduced ingame menu

-------------------------------------------------------------------------------*/

static void handleMainMenu(bool mode)
{
	// deprecated
#if 0
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

	if ( true )
	{
		// title pic
		SDL_Rect src;
		src.x = 20;
		src.y = 20;
		//src.w = title_bmp->w * (230.0 / 240.0); // new banner scaled to old size.
		//src.h = title_bmp->h * (230.0 / 240.0);
		if ( mode || introstage != 5 )
		{
			//drawImageScaled(title_bmp, nullptr, &src);
		}

		if ( mode && subtitleVisible )
		{
			Uint32 colorYellow = makeColor( 255, 255, 0, 255);
			Uint32 len = strlen(Language::get(1910 + subtitleCurrent));
			ttfPrintTextColor(ttf16, src.x + src.w / 2 - (len * TTF16_WIDTH) / 2, src.y + src.h - 32, colorYellow, true, Language::get(1910 + subtitleCurrent));
		}
#ifdef STEAMWORKS
		if ( mode )
		{
			if ( SteamUser()->BLoggedOn() )
			{
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
#endif

		// gray text color
		Uint32 colorGray = makeColor( 128, 128, 128, 255);

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
			if ( Mods::disableSteamAchievements
				|| (intro == false && 
					(conductGameChallenges[CONDUCT_CHEATS_ENABLED]
					|| conductGameChallenges[CONDUCT_LIFESAVING])) )
			{
				getSizeOfText(ttf8, Language::get(3003), &w, &h);
				if ( gamemods_numCurrentModsLoaded < 0 && !conductGameChallenges[CONDUCT_MODDED] )
				{
					h = -4;
				}
				if ( gameModeManager.getMode() != GameModeManager_t::GAME_MODE_DEFAULT )
				{
					// achievements are disabled
					ttfPrintTextFormatted(ttf8, xres - 8 - w, yres - 16 - h - h2 * 3, Language::get(3003));
				}
				else
				{
					// achievements are disabled
					ttfPrintTextFormatted(ttf8, xres - 8 - w, yres - 16 - h - h2 * 3, Language::get(3003));
				}
			}
#endif

#ifdef STEAMWORKS
			getSizeOfText(ttf8, Language::get(2549), &w, &h);
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
				ttfPrintTextFormattedColor(ttf8, xres - 8 - w, 8, colorGray, Language::get(2549), steamOnlinePlayers);
			}
			else if ( SteamUser()->BLoggedOn() )
			{
				ttfPrintTextFormatted(ttf8, xres - 8 - w, 8, Language::get(2549), steamOnlinePlayers);
			}
			if ( intro == false )
			{
				if ( conductGameChallenges[CONDUCT_CHEATS_ENABLED] )
				{
					getSizeOfText(ttf8, Language::get(2986), &w, &h);
					ttfPrintTextFormatted(ttf8, xres - 8 - w, 8 + h, Language::get(2986));
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
				getSizeOfText(ttf8, Language::get(3402), &w, &h);
				if ( (omousex >= xres - 8 - w && omousex < xres && omousey >= 8 && omousey < 8 + h)
					&& subwindow == 0 )
				{
					if ( inputs.bMouseLeft(clientnum) )
					{
						inputs.mouseClearLeft(clientnum);
						playSound(139, 64);
						windowEnterSerialPrompt();
						
					}
					ttfPrintTextFormattedColor(ttf8, xres - 8 - w, 8, colorGray, Language::get(3402));
				}
				else
				{
					ttfPrintTextFormatted(ttf8, xres - 8 - w, 8, Language::get(3402));
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
			if ( keystatus[SDLK_l] && (keystatus[SDLK_LCTRL] || keystatus[SDLK_RCTRL]) )
			{
				buttonOpenCharacterCreationWindow(nullptr);
				client_classes[clientnum] = CLASS_BARBARIAN;
				stats[0]->appearance = 0;
				stats[0]->playerRace = RACE_HUMAN;
				initClass(0);
				strcpy(stats[0]->name, "The Server");
				keystatus[SDLK_l] = 0;
				keystatus[SDLK_LCTRL] = 0;
				keystatus[SDLK_RCTRL] = 0;
				multiplayerselect = SERVER;
				charcreation_step = 6;
				camera_charsheet_offsetyaw = (330) * PI / 180;
				directConnect = true;
				strcpy(portnumber_char, "12345");
				buttonHostLobby(nullptr);
			}

			if ( keystatus[SDLK_m] && (keystatus[SDLK_LCTRL] || keystatus[SDLK_RCTRL]) )
			{
				buttonOpenCharacterCreationWindow(nullptr);
				client_classes[clientnum] = CLASS_BARBARIAN;
				stats[0]->appearance = 0;
				stats[0]->playerRace = RACE_HUMAN;
				initClass(0);
				strcpy(stats[0]->name, "The Client");
				keystatus[SDLK_m] = 0;
				keystatus[SDLK_LCTRL] = 0;
				keystatus[SDLK_RCTRL] = 0;
				multiplayerselect = CLIENT;
				charcreation_step = 6;
				camera_charsheet_offsetyaw = (330) * PI / 180;
				directConnect = true;
				strcpy(connectaddress, "localhost:12345");
				buttonJoinLobby(nullptr);
			}

			bool mainMenuSelectInputIsPressed = (inputs.bMouseLeft(clientnum) || keystatus[SDLK_RETURN] || (inputs.bControllerInputPressed(clientnum, INJOY_MENU_NEXT) && rebindaction == -1));

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
						getSizeOfText(ttf16, Language::get(2990), &w, &h);
						ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, Language::get(2990));
						GO_SwapBuffers(screen);

						physfsModelIndexUpdate(modelsIndexUpdateStart, modelsIndexUpdateEnd);
						generatePolyModels(modelsIndexUpdateStart, modelsIndexUpdateEnd, false);
						generateVBOs(modelsIndexUpdateStart, modelsIndexUpdateEnd);
						gamemods_modelsListLastStartedUnmodded = true;
					}
					if ( reloadSounds )
					{
						// print a loading message
						drawClearBuffers();
						int w, h;
						getSizeOfText(ttf16, Language::get(2988), &w, &h);
						ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, Language::get(2988));
						GO_SwapBuffers(screen);
						physfsReloadSounds(true);
						gamemods_soundsListLastStartedUnmodded = true;
					}

					if ( gamemods_tileListRequireReloadUnmodded )
					{
						drawClearBuffers();
						int w, h;
						getSizeOfText(ttf16, Language::get(3018), &w, &h);
						ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, Language::get(3018));
						GO_SwapBuffers(screen);
						physfsReloadTiles(true);
						gamemods_tileListRequireReloadUnmodded = false;
					}

					if ( gamemods_booksRequireReloadUnmodded )
					{
						drawClearBuffers();
						int w, h;
						getSizeOfText(ttf16, Language::get(2992), &w, &h);
						ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, Language::get(2992));
						GO_SwapBuffers(screen);
						physfsReloadBooks();
						gamemods_booksRequireReloadUnmodded = false;
					}

					if ( gamemods_musicRequireReloadUnmodded )
					{
						gamemodsUnloadCustomThemeMusic();
						drawClearBuffers();
						int w, h;
						getSizeOfText(ttf16, Language::get(2994), &w, &h);
						ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, Language::get(2994));
						GO_SwapBuffers(screen);
						bool reloadIntroMusic = false;
						physfsReloadMusic(reloadIntroMusic, true);
						if ( reloadIntroMusic )
						{
#ifdef SOUND
							playMusic(intromusic[local_rng.rand() % (NUMINTROMUSIC - 1)], false, true, true);
#endif			
						}
						gamemods_musicRequireReloadUnmodded = false;
					}

					if ( gamemods_langRequireReloadUnmodded )
					{
						drawClearBuffers();
						int w, h;
						getSizeOfText(ttf16, Language::get(3005), &w, &h);
						ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, Language::get(3005));
						GO_SwapBuffers(screen);
						reloadLanguage();
						gamemods_langRequireReloadUnmodded = false;
					}

					if ( gamemods_spriteImagesRequireReloadUnmodded )
					{
						drawClearBuffers();
						int w, h;
						getSizeOfText(ttf16, Language::get(3016), &w, &h);
						ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, Language::get(3016));
						GO_SwapBuffers(screen);
						physfsReloadSprites(true);
						gamemods_spriteImagesRequireReloadUnmodded = false;
					}

					if ( gamemods_monsterLimbsRequireReloadUnmodded )
					{
						drawClearBuffers();
						int w, h;
						getSizeOfText(ttf16, Language::get(3014), &w, &h);
						ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, Language::get(3014));
						GO_SwapBuffers(screen);
						physfsReloadMonsterLimbFiles();
						gamemods_monsterLimbsRequireReloadUnmodded = false;
					}

					if ( gamemods_systemImagesReloadUnmodded )
					{
						drawClearBuffers();
						int w, h;
						getSizeOfText(ttf16, Language::get(3016), &w, &h);
						ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, Language::get(3016));
						GO_SwapBuffers(screen);
						physfsReloadSystemImages();
						gamemods_systemImagesReloadUnmodded = false;
						systemResourceImagesToReload.clear();
					}

					Mods::disableSteamAchievements = false;

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

					introstage = 6; // goes to intro
					fadeout = true;
#ifdef MUSIC
					playMusic(introductionmusic, true, true, false);
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
					strcpy(subtext, Language::get(1128));

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
					button->key = SDLK_ESCAPE;
					button->joykey = joyimpulses[INJOY_MENU_CANCEL];

					// yes button
					button = newButton();
					strcpy(button->label, Language::get(1314));
					button->x = subx1 + 8;
					button->y = suby2 - 28;
					button->sizex = strlen(Language::get(1314)) * 12 + 8;
					button->sizey = 20;
					button->action = &buttonQuitConfirm;
					button->visible = 1;
					button->focused = 1;
					button->key = SDLK_RETURN;
					button->joykey = joyimpulses[INJOY_MENU_NEXT];

					// no button
					button = newButton();
					strcpy(button->label, Language::get(1315));
					button->x = subx2 - strlen(Language::get(1315)) * 12 - 16;
					button->y = suby2 - 28;
					button->sizex = strlen(Language::get(1315)) * 12 + 8;
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
					drawRect(&saveBox, uint32ColorGreen, 32);
				}
				else
				{
					drawRect(&saveBox, uint32ColorBaronyBlue, 32);
				}
				if ( loadGameSaveShowRectangle == 2 )
				{
					saveBox.y = suby1 + TTF12_HEIGHT * 5 + 2;
					//drawTooltip(&saveBox);
					drawWindowFancy(saveBox.x, saveBox.y, saveBox.x + saveBox.w, saveBox.y + saveBox.h);
					if ( gamemods_numCurrentModsLoaded >= 0 )
					{
						drawRect(&saveBox, uint32ColorGreen, 32);
					}
					else
					{
						drawRect(&saveBox, uint32ColorBaronyBlue, 32);
					}
				}
			}
			if ( gamemods_window == 1 || gamemods_window == 2 || gamemods_window == 5 )
			{
				drawWindowFancy(subx1 + 4, suby1 + 44 + 10 * TTF12_HEIGHT,
					subx2 - 4, suby2 - 4);
			}
			if ( subtext[0] != '\0' )
			{
				if ( strncmp(subtext, Language::get(740), 12) )
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
				ttfPrintTextFormattedColor(ttf12, subx1 + 8, suby2 - TTF12_HEIGHT * 5, uint32ColorBaronyBlue, "%s", Language::get(2982));
			}
		}
		else
		{
			loadGameSaveShowRectangle = 0;
		}

		LobbyHandler.drawLobbyFilters();

		// process button actions
		handleButtons();

	}

	// character creation screen
	if ( charcreation_step >= 1 && charcreation_step < 6 )
	{
		if ( gamemods_numCurrentModsLoaded >= 0 )
		{
			ttfPrintText(ttf16, subx1 + 8, suby1 + 8, Language::get(2980));
		}
		else
		{
			ttfPrintText(ttf16, subx1 + 8, suby1 + 8, Language::get(1318));
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
					//glDrawVoxel(&camera_charsheet, players[clientnum]->entity, REALCOLORS);
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
						//glDrawVoxel(&camera_charsheet, entity, REALCOLORS);
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
			raceInfoBtn.w = longestline(Language::get(3373)) * TTF12_WIDTH + 8 + 4;
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
				drawLine(pos.x, pos.y, pos.x + pos.w, pos.y, makeColorRGB(0, 192, 255), 255);
				drawLine(pos.x, pos.y + pos.h, pos.x + pos.w, pos.y + pos.h, makeColorRGB(0, 192, 255), 255);
				drawLine(pos.x, pos.y, pos.x, pos.y + pos.h, makeColorRGB(0, 192, 255), 255);
				drawLine(pos.x + pos.w, pos.y, pos.x + pos.w, pos.y + pos.h, makeColorRGB(0, 192, 255), 255);
				if ( stats[0]->playerRace >= RACE_HUMAN )
				{
					ttfPrintText(ttf12, pos.x + 12, pos.y + 6, Language::get(3375 + stats[0)->playerRace]);
				}
				ttfPrintText(ttf12, raceInfoBtn.x + 4, raceInfoBtn.y + 6, Language::get(3374));
			}
			else
			{
				ttfPrintText(ttf12, raceInfoBtn.x + 4, raceInfoBtn.y + 6, Language::get(3373));
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
			ttfPrintText(ttf16, subx1 + 24, suby1 + 32, Language::get(1319));
			Uint32 colorStep1 = uint32ColorWhite;
			if ( raceSelect != 0 )
			{
				colorStep1 = uint32ColorGray;
			}
			if ( stats[0]->sex == 0 )
			{
				ttfPrintTextFormattedColor(ttf16, subx1 + 32, suby1 + 56, colorStep1, "[o] %s", Language::get(1321));
				ttfPrintTextFormattedColor(ttf16, subx1 + 32, suby1 + 73, colorStep1, "[ ] %s", Language::get(1322));

				ttfPrintTextFormattedColor(ttf12, subx1 + 8, suby2 - 80, uint32ColorWhite, Language::get(1320), Language::get(1321));
			}
			else
			{
				ttfPrintTextFormattedColor(ttf16, subx1 + 32, suby1 + 56, colorStep1, "[ ] %s", Language::get(1321));
				ttfPrintTextFormattedColor(ttf16, subx1 + 32, suby1 + 73, colorStep1, "[o] %s", Language::get(1322));

				ttfPrintTextFormattedColor(ttf12, subx1 + 8, suby2 - 80, uint32ColorWhite, Language::get(1320), Language::get(1322));
			}
			ttfPrintTextFormattedColor(ttf12, subx1 + 8, suby2 - 56, uint32ColorWhite, Language::get(3175));

			// race
			if ( raceSelect != 1 )
			{
				colorStep1 = uint32ColorGray;
			}
			else if ( raceSelect == 1 )
			{
				colorStep1 = uint32ColorWhite;
			}
			ttfPrintText(ttf16, subx1 + 24, suby1 + 108, Language::get(3160));
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
							colorStep1 = uint32ColorGray;
						}
						else
						{
							colorStep1 = uint32ColorWhite;
						}
					}
					else if ( skipFirstDLC )
					{
						if ( c > RACE_HUMAN && c <= RACE_GOATMAN )
						{
							colorStep1 = uint32ColorGray;
						}
						else
						{
							colorStep1 = uint32ColorWhite;
						}
					}
					else if ( !(enabledDLCPack2 && enabledDLCPack1) )
					{
						if ( c > RACE_HUMAN )
						{
							colorStep1 = uint32ColorGray;
						}
						else
						{
							colorStep1 = uint32ColorWhite;
						}
					}
					else if ( enabledDLCPack2 && enabledDLCPack1 )
					{
						colorStep1 = uint32ColorWhite;
					}
				}
				if ( stats[0]->playerRace == c )
				{
					ttfPrintTextFormattedColor(ttf16, subx1 + 32, pady, colorStep1, "[o] %s", Language::get(3161 + c));
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
						ttfPrintTextFormattedColor(ttf16, subx1 + 32, pady, colorStep1, "[ ] %s", Language::get(3161 + c));
					}
					else
					{
						ttfPrintTextFormattedColor(ttf16, subx1 + 32, pady, colorStep1, "[ ] %s", Language::get(3161 + c));
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
				colorStep1 = uint32ColorGray;
			}
			else
			{
				colorStep1 = uint32ColorWhite;
			}
			if ( stats[0]->playerRace > 0 )
			{
				displayRaceOptions = true;
				ttfPrintText(ttf16, subx1 + 24, pady, Language::get(3176));
				pady += 24;
				char raceOptionBuffer[128];
				snprintf(raceOptionBuffer, 63, Language::get(3177), Language::get(3161 + stats[0)->playerRace]);
				if ( stats[0]->appearance > 1 )
				{
					stats[0]->appearance = lastAppearance;
				}
				if ( stats[0]->appearance == 0 )
				{
					ttfPrintTextFormattedColor(ttf16, subx1 + 32, pady, colorStep1, "[o] %s", raceOptionBuffer);
					ttfPrintTextFormattedColor(ttf16, subx1 + 32, pady + 17, colorStep1, "[ ] %s", Language::get(3178));
				}
				else if ( stats[0]->appearance == 1 )
				{
					ttfPrintTextFormattedColor(ttf16, subx1 + 32, pady, colorStep1, "[ ] %s", raceOptionBuffer);
					ttfPrintTextFormattedColor(ttf16, subx1 + 32, pady + 17, colorStep1, "[o] %s", Language::get(3178));
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
										stats[0]->appearance = local_rng.rand() % NUMAPPEARANCES;
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
									tooltip.w = longestline(Language::get(3917)) * TTF12_WIDTH + 8;
									drawTooltip(&tooltip);
									ttfPrintTextFormattedColor(ttf12, tooltip.x + 4, tooltip.y + 6, uint32ColorOrange, Language::get(3917));
								}
								else
								{
									tooltip.h = TTF12_HEIGHT * 2 + 8;
									tooltip.w = longestline(Language::get(3200)) * TTF12_WIDTH + 8;
									drawTooltip(&tooltip);
									ttfPrintTextFormattedColor(ttf12, tooltip.x + 4, tooltip.y + 6, uint32ColorOrange, Language::get(3200));
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
												openURLTryWithOverlay(Language::get(3993));
											}
											else
											{
												openURLTryWithOverlay(Language::get(3992));
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
										openURLTryWithOverlay(Language::get(3985));
										inputs.mouseClearLeft(clientnum);
									}
								}
								else
								{
									if ( inputs.bMouseLeft(clientnum) )
									{
										openURLTryWithOverlay(Language::get(3984));
										inputs.mouseClearLeft(clientnum);
									}
								}
#endif
#else
								if ( c > RACE_GOATMAN && c <= RACE_INSECTOID )
								{
									tooltip.w = longestline(Language::get(3372)) * TTF12_WIDTH + 8;
									drawTooltip(&tooltip);
									ttfPrintTextFormattedColor(ttf12, tooltip.x + 4, tooltip.y + 6, uint32ColorOrange, Language::get(3372));
								}
								else
								{
									tooltip.w = longestline(Language::get(3199)) * TTF12_WIDTH + 8;
									drawTooltip(&tooltip);
									ttfPrintTextFormattedColor(ttf12, tooltip.x + 4, tooltip.y + 6, uint32ColorOrange, Language::get(3199));
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
			if ( keystatus[SDLK_UP] || (inputs.bControllerInputPressed(clientnum, INJOY_DPAD_UP) && rebindaction == -1) )
			{
				keystatus[SDLK_UP] = 0;
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
						stats[0]->appearance = local_rng.rand() % NUMAPPEARANCES;
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
			if ( keystatus[SDLK_DOWN] || (inputs.bControllerInputPressed(clientnum, INJOY_DPAD_DOWN) && rebindaction == -1) )
			{
				keystatus[SDLK_DOWN] = 0;
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
						stats[0]->appearance = local_rng.rand() % NUMAPPEARANCES;
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
			if ( keystatus[SDLK_RIGHT] || (inputs.bControllerInputPressed(clientnum, INJOY_DPAD_RIGHT) && rebindaction == -1) )
			{
				keystatus[SDLK_RIGHT] = 0;
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
			if ( keystatus[SDLK_LEFT] || (inputs.bControllerInputPressed(clientnum, INJOY_DPAD_LEFT) && rebindaction == -1) )
			{
				keystatus[SDLK_LEFT] = 0;
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
			ttfPrintText(ttf16, subx1 + 24, suby1 + 32, Language::get(1323));
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
						ttfPrintTextFormattedColor(ttf16, subx1 + 32, pady, uint32ColorGray, "[ ] %s", playerClassLangEntry(classToPick, 0));

						if ( mouseInBounds(clientnum, subx1 + 40, subx1 + 72, pady, pady + 16) )
						{
#if (defined STEAMWORKS || defined USE_EOS)
							tooltip.x = omousex + 16;
							tooltip.y = omousey + 16;
							tooltip.h = TTF12_HEIGHT + 8;
							if ( classToPick > CLASS_MONK )
							{
								int langline = 3927 + classToPick - CLASS_CONJURER;
								tooltip.w = longestline(Language::get(langline)) * TTF12_WIDTH + 8;
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

				if ( keystatus[SDLK_UP] || (inputs.bControllerInputPressed(clientnum, INJOY_DPAD_UP) && rebindaction == -1) )
				{
					keystatus[SDLK_UP] = 0;
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
				if ( keystatus[SDLK_DOWN] || (inputs.bControllerInputPressed(clientnum, INJOY_DPAD_DOWN) && rebindaction == -1) )
				{
					keystatus[SDLK_DOWN] = 0;
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
					uint32ColorOrange, Language::get(drawLockedTooltip));
			}
		}

		// faces
		else if ( charcreation_step == 3 )
		{
			ttfPrintText(ttf16, subx1 + 24, suby1 + 32, Language::get(1324));
			for ( c = 0; c < NUMAPPEARANCES; c++ )
			{
				if ( stats[0]->appearance == c )
				{
					ttfPrintTextFormatted(ttf16, subx1 + 32, suby1 + 56 + c * 16, "[o] %s", Language::get(20 + c));
					ttfPrintText(ttf12, subx1 + 8, suby2 - 80, Language::get(38 + c));
				}
				else
				{
					ttfPrintTextFormatted(ttf16, subx1 + 32, suby1 + 56 + c * 16, "[ ] %s", Language::get(20 + c));
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
				if ( keystatus[SDLK_UP] || (inputs.bControllerInputPressed(clientnum, INJOY_DPAD_UP) && rebindaction == -1) )
				{
					keystatus[SDLK_UP] = 0;
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
				if ( keystatus[SDLK_DOWN] || (inputs.bControllerInputPressed(clientnum, INJOY_DPAD_DOWN) && rebindaction == -1) )
				{
					keystatus[SDLK_DOWN] = 0;
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
			ttfPrintText(ttf16, subx1 + 24, suby1 + 32, Language::get(1325));
			drawDepressed(subx1 + 40, suby1 + 56, subx1 + 364, suby1 + 88);
			ttfPrintText(ttf16, subx1 + 48, suby1 + 64, stats[0]->name);
			ttfPrintText(ttf12, subx1 + 8, suby2 - 80, Language::get(1326));

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
			ttfPrintText(ttf16, subx1 + 24, suby1 + 32, Language::get(1327));

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
				optionTexts.insert(optionTexts.end(), { Language::get(1328), Language::get(1330), Language::get(1330), Language::get(1332), Language::get(1330), Language::get(1332) });
				optionDescriptions.insert(optionDescriptions.end(), { Language::get(1329), Language::get(3946), Language::get(3947), Language::get(3945), Language::get(1538), Language::get(1539) });
				optionSubtexts.insert(optionSubtexts.end(), { nullptr, Language::get(3943), Language::get(3944), nullptr, Language::get(1537), Language::get(1537) });
				displayedOptionToGamemode.insert(displayedOptionToGamemode.end(), { SINGLE, SERVER, SERVERCROSSPLAY, CLIENT, DIRECTSERVER, DIRECTCLIENT});
			}
			else
			{
				optionY.insert(optionY.end(), { suby1 + 56, suby1 + 76, suby1 + 96, suby1 + 136, suby1 + 176, 0 });
				optionTexts.insert(optionTexts.end(), { Language::get(1328), Language::get(1330), Language::get(1332), Language::get(1330), Language::get(1332), nullptr });
				optionDescriptions.insert(optionDescriptions.end(), { Language::get(1329), Language::get(1331), Language::get(1333), Language::get(1538), Language::get(1539), nullptr });
				optionSubtexts.insert(optionSubtexts.end(), { nullptr, nullptr, nullptr, Language::get(1537), Language::get(1537), nullptr });
				displayedOptionToGamemode.insert(displayedOptionToGamemode.end(), { SINGLE, SERVER, CLIENT, DIRECTSERVER, DIRECTCLIENT, 0 });
			}
#elif (defined(USE_EOS) || defined(STEAMWORKS))
			nummodes += 2;
			optionY.insert(optionY.end(), { suby1 + 56, suby1 + 76, suby1 + 96, suby1 + 136, suby1 + 176, 0 });
			optionTexts.insert(optionTexts.end(), { Language::get(1328), Language::get(1330), Language::get(1332), Language::get(1330), Language::get(1332), nullptr });
			optionDescriptions.insert(optionDescriptions.end(), { Language::get(1329), Language::get(1331), Language::get(1333), Language::get(1538), Language::get(1539), nullptr });
			optionSubtexts.insert(optionSubtexts.end(), { nullptr, nullptr, nullptr, Language::get(1537), Language::get(1537) });
			displayedOptionToGamemode.insert(displayedOptionToGamemode.end(), { SINGLE, SERVER, CLIENT, DIRECTSERVER, DIRECTCLIENT, 0 });
#else
			optionY.insert(optionY.end(), { suby1 + 56, suby1 + 76, suby1 + 96, suby1 + 136, suby1 + 176, 0 });
			optionTexts.insert(optionTexts.end(), { Language::get(1328), Language::get(1330), Language::get(1332), Language::get(1330), Language::get(1332), nullptr });
			optionDescriptions.insert(optionDescriptions.end(), { Language::get(1329), Language::get(1331), Language::get(1333), Language::get(1538), Language::get(1539), nullptr });
			optionSubtexts.insert(optionSubtexts.end(), { nullptr, nullptr, nullptr, Language::get(1537), Language::get(1537) });
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
						ttfPrintTextColor(ttf12, subx1 + 8, suby2 - 60, uint32ColorOrange, true, Language::get(2965));
					}
				}
				else if ( multiplayerselect > SINGLE )
				{
					if ( multiplayerSavegameFreeSlot == -1 )
					{
						ttfPrintTextColor(ttf12, subx1 + 8, suby2 - 60, uint32ColorOrange, true, Language::get(2966));
					}
					if ( gamemods_numCurrentModsLoaded >= 0 )
					{
						ttfPrintTextColor(ttf12, subx1 + 8, suby2 - 60 - TTF12_HEIGHT * 6, uint32ColorOrange, true, Language::get(2981));
					}
				}
				if ( gamemods_numCurrentModsLoaded >= 0 )
				{
					ttfPrintTextColor(ttf12, subx1 + 8, suby2 - 60 + TTF12_HEIGHT, uint32ColorBaronyBlue, true, Language::get(2982));
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
			if (keystatus[SDLK_UP] || (inputs.bControllerInputPressed(clientnum, INJOY_DPAD_UP) && rebindaction == -1) )
			{
				keystatus[SDLK_UP] = 0;
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
			if ( keystatus[SDLK_DOWN] || (inputs.bControllerInputPressed(clientnum, INJOY_DPAD_DOWN) && rebindaction == -1) )
			{
				keystatus[SDLK_DOWN] = 0;
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
	if ( intro && introstage == 1 && subwindow && !strcmp(subtext, Language::get(3403)) && serialEnterWindow )
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
				ttfPrintTextFormattedColor(ttf12, subx1 + 16, suby2 - 20, uint32ColorOrange, "Verifying");
			}
			else if ( serialVerifyWindow % 10 < 5 )
			{
				ttfPrintTextFormattedColor(ttf12, subx1 + 16, suby2 - 20, uint32ColorOrange, "Verifying.");
			}
			else if ( serialVerifyWindow % 10 < 7 )
			{
				ttfPrintTextFormattedColor(ttf12, subx1 + 16, suby2 - 20, uint32ColorOrange, "Verifying..");
			}
			else if ( serialVerifyWindow % 10 < 10 )
			{
				ttfPrintTextFormattedColor(ttf12, subx1 + 16, suby2 - 20, uint32ColorOrange, "Verifying...");
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
			ttfPrintText(ttf12, subx1 + 24, suby1 + 60, Language::get(1338));
			c=0;
			for ( auto cur : resolutions )
			{
				int width = 0, height = 0;
				//std::tie (width, height) = cur;
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
			ttfPrintText(ttf12, subx1 + 224, suby1 + 60, Language::get(1339));
			if ( settings_smoothlighting )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 236, suby1 + 84, "[x] %s", Language::get(1340));
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 236, suby1 + 84, "[ ] %s", Language::get(1340));
			}
			if ( settings_fullscreen )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 236, suby1 + 108, "[x] %s", Language::get(1341));
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 236, suby1 + 108, "[ ] %s", Language::get(1341));
			}
			if ( settings_shaking )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 236, suby1 + 132, "[x] %s", Language::get(1342));
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 236, suby1 + 132, "[ ] %s", Language::get(1342));
			}
			if ( settings_bobbing )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 236, suby1 + 156, "[x] %s", Language::get(1343));
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 236, suby1 + 156, "[ ] %s", Language::get(1343));
			}
			if ( settings_spawn_blood )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 236, suby1 + 180, "[x] %s", Language::get(1344));
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 236, suby1 + 180, "[ ] %s", Language::get(1344));
			}
			if ( settings_colorblind )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 236, suby1 + 204, "[x] %s", Language::get(1345));
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 236, suby1 + 204, "[ ] %s", Language::get(1345));
			}
			if ( settings_light_flicker )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 236, suby1 + 228, "[x] %s", Language::get(2967));
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 236, suby1 + 228, "[ ] %s", Language::get(2967));
			}
			if ( settings_vsync )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 236, suby1 + 252, "[x] %s", Language::get(3011));
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 236, suby1 + 252, "[ ] %s", Language::get(3011));
			}
			if ( settings_status_effect_icons )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 236, suby1 + 276, "[x] %s", Language::get(3357));
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 236, suby1 + 276, "[ ] %s", Language::get(3357));
			}
			if ( settings_borderless )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 236, suby1 + 300, "[x] %s", Language::get(3935));
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 236, suby1 + 300, "[ ] %s", Language::get(3935));
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
			ttfPrintText(ttf12, subx1 + 498, suby1 + 60, Language::get(3022));

			// total scale
			ttfPrintText(ttf12, subx1 + 498, suby1 + 84, Language::get(3025));
			doSlider(subx1 + 498, suby1 + 84 + 24, 10, 2, 16, 1, (int*)(&settings_minimap_scale));

			// objects (players/allies/minotaur) scale
			ttfPrintText(ttf12, subx1 + 498, suby1 + 130, Language::get(3026));
			doSlider(subx1 + 498, suby1 + 130 + 24, 10, 0, 4, 1, (int*)(&settings_minimap_object_zoom));

			// foreground transparency
			ttfPrintText(ttf12, subx1 + 498, suby1 + 176, Language::get(3023));
			doSlider(subx1 + 498, suby1 + 176 + 24, 10, 0, 100, 1, (int*)(&settings_minimap_transparency_foreground));

			// background transparency
			ttfPrintText(ttf12, subx1 + 498, suby1 + 222, Language::get(3024));
			doSlider(subx1 + 498, suby1 + 222 + 24, 10, 0, 100, 1, (int*)(&settings_minimap_transparency_background));

			// UI options
			ttfPrintText(ttf12, subx1 + 498, suby1 + 276, Language::get(3034));

			if ( settings_uiscale_charactersheet )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 498, suby1 + 300, "[x] %s", Language::get(3027));
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 498, suby1 + 300, "[ ] %s", Language::get(3027));
			}
			if ( settings_uiscale_skillspage )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 498, suby1 + 324, "[x] %s", Language::get(3028));
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 498, suby1 + 324, "[ ] %s", Language::get(3028));
			}
			if ( settings_hide_statusbar )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 498, suby1 + 348, "[x] %s", Language::get(3033));
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 498, suby1 + 348, "[ ] %s", Language::get(3033));
			}
			if ( settings_hide_playertags )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 498, suby1 + 372, "[x] %s", Language::get(3136));
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 498, suby1 + 372, "[ ] %s", Language::get(3136));
			}
			if ( settings_show_skill_values )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 498, suby1 + 396, "[x] %s", Language::get(3159));
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 498, suby1 + 396, "[ ] %s", Language::get(3159));
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
			ttfPrintText(ttf12, subx1 + 498, suby2 - 220, Language::get(3029));
			doSliderF(subx1 + 498, suby2 - 220 + 24, 10, 1.f, 2.f, 0.25, &settings_uiscale_hotbar);
			ttfPrintText(ttf12, subx1 + 498, suby2 - 174, Language::get(3030));
			doSliderF(subx1 + 498, suby2 - 174 + 24, 10, 1.f, 2.f, 0.25, &settings_uiscale_chatlog);
			ttfPrintText(ttf12, subx1 + 498, suby2 - 128, Language::get(3031));
			doSliderF(subx1 + 498, suby2 - 128 + 24, 10, 1.f, 2.f, 0.25, &settings_uiscale_playerbars);
			ttfPrintText(ttf12, subx1 + 498, suby2 - 80, Language::get(3032));
			doSliderF(subx1 + 498, suby2 - 80 + 24, 10, 1.f, 2.f, 0.25, &settings_uiscale_inventory);

			// fov slider
			ttfPrintText(ttf12, subx1 + 24, suby2 - 174, Language::get(1346));
			doSlider(subx1 + 24, suby2 - 148, 14, 40, 100, 1, (int*)(&settings_fov));

			// gamma slider
			ttfPrintText(ttf12, subx1 + 24, suby2 - 128, Language::get(1347));
			doSliderF(subx1 + 24, suby2 - 104, 14, 0.25, 2.f, 0.25, &settings_gamma);

			// fps slider
			ttfPrintText(ttf12, subx1 + 24, suby2 - 80, Language::get(2411));
			doSlider(subx1 + 24, suby2 - 56, 14, 60, 300, 1, (int*)(&settings_fps));
		}

		// audio tab
		if ( settings_tab == SETTINGS_AUDIO_TAB )
		{
			ttfPrintText(ttf12, subx1 + 24, suby1 + 60, Language::get(1348));
			doSlider(subx1 + 24, suby1 + 84, 15, 0, 128, 0, &settings_sfxvolume);

			ttfPrintText(ttf12, subx1 + 24, suby1 + 108, Language::get(3972));
			doSlider(subx1 + 24, suby1 + 132, 15, 0, 128, 0, &settings_sfxAmbientVolume);

			ttfPrintText(ttf12, subx1 + 24, suby1 + 156, Language::get(3973));
			doSlider(subx1 + 24, suby1 + 180, 15, 0, 128, 0, &settings_sfxEnvironmentVolume);

			ttfPrintText(ttf12, subx1 + 24, suby1 + 204, Language::get(1349));
			doSlider(subx1 + 24, suby1 + 228, 15, 0, 128, 0, &settings_musvolume);

			if ( settings_minimap_ping_mute )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 24, suby1 + 264, "[x] %s", Language::get(3012));
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 24, suby1 + 264, "[ ] %s", Language::get(3012));
			}
			if ( settings_mute_audio_on_focus_lost )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 24, suby1 + 288, "[x] %s", Language::get(3158));
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 24, suby1 + 288, "[ ] %s", Language::get(3158));
			}
			if ( settings_mute_player_monster_sounds )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 24, suby1 + 312, "[x] %s", Language::get(3371));
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 24, suby1 + 312, "[ ] %s", Language::get(3371));
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
			ttfPrintText(ttf12, subx1 + 24, suby1 + 60, Language::get(1350));

			bool rebindingkey = false;
			if ( rebindkey != -1 )
			{
				rebindingkey = true;
			}

			for ( int c = 0; c < NUMIMPULSES; c++ )
			{
				if ( c < 14 )
				{
					ttfPrintText(ttf12, subx1 + 24, suby1 + 84 + 16 * c, Language::get(1351 + c));
				}
				else if ( c < 16 )
				{
					ttfPrintText(ttf12, subx1 + 24, suby1 + 84 + 16 * c, Language::get(1940 + (c - 14)));
				}
				else if ( c < 22 )
				{
					ttfPrintText(ttf12, subx1 + 24, suby1 + 84 + 16 * c, Language::get(1986 + (c - 16)));
				}
				else if ( c < 25 )
				{
					ttfPrintText(ttf12, subx1 + 24, suby1 + 84 + 16 * c, Language::get(3901 + (c - 22)));
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
						ttfPrintTextColor(ttf12, subx1 + 256, suby1 + 84 + c * 16, uint32ColorBaronyBlue, true, getInputName(settings_impulses[c]));
					}
					else if ( !strcmp(getInputName(settings_impulses[c]), "Unknown key") || !strcmp(getInputName(settings_impulses[c]), "Unknown trigger") )
					{
						ttfPrintTextColor(ttf12, subx1 + 256, suby1 + 84 + c * 16, uint32ColorRed, true, getInputName(settings_impulses[c]));
					}
					else
					{
						ttfPrintText(ttf12, subx1 + 256, suby1 + 84 + c * 16, getInputName(settings_impulses[c]));
					}
				}
				else
				{
					ttfPrintTextColor(ttf12, subx1 + 256, suby1 + 84 + c * 16, uint32ColorGreen, true, "...");
				}
			}

			if ( rebindkey != -1 && lastkeypressed )
			{
				if ( lastkeypressed == SDLK_ESCAPE )
				{
					keystatus[SDLK_ESCAPE] = 0;
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
			ttfPrintText(ttf12, subx1 + 24, suby1 + 60, Language::get(1365));
			doSliderF(subx1 + 24, suby1 + 84, 11, 0, 128, 1, &settings_mousespeed);

			// checkboxes
			if ( settings_reversemouse )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 24, suby1 + 108, "[x] %s", Language::get(1366));
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 24, suby1 + 108, "[ ] %s", Language::get(1366));
			}
			if ( settings_smoothmouse )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 24, suby1 + 132, "[x] %s", Language::get(1367));
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 24, suby1 + 132, "[ ] %s", Language::get(1367));
			}
			if ( settings_disablemouserotationlimit )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 24, suby1 + 156, "[x] %s", Language::get(3918));
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 24, suby1 + 156, "[ ] %s", Language::get(3918));
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
			ttfPrintText(ttf8, currentPos.x, currentPos.y, Language::get(1996));
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
			drawLine(subx1 + 24, currentPos.y - 6, subx2 - 24, currentPos.y - 6, uint32ColorGray, 255);
			ttfPrintText(ttf8, currentPos.x, currentPos.y, Language::get(1994));
			currentPos.y += 18;
			for ( c = INDEX_JOYBINDINGS_START_MENU; c < INDEX_JOYBINDINGS_START_GAME; ++c, currentPos.y += 12 )
			{
				printJoybindingNames(currentPos, c, rebindingaction);
			}

			//Print out the game-exclusive bindings.
			currentPos.y += 12;
			drawLine(subx1 + 24, currentPos.y - 6, subx2 - 24, currentPos.y - 6, uint32ColorGray, 255);
			ttfPrintText(ttf8, currentPos.x, currentPos.y, Language::get(1995));
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
					//*inputPressed(lastkeypressed) = 0; //To prevent bugs where the button will still be treated as pressed after assigning it, potentially doing wonky things.
					rebindaction = -1;
				}
				else
				{
					if (lastkeypressed == SDLK_ESCAPE)
					{
						keystatus[SDLK_ESCAPE] = 0;
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
				ttfPrintTextFormatted(ttf12, current_option_x, current_option_y, "[x] %s", Language::get(2401));
			}
			else
			{
				ttfPrintTextFormatted(ttf12, current_option_x, current_option_y, "[ ] %s", Language::get(2401));
			}

			if (inputs.bMouseLeft(clientnum) && mouseInBounds(clientnum, current_option_x, current_option_x + strlen("[x]")*TTF12_WIDTH, current_option_y, current_option_y + TTF12_HEIGHT))
			{
				inputs.mouseClearLeft(clientnum);
				settings_gamepad_leftx_invert = !settings_gamepad_leftx_invert;
			}

			current_option_y += 24;

			if (settings_gamepad_lefty_invert)
			{
				ttfPrintTextFormatted(ttf12, current_option_x, current_option_y, "[x] %s", Language::get(2402));
			}
			else
			{
				ttfPrintTextFormatted(ttf12, current_option_x, current_option_y, "[ ] %s", Language::get(2402));
			}

			if (inputs.bMouseLeft(clientnum) && mouseInBounds(clientnum, current_option_x, current_option_x + strlen("[x]")*TTF12_WIDTH, current_option_y, current_option_y + TTF12_HEIGHT))
			{
				inputs.mouseClearLeft(clientnum);
				settings_gamepad_lefty_invert = !settings_gamepad_lefty_invert;
			}

			current_option_y += 24;

			if (settings_gamepad_rightx_invert)
			{
				ttfPrintTextFormatted(ttf12, current_option_x, current_option_y, "[x] %s", Language::get(2403));
			}
			else
			{
				ttfPrintTextFormatted(ttf12, current_option_x, current_option_y, "[ ] %s", Language::get(2403));
			}

			if (inputs.bMouseLeft(clientnum) && mouseInBounds(clientnum, current_option_x, current_option_x + strlen("[x]")*TTF12_WIDTH, current_option_y, current_option_y + TTF12_HEIGHT))
			{
				inputs.mouseClearLeft(clientnum);
				settings_gamepad_rightx_invert = !settings_gamepad_rightx_invert;
			}

			current_option_y += 24;

			if (settings_gamepad_righty_invert)
			{
				ttfPrintTextFormatted(ttf12, current_option_x, current_option_y, "[x] %s", Language::get(2404));
			}
			else
			{
				ttfPrintTextFormatted(ttf12, current_option_x, current_option_y, "[ ] %s", Language::get(2404));
			}

			if (inputs.bMouseLeft(clientnum) && mouseInBounds(clientnum, current_option_x, current_option_x + strlen("[x]")*TTF12_WIDTH, current_option_y, current_option_y + TTF12_HEIGHT))
			{
				inputs.mouseClearLeft(clientnum);
				settings_gamepad_righty_invert = !settings_gamepad_righty_invert;
			}

			current_option_y += 24;

			if (settings_gamepad_menux_invert)
			{
				ttfPrintTextFormatted(ttf12, current_option_x, current_option_y, "[x] %s", Language::get(2405));
			}
			else
			{
				ttfPrintTextFormatted(ttf12, current_option_x, current_option_y, "[ ] %s", Language::get(2405));
			}

			if (inputs.bMouseLeft(clientnum) && mouseInBounds(clientnum, current_option_x, current_option_x + strlen("[x]")*TTF12_WIDTH, current_option_y, current_option_y + TTF12_HEIGHT))
			{
				inputs.mouseClearLeft(clientnum);
				settings_gamepad_menux_invert = !settings_gamepad_menux_invert;
			}

			current_option_y += 24;

			if (settings_gamepad_menuy_invert)
			{
				ttfPrintTextFormatted(ttf12, current_option_x, current_option_y, "[x] %s", Language::get(2406));
			}
			else
			{
				ttfPrintTextFormatted(ttf12, current_option_x, current_option_y, "[ ] %s", Language::get(2406));
			}

			if (inputs.bMouseLeft(clientnum) && mouseInBounds(clientnum, current_option_x, current_option_x + strlen("[x]")*TTF12_WIDTH, current_option_y, current_option_y + TTF12_HEIGHT))
			{
				inputs.mouseClearLeft(clientnum);
				settings_gamepad_menuy_invert = !settings_gamepad_menuy_invert;
			}

			current_option_y += 24;

			ttfPrintText(ttf12, current_option_x, current_option_y, Language::get(2407));
			current_option_y += 24;
			//doSlider(current_option_x, current_option_y, 11, 1, 2000, 200, &settings_gamepad_rightx_sensitivity, font8x8_bmp, 12); //Doesn't like any fonts besides the default.
			doSlider(current_option_x, current_option_y, 11, 1, 4096, 100, &settings_gamepad_rightx_sensitivity);

			current_option_y += 24;

			ttfPrintText(ttf12, current_option_x, current_option_y, Language::get(2408));
			current_option_y += 24;
			//doSlider(current_option_x, current_option_y, 11, 1, 2000, 200, &settings_gamepad_righty_sensitivity, font8x8_bmp, 12);
			doSlider(current_option_x, current_option_y, 11, 1, 4096, 100, &settings_gamepad_righty_sensitivity);

			current_option_y += 24;

			ttfPrintText(ttf12, current_option_x, current_option_y, Language::get(2409));
			current_option_y += 24;
			//doSlider(current_option_x, current_option_y, 11, 1, 2000, 200, &settings_gamepad_menux_sensitivity, font8x8_bmp, 12);
			doSlider(current_option_x, current_option_y, 11, 1, 4096, 100, &settings_gamepad_menux_sensitivity);

			current_option_y += 24;

			ttfPrintText(ttf12, current_option_x, current_option_y, Language::get(2410));
			current_option_y += 24;
			//doSlider(current_option_x, current_option_y, 11, 1, 2000, 200, &settings_gamepad_menuy_sensitivity, font8x8_bmp, 12);
			doSlider(current_option_x, current_option_y, 11, 1, 4096, 100, &settings_gamepad_menuy_sensitivity);
		}

		// miscellaneous options
		if (settings_tab == SETTINGS_MISC_TAB)
		{
			int current_x = subx1;
			int current_y = suby1 + 60;

			ttfPrintText(ttf12, subx1 + 24, current_y, Language::get(1371));
			current_y += 24;

			int options_start_y = current_y;
			if ( settings_broadcast )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 36, current_y, "[x] %s", Language::get(1372));
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 36, current_y, "[ ] %s", Language::get(1372));
			}
			current_y += 16;
			if ( settings_nohud )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 36, current_y, "[x] %s", Language::get(1373));
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 36, current_y, "[ ] %s", Language::get(1373));
			}
			current_y += 16;
			int hotbar_options_x = subx1 + 72 + 256;
			int hotbar_options_y = current_y;
			if ( settings_auto_hotbar_new_items )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 36, current_y, "[x] %s", Language::get(1374));
				int pad_x = hotbar_options_x;
				int pad_y = hotbar_options_y;
				drawWindowFancy(pad_x - 16, pad_y - 32, pad_x + 4 * 128 + 16, pad_y + 48 + 16);
				ttfPrintTextFormatted(ttf12, pad_x, current_y - 16, "%s", Language::get(2583));
				for ( int i = 0; i < (NUM_HOTBAR_CATEGORIES); ++i )
				{
					if ( settings_auto_hotbar_categories[i] == true )
					{
						ttfPrintTextFormatted(ttf12, pad_x, pad_y, "[x] %s", Language::get(2571 + i));
					}
					else
					{
						ttfPrintTextFormatted(ttf12, pad_x, pad_y, "[ ] %s", Language::get(2571 + i));
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
				ttfPrintTextFormatted(ttf12, subx1 + 36, current_y, "[ ] %s", Language::get(1374));
			}

			// autosort inventory categories
			int autosort_options_x = subx1 + 72 + 256;
			int autosort_options_y = current_y + 112;
			int pad_x = autosort_options_x;
			int pad_y = autosort_options_y;
			drawWindowFancy(pad_x - 16, pad_y - 32, pad_x + 4 * 128 + 16, pad_y + 48 + 16);
			ttfPrintTextFormatted(ttf12, pad_x, current_y - 16 + 112, "%s", Language::get(2912));

			// draw the values for autosort
			for ( int i = 0; i < (NUM_AUTOSORT_CATEGORIES); ++i )
			{
				ttfPrintTextFormatted(ttf12, pad_x, pad_y, "<");
				Uint32 autosortColor = uint32ColorGreen;
				int padValue_x = pad_x;
				if ( settings_autosort_inventory_categories[i] < 0 )
				{
					autosortColor = uint32ColorRed;
					padValue_x += 4; // centre the negative numbers.
				}
				else if ( settings_autosort_inventory_categories[i] == 0 )
				{
					autosortColor = uint32ColorWhite;
				}
				ttfPrintTextFormattedColor(ttf12, padValue_x, pad_y, autosortColor, " %2d", settings_autosort_inventory_categories[i]);
				if ( i == NUM_AUTOSORT_CATEGORIES - 1 )
				{
					ttfPrintTextFormatted(ttf12, pad_x, pad_y, "    > %s", Language::get(2916));
				}
				else
				{
					ttfPrintTextFormatted(ttf12, pad_x, pad_y, "    > %s", Language::get(2571 + i));
				}
				pad_x += 128;
				if ( i == 3 || i == 7 )
				{
					pad_x = autosort_options_x;
					pad_y += 16;
				}
			}

			pad_x = autosort_options_x + (strlen(Language::get(2912)) - 3) * (TTF12_WIDTH) + 8; // 3 chars from the end of string.
			pad_y = autosort_options_y;
			// hover text for autosort title text
			if ( mouseInBounds(clientnum, pad_x - 4, pad_x + 3 * TTF12_WIDTH + 8, current_y - 16 + 112, current_y - 16 + 124) )
			{
				tooltip_box.x = omousex - TTF12_WIDTH * 32;
				tooltip_box.y = omousey - (TTF12_HEIGHT * 3 + 16);
				tooltip_box.w = strlen(Language::get(2914)) * TTF12_WIDTH + 8;
				tooltip_box.h = TTF12_HEIGHT * 3 + 8;
				drawTooltip(&tooltip_box);
				ttfPrintTextFormatted(ttf12, tooltip_box.x + 4, tooltip_box.y + 4, Language::get(2913));
				ttfPrintTextFormatted(ttf12, tooltip_box.x + 4, tooltip_box.y + 4 + TTF12_HEIGHT, Language::get(2914));
				ttfPrintTextFormatted(ttf12, tooltip_box.x + 4, tooltip_box.y + 4 + TTF12_HEIGHT * 2, Language::get(2915));
			}

			current_y += 16;
			if ( settings_auto_appraise_new_items )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 36, current_y, "[x] %s", Language::get(1997));
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 36, current_y, "[ ] %s", Language::get(1997));
			}
			current_y += 16;
			if ( settings_disable_messages )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 36, current_y, "[x] %s", Language::get(1536));
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 36, current_y, "[ ] %s", Language::get(1536));
			}
			current_y += 16;
			if ( settings_right_click_protect )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 36, current_y, "[x] %s", Language::get(1998));
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 36, current_y, "[ ] %s", Language::get(1998));
			}
			current_y += 16;
			if ( settings_hotbar_numkey_quick_add )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 36, current_y, "[x] %s", Language::get(2590));
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 36, current_y, "[ ] %s", Language::get(2590));
			}
			current_y += 16;
			if ( settings_lock_right_sidebar )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 36, current_y, "[x] %s", Language::get(2598));
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 36, current_y, "[ ] %s", Language::get(2598));
			}
			current_y += 16;
			if ( settings_show_game_timer_always )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 36, current_y, "[x] %s", Language::get(2983));
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 36, current_y, "[ ] %s", Language::get(2983));
			}
			current_y += 32;

			// server flag elements
			ttfPrintText(ttf12, subx1 + 24, current_y, Language::get(1375));
			current_y += 24;


			int server_flags_start_y = current_y;
			for ( int i = 0; i < NUM_SERVER_FLAGS; i++, current_y += 16 )
			{
				char flagStringBuffer[512] = "";
				if ( i < 5 )
				{
					strncpy(flagStringBuffer, Language::get(153 + i), 255);
				}
				else
				{
					strncpy(flagStringBuffer, Language::get(2917 - 5 + i), 255);
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
						strncpy(flagStringBuffer, Language::get(1942 + i), 255);
					}
					else
					{
						strncpy(flagStringBuffer, Language::get(2921 - 5 + i), 255);
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
							strcat(flagStringBuffer, Language::get(3962)); // changing flags disabled.
							tooltip_box.h += TTF12_HEIGHT;
						}
						tooltip_box.w = longestline(flagStringBuffer) * TTF12_WIDTH + 8; //MORE MAGIC NUMBERS. HNNGH. I can guess what they all do, but dang.
					}
				}
			}

			// network options
			current_y += 32;
			ttfPrintText(ttf12, subx1 + 24, current_y, Language::get(3146));
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
				ttfPrintTextFormatted(ttf12, subx1 + 36, current_y, "[x] %s", Language::get(3147));
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 36, current_y, "[ ] %s", Language::get(3147));
			}
#ifdef USE_EOS
			current_y += 16;
			//ttfPrintTextFormatted(ttf12, subx1 + 36, current_y, "[%c] %s", LobbyHandler.settings_crossplayEnabled ? 'x' : ' ', Language::get(3948));
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
						strncpy(flagStringBuffer, Language::get(1942 + hovering_selection), 255);
					}
					else
					{
						strncpy(flagStringBuffer, Language::get(2921 - 5 + hovering_selection), 255);
					}
					if ( gameModeManager.isServerflagDisabledForCurrentMode(hovering_selection) )
					{
						strcat(flagStringBuffer, Language::get(3962)); // changing flags disabled.
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
					//tooltip_box.w = longestline(Language::get(3148)) * TTF12_WIDTH + 8;
					//tooltip_box.h = TTF12_HEIGHT * 2 + 8;
					//drawTooltip(&tooltip_box);
					//ttfPrintTextFormatted(ttf12, tooltip_box.x + 4, tooltip_box.y + 4, Language::get(3148));
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
					tooltip_box.w = longestline(Language::get(3148)) * TTF12_WIDTH + 8;
					tooltip_box.h = TTF12_HEIGHT * 2 + 8;
					drawTooltip(&tooltip_box);
					ttfPrintTextFormatted(ttf12, tooltip_box.x + 4, tooltip_box.y + 4, Language::get(3148));
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
					/*tooltip_box.w = longestline(Language::get(3148)) * TTF12_WIDTH + 8;
					tooltip_box.h = TTF12_HEIGHT * 2 + 8;
					drawTooltip(&tooltip_box);
					ttfPrintTextFormatted(ttf12, tooltip_box.x + 4, tooltip_box.y + 4, Language::get(3148));*/
					if ( inputs.bMouseLeft(clientnum) )
					{
						inputs.mouseClearLeft(clientnum);
						//LobbyHandler.settings_crossplayEnabled = !LobbyHandler.settings_crossplayEnabled;
					}
				}
#endif
#endif // STEAMWORKS


				current_y = options_start_y;

				if ( omousey >= current_y && omousey < current_y + 12 ) // ip broadcast
				{
					tooltip_box.w = longestline(Language::get(3149)) * TTF12_WIDTH + 8;
					tooltip_box.h = TTF12_HEIGHT + 8;
					drawTooltip(&tooltip_box);
					ttfPrintTextFormatted(ttf12, tooltip_box.x + 4, tooltip_box.y + 4, Language::get(3149));
				}
				else if ( omousey >= (current_y += 16) && omousey < current_y + 12 ) // no hud
				{
					tooltip_box.w = longestline(Language::get(3150)) * TTF12_WIDTH + 8;
					tooltip_box.h = TTF12_HEIGHT + 8;
					drawTooltip(&tooltip_box);
					ttfPrintTextFormatted(ttf12, tooltip_box.x + 4, tooltip_box.y + 4, Language::get(3150));
				}
				else if ( omousey >= (current_y += 16) && omousey < current_y + 12 ) // auto add to hotbar
				{
					tooltip_box.w = longestline(Language::get(3151)) * TTF12_WIDTH + 8;
					tooltip_box.h = TTF12_HEIGHT * 2 + 8;
					drawTooltip(&tooltip_box);
					ttfPrintTextFormatted(ttf12, tooltip_box.x + 4, tooltip_box.y + 4, Language::get(3151));
				}
				else if ( omousey >= (current_y += 16) && omousey < current_y + 12 ) // auto appraisal
				{
					tooltip_box.w = longestline(Language::get(3152)) * TTF12_WIDTH + 8;
					tooltip_box.h = TTF12_HEIGHT * 2 + 8;
					drawTooltip(&tooltip_box);
					ttfPrintTextFormatted(ttf12, tooltip_box.x + 4, tooltip_box.y + 4, Language::get(3152));
				}
				else if ( omousey >= (current_y += 16) && omousey < current_y + 12 ) // no messages
				{
					tooltip_box.w = longestline(Language::get(3153)) * TTF12_WIDTH + 8;
					tooltip_box.h = TTF12_HEIGHT * 2 + 8;
					drawTooltip(&tooltip_box);
					ttfPrintTextFormatted(ttf12, tooltip_box.x + 4, tooltip_box.y + 4, Language::get(3153));
				}
				else if ( omousey >= (current_y += 16) && omousey < current_y + 12 ) // right click protect
				{
					tooltip_box.w = longestline(Language::get(3154)) * TTF12_WIDTH + 8;
					tooltip_box.h = TTF12_HEIGHT * 2 + 8;
					drawTooltip(&tooltip_box);
					ttfPrintTextFormatted(ttf12, tooltip_box.x + 4, tooltip_box.y + 4, Language::get(3154));
				}
				else if ( omousey >= (current_y += 16) && omousey < current_y + 12 ) // numkey hotbar
				{
					tooltip_box.w = longestline(Language::get(3155)) * TTF12_WIDTH + 8;
					tooltip_box.h = TTF12_HEIGHT * 3 + 8;
					drawTooltip(&tooltip_box);
					ttfPrintTextFormatted(ttf12, tooltip_box.x + 4, tooltip_box.y + 4, Language::get(3155));
				}
				else if ( omousey >= (current_y += 16) && omousey < current_y + 12 ) // lock ride sidebar
				{
					tooltip_box.w = longestline(Language::get(3156)) * TTF12_WIDTH + 8;
					tooltip_box.h = TTF12_HEIGHT * 3 + 8;
					drawTooltip(&tooltip_box);
					ttfPrintTextFormatted(ttf12, tooltip_box.x + 4, tooltip_box.y + 4, Language::get(3156));
				}
				else if ( omousey >= (current_y += 16) && omousey < current_y + 12 ) // show timer always
				{
					tooltip_box.w = longestline(Language::get(3157)) * TTF12_WIDTH + 8;
					tooltip_box.h = TTF12_HEIGHT * 3 + 8;
					drawTooltip(&tooltip_box);
					ttfPrintTextFormatted(ttf12, tooltip_box.x + 4, tooltip_box.y + 4, Language::get(3157));
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
					drawRect(&bodyHighlight, uint32ColorBaronyBlue, 64);
					drawRect(&iconHighlight, makeColorRGB(1, 0, 16), 255);
				}
				else
				{
					drawRect(&bodyHighlight, makeColorRGB(128, 128, 128), 64);
					drawRect(&iconHighlight, makeColorRGB(36, 36, 36), 255);
				}

				// draw name
				Uint32 nameColor = unlocked ? makeColorRGB(0, 255, 255) : makeColorRGB(128, 128, 128);
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
						// ttfPrintTextColor(ttf12, subx2 - 330, suby1 + 92 + (index - first_ach) * 80, uint32ColorWhite, true, percent_str);

						SDL_Rect progressbar;
						progressbar.x = subx2 - 330 + (4 * TTF12_WIDTH) + TTF12_WIDTH;
						progressbar.y = suby1 + 92 + (index - first_ach) * 80 - 4;
						progressbar.h = TTF12_HEIGHT + 2;
						progressbar.w = (bodyBox.x + bodyBox.w) - progressbar.x - 4;
						drawWindowFancy(progressbar.x - 2, progressbar.y - 2, progressbar.x + progressbar.w + 2, progressbar.y + progressbar.h + 2);

						drawRect(&progressbar, makeColorRGB(36, 36, 36), 255);
						progressbar.w = std::min((bodyBox.x + bodyBox.w) - progressbar.x - 4, static_cast<int>(progressbar.w * percent / 100.0));
						drawRect(&progressbar, uint32ColorBaronyBlue, 92);
						progressbar.w = (bodyBox.x + bodyBox.w) - progressbar.x - TTF12_WIDTH;

						char progress_str[32] = { 0 };
						snprintf(progress_str, sizeof(progress_str), "%d / %d", currentValue, maxValue);
						ttfPrintTextColor(ttf12, progressbar.x + progressbar.w / 2 - (strlen(progress_str) * TTF12_WIDTH) / 2,
							suby1 + 92 + (index - first_ach) * 80, uint32ColorWhite, true, progress_str);
					}
				}

				// draw unlock time
				if ( unlocked )
				{
					// deprecated
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
			ttfPrintText(ttf12, subx1 + 8, suby1 + 100, Language::get(709));
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

			std::string raceAndClass = Language::get(3161 + stats[c)->playerRace];
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
				ttfPrintTextFormatted(ttf12, subx1 + 8, suby1 + 80 + 60 * c, "%d:  %s\n    %s\n    %s", c + 1, charDisplayName.c_str(), Language::get(1322), raceAndClass.c_str());
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 8, suby1 + 80 + 60 * c, "%d:  %s\n    %s\n    %s", c + 1, charDisplayName.c_str(), Language::get(1321), raceAndClass.c_str());
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
			if ( keystatus[SDLK_TAB] )
			{
				keystatus[SDLK_TAB] = 0;
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
				strncpy(flagStringBuffer, Language::get(153 + i), 255);
			}
			else
			{
				strncpy(flagStringBuffer, Language::get(2917 - 5 + i), 255);
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
					strncpy(flagStringBuffer, Language::get(1942 + i), 255);
				}
				else
				{
					strncpy(flagStringBuffer, Language::get(2921 - 5 + i), 255);
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
						ttfPrintTextFormatted(ttf12, xres / 2 + 8, suby1 + 256 + 16 * i, "[o] %s", Language::get(250 + i));
					}
					else
					{
						ttfPrintTextFormatted(ttf12, xres / 2 + 8, suby1 + 256 + 16 * i, "[ ] %s", Language::get(250 + i));
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
					ttfPrintTextFormatted(ttf12, xres / 2 + 8, suby1 + 256 + 16 * i, "[o] %s", Language::get(250 + i));
				}
				else
				{
					ttfPrintTextFormatted(ttf12, xres / 2 + 8, suby1 + 256 + 16 * i, "[ ] %s", Language::get(250 + i));
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
		if ( keystatus[SDLK_RETURN] && strlen(lobbyChatbox) > 0 )
		{
			keystatus[SDLK_RETURN] = 0;
			if ( multiplayer != CLIENT )
			{
				playSound(Message::CHAT_MESSAGE_SFX, 64);
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
				//newString(&lobbyChatboxMessages, 0xFFFFFFFF, msg);  // servers print their messages right away
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
							snprintf(kickMsg, LOBBY_CHATBOX_LENGTH, Language::get(279), playerToKick + 1, clientShortName);
							//newString(&lobbyChatboxMessages, 0xFFFFFFFF, kickMsg);  // servers print their messages right away

							strcpy(msg, kickMsg);
						}
						else
						{
							//newString(&lobbyChatboxMessages, 0xFFFFFFFF, "***   Invalid player to kick   ***");
							skipMessageRelayToClients = true;
							playerToKick = -1;
						}
					}
					else
					{
						//newString(&lobbyChatboxMessages, 0xFFFFFFFF, "***   Invalid player to kick   ***");
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
						strncpy((char*)(net_packet->data), "DISC", 4);
						net_packet->data[4] = playerToKick;
						net_packet->address.host = net_clients[i - 1].host;
						net_packet->address.port = net_clients[i - 1].port;
						net_packet->len = 5;
						sendPacketSafe(net_sock, -1, net_packet, i - 1);
					}
					char shortname[32] = { 0 };
					strncpy(shortname, stats[playerToKick]->name, 22);
					//newString(&lobbyChatboxMessages, 0xFFFFFFFF, Language::get(1376), shortname);\
					// TODO print a message about it in lobby chat box
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
				strncpy(flagStringBuffer, Language::get(1942 + hovering_selection), 255);
			}
			else
			{
				strncpy(flagStringBuffer, Language::get(2921 - 5 + hovering_selection), 255);
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
				modList.append(Language::get(2984)).append("' and \n'").append(Language::get(2985));
				modList.append("' to automatically subscribe and \n");
				modList.append("mount workshop items loaded in the host's lobby.\n\n");
				modList.append("All clients should be running the same mod load order\n");
				modList.append("to prevent any crashes or undefined behavior.\n\n");
				modList.append("Game client may need to be closed to install and detect\nnew subscriptions due to Workshop limitations.\n");
				int numToolboxLines = 9;
				bool itemNeedsSubscribing = false;
				bool itemNeedsMounting = false;
				Uint32 modsStatusColor = uint32ColorBaronyBlue;
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
						modsStatusColor = uint32ColorOrange;
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
						modsStatusColor = uint32ColorGreen;
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
						modsStatusColor = uint32ColorOrange;
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
						ttfPrintTextFormattedColor(ttf12, subx2 - 64 * TTF12_WIDTH, suby2 - 4 - TTF12_HEIGHT, uint32ColorOrange, 
							"retrieving data...");
					}
					else if ( g_SteamWorkshop->subscribedCallStatus == 2 )
					{
						ttfPrintTextFormattedColor(ttf12, subx2 - 64 * TTF12_WIDTH, suby2 - 4 - TTF12_HEIGHT, uint32ColorOrange,
							"please retry mount operation.");
					}
					else
					{
						ttfPrintTextFormattedColor(ttf12, subx2 - 64 * TTF12_WIDTH, suby2 - 4 - TTF12_HEIGHT, uint32ColorOrange,
							"press mount button.");
					}
				}
			}
		}
#endif // STEAMWORKS
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

		ttfPrintTextFormattedColor(ttf16, filename_padx, filename_pady, uint32ColorWhite, "%s", 
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
			ttfPrintTextFormattedColor(ttf12, filename_padx, filename_pady + 2 * TTF12_HEIGHT, uint32ColorOrange, "Downloading entries...");
		}
		else 
		{

			if ( g_SteamLeaderboards->b_ScoresDownloaded )
			{
				numEntriesTotal = g_SteamLeaderboards->m_nLeaderboardEntries;
				if ( numEntriesTotal <= 0 )
				{
					ttfPrintTextFormattedColor(ttf12, filename_padx, filename_pady + 2 * TTF12_HEIGHT, uint32ColorGreen, "No Leaderboard entries for this category");
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
				drawRect(&slider, makeColorRGB(64, 64, 64), 255);
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

				if ( keystatus[SDLK_UP] )
				{
					g_SteamLeaderboards->LeaderboardView.scrollIndex = std::max(g_SteamLeaderboards->LeaderboardView.scrollIndex - 1, 0);
					keystatus[SDLK_UP] = 0;
				}
				if ( keystatus[SDLK_DOWN] )
				{
					g_SteamLeaderboards->LeaderboardView.scrollIndex = std::min(g_SteamLeaderboards->LeaderboardView.scrollIndex + 1, entriesToScroll);
					keystatus[SDLK_DOWN] = 0;
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
				//drawRect(&slider, makeColorRGB(64, 64, 64), 255);
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
					drawRect(&highlightEntry, uint32ColorBaronyBlue, 64);

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
#define NOSCORESSTR Language::get(1389)
			ttfPrintTextFormatted(ttf16, xres / 2 - strlen(NOSCORESSTR) * 9, yres / 2 - 9, NOSCORESSTR);
		}
		else
		{
			if ( !score_leaderboard_window )
			{
				if ( scoreDisplayMultiplayer )
				{
					ttfPrintTextFormatted(ttf16, subx1 + 8, suby1 + 8, "%s - %d / %d", Language::get(2958), score_window, list_Size(&topscoresMultiplayer));
				}
				else
				{
					ttfPrintTextFormatted(ttf16, subx1 + 8, suby1 + 8, "%s - %d / %d", Language::get(1390), score_window, list_Size(&topscores));
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
					//glDrawVoxel(&camera_charsheet, players[clientnum]->entity, REALCOLORS);
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
						//glDrawVoxel(&camera_charsheet, entity, REALCOLORS);
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
				ttfPrintTextFormatted(ttf16, subx1 + 448, suby1 + 40, Language::get(1391));
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
						ttfPrintTextFormatted(ttf16, subx1 + 448, suby1 + 72, Language::get(3359), monstertypenamecapitalized[creature]);
					}
					else
					{
						ttfPrintTextFormatted(ttf16, subx1 + 448, suby1 + 72, Language::get(1392));
					}
				}
				else if ( victory == 2 )
				{
					if ( creature != HUMAN )
					{
						ttfPrintTextFormatted(ttf16, subx1 + 448, suby1 + 72, Language::get(3360), monstertypenamecapitalized[creature]);
					}
					else
					{
						ttfPrintTextFormatted(ttf16, subx1 + 448, suby1 + 72, Language::get(1393));
					}
				}
				else if ( victory == 3 || victory == 4 || victory == 5 )
				{
					if ( creature != HUMAN )
					{
						ttfPrintTextFormatted(ttf16, subx1 + 448, suby1 + 72, Language::get(3361), 
							Language::get(3363 - 1 + stats[clientnum)->playerRace]);
					}
					else
					{
						ttfPrintTextFormatted(ttf16, subx1 + 448, suby1 + 72, Language::get(2911));
					}
				}
			}
			else
			{
				ttfPrintTextFormatted(ttf16, subx1 + 448, suby1 + 40, Language::get(1394));
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
					ttfPrintTextFormatted(ttf16, subx1 + 448, suby1 + 72, Language::get(3362), monstertypenamecapitalized[creature], classname);
				}
				else
				{
					ttfPrintTextFormatted(ttf16, subx1 + 448, suby1 + 72, Language::get(1395), classname);
				}
			}

			// print total score
			if ( score_leaderboard_window != 3 )
			{
				node = list_Node(scoresPtr, score_window - 1);
				if ( node )
				{
					score_t* score = (score_t*)node->element;
					ttfPrintTextFormatted(ttf16, subx1 + 448, suby1 + 104, Language::get(1404), totalScore(score));
				}
			}
			else
			{
#ifdef STEAMWORKS
				ttfPrintTextFormatted(ttf16, subx1 + 448, suby1 + 104, Language::get(1404), g_SteamLeaderboards->downloadedTags[g_SteamLeaderboards->currentLeaderBoardIndex][TAG_TOTAL_SCORE]);
#endif // STEAMWORKS
			}

			Entity* playerEntity = nullptr;
			if ( players[clientnum] )
			{
				playerEntity = players[clientnum]->entity;
			}

			// print character stats
			ttfPrintTextFormatted(ttf12, subx1 + 456, suby1 + 128, Language::get(359), stats[clientnum]->LVL, playerClassLangEntry(client_classes[clientnum], clientnum));
			ttfPrintTextFormatted(ttf12, subx1 + 456, suby1 + 140, Language::get(1396), stats[clientnum]->EXP);
			ttfPrintTextFormatted(ttf12, subx1 + 456, suby1 + 152, Language::get(1397), stats[clientnum]->GOLD);
			ttfPrintTextFormatted(ttf12, subx1 + 456, suby1 + 164, Language::get(361), currentlevel);

			ttfPrintTextFormatted(ttf12, subx1 + 456, suby1 + 188, Language::get(1398), statGetSTR(stats[clientnum], playerEntity), stats[clientnum]->STR);
			ttfPrintTextFormatted(ttf12, subx1 + 456, suby1 + 200, Language::get(1399), statGetDEX(stats[clientnum], playerEntity), stats[clientnum]->DEX);
			ttfPrintTextFormatted(ttf12, subx1 + 456, suby1 + 212, Language::get(1400), statGetCON(stats[clientnum], playerEntity), stats[clientnum]->CON);
			ttfPrintTextFormatted(ttf12, subx1 + 456, suby1 + 224, Language::get(1401), statGetINT(stats[clientnum], playerEntity), stats[clientnum]->INT);
			ttfPrintTextFormatted(ttf12, subx1 + 456, suby1 + 236, Language::get(1402), statGetPER(stats[clientnum], playerEntity), stats[clientnum]->PER);
			ttfPrintTextFormatted(ttf12, subx1 + 456, suby1 + 248, Language::get(1403), statGetCHR(stats[clientnum], playerEntity), stats[clientnum]->CHR);

			// time
			Uint32 sec = (completionTime / TICKS_PER_SECOND) % 60;
			Uint32 min = ((completionTime / TICKS_PER_SECOND) / 60) % 60;
			Uint32 hour = ((completionTime / TICKS_PER_SECOND) / 60) / 60;
			ttfPrintTextFormatted(ttf12, subx1 + 32, suby2 - 80, "%s: %02d:%02d:%02d. %s:", Language::get(1405), hour, min, sec, Language::get(1406));
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
				ttfPrintText(ttf12, subx1 + 32, suby2 - 64, Language::get(1407));
			}
			else
			{
				int b = 0;
				strcpy(tempstr, " ");
				if ( conductPenniless )
				{
					strcat(tempstr, Language::get(1408));
					++b;
				}
				if ( conductFoodless )
				{
					if ( b > 0 )
					{
						strcat(tempstr, ", ");
					}
					strcat(tempstr, Language::get(1409));
					++b;
				}
				if ( conductVegetarian )
				{
					if ( b > 0 )
					{
						strcat(tempstr, ", ");
					}
					strcat(tempstr, Language::get(1410));
					++b;
				}
				if ( conductIlliterate )
				{
					if ( b > 0 )
					{
						strcat(tempstr, ", ");
					}
					strcat(tempstr, Language::get(1411));
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
						strcat(tempstr, Language::get(2925 + c));
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
			ttfPrintText(ttf12, subx1 + 456, suby1 + 272, Language::get(1412));
			bool nokills = true;
			for ( x = 0; x < NUMMONSTERS; x++ )
			{
				if ( kills[x] )
				{
					nokills = false;
					if ( kills[x] > 1 )
					{
						ttfPrintTextFormatted(ttf12, subx1 + 456 + (y / 14) * 156, suby1 + 296 + (y % 14) * 12, "%d %s", 
							kills[x], getMonsterLocalizedPlural((Monster)x).c_str());
					}
					else
					{
						ttfPrintTextFormatted(ttf12, subx1 + 456 + (y / 14) * 156, suby1 + 296 + (y % 14) * 12, "%d %s", 
							kills[x], getMonsterLocalizedName((Monster)x).c_str());
					}
					y++;
				}
			}
			if ( nokills )
			{
				ttfPrintText(ttf12, subx1 + 456, suby1 + 296, Language::get(1413));
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
			//ttfPrintTextFormattedColor(ttf12, filename_padx, filename_pady, uint32ColorGreen, Language::get(3066));
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
			drawRect(&slider, makeColorRGB(64, 64, 64), 255);
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

			if ( keystatus[SDLK_UP] )
			{
				savegames_window_scroll = std::max(savegames_window_scroll - 1, 0);
				keystatus[SDLK_UP] = 0;
			}
			if ( keystatus[SDLK_DOWN] )
			{
				savegames_window_scroll = std::min(savegames_window_scroll + 1, entriesToScroll);
				keystatus[SDLK_DOWN] = 0;
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
			//drawRect(&slider, makeColorRGB(64, 64, 64), 255);
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
						drawRect(&highlightEntry, uint32ColorGreen, 64);
					}
					else
					{
						drawRect(&highlightEntry, uint32ColorGreen, 32);
					}
				}
				else
				{
					if ( std::get<1>(entry) == SINGLE ) // single player.
					{
						drawRect(&highlightEntry, makeColorRGB(128, 128, 128), 48);
						//drawRect(&highlightEntry, uint32ColorBaronyBlue, 16);
					}
					else
					{
						drawRect(&highlightEntry, uint32ColorBaronyBlue, 32);
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
				if ( drawClickableButton(filename_padx, filename_pady, 2 * TTF12_WIDTH + 8, TTF12_HEIGHT * 2 + 4, uint32ColorRed) )
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

		Uint32 saveNumColor = uint32ColorGreen;
		if ( numSingleplayerSaves == SAVE_GAMES_MAX )
		{
			saveNumColor = uint32ColorOrange;
		}
		ttfPrintTextFormattedColor(ttf12, subx2 - (longestline(Language::get(3067)) * TTF12_WIDTH), suby1 + 44, saveNumColor,
			Language::get(3067), numSingleplayerSaves, SAVE_GAMES_MAX);

		saveNumColor = uint32ColorGreen;
		if ( numMultiplayerSaves == SAVE_GAMES_MAX )
		{
			saveNumColor = uint32ColorOrange;
		}
		ttfPrintTextFormattedColor(ttf12, subx2 - (longestline(Language::get(3068)) * TTF12_WIDTH), suby1 + 44 + TTF12_HEIGHT + 4, saveNumColor,
			Language::get(3068), numMultiplayerSaves, SAVE_GAMES_MAX);

		// draw the tooltip we initialised earlier.
		if ( drawDeleteTooltip )
		{
			tooltip.w = longestline(Language::get(3064)) * TTF12_WIDTH + 16;
			drawTooltip(&tooltip);
			ttfPrintTextFormatted(ttf12, tooltip.x + 6, tooltip.y + 6, Language::get(3064));
		}
		else if ( drawScrollTooltip )
		{
			tooltip.w = longestline(Language::get(3066)) * TTF12_WIDTH + 16;
			drawTooltip(&tooltip);
			ttfPrintTextFormatted(ttf12, tooltip.x + 6, tooltip.y + 6, Language::get(3066));
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
					drawRect(&pos, makeColorRGB(64, 64, 64), 255);
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
							// causes a warning so this code is disabled for now
							//case 0:
							//	ttfPrintTextFormatted(ttf12, status_padx, status_pady, "creating item...");
							//	break;
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
										ttfPrintTextFormattedColor(ttf12, status_padx, status_pady, uint32ColorOrange, "item created! awaiting file handle...");
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
										ttfPrintTextFormattedColor(ttf12, status_padx, status_pady, uint32ColorGreen, "item and file handle create success!");
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
											ttfPrintTextColor(ttf12, status_padx + 20 * TTF12_WIDTH, status_pady - TTF12_HEIGHT, uint32ColorGreen, true, "success set");
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
										ttfPrintTextColor(ttf12, status_padx + 20 * TTF12_WIDTH, status_pady - TTF12_HEIGHT, uint32ColorGreen, true, "success set");
									}
									else
									{
										ttfPrintTextColor(ttf12, status_padx + 20 * TTF12_WIDTH, status_pady - TTF12_HEIGHT, uint32ColorRed, true, "error!");
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
									ttfPrintTextColor(ttf12, status_padx, status_pady, uint32ColorGreen, true, "folder path success set");
								}
								else
								{
									ttfPrintTextColor(ttf12, status_padx, status_pady, uint32ColorRed, true, "error in folder path!");
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
					ttfPrintTextFormattedColor(ttf12, status_padx + 8, status_pady, uint32ColorBaronyBlue, "Title:");
					status_pady += TTF12_HEIGHT;
					ttfPrintTextFormatted(ttf12, status_padx + 8, status_pady, "%s", line.c_str());

					line = g_SteamWorkshop->m_myWorkshopItemToModify.m_rgchDescription;
					if ( line.size() > filenameMaxLength )
					{
						line = line.substr(0, filenameMaxLength - 2);
						line.append("..");
					}
					status_pady += TTF12_HEIGHT;
					ttfPrintTextFormattedColor(ttf12, status_padx + 8, status_pady, uint32ColorBaronyBlue, "Description:");
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
						ttfPrintTextFormattedColor(ttf12, status_padx, status_pady, uint32ColorGreen, "Only Workshop tags will be updated.");
					}
					else
					{
						ttfPrintTextFormattedColor(ttf12, status_padx, status_pady, uint32ColorOrange, "Workshop file contents will be updated.");
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
							ttfPrintTextFormattedColor(ttf12, status_padx, status_pady, uint32ColorGreen, "successfully uploaded!");
							ttfPrintTextFormattedColor(ttf12, status_padx, status_pady + TTF12_HEIGHT, uint32ColorGreen, "reloading window in %d...!", 5 - ((ticks - g_SteamWorkshop->uploadSuccessTicks) / TICKS_PER_SECOND));
						}
						else
						{
							ttfPrintTextFormattedColor(ttf12, status_padx, status_pady, uint32ColorOrange, "error! %d", g_SteamWorkshop->SubmitItemUpdateResult.m_eResult);
							ttfPrintTextFormattedColor(ttf12, status_padx, status_pady + TTF12_HEIGHT, uint32ColorOrange, "close the window and try again.");
						}
					}
					else
					{
						ttfPrintTextFormattedColor(ttf12, status_padx, status_pady, uint32ColorOrange, "uploading... status %d", status);
						ttfPrintTextFormattedColor(ttf12, status_padx, status_pady + TTF12_HEIGHT, uint32ColorOrange, "bytes processed: %d", bytesProc);
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
					ttfPrintTextFormattedColor(ttf12, filename_padx, filename_pady, uint32ColorGreen, "successfully retrieved subscribed items!");
				}
				else
				{
					ttfPrintTextFormattedColor(ttf12, filename_padx, filename_pady, uint32ColorGreen, "successfully retrieved my workshop items!");
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
					drawRect(&slider, makeColorRGB(64, 64, 64), 255);
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
				
					if ( keystatus[SDLK_UP] )
					{
						gamemods_window_scroll = std::max(gamemods_window_scroll - 1, 0);
						keystatus[SDLK_UP] = 0;
					}
					if ( keystatus[SDLK_DOWN] )
					{
						gamemods_window_scroll = std::min(gamemods_window_scroll + 1, entriesToScroll);
						keystatus[SDLK_DOWN] = 0;
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
					ttfPrintTextFormattedColor(ttf12, filename_padx + 8, suby2 - TTF12_HEIGHT - 4, uint32ColorOrange, "%s returned status %d", 
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
						drawRect(&highlightEntry, makeColorRGB(128, 128, 128), 64);

						bool itemDownloaded = SteamUGC()->GetItemInstallInfo(itemDetails.m_nPublishedFileId, NULL, fullpath, PATH_MAX, NULL);
						bool pathIsMounted = gamemodsIsPathInMountedFiles(fullpath);

						if ( pathIsMounted && gamemods_window == 3 )
						{
							SDL_Rect pos;
							pos.x = filename_padx + 2;
							pos.y = filename_pady - 6;
							pos.w = filename_padx2 - filename_padx - 4;
							pos.h = filename_rowHeight + 4;
							drawRect(&pos, uint32ColorGreen, 64);
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
								if ( gamemodsDrawClickableButton(filename_padx, filename_pady, 12 * TTF12_WIDTH + 8, TTF12_HEIGHT, uint32ColorBaronyBlue, " Download ", 0) )
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
							if ( gamemodsDrawClickableButton(filename_padx, filename_pady + filename_rowHeight / 4, 12 * TTF12_WIDTH + 8, TTF12_HEIGHT, uint32ColorBaronyBlue, "  Update  ", 0) )
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
							if ( gamemodsDrawClickableButton(filename_padx, filename_pady, 12 * TTF12_WIDTH + 8, TTF12_HEIGHT, uint32ColorRed, "Unsubscribe", 0) )
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
					ttfPrintTextFormattedColor(ttf12, tooltip.x + 4, tooltip.y + 4, uint32ColorBaronyBlue, 
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
					ttfPrintTextFormattedColor(ttf12, tooltip.x + 8, tooltip.y + tooltip_pady, uint32ColorBaronyBlue, "%s", line.c_str());
					tooltip_pady += TTF12_HEIGHT * 2;

					// draw description body.
					ttfPrintTextFormatted(ttf12, tooltip.x + 8, tooltip.y + tooltip_pady, "  %s", outputStr.c_str());

					tooltip_pady += TTF12_HEIGHT * (numlines + 2);
					
					// draw tags.
					ttfPrintTextFormattedColor(ttf12, tooltip.x + 8, tooltip.y + tooltip_pady, uint32ColorBaronyBlue, "tags:");
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
					ttfPrintTextFormattedColor(ttf12, filename_padx, filename_pady, uint32ColorRed, "Error: could not create directory %s/, already exists in mods/ folder", gamemods_newBlankDirectory);
				}
				else if ( gamemods_newBlankDirectoryStatus == 1 )
				{
					ttfPrintTextFormattedColor(ttf12, filename_padx, filename_pady, uint32ColorGreen, "Successfully created directory %s/ in mods/ folder", gamemods_newBlankDirectory);
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
				ttfPrintTextFormattedColor(ttf12, filename_padx, filename_pady, uint32ColorGreen, "successfully retrieved local items!");
			}
			else
			{
				ttfPrintTextFormattedColor(ttf12, filename_padx, filename_pady, uint32ColorOrange, "no folders found!");
				ttfPrintTextFormattedColor(ttf12, filename_padx, filename_pady + TTF12_HEIGHT + 8, uint32ColorOrange, "to get started create a new folder, or copy shared custom content to the mods/ folder");
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
				drawRect(&slider, makeColorRGB(64, 64, 64), 255);
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

				if ( keystatus[SDLK_UP] )
				{
					gamemods_window_scroll = std::max(gamemods_window_scroll - 1, 0);
					keystatus[SDLK_UP] = 0;
				}
				if ( keystatus[SDLK_DOWN] )
				{
					gamemods_window_scroll = std::min(gamemods_window_scroll + 1, entriesToScroll);
					keystatus[SDLK_DOWN] = 0;
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
				drawRect(&highlightEntry, makeColorRGB(128, 128, 128), 64);

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
					drawRect(&pos, uint32ColorGreen, 64);
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
				ttfPrintTextFormattedColor(ttf12, tooltip.x + 4, tooltip.y + 4, uint32ColorBaronyBlue,
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
		ttfPrintTextFormattedColor(ttf12, filename_padx + 8, filename_pady + 8, uint32ColorWhite, "%s", menu.windowTitle.c_str());

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
			drawRect(&slider, makeColorRGB(64, 64, 64), 255);
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

			if ( keystatus[SDLK_UP] )
			{
				menu.windowScroll = std::max(menu.windowScroll - 1, 0);
				keystatus[SDLK_UP] = 0;
			}
			if ( keystatus[SDLK_DOWN] )
			{
				menu.windowScroll = std::min(menu.windowScroll + 1, entriesToScroll);
				keystatus[SDLK_DOWN] = 0;
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
			drawRect(&highlightEntry, makeColorRGB(128, 128, 128), 64);

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
				drawRect(&pos, uint32ColorBaronyBlue, 64);
				ttfPrintTextFormattedColor(ttf12, filename_padx + 8, suby2 - 3 * TTF12_HEIGHT, uint32ColorYellow, "%s", (*it).description.c_str());
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
			ttfPrintTextFormattedColor(ttf12, subx1 + 16 + 8, suby2 - 3 * TTF12_HEIGHT, uint32ColorYellow, "%s", menu.defaultHoverText.c_str());
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
			doQuitGame();
		}
		else if ( introstage == 3 )     // new game
		{
			doNewGame(!mode);
		}
		else if ( introstage == 4 )     // credits
		{
			doCredits();
		}
		else if ( introstage == 5 )     // end game
		{
			doEndgame(true);
		}
		else if ( introstage == 6 )     // introduction cutscene
		{
			doIntro();
		}
		else if ( introstage == 7 )     // win game sequence (herx)
		{
			doEndgameHerx();
		}
		else if ( introstage == 8 )     // win game sequence (devil)
		{
			doEndgameDevil();
		}
		else if ( introstage == 9 )     // mid game sequence
		{
			doMidgame();
		}
		else if ( introstage == 10 )     // expansion end game sequence
		{
			doEndgameCitadel();
		}
		else if ( introstage >= 11 && introstage <= 15 )     // new mid and classic end sequences.
		{
			doEndgameClassicAndExtraMidGame();
		}
		else if ( introstage >= 16 && introstage <= 18 )     // expansion end game sequence DLC
		{
			doEndgameExpansion();
		}
	}

	// credits sequence
	//if ( creditstage > 0 )
	//{
	//	if ( (credittime >= 300 && (creditstage <= 11 || creditstage > 13)) || (credittime >= 180 && creditstage == 12) ||
	//	        (credittime >= 480 && creditstage == 13) || inputs.bMouseLeft(clientnum) || (inputs.bControllerInputPressed(clientnum, INJOY_MENU_NEXT) && rebindaction == -1) )
	//	{
	//		inputs.mouseClearLeft(clientnum);
	//		if ( rebindaction == -1 )
	//		{
	//			inputs.controllerClearInput(clientnum, INJOY_MENU_NEXT);
	//		}
	//		introstage = 4;
	//		fadeout = true;
	//	}

	//	// stages
	//	Uint32 colorBlue = makeColor( 0, 92, 255, 255);
	//	if ( creditstage == 1 )
	//	{
	//		ttfPrintTextFormattedColor(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(Language::get(56)), yres / 2 - 9 - 18, colorBlue, Language::get(56));
	//		ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE02), yres / 2 - 9 + 18, CREDITSLINE02);
	//	}
	//	else if ( creditstage == 2 )
	//	{
	//		ttfPrintTextFormattedColor(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(Language::get(57)), yres / 2 - 9 - 18, colorBlue, Language::get(57));
	//		ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE04), yres / 2 - 9 + 18, CREDITSLINE04);
	//	}
	//	else if ( creditstage == 3 )
	//	{
	//		ttfPrintTextFormattedColor(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(Language::get(58)), yres / 2 - 9 - 18, colorBlue, Language::get(58));
	//		ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE06), yres / 2 - 9, CREDITSLINE06);
	//		ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE40), yres / 2 - 9 + 18, CREDITSLINE40);
	//	}
	//	else if ( creditstage == 4 )
	//	{
	//		ttfPrintTextFormattedColor(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(Language::get(59)), yres / 2 - 9 - 18, colorBlue, Language::get(59));
	//		ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE39), yres / 2 + 9, CREDITSLINE39);
	//	}
	//	else if ( creditstage == 5 )
	//	{
	//		ttfPrintTextFormattedColor(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(Language::get(60)), yres / 2 - 9 - 18, colorBlue, Language::get(60));
	//		ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE11), yres / 2 - 9, CREDITSLINE11);
	//		ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE08), yres / 2 - 9 + 18, CREDITSLINE08);
	//		ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE09), yres / 2 + 9 + 18 * 1, CREDITSLINE09);
	//	}
	//	else if ( creditstage == 6 )
	//	{
	//		ttfPrintTextFormattedColor(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(Language::get(61)), yres / 2 - 9 - 18, colorBlue, Language::get(61));
	//		ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE13), yres / 2 - 9 + 18, CREDITSLINE13);
	//	}
	//	else if ( creditstage == 7 )
	//	{
	//		ttfPrintTextFormattedColor(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(Language::get(62)), yres / 2 - 9 - 18 * 4, colorBlue, Language::get(62));
	//		ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE15), yres / 2 - 9 - 18 * 2, CREDITSLINE15);
	//		ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE16), yres / 2 - 9 - 18 * 1, CREDITSLINE16);
	//		ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE17), yres / 2 - 9, CREDITSLINE17);
	//		ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE18), yres / 2 + 9, CREDITSLINE18);
	//		ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE19), yres / 2 + 9 + 18 * 1, CREDITSLINE19);
	//		ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE20), yres / 2 + 9 + 18 * 2, CREDITSLINE20);
	//		ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE21), yres / 2 + 9 + 18 * 3, CREDITSLINE21);
	//	}
	//	else if ( creditstage == 8 )
	//	{
	//		ttfPrintTextFormattedColor(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(Language::get(63)), yres / 2 - 9 - 18 * 4, colorBlue, Language::get(63));
	//		ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE23), yres / 2 - 9 - 18 * 2, CREDITSLINE23);
	//		ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE24), yres / 2 - 9 - 18 * 1, CREDITSLINE24);
	//		ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE25), yres / 2 - 9, CREDITSLINE25);
	//		ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE26), yres / 2 + 9, CREDITSLINE26);
	//		ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE27), yres / 2 + 9 + 18 * 1, CREDITSLINE27);
	//		ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE28), yres / 2 + 9 + 18 * 2, CREDITSLINE28);
	//		ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE29), yres / 2 + 9 + 18 * 3, CREDITSLINE29);
	//		ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE30), yres / 2 + 9 + 18 * 4, CREDITSLINE30);
	//	}
	//	else if ( creditstage == 9 )
	//	{
	//		ttfPrintTextFormattedColor(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(Language::get(2585)), yres / 2 - 9 - 18, colorBlue, Language::get(2585));
	//		ttfPrintTextFormattedColor(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(Language::get(2586)), yres / 2 - 9, colorBlue, Language::get(2586));
	//		ttfPrintTextFormattedColor(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(Language::get(2587)), yres / 2 - 9 + 18, colorBlue, Language::get(2587));
	//		ttfPrintTextFormattedColor(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(Language::get(2588)), yres / 2 + 9 + 18, colorBlue, Language::get(2588));
	//	}
	//	else if ( creditstage == 10 )
	//	{
	//		ttfPrintTextFormattedColor(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(Language::get(64)), yres / 2 - 9 - 18, colorBlue, Language::get(64));
	//		ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(Language::get(65)), yres / 2 - 9 + 18, Language::get(65));
	//	}
	//	else if ( creditstage == 11 )
	//	{

	//		// title
	//		SDL_Rect src;
	//		src.x = 0;
	//		src.y = 0;
	//		src.w = title_bmp->w;
	//		src.h = title_bmp->h;
	//		SDL_Rect dest;
	//		dest.x = xres / 2 - (title_bmp->w) / 2;
	//		dest.y = yres / 2 - title_bmp->h / 2 - 96;
	//		dest.w = xres;
	//		dest.h = yres;
	//		drawImage(title_bmp, &src, &dest);
	//		// text
	//		ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2) * strlen(Language::get(66)), yres / 2, Language::get(66));
	//		ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2) * strlen(Language::get(67)), yres / 2 + 20, Language::get(67));
	//		ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2) * strlen(Language::get(68)), yres / 2 + 40, Language::get(68));
	//		ttfPrintTextFormattedColor(ttf16, xres / 2 - (TTF16_WIDTH / 2) * strlen(Language::get(69)), yres / 2 + 60, colorBlue, Language::get(69));

	//		// logo
	//		src.x = 0;
	//		src.y = 0;
	//		src.w = logo_bmp->w;
	//		src.h = logo_bmp->h;
	//		dest.x = xres / 2 - (logo_bmp->w) / 2;
	//		dest.y = yres / 2 + 80;
	//		dest.w = xres;
	//		dest.h = yres;
	//		drawImage(logo_bmp, &src, &dest);
	//	}
	//	else if ( creditstage == 13 )
	//	{
	//		ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE37), yres / 2 - 9, CREDITSLINE37);
	//		//ttfPrintTextFormattedColor(ttf16,xres/2-(TTF16_WIDTH/2)*strlen(CREDITSLINE37),yres/2+9,colorBlue,CREDITSLINE38);
	//	}
	//}

	// intro sequence
	if ( intromoviestage > 0 )
	{
		SDL_Rect pos;
		pos.x = 0;
		pos.y = 0;
		pos.w = xres;
		pos.h = (((real_t)xres) / backdrop_cursed_bmp->w) * backdrop_cursed_bmp->h;
		drawImageScaled(backdrop_cursed_bmp, NULL, &pos);

		if ( intromovietime >= 600 || inputs.bMouseLeft(clientnum) || keystatus[SDLK_ESCAPE] ||
		        keystatus[SDLK_SPACE] || keystatus[SDLK_RETURN] || (intromovietime >= 120 && intromoviestage == 1) || (inputs.bControllerInputPressed(clientnum, INJOY_MENU_NEXT) && rebindaction == -1) )
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
			ttfPrintTextColor(ttf16, 16, yres - 32, color, true, Language::get(1414));
		}
		if ( intromoviestage >= 2 )
		{
			intromoviealpha[0] = std::min(intromoviealpha[0] + 2, 255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0, intromoviealpha[0]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, Language::get(1415));
		}
		if ( intromoviestage >= 3 )
		{
			intromoviealpha[1] = std::min(intromoviealpha[1] + 2, 255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0, intromoviealpha[1]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, Language::get(1416));
		}
		if ( intromoviestage >= 4 )
		{
			intromoviealpha[2] = std::min(intromoviealpha[2] + 2, 255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0, intromoviealpha[2]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, Language::get(1417));
		}
		if ( intromoviestage >= 5 )
		{
			intromoviealpha[3] = std::min(intromoviealpha[3] + 2, 255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0, intromoviealpha[3]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, Language::get(1418));
		}
		if ( intromoviestage >= 6 )
		{
			intromoviealpha[4] = std::min(intromoviealpha[4] + 2, 255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0, intromoviealpha[4]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, Language::get(1419));
		}
		if ( intromoviestage >= 7 )
		{
			intromoviealpha[5] = std::min(intromoviealpha[5] + 2, 255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0, intromoviealpha[5]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, Language::get(1420));
		}
		if ( intromoviestage >= 8 )
		{
			intromoviealpha[6] = std::min(intromoviealpha[6] + 2, 255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0, intromoviealpha[6]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, Language::get(1421));
		}
		if ( intromoviestage == 9 )
		{
			intromoviealpha[7] = std::min(intromoviealpha[7] + 2, 255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0, intromoviealpha[7]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, Language::get(1422));
		}
	}

	// first end sequence (defeating herx)
	if ( firstendmoviestage > 0 )
	{
		SDL_Rect pos;
		pos.x = 0;
		pos.y = 0;
		pos.w = xres;
		pos.h = (((real_t)xres) / backdrop_cursed_bmp->w) * backdrop_cursed_bmp->h;
		drawImageScaled(backdrop_cursed_bmp, NULL, &pos);

		if ( firstendmovietime >= 600 || inputs.bMouseLeft(clientnum) || keystatus[SDLK_ESCAPE] ||
		        keystatus[SDLK_SPACE] || keystatus[SDLK_RETURN] || (firstendmovietime >= 120 && firstendmoviestage == 1) )
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
			ttfPrintTextColor(ttf16, 16, yres - 32, color, true, Language::get(1414));

			int titlex = 16;
			int titley = 12;
			// epilogues
			if ( epilogueMultiplayerType == CLIENT )
			{
				if ( strcmp(epilogueHostName, "") )
				{
					ttfPrintTextFormattedColor(ttf16, titlex, titley, color, Language::get(3819), epilogueHostName, Language::get(3821 + epilogueHostRace)); // says who's story type it is.
				}
			}
			else
			{
				ttfPrintTextFormattedColor(ttf16, titlex, titley, color, Language::get(3830), Language::get(3821 + epilogueHostRace)); // says who's story type it is
			}
		}
		if ( firstendmoviestage >= 2 )
		{
			firstendmoviealpha[0] = std::min(firstendmoviealpha[0] + 2, 255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0, firstendmoviealpha[0]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, Language::get(1423));
		}
		if ( firstendmoviestage >= 3 )
		{
			firstendmoviealpha[1] = std::min(firstendmoviealpha[1] + 2, 255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0, firstendmoviealpha[1]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, Language::get(1424));
		}
		if ( firstendmoviestage >= 4 )
		{
			firstendmoviealpha[2] = std::min(firstendmoviealpha[2] + 2, 255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0, firstendmoviealpha[2]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, Language::get(1425));
		}
		if ( firstendmoviestage == 5 )
		{
			firstendmoviealpha[3] = std::min(firstendmoviealpha[3] + 2, 255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0, firstendmoviealpha[3]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, Language::get(1426));
		}
	}

	// second end sequence (defeating the devil)
	if ( secondendmoviestage > 0 )
	{
		SDL_Rect pos;
		pos.x = 0;
		pos.y = 0;
		pos.w = xres;
		pos.h = (((real_t)xres) / backdrop_cursed_bmp->w) * backdrop_cursed_bmp->h;
		drawImageScaled(backdrop_cursed_bmp, NULL, &pos);

		if ( secondendmovietime >= 600 || inputs.bMouseLeft(clientnum) || keystatus[SDLK_ESCAPE] ||
		        keystatus[SDLK_SPACE] || keystatus[SDLK_RETURN] || (secondendmovietime >= 120 && secondendmoviestage == 1) )
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
			ttfPrintTextColor(ttf16, 16, yres - 32, color, true, Language::get(1414));

			int titlex = 16;
			int titley = 12;
			// epilogues
			if ( epilogueMultiplayerType == CLIENT )
			{
				if ( strcmp(epilogueHostName, "") )
				{
					ttfPrintTextFormattedColor(ttf16, titlex, titley, color, Language::get(3819), epilogueHostName, Language::get(3821 + epilogueHostRace)); // says who's story type it is.
				}
			}
			else
			{
				ttfPrintTextFormattedColor(ttf16, titlex, titley, color, Language::get(3830), Language::get(3821 + epilogueHostRace)); // says who's story type it is
			}
		}
		if ( secondendmoviestage >= 2 )
		{
			secondendmoviealpha[0] = std::min(secondendmoviealpha[0] + 2, 255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0, secondendmoviealpha[0]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 22, color, true, Language::get(1427));
		}
		if ( secondendmoviestage >= 3 )
		{
			secondendmoviealpha[1] = std::min(secondendmoviealpha[1] + 2, 255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0, secondendmoviealpha[1]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, Language::get(1428));
		}
		if ( secondendmoviestage >= 4 )
		{
			secondendmoviealpha[2] = std::min(secondendmoviealpha[2] + 2, 255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0, secondendmoviealpha[2]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, Language::get(1429));
		}
		if ( secondendmoviestage >= 5 )
		{
			secondendmoviealpha[3] = std::min(secondendmoviealpha[3] + 2, 255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0, secondendmoviealpha[3]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, Language::get(1430));
		}
		if ( secondendmoviestage >= 6 )
		{
			secondendmoviealpha[4] = std::min(secondendmoviealpha[4] + 2, 255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0, secondendmoviealpha[4]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, Language::get(1431));
		}
		if ( secondendmoviestage == 7 )
		{
			secondendmoviealpha[5] = std::min(secondendmoviealpha[5] + 2, 255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0, secondendmoviealpha[5]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, Language::get(1432));
		}
	}

	if ( thirdendmoviestage > 0 )
	{
		SDL_Rect pos;
		pos.x = 0;
		pos.y = 0;
		pos.w = xres;
		pos.h = (((real_t)xres) / backdrop_cursed_bmp->w) * backdrop_cursed_bmp->h;
		drawRect(&pos, 0, 255);
		drawImageScaled(backdrop_cursed_bmp, NULL, &pos);

		if ( thirdendmovietime >= 600 || inputs.bMouseLeft(clientnum) || keystatus[SDLK_ESCAPE] ||
			keystatus[SDLK_SPACE] || keystatus[SDLK_RETURN] || (thirdendmovietime >= 120 && thirdendmoviestage == 1) )
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
						ttfPrintTextColor(ttf16, 16, yres - 32, color, true, Language::get(3833));
					}
					else
					{
						ttfPrintTextColor(ttf16, 16, yres - 32, color, true, Language::get(3832));
					}
				}
			}
			else
			{
				thirdendmoviealpha[8] = std::min(thirdendmoviealpha[8] + 2, 255); // click to continue increase alpha
			}
			color = 0x00FFFFFF;
			color += std::min(std::max(0, thirdendmoviealpha[8]), 255) << 24;
			ttfPrintTextColor(ttf16, 16, yres - 32, color, true, Language::get(2606));

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
						ttfPrintTextFormattedColor(ttf16, titlex, titley, color, Language::get(3820), stats[0]->name, Language::get(3821 + race)); // says who's story type it is.
					}
				}
				else
				{
					ttfPrintTextFormattedColor(ttf16, titlex, titley, color, Language::get(3831), Language::get(3821 + race)); // says who's story type it is.
				}
			}
		}
		if ( thirdendmoviestage >= 2 )
		{
			thirdendmoviealpha[0] = std::min(thirdendmoviealpha[0] + 2, 255);
			color = 0x00FFFFFF;
			color += std::min(std::max(0, thirdendmoviealpha[0]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, Language::get(2600));
		}
		if ( thirdendmoviestage >= 3 )
		{
			thirdendmoviealpha[1] = std::min(thirdendmoviealpha[1] + 2, 255);
			color = 0x00FFFFFF;
			color += std::min(std::max(0, thirdendmoviealpha[1]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, Language::get(2601));
		}
		if ( thirdendmoviestage >= 4 )
		{
			thirdendmoviealpha[2] = std::min(thirdendmoviealpha[2] + 2, 255);
			color = 0x00FFFFFF;
			color += std::min(std::max(0, thirdendmoviealpha[2]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, Language::get(2602));
		}
		if ( thirdendmoviestage >= 5 )
		{
			thirdendmoviealpha[3] = std::min(thirdendmoviealpha[3] + 2, 255);
			color = 0x00FFFFFF;
			color += std::min(std::max(0, thirdendmoviealpha[3]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, Language::get(2603));
		}
	}
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
			ttfPrintTextColor(ttf16, 16, yres - 32, color, true, Language::get(2606));

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
						ttfPrintTextFormattedColor(ttf16, titlex, titley, color, Language::get(3819), epilogueHostName, Language::get(3821 + epilogueHostRace)); // says who's story type it is.
					}
				}
				else
				{
					ttfPrintTextFormattedColor(ttf16, titlex, titley, color, Language::get(3830), Language::get(3821 + epilogueHostRace)); // singleplayer story
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
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, Language::get(2607));
		}
		if ( fourthendmoviestage >= 3 )
		{
			if ( fourthendmoviestage < 5 )
			{
				fourthendmoviealpha[1] = std::min(fourthendmoviealpha[1] + 2, 255);
			}
			color = 0x00FFFFFF;
			color += std::min(std::max(0, fourthendmoviealpha[1]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, Language::get(2608));
		}
		if ( fourthendmoviestage >= 4 )
		{
			if ( fourthendmoviestage < 5 )
			{
				fourthendmoviealpha[2] = std::min(fourthendmoviealpha[2] + 2, 255);
			}
			color = 0x00FFFFFF;
			color += std::min(std::max(0, fourthendmoviealpha[2]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, Language::get(2609));
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
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, Language::get(2610));
		}
		if ( fourthendmoviestage >= 7 )
		{
			if ( fourthendmoviestage < 10 )
			{
				fourthendmoviealpha[4] = std::min(fourthendmoviealpha[4] + 2, 255);
			}
			color = 0x00FFFFFF;
			color += std::min(std::max(0, fourthendmoviealpha[4]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, Language::get(2611));
		}
		if ( fourthendmoviestage >= 8 )
		{
			if ( fourthendmoviestage < 10 )
			{
				fourthendmoviealpha[5] = std::min(fourthendmoviealpha[5] + 2, 255);
			}
			color = 0x00FFFFFF;
			color += std::min(std::max(0, fourthendmoviealpha[5]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, Language::get(2612));
		}
		if ( fourthendmoviestage >= 9 )
		{
			if ( fourthendmoviestage < 10 )
			{
				fourthendmoviealpha[6] = std::min(fourthendmoviealpha[6] + 2, 255);
			}
			color = 0x00FFFFFF;
			color += std::min(std::max(0, fourthendmoviealpha[6]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, Language::get(2613));
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
			ttfPrintTextColor(ttf16, 16 + (xres/ 2) - 256, (yres / 2) - 64, color, true, Language::get(2614));
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
		pos.h = (((real_t)xres) / backdrop_cursed_bmp->w) * backdrop_cursed_bmp->h;
		drawRect(&pos, 0, 255);
		drawImageScaled(backdrop_cursed_bmp, NULL, &pos);

		if ( DLCendmovieStageAndTime[movieType][MOVIE_TIME] >= 600 || inputs.bMouseLeft(clientnum) || keystatus[SDLK_ESCAPE] ||
			keystatus[SDLK_SPACE] || keystatus[SDLK_RETURN] || (DLCendmovieStageAndTime[movieType][MOVIE_TIME] >= 120 && DLCendmovieStageAndTime[movieType][MOVIE_STAGE] == 1) )
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
				langEntries.push_back(Language::get(3771));
				langEntries.push_back(Language::get(3772));
				langEntries.push_back(Language::get(3773));
				langEntries.push_back(Language::get(3774));
				langEntries.push_back(Language::get(3775));
				break;
			case MOVIE_MIDGAME_BAPHOMET_MONSTERS:
				langEntries.push_back(Language::get(3776));
				langEntries.push_back(Language::get(3777));
				langEntries.push_back(Language::get(3778));
				langEntries.push_back(Language::get(3779));
				langEntries.push_back(Language::get(3780));
				langEntries.push_back(Language::get(3781));
				break;
			case MOVIE_MIDGAME_BAPHOMET_HUMAN_AUTOMATON:
				langEntries.push_back(Language::get(3782));
				langEntries.push_back(Language::get(3783));
				langEntries.push_back(Language::get(3784));
				langEntries.push_back(Language::get(3785));
				langEntries.push_back(Language::get(3786));
				langEntries.push_back(Language::get(3787));
				break;
			case MOVIE_CLASSIC_WIN_MONSTERS:
				langEntries.push_back(Language::get(3788));
				langEntries.push_back(Language::get(3789));
				langEntries.push_back(Language::get(3790));
				langEntries.push_back(Language::get(3791));
				break;
			case MOVIE_CLASSIC_WIN_BAPHOMET_MONSTERS:
				langEntries.push_back(Language::get(3792));
				langEntries.push_back(Language::get(3793));
				langEntries.push_back(Language::get(3794));
				langEntries.push_back(Language::get(3795));
				langEntries.push_back(Language::get(3796));
				langEntries.push_back(Language::get(3797));
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
							ttfPrintTextColor(ttf16, 16, yres - 32, color, true, Language::get(3833));
						}
						else
						{
							ttfPrintTextColor(ttf16, 16, yres - 32, color, true, Language::get(3832));
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
			ttfPrintTextColor(ttf16, 16, yres - 32, color, true, Language::get(2606)); // click to continue

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
							ttfPrintTextFormattedColor(ttf16, titlex, titley, color, Language::get(3819), epilogueHostName, Language::get(3821 + epilogueHostRace)); // says who's story type it is.
						}
					}
					else
					{
						ttfPrintTextFormattedColor(ttf16, titlex, titley, color, Language::get(3830), Language::get(3821 + epilogueHostRace)); // says who's story type it is
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
							ttfPrintTextFormattedColor(ttf16, titlex, titley, color, Language::get(3820), stats[0]->name, Language::get(3821 + race)); // says who's story type it is.
						}
					}
					else
					{
						ttfPrintTextFormattedColor(ttf16, titlex, titley, color, Language::get(3831), Language::get(3821 + race)); // says who's story type it is.
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
				langEntries.push_back(Language::get(3798));
				langEntries.push_back(Language::get(3799));
				langEntries.push_back(Language::get(3800));
				langEntries.push_back(Language::get(3801));
				langEntries.push_back(Language::get(3802));
				langEntries.push_back(Language::get(3803));
				langEntries.push_back(Language::get(3804));
				break;
			case MOVIE_WIN_DEMONS_UNDEAD:
				langEntries.push_back(Language::get(3805));
				langEntries.push_back(Language::get(3806));
				langEntries.push_back(Language::get(3807));
				langEntries.push_back(Language::get(3808));
				langEntries.push_back(Language::get(3809));
				langEntries.push_back(Language::get(3810));
				langEntries.push_back(Language::get(3811));
				break;
			case MOVIE_WIN_BEASTS:
				langEntries.push_back(Language::get(3812));
				langEntries.push_back(Language::get(3813));
				langEntries.push_back(Language::get(3814));
				langEntries.push_back(Language::get(3815));
				langEntries.push_back(Language::get(3816));
				langEntries.push_back(Language::get(3817));
				langEntries.push_back(Language::get(3818));
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
			ttfPrintTextColor(ttf16, 16, yres - 32, color, true, Language::get(2606)); // click to continue.

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
						ttfPrintTextFormattedColor(ttf16, titlex, titley, color, Language::get(3819), epilogueHostName, Language::get(3821 + epilogueHostRace)); // says who's story type it is.
					}
				}
				else
				{
					ttfPrintTextFormattedColor(ttf16, titlex, titley, color, Language::get(3830), Language::get(3821 + epilogueHostRace)); // singleplayer story
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
			ttfPrintTextColor(ttf16, 16 + (xres / 2) - 256, (yres / 2) - 64, color, true, Language::get(2614));
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
#endif
}

void doQuitGame() {
	introstage = 0;
	mainloop = 0;
}

void doNewGame(bool makeHighscore) {
	bool bWasOnMainMenu = intro;
	introstage = 1;
	fadefinished = false;
	fadeout = false;
	gamePaused = false;
	multiplayerselect = SINGLE;
	intro = true; //Fix items auto-adding to the hotbar on game restart.

    for ( int i = 0; i < MAXPLAYERS; ++i )
    {
        Input::inputs[i].refresh();
    }

	bool localScores = gameModeManager.allowsHiscores();
	bool onlineScores = gameModeManager.allowsGlobalHiscores();
	{
		if ( makeHighscore )
		{
			// restarting game, make a highscore
            if (splitscreen) {
                for (int c = 0; c < MAXPLAYERS; ++c) {
                    if (!client_disconnected[c]) {
						if ( localScores )
						{
							saveScore(c);
						}
#ifdef USE_PLAYFAB
						if ( c == 0 )
						{
							if ( onlineScores )
							{
								playfabUser.postScore(c);
							}
						}
#endif
                    }
                }
            } else {
				if ( localScores )
				{
					saveScore(clientnum);
				}
#ifdef USE_PLAYFAB
				if ( onlineScores )
				{
					playfabUser.postScore(clientnum);
				}
				playfabUser.gameEnd();
#endif
            }
            saveAllScores(SCORESFILE);
            saveAllScores(SCORESFILE_MULTIPLAYER);
		}
	}

	if ( gameModeManager.allowsSaves() )
	{
		if ( makeHighscore )
		{
			deleteSaveGame(multiplayer);
			loadingsavegame = 0;
			loadinglobbykey = 0;
		}
	}
	camera_charsheet_offsetyaw = (330) * PI / 180; // reset player camera view.

	// undo shopkeeper grudge
	ShopkeeperPlayerHostility.reset();

	// setup game //TODO: Move into a function startGameStuff() or something.
	ticks = 0;
	entity_uids = 1;
	lastEntityUIDs = entity_uids;
	loading = true;
	darkmap = false;
	std::string prevmapname = map.name;
	bool died = players[clientnum]->entity == nullptr;

	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
	    client_keepalive[i] = ticks; // this way nobody times out when we reset ticks!
		players[i]->init();
		players[i]->hud.reset();
		players[i]->hud.followerBars.clear();
		players[i]->hud.playerBars.clear();
		deinitShapeshiftHotbar(i);
		for ( int c = 0; c < NUM_HOTBAR_ALTERNATES; ++c )
		{
			players[i]->hotbar.hotbarShapeshiftInit[c] = false;
		}
		players[i]->shootmode = true;
		players[i]->magic.clearSelectedSpells();
		players[i]->paperDoll.resetPortrait(); // reset paper doll camera view.
		players[i]->closeAllGUIs(CLOSEGUI_ENABLE_SHOOTMODE, CLOSEGUI_CLOSE_ALL);
	}
	EnemyHPDamageBarHandler::dumpCache();
	monsterAllyFormations.reset();
	PingNetworkStatus_t::reset();
	particleTimerEmitterHitEntities.clear();
	monsterTrapIgnoreEntities.clear();
	minimapHighlights.clear();

	bool bOldSecretLevel = secretlevel;
	int oldCurrentLevel = currentlevel;
	Compendium_t::Events_t::previousCurrentLevel = 0;
	Compendium_t::Events_t::previousSecretlevel = false;

	currentlevel = startfloor;
	secretlevel = false;
	victory = 0;
	completionTime = 0;

	setDefaultPlayerConducts(); // penniless, foodless etc.
	if ( startfloor != 0 )
	{
		conductGameChallenges[CONDUCT_CHEATS_ENABLED] = 1;
	}

	std::string challengeRunCustomStartLevel = "";
	bool quickStartPortal = (gameModeManager.getMode() == GameModeManager_t::GAME_MODE_CUSTOM_RUN || gameModeManager.getMode() == GameModeManager_t::GAME_MODE_CUSTOM_RUN_ONESHOT) && !loadingsavegame;
	if ( gameModeManager.currentSession.challengeRun.isActive(GameModeManager_t::CurrentSession_t::ChallengeRun_t::CHEVENT_SHOPPING_SPREE) )
	{
		if ( currentlevel == 0 && !loadingsavegame )
		{
			challengeRunCustomStartLevel = "minetown";
			secretlevel = true;
		}
	}

	if ( Mods::numCurrentModsLoaded <= 0 )
	{
		Mods::disableSteamAchievements = false;
	}

	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		minimapPings[i].clear(); // clear minimap pings
        auto& camera = players[i]->camera();
        camera.globalLightModifierActive = GLOBAL_LIGHT_MODIFIER_STOPPED;
        camera.luminance = defaultLuminance;
	}
	gameplayCustomManager.readFromFile();
	textSourceScript.scriptVariables.clear();

	if ( multiplayer == CLIENT )
	{
		if ( bWasOnMainMenu )
		{
			if ( !(gameModeManager.getMode() == GameModeManager_t::GAME_MODE_CUSTOM_RUN_ONESHOT
				|| gameModeManager.getMode() == GameModeManager_t::GAME_MODE_CUSTOM_RUN) )
			{
				gameModeManager.currentSession.saveServerFlags();
			}
		}
		svFlags = lobbyWindowSvFlags;
	}
	else if ( !loadingsavegame && bWasOnMainMenu )
	{
		if ( !(gameModeManager.getMode() == GameModeManager_t::GAME_MODE_CUSTOM_RUN_ONESHOT
			|| gameModeManager.getMode() == GameModeManager_t::GAME_MODE_CUSTOM_RUN) )
		{
			gameModeManager.currentSession.saveServerFlags();
		}
	}

	if ( gameModeManager.getMode() == GameModeManager_t::GAME_MODE_TUTORIAL )
	{
		svFlags &= ~(SV_FLAG_HARDCORE);
		svFlags &= ~(SV_FLAG_CHEATS);
		svFlags &= ~(SV_FLAG_LIFESAVING);
		svFlags &= ~(SV_FLAG_ASSIST_ITEMS);
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
    
    // very important: set this AFTER svFlags is configured
    keepInventoryGlobal = svFlags & SV_FLAG_KEEPINVENTORY;

	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		// clear follower menu entities.
		FollowerMenu[i].closeFollowerMenuGUI(true);
		CalloutMenu[i].closeCalloutMenuGUI();
	}
	for ( int c = 0; c < NUMMONSTERS; c++ )
	{
		kills[c] = 0;
	}

	for ( int c = 0; c < MAXPLAYERS; ++c )
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

		if ( players[c]->isLocalPlayer() )
		{
#ifndef NINTENDO
			if ( inputs.hasController(c) )
			{
				players[c]->hotbar.useHotbarFaceMenu = playerSettings[c].gamepad_facehotbar;
			}
			else if ( inputs.bPlayerUsingKeyboardControl(c) )
			{
				players[c]->hotbar.useHotbarFaceMenu = false;
			}
#else
			players[c]->hotbar.useHotbarFaceMenu = playerSettings[c].gamepad_facehotbar;
#endif // NINTENDO
		}
	}

	Player::Minimap_t::mapDetails.clear();

	// disable cheats
	noclip = false;
	godmode = false;
	buddhamode = false;
	everybodyfriendly = false;
	gameloopFreezeEntities = false;
	monsterGlobalAnimationMultiplier = 10;
	monsterGlobalAttackTimeMultiplier = 1;
	skipLevelsOnLoad = 0;

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

	SaveGameInfo saveGameInfo;
	if (loadingsavegame) {
		saveGameInfo = getSaveGameInfo(multiplayer == SINGLE);
	}

	// generate mimics
	{
		mimic_generator.init();
	}

	Compendium_t::Events_t::clientReceiveData.clear();
	for ( int c = 0; c < MAXPLAYERS; ++c )
	{
		Compendium_t::Events_t::clientDataStrings[c].clear();
		bool bOldIntro = intro;
		intro = false;
		if ( !bWasOnMainMenu && !loadingsavegame )
		{
			players[c]->compendiumProgress.updateFloorEvents();
		}
		intro = bOldIntro;
		if ( !loadingsavegame )
		{
			players[c]->compendiumProgress.itemEvents.clear();
			players[c]->compendiumProgress.floorEvents.clear();
		}
	}

	// load dungeon
	if ( multiplayer != CLIENT )
	{
		// stop all sounds
#ifdef USE_FMOD
		if ( sound_group )
		{
			sound_group->stop();
		}
		if ( soundAmbient_group )
		{
			soundAmbient_group->stop();
		}
		if ( soundEnvironment_group )
		{
			soundEnvironment_group->stop();
		}
		if ( soundNotification_group )
		{
			soundNotification_group->stop();
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

        if ( !loadingsavegame )
        {
			mapseed = 0;
			if ( challengeRunCustomStartLevel != "" )
			{
				mapseed = uniqueGameKey;
			}
		}

		// reset class loadout
		if (loadingsavegame) {
			Uint32 oldSvFlags = gameModeManager.currentSession.serverFlags;
			bool bOldSvFlags = gameModeManager.currentSession.bHasSavedServerFlags;
			for (int c = 0; c < MAXPLAYERS; ++c) {
				if (!client_disconnected[c]) {
					stats[c]->clearStats();
					loadGame(c, saveGameInfo);
				}
			}
			if ( bOldSvFlags )
			{
				// restore flags as we saved them in the lobby before this game started
				gameModeManager.currentSession.serverFlags = oldSvFlags;
			}
			if ( gameModeManager.currentSession.challengeRun.isActive(GameModeManager_t::CurrentSession_t::ChallengeRun_t::CHEVENT_SHOPPING_SPREE) )
			{
				if ( currentlevel == 0 )
				{
					challengeRunCustomStartLevel = "minetown";
					secretlevel = true;
				}
			}
			for ( int c = 0; c < MAXPLAYERS; ++c ) {
				GenericGUI[c].assistShrineGUI.resetSavedCharacterChanges();
			}
		} else {
			for (int c = 0; c < MAXPLAYERS; ++c) {
				if (!client_disconnected[c]) {
					GenericGUI[c].assistShrineGUI.onGameStart();
					stats[c]->clearStats();
					initClass(c);
				}
				else
				{
					GenericGUI[c].assistShrineGUI.resetSavedCharacterChanges();
				}
			}
		}

		// hack to fix these things from breaking everything...
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			players[i]->hud.arm = nullptr;
			players[i]->hud.weapon = nullptr;
			players[i]->hud.magicLeftHand = nullptr;
			players[i]->hud.magicRightHand = nullptr;
			players[i]->ghost.reset();
			FollowerMenu[i].recentEntity = nullptr;
			FollowerMenu[i].followerToCommand = nullptr;
			FollowerMenu[i].entityToInteractWith = nullptr;
			CalloutMenu[i].closeCalloutMenuGUI();
			CalloutMenu[i].callouts.clear();
		}

		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			players[i]->hud.resetBars(); // reset XP/HP/MP bars
			if ( !loadingsavegame && players[i]->isLocalPlayer() && !client_disconnected[i] )
			{
				ClassHotbarConfig_t::assignHotbarSlots(i); // assign custom hotbar configuration
			}
		}

		for ( node_t* node = map.entities->first; node != nullptr; node = node->next )
		{
			Entity* entity = (Entity*)node->element;
			entity->flags[NOUPDATE] = true;
		}
		lastEntityUIDs = entity_uids;
		numplayers = 0;
		int checkMapHash = -1;
		if ( loadingmap == false )
		{
			if ( challengeRunCustomStartLevel != "" )
			{
				std::string fullMapName = physfsFormatMapName(challengeRunCustomStartLevel.c_str());
				loadMap(fullMapName.c_str(), &map, map.entities, map.creatures, &checkMapHash);
				if ( !verifyMapHash(fullMapName.c_str(), checkMapHash) )
				{
					conductGameChallenges[CONDUCT_MODDED] = 1;
				}
			}
			else
			{
				physfsLoadMapFile(currentlevel, mapseed, false, &checkMapHash);
				if (!verifyMapHash(map.filename, checkMapHash))
				{
					conductGameChallenges[CONDUCT_MODDED] = 1;
					Mods::disableSteamAchievements = true;
				}

				if ( quickStartPortal && !strcmp(map.name, "Start Map") && currentlevel == 0 )
				{
					Entity* portal = newEntity(118, 0, map.entities, map.creatures);
					setSpriteAttributes(portal, nullptr, nullptr);
					portal->x = 6 * 16;
					portal->y = 13 * 16;
					portal->teleporterX = 6;
					portal->teleporterY = 25;
					portal->teleporterType = 2;
				}
			}
		}
		else
		{
			if ( genmap == false )
			{
				std::string fullMapName = physfsFormatMapName(maptoload);
				loadMap(fullMapName.c_str(), &map, map.entities, map.creatures, &checkMapHash);
				if (!verifyMapHash(fullMapName.c_str(), checkMapHash))
				{
					conductGameChallenges[CONDUCT_MODDED] = 1;
					Mods::disableSteamAchievements = true;
				}
			}
			else
			{
				generateDungeon(maptoload, mapseed);
			}
		}
		assignActions(&map);

		if ( !loadingsavegame && challengeRunCustomStartLevel == "minetown" )
		{
			std::vector<Entity*> shopkeepersToInsert;
			for ( node_t* node = map.creatures->first; node; node = node->next )
			{
				Entity* entity = (Entity*)node->element;
				if ( entity->sprite == 35 )
				{
					shopkeepersToInsert.push_back(entity);
				}
			}

			for ( auto entity : shopkeepersToInsert )
			{
				if ( auto monster = summonMonsterNoSmoke(SHOPKEEPER, entity->x, entity->y, true) )
				{
					if ( entity->entity_rng )
					{
						BaronyRNG shop_rng;
						Uint32 tmpSeed = entity->entity_rng->getU32();
						shop_rng.seedBytes(&tmpSeed, sizeof(tmpSeed));
						monster->seedEntityRNG(shop_rng.getU32());
					}
				}
			}
		}

		generatePathMaps();
        clearChunks();
        createChunks();

		achievementObserver.updateData();

		if ( loadingsavegame )
		{
			for ( int c = 0; c < MAXPLAYERS; c++ )
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

			list_t* followers = loadGameFollowers(saveGameInfo);
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
									monster->monsterAllyIndex = c;
									if ( multiplayer == SERVER )
									{
										serverUpdateEntitySkill(monster, 42); // update monsterAllyIndex for clients.
									}

									if ( multiplayer != CLIENT )
									{
										monster->monsterAllyClass = monsterStats->allyClass;
										monster->monsterAllyPickupItems = monsterStats->allyItemPickup;
										if ( stats[c]->playerSummonPERCHR != 0 && MonsterData_t::nameMatchesSpecialNPCName(*monsterStats, "skeleton knight") )
										{
											monster->monsterAllySummonRank = (stats[c]->playerSummonPERCHR & 0x0000FF00) >> 8;
										}
										else if ( stats[c]->playerSummon2PERCHR != 0 && MonsterData_t::nameMatchesSpecialNPCName(*monsterStats, "skeleton sentinel") )
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

									if ( c > 0 && multiplayer == SERVER && !players[c]->isLocalPlayer() )
									{
										strcpy((char*)net_packet->data, "LEAD");
										SDLNet_Write32((Uint32)monster->getUID(), &net_packet->data[4]);
										std::string name = monsterStats->name;
										if ( name != "" && name == MonsterData_t::getSpecialNPCName(*monsterStats) )
										{
											name = monsterStats->getAttribute("special_npc");
											name.insert(0, "$");
										}
                                        SDLNet_Write32(monsterStats->type, &net_packet->data[8]);
										strcpy((char*)(&net_packet->data[12]), name.c_str());
										net_packet->data[12 + strlen(name.c_str())] = 0;
										net_packet->address.host = net_clients[c - 1].host;
										net_packet->address.port = net_clients[c - 1].port;
										net_packet->len = 12 + strlen(name.c_str()) + 1;
										sendPacketSafe(net_sock, -1, net_packet, c - 1);

										serverUpdateAllyStat(c, monster->getUID(), monsterStats->LVL, monsterStats->HP, monsterStats->MAXHP, monsterStats->type);
									}
                                    else if (multiplayer != CLIENT && players[c]->isLocalPlayer())
                                    {
                                        if (monsterStats->name[0] && (!monsterNameIsGeneric(*monsterStats) || monsterStats->type == SLIME)) {
                                            Entity* nametag = newEntity(-1, 1, map.entities, nullptr);
                                            nametag->x = monster->x;
                                            nametag->y = monster->y;
                                            nametag->z = monster->z - 6;
                                            nametag->sizex = 1;
                                            nametag->sizey = 1;
                                            nametag->flags[NOUPDATE] = true;
                                            nametag->flags[PASSABLE] = true;
                                            nametag->flags[SPRITE] = true;
                                            nametag->flags[UNCLICKABLE] = true;
                                            nametag->flags[BRIGHT] = true;
                                            nametag->behavior = &actSpriteNametag;
                                            nametag->parent = monster->getUID();
                                            nametag->scalex = 0.2;
                                            nametag->scaley = 0.2;
                                            nametag->scalez = 0.2;
                                            nametag->skill[0] = c;
                                            nametag->skill[1] = playerColor(c, colorblind_lobby, true);
                                        }
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
			if ( gameModeManager.allowsSaves() )
			{
				saveGame();
			}
		}
	}
	else // if ( multiplayer != CLIENT ) (ergo in the block below, it is)
	{
		// hack to fix these things from breaking everything...
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			players[i]->hud.arm = nullptr;
			players[i]->hud.weapon = nullptr;
			players[i]->hud.magicLeftHand = nullptr;
			players[i]->hud.magicRightHand = nullptr;
			players[i]->ghost.reset();
			FollowerMenu[i].recentEntity = nullptr;
			FollowerMenu[i].followerToCommand = nullptr;
			FollowerMenu[i].entityToInteractWith = nullptr;
			CalloutMenu[i].closeCalloutMenuGUI();
			CalloutMenu[i].callouts.clear();
		}

		client_disconnected[0] = false;

		// initialize class
		if ( !loadingsavegame )
		{
			for (int c = 0; c < MAXPLAYERS; ++c) {
				GenericGUI[c].assistShrineGUI.onGameStart();
				stats[c]->clearStats();
				initClass(c);
			}
			mapseed = 0;
			if ( challengeRunCustomStartLevel != "" )
			{
				mapseed = uniqueGameKey;
			}
		}
		else
		{
			Uint32 oldSvFlags = gameModeManager.currentSession.serverFlags;
			bool bOldSvFlags = gameModeManager.currentSession.bHasSavedServerFlags;
			loadGame(clientnum, saveGameInfo);
			if ( bOldSvFlags )
			{
				// restore flags as we saved them in the lobby before this game started
				gameModeManager.currentSession.serverFlags = oldSvFlags;
			}
			if ( gameModeManager.currentSession.challengeRun.isActive(GameModeManager_t::CurrentSession_t::ChallengeRun_t::CHEVENT_SHOPPING_SPREE) )
			{
				if ( currentlevel == 0 )
				{
					challengeRunCustomStartLevel = "minetown";
					secretlevel = true;
				}
			}
			for ( int c = 0; c < MAXPLAYERS; ++c ) {
				GenericGUI[c].assistShrineGUI.resetSavedCharacterChanges();
			}
		}

		players[clientnum]->hud.resetBars();
		if ( !loadingsavegame )
		{
			ClassHotbarConfig_t::assignHotbarSlots(clientnum); // assign custom hotbar configuration
		}

		// stop all sounds
#ifdef USE_FMOD
		if ( sound_group )
		{
			sound_group->stop();
		}
		if ( soundAmbient_group )
		{
			soundAmbient_group->stop();
		}
		if ( soundEnvironment_group )
		{
			soundEnvironment_group->stop();
		}
		if ( soundNotification_group )
		{
			soundNotification_group->stop();
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
			if ( challengeRunCustomStartLevel != "" )
			{
				std::string fullMapName = physfsFormatMapName(challengeRunCustomStartLevel.c_str());
				loadMap(fullMapName.c_str(), &map, map.entities, map.creatures, &checkMapHash);
				if ( !verifyMapHash(fullMapName.c_str(), checkMapHash) )
				{
					conductGameChallenges[CONDUCT_MODDED] = 1;
				}
			}
			else
			{
				physfsLoadMapFile(currentlevel, mapseed, false, &checkMapHash);
				if (!verifyMapHash(map.filename, checkMapHash))
				{
					conductGameChallenges[CONDUCT_MODDED] = 1;
					Mods::disableSteamAchievements = true;
				}

				if ( quickStartPortal && !strcmp(map.name, "Start Map") && currentlevel == 0 )
				{
					Entity* portal = newEntity(118, 0, map.entities, map.creatures);
					setSpriteAttributes(portal, nullptr, nullptr);
					portal->x = 6 * 16;
					portal->y = 13 * 16;
					portal->teleporterX = 6;
					portal->teleporterY = 25;
					portal->teleporterType = 2;
				}
			}
		}
		else
		{
			if ( genmap == false )
			{
				std::string fullMapName = physfsFormatMapName(maptoload);
				loadMap(fullMapName.c_str(), &map, map.entities, map.creatures, &checkMapHash);
				if (!verifyMapHash(fullMapName.c_str(), checkMapHash))
				{
					conductGameChallenges[CONDUCT_MODDED] = 1;
					Mods::disableSteamAchievements = true;
				}
			}
			else
			{
				generateDungeon(maptoload, local_rng.rand());
			}
		}
		assignActions(&map);
		generatePathMaps();
        clearChunks();
        createChunks();

		node_t* nextnode;
		for ( node_t* node = map.entities->first; node != nullptr; node = nextnode )
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
	for ( int c = 0; c <= CLASS_MONK; c++ )
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
	for ( int c = CLASS_CONJURER; c <= CLASS_HUNTER; ++c )
	{
		if ( !usedClass[c] )
		{
			usedAllClasses = false;
		}
	}
	bool usedAllRaces = true;
	for ( int c = RACE_SKELETON; c <= RACE_INSECTOID; ++c )
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
#ifdef USE_PLAYFAB
		if ( !loadingsavegame )
		{
			playfabUser.gameBegin();
		}
#endif
		//achievementObserver.updateGlobalStat(STEAM_GSTAT_GAMES_STARTED);
	}

	// delete game data clutter
	list_FreeAll(&messages);
	list_FreeAll(&command_history);
	list_FreeAll(&safePacketsSent);
	for ( int c = 0; c < MAXPLAYERS; c++ )
	{
		safePacketsReceivedMap[c].clear();
		players[c]->messageZone.deleteAllNotificationMessages();
	}
	if ( !loadingsavegame ) // don't delete the followers we just created!
	{
		for (int c = 0; c < MAXPLAYERS; c++)
		{
			list_FreeAll(&stats[c]->FOLLOWERS);
		}
	}

	const auto wasLoadingSaveGame = loadingsavegame;

	if ( loadingsavegame && multiplayer != CLIENT )
	{
		loadingsavegame = 0;
		loadinglobbykey = 0;
	}

    // shuffle scroll names
    {
	    enchantedFeatherScrollsShuffled.clear();
	    enchantedFeatherScrollsShuffled.reserve(enchantedFeatherScrollsFixedList.size());
	    auto shuffle = enchantedFeatherScrollsFixedList;
		BaronyRNG feather_rng;
		feather_rng.seedBytes(&uniqueGameKey, sizeof(uniqueGameKey));
	    while (!shuffle.empty()) {
	        int index = feather_rng.getU8() % shuffle.size();
	        enchantedFeatherScrollsShuffled.push_back(shuffle[index]);
	        shuffle.erase(shuffle.begin() + index);
	    }
	}
	/*for ( auto it = enchantedFeatherScrollsShuffled.begin(); it != enchantedFeatherScrollsShuffled.end(); ++it )
	{
		printlog("Sequence: %d", *it);
	}*/

	list_FreeAll(&removedEntities);

	for ( int c = 0; c < MAXPLAYERS; c++ )
	{
		list_FreeAll(&chestInv[c]);
	}

	Frame::guiDestroy();
	Frame::guiInit();

	// make some messages
	Player::MessageZone_t::startMessages();

	for ( auto& pair : Player::Minimap_t::mapDetails )
	{
		if ( pair.second != "" )
		{
			messagePlayer(clientnum, MESSAGE_HINT, pair.second.c_str());
		}
	}

#ifdef LOCAL_ACHIEVEMENTS
	LocalAchievements_t::writeToFile();
#endif

	// kick off the main loop!
	pauseGame(1, 0);
	loading = false;
	intro = false;

	if ( !wasLoadingSaveGame )
	{
		if ( gameModeManager.getMode() == GameModeManager_t::GAME_MODE_TUTORIAL )
		{
			const std::string mapname = map.name;
			if ( mapname.find("Tutorial Hub") == std::string::npos
				&& mapname.find("Tutorial ") != std::string::npos )
			{
				Compendium_t::Events_t::eventUpdateWorld(clientnum, Compendium_t::CPDM_TRIALS_ATTEMPTS, "hall of trials", 1);
			}

			// restarting from a trial, this is a failure
			if ( died && !bWasOnMainMenu )
			{
				Compendium_t::Events_t::eventUpdateWorld(clientnum, Compendium_t::CPDM_LEVELS_DEATHS, "hall of trials", 1);
				Compendium_t::Events_t::eventUpdateWorld(clientnum, Compendium_t::CPDM_LEVELS_DEATHS_FASTEST, "hall of trials", players[clientnum]->compendiumProgress.playerAliveTimeTotal);
				Compendium_t::Events_t::eventUpdateWorld(clientnum, Compendium_t::CPDM_LEVELS_DEATHS_SLOWEST, "hall of trials", players[clientnum]->compendiumProgress.playerAliveTimeTotal);
			}

			Compendium_t::Events_t::eventUpdateWorld(clientnum, Compendium_t::CPDM_LEVELS_TIME_SPENT, "hall of trials", players[clientnum]->compendiumProgress.playerAliveTimeTotal);
		}
		else
		{
			if ( currentlevel == 0 && !secretlevel )
			{
				Compendium_t::Events_t::eventUpdateWorld(clientnum, Compendium_t::CPDM_MINEHEAD_ENTER, "minehead", 1);
				Compendium_t::Events_t::eventUpdateCodex(clientnum, Compendium_t::CPDM_CLASS_GAMES_STARTED, "class", 1);
				Compendium_t::Events_t::eventUpdateCodex(clientnum, Compendium_t::CPDM_RACE_GAMES_STARTED, "races", 1);
				if ( multiplayer == SERVER || multiplayer == CLIENT || (multiplayer == SINGLE && splitscreen) )
				{
					Compendium_t::Events_t::eventUpdateCodex(clientnum, Compendium_t::CPDM_CLASS_GAMES_MULTI, "class", 1);
					if ( multiplayer != SINGLE )
					{
						if ( directConnect )
						{
							Compendium_t::Events_t::eventUpdateWorld(clientnum, Compendium_t::CPDM_MINEHEAD_ENTER_LAN_MP, "minehead", 1);
						}
						else
						{
							Compendium_t::Events_t::eventUpdateWorld(clientnum, Compendium_t::CPDM_MINEHEAD_ENTER_ONLINE_MP, "minehead", 1);
						}
					}
					else if ( multiplayer == SINGLE )
					{
						if ( splitscreen )
						{
							Compendium_t::Events_t::eventUpdateWorld(clientnum, Compendium_t::CPDM_MINEHEAD_ENTER_SPLIT_MP, "minehead", 1);
						}
					}
				}
				else
				{
					Compendium_t::Events_t::eventUpdateCodex(clientnum, Compendium_t::CPDM_CLASS_GAMES_SOLO, "class", 1);
					Compendium_t::Events_t::eventUpdateWorld(clientnum, Compendium_t::CPDM_MINEHEAD_ENTER_SOLO, "minehead", 1);
				}

				if ( !bWasOnMainMenu )
				{
					const char* currentWorldString = Compendium_t::compendiumCurrentLevelToWorldString(oldCurrentLevel, bOldSecretLevel);
					if ( strcmp(currentWorldString, "") )
					{
						if ( died )
						{
							Compendium_t::Events_t::eventUpdateWorld(clientnum, Compendium_t::CPDM_LEVELS_DEATHS, currentWorldString, 1);
							Compendium_t::Events_t::eventUpdateWorld(clientnum, Compendium_t::CPDM_LEVELS_DEATHS_FASTEST, currentWorldString, players[clientnum]->compendiumProgress.playerAliveTimeTotal);
							Compendium_t::Events_t::eventUpdateWorld(clientnum, Compendium_t::CPDM_LEVELS_DEATHS_SLOWEST, currentWorldString, players[clientnum]->compendiumProgress.playerAliveTimeTotal);
						}
						Compendium_t::Events_t::eventUpdateWorld(clientnum, Compendium_t::CPDM_LEVELS_TIME_SPENT, currentWorldString, players[clientnum]->compendiumProgress.playerAliveTimeTotal);
						Compendium_t::Events_t::eventUpdateWorld(clientnum, Compendium_t::CPDM_TOTAL_TIME_SPENT, "minehead",
							players[clientnum]->compendiumProgress.playerGameTimeTotal);
					}
				}
			}
		}
	}
	Compendium_t::Events_t::writeItemsSaveData();
	Compendium_t::writeUnlocksSaveData();
	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		players[i]->compendiumProgress.playerAliveTimeTotal = 0;
		players[i]->compendiumProgress.playerGameTimeTotal = 0;
	}
}

void doCredits() {
	fadefinished = false;
	fadeout = false;
	if ( creditstage == 0 && victory == 3 )
	{
#ifdef MUSIC
		playMusic(citadelmusic[0], true, false, false);
#endif
	}
	creditstage++;
	if ( creditstage >= 15 )
	{
		introstage = 1;
		credittime = 0;
		creditstage = 0;
	}
	else
	{
		credittime = 0;
	}
}


void doEndgameOnDisconnect()
{
	client_disconnected[0] = false;

	introstage = 1;
	intro = true;

	// load menu level
	int menuMapType = 0;
	if ( victory == 3 || victory == 4 || victory == 5 )
	{
		menuMapType = loadMainMenuMap(true, true);
	}
	else
	{
		switch ( local_rng.rand() % 2 )
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
	for ( int c = 0; c < MAXPLAYERS; ++c ) {
		cameras[c].vang = 0;
		GenericGUI[c].assistShrineGUI.onMainMenuEnd();
	}
	numplayers = 0;
	assignActions(&map);
	generatePathMaps();

	gamePaused = false;
	if ( !victory )
	{
		fadefinished = false;
		fadeout = false;
	}
}

void doEndgame(bool saveHighscore, bool onServerDisconnect) {
	int c, x;
	bool endTutorial = false;
	bool localScores = gameModeManager.allowsHiscores();
	bool onlineScores = gameModeManager.allowsGlobalHiscores();
	bool allowedSavegames = gameModeManager.allowsSaves();
	if ( gameModeManager.getMode() == GameModeManager_t::GAME_MODE_TUTORIAL )
	{
		victory = 0;
		endTutorial = true;
		gameModeManager.setMode(GameModeManager_t::GAME_MODE_DEFAULT);
	}
	else if ( gameModeManager.getMode() == GameModeManager_t::GAME_MODE_CUSTOM_RUN_ONESHOT )
	{
		gameModeManager.setMode(GameModeManager_t::GAME_MODE_DEFAULT);
	}
	else if ( gameModeManager.getMode() == GameModeManager_t::GAME_MODE_CUSTOM_RUN )
	{
		gameModeManager.setMode(GameModeManager_t::GAME_MODE_DEFAULT);
	}

	// in greater numbers achievement
	if ( victory && victory <= 5 )
	{
		int k = 0;
		for ( int c = 0; c < MAXPLAYERS; c++ )
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
			|| ((victory == 3 || victory == 4 || victory == 5) && currentlevel >= 35) )
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
			achievementObserver.updateGlobalStat(STEAM_GSTAT_GAMES_WON, clientnum);

			for ( int c = 0; c < MAXPLAYERS; ++c )
			{
				if ( players[c]->isLocalPlayer() )
				{
					for ( node_t* node = stats[c]->FOLLOWERS.first; node != nullptr; node = node->next )
					{
						Entity* follower = nullptr;
						if ( (Uint32*)node->element )
						{
							follower = uidToEntity(*((Uint32*)node->element));
						}
						if ( follower )
						{
							if ( follower->getMonsterTypeFromSprite() == HUMAN )
							{
								Compendium_t::Events_t::eventUpdateWorld(c, Compendium_t::CPDM_HUMANS_SAVED, "the church", 1);
							}
						}
					}
				}
			}
		}
	}

	bool died = stats[clientnum] && stats[clientnum]->HP <= 0;
	Compendium_t::Events_t::onEndgameEvent(clientnum, endTutorial, saveHighscore, died);

	// make a highscore!
	if ( !endTutorial && saveHighscore )
	{
        if (splitscreen) {
            for (int c = 0; c < MAXPLAYERS; ++c) {
                if (!client_disconnected[c]) {
					if ( localScores )
					{
						saveScore(c);
					}
#ifdef USE_PLAYFAB
					if ( c == 0 )
					{
						if ( onlineScores )
						{
							playfabUser.postScore(c);
						}
					}
#endif
                }
            }
        } else {
			if ( localScores )
			{
				saveScore(clientnum);
			}
#ifdef USE_PLAYFAB
			if ( onlineScores )
			{
				playfabUser.postScore(clientnum);
			}
			playfabUser.gameEnd();
#endif
        }
        saveAllScores(SCORESFILE);
        saveAllScores(SCORESFILE_MULTIPLAYER);
	}

	for ( c = 0; c < NUMMONSTERS; c++ )
	{
		kills[c] = 0;
	}

	// stop all sounds
#ifdef USE_FMOD
	if ( sound_group )
	{
		sound_group->stop();
	}
	if ( soundAmbient_group )
	{
		soundAmbient_group->stop();
	}
	if ( soundEnvironment_group )
	{
		soundEnvironment_group->stop();
	}
	if ( soundNotification_group )
	{
		soundNotification_group->stop();
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

	if ( net_packet )
	{
		// send disconnect messages
		if (multiplayer == CLIENT)
		{
			strcpy((char*)net_packet->data, "DISC");
			net_packet->data[4] = clientnum;
			net_packet->address.host = net_server.host;
			net_packet->address.port = net_server.port;
			net_packet->len = 5;
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
				strcpy((char*)net_packet->data, "DISC");
				net_packet->data[4] = clientnum;
				net_packet->address.host = net_clients[x - 1].host;
				net_packet->address.port = net_clients[x - 1].port;
				net_packet->len = 5;
				sendPacketSafe(net_sock, -1, net_packet, x - 1);
				client_disconnected[x] = true;
			}
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

	if ( victory )
	{
		// conduct achievements
		if ( (victory == 1 && currentlevel >= 20)
			|| (victory == 2 && currentlevel >= 24)
			|| ((victory == 3 || victory == 4 || victory == 5) && currentlevel >= 35) )
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

		if ( gameModeManager.currentSession.challengeRun.isActive() )
		{
			steamAchievement("BARONY_ACH_REAP_SOW");
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
		else if ( victory == 3 || victory == 4 || victory == 5 )
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

				for ( int i = 0; i < MAXPLAYERS; ++i )
				{
					if ( players[i]->isLocalPlayer() && !client_disconnected[i] )
					{
						if ( client_classes[i] == CLASS_MESMER )
						{
							steamAchievement("BARONY_ACH_COMMANDER_CHIEF");
						}
						else if ( client_classes[i] == CLASS_BREWER )
						{
							steamAchievement("BARONY_ACH_DRUNK_POWER");
						}
						else if ( client_classes[i] == CLASS_ACCURSED )
						{
							steamAchievement("BARONY_ACH_POWER_HUNGRY");
							if ( stats[i]->EFFECTS[EFF_VAMPIRICAURA] && stats[i]->EFFECTS_TIMERS[EFF_VAMPIRICAURA] == -2 )
							{
								if ( stats[i] && (svFlags & SV_FLAG_HUNGER) )
								{
									steamAchievement("BARONY_ACH_BLOOD_IS_THE_LIFE");
								}
							}
						}
						else if ( client_classes[i] == CLASS_HUNTER )
						{
							steamAchievement("BARONY_ACH_RANGER_DANGER");
							if ( conductGameChallenges[CONDUCT_RANGED_ONLY] )
							{
								steamAchievement("BARONY_ACH_GUDIPARIAN_BAZI");
							}
						}
						else if ( client_classes[i] == CLASS_CONJURER )
						{
							steamAchievement("BARONY_ACH_TURN_UNDEAD");
						}
						else if ( client_classes[i] == CLASS_SHAMAN )
						{
							steamAchievement("BARONY_ACH_MY_FINAL_FORM");
						}
						else if ( client_classes[i] == CLASS_PUNISHER )
						{
							steamAchievement("BARONY_ACH_TIME_TO_SUFFER");
						}
						else if ( client_classes[i] == CLASS_MACHINIST )
						{
							steamAchievement("BARONY_ACH_LIKE_CLOCKWORK");
						}

						if ( stats[i] && stats[i]->stat_appearance == 0 )
						{
							switch ( stats[i]->playerRace )
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
		}
	}

	if ( !endTutorial && victory > 0 && allowedSavegames )
	{
		deleteSaveGame(multiplayer);
	}

	Compendium_t::Events_t::writeItemsSaveData();
	Compendium_t::writeUnlocksSaveData();

	gameModeManager.currentSession.seededRun.reset();
	gameModeManager.currentSession.challengeRun.reset();

	// disable cheats
	noclip = false;
	godmode = false;
	buddhamode = false;
	everybodyfriendly = false;
	skipLevelsOnLoad = 0;
	monsterGlobalAnimationMultiplier = 10;
	monsterGlobalAttackTimeMultiplier = 1;

	// reset game
	darkmap = false;
	multiplayer = SINGLE;
	currentlevel = 0;
	secretlevel = false;
	clientnum = 0;
	if ( !onServerDisconnect )
	{
		introstage = 1;
		intro = true;
	}
	splitscreen = false;

#ifdef NINTENDO
	fpsLimit = 60; // revert to 60 for the main menu
	nxEnableAutoSleep();
	nxEndParentalControls();
	if (directConnect) {
		// cleanse wireless connection state
		nxShutdownWireless();
	} else {
		MainMenu::logoutOfEpic();
	}
#endif

    // this is done so that save game screenshots get
    // reloaded after the game is done.
	//Image::dumpCache();

	for ( int i = 0; i < MAXPLAYERS; ++i )
	{
		players[i]->inventoryUI.appraisal.timer = 0;
		players[i]->inventoryUI.appraisal.current_item = 0;
		players[i]->hud.reset();
		players[i]->hud.followerBars.clear();
		players[i]->hud.playerBars.clear();
		deinitShapeshiftHotbar(i);
		for ( c = 0; c < NUM_HOTBAR_ALTERNATES; ++c )
		{
			players[i]->hotbar.hotbarShapeshiftInit[c] = false;
		}
		players[i]->shootmode = true;
		players[i]->magic.clearSelectedSpells();
		players[i]->paperDoll.resetPortrait(); // reset paper doll camera view.
		players[i]->closeAllGUIs(CLOSEGUI_ENABLE_SHOOTMODE, CLOSEGUI_CLOSE_ALL);
	}
	EnemyHPDamageBarHandler::dumpCache();
	monsterAllyFormations.reset();
	particleTimerEmitterHitEntities.clear();
	monsterTrapIgnoreEntities.clear();
	minimapHighlights.clear();
	PingNetworkStatus_t::reset();
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
			if ( !onServerDisconnect )
			{
				client_disconnected[c] = false;
			}
		}
		players[c]->entity = nullptr; //TODO: PLAYERSWAP VERIFY. Need to do anything else?
		players[c]->cleanUpOnEntityRemoval();
		stats[c]->sex = static_cast<sex_t>(0);
		stats[c]->stat_appearance = 0;
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
		players[i]->ghost.reset();
		FollowerMenu[i].recentEntity = nullptr;
		FollowerMenu[i].followerToCommand = nullptr;
		FollowerMenu[i].entityToInteractWith = nullptr;
		CalloutMenu[i].closeCalloutMenuGUI();
		CalloutMenu[i].callouts.clear();
	}

	if ( !onServerDisconnect )
	{
		// load menu level
		int menuMapType = 0;
		if ( victory == 3 || victory == 4 || victory == 5 )
		{
			menuMapType = loadMainMenuMap(true, true);
		}
		else
		{
			switch ( local_rng.rand() % 2 )
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
		}
	}

#if defined USE_EOS
	if ( !directConnect )
	{
		if ( EOS.CurrentLobbyData.currentLobbyIsValid() )
		{
			EOS.leaveLobby();
		}
	}
#endif

	Compendium_t::Events_t::clientReceiveData.clear();
	for ( int c = 0; c < MAXPLAYERS; ++c )
	{
		GenericGUI[c].assistShrineGUI.onMainMenuEnd();
		Compendium_t::Events_t::clientDataStrings[c].clear();
		players[c]->compendiumProgress.itemEvents.clear();
		players[c]->compendiumProgress.floorEvents.clear();
		players[c]->compendiumProgress.playerAliveTimeTotal = 0;
		players[c]->compendiumProgress.playerGameTimeTotal = 0;
		Compendium_t::Events_t::serverPlayerEvents[c].clear();
	}
#ifdef LOCAL_ACHIEVEMENTS
	LocalAchievements_t::writeToFile();
#endif
}

void doIntro() {
	fadefinished = false;
	fadeout = false;
	intromoviestage++;
	if ( intromoviestage >= 9 )
	{
		introstage = 1;
		intromovietime = 0;
		intromoviestage = 0;
		int c;
		for ( c = 0; c < 30; c++ )
		{
			intromoviealpha[c] = 0;
		}
	}
	else
	{
		intromovietime = 0;
	}

}

void doEndgameHerx() {
#ifdef MUSIC
	if ( firstendmoviestage == 0 )
	{
		playMusic(endgamemusic, true, true, false);
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
	}
}

void doEndgameDevil() {
#ifdef MUSIC
	if ( secondendmoviestage == 0 )
	{
		playMusic(endgamemusic, true, true, false);
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
	}
}

void doMidgame() {
#ifdef MUSIC
	if ( thirdendmoviestage == 0 )
	{
		playMusic(endgamemusic, true, true, false);
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
	}
}

void doEndgameCitadel() {
#ifdef MUSIC
	if ( fourthendmoviestage == 0 )
	{
		playMusic(endgamemusic, true, true, false);
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
	}
}

void doEndgameClassicAndExtraMidGame() {
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
		playMusic(endgamemusic, true, true, false);
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
		}
	}
}

void doEndgameExpansion() {
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
		playMusic(endgamemusic, true, true, false);
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
	}
}

/*-------------------------------------------------------------------------------

	button functions

	this section contains numerous button functions for the game

-------------------------------------------------------------------------------*/

// opens the gameover window
void openGameoverWindow()
{
	// deprecated
    return;

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
	score_t* score = scoreConstructor(clientnum);
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
		strcpy(subtext, Language::get(1133));

		strcat(subtext, Language::get(1134));

		strcat(subtext, Language::get(1135));
		strcat(subtext, scorenum);

		if ( madetop )
		{
			strcat(subtext, Language::get(1136));
		}
		else
		{
			strcat(subtext, Language::get(1137));
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
		strcpy(button->label, Language::get(1138));
		button->x = subx2 - strlen(Language::get(1138)) * 12 - 16;
		button->y = suby2 - 28;
		button->sizex = strlen(Language::get(1138)) * 12 + 8;
		button->sizey = 20;
		button->action = &buttonStartSingleplayer;
		button->visible = 1;
		button->focused = 1;
		button->joykey = joyimpulses[INJOY_MENU_NEXT];

		// Return to Main Menu
		button = newButton();
		strcpy(button->label, Language::get(1139));
		button->x = subx1 + 8;
		button->y = suby2 - 28;
		button->sizex = strlen(Language::get(1139)) * 12 + 8;
		button->sizey = 20;
		button->action = &buttonEndGameConfirm;
		button->visible = 1;
		button->focused = 1;
		button->joykey = joyimpulses[INJOY_MENU_CANCEL];
	}
	else
	{
		strcpy(subtext, Language::get(1140));

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
			strcat(subtext, Language::get(1141));
		}
		else
		{
			strcat(subtext, Language::get(1142));
		}

		strcat(subtext, Language::get(1143));
		strcat(subtext, scorenum);

		strcat(subtext, "\n\n");

		// Okay
		button = newButton();
		strcpy(button->label, Language::get(1144));
		button->sizex = strlen(Language::get(1144)) * 12 + 8;
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
		strcat(subtext, Language::get(1145 + local_rng.rand() % 15));
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
	button->key = SDLK_ESCAPE;
	button->joykey = joyimpulses[INJOY_MENU_CANCEL];
}

int getNumDisplays()
{
	int numdisplays = SDL_GetNumVideoDisplays();
	printlog("display count: %d.\n", numdisplays);
	return numdisplays;
}

void getResolutionList(int device_id, std::list<resolution>& resolutions)
{
	int nummodes = SDL_GetNumDisplayModes(device_id);
	printlog("display mode count: %d.\n", nummodes);

	SDL_DisplayMode mode;
	for (int i = 0; i < nummodes; i++)
	{
		SDL_GetDisplayMode(device_id, i, &mode);

		// resolutions below 1024x768 are not supported
		if ( mode.w < 1024 || mode.h < 720 || mode.refresh_rate == 0 )
		{
		    continue;
		}

		resolution res{mode.w, mode.h, mode.refresh_rate};
		resolutions.push_back(res);
	}

	// sort first by hz, then xres, and then by yres
	resolutions.sort([](const resolution& a, const resolution& b) {
		if (a.hz == b.hz) {
			if (a.x == b.x) {
				return a.y > b.y;
			} else {
				return a.x > b.x;
			}
		} else {
			return a.hz > b.hz;
		}
	});

	// remove identical modes
	resolutions.unique();
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
	int num_achievements = Compendium_t::achievements.size();
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
	strcpy(subtext, Language::get(3971));
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
		button->key = SDLK_ESCAPE;
		button->joykey = joyimpulses[INJOY_MENU_CANCEL];
	}

	// up / prev page button
	{
		button_t* button = newButton();
		//strcpy(button->label, u8"\u25B2");
		button->x = subx2 - 33;
		button->y = suby1 + 84;
		button->sizex = 30;
		button->sizey = 30;
		button->action = &buttonAchievementsUp;
		button->visible = 1;
		button->focused = 1;
		button->key = SDLK_UP;
		button->joykey = joyimpulses[INJOY_MENU_SETTINGS_PREV];
	}

	// down / next page button
	{
		button_t* button = newButton();
		//strcpy(button->label, u8"\u25BC");
		button->x = subx2 - 33;
		button->y = suby2 - 34;
		button->sizex = 30;
		button->sizey = 30;
		button->action = &buttonAchievementsDown;
		button->visible = 1;
		button->focused = 1;
		button->key = SDLK_DOWN;
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
    // deprecated
}


// opens the wait window for steam lobby (getting lobby list, etc.)
void openSteamLobbyWaitWindow(button_t* my)
{
	// deprecated
}

// "failed to connect" message
void openFailedConnectionWindow(int mode)
{
	// deprecated
}

// opens the lobby browser window (steam client only)
void openSteamLobbyBrowserWindow(button_t* my)
{
	// deprecated
}

// steam lobby browser join game
void buttonSteamLobbyBrowserJoinGame(button_t* my)
{
	// deprecated
}

// steam lobby browser refresh
void buttonSteamLobbyBrowserRefresh(button_t* my)
{
	// deprecated
}

// quit game button
void buttonQuitConfirm(button_t* my)
{
	// deprecated
}

// quit game button (no save)
void buttonQuitNoSaveConfirm(button_t* my)
{
	// deprecated
}

// end game button
void buttonEndGameConfirm(button_t* my)
{
	// deprecated
}

void buttonEndGameConfirmSave(button_t* my)
{
	// deprecated
}

// generic close window button
void buttonCloseSubwindow(button_t* my)
{
    return; // deprecated
}

void buttonCloseSettingsSubwindow(button_t* my)
{
	// deprecated
}

void buttonCloseAndEndGameConfirm(button_t* my)
{
	// deprecated
}

Uint32 charcreation_ticks = 0;

// move player forward through creation dialogue
void buttonContinue(button_t* my)
{
	return; // deprecated
}

// move player backward through creation dialogue
void buttonBack(button_t* my)
{
	// deprecated
}

// start a singleplayer game
void buttonStartSingleplayer(button_t* my)
{
	// deprecated
}

// host a multiplayer game
void buttonHostMultiplayer(button_t* my)
{
	return; // deprecated
}

// join a multiplayer game
void buttonJoinMultiplayer(button_t* my)
{
	// deprecated
}

// starts a lobby as host
void buttonHostLobby(button_t* my)
{
	return; // deprecated
}

// joins a lobby as client
// if direct-ip, this is called directly after pressing join
// otherwise for matchmaking, this is called asynchronously after a matchmaking lobby has been joined
void buttonJoinLobby(button_t* my)
{
    // deprecated
    return;
}

// starts the game as server
void buttonStartServer(button_t* my)
{
    // deprecated
	return;
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
	// deprecated
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
	// deprecated
}

void openConfirmResolutionWindow()
{
	inputs.mouseClearLeft(clientnum);
	keystatus[SDLK_RETURN] = 0;
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
	button->key = SDLK_RETURN;
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
	button->key = SDLK_ESCAPE;
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
		button->key = SDLK_ESCAPE;
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
		button->key = SDLK_RIGHT;
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
		button->key = SDLK_LEFT;
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
    // deprecated
    return;
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
	// deprecated
}

void openNewLoadGameWindow(button_t* my)
{
	// deprecated
}

void buttonDeleteSavedSoloGame(button_t* my)
{
	// deprecated
}

void buttonDeleteSavedMultiplayerGame(button_t* my)
{
	// deprecated
}

void buttonConfirmDeleteSoloFile(button_t* my)
{
	// deprecated
}

void buttonConfirmDeleteMultiplayerFile(button_t* my)
{
	// deprecated
}

void buttonOpenCharacterCreationWindow(button_t* my)
{
	button_t* button;

	playing_random_char = false;
	loadingsavegame = 0;
	loadinglobbykey = 0;
	loadGameSaveShowRectangle = 0;
	// reset class loadout
	clientnum = 0;
	stats[0]->sex = static_cast<sex_t>(0 + local_rng.rand() % 2);
	stats[0]->stat_appearance = 0 + local_rng.rand() % NUMAPPEARANCES;
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

	//if ( lastCreatedCharacterClass >= 0
	//	&& lastCreatedCharacterAppearance >= 0 
	//	&& lastCreatedCharacterSex >= 0 )
	//{
	//	button_t* replayCharacterBtn = newButton();
	//	strcpy(replayCharacterBtn->label, Language::get(3000));
	//	replayCharacterBtn->sizex = strlen(Language::get(3000)) * 12 + 8;
	//	replayCharacterBtn->sizey = 20;
	//	replayCharacterBtn->x = button->x - (replayCharacterBtn->sizex + 4); // take position of button attributes above.
	//	replayCharacterBtn->y = button->y;
	//	replayCharacterBtn->action = &buttonReplayLastCharacter;
	//	replayCharacterBtn->visible = 1;
	//	replayCharacterBtn->focused = 1;
	//}

	// Continue ...
	button = newButton();
	strcpy(button->label, Language::get(1464));
	button->sizex = strlen(Language::get(1464)) * 12 + 8;
	button->sizey = 20;
	button->x = subx2 - button->sizex - 4;
	button->y = suby2 - 24;
	button->action = &buttonContinue;
	button->visible = 1;
	button->focused = 1;
	button->key = SDLK_RETURN;
	button->joykey = joyimpulses[INJOY_MENU_NEXT];

	// Back ...
	button = newButton();
	strcpy(button->label, Language::get(1465));
	button->x = subx1 + 4;
	button->y = suby2 - 24;
	button->sizex = strlen(Language::get(1465)) * 12 + 8;
	button->sizey = 20;
	button->action = &buttonBack;
	button->visible = 1;
	button->focused = 1;
	button->key = SDLK_ESCAPE;
	button->joykey = joyimpulses[INJOY_MENU_CANCEL];
	int button_back_x = button->x;
	int button_back_width = button->sizex;

	// Random Character ...
	button = newButton();
	strcpy(button->label, Language::get(1466));
	button->x = button_back_x + button_back_width + 4;
	button->y = suby2 - 24;
	button->sizex = strlen(Language::get(1466)) * 12 + 8;
	button->sizey = 20;
	button->action = &buttonRandomCharacter;
	button->visible = 1;
	button->focused = 1;
	button->key = SDLK_r; //NOTE: This might cause the character to randomly R when you're typing a name. So far, exactly one user has reported something like this happening exactly once in the entirety of existence.
	button->joykey = joyimpulses[INJOY_MENU_RANDOM_CHAR]; //random character => "y" button

	//Random Name.
	button = newButton();
	strcpy(button->label, Language::get(2498));
	button->x = button_back_x + button_back_width + 4;
	button->y = suby2 - 24;
	button->sizex = strlen(Language::get(2498)) * 12 + 8;
	button->sizey = 20;
	button->action = &buttonRandomName;
	button->visible = 1;
	button->focused = 1;
	button->joykey = joyimpulses[INJOY_MENU_RANDOM_NAME];
}

void buttonLoadSingleplayerGame(button_t* button)
{
	return; // deprecated
}

void buttonLoadMultiplayerGame(button_t* button)
{
	return; // deprecated
}

void buttonRandomCharacter(button_t* my)
{
	playing_random_char = true;
	charcreation_step = 4;
	camera_charsheet_offsetyaw = (330) * PI / 180;
	stats[0]->sex = static_cast<sex_t>(local_rng.rand() % 2);
	client_classes[0] = local_rng.rand() % (CLASS_MONK + 1);//NUMCLASSES;
	stats[0]->clearStats();
	if ( enabledDLCPack1 || enabledDLCPack2 )
	{
		stats[0]->playerRace = local_rng.rand() % NUMPLAYABLERACES;
		if ( !enabledDLCPack1 )
		{
			while ( stats[0]->playerRace == RACE_SKELETON || stats[0]->playerRace == RACE_VAMPIRE
				|| stats[0]->playerRace == RACE_SUCCUBUS || stats[0]->playerRace == RACE_GOATMAN )
			{
				stats[0]->playerRace = local_rng.rand() % NUMPLAYABLERACES;
			}
		}
		else if ( !enabledDLCPack2 )
		{
			while ( stats[0]->playerRace == RACE_AUTOMATON || stats[0]->playerRace == RACE_GOBLIN
				|| stats[0]->playerRace == RACE_INCUBUS || stats[0]->playerRace == RACE_INSECTOID )
			{
				stats[0]->playerRace = local_rng.rand() % NUMPLAYABLERACES;
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
			client_classes[0] = local_rng.rand() % (NUMCLASSES);
			if ( !enabledDLCPack1 )
			{
				while ( client_classes[0] == CLASS_CONJURER || client_classes[0] == CLASS_ACCURSED
					|| client_classes[0] == CLASS_MESMER || client_classes[0] == CLASS_BREWER )
				{
					client_classes[0] = local_rng.rand() % (NUMCLASSES);
				}
			}
			else if ( !enabledDLCPack2 )
			{
				while ( client_classes[0] == CLASS_HUNTER || client_classes[0] == CLASS_SHAMAN
					|| client_classes[0] == CLASS_PUNISHER || client_classes[0] == CLASS_MACHINIST )
				{
					client_classes[0] = local_rng.rand() % (NUMCLASSES);
				}
			}
			stats[0]->stat_appearance = local_rng.rand() % NUMAPPEARANCES;
		}
		else
		{
			client_classes[0] = local_rng.rand() % (CLASS_MONK + 2);
			if ( client_classes[0] > CLASS_MONK )
			{
				client_classes[0] = CLASS_MONK + stats[0]->playerRace; // monster specific classes.
			}
			stats[0]->stat_appearance = 0;
		}
	}
	else
	{
		stats[0]->playerRace = RACE_HUMAN;
		stats[0]->stat_appearance = local_rng.rand() % NUMAPPEARANCES;
	}
	initClass(0);
}

bool replayLastCharacter(const int index, int multiplayer)
{
	if ( multiplayer != SINGLE )
	{
		if ( index != clientnum )
		{
			return false;
		}
		if ( client_disconnected[index] )
		{
			return false;
		}
	}
	
	int savedCharacterIndex = index;
	if ( multiplayer != SINGLE )
	{
		if ( multiplayer == DIRECTCLIENT || multiplayer == DIRECTSERVER )
		{
			savedCharacterIndex = LastCreatedCharacter::LASTCHAR_LAN_PERSONA_INDEX;
		}
		else
		{
			savedCharacterIndex = LastCreatedCharacter::LASTCHAR_ONLINE_PERSONA_INDEX;
		}
	}

	if ( savedCharacterIndex < 0 || savedCharacterIndex > LastCreatedCharacter::LASTCHAR_ONLINE_PERSONA_INDEX )
	{
		return false;
	}

	auto& lastClass = LastCreatedCharacterSettings.characterClass[savedCharacterIndex];
	auto& lastSex = LastCreatedCharacterSettings.characterSex[savedCharacterIndex];
	auto& lastRace = LastCreatedCharacterSettings.characterRace[savedCharacterIndex];
	auto& lastAppearance = LastCreatedCharacterSettings.characterAppearance[savedCharacterIndex];
	auto& lastName = LastCreatedCharacterSettings.characterName[savedCharacterIndex];

	if ( lastClass >= 0 && lastSex >= 0 && lastRace >= 0 && lastAppearance >= 0 && lastName != "" )
	{
		stats[index]->sex = static_cast<sex_t>(std::min(lastSex, (int)sex_t::FEMALE));
		stats[index]->playerRace = std::min(std::max(static_cast<int>(RACE_HUMAN), lastRace), static_cast<int>(NUMPLAYABLERACES));
		stats[index]->stat_appearance = lastAppearance;
		client_classes[index] = std::min(std::max(0, lastClass), static_cast<int>(CLASS_HUNTER));

		switch ( isCharacterValidFromDLC(*stats[index], lastClass) )
		{
			case VALID_OK_CHARACTER:
				// do nothing.
				break;
			case INVALID_REQUIREDLC1:
			case INVALID_REQUIREDLC2:
				// class or race invalid.
				if ( stats[index]->playerRace > RACE_HUMAN )
				{
					stats[index]->playerRace = RACE_HUMAN;
				}
				if ( client_classes[index] > CLASS_MONK )
				{
					client_classes[index] = CLASS_BARBARIAN;
				}
				break;
			case INVALID_CHARACTER:
				// invalid for whatever reason, reset.
				stats[index]->playerRace = RACE_HUMAN;
				client_classes[index] = CLASS_BARBARIAN;
				break;
			case INVALID_REQUIRE_ACHIEVEMENT:
				// required achievement for class mixing among races, so race is valid.
				client_classes[index] = CLASS_BARBARIAN;
				break;
			default:
				// invalid for whatever reason, reset.
				stats[index]->playerRace = RACE_HUMAN;
				client_classes[index] = CLASS_BARBARIAN;
				break;
		}
		
		stats[index]->clearStats();
		initClass(index);

		auto name = lastName.c_str();
		size_t len = strlen(name);
		len = std::min(sizeof(Stat::name) - 1, len);
		memcpy(stats[index]->name, name, len);
		stats[index]->name[len] = '\0';
		return true;
	}
	return false;
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


//void writeLevelsTxt(std::string modFolder)
//{
//	std::string path = BASE_DATA_DIR;
//	path.append("mods/").append(modFolder);
//	if ( access(path.c_str(), F_OK) == 0 )
//	{
//		std::string writeFile = modFolder + "/maps/levels.txt";
//		PHYSFS_File *physfp = PHYSFS_openWrite(writeFile.c_str());
//		if ( physfp != NULL )
//		{
//			PHYSFS_writeBytes(physfp, "map: start\n", 11);
//			PHYSFS_writeBytes(physfp, "gen: mine\n", 10);
//			PHYSFS_writeBytes(physfp, "gen: mine\n", 10);
//			PHYSFS_writeBytes(physfp, "gen: mine\n", 10);
//			PHYSFS_writeBytes(physfp, "gen: mine\n", 10);
//			PHYSFS_writeBytes(physfp, "map: minetoswamp\n", 17);
//			PHYSFS_writeBytes(physfp, "gen: swamp\n", 11);
//			PHYSFS_writeBytes(physfp, "gen: swamp\n", 11);
//			PHYSFS_writeBytes(physfp, "gen: swamp\n", 11);
//			PHYSFS_writeBytes(physfp, "gen: swamp\n", 11);
//			PHYSFS_writeBytes(physfp, "map: swamptolabyrinth\n", 22);
//			PHYSFS_writeBytes(physfp, "gen: labyrinth\n", 15);
//			PHYSFS_writeBytes(physfp, "gen: labyrinth\n", 15);
//			PHYSFS_writeBytes(physfp, "gen: labyrinth\n", 15);
//			PHYSFS_writeBytes(physfp, "gen: labyrinth\n", 15);
//			PHYSFS_writeBytes(physfp, "map: labyrinthtoruins\n", 22);
//			PHYSFS_writeBytes(physfp, "gen: ruins\n", 11);
//			PHYSFS_writeBytes(physfp, "gen: ruins\n", 11);
//			PHYSFS_writeBytes(physfp, "gen: ruins\n", 11);
//			PHYSFS_writeBytes(physfp, "gen: ruins\n", 11);
//			PHYSFS_writeBytes(physfp, "map: boss\n", 10);
//			PHYSFS_writeBytes(physfp, "gen: hell\n", 10);
//			PHYSFS_writeBytes(physfp, "gen: hell\n", 10);
//			PHYSFS_writeBytes(physfp, "gen: hell\n", 10);
//			PHYSFS_writeBytes(physfp, "map: hellboss\n", 14);
//			PHYSFS_writeBytes(physfp, "map: hamlet\n", 12);
//			PHYSFS_writeBytes(physfp, "gen: caves\n", 11);
//			PHYSFS_writeBytes(physfp, "gen: caves\n", 11);
//			PHYSFS_writeBytes(physfp, "gen: caves\n", 11);
//			PHYSFS_writeBytes(physfp, "gen: caves\n", 11);
//			PHYSFS_writeBytes(physfp, "map: cavestocitadel\n", 20);
//			PHYSFS_writeBytes(physfp, "gen: citadel\n", 13);
//			PHYSFS_writeBytes(physfp, "gen: citadel\n", 13);
//			PHYSFS_writeBytes(physfp, "gen: citadel\n", 13);
//			PHYSFS_writeBytes(physfp, "gen: citadel\n", 13);
//			PHYSFS_writeBytes(physfp, "map: sanctum", 12);
//			PHYSFS_close(physfp);
//		}
//		else
//		{
//			printlog("[PhysFS]: Failed to open %s/maps/levels.txt for writing.", path.c_str());
//		}
//	}
//	else
//	{
//		printlog("[PhysFS]: Failed to write levels.txt in %s", path.c_str());
//	}
//}

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
	button->key = SDLK_ESCAPE;
	button->joykey = joyimpulses[INJOY_MENU_CANCEL];

	// create button
	button = newButton();
	strcpy(button->label, "create");
	button->x = subx2 - (strlen(button->label) * TTF12_WIDTH + 8);
	button->y = suby2 - TTF12_HEIGHT - 8;
	button->sizex = strlen(button->label) * TTF12_WIDTH + 8;
	button->sizey = 20;
	//button->action = &buttonGamemodsCreateModDirectory;
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
#ifdef _UNICODE
			wchar_t pathbuffer[PATH_MAX];
			const int len1 = MultiByteToWideChar(CP_ACP, 0, directoryToUpload.c_str(), directoryToUpload.size() + 1, 0, 0);
			auto buf1 = new wchar_t[len1];
			MultiByteToWideChar(CP_ACP, 0, directoryToUpload.c_str(), directoryToUpload.size() + 1, buf1, len1);
			const int pathlen = GetFullPathNameW(buf1, PATH_MAX, pathbuffer, NULL);
			delete[] buf1;
			const int len2 = WideCharToMultiByte(CP_ACP, 0, pathbuffer, pathlen, 0, 0, 0, 0);
			auto buf2 = new char[len2];
			WideCharToMultiByte(CP_ACP, 0, pathbuffer, pathlen, buf2, len2, 0, 0);
			std::string fullpath = buf2;
#else
			char pathbuffer[PATH_MAX];
			GetFullPathNameA(directoryToUpload.c_str(), PATH_MAX, pathbuffer, NULL);
			std::string fullpath = pathbuffer;
#endif
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
#ifdef _UNICODE
				wchar_t pathbuffer[PATH_MAX];
				const int len1 = MultiByteToWideChar(CP_ACP, 0, directoryToUpload.c_str(), directoryToUpload.size() + 1, 0, 0);
				auto buf1 = new wchar_t[len1];
				MultiByteToWideChar(CP_ACP, 0, directoryToUpload.c_str(), directoryToUpload.size() + 1, buf1, len1);
				const int pathlen = GetFullPathNameW(buf1, PATH_MAX, pathbuffer, NULL);
				delete[] buf1;
				const int len2 = WideCharToMultiByte(CP_ACP, 0, pathbuffer, pathlen, 0, 0, 0, 0);
				auto buf2 = new char[len2];
				WideCharToMultiByte(CP_ACP, 0, pathbuffer, pathlen, buf2, len2, 0, 0);
				std::string fullpath = buf2;
#else
				char pathbuffer[PATH_MAX];
				GetFullPathNameA(directoryToUpload.c_str(), PATH_MAX, pathbuffer, NULL);
				std::string fullpath = pathbuffer;
#endif
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
	button->key = SDLK_ESCAPE;
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
	button->key = SDLK_ESCAPE;
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
		ttfPrintTextColor(ttf12, x, y, makeColor( 128, 128, 128, 255), true, printText.c_str());
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
//	if ( gamemods_modPreload )
//	{
//		// look for a save game
//		if ( anySaveFileExists() )
//		{
//			openNewLoadGameWindow(nullptr);
//		}
//		else
//		{
//			buttonOpenCharacterCreationWindow(NULL);
//		}
//		return;
//	}
//
//	gamemods_numCurrentModsLoaded = gamemods_mountedFilepaths.size();
//	if ( gamemods_numCurrentModsLoaded > 0 )
//	{
//		steamAchievement("BARONY_ACH_LOCAL_CUSTOMS");
//	}
//
//	if ( physfsIsMapLevelListModded() )
//	{
//		Mods::disableSteamAchievements = true;
//	}
//	else
//	{
//		Mods::disableSteamAchievements = false;
//	}
//
//	int w, h;
//
//	if ( !gamemods_modelsListRequiresReload && gamemods_modelsListLastStartedUnmodded )
//	{
//		if ( physfsSearchModelsToUpdate() || !gamemods_modelsListModifiedIndexes.empty() )
//		{
//			gamemods_modelsListRequiresReload = true;
//		}
//		gamemods_modelsListLastStartedUnmodded = false;
//	}
//	if ( !gamemods_soundListRequiresReload && gamemods_soundsListLastStartedUnmodded )
//	{
//		if ( physfsSearchSoundsToUpdate() )
//		{
//			gamemods_soundListRequiresReload = true;
//		}
//		gamemods_soundsListLastStartedUnmodded = false;
//	}
//
//	// process any new model files encountered in the mod load list.
//	int modelsIndexUpdateStart = 1;
//	int modelsIndexUpdateEnd = nummodels;
//	if ( gamemods_modelsListRequiresReload )
//	{
//		if ( physfsSearchModelsToUpdate() || !gamemods_modelsListModifiedIndexes.empty() )
//		{
//			// print a loading message
//			drawClearBuffers();
//			getSizeOfText(ttf16, Language::get(2989), &w, &h);
//			ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, Language::get(2989));
//			GO_SwapBuffers(screen);
//			physfsModelIndexUpdate(modelsIndexUpdateStart, modelsIndexUpdateEnd, true);
//			generatePolyModels(modelsIndexUpdateStart, modelsIndexUpdateEnd, false);
//			generateVBOs(modelsIndexUpdateStart, modelsIndexUpdateEnd);
//		}
//		gamemods_modelsListRequiresReload = false;
//	}
//	if ( gamemods_soundListRequiresReload )
//	{
//		if ( physfsSearchSoundsToUpdate() )
//		{
//			// print a loading message
//			drawClearBuffers();
//			getSizeOfText(ttf16, Language::get(2987), &w, &h);
//			ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, Language::get(2987));
//			GO_SwapBuffers(screen);
//			physfsReloadSounds(true);
//		}
//		gamemods_soundListRequiresReload = false;
//	}
//
//	if ( physfsSearchTilesToUpdate() )
//	{
//		// print a loading message
//		drawClearBuffers();
//		getSizeOfText(ttf16, Language::get(3017), &w, &h);
//		ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, Language::get(3017));
//		GO_SwapBuffers(screen);
//		physfsReloadTiles(false);
//		gamemods_tileListRequireReloadUnmodded = true;
//	}
//
//	if ( physfsSearchSpritesToUpdate() )
//	{
//		// print a loading message
//		drawClearBuffers();
//		getSizeOfText(ttf16, Language::get(3015), &w, &h);
//		ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, Language::get(3015));
//		GO_SwapBuffers(screen);
//		physfsReloadSprites(false);
//		gamemods_spriteImagesRequireReloadUnmodded = true;
//	}
//
//	if ( physfsSearchBooksToUpdate() )
//	{
//		// print a loading message
//		drawClearBuffers();
//		getSizeOfText(ttf16, Language::get(2991), &w, &h);
//		ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, Language::get(2991));
//		GO_SwapBuffers(screen);
//		physfsReloadBooks();
//		gamemods_booksRequireReloadUnmodded = true;
//	}
//
//	gamemodsUnloadCustomThemeMusic();
//
//	if ( physfsSearchMusicToUpdate() )
//	{
//		// print a loading message
//		drawClearBuffers();
//		getSizeOfText(ttf16, Language::get(2993), &w, &h);
//		ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, Language::get(2993));
//		GO_SwapBuffers(screen);
//		bool reloadIntroMusic = false;
//		physfsReloadMusic(reloadIntroMusic, false);
//		if ( reloadIntroMusic )
//		{
//#ifdef SOUND
//			playMusic(intromusic[local_rng.rand() % (NUMINTROMUSIC - 1)], false, true, true);
//#endif			
//		}
//		gamemods_musicRequireReloadUnmodded = true;
//	}
//
//	std::string langDirectory = PHYSFS_getRealDir("lang/en.txt");
//	if ( langDirectory.compare("./") != 0 )
//	{
//		// print a loading message
//		drawClearBuffers();
//		getSizeOfText(ttf16, Language::get(3004), &w, &h);
//		ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, Language::get(3004));
//		GO_SwapBuffers(screen);
//		if ( reloadLanguage() != 0 )
//		{
//			printlog("[PhysFS]: Error reloading modified language file in lang/ directory!");
//		}
//		else
//		{
//			printlog("[PhysFS]: Found modified language file in lang/ directory, reloading en.txt...");
//		}
//		gamemods_langRequireReloadUnmodded = true;
//	}
//
//	if ( physfsSearchMonsterLimbFilesToUpdate() )
//	{
//		// print a loading message
//		drawClearBuffers();
//		getSizeOfText(ttf16, Language::get(3013), &w, &h);
//		ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, Language::get(3013));
//		GO_SwapBuffers(screen);
//		physfsReloadMonsterLimbFiles();
//		gamemods_monsterLimbsRequireReloadUnmodded = true;
//	}
//
//	if ( physfsSearchSystemImagesToUpdate() )
//	{
//		// print a loading message
//		drawClearBuffers();
//		getSizeOfText(ttf16, Language::get(3015), &w, &h);
//		ttfPrintText(ttf16, (xres - w) / 2, (yres - h) / 2, Language::get(3015));
//		GO_SwapBuffers(screen);
//		physfsReloadSystemImages();
//		gamemods_systemImagesReloadUnmodded = true;
//	}
//
//	// look for a save game
//	if ( anySaveFileExists() )
//	{
//		//openLoadGameWindow(NULL);
//		openNewLoadGameWindow(nullptr);
//	}
//	else
//	{
//		buttonOpenCharacterCreationWindow(NULL);
//	}
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
	button->key = SDLK_ESCAPE;
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
	return true;
	//bool success = true;
	//char **i;
	//for ( i = PHYSFS_getSearchPath(); *i != NULL; i++ )
	//{
	//	std::string line = *i;
	//	if ( line.compare(outputdir) != 0 && line.compare(datadir) != 0 && line.compare("./") != 0 ) // don't unmount the base ./ directory
	//	{
	//		if ( PHYSFS_unmount(*i) == 0 )
	//		{
	//			success = false;
	//			printlog("[%s] unsuccessfully removed from the search path.\n", line.c_str());
	//		}
	//		else
	//		{
	//			printlog("[%s] is removed from the search path.\n", line.c_str());
	//		}
	//	}
	//}
	//gamemods_numCurrentModsLoaded = -1;
	//PHYSFS_freeList(*i);
	//return success;
}

bool gamemodsMountAllExistingPaths()
{
	return true;
//	bool success = true;
//	std::vector<std::pair<std::string, std::string>>::iterator it;
//	for ( it = gamemods_mountedFilepaths.begin(); it != gamemods_mountedFilepaths.end(); ++it )
//	{
//		std::pair<std::string, std::string> itpair = *it;
//		if ( PHYSFS_mount(itpair.first.c_str(), NULL, 0) )
//		{
//			printlog("[%s] is in the search path.\n", itpair.first.c_str());
//		}
//		else
//		{
//			printlog("[%s] unsuccessfully added to search path.\n", itpair.first.c_str());
//			success = false;
//		}
//	}
//	gamemods_numCurrentModsLoaded = gamemods_mountedFilepaths.size();
//	gamemods_customContentLoadedFirstTime = true;
//	return success;
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
			Language::reloadLanguage();
			if ( addToPath )
			{
				gamemods_mountedFilepaths.push_back(std::make_pair(fullpath, modTitle)); // change string to your mod name here.
				gamemods_workshopLoadedFileIDMap.push_back(std::make_pair(modTitle, fileID));
			}
		}
	}
}
#else
size_t serialHash(const std::string& input)
{
	if ( input.empty() || input.size() != 19 )
	{
		return 0;
	}
	int i = 0;
	size_t hash = 0;
	for ( auto it : input )
	{
		if ( it == '\0' || it == '\n' )
		{
			break;
		}
		hash += static_cast<size_t>(it) * (i * i);
		++i;
	}
	return hash;
}
#endif // STEAMWORKS

LastCreatedCharacter LastCreatedCharacterSettings;
