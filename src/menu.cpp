/*-------------------------------------------------------------------------------

	BARONY
	File: menu.cpp
	Desc: contains code for all menu buttons in the game

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include <list>
#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "messages.hpp"
#include "entity.hpp"
#include "menu.hpp"
#include "classdescriptions.hpp"
#include "interface/interface.hpp"
#include "magic/magic.hpp"
#include "sound.hpp"
#include "items.hpp"
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

#ifdef STEAMWORKS
//Helper func. //TODO: Bugger.
void* cpp_SteamMatchmaking_GetLobbyOwner(void* steamIDLobby)
{
	CSteamID* id = new CSteamID();
	*id = SteamMatchmaking()->GetLobbyOwner(*static_cast<CSteamID*>(steamIDLobby));
	return id; //Still don't like this method.
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
int settings_xres, settings_yres;

typedef std::tuple<int, int> resolution;
std::list<resolution> resolutions;
Uint32 settings_fov;
Uint32 settings_fps;
bool settings_smoothlighting;
int settings_fullscreen, settings_shaking, settings_bobbing;
real_t settings_gamma;
int settings_sfxvolume, settings_musvolume;
int settings_impulses[NUMIMPULSES];
int settings_joyimpulses[NUM_JOY_IMPULSES];
int settings_reversemouse;
real_t settings_mousespeed;
bool settings_broadcast;
bool settings_nohud;
bool settings_colorblind;
bool settings_spawn_blood;
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
bool settings_disable_messages = true;
bool settings_right_click_protect = false;
bool settings_auto_appraise_new_items = true;
bool playing_random_char = false;
bool colorblind = false;
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
real_t drunkextend = 0;
bool losingConnection[4] = { false };
bool subtitleVisible = false;
int subtitleCurrent = 0;

//Confirm resolution window stuff.
bool resolutionChanged = false;
bool confirmResolutionWindow = false;
int resolutionConfirmationTimer = 0;
Sint32 oldXres;
Sint32 oldYres;
Sint32 oldFullscreen;
real_t oldGamma;
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
		SDL_WarpMouseInWindow(screen, x, y);
	}
}

void navigateMainMenuItems(bool mode)
{
	int warpx, warpy;
	if (menuselect == 0)
	{
		//No menu item selected.
		if ( keystatus[SDL_SCANCODE_UP] || (*inputPressed(joyimpulses[INJOY_DPAD_UP]) && rebindaction == -1) )
		{
			keystatus[SDL_SCANCODE_UP] = 0;
			if ( rebindaction == -1 )
			{
				*inputPressed(joyimpulses[INJOY_DPAD_UP]) = 0;
			}
			draw_cursor = false;
			menuselect = 1;
			//Warp cursor to menu item, for gamepad convenience.
			warpx = 50 + 18;
			warpy = (yres / 4) + 80 + (18 / 2); //I am a wizard. I hate magic numbers.
			SDL_WarpMouseInWindow(screen, warpx, warpy);
		}
		else if ( keystatus[SDL_SCANCODE_DOWN] || (*inputPressed(joyimpulses[INJOY_DPAD_DOWN]) && rebindaction == -1) )
		{
			keystatus[SDL_SCANCODE_DOWN] = 0;
			if ( rebindaction == -1 )
			{
				*inputPressed(joyimpulses[INJOY_DPAD_DOWN]) = 0;
			}
			draw_cursor = false;
			menuselect = 1;
			warpx = 50 + 18;
			warpy = (yres / 4) + 80 + (18 / 2);
			SDL_WarpMouseInWindow(screen, warpx, warpy);
		}
	}
	else
	{
		if ( keystatus[SDL_SCANCODE_UP] || (*inputPressed(joyimpulses[INJOY_DPAD_UP]) && rebindaction == -1) )
		{
			keystatus[SDL_SCANCODE_UP] = 0;
			if ( rebindaction == -1 )
			{
				*inputPressed(joyimpulses[INJOY_DPAD_UP]) = 0;
			}
			draw_cursor = false;
			menuselect--;
			if (menuselect == 0)
			{
				if (mode)
				{
					menuselect = 6;
				}
				else
				{
					menuselect = 4 + (multiplayer != CLIENT);
				}
			}

			warpx = 50 + 18;
			warpy = (((yres / 4) + 80 + (18 / 2)) + ((menuselect - 1) * 24));
			SDL_WarpMouseInWindow(screen, warpx, warpy);
		}
		else if (keystatus[SDL_SCANCODE_DOWN] || (*inputPressed(joyimpulses[INJOY_DPAD_DOWN]) && rebindaction == -1) )
		{
			keystatus[SDL_SCANCODE_DOWN] = 0;
			if ( rebindaction == -1 )
			{
				*inputPressed(joyimpulses[INJOY_DPAD_DOWN]) = 0;
			}
			draw_cursor = false;
			menuselect++;
			if (mode)
			{
				if (menuselect > 6)
				{
					menuselect = 1;
				}
			}
			else
			{
				if (menuselect > 4 + ( multiplayer != CLIENT))
				{
					menuselect = 1;
				}
			}

			warpx = 50 + 18;
			warpy = (((yres / 4) + 80 + (18 / 2)) + ((menuselect - 1) * 24));
			SDL_WarpMouseInWindow(screen, warpx, warpy);
		}
	}
}

void inline printJoybindingNames(const SDL_Rect& currentPos, int c, bool &rebindingaction)
{
	ttfPrintText(ttf8, currentPos.x, currentPos.y, language[1948 + c]);
	if ( mousestatus[SDL_BUTTON_LEFT] && !rebindingaction )
	{
		if ( omousex >= currentPos.x && omousex < subx2 - 24 )
		{
			if ( omousey >= currentPos.y && omousey < currentPos.y + 12 )
			{
				mousestatus[SDL_BUTTON_LEFT] = 0;
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

/*-------------------------------------------------------------------------------

	handleMainMenu

	draws & processes the game menu; if passed true, does the whole menu,
	otherwise just handles the reduced ingame menu

-------------------------------------------------------------------------------*/

void handleMainMenu(bool mode)
{
	SDL_Rect pos, src, dest;
	int x, c;
	//int y;
	bool b;
	//int tilesreceived=0;
	//Mix_Music **music, *intromusic, *splashmusic, *creditsmusic;
	node_t* node, *nextnode;
	Entity* entity;
	FILE* fp;
	//SDL_Surface *sky_bmp;
	button_t* button;

	if ( !movie )
	{
		// title pic
		src.x = 0;
		src.y = 0;
		src.w = title_bmp->w;
		src.h = title_bmp->h;
		dest.x = 20;
		dest.y = 20;
		dest.w = xres;
		dest.h = yres;
		if ( mode || introstage != 5 )
		{
			drawImage(title_bmp, &src, &dest);
		}
		if ( mode && subtitleVisible )
		{
			Uint32 colorYellow = SDL_MapRGBA(mainsurface->format, 255, 255, 0, 255);
			ttfPrintTextColor(ttf16, 176, 20 + title_bmp->h - 24, colorYellow, true, language[1910 + subtitleCurrent]);
		}

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
			TTF_SizeUTF8(ttf8, version, &w, &h);
			ttfPrintTextFormatted(ttf8, xres - 8 - w, yres - 4 - h, "%s", version);
			int h2 = h;
			TTF_SizeUTF8(ttf8, VERSION, &w, &h);
			ttfPrintTextFormatted(ttf8, xres - 8 - w, yres - 8 - h - h2, VERSION);

#ifdef STEAMWORKS
			TTF_SizeUTF8(ttf8, language[2570], &w, &h);
			if ( (omousex >= xres - 8 - w && omousex < xres && omousey >= 8 && omousey < 8 + h) 
				&& subwindow == 0 
				&& introstage == 1
				&& SteamUser()->BLoggedOn() )
			{
				if ( mousestatus[SDL_BUTTON_LEFT] )
				{
					mousestatus[SDL_BUTTON_LEFT] = 0;
					playSound(139, 64);
					SteamFriends()->ActivateGameOverlayToWebPage(language[2570]);
				}
				ttfPrintTextFormattedColor(ttf8, xres - 8 - w, 8, colorGray, language[2570]);
			}
			else
			{
				ttfPrintText(ttf8, xres - 8 - w, 8, language[2570]);
			}
			h2 = h;
			TTF_SizeUTF8(ttf8, language[2549], &w, &h);
			if ( (omousex >= xres - 8 - w && omousex < xres && omousey >= 8 + h2 && omousey < 8 + h + h2) 
				&& subwindow == 0 
				&& introstage == 1
				&& SteamUser()->BLoggedOn() )
			{
				if ( mousestatus[SDL_BUTTON_LEFT] )
				{
					mousestatus[SDL_BUTTON_LEFT] = 0;
					playSound(139, 64);
					SteamAPICall_NumPlayersOnline = SteamUserStats()->GetNumberOfCurrentPlayers();
				}
				ttfPrintTextFormattedColor(ttf8, xres - 8 - w, 8 + h2, colorGray, language[2549], steamOnlinePlayers);
			}
			else if ( SteamUser()->BLoggedOn() )
			{
				ttfPrintTextFormatted(ttf8, xres - 8 - w, 8 + h2, language[2549], steamOnlinePlayers);
			}
			if ( SteamUser()->BLoggedOn() && SteamAPICall_NumPlayersOnline == 0 && ticks % 250 == 0 )
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

			/*
			bool mouseover = false;
			if ( ((omousex >= 50 && omousex < 50 + strlen(language[1303]) * 18 && omousey >= yres / 4 + 80 && omousey < yres / 4 + 80 + 18)) ) {
				//Mouse hovering over a menu item.
				mouseover = true;
				menuselect = 1;
			}

			if ( (mouseover || (menuselect == 1)) && subwindow == 0 && introstage == 1 ) {
				ttfPrintTextFormattedColor(ttf16, 50, yres/4+80, colorGray, language[1303]);
				//...etc
			 */
			if ( keystatus[SDL_SCANCODE_L] && (keystatus[SDL_SCANCODE_LCTRL] || keystatus[SDL_SCANCODE_RCTRL]) )
			{
				buttonOpenCharacterCreationWindow(nullptr);
				keystatus[SDL_SCANCODE_L] = 0;
				keystatus[SDL_SCANCODE_LCTRL] = 0;
				keystatus[SDL_SCANCODE_RCTRL] = 0;
				multiplayerselect = SERVER;
				charcreation_step = 6;
				directConnect = true;
				strcpy(portnumber_char, "12345");
				buttonHostLobby(nullptr);
			}

			if ( keystatus[SDL_SCANCODE_M] && (keystatus[SDL_SCANCODE_LCTRL] || keystatus[SDL_SCANCODE_RCTRL]) )
			{
				buttonOpenCharacterCreationWindow(nullptr);
				
				keystatus[SDL_SCANCODE_M] = 0;
				keystatus[SDL_SCANCODE_LCTRL] = 0;
				keystatus[SDL_SCANCODE_RCTRL] = 0;
				multiplayerselect = CLIENT;
				charcreation_step = 6;
				directConnect = true;
				strcpy(connectaddress, "localhost:12345");
				buttonJoinLobby(nullptr);
			}

			//"Start Game" button.
			if ( ((omousex >= 50 && omousex < 50 + strlen(language[1303]) * 18 && omousey >= yres / 4 + 80 && omousey < yres / 4 + 80 + 18) || (menuselect == 1)) && subwindow == 0 && introstage == 1 )
			{
				menuselect = 1;
				ttfPrintTextFormattedColor(ttf16, 50, yres / 4 + 80, colorGray, language[1303]);
				if ( mousestatus[SDL_BUTTON_LEFT] || keystatus[SDL_SCANCODE_RETURN] || (*inputPressed(joyimpulses[INJOY_MENU_NEXT]) && rebindaction == -1) )
				{
					mousestatus[SDL_BUTTON_LEFT] = 0;
					keystatus[SDL_SCANCODE_RETURN] = 0;
					if ( rebindaction == -1 )
					{
						*inputPressed(joyimpulses[INJOY_MENU_NEXT]) = 0;
					}
					playSound(139, 64);

					// look for a save game
					if ( saveGameExists() )
					{
						openLoadGameWindow(NULL);
					}
					else
					{
						buttonOpenCharacterCreationWindow(NULL);
					}
				}
			}
			else
			{
				ttfPrintText(ttf16, 50, yres / 4 + 80, language[1303]);
			}
			//"Introduction" button.
			if ( ((omousex >= 50 && omousex < 50 + strlen(language[1304]) * 18 && omousey >= yres / 4 + 104 && omousey < yres / 4 + 104 + 18) || (menuselect == 2)) && subwindow == 0 && introstage == 1 )
			{
				menuselect = 2;
				ttfPrintTextFormattedColor(ttf16, 50, yres / 4 + 104, colorGray, language[1304]);
				if ( mousestatus[SDL_BUTTON_LEFT] || keystatus[SDL_SCANCODE_RETURN] || (*inputPressed(joyimpulses[INJOY_MENU_NEXT]) && rebindaction == -1) )
				{
					mousestatus[SDL_BUTTON_LEFT] = 0;
					keystatus[SDL_SCANCODE_RETURN] = 0;
					if ( rebindaction == -1 )
					{
						*inputPressed(joyimpulses[INJOY_MENU_NEXT]) = 0;
					}
					playSound(139, 64);
					introstage = 6; // goes to intro movie
					fadeout = true;
#ifdef MUSIC
					playmusic(introductionmusic, true, true, false);
#endif
				}
			}
			else
			{
				ttfPrintText(ttf16, 50, yres / 4 + 104, language[1304]);
			}
			//"Statistics" Button.
			if ( ((omousex >= 50 && omousex < 50 + strlen(language[1305]) * 18 && omousey >= yres / 4 + 128 && omousey < yres / 4 + 128 + 18) || (menuselect == 3)) && subwindow == 0 && introstage == 1 )
			{
				menuselect = 3;
				ttfPrintTextFormattedColor(ttf16, 50, yres / 4 + 128, colorGray, language[1305]);
				if ( mousestatus[SDL_BUTTON_LEFT] || keystatus[SDL_SCANCODE_RETURN] || (*inputPressed(joyimpulses[INJOY_MENU_NEXT]) && rebindaction == -1) )
				{
					mousestatus[SDL_BUTTON_LEFT] = 0;
					keystatus[SDL_SCANCODE_RETURN] = 0;
					if ( rebindaction == -1 )
					{
						*inputPressed(joyimpulses[INJOY_MENU_NEXT]) = 0;
					}
					playSound(139, 64);

					// create statistics window
					clientnum = 0;
					subwindow = 1;
					score_window = 1;
					loadScore(0);
					subx1 = xres / 2 - 400;
					subx2 = xres / 2 + 400;
#ifdef PANDORA
					suby1 = yres / 2 - ((yres==480)?200:240);
					suby2 = yres / 2 + ((yres==480)?200:240);
#else
					suby1 = yres / 2 - 240;
					suby2 = yres / 2 + 240;
#endif
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
				}
			}
			else
			{
				ttfPrintText(ttf16, 50, yres / 4 + 128, language[1305]);
			}
			//"Settings" button.
			if ( ((omousex >= 50 && omousex < 50 + strlen(language[1306]) * 18 && omousey >= yres / 4 + 152 && omousey < yres / 4 + 152 + 18) || (menuselect == 4)) && subwindow == 0 && introstage == 1 )
			{
				menuselect = 4;
				ttfPrintTextFormattedColor(ttf16, 50, yres / 4 + 152, colorGray, language[1306]);
				if ( mousestatus[SDL_BUTTON_LEFT] || keystatus[SDL_SCANCODE_RETURN] || (*inputPressed(joyimpulses[INJOY_MENU_NEXT]) && rebindaction == -1) )
				{
					mousestatus[SDL_BUTTON_LEFT] = 0;
					keystatus[SDL_SCANCODE_RETURN] = 0;
					if ( rebindaction == -1 )
					{
						*inputPressed(joyimpulses[INJOY_MENU_NEXT]) = 0;
					}
					playSound(139, 64);
					openSettingsWindow();
				}
			}
			else
			{
				ttfPrintText(ttf16, 50, yres / 4 + 152, language[1306]);
			}
			//"Credits" button
			if ( ((omousex >= 50 && omousex < 50 + strlen(language[1307]) * 18 && omousey >= yres / 4 + 176 && omousey < yres / 4 + 176 + 18) || (menuselect == 5)) && subwindow == 0 && introstage == 1 )
			{
				menuselect = 5;
				ttfPrintTextFormattedColor(ttf16, 50, yres / 4 + 176, colorGray, language[1307]);
				if ( mousestatus[SDL_BUTTON_LEFT] || keystatus[SDL_SCANCODE_RETURN] || (*inputPressed(joyimpulses[INJOY_MENU_NEXT]) && rebindaction == -1) )
				{
					mousestatus[SDL_BUTTON_LEFT] = 0;
					keystatus[SDL_SCANCODE_RETURN] = 0;
					if ( rebindaction == -1 )
					{
						*inputPressed(joyimpulses[INJOY_MENU_NEXT]) = 0;
					}
					playSound(139, 64);
					introstage = 4; // goes to credits
					fadeout = true;
				}
			}
			else
			{
				ttfPrintText(ttf16, 50, yres / 4 + 176, language[1307]);
			}
			//"Quit" button.
			if ( ((omousex >= 50 && omousex < 50 + strlen(language[1308]) * 18 && omousey >= yres / 4 + 200 && omousey < yres / 4 + 200 + 18) || (menuselect == 6)) && subwindow == 0 && introstage == 1 )
			{
				menuselect = 6;
				ttfPrintTextFormattedColor(ttf16, 50, yres / 4 + 200, colorGray, language[1308]);
				if ( mousestatus[SDL_BUTTON_LEFT] || keystatus[SDL_SCANCODE_RETURN] || (*inputPressed(joyimpulses[INJOY_MENU_NEXT]) && rebindaction == -1) )
				{
					mousestatus[SDL_BUTTON_LEFT] = 0;
					keystatus[SDL_SCANCODE_RETURN] = 0;
					if ( rebindaction == -1 )
					{
						*inputPressed(joyimpulses[INJOY_MENU_NEXT]) = 0;
					}
					playSound(139, 64);

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
				ttfPrintText(ttf16, 50, yres / 4 + 200, language[1308]);
			}
		}
		else
		{
			if ( introstage != 5 )
			{
				if ( ((omousex >= 50 && omousex < 50 + strlen(language[1309]) * 18 && omousey >= yres / 4 + 80 && omousey < yres / 4 + 80 + 18) || (menuselect == 1)) && subwindow == 0 && introstage == 1 )
				{
					menuselect = 1;
					ttfPrintTextFormattedColor(ttf16, 50, yres / 4 + 80, colorGray, language[1309]);
					if ( mousestatus[SDL_BUTTON_LEFT] || keystatus[SDL_SCANCODE_RETURN] || (*inputPressed(joyimpulses[INJOY_MENU_NEXT]) && rebindaction == -1) )
					{
						if ( rebindaction == -1 )
						{
							*inputPressed(joyimpulses[INJOY_MENU_NEXT]) = 0;
						}
						mousestatus[SDL_BUTTON_LEFT] = 0;
						keystatus[SDL_SCANCODE_RETURN] = 0;
						playSound(139, 64);
						pauseGame(1, MAXPLAYERS);
					}
				}
				else
				{
					ttfPrintText(ttf16, 50, yres / 4 + 80, language[1309]);
				}
				if ( ((omousex >= 50 && omousex < 50 + strlen(language[1306]) * 18 && omousey >= yres / 4 + 104 && omousey < yres / 4 + 104 + 18) || (menuselect == 2)) && subwindow == 0 && introstage == 1 )
				{
					menuselect = 2;
					ttfPrintTextFormattedColor(ttf16, 50, yres / 4 + 104, colorGray, language[1306]);
					if ( mousestatus[SDL_BUTTON_LEFT] || keystatus[SDL_SCANCODE_RETURN] || (*inputPressed(joyimpulses[INJOY_MENU_NEXT]) && rebindaction == -1) )
					{
						mousestatus[SDL_BUTTON_LEFT] = 0;
						keystatus[SDL_SCANCODE_RETURN] = 0;
						if ( rebindaction == -1 )
						{
							*inputPressed(joyimpulses[INJOY_MENU_NEXT]) = 0;
						}
						playSound(139, 64);
						openSettingsWindow();
					}
				}
				else
				{
					ttfPrintText(ttf16, 50, yres / 4 + 104, language[1306]);
				}
				char* endgameText = NULL;
				if ( multiplayer == SINGLE )
				{
					endgameText = language[1310];
				}
				else
				{
					endgameText = language[1311];
				}
				if ( ((omousex >= 50 && omousex < 50 + strlen(endgameText) * 18 && omousey >= yres / 4 + 128 && omousey < yres / 4 + 128 + 18) || (menuselect == 3)) && subwindow == 0 && introstage == 1 )
				{
					menuselect = 3;
					ttfPrintTextFormattedColor(ttf16, 50, yres / 4 + 128, colorGray, endgameText);
					if ( mousestatus[SDL_BUTTON_LEFT] || keystatus[SDL_SCANCODE_RETURN] || (*inputPressed(joyimpulses[INJOY_MENU_NEXT]) && rebindaction == -1) )
					{
						mousestatus[SDL_BUTTON_LEFT] = 0;
						keystatus[SDL_SCANCODE_RETURN] = 0;
						if ( rebindaction == -1 )
						{
							*inputPressed(joyimpulses[INJOY_MENU_NEXT]) = 0;
						}
						playSound(139, 64);

						// create confirmation window
						subwindow = 1;
						subx1 = xres / 2 - 140;
						subx2 = xres / 2 + 140;
						suby1 = yres / 2 - 48;
						suby2 = yres / 2 + 48;
						strcpy(subtext, language[1129]);

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
						button->action = &buttonEndGameConfirm;
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
					ttfPrintText(ttf16, 50, yres / 4 + 128, endgameText);
				}
				if ( multiplayer != CLIENT )
				{
					if ( ((omousex >= 50 && omousex < 50 + strlen(language[1312]) * 18 && omousey >= yres / 4 + 152 && omousey < yres / 4 + 152 + 18) || (menuselect == 4)) && subwindow == 0 && introstage == 1 )
					{
						menuselect = 4;
						ttfPrintTextFormattedColor(ttf16, 50, yres / 4 + 152, colorGray, language[1312]);
						if ( mousestatus[SDL_BUTTON_LEFT] || keystatus[SDL_SCANCODE_RETURN] || (*inputPressed(joyimpulses[INJOY_MENU_NEXT]) && rebindaction == -1) )
						{
							mousestatus[SDL_BUTTON_LEFT] = 0;
							keystatus[SDL_SCANCODE_RETURN] = 0;
							if ( rebindaction == -1 )
							{
								*inputPressed(joyimpulses[INJOY_MENU_NEXT]) = 0;
							}
							playSound(139, 64);

							// create confirmation window
							subwindow = 1;
							subx1 = xres / 2 - 164;
							subx2 = xres / 2 + 164;
							suby1 = yres / 2 - 48;
							suby2 = yres / 2 + 48;
							strcpy(subtext, language[1130]);

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
						ttfPrintText(ttf16, 50, yres / 4 + 152, language[1312]);
					}
				}
				if ( ((omousex >= 50 && omousex < 50 + strlen(language[1313]) * 18 && omousey >= yres / 4 + 152 + 24 * (multiplayer != CLIENT) && omousey < yres / 4 + 152 + 18 + 24 * (multiplayer != CLIENT)) || (menuselect == 4 + (multiplayer != CLIENT))) && subwindow == 0 && introstage == 1 )
				{
					menuselect = 4 + (multiplayer != CLIENT);
					ttfPrintTextFormattedColor(ttf16, 50, yres / 4 + 152 + 24 * (multiplayer != CLIENT), colorGray, language[1313]);
					if ( mousestatus[SDL_BUTTON_LEFT] || keystatus[SDL_SCANCODE_RETURN] || (*inputPressed(joyimpulses[INJOY_MENU_NEXT]) && rebindaction == -1) )
					{
						mousestatus[SDL_BUTTON_LEFT] = 0;
						keystatus[SDL_SCANCODE_RETURN] = 0;
						if ( rebindaction == -1 )
						{
							*inputPressed(joyimpulses[INJOY_MENU_NEXT]) = 0;
						}
						playSound(139, 64);

						// create confirmation window
						subwindow = 1;
						subx1 = xres / 2 - 188;
						subx2 = xres / 2 + 188;
						suby1 = yres / 2 - 64;
						suby2 = yres / 2 + 64;
						strcpy(subtext, language[1131]);

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
						button->joykey = joyimpulses[INJOY_MENU_NEXT]; //TODO: Select which button to activate via dpad.

						// no button
						button = newButton();
						strcpy(button->label, language[1315]);
						button->sizex = strlen(language[1315]) * 12 + 8;
						button->sizey = 20;
						button->x = subx1 + (subx2 - subx1) / 2 - button->sizex / 2;
						button->y = suby2 - 28;
						button->action = &buttonQuitNoSaveConfirm;
						button->visible = 1;
						button->focused = 1;

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
					ttfPrintText(ttf16, 50, yres / 4 + 152 + 24 * (multiplayer != CLIENT), language[1313]);
				}
			}
		}

#ifdef STEAMWORKS
		if ( intro )
		{
			// lobby list request succeeded
			if ( !requestingLobbies && !strcmp(subtext, language[1132]) )
			{
				openSteamLobbyBrowserWindow(NULL);
			}

			// lobby entered
			if ( !connectingToLobby && connectingToLobbyWindow )
			{
				connectingToLobbyWindow = false;
				connectingToLobby = false;

				// close current window
				buttonCloseSubwindow(NULL);
				list_FreeAll(&button_l);
				deleteallbuttons = true;

				// we are assuming here that the lobby join was successful
				// otherwise, the callback would've flipped off the connectingToLobbyWindow and opened an error window

				// get number of lobby members (capped to game limit)

				// record CSteamID of lobby owner (and nobody else)
				SteamMatchmaking()->GetNumLobbyMembers(*static_cast<CSteamID*>(currentLobby));
				if ( steamIDRemote[0] )
				{
					cpp_Free_CSteamID(steamIDRemote[0]);
				}
				steamIDRemote[0] = cpp_SteamMatchmaking_GetLobbyOwner(currentLobby); //TODO: Bugger void pointers!
				int c;
				for ( c = 1; c < MAXPLAYERS; c++ )
				{
					if ( steamIDRemote[c] )
					{
						cpp_Free_CSteamID(steamIDRemote[c]);
						steamIDRemote[c] = NULL;
					}
				}

				buttonJoinLobby(NULL);
			}
		}
#endif

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
		}

		// process button actions
		handleButtons();
	}

	// character creation screen
	if ( charcreation_step >= 1 && charcreation_step < 6 )
	{
		ttfPrintText(ttf16, subx1 + 8, suby1 + 8, language[1318]);

		// draw character window
		if (players[clientnum] != nullptr && players[clientnum]->entity != nullptr)
		{
			camera_charsheet.x = players[clientnum]->entity->x / 16.0 + 1;
			camera_charsheet.y = players[clientnum]->entity->y / 16.0 - .5;
			camera_charsheet.z = players[clientnum]->entity->z * 2;
			camera_charsheet.ang = atan2(players[clientnum]->entity->y / 16.0 - camera_charsheet.y, players[clientnum]->entity->x / 16.0 - camera_charsheet.x);
			camera_charsheet.vang = PI / 24;
			camera_charsheet.winw = 360;
			camera_charsheet.winy = suby1 + 32;
			camera_charsheet.winh = suby2 - 96 - camera_charsheet.winy;
			camera_charsheet.winx = subx2 - camera_charsheet.winw - 32;
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
		}

		// sexes
		if ( charcreation_step == 1 )
		{
			ttfPrintText(ttf16, subx1 + 24, suby1 + 32, language[1319]);
			if ( stats[0]->sex == 0 )
			{
				ttfPrintTextFormatted(ttf16, subx1 + 32, suby1 + 56, "[o] %s", language[1321]);
				ttfPrintTextFormatted(ttf16, subx1 + 32, suby1 + 72, "[ ] %s", language[1322]);

				ttfPrintTextFormatted(ttf12, subx1 + 8, suby2 - 80, language[1320], language[1321]);
			}
			else
			{
				ttfPrintTextFormatted(ttf16, subx1 + 32, suby1 + 56, "[ ] %s", language[1321]);
				ttfPrintTextFormatted(ttf16, subx1 + 32, suby1 + 72, "[o] %s", language[1322]);

				ttfPrintTextFormatted(ttf12, subx1 + 8, suby2 - 80, language[1320], language[1322]);
			}
			if ( mousestatus[SDL_BUTTON_LEFT] )
			{
				if ( omousex >= subx1 + 40 && omousex < subx1 + 72 )
				{
					if ( omousey >= suby1 + 56 && omousey < suby1 + 72 )
					{
						mousestatus[SDL_BUTTON_LEFT] = 0;
						stats[0]->sex = MALE;
					}
					else if ( omousey >= suby1 + 72 && omousey < suby1 + 88 )
					{
						mousestatus[SDL_BUTTON_LEFT] = 0;
						stats[0]->sex = FEMALE;
					}
				}
			}
			if ( keystatus[SDL_SCANCODE_UP] || (*inputPressed(joyimpulses[INJOY_DPAD_UP]) && rebindaction == -1) )
			{
				keystatus[SDL_SCANCODE_UP] = 0;
				if ( rebindaction == -1 )
				{
					*inputPressed(joyimpulses[INJOY_DPAD_UP]) = 0;
				}
				draw_cursor = false;
				stats[0]->sex = static_cast<sex_t>((stats[0]->sex == MALE));
			}
			if ( keystatus[SDL_SCANCODE_DOWN] || (*inputPressed(joyimpulses[INJOY_DPAD_DOWN]) && rebindaction == -1) )
			{
				keystatus[SDL_SCANCODE_DOWN] = 0;
				if ( rebindaction == -1 )
				{
					*inputPressed(joyimpulses[INJOY_DPAD_DOWN]) = 0;
				}
				draw_cursor = false;
				stats[0]->sex = static_cast<sex_t>((stats[0]->sex == MALE));
			}
		}

		// classes
		else if ( charcreation_step == 2 )
		{
			ttfPrintText(ttf16, subx1 + 24, suby1 + 32, language[1323]);
			for ( c = 0; c < NUMCLASSES; c++ )
			{
				if ( c == client_classes[0] )
				{
					ttfPrintTextFormatted(ttf16, subx1 + 32, suby1 + 56 + 16 * c, "[o] %s", playerClassLangEntry(c));
				}
				else
				{
					ttfPrintTextFormatted(ttf16, subx1 + 32, suby1 + 56 + 16 * c, "[ ] %s", playerClassLangEntry(c));
				}

				if ( mousestatus[SDL_BUTTON_LEFT] )
				{
					if ( omousex >= subx1 + 40 && omousex < subx1 + 72 )
					{
						if ( omousey >= suby1 + 56 + 16 * c && omousey < suby1 + 72 + 16 * c )
						{
							mousestatus[SDL_BUTTON_LEFT] = 0;
							client_classes[0] = c;

							// reset class loadout
							stats[0]->clearStats();
							initClass(0);
						}
					}
				}
				if ( keystatus[SDL_SCANCODE_UP] || (*inputPressed(joyimpulses[INJOY_DPAD_UP]) && rebindaction == -1) )
				{
					keystatus[SDL_SCANCODE_UP] = 0;
					if ( rebindaction == -1 )
					{
						*inputPressed(joyimpulses[INJOY_DPAD_UP]) = 0;
					}
					draw_cursor = false;
					client_classes[0]--;
					if (client_classes[0] < 0)
					{
						client_classes[0] = NUMCLASSES - 1;
					}

					// reset class loadout
					stats[0]->clearStats();
					initClass(0);
				}
				if ( keystatus[SDL_SCANCODE_DOWN] || (*inputPressed(joyimpulses[INJOY_DPAD_DOWN]) && rebindaction == -1) )
				{
					keystatus[SDL_SCANCODE_DOWN] = 0;
					if ( rebindaction == -1 )
					{
						*inputPressed(joyimpulses[INJOY_DPAD_DOWN]) = 0;
					}
					draw_cursor = false;
					client_classes[0]++;
					if ( client_classes[0] > NUMCLASSES - 1 )
					{
						client_classes[0] = 0;
					}

					// reset class loadout
					stats[0]->clearStats();
					initClass(0);
				}
			}

			// class description
			ttfPrintText(ttf12, subx1 + 8, suby2 - 80, playerClassDescription(client_classes[0]));
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
				if ( mousestatus[SDL_BUTTON_LEFT] )
				{
					if ( omousex >= subx1 + 40 && omousex < subx1 + 72 )
					{
						if ( omousey >= suby1 + 56 + 16 * c && omousey < suby1 + 72 + 16 * c )
						{
							mousestatus[SDL_BUTTON_LEFT] = 0;
							stats[0]->appearance = c;
						}
					}
				}
				if ( keystatus[SDL_SCANCODE_UP] || (*inputPressed(joyimpulses[INJOY_DPAD_UP]) && rebindaction == -1) )
				{
					keystatus[SDL_SCANCODE_UP] = 0;
					if ( rebindaction == -1 )
					{
						*inputPressed(joyimpulses[INJOY_DPAD_UP]) = 0;
					}
					draw_cursor = false;
					stats[0]->appearance--;
					if (stats[0]->appearance >= NUMAPPEARANCES)
					{
						stats[0]->appearance = NUMAPPEARANCES - 1;
					}
				}
				if ( keystatus[SDL_SCANCODE_DOWN] || (*inputPressed(joyimpulses[INJOY_DPAD_DOWN]) && rebindaction == -1) )
				{
					keystatus[SDL_SCANCODE_DOWN] = 0;
					if ( rebindaction == -1 )
					{
						*inputPressed(joyimpulses[INJOY_DPAD_DOWN]) = 0;
					}
					draw_cursor = false;
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
				inputstr = stats[0]->name;
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
				TTF_SizeUTF8(ttf16, stats[0]->name, &x, NULL);
				ttfPrintText(ttf16, subx1 + 48 + x, suby1 + 64, "_");
			}
		}

		// gamemode
		else if ( charcreation_step == 5 )
		{
			ttfPrintText(ttf16, subx1 + 24, suby1 + 32, language[1327]);

			int nummodes = 3;
#ifdef STEAMWORKS
			nummodes += 2;
#endif

			for ( c = 0; c < nummodes; c++ )
			{
				if ( multiplayerselect == c )
				{
					switch ( c )
					{
						case 0:
							ttfPrintTextFormatted(ttf16, subx1 + 32, suby1 + 56, "[o] %s", language[1328]);
							ttfPrintText(ttf12, subx1 + 8, suby2 - 80, language[1329]);
							break;
						case 1:
							ttfPrintTextFormatted(ttf16, subx1 + 32, suby1 + 76, "[o] %s", language[1330]);
							ttfPrintText(ttf12, subx1 + 8, suby2 - 80, language[1331]);
							break;
						case 2:
							ttfPrintTextFormatted(ttf16, subx1 + 32, suby1 + 96, "[o] %s", language[1332]);
							ttfPrintText(ttf12, subx1 + 8, suby2 - 80, language[1333]);
							break;
						case 3:
							ttfPrintTextFormatted(ttf16, subx1 + 32, suby1 + 136, "[o] %s\n     %s", language[1330], language[1537]);
							ttfPrintText(ttf12, subx1 + 8, suby2 - 80, language[1538]);
							break;
						case 4:
							ttfPrintTextFormatted(ttf16, subx1 + 32, suby1 + 176, "[o] %s\n     %s", language[1332], language[1537]);
							ttfPrintText(ttf12, subx1 + 8, suby2 - 80, language[1539]);
							break;
					}
				}
				else
				{
					switch ( c )
					{
						case 0:
							ttfPrintTextFormatted(ttf16, subx1 + 32, suby1 + 56, "[ ] %s", language[1328]);
							break;
						case 1:
							ttfPrintTextFormatted(ttf16, subx1 + 32, suby1 + 76, "[ ] %s", language[1330]);
							break;
						case 2:
							ttfPrintTextFormatted(ttf16, subx1 + 32, suby1 + 96, "[ ] %s", language[1332]);
							break;
						case 3:
							ttfPrintTextFormatted(ttf16, subx1 + 32, suby1 + 136, "[ ] %s\n     %s", language[1330], language[1537]);
							break;
						case 4:
							ttfPrintTextFormatted(ttf16, subx1 + 32, suby1 + 176, "[ ] %s\n     %s", language[1332], language[1537]);
							break;
					}
				}
				if ( mousestatus[SDL_BUTTON_LEFT] )
				{
					if ( omousex >= subx1 + 40 && omousex < subx1 + 72 )
					{
						if ( c < 3 )
						{
							if ( omousey >= suby1 + 56 + 20 * c && omousey < suby1 + 74 + 20 * c )
							{
								mousestatus[SDL_BUTTON_LEFT] = 0;
								multiplayerselect = c;
							}
						}
						else
						{
							if ( omousey >= suby1 + 136 + 40 * (c - 3) && omousey < suby1 + 148 + 40 * (c - 3) )
							{
								mousestatus[SDL_BUTTON_LEFT] = 0;
								multiplayerselect = c;
							}
						}
					}
				}
				if (keystatus[SDL_SCANCODE_UP] || (*inputPressed(joyimpulses[INJOY_DPAD_UP]) && rebindaction == -1) )
				{
					keystatus[SDL_SCANCODE_UP] = 0;
					if ( rebindaction == -1 )
					{
						*inputPressed(joyimpulses[INJOY_DPAD_UP]) = 0;
					}
					draw_cursor = false;
					multiplayerselect--;
					if (multiplayerselect < 0)
					{
						multiplayerselect = nummodes - 1;
					}
				}
				if ( keystatus[SDL_SCANCODE_DOWN] || (*inputPressed(joyimpulses[INJOY_DPAD_DOWN]) && rebindaction == -1) )
				{
					keystatus[SDL_SCANCODE_DOWN] = 0;
					if ( rebindaction == -1 )
					{
						*inputPressed(joyimpulses[INJOY_DPAD_DOWN]) = 0;
					}
					draw_cursor = false;
					multiplayerselect++;
					if (multiplayerselect > nummodes - 1)
					{
						multiplayerselect = 0;
					}
				}
			}
		}
	}

	// steam lobby browser
#ifdef STEAMWORKS
	if ( subwindow && !strcmp(subtext, language[1334]) )
	{
		drawDepressed(subx1 + 8, suby1 + 24, subx2 - 32, suby2 - 64);
		drawDepressed(subx2 - 32, suby1 + 24, subx2 - 8, suby2 - 64);

		// slider
		slidersize = std::min<int>(((suby2 - 65) - (suby1 + 25)), ((suby2 - 65) - (suby1 + 25)) / ((real_t)std::max(numSteamLobbies + 1, 1) / 20));
		slidery = std::min(std::max(suby1 + 25, slidery), suby2 - 65 - slidersize);
		drawWindowFancy(subx2 - 31, slidery, subx2 - 9, slidery + slidersize);

		// directory list offset from slider
		Sint32 y2 = ((real_t)(slidery - suby1 - 20) / ((suby2 - 52) - (suby1 + 20))) * (numSteamLobbies + 1);
		if ( mousestatus[SDL_BUTTON_LEFT] && omousex >= subx2 - 32 && omousex < subx2 - 8 && omousey >= suby1 + 24 && omousey < suby2 - 64 )
		{
			slidery = oslidery + mousey - omousey;
		}
		else if ( mousestatus[SDL_BUTTON_WHEELUP] || mousestatus[SDL_BUTTON_WHEELDOWN] )
		{
			slidery += 16 * mousestatus[SDL_BUTTON_WHEELDOWN] - 16 * mousestatus[SDL_BUTTON_WHEELUP];
			mousestatus[SDL_BUTTON_WHEELUP] = 0;
			mousestatus[SDL_BUTTON_WHEELDOWN] = 0;
		}
		else
		{
			oslidery = slidery;
		}
		slidery = std::min(std::max(suby1 + 25, slidery), suby2 - 65 - slidersize);
		y2 = ((real_t)(slidery - suby1 - 20) / ((suby2 - 52) - (suby1 + 20))) * (numSteamLobbies + 1);

		// server flags tooltip variables
		SDL_Rect flagsBox;
		char flagsBoxText[256];
		int hoveringSelection = -1;

		// select/inspect lobbies
		if ( omousex >= subx1 + 8 && omousex < subx2 - 32 && omousey >= suby1 + 26 && omousey < suby2 - 64 )
		{
			hoveringSelection = std::min(std::max(0, y2 + ((omousey - suby1 - 24) >> 4)), MAX_STEAM_LOBBIES);

			// lobby info tooltip
			if ( lobbyIDs[hoveringSelection] )
			{
				const char* lobbySvFlagsChar = SteamMatchmaking()->GetLobbyData( *static_cast<CSteamID*>(lobbyIDs[hoveringSelection]), "svFlags" );
				Uint32 lobbySvFlags = atoi(lobbySvFlagsChar);

				int numSvFlags = 0, c;
				for ( c = 0; c < NUM_SERVER_FLAGS; c++ )
				{
					if ( lobbySvFlags & power(2, c) )
					{
						numSvFlags++;
					}
				}

				flagsBox.x = mousex + 8;
				flagsBox.y = mousey + 8;
				flagsBox.w = strlen(language[1335]) * 12 + 4;
				flagsBox.h = 16 + 12 * std::max(2, numSvFlags + 1);
				strcpy(flagsBoxText, language[1335]);
				strcat(flagsBoxText, "\n");

				if ( !numSvFlags )
				{
					strcat(flagsBoxText, language[1336]);
				}
				else
				{
					int y = 2;
					for ( c = 0; c < NUM_SERVER_FLAGS; c++ )
					{
						if ( lobbySvFlags & power(2, c) )
						{
							y += 12;
							strcat(flagsBoxText, "\n");
							strcat(flagsBoxText, language[153 + c]);
						}
					}
				}
			}

			// selecting lobby
			if ( mousestatus[SDL_BUTTON_LEFT] )
			{
				mousestatus[SDL_BUTTON_LEFT] = 0;
				selectedSteamLobby = hoveringSelection;
			}
		}
		selectedSteamLobby = std::min(std::max(y2, selectedSteamLobby), std::min(std::max(numSteamLobbies - 1, 0), y2 + 17));
		pos.x = subx1 + 10;
		pos.y = suby1 + 26 + (selectedSteamLobby - y2) * 16;
		pos.w = subx2 - subx1 - 44;
		pos.h = 16;
		drawRect(&pos, SDL_MapRGB(mainsurface->format, 64, 64, 64), 255);

		// print all lobby entries
		Sint32 x = subx1 + 10;
		Sint32 y = suby1 + 28;
		if ( numSteamLobbies > 0 )
		{
			Sint32 z;
			c = std::min(numSteamLobbies, 18 + y2);
			for (z = y2; z < c; z++)
			{
				ttfPrintTextFormatted(ttf12, x, y, lobbyText[z]); // name
				ttfPrintTextFormatted(ttf12, subx2 - 72, y, "%d/4", lobbyPlayers[z]); // player count
				y += 16;
			}
		}
		else
		{
			ttfPrintText(ttf12, x, y, language[1337]);
		}

		// draw server flags tooltip (if applicable)
		if ( hoveringSelection >= 0 && numSteamLobbies > 0 )
		{
			drawTooltip(&flagsBox);
			ttfPrintTextFormatted(ttf12, flagsBox.x + 2, flagsBox.y + 2, flagsBoxText);
		}
	}
#endif

	// settings window
	if ( settings_window == true )
	{
		drawWindowFancy(subx1 + 16, suby1 + 44, subx2 - 16, suby2 - 32);

		int hovering_selection = -1; //0 to NUM_SERVER_FLAGS used for the game flags settings, e.g. are traps enabled, are cheats enabled, is minotaur enabled, etc.
		SDL_Rect tooltip_box;

		if ( *inputPressed(joyimpulses[INJOY_MENU_SETTINGS_NEXT]) && rebindaction == -1 )
		{
			*inputPressed(joyimpulses[INJOY_MENU_SETTINGS_NEXT]) = 0;;
			changeSettingsTab(settings_tab + 1);
		}
		if ( *inputPressed(joyimpulses[INJOY_MENU_SETTINGS_PREV]) && rebindaction == -1 )
		{
			*inputPressed(joyimpulses[INJOY_MENU_SETTINGS_PREV]) = 0;
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
				if ( mousestatus[SDL_BUTTON_LEFT] )
				{
					if ( omousex >= subx1 + 38 && omousex < subx1 + 62 )
					{
						if ( omousey >= suby1 + 84 + c * 16 && omousey < suby1 + 96 + c * 16 )
						{
							mousestatus[SDL_BUTTON_LEFT] = 0;
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

			if ( mousestatus[SDL_BUTTON_LEFT] )
			{
				if ( omousex >= subx1 + 242 && omousex < subx1 + 266 )
				{
					if ( omousey >= suby1 + 84 && omousey < suby1 + 84 + 12 )
					{
						mousestatus[SDL_BUTTON_LEFT] = 0;
						settings_smoothlighting = (settings_smoothlighting == 0);
					}
					else if ( omousey >= suby1 + 108 && omousey < suby1 + 108 + 12 )
					{
						mousestatus[SDL_BUTTON_LEFT] = 0;
						settings_fullscreen = (settings_fullscreen == 0);
					}
					else if ( omousey >= suby1 + 132 && omousey < suby1 + 132 + 12 )
					{
						mousestatus[SDL_BUTTON_LEFT] = 0;
						settings_shaking = (settings_shaking == 0);
					}
					else if ( omousey >= suby1 + 156 && omousey < suby1 + 156 + 12 )
					{
						mousestatus[SDL_BUTTON_LEFT] = 0;
						settings_bobbing = (settings_bobbing == 0);
					}
					else if ( omousey >= suby1 + 180 && omousey < suby1 + 180 + 12 )
					{
						mousestatus[SDL_BUTTON_LEFT] = 0;
						settings_spawn_blood = (settings_spawn_blood == 0);
					}
					else if ( omousey >= suby1 + 204 && omousey < suby1 + 204 + 12 )
					{
						mousestatus[SDL_BUTTON_LEFT] = 0;
						settings_colorblind = (settings_colorblind == false);
					}
				}
			}

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
			ttfPrintText(ttf12, subx1 + 24, suby1 + 108, language[1349]);
			doSlider(subx1 + 24, suby1 + 132, 15, 0, 128, 0, &settings_musvolume);
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

			int c;
			for ( c = 0; c < NUMIMPULSES; c++ )
			{
				if ( c < 14 )
				{
					ttfPrintText(ttf12, subx1 + 24, suby1 + 84 + 16 * c, language[1351 + c]);
				}
				else
				{
					ttfPrintText(ttf12, subx1 + 24, suby1 + 84 + 16 * c, language[1940 + (c - 14)]);
				}
				if ( mousestatus[SDL_BUTTON_LEFT] && !rebindingkey )
				{
					if ( omousex >= subx1 + 24 && omousex < subx2 - 24 )
					{
						if ( omousey >= suby1 + 84 + c * 16 && omousey < suby1 + 96 + c * 16 )
						{
							mousestatus[SDL_BUTTON_LEFT] = 0;
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
						mousestatus[SDL_BUTTON_LEFT] = 0;  // fixes mouse-left not registering bug
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
			if ( mousestatus[SDL_BUTTON_LEFT] )
			{
				if ( omousex >= subx1 + 30 && omousex < subx1 + 54 )
				{
					if ( omousey >= suby1 + 108 && omousey < suby1 + 120 )
					{
						mousestatus[SDL_BUTTON_LEFT] = 0;
						settings_reversemouse = (settings_reversemouse == 0);
					}
					if ( omousey >= suby1 + 132 && omousey < suby1 + 144 )
					{
						mousestatus[SDL_BUTTON_LEFT] = 0;
						settings_smoothmouse = (settings_smoothmouse == 0);
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
			ttfPrintText(ttf8, currentPos.x, currentPos.y, language[1992]);
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
			ttfPrintText(ttf8, currentPos.x, currentPos.y, language[1990]);
			currentPos.y += 18;
			for ( c = INDEX_JOYBINDINGS_START_MENU; c < INDEX_JOYBINDINGS_START_GAME; ++c, currentPos.y += 12 )
			{
				printJoybindingNames(currentPos, c, rebindingaction);
			}

			//Print out the game-exclusive bindings.
			currentPos.y += 12;
			drawLine(subx1 + 24, currentPos.y - 6, subx2 - 24, currentPos.y - 6, uint32ColorGray(*mainsurface), 255);
			ttfPrintText(ttf8, currentPos.x, currentPos.y, language[1991]);
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

			if (mousestatus[SDL_BUTTON_LEFT] && mouseInBounds(current_option_x, current_option_x + strlen("[x]")*TTF12_WIDTH, current_option_y, current_option_y + TTF12_HEIGHT))
			{
				mousestatus[SDL_BUTTON_LEFT] = 0;
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

			if (mousestatus[SDL_BUTTON_LEFT] && mouseInBounds(current_option_x, current_option_x + strlen("[x]")*TTF12_WIDTH, current_option_y, current_option_y + TTF12_HEIGHT))
			{
				mousestatus[SDL_BUTTON_LEFT] = 0;
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

			if (mousestatus[SDL_BUTTON_LEFT] && mouseInBounds(current_option_x, current_option_x + strlen("[x]")*TTF12_WIDTH, current_option_y, current_option_y + TTF12_HEIGHT))
			{
				mousestatus[SDL_BUTTON_LEFT] = 0;
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

			if (mousestatus[SDL_BUTTON_LEFT] && mouseInBounds(current_option_x, current_option_x + strlen("[x]")*TTF12_WIDTH, current_option_y, current_option_y + TTF12_HEIGHT))
			{
				mousestatus[SDL_BUTTON_LEFT] = 0;
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

			if (mousestatus[SDL_BUTTON_LEFT] && mouseInBounds(current_option_x, current_option_x + strlen("[x]")*TTF12_WIDTH, current_option_y, current_option_y + TTF12_HEIGHT))
			{
				mousestatus[SDL_BUTTON_LEFT] = 0;
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

			if (mousestatus[SDL_BUTTON_LEFT] && mouseInBounds(current_option_x, current_option_x + strlen("[x]")*TTF12_WIDTH, current_option_y, current_option_y + TTF12_HEIGHT))
			{
				mousestatus[SDL_BUTTON_LEFT] = 0;
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
			current_y += 32;

			// server flag elements
			ttfPrintText(ttf12, subx1 + 24, current_y, language[1375]);
			current_y += 24;


			int server_flags_start_y = current_y;
			int i;
			for ( i = 0; i < NUM_SERVER_FLAGS; i++, current_y += 16 )
			{
				if ( svFlags & power(2, i) )
				{
					ttfPrintTextFormatted(ttf12, subx1 + 36, current_y, "[x] %s", language[153 + i]);
				}
				else
				{
					ttfPrintTextFormatted(ttf12, subx1 + 36, current_y, "[ ] %s", language[153 + i]);
				}
				if (mouseInBounds(subx1 + 36 + 6, subx1 + 36 + 24 + 6, current_y, current_y + 12))   //So many gosh dang magic numbers ._.
				{
					if (strlen(language[1942 + i]) > 0)   //Don't bother drawing a tooltip if the file doesn't say anything.
					{
						hovering_selection = i;
						tooltip_box.x = omousex + 16;
						tooltip_box.y = omousey + 8; //I hate magic numbers :|. These should probably be replaced with omousex + mousecursorsprite->width, omousey + mousecursorsprite->height, respectively.
						tooltip_box.w = strlen(language[1942 + i]) * TTF12_WIDTH + 8; //MORE MAGIC NUMBERS. HNNGH. I can guess what they all do, but dang.
						tooltip_box.h = TTF12_HEIGHT + 8;
					}
				}
			}

			if (hovering_selection > -1)
			{
				drawTooltip(&tooltip_box);
				if (hovering_selection < NUM_SERVER_FLAGS)
				{
					ttfPrintTextFormatted(ttf12, tooltip_box.x + 4, tooltip_box.y + 4, language[1942 + hovering_selection]);
				}
			}

			current_y = options_start_y;

			if ( mousestatus[SDL_BUTTON_LEFT] )
			{
				if ( omousex >= subx1 + 42 && omousex < subx1 + 66 )
				{
					if (omousey >= current_y && omousey < current_y + 12)
					{
						mousestatus[SDL_BUTTON_LEFT] = 0;
						settings_broadcast = (settings_broadcast == false);
					}
					else if (omousey >= (current_y += 16) && omousey < current_y + 12)
					{
						mousestatus[SDL_BUTTON_LEFT] = 0;
						settings_nohud = (settings_nohud == false);
					}
					else if (omousey >= (current_y += 16) && omousey < current_y + 12)
					{
						mousestatus[SDL_BUTTON_LEFT] = 0;
						settings_auto_hotbar_new_items = (settings_auto_hotbar_new_items == false);
					}
					else if (omousey >= (current_y += 16) && omousey < current_y + 12)
					{
						mousestatus[SDL_BUTTON_LEFT] = 0;
						settings_auto_appraise_new_items = (settings_auto_appraise_new_items == false);
					}
					else if (omousey >= (current_y += 16) && omousey < current_y + 12)
					{
						mousestatus[SDL_BUTTON_LEFT] = 0;
						settings_disable_messages = (settings_disable_messages == false);
					}
					else if (omousey >= (current_y += 16) && omousey < current_y + 12)
					{
						mousestatus[SDL_BUTTON_LEFT] = 0;
						settings_right_click_protect = (settings_right_click_protect == false);
					}
				}
				else 
				{
					if ( settings_auto_hotbar_new_items )
					{
						if ( mousestatus[SDL_BUTTON_LEFT] )
						{
							for ( i = 0; i < NUM_HOTBAR_CATEGORIES; ++i )
							{
								if ( mouseInBounds(hotbar_options_x, hotbar_options_x + 24, hotbar_options_y, hotbar_options_y + 12) )
								{
									settings_auto_hotbar_categories[i] = !settings_auto_hotbar_categories[i];
									mousestatus[SDL_BUTTON_LEFT] = 0;
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
				}

				if ( multiplayer != CLIENT )
				{
					current_y = server_flags_start_y;
					for (i = 0; i < NUM_SERVER_FLAGS; i++, current_y += 16)
					{
						if ( mouseInBounds(subx1 + 36 + 6, subx1 + 36 + 24 + 6, current_y, current_y + 12) )
						{
							mousestatus[SDL_BUTTON_LEFT] = 0;

							// toggle flag
							svFlags ^= power(2, i);

							if ( multiplayer == SERVER )
							{
								// update client flags
								strcpy((char*)net_packet->data, "SVFL");
								SDLNet_Write32(svFlags, &net_packet->data[4]);
								net_packet->len = 8;

								int c;
								for (c = 1; c < MAXPLAYERS; ++c)
								{
									if (client_disconnected[c])
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
					}
				}
			}
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
				SDL_StartTextInput();
				inputstr = portnumber_char;
			}
			//strncpy(portnumber_char,inputstr,5);
			inputlen = 5;
			if ( (ticks - cursorflash) % TICKS_PER_SECOND < TICKS_PER_SECOND / 2 )
			{
				int x;
				TTF_SizeUTF8(ttf12, portnumber_char, &x, NULL);
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
				inputstr = connectaddress;
			}
			//strncpy(connectaddress,inputstr,31);
			inputlen = 31;
			if ( (ticks - cursorflash) % TICKS_PER_SECOND < TICKS_PER_SECOND / 2 )
			{
				int x;
				TTF_SizeUTF8(ttf12, connectaddress, &x, NULL);
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

			if ( handleSafePacket() )
			{
				continue;
			}
			if (!strncmp((char*)net_packet->data, "BARONY_JOIN_REQUEST", 19))
			{
#ifdef STEAMWORKS
				if ( !directConnect )
				{
					bool skipJoin = false;
					for ( c = 0; c < MAXPLAYERS; c++ )
					{
						if ( client_disconnected[c] || !steamIDRemote[c] )
						{
							continue;
						}
						if ( newSteamID.ConvertToUint64() == (static_cast<CSteamID* >(steamIDRemote[c]))->ConvertToUint64() )
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
				}
#endif
				if ( strcmp( VERSION, (char*)net_packet->data + 54 ) )
				{
					c = MAXPLAYERS + 1; // wrong version number
				}
				else
				{
					Uint32 clientlsg = SDLNet_Read32(&net_packet->data[68]);
					Uint32 clientms = SDLNet_Read32(&net_packet->data[64]);
					if ( net_packet->data[63] == 0 )
					{
						// client will enter any player spot
						for ( c = 0; c < MAXPLAYERS; c++ )
						{
							if ( client_disconnected[c] == true )
							{
								break;    // no more player slots
							}
						}
					}
					else
					{
						// client is joining a particular player spot
						c = net_packet->data[63];
						if ( !client_disconnected[c] )
						{
							c = MAXPLAYERS;  // client wants to fill a space that is already filled
						}
					}
					if ( clientlsg != loadingsavegame && loadingsavegame == 0 )
					{
						c = MAXPLAYERS + 2;  // client shouldn't load save game
					}
					else if ( clientlsg == 0 && loadingsavegame != 0 )
					{
						c = MAXPLAYERS + 3;  // client is trying to join a save game without a save of their own
					}
					else if ( clientlsg != loadingsavegame )
					{
						c = MAXPLAYERS + 4;  // client is trying to join the game with an incompatible save
					}
					else if ( loadingsavegame && getSaveGameMapSeed() != clientms )
					{
						c = MAXPLAYERS + 5;  // client is trying to join the game with a slightly incompatible save (wrong level)
					}
				}
				if ( c >= MAXPLAYERS )
				{
					// on error, client gets a player number that is invalid (to be interpreted as an error code)
					net_clients[MAXPLAYERS - 1].host = net_packet->address.host;
					net_clients[MAXPLAYERS - 1].port = net_packet->address.port;
					if ( directConnect )
						while ((net_tcpclients[MAXPLAYERS - 1] = SDLNet_TCP_Accept(net_tcpsock)) == NULL);
					net_packet->address.host = net_clients[MAXPLAYERS - 1].host;
					net_packet->address.port = net_clients[MAXPLAYERS - 1].port;
					net_packet->len = 4;
					SDLNet_Write32(c, &net_packet->data[0]); // error code for client to interpret
					if ( directConnect )
					{
						SDLNet_TCP_Send(net_tcpclients[MAXPLAYERS - 1], net_packet->data, net_packet->len);
						SDLNet_TCP_Close(net_tcpclients[MAXPLAYERS - 1]);
					}
					else
					{
#ifdef STEAMWORKS
						SteamNetworking()->SendP2PPacket(newSteamID, net_packet->data, net_packet->len, k_EP2PSendReliable, 0);
						SDL_Delay(5);
						SteamNetworking()->SendP2PPacket(newSteamID, net_packet->data, net_packet->len, k_EP2PSendReliable, 0);
						SDL_Delay(5);
						SteamNetworking()->SendP2PPacket(newSteamID, net_packet->data, net_packet->len, k_EP2PSendReliable, 0);
						SDL_Delay(5);
						SteamNetworking()->SendP2PPacket(newSteamID, net_packet->data, net_packet->len, k_EP2PSendReliable, 0);
						SDL_Delay(5);
						SteamNetworking()->SendP2PPacket(newSteamID, net_packet->data, net_packet->len, k_EP2PSendReliable, 0);
						SDL_Delay(5);
#endif
					}
				}
				else
				{
					// on success, client gets legit player number
					strcpy(stats[c]->name, (char*)(&net_packet->data[19]));
					client_disconnected[c] = false;
					client_classes[c] = (int)SDLNet_Read32(&net_packet->data[42]);
					stats[c]->sex = static_cast<sex_t>((int)SDLNet_Read32(&net_packet->data[46]));
					stats[c]->appearance = (int)SDLNet_Read32(&net_packet->data[50]);
					net_clients[c - 1].host = net_packet->address.host;
					net_clients[c - 1].port = net_packet->address.port;
					if ( directConnect )
					{
						while ((net_tcpclients[c - 1] = SDLNet_TCP_Accept(net_tcpsock)) == NULL);
						const char* clientaddr = SDLNet_ResolveIP(&net_packet->address);
						printlog("client %d connected from %s:%d\n", c, clientaddr, net_packet->address.port);
					}
					else
					{
						printlog("client %d connected.\n", c);
					}
					client_keepalive[c] = ticks;

					// send existing clients info on new client
					for ( x = 1; x < MAXPLAYERS; x++ )
					{
						if ( client_disconnected[x] || c == x )
						{
							continue;
						}
						strcpy((char*)(&net_packet->data[0]), "NEWPLAYER");
						net_packet->data[9] = c; // clientnum
						net_packet->data[10] = client_classes[c]; // class
						net_packet->data[11] = stats[c]->sex; // sex
						strcpy((char*)(&net_packet->data[12]), stats[c]->name);  // name
						net_packet->address.host = net_clients[x - 1].host;
						net_packet->address.port = net_clients[x - 1].port;
						net_packet->len = 12 + strlen(stats[c]->name) + 1;
						sendPacketSafe(net_sock, -1, net_packet, x - 1);
					}
					char shortname[11] = { 0 };
					strncpy(shortname, stats[c]->name, 10);

					newString(&lobbyChatboxMessages, 0xFFFFFFFF, "\n***   %s has joined the game   ***\n", shortname);

					// send new client their id number + info on other clients
					SDLNet_Write32(c, &net_packet->data[0]);
					for ( x = 0; x < MAXPLAYERS; x++ )
					{
						net_packet->data[4 + x * (3 + 16)] = client_classes[x]; // class
						net_packet->data[5 + x * (3 + 16)] = stats[x]->sex; // sex
						net_packet->data[6 + x * (3 + 16)] = client_disconnected[x]; // connectedness :p
						strcpy((char*)(&net_packet->data[7 + x * (3 + 16)]), stats[x]->name);  // name
					}
					net_packet->address.host = net_clients[c - 1].host;
					net_packet->address.port = net_clients[c - 1].port;
					net_packet->len = 4 + MAXPLAYERS * (3 + 16);
					if ( directConnect )
					{
						SDLNet_TCP_Send(net_tcpclients[c - 1], net_packet->data, net_packet->len);
					}
					else
					{
#ifdef STEAMWORKS
						if ( steamIDRemote[c - 1] )
						{
							cpp_Free_CSteamID( steamIDRemote[c - 1] );
						}
						steamIDRemote[c - 1] = new CSteamID();
						*static_cast<CSteamID*>(steamIDRemote[c - 1]) = newSteamID;
						SteamNetworking()->SendP2PPacket(*static_cast<CSteamID* >(steamIDRemote[c - 1]), net_packet->data, net_packet->len, k_EP2PSendReliable, 0);
						SDL_Delay(5);
						SteamNetworking()->SendP2PPacket(*static_cast<CSteamID* >(steamIDRemote[c - 1]), net_packet->data, net_packet->len, k_EP2PSendReliable, 0);
						SDL_Delay(5);
						SteamNetworking()->SendP2PPacket(*static_cast<CSteamID* >(steamIDRemote[c - 1]), net_packet->data, net_packet->len, k_EP2PSendReliable, 0);
						SDL_Delay(5);
						SteamNetworking()->SendP2PPacket(*static_cast<CSteamID* >(steamIDRemote[c - 1]), net_packet->data, net_packet->len, k_EP2PSendReliable, 0);
						SDL_Delay(5);
						SteamNetworking()->SendP2PPacket(*static_cast<CSteamID* >(steamIDRemote[c - 1]), net_packet->data, net_packet->len, k_EP2PSendReliable, 0);
						SDL_Delay(5);
#endif
					}
				}
				continue;
			}

			// got a chat message
			else if (!strncmp((char*)net_packet->data, "CMSG", 4))
			{
				int i;
				for ( i = 0; i < MAXPLAYERS; i++ )
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
				for ( c = 1; c < MAXPLAYERS; c++ )
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
				char shortname[11] = { 0 };
				strncpy(shortname, stats[net_packet->data[16]]->name, 10);
				newString(&lobbyChatboxMessages, 0xFFFFFFFF, language[1376], shortname);
				continue;
			}

			// client requesting new svFlags
			else if (!strncmp((char*)net_packet->data, "SVFL", 4))
			{
				// update svFlags for everyone
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
#endif

			// trying to connect to the server and get a player number
			// receive the packet:
			bool gotPacket = false;
			if ( directConnect )
			{
				if ( SDLNet_TCP_Recv(net_tcpsock, net_packet->data, 4 + MAXPLAYERS * (3 + 16)) )
				{
					gotPacket = true;
				}
			}
			else
			{
#ifdef STEAMWORKS
				int numpacket;
				for ( numpacket = 0; numpacket < PACKET_LIMIT; numpacket++ )
				{
					uint32_t packetlen = 0;
					if ( !SteamNetworking()->IsP2PPacketAvailable(&packetlen, 0) )
					{
						break;
					}
					packetlen = std::min<int>(packetlen, NET_PACKET_SIZE - 1);
					Uint32 bytesRead = 0;
					if ( !SteamNetworking()->ReadP2PPacket(net_packet->data, packetlen, &bytesRead, &newSteamID, 0) || bytesRead != 4 + MAXPLAYERS * (3 + 16) )
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
					gotPacket = true;
					break;
				}
#endif
			}

			// parse the packet:
			if ( gotPacket )
			{
				list_FreeAll(&button_l);
				deleteallbuttons = true;
				clientnum = (int)SDLNet_Read32(&net_packet->data[0]);
				if ( clientnum >= MAXPLAYERS || clientnum <= 0 )
				{
					printlog("connection attempt denied by server.\n");
					multiplayer = SINGLE;

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
					for ( c = 0; c < MAXPLAYERS; c++ )
					{
						client_disconnected[c] = false;
						client_classes[c] = net_packet->data[4 + c * (3 + 16)]; // class
						stats[c]->sex = static_cast<sex_t>(net_packet->data[5 + c * (3 + 16)]); // sex
						client_disconnected[c] = net_packet->data[6 + c * (3 + 16)]; // connectedness :p
						strcpy(stats[c]->name, (char*)(&net_packet->data[7 + c * (3 + 16)]));  // name
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
				}
			}
		}
		else if ( multiplayer == CLIENT )
		{
#ifdef STEAMWORKS
			CSteamID newSteamID;
#endif
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
#ifdef STEAMWORKS
					uint32_t packetlen = 0;
					if ( !SteamNetworking()->IsP2PPacketAvailable(&packetlen, 0) )
					{
						break;
					}
					packetlen = std::min<int>(packetlen, NET_PACKET_SIZE - 1);
					Uint32 bytesRead = 0;
					if ( !SteamNetworking()->ReadP2PPacket(net_packet->data, packetlen, &bytesRead, &newSteamID, 0) )
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

				if ( handleSafePacket() )
				{
					continue;
				}

				// game start
				if (!strncmp((char*)net_packet->data, "BARONY_GAME_START", 17))
				{
					svFlags = SDLNet_Read32(&net_packet->data[17]);
					uniqueGameKey = SDLNet_Read32(&net_packet->data[21]);
					buttonCloseSubwindow(NULL);
					numplayers = MAXPLAYERS;
					introstage = 3;
					fadeout = true;
					continue;
				}

				// new player
				else if (!strncmp((char*)net_packet->data, "NEWPLAYER", 9))
				{
					client_disconnected[net_packet->data[9]] = false;
					client_classes[net_packet->data[9]] = net_packet->data[10];
					stats[net_packet->data[9]]->sex = static_cast<sex_t>(net_packet->data[11]);
					strcpy(stats[net_packet->data[9]]->name, (char*)(&net_packet->data[12]));

					char shortname[11] = { 0 };
					strncpy(shortname, stats[net_packet->data[9]]->name, 10);
					newString(&lobbyChatboxMessages, 0xFFFFFFFF, language[1388], shortname);
					continue;
				}

				// player disconnect
				else if (!strncmp((char*)net_packet->data, "PLAYERDISCONNECT", 16))
				{
					client_disconnected[net_packet->data[16]] = true;
					if ( net_packet->data[16] == 0 )
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
						strcpy(subtext, language[1126]);

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
						for ( c = 1; c < MAXPLAYERS; c++ )
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
					}
					else
					{
						char shortname[11] = { 0 };
						strncpy(shortname, stats[net_packet->data[16]]->name, 10);
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
					svFlags = SDLNet_Read32(&net_packet->data[4]);
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
		for ( c = 0; c < MAXPLAYERS; ++c )
		{
			if ( client_disconnected[c] )
			{
				continue;
			}
			string charDisplayName = "";
			charDisplayName = stats[c]->name;

#ifdef STEAMWORKS
			int remoteIDIndex = c;
			if ( multiplayer == SERVER )
			{
				remoteIDIndex--;
			}

			if ( !directConnect && steamIDRemote[remoteIDIndex] )
			{
				charDisplayName += " (";
				charDisplayName += SteamFriends()->GetFriendPersonaName(*static_cast<CSteamID* >(steamIDRemote[remoteIDIndex]));
				charDisplayName += ")";
			}
#endif

			if ( stats[c]->sex )
			{
				ttfPrintTextFormatted(ttf12, subx1 + 8, suby1 + 80 + 60 * c, "%d:  %s\n    %s\n    %s", c + 1, charDisplayName.c_str(), language[1322], playerClassLangEntry(client_classes[c]));
			}
			else
			{
				ttfPrintTextFormatted(ttf12, subx1 + 8, suby1 + 80 + 60 * c, "%d:  %s\n    %s\n    %s", c + 1, charDisplayName.c_str(), language[1321], playerClassLangEntry(client_classes[c]));
			}
		}

		// select gui element w/ mouse
		if ( mousestatus[SDL_BUTTON_LEFT] )
		{
			if ( mouseInBounds(subx1 + 16, subx2 - 16, suby2 - 48, suby2 - 32) )
			{
				mousestatus[SDL_BUTTON_LEFT] = 0;

				// chatbox
				inputstr = lobbyChatbox;
				inputlen = LOBBY_CHATBOX_LENGTH - 1;
				cursorflash = ticks;
			}
			else if ( mouseInBounds(xres / 2, subx2 - 32, suby1 + 56, suby1 + 68) && multiplayer == SERVER )
			{
				mousestatus[SDL_BUTTON_LEFT] = 0;

				// lobby name
#ifdef STEAMWORKS
				inputstr = currentLobbyName;
				inputlen = 31;
#endif
				cursorflash = ticks;
			}

			// server flags
			int i;
			if ( multiplayer == SERVER )
			{
				for ( i = 0; i < NUM_SERVER_FLAGS; i++ )
				{
					if ( mouseInBounds(xres / 2 + 8 + 6, xres / 2 + 8 + 30, suby1 + 80 + i * 16, suby1 + 92 + i * 16) )
					{
						mousestatus[SDL_BUTTON_LEFT] = 0;

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
#ifdef STEAMWORKS
						if ( !directConnect )
						{
							char svFlagsChar[16];
							snprintf(svFlagsChar, 15, "%d", svFlags);
							SteamMatchmaking()->SetLobbyData(*static_cast<CSteamID*>(currentLobby), "svFlags", svFlagsChar);
						}
#endif
					}
				}
			}

			// switch lobby type
#ifdef STEAMWORKS
			if ( !directConnect )
			{
				if ( multiplayer == SERVER )
				{
					for ( i = 0; i < 3; i++ )
					{
						if ( mouseInBounds(xres / 2 + 8 + 6, xres / 2 + 8 + 30, suby1 + 256 + i * 16, suby1 + 268 + i * 16) )
						{
							mousestatus[SDL_BUTTON_LEFT] = 0;
							switch ( i )
							{
								default:
									currentLobbyType = k_ELobbyTypePrivate;
									break;
								case 1:
									currentLobbyType = k_ELobbyTypeFriendsOnly;
									break;
								case 2:
									currentLobbyType = k_ELobbyTypePublic;
									break;
							}
							SteamMatchmaking()->SetLobbyType(*static_cast<CSteamID*>(currentLobby), currentLobbyType);
						}
					}
				}
			}
#endif
		}

		// switch textboxes with TAB
		if ( keystatus[SDL_SCANCODE_TAB] )
		{
			keystatus[SDL_SCANCODE_TAB] = 0;
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

		// server flag elements
		int i;
		for ( i = 0; i < NUM_SERVER_FLAGS; i++ )
		{
			if ( svFlags & power(2, i) )
			{
				ttfPrintTextFormatted(ttf12, xres / 2 + 8, suby1 + 80 + 16 * i, "[x] %s", language[153 + i]);
			}
			else
			{
				ttfPrintTextFormatted(ttf12, xres / 2 + 8, suby1 + 80 + 16 * i, "[ ] %s", language[153 + i]);
			}
			if (mouseInBounds((xres / 2) + 8 + 6, (xres / 2) + 8 + 30, suby1 + 80 + (i * 16), suby1 + 92 + (i * 16)))   //So many gosh dang magic numbers ._.
			{
				if (strlen(language[1942 + i]) > 0)   //Don't bother drawing a tooltip if the file doesn't say anything.
				{
					hovering_selection = i;
					tooltip_box.x = mousex + 16;
					tooltip_box.y = mousey + 8;
					tooltip_box.w = strlen(language[1942 + i]) * TTF12_WIDTH + 8; //MORE MAGIC NUMBERS. HNNGH. I can guess what they all do, but dang.
					tooltip_box.h = TTF12_HEIGHT + 8;
				}
			}
		}

		// lobby type elements
#ifdef STEAMWORKS
		if ( !directConnect )
		{
			if ( multiplayer == SERVER )
			{
				for ( i = 0; i < 3; i++ )
				{
					if ( currentLobbyType == static_cast<ELobbyType>(i) )
					{
						ttfPrintTextFormatted(ttf12, xres / 2 + 8, suby1 + 256 + 16 * i, "[o] %s", language[250 + i]);
					}
					else
					{
						ttfPrintTextFormatted(ttf12, xres / 2 + 8, suby1 + 256 + 16 * i, "[ ] %s", language[250 + i]);
					}
				}
			}
		}
#endif

#ifdef STEAMWORKS
		if ( !directConnect )
		{
			// server name
			drawDepressed(xres / 2, suby1 + 56, xres / 2 + 388, suby1 + 72);
			ttfPrintTextFormatted(ttf12, xres / 2 + 2, suby1 + 58, "%s", currentLobbyName);
			if ( inputstr == currentLobbyName )
			{
				if ( (ticks - cursorflash) % TICKS_PER_SECOND < TICKS_PER_SECOND / 2 )
				{
					int x;
					TTF_SizeUTF8(ttf12, currentLobbyName, &x, NULL);
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
			}
		}
#endif

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
				y -= str->lines * 12;
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

			char shortname[11] = {0};
			strncpy(shortname, stats[clientnum]->name, 10);

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

		// draw chatbox entry text and cursor
		ttfPrintTextFormatted(ttf12, subx1 + 18, suby2 - 46, ">%s", lobbyChatbox);
		if ( inputstr == lobbyChatbox )
		{
			if ( (ticks - cursorflash) % TICKS_PER_SECOND < TICKS_PER_SECOND / 2 )
			{
				int x;
				TTF_SizeUTF8(ttf12, lobbyChatbox, &x, NULL);
				ttfPrintTextFormatted(ttf12, subx1 + 18 + x + TTF12_WIDTH, suby2 - 46, "_");
			}
		}

		if (hovering_selection > -1)
		{
			drawTooltip(&tooltip_box);
			if (hovering_selection < NUM_SERVER_FLAGS)
			{
				ttfPrintTextFormatted(ttf12, tooltip_box.x + 4, tooltip_box.y + 4, language[1942 + hovering_selection]);
			}
		}

		// handle keepalive timeouts (lobby)
		if ( multiplayer == SERVER )
		{
			int i;
			for ( i = 1; i < MAXPLAYERS; i++ )
			{
				if ( client_disconnected[i] )
				{
					continue;
				}
				if ( ticks - client_keepalive[i] > TICKS_PER_SECOND * 30 )
				{
					client_disconnected[i] = true;
					strncpy((char*)(net_packet->data), "PLAYERDISCONNECT", 16);
					net_packet->data[16] = i;
					net_packet->len = 17;
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
					char shortname[11] = { 0 };
					strncpy(shortname, stats[i]->name, 10);
					newString(&lobbyChatboxMessages, 0xFFFFFFFF, language[1376], shortname);
					continue;
				}
			}
		}
		else if ( multiplayer == CLIENT )
		{
			if ( ticks - client_keepalive[0] > TICKS_PER_SECOND * 30 )
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

	// statistics window
	if ( score_window )
	{
		if ( !list_Size(&topscores) )
		{
#define NOSCORESSTR language[1389]
			ttfPrintTextFormatted(ttf16, xres / 2 - strlen(NOSCORESSTR) * 9, yres / 2 - 9, NOSCORESSTR);
		}
		else
		{
			ttfPrintTextFormatted(ttf16, subx1 + 8, suby1 + 8, "%s - #%d", language[1390], score_window);

			// draw character window
			if (players[clientnum] != nullptr && players[clientnum]->entity != nullptr)
			{
				camera_charsheet.x = players[clientnum]->entity->x / 16.0 + 1;
				camera_charsheet.y = players[clientnum]->entity->y / 16.0 - .5;
				camera_charsheet.z = players[clientnum]->entity->z * 2;
				camera_charsheet.ang = atan2(players[clientnum]->entity->y / 16.0 - camera_charsheet.y, players[clientnum]->entity->x / 16.0 - camera_charsheet.x);
				camera_charsheet.vang = PI / 24;
				camera_charsheet.winw = 400;
				camera_charsheet.winy = suby1 + 32;
				camera_charsheet.winh = suby2 - 96 - camera_charsheet.winy;
				camera_charsheet.winx = subx1 + 32;
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
			}

			// print name and class
			if ( victory )
			{
				ttfPrintTextFormatted(ttf16, subx1 + 448, suby1 + 40, language[1391]);
				ttfPrintTextFormatted(ttf16, subx1 + 448, suby1 + 56, "%s", stats[clientnum]->name);
				if ( victory == 1 )
				{
					ttfPrintTextFormatted(ttf16, subx1 + 448, suby1 + 72, language[1392]);
				}
				else if ( victory == 2 )
				{
					ttfPrintTextFormatted(ttf16, subx1 + 448, suby1 + 72, language[1393]);
				}
			}
			else
			{
				ttfPrintTextFormatted(ttf16, subx1 + 448, suby1 + 40, language[1394]);
				ttfPrintTextFormatted(ttf16, subx1 + 448, suby1 + 56, "%s", stats[clientnum]->name);

				char classname[32];
				strcpy(classname, playerClassLangEntry(client_classes[0]));
				classname[0] -= 32;
				ttfPrintTextFormatted(ttf16, subx1 + 448, suby1 + 72, language[1395], classname);
			}

			// print total score
			node = list_Node(&topscores, score_window - 1);
			if ( node )
			{
				score_t* score = (score_t*)node->element;
				ttfPrintTextFormatted(ttf16, subx1 + 448, suby1 + 104, language[1404], totalScore(score));
			}

			// print character stats
			ttfPrintTextFormatted(ttf12, subx1 + 456, suby1 + 128, language[359], stats[clientnum]->LVL, playerClassLangEntry(client_classes[clientnum]));
			ttfPrintTextFormatted(ttf12, subx1 + 456, suby1 + 140, language[1396], stats[clientnum]->EXP);
			ttfPrintTextFormatted(ttf12, subx1 + 456, suby1 + 152, language[1397], stats[clientnum]->GOLD);
			ttfPrintTextFormatted(ttf12, subx1 + 456, suby1 + 164, language[361], currentlevel);

			ttfPrintTextFormatted(ttf12, subx1 + 456, suby1 + 188, language[1398], statGetSTR(stats[clientnum]), stats[clientnum]->STR);
			ttfPrintTextFormatted(ttf12, subx1 + 456, suby1 + 200, language[1399], statGetDEX(stats[clientnum]), stats[clientnum]->DEX);
			ttfPrintTextFormatted(ttf12, subx1 + 456, suby1 + 212, language[1400], statGetCON(stats[clientnum]), stats[clientnum]->CON);
			ttfPrintTextFormatted(ttf12, subx1 + 456, suby1 + 224, language[1401], statGetINT(stats[clientnum]), stats[clientnum]->INT);
			ttfPrintTextFormatted(ttf12, subx1 + 456, suby1 + 236, language[1402], statGetPER(stats[clientnum]), stats[clientnum]->PER);
			ttfPrintTextFormatted(ttf12, subx1 + 456, suby1 + 248, language[1403], statGetCHR(stats[clientnum]), stats[clientnum]->CHR);

			// time
			Uint32 sec = (completionTime / TICKS_PER_SECOND) % 60;
			Uint32 min = ((completionTime / TICKS_PER_SECOND) / 60) % 60;
			Uint32 hour = ((completionTime / TICKS_PER_SECOND) / 60) / 60;
			ttfPrintTextFormatted(ttf12, subx1 + 32, suby2 - 80, "%s: %02d:%02d:%02d. %s:", language[1405], hour, min, sec, language[1406]);
			if ( !conductPenniless && !conductFoodless && !conductVegetarian && !conductIlliterate )
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
					b++;
				}
				if ( conductFoodless )
				{
					strcat(tempstr, language[1409]);
					b++;
				}
				if ( b == 2 )
				{
					strcat(tempstr, "\n ");
				}
				if ( conductVegetarian )
				{
					strcat(tempstr, language[1410]);
					b++;
				}
				if ( b == 2 )
				{
					strcat(tempstr, "\n ");
				}
				if ( conductIlliterate )
				{
					strcat(tempstr, language[1411]);
					b++;
				}
				if ( b == 2 )
				{
					strcat(tempstr, "\n ");
				}
				if ( b != 2 )
				{
					tempstr[strlen(tempstr) - 2] = 0;
				}
				else
				{
					tempstr[strlen(tempstr) - 4] = 0;
				}
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
							ttfPrintTextFormatted(ttf12, subx1 + 456 + (y / 14) * 180, suby1 + 296 + (y % 14) * 12, "%d %s", kills[x], language[111 + x]);
						}
						else
						{
							ttfPrintTextFormatted(ttf12, subx1 + 456 + (y / 14) * 180, suby1 + 296 + (y % 14) * 12, "%d %s", kills[x], language[2050 + (x - KOBOLD)]);
						}
					}
					else
					{
						if ( x < KOBOLD )
						{
							ttfPrintTextFormatted(ttf12, subx1 + 456 + (y / 14) * 180, suby1 + 296 + (y % 14) * 12, "%d %s", kills[x], language[90 + x]);
						}
						else
						{
							ttfPrintTextFormatted(ttf12, subx1 + 456 + (y / 14) * 180, suby1 + 296 + (y % 14) * 12, "%d %s", kills[x], language[2000 + (x - KOBOLD)]);
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
			introstage = 1;
			fadefinished = false;
			fadeout = false;
			gamePaused = false;
			multiplayerselect = 0;
			intro = true; //Fix items auto-adding to the hotbar on game restart.

			if ( !mode )
			{
				// restarting game, make a highscore
				saveScore();
				deleteSaveGame();
				loadingsavegame = 0;
			}

			// undo shopkeeper grudge
			swornenemies[SHOPKEEPER][HUMAN] = false;
			monsterally[SHOPKEEPER][HUMAN] = true;

			// setup game //TODO: Move into a function startGameStuff() or something.
			entity_uids = 1;
			loading = true;
			darkmap = false;
			selected_spell = NULL;
			shootmode = true;
			currentlevel = startfloor;
			secretlevel = false;
			victory = 0;
			completionTime = 0;
			conductPenniless = true;
			conductFoodless = true;
			conductVegetarian = true;
			conductIlliterate = true;
			list_FreeAll(&damageIndicators);
			for ( c = 0; c < NUMMONSTERS; c++ )
			{
				kills[c] = 0;
			}

			// disable cheats
			noclip = false;
			godmode = false;
			buddhamode = false;
			everybodyfriendly = false;

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
#endif

			// load dungeon
			if ( multiplayer != CLIENT )
			{
				// stop all sounds
#ifdef HAVE_FMOD
				if ( sound_group )
				{
					FMOD_ChannelGroup_Stop(sound_group);
				}
#elif defined HAVE_OPENAL
				if ( sound_group )
				{
					OPENAL_ChannelGroup_Stop(sound_group);
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
				hudarm = NULL;
				hudweapon = NULL;
				magicLeftHand = NULL;
				magicRightHand = NULL;

				for ( node = map.entities->first; node != NULL; node = node->next )
				{
					entity = (Entity*)node->element;
					entity->flags[NOUPDATE] = true;
				}
				lastEntityUIDs = entity_uids;
				numplayers = 0;
				if ( loadingmap == false )
				{
					if ( !secretlevel )
					{
						fp = openDataFile(LEVELSFILE, "r");
					}
					else
					{
						fp = openDataFile(SECRETLEVELSFILE, "r");
					}
					int i;
					for ( i = 0; i < currentlevel; i++ )
						while ( fgetc(fp) != '\n' ) if ( feof(fp) )
							{
								break;
							}
					fscanf(fp, "%s", tempstr);
					while ( fgetc(fp) != ' ' ) if ( feof(fp) )
						{
							break;
						}
					if ( !strcmp(tempstr, "gen:") )
					{
						fscanf(fp, "%s", tempstr);
						while ( fgetc(fp) != '\n' ) if ( feof(fp) )
							{
								break;
							}
						generateDungeon(tempstr, mapseed);
					}
					else if ( !strcmp(tempstr, "map:") )
					{
						fscanf(fp, "%s", tempstr);
						while ( fgetc(fp) != '\n' ) if ( feof(fp) )
							{
								break;
							}
						loadMap(tempstr, &map, map.entities);
					}
					fclose(fp);
				}
				else
				{
					if ( genmap == false )
					{
						loadMap(maptoload, &map, map.entities);
					}
					else
					{
						generateDungeon(maptoload, mapseed);
					}
				}
				assignActions(&map);
				generatePathMaps();

				if ( loadingsavegame )
				{
					loadingsavegame = 0;

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
									for ( node = tempFollowers->first; node != NULL; node = node->next )
									{
										Stat* tempStats = (Stat*)node->element;
										Entity* monster = summonMonster(tempStats->type, players[c]->entity->x, players[c]->entity->y);
										if ( monster )
										{
											monster->skill[3] = 1; // to mark this monster partially initialized
											list_RemoveNode(monster->children.last);

											node_t* newNode = list_AddNodeLast(&monster->children);
											newNode->element = tempStats->copyStats();
											//newNode->deconstructor = &tempStats->~Stat;
											newNode->size = sizeof(tempStats);

											Stat* monsterStats = (Stat*)newNode->element;
											monsterStats->leader_uid = players[c]->entity->getUID();
											if ( !monsterally[HUMAN][monsterStats->type] )
											{
												monster->flags[USERFLAG2] = true;
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
												net_packet->address.host = net_clients[c - 1].host;
												net_packet->address.port = net_clients[c - 1].port;
												net_packet->len = 8;
												sendPacketSafe(net_sock, -1, net_packet, c - 1);
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
				hudarm = NULL;
				hudweapon = NULL;
				magicLeftHand = NULL;
				magicRightHand = NULL;

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
#ifdef HAVE_FMOD
				if ( sound_group )
				{
					FMOD_ChannelGroup_Stop(sound_group);
				}
#elif defined HAVE_OPENAL
				if ( sound_group )
				{
					OPENAL_ChannelGroup_Stop(sound_group);
				}
#endif
				// load next level
				entity_uids = 1;
				lastEntityUIDs = entity_uids;
				numplayers = 0;
				if ( loadingmap == false )
				{
					if ( !secretlevel )
					{
						fp = openDataFile(LEVELSFILE, "r");
					}
					else
					{
						fp = openDataFile(SECRETLEVELSFILE, "r");
					}
					int i;
					for ( i = 0; i < currentlevel; i++ )
						while ( fgetc(fp) != '\n' ) if ( feof(fp) )
							{
								break;
							}
					fscanf(fp, "%s", tempstr);
					while ( fgetc(fp) != ' ' ) if ( feof(fp) )
						{
							break;
						}
					if ( !strcmp(tempstr, "gen:") )
					{
						fscanf(fp, "%s", tempstr);
						while ( fgetc(fp) != '\n' ) if ( feof(fp) )
							{
								break;
							}
						generateDungeon(tempstr, mapseed);
					}
					else if ( !strcmp(tempstr, "map:") )
					{
						fscanf(fp, "%s", tempstr);
						while ( fgetc(fp) != '\n' ) if ( feof(fp) )
							{
								break;
							}
						loadMap(tempstr, &map, map.entities);
					}
					fclose(fp);
				}
				else
				{
					if ( genmap == false )
					{
						loadMap(maptoload, &map, map.entities);
					}
					else
					{
						generateDungeon(maptoload, rand());
					}
				}
				assignActions(&map);
				generatePathMaps();
				for ( node = map.entities->first; node != NULL; node = nextnode )
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
			for ( c = 0; c < NUMCLASSES; c++ )
				if ( !usedClass[c] )
				{
					usedAllClasses = false;
				}
			if ( usedAllClasses )
			{
				steamAchievement("BARONY_ACH_SPICE_OF_LIFE");
			}

			// delete game data clutter
			list_FreeAll(&messages);
			list_FreeAll(&command_history);
			list_FreeAll(&safePacketsSent);
			for ( c = 0; c < MAXPLAYERS; c++ )
			{
				list_FreeAll(&safePacketsReceived[c]);
			}
			deleteAllNotificationMessages();
			for (c = 0; c < MAXPLAYERS; c++)
			{
				list_FreeAll(&stats[c]->FOLLOWERS);
			}
			list_FreeAll(&removedEntities);
			list_FreeAll(&chestInv);

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
			creditstage++;
			if ( creditstage >= 14 )
			{
#ifdef MUSIC
				playmusic(intromusic, true, false, false);
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
			}

			// make a highscore!
			saveScore();

			// pick a new subtitle :)
			subtitleCurrent = rand() % NUMSUBTITLES;
			subtitleVisible = true;

			for ( c = 0; c < NUMMONSTERS; c++ )
			{
				kills[c] = 0;
			}

			// stop all sounds
#ifdef HAVE_FMOD
			if ( sound_group )
			{
				FMOD_ChannelGroup_Stop(sound_group);
			}
#elif defined HAVE_OPENAL
			if ( sound_group )
			{
				OPENAL_ChannelGroup_Stop(sound_group);
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
				if ( shopInv )
				{
					list_FreeAll(shopInv);
					free(shopInv);
					shopInv = NULL;
				}
			}

			// delete save game
			if ( !savethisgame )
			{
				deleteSaveGame();
			}
			else
			{
				savethisgame = false;
			}

			// reset game
			darkmap = false;
			appraisal_timer = 0;
			appraisal_item = 0;
			multiplayer = 0;
			shootmode = true;
			currentlevel = 0;
			secretlevel = false;
			clientnum = 0;
			introstage = 1;
			intro = true;
			selected_spell = NULL; //So you don't start off with a spell when the game restarts.
			client_classes[0] = 0;
			spellcastingAnimationManager_deactivate(&cast_animation);
			SDL_StopTextInput();

			// delete game data clutter
			list_FreeAll(&messages);
			list_FreeAll(&command_history);
			list_FreeAll(&safePacketsSent);
			for ( c = 0; c < MAXPLAYERS; c++ )
			{
				list_FreeAll(&safePacketsReceived[c]);
			}
			deleteAllNotificationMessages();
			for (c = 0; c < MAXPLAYERS; c++)
			{
				stats[c]->freePlayerEquipment();
				list_FreeAll(&stats[c]->inventory);
				list_FreeAll(&stats[c]->FOLLOWERS);
			}
			list_FreeAll(&removedEntities);
			list_FreeAll(&chestInv);

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
				stats[c]->sex = static_cast<sex_t>(0);
				stats[c]->appearance = 0;
				strcpy(stats[c]->name, "");
				stats[c]->type = HUMAN;
				stats[c]->clearStats();
				entitiesToDelete[c].first = NULL;
				entitiesToDelete[c].last = NULL;
				if ( c == 0 )
				{
					initClass(c);
				}
			}

			// hack to fix these things from breaking everything...
			hudarm = NULL;
			hudweapon = NULL;
			magicLeftHand = NULL;
			magicRightHand = NULL;

			// load menu level
			switch ( rand() % 4 )
			{
				case 0:
					loadMap("mainmenu1", &map, map.entities);
					camera.x = 8;
					camera.y = 4.5;
					camera.z = 0;
					camera.ang = 0.6;
					break;
				case 1:
					loadMap("mainmenu2", &map, map.entities);
					camera.x = 7;
					camera.y = 4;
					camera.z = -4;
					camera.ang = 1.0;
					break;
				case 2:
					loadMap("mainmenu3", &map, map.entities);
					camera.x = 5;
					camera.y = 3;
					camera.z = 0;
					camera.ang = 1.0;
					break;
				case 3:
					loadMap("mainmenu4", &map, map.entities);
					camera.x = 6;
					camera.y = 14.5;
					camera.z = -24;
					camera.ang = 5.0;
					break;
			}
			camera.vang = 0;
			numplayers = 0;
			assignActions(&map);
			generatePathMaps();
			gamePaused = false;
			if ( !victory )
			{
				fadefinished = false;
				fadeout = false;
#ifdef MUSIC
				playmusic(intromusic, true, false, false);
#endif
			}
			else
			{
				// conduct achievements
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

				if ( victory == 1 )
				{
					introstage = 7;
				}
				else
				{
					introstage = 8;
				}
			}

			// finish handling invite
#ifdef STEAMWORKS
			if ( stillConnectingToLobby )
			{
				processLobbyInvite();
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
				playmusic(intromusic, true, false, false);
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
				thirdendmoviestage++;
			}
#endif
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
	}

	// credits sequence
	if ( creditstage > 0 )
	{
		if ( (credittime >= 300 && (creditstage <= 10 || creditstage > 12)) || (credittime >= 180 && creditstage == 11) ||
		        (credittime >= 480 && creditstage == 12) || mousestatus[SDL_BUTTON_LEFT] || (*inputPressed(joyimpulses[INJOY_MENU_NEXT]) && rebindaction == -1) )
		{
			mousestatus[SDL_BUTTON_LEFT] = 0;
			if ( rebindaction == -1 )
			{
				*inputPressed(joyimpulses[INJOY_MENU_NEXT]) = 0;
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
			ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(CREDITSLINE06), yres / 2 - 9 + 18, CREDITSLINE06);
		}
		else if ( creditstage == 4 )
		{
			ttfPrintTextFormattedColor(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(language[59]), yres / 2 - 9 - 18 * 2, colorBlue, language[59]);
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
			ttfPrintTextFormattedColor(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(language[64]), yres / 2 - 9 - 18, colorBlue, language[64]);
			ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(language[65]), yres / 2 - 9 + 18, language[65]);
		}
		else if ( creditstage == 10 )
		{
			// logo
			src.x = 0;
			src.y = 0;
			src.w = logo_bmp->w;
			src.h = logo_bmp->h;
			dest.x = xres / 2 - (logo_bmp->w + title_bmp->w) / 2 - 16;
			dest.y = yres / 2 - logo_bmp->h / 2;
			dest.w = xres;
			dest.h = yres;
			drawImage(logo_bmp, &src, &dest);
			// title
			src.x = 0;
			src.y = 0;
			src.w = title_bmp->w;
			src.h = title_bmp->h;
			dest.x = xres / 2 - (logo_bmp->w + title_bmp->w) / 2 + logo_bmp->w + 16;
			dest.y = yres / 2 - title_bmp->h / 2;
			dest.w = xres;
			dest.h = yres;
			drawImage(title_bmp, &src, &dest);
			// text
			ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(language[66]), yres / 2 + 96, language[66]);
			ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(language[67]), yres / 2 + 116, language[67]);
			ttfPrintTextFormatted(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(language[68]), yres / 2 + 136, language[68]);
			ttfPrintTextFormattedColor(ttf16, xres / 2 - (TTF16_WIDTH / 2)*strlen(language[69]), yres / 2 + 156, colorBlue, language[69]);
		}
		else if ( creditstage == 12 )
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
		pos.h = (((real_t)xres) / backdrop_bmp->w) * backdrop_bmp->h;
		drawImageScaled(backdrop_bmp, NULL, &pos);

		if ( intromovietime >= 600 || mousestatus[SDL_BUTTON_LEFT] || keystatus[SDL_SCANCODE_ESCAPE] ||
		        keystatus[SDL_SCANCODE_SPACE] || keystatus[SDL_SCANCODE_RETURN] || (intromovietime >= 120 && intromoviestage == 1) || (*inputPressed(joyimpulses[INJOY_MENU_NEXT]) && rebindaction == -1) )
		{
			intromovietime = 0;
			mousestatus[SDL_BUTTON_LEFT] = 0;
			if ( rebindaction == -1 )
			{
				*inputPressed(joyimpulses[INJOY_MENU_NEXT]) = 0;
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
		pos.h = (((real_t)xres) / backdrop_bmp->w) * backdrop_bmp->h;
		drawImageScaled(backdrop_bmp, NULL, &pos);

		if ( firstendmovietime >= 600 || mousestatus[SDL_BUTTON_LEFT] || keystatus[SDL_SCANCODE_ESCAPE] ||
		        keystatus[SDL_SCANCODE_SPACE] || keystatus[SDL_SCANCODE_RETURN] || (firstendmovietime >= 120 && firstendmoviestage == 1) )
		{
			firstendmovietime = 0;
			mousestatus[SDL_BUTTON_LEFT] = 0;
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
		pos.h = (((real_t)xres) / backdrop_bmp->w) * backdrop_bmp->h;
		drawImageScaled(backdrop_bmp, NULL, &pos);

		if ( secondendmovietime >= 600 || mousestatus[SDL_BUTTON_LEFT] || keystatus[SDL_SCANCODE_ESCAPE] ||
		        keystatus[SDL_SCANCODE_SPACE] || keystatus[SDL_SCANCODE_RETURN] || (secondendmovietime >= 120 && secondendmoviestage == 1) )
		{
			secondendmovietime = 0;
			mousestatus[SDL_BUTTON_LEFT] = 0;
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
		pos.h = (((real_t)xres) / backdrop_bmp->w) * backdrop_bmp->h;
		drawRect(&pos, 0, 255);
		drawImageScaled(backdrop_bmp, NULL, &pos);

		if ( thirdendmovietime >= 600 || mousestatus[SDL_BUTTON_LEFT] || keystatus[SDL_SCANCODE_ESCAPE] ||
			keystatus[SDL_SCANCODE_SPACE] || keystatus[SDL_SCANCODE_RETURN] || (thirdendmovietime >= 120 && thirdendmoviestage == 1) )
		{
			thirdendmovietime = 0;
			mousestatus[SDL_BUTTON_LEFT] = 0;
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
			thirdendmoviealpha[8] = std::min(thirdendmoviealpha[8] + 2, 255);
			color = 0x00FFFFFF;
			color += std::min(std::max(0, thirdendmoviealpha[8]), 255) << 24;
			ttfPrintTextColor(ttf16, 16, yres - 32, color, true, language[2606]);
		}
		if ( thirdendmoviestage >= 2 )
		{
			thirdendmoviealpha[0] = std::min(thirdendmoviealpha[0] + 2, 255);
			color = 0x00FFFFFF;
			color += std::min(std::max(0, thirdendmoviealpha[0]), 255) << 24;
			ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 22, color, true, language[2600]);
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
		if ( thirdendmoviestage >= 6 )
		{
			thirdendmoviealpha[4] = std::min(thirdendmoviealpha[4] + 2, 255);
			color = 0x00FFFFFF;
			color += std::min(std::max(0, thirdendmoviealpha[4]), 255) << 24;
			if ( multiplayer == CLIENT )
			{
				ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, language[2605]);
			}
			else
			{
				ttfPrintTextColor(ttf16, 16 + (xres - 960) / 2, 16 + (yres - 600) / 2, color, true, language[2604]);
			}
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
	scoreDeconstructor((void*)score);

	bool madetop = false;
	if ( !list_Size(&topscores) )
	{
		madetop = true;
	}
	else if ( list_Size(&topscores) < MAXTOPSCORES )
	{
		madetop = true;
	}
	else if ( totalScore((score_t*)topscores.last->element) < total )
	{
		madetop = true;
	}

	shootmode = false;
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
		for ( node = stats[clientnum]->inventory.first; node != NULL; node = node->next )
		{
			Item* item = (Item*)node->element;
			item->identified = true;
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
	int c;

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
	settings_smoothlighting = smoothlighting;
	settings_fullscreen = fullscreen;
	settings_shaking = shaking;
	settings_bobbing = bobbing;
	settings_spawn_blood = spawn_blood;
	settings_colorblind = colorblind;
	settings_gamma = vidgamma;
	settings_fps = fpsLimit;
	settings_sfxvolume = sfxvolume;
	settings_musvolume = musvolume;
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
	settings_mousespeed = mousespeed;
	settings_broadcast = broadcast;
	settings_nohud = nohud;
	settings_auto_hotbar_new_items = auto_hotbar_new_items;
	for ( c = 0; c < NUM_HOTBAR_CATEGORIES; ++c )
	{
		settings_auto_hotbar_categories[c] = auto_hotbar_categories[c];
	}
	settings_disable_messages = disable_messages;
	settings_right_click_protect = right_click_protect;
	settings_auto_appraise_new_items = auto_appraise_new_items;

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
	suby1 = yres / 2 - 288;
	suby2 = yres / 2 + 288;
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

void openSteamLobbyWaitWindow(button_t* my);

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

// opens the wait window for steam lobby (getting lobby list, etc.)
void openSteamLobbyWaitWindow(button_t* my)
{
	button_t* button;

	// close current window
	buttonCloseSubwindow(NULL);
	list_FreeAll(&button_l);
	deleteallbuttons = true;

	// create new window
	subwindow = 1;
#ifdef STEAMWORKS
	requestingLobbies = true;
#endif
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
	suby1 = yres / 2 - 192;
	suby2 = yres / 2 + 192;
	strcpy(subtext, language[1334]);

	// setup lobby browser
#ifdef STEAMWORKS //TODO: Should this whole function be ifdeffed?
	selectedSteamLobby = 0;
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
#ifdef STEAMWORKS
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
#ifdef STEAMWORKS
	button->action = &buttonSteamLobbyBrowserRefresh;
#endif
	button->visible = 1;
	button->focused = 1;
	button->joykey = joyimpulses[INJOY_MENU_REFRESH_LOBBY]; //"y" refreshes
}

// steam lobby browser join game
void buttonSteamLobbyBrowserJoinGame(button_t* my)
{
#ifndef STEAMWORKS
	return;
#else

	button_t* button;
	int lobbyIndex = std::min(std::max(0, selectedSteamLobby), MAX_STEAM_LOBBIES - 1);
	if ( lobbyIDs[lobbyIndex] )
	{
		// close current window
		int temp1 = connectingToLobby;
		int temp2 = connectingToLobbyWindow;
		//buttonCloseSubwindow(my);
		list_FreeAll(&button_l);
		deleteallbuttons = true;
		connectingToLobby = temp1;
		connectingToLobbyWindow = temp2;

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
		strncpy( currentLobbyName, lobbyText[lobbyIndex], 31 );
		cpp_SteamMatchmaking_JoinLobby(*static_cast<CSteamID* >(lobbyIDs[lobbyIndex]));
	}
#endif
}

// steam lobby browser refresh
void buttonSteamLobbyBrowserRefresh(button_t* my)
{
#ifndef STEAMWORKS
	return;
#else
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
	deleteSaveGame();

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
	if ( score_window )
	{
		// reset class loadout
		stats[0]->sex = static_cast<sex_t>(0);
		stats[0]->appearance = 0;
		strcpy(stats[0]->name, "");
		stats[0]->type = HUMAN;
		client_classes[0] = 0;
		stats[0]->clearStats();
		initClass(0);
	}
	rebindkey = -1;
#ifdef STEAMWORKS
	requestingLobbies = false;
#endif
	score_window = 0;
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
	connectingToLobbyWindow = false;
	connectingToLobby = false;
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
	button_t* button;

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
	if ( charcreation_step == 4 )
	{
		inputstr = stats[0]->name;
		SDL_StartTextInput();
	}
	else if ( charcreation_step == 5 )
	{
		if ( SDL_IsTextInputActive() )
		{
			lastname = (string)stats[0]->name;
			SDL_StopTextInput();
		}
#ifdef STEAMWORKS
		if ( lobbyToConnectTo )
		{
			charcreation_step = 0;

			// close current window
			int temp1 = connectingToLobby;
			int temp2 = connectingToLobbyWindow;
			//buttonCloseSubwindow(my);
			list_FreeAll(&button_l);
			deleteallbuttons = true;
			connectingToLobby = temp1;
			connectingToLobbyWindow = temp2;

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
			cpp_SteamMatchmaking_JoinLobby(*static_cast<CSteamID*>(lobbyToConnectTo));
			cpp_Free_CSteamID(lobbyToConnectTo); //TODO: Bugger this.
			lobbyToConnectTo = NULL;
		}
#endif
	}
	else if ( charcreation_step == 6 )
	{
		if ( multiplayerselect == SINGLE )
		{
			buttonStartSingleplayer(my);
		}
		else if ( multiplayerselect == SERVER )
		{
#ifdef STEAMWORKS
			directConnect = false;
#else
			directConnect = true;
#endif
			buttonHostMultiplayer(my);
		}
		else if ( multiplayerselect == CLIENT )
		{
#ifndef STEAMWORKS
			directConnect = true;
			buttonJoinMultiplayer(my);
#else
			directConnect = false;
			openSteamLobbyWaitWindow(my);
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
	int c;

	// close current window
	buttonCloseSubwindow(my);
	list_FreeAll(&button_l);
	deleteallbuttons = true;
	portnumber = atoi(portnumber_char); // get the port number from the text field
	list_FreeAll(&lobbyChatboxMessages);
	newString(&lobbyChatboxMessages, 0xFFFFFFFF, language[1452]);
	if ( loadingsavegame )
	{
		newString(&lobbyChatboxMessages, 0xFFFFFFFF, language[1453]);
	}

	// close any existing net interfaces
	closeNetworkInterfaces();

	if ( !directConnect )
	{
#ifdef STEAMWORKS
		for ( c = 0; c < MAXPLAYERS; c++ )
		{
			if ( steamIDRemote[c] )
			{
				cpp_Free_CSteamID( steamIDRemote[c] ); //TODO: Bugger this.
				steamIDRemote[c] = NULL;
			}
		}
		currentLobbyType = k_ELobbyTypePrivate;
		cpp_SteamMatchmaking_CreateLobby(currentLobbyType, 4);
#endif
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
		tcpset = SDLNet_AllocSocketSet(4);
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
		button = newButton();
		strcpy(button->label, language[1458]);
		button->sizex = strlen(language[1458]) * 12 + 8;
		button->sizey = 20;
		button->x = c;
		button->y = suby2 - 24;
		button->action = &buttonInviteFriends;
		button->visible = 1;
		button->focused = 1;
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
void buttonJoinLobby(button_t* my)
{
	button_t* button;
	int c;

	// refresh keepalive
	client_keepalive[0] = ticks;

	// close current window
#ifdef STEAMWORKS
	int temp1 = connectingToLobby;
	int temp2 = connectingToLobbyWindow;
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

	if ( loadingsavegame )
	{
		loadGame(getSaveGameClientnum());
	}

	// open wait window
	list_FreeAll(&lobbyChatboxMessages);
	newString(&lobbyChatboxMessages, 0xFFFFFFFF, language[1452]);
	multiplayer = CLIENT;
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
		strncpy(address, connectaddress, c); // get the address from the text field
		portnumber = atoi(&connectaddress[c + 1]); // get the port number from the text field
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
		tcpset = SDLNet_AllocSocketSet(4);
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
		strncpy((char*)net_packet->data + 19, stats[getSaveGameClientnum()]->name, 22);
		SDLNet_Write32((Uint32)client_classes[getSaveGameClientnum()], &net_packet->data[42]);
		SDLNet_Write32((Uint32)stats[getSaveGameClientnum()]->sex, &net_packet->data[46]);
		SDLNet_Write32((Uint32)stats[getSaveGameClientnum()]->appearance, &net_packet->data[50]);
		strcpy((char*)net_packet->data + 54, VERSION);
		net_packet->data[62] = 0;
		net_packet->data[63] = getSaveGameClientnum();
	}
	else
	{
		strncpy((char*)net_packet->data + 19, stats[0]->name, 22);
		SDLNet_Write32((Uint32)client_classes[0], &net_packet->data[42]);
		SDLNet_Write32((Uint32)stats[0]->sex, &net_packet->data[46]);
		SDLNet_Write32((Uint32)stats[0]->appearance, &net_packet->data[50]);
		strcpy((char*)net_packet->data + 54, VERSION);
		net_packet->data[62] = 0;
		net_packet->data[63] = 0;
	}
	if ( loadingsavegame )
	{
		// send over the map seed being used
		SDLNet_Write32(getSaveGameMapSeed(), &net_packet->data[64]);
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
#ifdef STEAMWORKS
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
		net_packet->address.host = net_clients[c - 1].host;
		net_packet->address.port = net_clients[c - 1].port;
		net_packet->len = 25;
		sendPacketSafe(net_sock, -1, net_packet, c - 1);
	}
}

// opens the steam dialog to invite friends
#ifdef STEAMWORKS
void buttonInviteFriends(button_t* my)
{
	if (SteamUser()->BLoggedOn())
	{
		SteamFriends()->ActivateGameOverlayInviteDialog(*static_cast<CSteamID*>(currentLobby));
	}
	return;
}
#endif

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
	fullscreen = settings_fullscreen;
	shaking = settings_shaking;
	bobbing = settings_bobbing;
	spawn_blood = settings_spawn_blood;
	colorblind = settings_colorblind;
	oldGamma = vidgamma;
	vidgamma = settings_gamma;
	fpsLimit = settings_fps;
	oldXres = xres;
	oldYres = yres;
	xres = settings_xres;
	yres = settings_yres;
	camera.winx = 0;
	camera.winy = 0;
	camera.winw = std::min(camera.winw, xres);
	camera.winh = std::min(camera.winh, yres);
	if(xres!=oldXres || yres!=oldYres || oldFullscreen!=fullscreen || oldGamma!=vidgamma)
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
	musvolume = settings_musvolume;

#ifdef HAVE_FMOD
	FMOD_ChannelGroup_SetVolume(music_group, musvolume / 128.f);
	FMOD_ChannelGroup_SetVolume(sound_group, sfxvolume / 128.f);
#elif defined HAVE_OPENAL
	OPENAL_ChannelGroup_SetVolume(music_group, musvolume / 128.f);
	OPENAL_ChannelGroup_SetVolume(sound_group, sfxvolume / 128.f);
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
	mousespeed = settings_mousespeed;

	// set misc options
	broadcast = settings_broadcast;
	nohud = settings_nohud;

	auto_hotbar_new_items = settings_auto_hotbar_new_items;
	for ( c = 0; c < NUM_HOTBAR_CATEGORIES; ++c )
	{
		auto_hotbar_categories[c] = settings_auto_hotbar_categories[c];
	}
	disable_messages = settings_disable_messages;
	right_click_protect = settings_right_click_protect;
	auto_appraise_new_items = settings_auto_appraise_new_items;

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

	saveConfig("default.cfg");
}

void openConfirmResolutionWindow()
{
	mousestatus[SDL_BUTTON_LEFT] = 0;
	keystatus[SDL_SCANCODE_RETURN] = 0;
	*inputPressed(joyimpulses[INJOY_MENU_NEXT]) = 0;
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
	score_window = std::min<int>(score_window + 1, std::max<Uint32>(1, list_Size(&topscores)));
	loadScore(score_window - 1);
}

// previous score button (statistics window)
void buttonScorePrev(button_t* my)
{
	score_window = std::max(score_window - 1, 1);
	loadScore(score_window - 1);
}

// handles slider
void doSlider(int x, int y, int dots, int minvalue, int maxvalue, int increment, int* var, SDL_Surface* slider_font, int slider_font_char_width)
{
	int c;

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
	if ( mousestatus[SDL_BUTTON_LEFT] )
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
	if ( mousestatus[SDL_BUTTON_LEFT] )
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
	char* saveGameName = getSaveGameName();
	strcat(subtext, saveGameName);
	free(saveGameName);
	strcat(subtext, language[1461]);

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
	strcpy(button->label, language[1462]);
	button->sizex = strlen(language[1462]) * 12 + 8;
	button->sizey = 20;
	button->x = subx1 + (subx2 - subx1) / 2 - button->sizex / 2;
	button->y = suby2 - 52;
	button->action = &buttonLoadGame;
	button->visible = 1;
	button->focused = 1;
	button->joykey = joyimpulses[INJOY_MENU_NEXT]; //load save game yes => "a" button

	// no button
	button = newButton();
	strcpy(button->label, language[1463]);
	button->sizex = strlen(language[1463]) * 12 + 8;
	button->sizey = 20;
	button->x = subx1 + (subx2 - subx1) / 2 - button->sizex / 2;
	button->y = suby2 - 28;
	button->action = &buttonOpenCharacterCreationWindow;
	button->visible = 1;
	button->focused = 1;
	button->key = SDL_SCANCODE_RETURN;
	button->joykey = joyimpulses[INJOY_MENU_DONT_LOAD_SAVE]; //load save game no => "y" button
}

void buttonOpenCharacterCreationWindow(button_t* my)
{
	button_t* button;

	playing_random_char = false;
	loadingsavegame = 0;

	// reset class loadout
	clientnum = 0;
	stats[0]->sex = static_cast<sex_t>(0);
	stats[0]->appearance = 0;
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
	subwindow = 1;
	subx1 = xres / 2 - 400;
	subx2 = xres / 2 + 400;
	suby1 = yres / 2 - 240;
	suby2 = yres / 2 + 240;
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

void buttonLoadGame(button_t* button)
{
	loadingsavegame = getSaveGameUniqueGameKey();
	int mul = getSaveGameType();

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
#ifdef STEAMWORKS
		if ( mul == SERVER )
		{
			buttonHostMultiplayer(button);
		}
		else if ( mul == CLIENT )
		{
			if ( !lobbyToConnectTo )
			{
				openSteamLobbyBrowserWindow(button);
			}
			else
			{
				// close current window
				int temp1 = connectingToLobby;
				int temp2 = connectingToLobbyWindow;
				//buttonCloseSubwindow(button);
				list_FreeAll(&button_l);
				deleteallbuttons = true;
				connectingToLobby = temp1;
				connectingToLobbyWindow = temp2;

				// create new window
				subwindow = 1;
				subx1 = xres / 2 - 256;
				subx2 = xres / 2 + 256;
				suby1 = yres / 2 - 64;
				suby2 = yres / 2 + 64;
				strcpy(subtext, language[1467]);

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
				cpp_SteamMatchmaking_JoinLobby(*static_cast<CSteamID* >(lobbyToConnectTo));
				cpp_Free_CSteamID(lobbyToConnectTo);
				lobbyToConnectTo = NULL;
			}
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
	stats[0]->sex = static_cast<sex_t>(rand() % 2);
	client_classes[0] = rand() % NUMCLASSES;
	stats[0]->clearStats();
	initClass(0);
	stats[0]->appearance = rand() % NUMAPPEARANCES;
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

	strncpy(inputstr, name.c_str(), std::min<size_t>(name.length(), inputlen));
	inputstr[std::min<size_t>(name.length(), inputlen)] = '\0';
}
