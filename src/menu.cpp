/*-------------------------------------------------------------------------------

	BARONY
	File: menu.cpp
	Desc: contains code for all menu buttons in the game

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

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

#ifdef STEAMWORKS
//Helper func. //TODO: Bugger.
void* cpp_SteamMatchmaking_GetLobbyOwner(void *steamIDLobby)
{
	CSteamID *id = new CSteamID();
	*id = SteamMatchmaking()->GetLobbyOwner(*static_cast<CSteamID*>(steamIDLobby));
	return id; //Still don't like this method.
}
#endif

// menu variables
bool lobby_window=FALSE;
bool settings_window=FALSE;
int connect_window=0;
int charcreation_step=0;
int settings_tab=0;
int score_window=0;
int resolutions[NUMRESOLUTIONS][2] = {
	{ 960, 600 },
	{ 1024, 768 },
	{ 1280, 720 },
	{ 1280, 800 },
	{ 1280, 960 },
	{ 1366, 768 },
	{ 1600, 900 },
	{ 1600, 1000 },
	{ 1920, 1080 },
	{ 1920, 1200 }
};
int settings_xres, settings_yres;
Uint32 settings_fov;
bool settings_smoothlighting;
int settings_fullscreen, settings_shaking, settings_bobbing;
double settings_gamma;
int settings_sfxvolume, settings_musvolume;
int settings_impulses[NUMIMPULSES];
int settings_reversemouse;
double settings_mousespeed;
bool settings_broadcast;
bool settings_nohud;
bool settings_colorblind;
bool settings_spawn_blood;
char portnumber_char[6];
char connectaddress[64];
char classtoquickstart[256]="";
bool spawn_blood = TRUE;
int multiplayerselect=SINGLE;
int menuselect=0;
bool settings_auto_hotbar_new_items = TRUE;
bool settings_disable_messages = TRUE;
bool playing_random_char = FALSE;
bool colorblind = FALSE;
Sint32 oslidery=0;

Uint32 colorWhite = 0xFFFFFFFF;

/*-------------------------------------------------------------------------------

	handleMainMenu
	
	draws & processes the game menu; if passed TRUE, does the whole menu,
	otherwise just handles the reduced ingame menu

-------------------------------------------------------------------------------*/

int firstendmoviealpha[30];
int secondendmoviealpha[30];
int intromoviealpha[30];
int rebindkey=-1;

Sint32 gearrot=0;
Sint32 gearsize=5000;
Uint16 logoalpha=0;
int credittime=0;
int creditstage=0;
int intromovietime=0;
int intromoviestage=0;
int firstendmovietime=0;
int firstendmoviestage=0;
int secondendmovietime=0;
int secondendmoviestage=0;
double drunkextend=0;
bool losingConnection[4] = { FALSE };
bool subtitleVisible=FALSE;
int subtitleCurrent=0;

void handleMainMenu(bool mode) {
	SDL_Rect pos, src, dest;
	int x, c;
	//int y;
	bool b;
	//int tilesreceived=0;
	//Mix_Music **music, *intromusic, *splashmusic, *creditsmusic;
	node_t *node, *nextnode;
	Entity *entity;
	FILE *fp;
	//SDL_Surface *sky_bmp;
	button_t *button;
	
	if( !movie ) {
		// title pic
		src.x = 0; src.y = 0;
		src.w = title_bmp->w; src.h = title_bmp->h;
		dest.x = 20; dest.y = 20;
		dest.w = xres; dest.h = yres;
		if( mode || introstage!=5 ) {
			drawImage(title_bmp, &src, &dest);
		}
		if( mode && subtitleVisible ) {
			Uint32 colorYellow = SDL_MapRGBA(mainsurface->format,255,255,0,255);
			ttfPrintTextColor(ttf16,176,20+title_bmp->h-24,colorYellow,TRUE,language[1910+subtitleCurrent]);
		}
		
		// print game version
		if( mode || introstage!=5 ) {
			char version[64];
			strcpy(version,__DATE__+7);
			strcat(version,".");
			if( !strncmp(__DATE__,"Jan",3) )
				strcat(version,"01");
			else if( !strncmp(__DATE__,"Feb",3) )
				strcat(version,"02");
			else if( !strncmp(__DATE__,"Mar",3) )
				strcat(version,"03");
			else if( !strncmp(__DATE__,"Apr",3) )
				strcat(version,"04");
			else if( !strncmp(__DATE__,"May",3) )
				strcat(version,"05");
			else if( !strncmp(__DATE__,"Jun",3) )
				strcat(version,"06");
			else if( !strncmp(__DATE__,"Jul",3) )
				strcat(version,"07");
			else if( !strncmp(__DATE__,"Aug",3) )
				strcat(version,"08");
			else if( !strncmp(__DATE__,"Sep",3) )
				strcat(version,"09");
			else if( !strncmp(__DATE__,"Oct",3) )
				strcat(version,"10");
			else if( !strncmp(__DATE__,"Nov",3) )
				strcat(version,"11");
			else if( !strncmp(__DATE__,"Dec",3) )
				strcat(version,"12");
			strcat(version,".");
			int day = atoi(__DATE__+4);
			if(day >= 10) {
				strncat(version,__DATE__+4,2);
			} else {
				strcat(version,"0");
				strncat(version,__DATE__+5,1);
			}
			int w, h;
			TTF_SizeUTF8(ttf8,version,&w,&h);
			ttfPrintTextFormatted(ttf8,xres-8-w,yres-4-h,"%s",version);
			int h2 = h;
			TTF_SizeUTF8(ttf8,VERSION,&w,&h);
			ttfPrintTextFormatted(ttf8,xres-8-w,yres-8-h-h2,VERSION);
		}

		// navigate with arrow keys
		if( !subwindow ) {
			if( menuselect==0 ) {
				if( keystatus[SDL_SCANCODE_UP] ) {
					keystatus[SDL_SCANCODE_UP] = 0;
					menuselect=1;
				} else if( keystatus[SDL_SCANCODE_DOWN] ) {
					keystatus[SDL_SCANCODE_DOWN] = 0;
					menuselect=1;
				}
			} else {
				if( keystatus[SDL_SCANCODE_UP] ) {
					keystatus[SDL_SCANCODE_UP] = 0;
					menuselect--;
					if( menuselect==0 ) {
						if( mode )
							menuselect=6;
						else
							menuselect=4+(multiplayer!=CLIENT);
					}
				} else if( keystatus[SDL_SCANCODE_DOWN] ) {
					keystatus[SDL_SCANCODE_DOWN] = 0;
					menuselect++;
					if( mode ) {
						if( menuselect>6 )
							menuselect=1;
					} else {
						if( menuselect>4+(multiplayer!=CLIENT) )
							menuselect=1;
					}
				}
			}
		}
		
		// gray text color
		Uint32 colorGray = SDL_MapRGBA(mainsurface->format,128,128,128,255);

		// draw menu
		if( mode ) {
			if( ((omousex >= 50 && omousex < 50+strlen(language[1303])*18 && omousey >= yres/4+80 && omousey < yres/4+80+18) || (menuselect==1)) && subwindow==0 && introstage==1 ) {
				ttfPrintTextFormattedColor(ttf16, 50, yres/4+80, colorGray, language[1303]);
				if( mousestatus[SDL_BUTTON_LEFT] || keystatus[SDL_SCANCODE_RETURN] ) {
					mousestatus[SDL_BUTTON_LEFT]=0;
					keystatus[SDL_SCANCODE_RETURN]=0;
					playSound(139,64);
					
					// look for a save game
					if( saveGameExists() ) {
						openLoadGameWindow(NULL);
					} else {
						buttonOpenCharacterCreationWindow(NULL);
					}
				}
			} else {
				ttfPrintText(ttf16, 50, yres/4+80, language[1303]);
			}
			if( ((omousex >= 50 && omousex < 50+strlen(language[1304])*18 && omousey >= yres/4+104 && omousey < yres/4+104+18) || (menuselect==2)) && subwindow==0 && introstage==1 ) {
				ttfPrintTextFormattedColor(ttf16, 50, yres/4+104, colorGray, language[1304]);
				if( mousestatus[SDL_BUTTON_LEFT] || keystatus[SDL_SCANCODE_RETURN] ) {
					mousestatus[SDL_BUTTON_LEFT]=0;
					keystatus[SDL_SCANCODE_RETURN]=0;
					playSound(139,64);
					introstage=6; // goes to intro movie
					playmusic(introductionmusic, TRUE, TRUE, FALSE);
					fadeout=TRUE;
				}
			} else {
				ttfPrintText(ttf16, 50, yres/4+104, language[1304]);
			}
			if( ((omousex >= 50 && omousex < 50+strlen(language[1305])*18 && omousey >= yres/4+128 && omousey < yres/4+128+18) || (menuselect==3)) && subwindow==0 && introstage==1 ) {
				ttfPrintTextFormattedColor(ttf16, 50, yres/4+128, colorGray, language[1305]);
				if( mousestatus[SDL_BUTTON_LEFT] || keystatus[SDL_SCANCODE_RETURN] ) {
					mousestatus[SDL_BUTTON_LEFT]=0;
					keystatus[SDL_SCANCODE_RETURN]=0;
					playSound(139,64);
					
					// create statistics window
					clientnum=0;
					subwindow = 1;
					score_window = 1;
					loadScore(0);
					subx1 = xres/2-400;
					subx2 = xres/2+400;
					suby1 = yres/2-240;
					suby2 = yres/2+240;
					strcpy(subtext,"");

					// close button
					button = newButton();
					strcpy(button->label,"x");
					button->x=subx2-20; button->y=suby1;
					button->sizex=20; button->sizey=20;
					button->action=&buttonCloseSubwindow;
					button->visible=1;
					button->focused=1;
					button->key=SDL_SCANCODE_ESCAPE;
					
					// next button
					button = newButton();
					strcpy(button->label,">");
					button->sizex=strlen(">")*12+8; button->sizey=20;
					button->x=subx2-button->sizex-4; button->y=suby2-24;
					button->action=&buttonScoreNext;
					button->visible=1;
					button->focused=1;
					button->key=SDL_SCANCODE_RIGHT;
					
					// previous button
					button = newButton();
					strcpy(button->label,"<");
					button->sizex=strlen("<")*12+8; button->sizey=20;
					button->x=subx1+4; button->y=suby2-24;
					button->action=&buttonScorePrev;
					button->visible=1;
					button->focused=1;
					button->key=SDL_SCANCODE_LEFT;
				}
			} else {
				ttfPrintText(ttf16, 50, yres/4+128, language[1305]);
			}
			if( ((omousex >= 50 && omousex < 50+strlen(language[1306])*18 && omousey >= yres/4+152 && omousey < yres/4+152+18) || (menuselect==4)) && subwindow==0 && introstage==1 ) {
				ttfPrintTextFormattedColor(ttf16, 50, yres/4+152, colorGray, language[1306]);
				if( mousestatus[SDL_BUTTON_LEFT] || keystatus[SDL_SCANCODE_RETURN] ) {
					mousestatus[SDL_BUTTON_LEFT]=0;
					keystatus[SDL_SCANCODE_RETURN]=0;
					playSound(139,64);
					openSettingsWindow();
				}
			} else {
				ttfPrintText(ttf16, 50, yres/4+152, language[1306]);
			}
			if( ((omousex >= 50 && omousex < 50+strlen(language[1307])*18 && omousey >= yres/4+176 && omousey < yres/4+176+18) || (menuselect==5)) && subwindow==0 && introstage==1 ) {
				ttfPrintTextFormattedColor(ttf16, 50, yres/4+176, colorGray, language[1307]);
				if( mousestatus[SDL_BUTTON_LEFT] || keystatus[SDL_SCANCODE_RETURN] ) {
					mousestatus[SDL_BUTTON_LEFT]=0;
					keystatus[SDL_SCANCODE_RETURN]=0;
					playSound(139,64);
					introstage=4; // goes to credits
					fadeout=TRUE;
				}
			} else {
				ttfPrintText(ttf16, 50, yres/4+176, language[1307]);
			}
			if( ((omousex >= 50 && omousex < 50+strlen(language[1308])*18 && omousey >= yres/4+200 && omousey < yres/4+200+18) || (menuselect==6)) && subwindow==0 && introstage==1 ) {
				ttfPrintTextFormattedColor(ttf16, 50, yres/4+200, colorGray, language[1308]);
				if( mousestatus[SDL_BUTTON_LEFT] || keystatus[SDL_SCANCODE_RETURN] ) {
					mousestatus[SDL_BUTTON_LEFT]=0;
					keystatus[SDL_SCANCODE_RETURN]=0;
					playSound(139,64);
					
					// create confirmation window
					subwindow = 1;
					subx1 = xres/2-128;
					subx2 = xres/2+128;
					suby1 = yres/2-40;
					suby2 = yres/2+40;
					strcpy(subtext,language[1128]);

					// close button
					button = newButton();
					strcpy(button->label,"x");
					button->x=subx2-20; button->y=suby1;
					button->sizex=20; button->sizey=20;
					button->action=&buttonCloseSubwindow;
					button->visible=1;
					button->focused=1;
					button->key=SDL_SCANCODE_ESCAPE;
					
					// yes button
					button = newButton();
					strcpy(button->label,language[1314]);
					button->x=subx1+8; button->y=suby2-28;
					button->sizex=strlen(language[1314])*12+8; button->sizey=20;
					button->action=&buttonQuitConfirm;
					button->visible=1;
					button->focused=1;
					button->key=SDL_SCANCODE_RETURN;
					
					// no button
					button = newButton();
					strcpy(button->label,language[1315]);
					button->x=subx2-strlen(language[1315])*12-16; button->y=suby2-28;
					button->sizex=strlen(language[1315])*12+8; button->sizey=20;
					button->action=&buttonCloseSubwindow;
					button->visible=1;
					button->focused=1;
				}
			} else {
				ttfPrintText(ttf16, 50, yres/4+200, language[1308]);
			}
		} else {
			if( introstage!=5 ) {
				if( ((omousex >= 50 && omousex < 50+strlen(language[1309])*18 && omousey >= yres/4+80 && omousey < yres/4+80+18) || (menuselect==1)) && subwindow==0 && introstage==1 ) {
					ttfPrintTextFormattedColor(ttf16, 50, yres/4+80, colorGray, language[1309]);
					if( mousestatus[SDL_BUTTON_LEFT] || keystatus[SDL_SCANCODE_RETURN] ) {
						mousestatus[SDL_BUTTON_LEFT] = 0;
						keystatus[SDL_SCANCODE_RETURN]=0;
						playSound(139,64);
						pauseGame(1,MAXPLAYERS);
					}
				} else {
					ttfPrintText(ttf16, 50, yres/4+80, language[1309]);
				}
				if( ((omousex >= 50 && omousex < 50+strlen(language[1306])*18 && omousey >= yres/4+104 && omousey < yres/4+104+18) || (menuselect==2)) && subwindow==0 && introstage==1 ) {
					ttfPrintTextFormattedColor(ttf16, 50, yres/4+104, colorGray, language[1306]);
					if( mousestatus[SDL_BUTTON_LEFT] || keystatus[SDL_SCANCODE_RETURN] ) {
						mousestatus[SDL_BUTTON_LEFT] = 0;
						keystatus[SDL_SCANCODE_RETURN]=0;
						playSound(139,64);
						openSettingsWindow();
					}
				} else {
					ttfPrintText(ttf16, 50, yres/4+104, language[1306]);
				}
				char *endgameText=NULL;
				if( multiplayer==SINGLE )
					endgameText = language[1310];
				else
					endgameText = language[1311];
				if( ((omousex >= 50 && omousex < 50+strlen(endgameText)*18 && omousey >= yres/4+128 && omousey < yres/4+128+18) || (menuselect==3)) && subwindow==0 && introstage==1 ) {
					ttfPrintTextFormattedColor(ttf16, 50, yres/4+128, colorGray, endgameText);
					if( mousestatus[SDL_BUTTON_LEFT] || keystatus[SDL_SCANCODE_RETURN] ) {
						mousestatus[SDL_BUTTON_LEFT] = 0;
						keystatus[SDL_SCANCODE_RETURN]=0;
						playSound(139,64);
						
						// create confirmation window
						subwindow = 1;
						subx1 = xres/2-140;
						subx2 = xres/2+140;
						suby1 = yres/2-48;
						suby2 = yres/2+48;
						strcpy(subtext,language[1129]);

						// close button
						button = newButton();
						strcpy(button->label,"x");
						button->x=subx2-20; button->y=suby1;
						button->sizex=20; button->sizey=20;
						button->action=&buttonCloseSubwindow;
						button->visible=1;
						button->focused=1;
						button->key=SDL_SCANCODE_ESCAPE;
						
						// yes button
						button = newButton();
						strcpy(button->label,language[1314]);
						button->x=subx1+8; button->y=suby2-28;
						button->sizex=strlen(language[1314])*12+8; button->sizey=20;
						button->action=&buttonEndGameConfirm;
						button->visible=1;
						button->focused=1;
						button->key=SDL_SCANCODE_RETURN;
						
						// no button
						button = newButton();
						strcpy(button->label,language[1315]);
						button->x=subx2-strlen(language[1315])*12-16; button->y=suby2-28;
						button->sizex=strlen(language[1315])*12+8; button->sizey=20;
						button->action=&buttonCloseSubwindow;
						button->visible=1;
						button->focused=1;
					}
				} else {
					ttfPrintText(ttf16, 50, yres/4+128, endgameText);
				}
				if( multiplayer!=CLIENT ) {
					if( ((omousex >= 50 && omousex < 50+strlen(language[1312])*18 && omousey >= yres/4+152 && omousey < yres/4+152+18) || (menuselect==4)) && subwindow==0 && introstage==1 ) {
						ttfPrintTextFormattedColor(ttf16, 50, yres/4+152, colorGray, language[1312]);
						if( mousestatus[SDL_BUTTON_LEFT] || keystatus[SDL_SCANCODE_RETURN] ) {
							mousestatus[SDL_BUTTON_LEFT] = 0;
							keystatus[SDL_SCANCODE_RETURN]=0;
							playSound(139,64);
						
							// create confirmation window
							subwindow = 1;
							subx1 = xres/2-164;
							subx2 = xres/2+164;
							suby1 = yres/2-48;
							suby2 = yres/2+48;
							strcpy(subtext,language[1130]);

							// close button
							button = newButton();
							strcpy(button->label,"x");
							button->x=subx2-20; button->y=suby1;
							button->sizex=20; button->sizey=20;
							button->action=&buttonCloseSubwindow;
							button->visible=1;
							button->focused=1;
							button->key=SDL_SCANCODE_ESCAPE;
						
							// yes button
							button = newButton();
							strcpy(button->label,language[1314]);
							button->x=subx1+8; button->y=suby2-28;
							button->sizex=strlen(language[1314])*12+8; button->sizey=20;
							if( multiplayer==SINGLE )
								button->action=&buttonStartSingleplayer;
							else
								button->action=&buttonStartServer;
							button->visible=1;
							button->focused=1;
							button->key=SDL_SCANCODE_RETURN;
						
							// no button
							button = newButton();
							strcpy(button->label,language[1315]);
							button->x=subx2-strlen(language[1315])*12-16; button->y=suby2-28;
							button->sizex=strlen(language[1315])*12+8; button->sizey=20;
							button->action=&buttonCloseSubwindow;
							button->visible=1;
							button->focused=1;
						}
					} else {
						ttfPrintText(ttf16, 50, yres/4+152, language[1312]);
					}
				}
				if( ((omousex >= 50 && omousex < 50+strlen(language[1313])*18 && omousey >= yres/4+152+24*(multiplayer!=CLIENT) && omousey < yres/4+152+18+24*(multiplayer!=CLIENT)) || (menuselect==4+(multiplayer!=CLIENT))) && subwindow==0 && introstage==1 ) {
					ttfPrintTextFormattedColor(ttf16, 50, yres/4+152+24*(multiplayer!=CLIENT), colorGray, language[1313]);
					if( mousestatus[SDL_BUTTON_LEFT] || keystatus[SDL_SCANCODE_RETURN] ) {
						mousestatus[SDL_BUTTON_LEFT] = 0;
						keystatus[SDL_SCANCODE_RETURN]=0;
						playSound(139,64);
						
						// create confirmation window
						subwindow = 1;
						subx1 = xres/2-188;
						subx2 = xres/2+188;
						suby1 = yres/2-64;
						suby2 = yres/2+64;
						strcpy(subtext,language[1131]);
						
						// yes button
						button = newButton();
						strcpy(button->label,language[1314]);
						button->x=subx1+8; button->y=suby2-28;
						button->sizex=strlen(language[1314])*12+8; button->sizey=20;
						button->action=&buttonQuitConfirm;
						button->visible=1;
						button->focused=1;
						button->key=SDL_SCANCODE_RETURN;

						// no button
						button = newButton();
						strcpy(button->label,language[1315]);
						button->sizex=strlen(language[1315])*12+8; button->sizey=20;
						button->x=subx1+(subx2-subx1)/2-button->sizex/2; button->y=suby2-28;
						button->action=&buttonQuitNoSaveConfirm;
						button->visible=1;
						button->focused=1;
						
						// cancel button
						button = newButton();
						strcpy(button->label,language[1316]);
						button->x=subx2-strlen(language[1316])*12-16; button->y=suby2-28;
						button->sizex=strlen(language[1316])*12+8; button->sizey=20;
						button->action=&buttonCloseSubwindow;
						button->visible=1;
						button->focused=1;

						// close button
						button = newButton();
						strcpy(button->label,"x");
						button->x=subx2-20; button->y=suby1;
						button->sizex=20; button->sizey=20;
						button->action=&buttonCloseSubwindow;
						button->visible=1;
						button->focused=1;
						button->key=SDL_SCANCODE_ESCAPE;
					}
				} else {
					ttfPrintText(ttf16, 50, yres/4+152+24*(multiplayer!=CLIENT), language[1313]);
				}
			}
		}

		#ifdef STEAMWORKS
		if ( intro ) {
			// lobby list request succeeded
			if ( !requestingLobbies && !strcmp(subtext,language[1132]) ) {
				openSteamLobbyBrowserWindow(NULL);
			}

			// lobby entered
			if ( !connectingToLobby && connectingToLobbyWindow ) {
				connectingToLobbyWindow = FALSE;
				connectingToLobby = FALSE;

				// close current window
				buttonCloseSubwindow(NULL);
				list_FreeAll(&button_l);
				deleteallbuttons=TRUE;

				// we are assuming here that the lobby join was successful
				// otherwise, the callback would've flipped off the connectingToLobbyWindow and opened an error window

				// get number of lobby members (capped to game limit)

				// record CSteamID of lobby owner (and nobody else)
				SteamMatchmaking()->GetNumLobbyMembers(*static_cast<CSteamID*>(currentLobby));
				if ( steamIDRemote[0] )
					cpp_Free_CSteamID(steamIDRemote[0]);
				steamIDRemote[0] = cpp_SteamMatchmaking_GetLobbyOwner(currentLobby); //TODO: Bugger void pointers!
				int c;
				for ( c=1; c<MAXPLAYERS; c++ ) {
					if ( steamIDRemote[c] ) {
						cpp_Free_CSteamID(steamIDRemote[c]);
						steamIDRemote[c] = NULL;
					}
				}

				buttonJoinLobby(NULL);
			}
		}
		#endif
		
		// draw subwindow
		if( subwindow ) {
			drawWindowFancy(subx1,suby1,subx2,suby2);
			if( subtext != NULL ) {
				if( strncmp(subtext,language[740],12) )
					ttfPrintTextFormatted(ttf12, subx1+8, suby1+8, subtext);
				else
					ttfPrintTextFormatted(ttf16, subx1+8, suby1+8, subtext);
			}
		}

		// process button actions
		handleButtons();
	}
	
	// character creation screen
	if( charcreation_step >= 1 && charcreation_step < 6 ) {
		ttfPrintText(ttf16, subx1+8, suby1+8, language[1318]);
	
		// draw character window
		if( players[clientnum] != NULL ) {
			camera_charsheet.x=players[clientnum]->x/16.0+1;
			camera_charsheet.y=players[clientnum]->y/16.0-.5;
			camera_charsheet.z=players[clientnum]->z*2;
			camera_charsheet.ang=atan2(players[clientnum]->y/16.0-camera_charsheet.y,players[clientnum]->x/16.0-camera_charsheet.x);
			camera_charsheet.vang=PI/24;
			camera_charsheet.winw=360;
			camera_charsheet.winy=suby1+32;
			camera_charsheet.winh=suby2-96-camera_charsheet.winy;
			camera_charsheet.winx=subx2-camera_charsheet.winw-32;
			pos.x = camera_charsheet.winx; pos.y = camera_charsheet.winy;
			pos.w = camera_charsheet.winw; pos.h = camera_charsheet.winh;
			drawRect(&pos,0,255);
			b=players[clientnum]->flags[BRIGHT];
			players[clientnum]->flags[BRIGHT]=TRUE;
			if (!playing_random_char) {
				if( !players[clientnum]->flags[INVISIBLE] ) {
					double ofov = fov;
					fov = 50;
					glDrawVoxel(&camera_charsheet,players[clientnum],REALCOLORS);
					fov = ofov;
				}
				players[clientnum]->flags[BRIGHT]=b;
				c=0;
				for( node=players[clientnum]->children.first; node!=NULL; node=node->next ) {
					if( c==0 ) {
						c++;
					}
					entity = (Entity *) node->element;
					if( !entity->flags[INVISIBLE] ) {
						b=entity->flags[BRIGHT];
						entity->flags[BRIGHT]=TRUE;
						double ofov = fov;
						fov = 50;
						glDrawVoxel(&camera_charsheet,entity,REALCOLORS);
						fov = ofov;
						entity->flags[BRIGHT]=b;
					}
					c++;
				}
			}
		}
			
		//TODO: Loop through buttons. Disable the random character button if charcreation_step != 1;
		// sexes
		if( charcreation_step==1 ) {
			ttfPrintText(ttf16, subx1+24, suby1+32, language[1319]);
			if( stats[0].sex==0 ) {
				ttfPrintTextFormatted(ttf16, subx1+32, suby1+56, "[o] %s", language[1321]);
				ttfPrintTextFormatted(ttf16, subx1+32, suby1+72, "[ ] %s", language[1322]);
			
				ttfPrintTextFormatted(ttf12, subx1+8, suby2-80, language[1320], language[1321]);
			} else {
				ttfPrintTextFormatted(ttf16, subx1+32, suby1+56, "[ ] %s", language[1321]);
				ttfPrintTextFormatted(ttf16, subx1+32, suby1+72, "[o] %s", language[1322]);
			
				ttfPrintTextFormatted(ttf12, subx1+8, suby2-80, language[1320], language[1322]);
			}
			if( mousestatus[SDL_BUTTON_LEFT] ) {
				if( omousex >= subx1+40 && omousex < subx1+72 ) {
					if( omousey >= suby1+56 && omousey < suby1+72 ) {
						mousestatus[SDL_BUTTON_LEFT] = 0;
						stats[0].sex = MALE;
					}
					else if( omousey >= suby1+72 && omousey < suby1+88 ) {
						mousestatus[SDL_BUTTON_LEFT] = 0;
						stats[0].sex = FEMALE;
					}
				}
			}
			if( keystatus[SDL_SCANCODE_UP] ) {
				keystatus[SDL_SCANCODE_UP] = 0;
				stats[0].sex = static_cast<sex_t>((stats[0].sex==MALE));
			}
			if( keystatus[SDL_SCANCODE_DOWN] ) {
				keystatus[SDL_SCANCODE_DOWN] = 0;
				stats[0].sex = static_cast<sex_t>((stats[0].sex==MALE));
			}
		}
	
		// classes
		else if( charcreation_step==2 ) {
			ttfPrintText(ttf16, subx1+24, suby1+32, language[1323]);
			for( c=0; c<10; c++ ) {
				if( c==client_classes[0] ) {
					ttfPrintTextFormatted(ttf16, subx1+32, suby1+56+16*c, "[o] %s",language[1900+c]);
				} else {
					ttfPrintTextFormatted(ttf16, subx1+32, suby1+56+16*c, "[ ] %s",language[1900+c]);
				}
		
				if( mousestatus[SDL_BUTTON_LEFT] ) {
					if( omousex >= subx1+40 && omousex < subx1+72 ) {
						if( omousey >= suby1+56+16*c && omousey < suby1+72+16*c ) {
							mousestatus[SDL_BUTTON_LEFT] = 0;
							client_classes[0] = c;
						
							// reset class loadout
							clearStats(&stats[0]);
							initClass(0);
						}
					}
				}
				if( keystatus[SDL_SCANCODE_UP] ) {
					keystatus[SDL_SCANCODE_UP] = 0;
					client_classes[0]--;
					if( client_classes[0]<0 )
						client_classes[0]=9;
				
					// reset class loadout
					clearStats(&stats[0]);
					initClass(0);
				}
				if( keystatus[SDL_SCANCODE_DOWN] ) {
					keystatus[SDL_SCANCODE_DOWN] = 0;
					client_classes[0]++;
					if( client_classes[0]>9 )
						client_classes[0]=0;
				
					// reset class loadout
					clearStats(&stats[0]);
					initClass(0);
				}
			}
		
			// class description
			ttfPrintText(ttf12, subx1+8, suby2-80, language[10+client_classes[0]]);
		}
	
		// faces
		else if( charcreation_step==3 ) {
			ttfPrintText(ttf16, subx1+24, suby1+32, language[1324]);
			for( c=0; c<NUMAPPEARANCES; c++ ) {
				if( stats[0].appearance==c ) {
					ttfPrintTextFormatted(ttf16, subx1+32, suby1+56+c*16, "[o] %s", language[20+c]);
					ttfPrintText(ttf12, subx1+8, suby2-80, language[38+c]);
				} else {
					ttfPrintTextFormatted(ttf16, subx1+32, suby1+56+c*16, "[ ] %s", language[20+c]);
				}
				if( mousestatus[SDL_BUTTON_LEFT] ) {
					if( omousex >= subx1+40 && omousex < subx1+72 ) {
						if( omousey >= suby1+56+16*c && omousey < suby1+72+16*c ) {
							mousestatus[SDL_BUTTON_LEFT] = 0;
							stats[0].appearance = c;
						}
					}
				}
				if( keystatus[SDL_SCANCODE_UP] ) {
					keystatus[SDL_SCANCODE_UP] = 0;
					stats[0].appearance--;
					if( stats[0].appearance>=NUMAPPEARANCES )
						stats[0].appearance=NUMAPPEARANCES-1;
				}
				if( keystatus[SDL_SCANCODE_DOWN] ) {
					keystatus[SDL_SCANCODE_DOWN] = 0;
					stats[0].appearance++;
					if( stats[0].appearance>=NUMAPPEARANCES )
						stats[0].appearance=0;
				}
			}
		}
	
		// name
		else if( charcreation_step==4 ) {
			ttfPrintText(ttf16, subx1+24, suby1+32, language[1325]);
			drawDepressed(subx1+40,suby1+56,subx1+364,suby1+88);
			ttfPrintText(ttf16,subx1+48,suby1+64,stats[0].name);
			ttfPrintText(ttf12, subx1+8, suby2-80, language[1326]);
		
			// enter character name
			if( !SDL_IsTextInputActive() ) {
				inputstr = stats[0].name;
				SDL_StartTextInput();
			}
			//strncpy(stats[0].name,inputstr,16);
			inputlen = 22;
			if( (ticks-cursorflash)%TICKS_PER_SECOND<TICKS_PER_SECOND/2 ) {
				int x;
				TTF_SizeUTF8(ttf16,stats[0].name,&x,NULL);
				ttfPrintText(ttf16,subx1+48+x,suby1+64,"_");
			}
		}

		// gamemode
		else if( charcreation_step==5 ) {
			ttfPrintText(ttf16, subx1+24, suby1+32, language[1327]);

			int nummodes=3;
			#ifdef STEAMWORKS
			nummodes+=2;
			#endif

			for( c=0; c<nummodes; c++ ) {
				if( multiplayerselect==c ) {
					switch( c ) {
						case 0:
							ttfPrintTextFormatted(ttf16, subx1+32, suby1+56, "[o] %s",language[1328]);
							ttfPrintText(ttf12, subx1+8, suby2-80, language[1329]);
							break;
						case 1:
							ttfPrintTextFormatted(ttf16, subx1+32, suby1+76, "[o] %s",language[1330]);
							ttfPrintText(ttf12, subx1+8, suby2-80, language[1331]);
							break;
						case 2:
							ttfPrintTextFormatted(ttf16, subx1+32, suby1+96, "[o] %s",language[1332]);
							ttfPrintText(ttf12, subx1+8, suby2-80, language[1333]);
							break;
						case 3:
							ttfPrintTextFormatted(ttf16, subx1+32, suby1+136, "[o] %s\n     %s",language[1330],language[1537]);
							ttfPrintText(ttf12, subx1+8, suby2-80, language[1538]);
							break;
						case 4:
							ttfPrintTextFormatted(ttf16, subx1+32, suby1+176, "[o] %s\n     %s",language[1332],language[1537]);
							ttfPrintText(ttf12, subx1+8, suby2-80, language[1539]);
							break;
					}
				} else {
					switch( c ) {
						case 0:
							ttfPrintTextFormatted(ttf16, subx1+32, suby1+56, "[ ] %s",language[1328]);
							break;
						case 1:
							ttfPrintTextFormatted(ttf16, subx1+32, suby1+76, "[ ] %s",language[1330]);
							break;
						case 2:
							ttfPrintTextFormatted(ttf16, subx1+32, suby1+96, "[ ] %s",language[1332]);
							break;
						case 3:
							ttfPrintTextFormatted(ttf16, subx1+32, suby1+136, "[ ] %s\n     %s",language[1330],language[1537]);
							break;
						case 4:
							ttfPrintTextFormatted(ttf16, subx1+32, suby1+176, "[ ] %s\n     %s",language[1332],language[1537]);
							break;
					}
				}
				if( mousestatus[SDL_BUTTON_LEFT] ) {
					if( omousex >= subx1+40 && omousex < subx1+72 ) {
						if( c<3 ) {
							if( omousey >= suby1+56+20*c && omousey < suby1+74+20*c ) {
								mousestatus[SDL_BUTTON_LEFT] = 0;
								multiplayerselect = c;
							}
						} else {
							if( omousey >= suby1+136+40*(c-3) && omousey < suby1+148+40*(c-3) ) {
								mousestatus[SDL_BUTTON_LEFT] = 0;
								multiplayerselect = c;
							}
						}
					}
				}
				if( keystatus[SDL_SCANCODE_UP] ) {
					keystatus[SDL_SCANCODE_UP] = 0;
					multiplayerselect--;
					if( multiplayerselect<0 )
						multiplayerselect=nummodes-1;
				}
				if( keystatus[SDL_SCANCODE_DOWN] ) {
					keystatus[SDL_SCANCODE_DOWN] = 0;
					multiplayerselect++;
					if( multiplayerselect>nummodes-1 )
						multiplayerselect=0;
				}
			}
		}
	}

	// steam lobby browser
	#ifdef STEAMWORKS
	if ( subwindow && !strcmp(subtext,language[1334]) ) {
		drawDepressed(subx1+8, suby1+24, subx2-32, suby2-64);
		drawDepressed(subx2-32, suby1+24, subx2-8, suby2-64);

		// slider
		slidersize=std::min<int>(((suby2-65)-(suby1+25)),((suby2-65)-(suby1+25)) / ((double)std::max(numSteamLobbies+1,1)/20));
		slidery=std::min(std::max(suby1+25,slidery),suby2-65-slidersize);
		drawWindowFancy(subx2-31,slidery,subx2-9,slidery+slidersize);
					
		// directory list offset from slider
		Sint32 y2 = ((double)(slidery-suby1-20) / ((suby2-52)-(suby1+20)))*(numSteamLobbies+1);
		if ( mousestatus[SDL_BUTTON_LEFT] && omousex >= subx2-32 && omousex < subx2-8 && omousey >= suby1+24 && omousey < suby2-64 ) {
			slidery = oslidery+mousey-omousey;
		} else if ( mousestatus[SDL_BUTTON_WHEELUP] || mousestatus[SDL_BUTTON_WHEELDOWN] ) {
			slidery += 16*mousestatus[SDL_BUTTON_WHEELDOWN] - 16*mousestatus[SDL_BUTTON_WHEELUP];
			mousestatus[SDL_BUTTON_WHEELUP] = 0;
			mousestatus[SDL_BUTTON_WHEELDOWN] = 0;
		} else {
			oslidery = slidery;
		}
		slidery=std::min(std::max(suby1+25,slidery),suby2-65-slidersize);
		y2 = ((double)(slidery-suby1-20) / ((suby2-52)-(suby1+20)))*(numSteamLobbies+1);

		// server flags tooltip variables
		SDL_Rect flagsBox;
		char flagsBoxText[256];
		int hoveringSelection = -1;
					
		// select/inspect lobbies
		if ( omousex >= subx1+8 && omousex < subx2-32 && omousey >= suby1+26 && omousey < suby2-64 ) {
			hoveringSelection = std::min(std::max(0,y2+((omousey-suby1-24)>>4)),MAX_STEAM_LOBBIES);

			// lobby info tooltip
			if ( lobbyIDs[hoveringSelection] ) {
				const char *lobbySvFlagsChar = SteamMatchmaking()->GetLobbyData( *static_cast<CSteamID*>(lobbyIDs[hoveringSelection]), "svFlags" );
				Uint32 lobbySvFlags = atoi(lobbySvFlagsChar);

				int numSvFlags=0, c;
				for ( c=0; c<NUM_SERVER_FLAGS; c++ ) {
					if ( lobbySvFlags&power(2,c) ) {
						numSvFlags++;
					}
				}
				
				flagsBox.x = mousex+8; flagsBox.y = mousey+8;
				flagsBox.w = strlen(language[1335])*12+4; flagsBox.h = 16+12*std::max(2,numSvFlags+1);
				strcpy(flagsBoxText,language[1335]);
				strcat(flagsBoxText,"\n");

				if ( !numSvFlags ) {
					strcat(flagsBoxText,language[1336]);
				} else {
					int y=2;
					for ( c=0; c<NUM_SERVER_FLAGS; c++ ) {
						if ( lobbySvFlags&power(2,c) ) {
							y += 12;
							strcat(flagsBoxText, "\n");
							strcat(flagsBoxText, language[153+c]);
						}
					}
				}
			}

			// selecting lobby
			if ( mousestatus[SDL_BUTTON_LEFT] ) {
				mousestatus[SDL_BUTTON_LEFT] = 0;
				selectedSteamLobby = hoveringSelection;
			}
		}
		selectedSteamLobby = std::min(std::max(y2,selectedSteamLobby),std::min(std::max(numSteamLobbies-1,0),y2+17));
		pos.x = subx1+10; pos.y = suby1+26+(selectedSteamLobby-y2)*16;
		pos.w = subx2-subx1-44; pos.h = 16;
		drawRect(&pos,SDL_MapRGB(mainsurface->format,64,64,64),255);
					
		// print all lobby entries
		Sint32 x=subx1+10; Sint32 y=suby1+28;
		if ( numSteamLobbies>0 ) {
			Sint32 z;
			c=std::min(numSteamLobbies,18+y2);
			for(z=y2;z<c;z++) {
				ttfPrintTextFormatted(ttf12,x,y,lobbyText[z]); // name
				ttfPrintTextFormatted(ttf12,subx2-72,y,"%d/4",lobbyPlayers[z]); // player count
				y+=16;
			}
		} else {
			ttfPrintText(ttf12,x,y,language[1337]);
		}

		// draw server flags tooltip (if applicable)
		if ( hoveringSelection>=0 && numSteamLobbies>0 ) {
			drawTooltip(&flagsBox);
			ttfPrintTextFormatted(ttf12, flagsBox.x+2, flagsBox.y+2, flagsBoxText);
		}
	}
	#endif

	// settings window
	if( settings_window==TRUE ) {
		drawWindowFancy(subx1+16,suby1+44,subx2-16,suby2-32);

		int hovering_selection = -1; //0 to NUM_SERVER_FLAGS used for the game flags settings, e.g. are traps enabled, are cheats enabled, is minotaur enabled, etc.
		SDL_Rect tooltip_box;

		// video tab
		if( settings_tab==0 ) {
			// resolution
			ttfPrintText(ttf12, subx1+24, suby1+60, language[1338]);
			for( c=0; c<NUMRESOLUTIONS; c++ ) {
				if( settings_xres==resolutions[c][0] && settings_yres==resolutions[c][1] ) {
					ttfPrintTextFormatted(ttf12, subx1+32, suby1+84+c*16, "[o] %dx%d",resolutions[c][0],resolutions[c][1]);
				} else {
					ttfPrintTextFormatted(ttf12, subx1+32, suby1+84+c*16, "[ ] %dx%d",resolutions[c][0],resolutions[c][1]);
				}
				if( mousestatus[SDL_BUTTON_LEFT] ) {
					if( omousex >= subx1+38 && omousex < subx1+62 ) {
						if( omousey >= suby1+84+c*16 && omousey < suby1+96+c*16 ) {
							mousestatus[SDL_BUTTON_LEFT] = 0;
							settings_xres = resolutions[c][0];
							settings_yres = resolutions[c][1];
						}
					}
				}
			}
			
			// extra options
			ttfPrintText(ttf12, subx1+224, suby1+60, language[1339]);
			if( settings_smoothlighting )
				ttfPrintTextFormatted(ttf12, subx1+236, suby1+84, "[x] %s", language[1340]);
			else
				ttfPrintTextFormatted(ttf12, subx1+236, suby1+84, "[ ] %s", language[1340]);
			if( settings_fullscreen )
				ttfPrintTextFormatted(ttf12, subx1+236, suby1+108, "[x] %s", language[1341]);
			else
				ttfPrintTextFormatted(ttf12, subx1+236, suby1+108, "[ ] %s", language[1341]);
			if( settings_shaking )
				ttfPrintTextFormatted(ttf12, subx1+236, suby1+132, "[x] %s", language[1342]);
			else
				ttfPrintTextFormatted(ttf12, subx1+236, suby1+132, "[ ] %s", language[1342]);
			if( settings_bobbing )
				ttfPrintTextFormatted(ttf12, subx1+236, suby1+156, "[x] %s", language[1343]);
			else
				ttfPrintTextFormatted(ttf12, subx1+236, suby1+156, "[ ] %s", language[1343]);
			if( settings_spawn_blood )
				ttfPrintTextFormatted(ttf12, subx1+236, suby1+180, "[x] %s", language[1344]);
			else
				ttfPrintTextFormatted(ttf12, subx1+236, suby1+180, "[ ] %s", language[1344]);
			if( settings_colorblind )
				ttfPrintTextFormatted(ttf12, subx1+236, suby1+204, "[x] %s", language[1345]);
			else
				ttfPrintTextFormatted(ttf12, subx1+236, suby1+204, "[ ] %s", language[1345]);
				
			if( mousestatus[SDL_BUTTON_LEFT] ) {
				if( omousex >= subx1+242 && omousex < subx1+266 ) {
					if( omousey >= suby1+84 && omousey < suby1+84+12 ) {
						mousestatus[SDL_BUTTON_LEFT] = 0;
						settings_smoothlighting=(settings_smoothlighting==0);
					}
					else if( omousey >= suby1+108 && omousey < suby1+108+12 ) {
						mousestatus[SDL_BUTTON_LEFT] = 0;
						settings_fullscreen=(settings_fullscreen==0);
					}
					else if( omousey >= suby1+132 && omousey < suby1+132+12 ) {
						mousestatus[SDL_BUTTON_LEFT] = 0;
						settings_shaking=(settings_shaking==0);
					}
					else if( omousey >= suby1+156 && omousey < suby1+156+12 ) {
						mousestatus[SDL_BUTTON_LEFT] = 0;
						settings_bobbing=(settings_bobbing==0);
					}
					else if( omousey >= suby1+180 && omousey < suby1+180+12 ) {
						mousestatus[SDL_BUTTON_LEFT] = 0;
						settings_spawn_blood = (settings_spawn_blood == 0);
					}
					else if( omousey >= suby1+204 && omousey < suby1+204+12 ) {
						mousestatus[SDL_BUTTON_LEFT] = 0;
						settings_colorblind = (settings_colorblind==FALSE);
					}
				}
			}
			
			// fov slider
			ttfPrintText(ttf12, subx1+24, suby2-128, language[1346]);
			doSlider(subx1+24,suby2-104,14,40,100,1,(int *)(&settings_fov));
			
			// gamma slider
			ttfPrintText(ttf12, subx1+24, suby2-80, language[1347]);
			doSliderF(subx1+24,suby2-56,14,0.25,2.f,0.25,&settings_gamma);
		}
		
		// audio tab
		if( settings_tab==1 ) {
			ttfPrintText(ttf12, subx1+24, suby1+60, language[1348]);
			doSlider(subx1+24,suby1+84,15,0,128,0,&settings_sfxvolume);
			ttfPrintText(ttf12, subx1+24, suby1+108, language[1349]);
			doSlider(subx1+24,suby1+132,15,0,128,0,&settings_musvolume);
		}
		
		// keyboard tab
		if( settings_tab==2 ) {
			ttfPrintText(ttf12, subx1+24, suby1+60, language[1350]);

			bool rebindingkey=FALSE;
			if( rebindkey!=-1 )
				rebindingkey=TRUE;

			int c;
			for( c=0; c<NUMIMPULSES; c++ ) {
				if( c<14 )
					ttfPrintText(ttf12, subx1+24, suby1+84+16*c, language[1351+c]);
				else
					ttfPrintText(ttf12, subx1+24, suby1+84+16*c, language[1940+(c-14)]);
				if( mousestatus[SDL_BUTTON_LEFT] && !rebindingkey ) {
					if( omousex>=subx1+24 && omousex<subx2-24 ) {
						if( omousey>=suby1+84+c*16 && omousey<suby1+96+c*16 ) {
							mousestatus[SDL_BUTTON_LEFT] = 0;
							lastkeypressed = 0;
							rebindingkey = TRUE;
							rebindkey = c;
						}
					}
				}
				if( c != rebindkey )
					ttfPrintText(ttf12, subx1+256, suby1+84+c*16, getInputName(settings_impulses[c]));
				else
					ttfPrintText(ttf12, subx1+256, suby1+84+c*16, "...");
			}
			
			if( rebindkey != -1 && lastkeypressed ) {
				if( lastkeypressed == SDL_SCANCODE_ESCAPE ) {
					rebindkey=-1;
				} else {
					settings_impulses[rebindkey] = lastkeypressed;
					if( lastkeypressed==283 )
						mousestatus[SDL_BUTTON_LEFT]=0; // fixes mouse-left not registering bug
					rebindkey=-1;
				}
			}
		}
		
		// mouse tab
		if( settings_tab==3 ) {
			ttfPrintText(ttf12, subx1+24, suby1+60, language[1365]);
			doSliderF(subx1+24,suby1+84,11,0,128,1,&settings_mousespeed);
			
			// checkboxes
			if( settings_reversemouse )
				ttfPrintTextFormatted(ttf12, subx1+24, suby1+108, "[x] %s", language[1366]);
			else
				ttfPrintTextFormatted(ttf12, subx1+24, suby1+108, "[ ] %s", language[1366]);
			if( settings_smoothmouse )
				ttfPrintTextFormatted(ttf12, subx1+24, suby1+132, "[x] %s", language[1367]);
			else
				ttfPrintTextFormatted(ttf12, subx1+24, suby1+132, "[ ] %s", language[1367]);
			if( mousestatus[SDL_BUTTON_LEFT] ) {
				if( omousex>=subx1+30 && omousex<subx1+54 ) {
					if( omousey>=suby1+108 && omousey<suby1+120 ) {
						mousestatus[SDL_BUTTON_LEFT]=0;
						settings_reversemouse=(settings_reversemouse==0);
					}
					if( omousey>=suby1+132 && omousey<suby1+144 ) {
						mousestatus[SDL_BUTTON_LEFT]=0;
						settings_smoothmouse=(settings_smoothmouse==0);
					}
				}
			}
		}

		// miscellaneous options
		if( settings_tab==4 ) {
			ttfPrintText(ttf12, subx1+24, suby1+60, language[1371]);
			if( settings_broadcast )
				ttfPrintTextFormatted(ttf12, subx1+36, suby1+84, "[x] %s", language[1372]);
			else
				ttfPrintTextFormatted(ttf12, subx1+36, suby1+84, "[ ] %s", language[1372]);
			if( settings_nohud )
				ttfPrintTextFormatted(ttf12, subx1+36, suby1+108, "[x] %s", language[1373]);
			else
				ttfPrintTextFormatted(ttf12, subx1+36, suby1+108, "[ ] %s", language[1373]);
			if( settings_auto_hotbar_new_items)
				ttfPrintTextFormatted(ttf12, subx1+36, suby1+132, "[x] %s", language[1374]);
			else
				ttfPrintTextFormatted(ttf12, subx1+36, suby1+132, "[ ] %s", language[1374]);
			if( settings_disable_messages )
				ttfPrintTextFormatted(ttf12, subx1+36, suby1+156, "[x] %s", language[1536]);
			else
				ttfPrintTextFormatted(ttf12, subx1+36, suby1+156, "[ ] %s", language[1536]);

			// server flag elements
			ttfPrintText(ttf12, subx1+24, suby1+180, language[1375]);

			int i;
			for( i=0; i<NUM_SERVER_FLAGS; i++ ) {
				if( svFlags&power(2,i) ) {
					ttfPrintTextFormatted(ttf12,subx1+36,suby1+204+16*i,"[x] %s",language[153+i]);
				} else {
					ttfPrintTextFormatted(ttf12,subx1+36,suby1+204+16*i,"[ ] %s",language[153+i]);
				}
				if (mouseInBounds(subx1 + 36 + 6, subx1 + 36 + 24 + 6, suby1 + 204 + (i*16), suby1 + 216 + (i*16))) //So many gosh dang magic numbers ._.
				{
					if (strlen(language[1942 + i]) > 0) //Don't bother drawing a tooltip if the file doesn't say anything.
					{
						hovering_selection = i;
						tooltip_box.x = omousex + 16; tooltip_box.y = omousey + 8; //I hate magic numbers :|. These should probably be replaced with omousex + mousecursorsprite->width, omousey + mousecursorsprite->height, respectively.
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

			if( mousestatus[SDL_BUTTON_LEFT] ) {
				if( omousex >= subx1+42 && omousex < subx1+66 ) {
					if( omousey >= suby1+84 && omousey < suby1+84+12 ) {
						mousestatus[SDL_BUTTON_LEFT] = 0;
						settings_broadcast=(settings_broadcast==FALSE);
					}
					else if( omousey >= suby1+108 && omousey < suby1+108+12 ) {
						mousestatus[SDL_BUTTON_LEFT] = 0;
						settings_nohud=(settings_nohud==FALSE);
					}
					else if( omousey >= suby1+132 && omousey < suby1+132+12 ) {
						mousestatus[SDL_BUTTON_LEFT] = 0;
						settings_auto_hotbar_new_items = (settings_auto_hotbar_new_items == FALSE);
					}
					else if( omousey >= suby1+156 && omousey < suby1+156+12 ) {
						mousestatus[SDL_BUTTON_LEFT] = 0;
						settings_disable_messages = (settings_disable_messages == FALSE);
					}
				}
				if( multiplayer!=CLIENT ) {
					for( i=0; i<NUM_SERVER_FLAGS; i++ ) {
						if( mouseInBounds(subx1+36+6,subx1+36+24+6,suby1+204+i*16,suby1+216+i*16) ) {
							mousestatus[SDL_BUTTON_LEFT] = 0;

							// toggle flag
							svFlags ^= power(2,i);

							if( multiplayer==SERVER ) {
								// update client flags
								strcpy((char *)net_packet->data,"SVFL");
								SDLNet_Write32(svFlags,&net_packet->data[4]);
								net_packet->len = 8;

								int c;
								for( c=1; c<MAXPLAYERS; c++ ) {
									if( client_disconnected[c] )
										continue;
									net_packet->address.host = net_clients[c-1].host;
									net_packet->address.port = net_clients[c-1].port;
									sendPacketSafe(net_sock, -1, net_packet, c-1);
									messagePlayer(c,language[276]);
								}
								messagePlayer(clientnum,language[276]);
							}
						}
					}
				}
			}
		}
	}

	// connect window
	if( connect_window ) {
		if( connect_window==SERVER ) {
			drawDepressed(subx1+8,suby1+40,subx2-8,suby1+64);
			ttfPrintText(ttf12,subx1+12,suby1+46,portnumber_char);
		
			// enter port number
			if( !SDL_IsTextInputActive() ) {
				SDL_StartTextInput();
				inputstr = portnumber_char;
			}
			//strncpy(portnumber_char,inputstr,5);
			inputlen = 5;
			if( (ticks-cursorflash)%TICKS_PER_SECOND<TICKS_PER_SECOND/2 ) {
				int x;
				TTF_SizeUTF8(ttf12,portnumber_char,&x,NULL);
				ttfPrintText(ttf12,subx1+12+x,suby1+46,"_");
			}
		}
		else if( connect_window==CLIENT ) {
			drawDepressed(subx1+8,suby1+40,subx2-8,suby1+64);
			if( !broadcast ) {
				ttfPrintText(ttf12,subx1 +12,suby1+46,connectaddress);
			} else {
				int i;
				for( i=0; i<strlen(connectaddress); i++ )
					ttfPrintText(ttf12,subx1+12+12*i,suby1+46,"*");
			}
			
			// enter address
			if( !SDL_IsTextInputActive() ) {
				SDL_StartTextInput();
				inputstr = connectaddress;
			}
			//strncpy(connectaddress,inputstr,31);
			inputlen = 31;
			if( (ticks-cursorflash)%TICKS_PER_SECOND<TICKS_PER_SECOND/2 ) {
				int x;
				TTF_SizeUTF8(ttf12,connectaddress,&x,NULL);
				ttfPrintText(ttf12,subx1+12+x,suby1+46,"_");
			}
		}
	}

	// communicating with clients
	if ( multiplayer==SERVER && mode ) {
		//void *newSteamID = NULL; //TODO: Bugger void pointers!
		#ifdef STEAMWORKS
		CSteamID newSteamID;
		#endif

		// hosting the lobby
		int numpacket;
		for ( numpacket=0; numpacket<PACKET_LIMIT; numpacket++ ) {
			if ( directConnect ) {
				if ( !SDLNet_UDP_Recv(net_sock, net_packet) )
					break;
			} else {
				#ifdef STEAMWORKS
				uint32_t packetlen = 0;
				if ( !SteamNetworking()->IsP2PPacketAvailable(&packetlen, 0) )
					break;
				packetlen = std::min<int>(packetlen,NET_PACKET_SIZE-1);
				/*if ( newSteamID ) {
					cpp_Free_CSteamID( newSteamID );
					newSteamID = NULL;
				}*/
				//newSteamID = c_AllocateNew_CSteamID();
				Uint32 bytesRead = 0;
				if ( !SteamNetworking()->ReadP2PPacket(net_packet->data, packetlen, &bytesRead, &newSteamID, 0) )
					continue;
				net_packet->len = packetlen;
				if ( packetlen < sizeof(DWORD) )
					continue; // junk packet, skip //TODO: Investigate the cause of this. During earlier testing, we were getting bombarded with untold numbers of these malformed packets, as if the entire steam network were being routed through this game.

				CSteamID mySteamID = SteamUser()->GetSteamID();
				if ( mySteamID.ConvertToUint64() == newSteamID.ConvertToUint64() ) {
					continue;
				}
				#endif
			}

			if ( handleSafePacket() )
				continue;
			if (!strncmp((char *)net_packet->data,"BARONY_JOIN_REQUEST",19)) {
				#ifdef STEAMWORKS
				if ( !directConnect ) {
					bool skipJoin=FALSE;
					for ( c=0; c<MAXPLAYERS; c++ ) {
						if ( client_disconnected[c] || !steamIDRemote[c] )
							continue;
						if ( newSteamID.ConvertToUint64() == (static_cast<CSteamID* >(steamIDRemote[c]))->ConvertToUint64() ) {
							// we've already accepted this player. NEXT!
							skipJoin=TRUE;
							break;
						}
					}
					if ( skipJoin )
						continue;
				}
				#endif
				if ( strcmp( VERSION, (char *)net_packet->data+54 ) ) {
					c = MAXPLAYERS+1; // wrong version number
				} else {
					Uint32 clientlsg = SDLNet_Read32(&net_packet->data[68]);
					Uint32 clientms = SDLNet_Read32(&net_packet->data[64]);
					if ( net_packet->data[63]==0 ) {
						// client will enter any player spot
						for ( c=0; c<MAXPLAYERS; c++ ) {
							if ( client_disconnected[c]==TRUE )
								break; // no more player slots
						}
					} else {
						// client is joining a particular player spot
						c = net_packet->data[63];
						if ( !client_disconnected[c] )
							c=MAXPLAYERS; // client wants to fill a space that is already filled
					}
					if ( clientlsg != loadingsavegame && loadingsavegame == 0 )
						c = MAXPLAYERS+2; // client shouldn't load save game
					else if ( clientlsg == 0 && loadingsavegame != 0 )
						c = MAXPLAYERS+3; // client is trying to join a save game without a save of their own
					else if ( clientlsg != loadingsavegame )
						c = MAXPLAYERS+4; // client is trying to join the game with an incompatible save
					else if ( loadingsavegame && getSaveGameMapSeed()!=clientms )
						c = MAXPLAYERS+5; // client is trying to join the game with a slightly incompatible save (wrong level)
				}
				if ( c>=MAXPLAYERS ) {
					// on error, client gets a player number that is invalid (to be interpreted as an error code)
					net_clients[MAXPLAYERS-1].host = net_packet->address.host;
					net_clients[MAXPLAYERS-1].port = net_packet->address.port;
					if ( directConnect )
						while((net_tcpclients[MAXPLAYERS-1]=SDLNet_TCP_Accept(net_tcpsock))==NULL);
					net_packet->address.host = net_clients[MAXPLAYERS-1].host;
					net_packet->address.port = net_clients[MAXPLAYERS-1].port;
					net_packet->len = 4;
					SDLNet_Write32(c,&net_packet->data[0]); // error code for client to interpret
					if ( directConnect ) {
						SDLNet_TCP_Send(net_tcpclients[MAXPLAYERS-1],net_packet->data,net_packet->len);
						SDLNet_TCP_Close(net_tcpclients[MAXPLAYERS-1]);
					} else {
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
				} else {
					// on success, client gets legit player number
					strcpy(stats[c].name,(char *)(&net_packet->data[19]));
					client_disconnected[c]=FALSE;
					client_classes[c]=(int)SDLNet_Read32(&net_packet->data[42]);
					stats[c].sex=static_cast<sex_t>((int)SDLNet_Read32(&net_packet->data[46]));
					stats[c].appearance=(int)SDLNet_Read32(&net_packet->data[50]);
					net_clients[c-1].host = net_packet->address.host;
					net_clients[c-1].port = net_packet->address.port;
					if ( directConnect ) {
						while((net_tcpclients[c-1]=SDLNet_TCP_Accept(net_tcpsock))==NULL);
						const char *clientaddr = SDLNet_ResolveIP(&net_packet->address);
						printlog("client %d connected from %s:%d\n",c,clientaddr,net_packet->address.port);
					} else {
						printlog("client %d connected.\n",c);
					}
					client_keepalive[c]=ticks;
						
					// send existing clients info on new client
					for ( x=1; x<MAXPLAYERS; x++ ) {
						if ( client_disconnected[x] || c==x )
							continue;
						strcpy((char *)(&net_packet->data[0]),"NEWPLAYER");
						net_packet->data[9] = c; // clientnum
						net_packet->data[10] = client_classes[c]; // class
						net_packet->data[11] = stats[c].sex; // sex
						strcpy((char *)(&net_packet->data[12]),stats[c].name); // name
						net_packet->address.host = net_clients[x-1].host;
						net_packet->address.port = net_clients[x-1].port;
						net_packet->len = 12+strlen(stats[c].name)+1;
						sendPacketSafe(net_sock, -1, net_packet, x-1);
					}
					char shortname[11] = { 0 };
					strncpy(shortname,stats[c].name,10);

					newString(&lobbyChatboxMessages,0xFFFFFFFF,"\n***   %s has joined the game   ***\n",shortname);

					// send new client their id number + info on other clients
					SDLNet_Write32(c,&net_packet->data[0]);
					for ( x=0; x<MAXPLAYERS; x++ ) {
						net_packet->data[4+x*(3+16)] = client_classes[x]; // class
						net_packet->data[5+x*(3+16)] = stats[x].sex; // sex
						net_packet->data[6+x*(3+16)] = client_disconnected[x]; // connectedness :p
						strcpy((char *)(&net_packet->data[7+x*(3+16)]),stats[x].name); // name
					}
					net_packet->address.host = net_clients[c-1].host;
					net_packet->address.port = net_clients[c-1].port;
					net_packet->len = 4+MAXPLAYERS*(3+16);
					if ( directConnect ) {
						SDLNet_TCP_Send(net_tcpclients[c-1],net_packet->data,net_packet->len);
					} else {
						#ifdef STEAMWORKS
						if ( steamIDRemote[c-1] )
							cpp_Free_CSteamID( steamIDRemote[c-1] );
						steamIDRemote[c-1] = new CSteamID();
						*static_cast<CSteamID *>(steamIDRemote[c-1]) = newSteamID;
						SteamNetworking()->SendP2PPacket(*static_cast<CSteamID* >(steamIDRemote[c-1]), net_packet->data, net_packet->len, k_EP2PSendReliable, 0);
						SDL_Delay(5);
						SteamNetworking()->SendP2PPacket(*static_cast<CSteamID* >(steamIDRemote[c-1]), net_packet->data, net_packet->len, k_EP2PSendReliable, 0);
						SDL_Delay(5);
						SteamNetworking()->SendP2PPacket(*static_cast<CSteamID* >(steamIDRemote[c-1]), net_packet->data, net_packet->len, k_EP2PSendReliable, 0);
						SDL_Delay(5);
						SteamNetworking()->SendP2PPacket(*static_cast<CSteamID* >(steamIDRemote[c-1]), net_packet->data, net_packet->len, k_EP2PSendReliable, 0);
						SDL_Delay(5);
						SteamNetworking()->SendP2PPacket(*static_cast<CSteamID* >(steamIDRemote[c-1]), net_packet->data, net_packet->len, k_EP2PSendReliable, 0);
						SDL_Delay(5);
						#endif
					}
				}
				continue;
			}

			// got a chat message
			else if(!strncmp((char *)net_packet->data,"CMSG",4)) {
				int i;
				for( i=0; i<MAXPLAYERS; i++ ) {
					if( client_disconnected[i] )
						continue;
					net_packet->address.host = net_clients[i-1].host;
					net_packet->address.port = net_clients[i-1].port;
					sendPacketSafe(net_sock, -1, net_packet, i-1);
				}
				newString(&lobbyChatboxMessages,0xFFFFFFFF,(char *)(&net_packet->data[4]));
				playSound(238,64);
				continue;
			}
			
			// player disconnected
			else if(!strncmp((char *)net_packet->data,"PLAYERDISCONNECT",16)) {
				client_disconnected[net_packet->data[16]] = TRUE;
				for( c=1; c<MAXPLAYERS; c++ ) {
					if( client_disconnected[c] )
						continue;
					net_packet->address.host = net_clients[c-1].host;
					net_packet->address.port = net_clients[c-1].port;
					net_packet->len = 17;
					sendPacketSafe(net_sock, -1, net_packet, c-1);
				}
				char shortname[11] = { 0 };
				strncpy(shortname,stats[net_packet->data[16]].name,10);
				newString(&lobbyChatboxMessages,0xFFFFFFFF,language[1376],shortname);
				continue;
			}

			// client requesting new svFlags
			else if(!strncmp((char *)net_packet->data,"SVFL",4)) {
				// update svFlags for everyone
				SDLNet_Write32(svFlags,&net_packet->data[4]);
				net_packet->len = 8;

				int c;
				for( c=1; c<MAXPLAYERS; c++ ) {
					if( client_disconnected[c] )
						continue;
					net_packet->address.host = net_clients[c-1].host;
					net_packet->address.port = net_clients[c-1].port;
					sendPacketSafe(net_sock, -1, net_packet, c-1);
				}
				continue;
			}
				
			// keepalive
			else if(!strncmp((char *)net_packet->data,"KEEPALIVE",9)) {
				client_keepalive[net_packet->data[9]] = ticks;
				continue; // just a keep alive
			}
		}
	}

	// communicating with server
	if ( multiplayer==CLIENT && mode ) {
		if ( receivedclientnum==FALSE ) {
			#ifdef STEAMWORKS
			CSteamID newSteamID;
			#endif

			// trying to connect to the server and get a player number
			// receive the packet:
			bool gotPacket=FALSE;
			if ( directConnect ) {
				if ( SDLNet_TCP_Recv(net_tcpsock, net_packet->data, 4+MAXPLAYERS*(3+16)) ) {
					gotPacket = TRUE;
				}
			} else {
				#ifdef STEAMWORKS
				int numpacket;
				for ( numpacket=0; numpacket<PACKET_LIMIT; numpacket++ ) {
					uint32_t packetlen = 0;
					if ( !SteamNetworking()->IsP2PPacketAvailable(&packetlen, 0) )
						break;
					packetlen = std::min<int>(packetlen,NET_PACKET_SIZE-1);
					Uint32 bytesRead = 0;
					if ( !SteamNetworking()->ReadP2PPacket(net_packet->data, packetlen, &bytesRead, &newSteamID, 0) || bytesRead!=4+MAXPLAYERS*(3+16) )
						continue;
					net_packet->len = packetlen;
					if ( packetlen < sizeof(DWORD) )
						continue;

					CSteamID mySteamID = SteamUser()->GetSteamID();
					if ( mySteamID.ConvertToUint64() == newSteamID.ConvertToUint64() ) {
						continue;
					}
					gotPacket = TRUE;
					break;
				}
				#endif
			}

			// parse the packet:
			if ( gotPacket ) {
				list_FreeAll(&button_l);
				deleteallbuttons=TRUE;
				clientnum=(int)SDLNet_Read32(&net_packet->data[0]);
				if ( clientnum>=MAXPLAYERS || clientnum<=0 ) {
					printlog("connection attempt denied by server.\n");
					multiplayer=SINGLE;
						
					// close current window
					buttonCloseSubwindow(NULL);
					for ( node=button_l.first; node!=NULL; node=nextnode ) {
						nextnode = node->next;
						button = (button_t *)node->element;
						if ( button->focused )
							list_RemoveNode(button->node);
					}

					#ifdef STEAMWORKS
					if ( !directConnect ) {
						if ( currentLobby ) {
							SteamMatchmaking()->LeaveLobby(*static_cast<CSteamID*>(currentLobby));
							cpp_Free_CSteamID( currentLobby ); //TODO: Bugger this.
							currentLobby = NULL;
						}
					}
					#endif

					// create new window
					subwindow = 1;
					subx1 = xres/2-256;
					subx2 = xres/2+256;
					suby1 = yres/2-48;
					suby2 = yres/2+48;
					strcpy(subtext,language[1377]);
					if ( clientnum==MAXPLAYERS )
						strcat(subtext,language[1378]);
					else if ( clientnum==MAXPLAYERS+1 )
						strcat(subtext,language[1379]);
					else if ( clientnum==MAXPLAYERS+2 )
						strcat(subtext,language[1380]);
					else if ( clientnum==MAXPLAYERS+3 )
						strcat(subtext,language[1381]);
					else if ( clientnum==MAXPLAYERS+4 )
						strcat(subtext,language[1382]);
					else if ( clientnum==MAXPLAYERS+5 )
						strcat(subtext,language[1383]);
					else
						strcat(subtext,language[1384]);
					clientnum=0;

					// close button
					button = newButton();
					strcpy(button->label,"x");
					button->x=subx2-20; button->y=suby1;
					button->sizex=20; button->sizey=20;
					button->action=&buttonCloseSubwindow;
					button->visible=1;
					button->focused=1;
					button->key=SDL_SCANCODE_ESCAPE;
						
					// okay button
					button = newButton();
					strcpy(button->label,language[732]);
					button->x=subx2-(subx2-subx1)/2-28; button->y=suby2-28;
					button->sizex=56; button->sizey=20;
					button->action=&buttonCloseSubwindow;
					button->visible=1;
					button->focused=1;
					button->key=SDL_SCANCODE_RETURN;
				} else {
					// join game succeeded, advance to lobby
					client_keepalive[0]=ticks;
					receivedclientnum=TRUE;
					printlog("connected to server.\n");
					client_disconnected[clientnum] = FALSE;
					if ( !loadingsavegame )
						stats[clientnum].appearance=stats[0].appearance;
						
					// now set up everybody else
					for ( c=0; c<MAXPLAYERS; c++ ) {
						client_disconnected[c] = FALSE;
						client_classes[c] = net_packet->data[4+c*(3+16)]; // class
						stats[c].sex = static_cast<sex_t>(net_packet->data[5+c*(3+16)]); // sex
						client_disconnected[c] = net_packet->data[6+c*(3+16)]; // connectedness :p
						strcpy(stats[c].name,(char *)(&net_packet->data[7+c*(3+16)])); // name
					}

					// request svFlags
					strcpy((char *)net_packet->data,"SVFL");
					net_packet->len = 4;
					net_packet->address.host = net_server.host;
					net_packet->address.port = net_server.port;
					sendPacketSafe(net_sock, -1, net_packet, 0);

					// open lobby window
					lobby_window = TRUE;
					subwindow = 1;
					subx1 = xres/2-400;
					subx2 = xres/2+400;
					suby1 = yres/2-300;
					suby2 = yres/2+300;

					if ( directConnect ) {
						strcpy(subtext, language[1385]);
						if ( !broadcast ) {
							strcat(subtext, last_ip);
						} else {
							strcat(subtext, "HIDDEN FOR BROADCAST");
						}
					} else {
						strcpy(subtext, language[1386]);
					}
					strcat(subtext, language[1387]);
		
					// disconnect button
					button = newButton();
					strcpy(button->label,language[1311]);
					button->sizex=strlen(language[1311])*12+8; button->sizey=20;
					button->x=subx1+4; button->y=suby2-24;
					button->action=&buttonDisconnect;
					button->visible=1;
					button->focused=1;
				}
			}
		} else if ( multiplayer==CLIENT ) {
			#ifdef STEAMWORKS
			CSteamID newSteamID;
			#endif
			int numpacket;
			for ( numpacket=0; numpacket<PACKET_LIMIT; numpacket++ ) {
				if ( directConnect ) {
					if ( !SDLNet_UDP_Recv(net_sock, net_packet) ) {
						break;
					}
				} else {
					#ifdef STEAMWORKS
					uint32_t packetlen = 0;
					if ( !SteamNetworking()->IsP2PPacketAvailable(&packetlen, 0) )
						break;
					packetlen = std::min<int>(packetlen,NET_PACKET_SIZE-1);
					Uint32 bytesRead = 0;
					if ( !SteamNetworking()->ReadP2PPacket(net_packet->data, packetlen, &bytesRead, &newSteamID, 0) )
						continue;
					net_packet->len = packetlen;
					if ( packetlen < sizeof(DWORD) )
						continue; //TODO: Again, figure out why this is happening.

					CSteamID mySteamID = SteamUser()->GetSteamID();
					if (mySteamID.ConvertToUint64() == newSteamID.ConvertToUint64()) {
						continue;
					}
					#endif
				}

				if ( handleSafePacket() )
					continue;

				// game start
				if (!strncmp((char *)net_packet->data,"BARONY_GAME_START",17)) {
					svFlags = SDLNet_Read32(&net_packet->data[17]);
					uniqueGameKey = SDLNet_Read32(&net_packet->data[21]);
					buttonCloseSubwindow(NULL);
					numplayers=MAXPLAYERS;
					introstage=3;
					fadeout=TRUE;
					continue;
				}
			
				// new player
				else if (!strncmp((char *)net_packet->data,"NEWPLAYER",9)) {
					client_disconnected[net_packet->data[9]] = FALSE;
					client_classes[net_packet->data[9]] = net_packet->data[10];
					stats[net_packet->data[9]].sex = static_cast<sex_t>(net_packet->data[11]);
					strcpy(stats[net_packet->data[9]].name,(char *)(&net_packet->data[12]));

					char shortname[11] = { 0 };
					strncpy(shortname,stats[net_packet->data[9]].name,10);
					newString(&lobbyChatboxMessages,0xFFFFFFFF,language[1388],shortname);
					continue;
				}
			
				// player disconnect
				else if (!strncmp((char *)net_packet->data,"PLAYERDISCONNECT",16)) {
					client_disconnected[net_packet->data[16]] = TRUE;
					if ( net_packet->data[16]==0 ) {
						// close lobby window
						buttonCloseSubwindow(NULL);
						for ( node=button_l.first; node!=NULL; node=nextnode ) {
							nextnode = node->next;
							button = (button_t *)node->element;
							if ( button->focused )
								list_RemoveNode(button->node);
						}
				
						// create new window
						subwindow = 1;
						subx1 = xres/2-256;
						subx2 = xres/2+256;
						suby1 = yres/2-40;
						suby2 = yres/2+40;
						strcpy(subtext,language[1126]);
				
						// close button
						button = newButton();
						strcpy(button->label,"x");
						button->x=subx2-20; button->y=suby1;
						button->sizex=20; button->sizey=20;
						button->action=&buttonCloseSubwindow;
						button->visible=1;
						button->focused=1;
				
						// okay button
						button = newButton();
						strcpy(button->label,language[732]);
						button->x=subx2-(subx2-subx1)/2-20; button->y=suby2-24;
						button->sizex=56; button->sizey=20;
						button->action=&buttonCloseSubwindow;
						button->visible=1;
						button->focused=1;
				
						// reset multiplayer status
						multiplayer = SINGLE;
						stats[0].sex = stats[clientnum].sex;
						client_classes[0] = client_classes[clientnum];
						strcpy(stats[0].name,stats[clientnum].name);
						clientnum = 0;
						client_disconnected[0]=FALSE;
						for ( c=1; c<MAXPLAYERS; c++ ) {
							client_disconnected[c]=TRUE;
						}

						// close any existing net interfaces
						closeNetworkInterfaces();

						#ifdef STEAMWORKS
						if ( !directConnect ) {
							if ( currentLobby ) {
								SteamMatchmaking()->LeaveLobby(*static_cast<CSteamID*>(currentLobby));
								cpp_Free_CSteamID(currentLobby); //TODO: Bugger this.
								currentLobby = NULL;
							}
						}
						#endif
					} else {
						char shortname[11] = { 0 };
						strncpy(shortname,stats[net_packet->data[16]].name,10);
						newString(&lobbyChatboxMessages,0xFFFFFFFF,language[1376],shortname);
					}
					continue;
				}

				// got a chat message
				else if (!strncmp((char *)net_packet->data,"CMSG",4)) {
					newString(&lobbyChatboxMessages,0xFFFFFFFF,(char *)(&net_packet->data[4]));
					playSound(238,64);
					continue;
				}

				// update svFlags
				else if (!strncmp((char *)net_packet->data,"SVFL",4)) {
					svFlags = SDLNet_Read32(&net_packet->data[4]);
					continue;
				}
					
				// keepalive
				else if (!strncmp((char *)net_packet->data,"KEEPALIVE",9)) {
					client_keepalive[0] = ticks;
					continue; // just a keep alive
				}
			}
		}
	}
	if( multiplayer==SINGLE ) {
		receivedclientnum = FALSE;
	}
	
	// lobby window
	if( lobby_window ) {

		int hovering_selection = -1; //0 to NUM_SERVER_FLAGS used for the server flags settings, e.g. are traps, cheats, minotaur, etc enabled.
		SDL_Rect tooltip_box;

		// player info text
		for( c=0; c<MAXPLAYERS; c++ ) {
			if( client_disconnected[c] )
				continue;
			if( stats[c].sex )
				ttfPrintTextFormatted(ttf12, subx1+8, suby1 + 80+60*c,"%d:  %s\n    %s\n    %s",c+1,stats[c].name,language[1322],language[1900+client_classes[c]]);
			else
				ttfPrintTextFormatted(ttf12, subx1+8, suby1 + 80+60*c,"%d:  %s\n    %s\n    %s",c+1,stats[c].name,language[1321],language[1900+client_classes[c]]);
		}

		// select gui element w/ mouse
		if( mousestatus[SDL_BUTTON_LEFT] ) {
			if( mouseInBounds(subx1+16,subx2-16,suby2-48,suby2-32) ) {
				mousestatus[SDL_BUTTON_LEFT] = 0;

				// chatbox
				inputstr = lobbyChatbox;
				inputlen = LOBBY_CHATBOX_LENGTH-1;
				cursorflash = ticks;
			}
			else if( mouseInBounds(xres/2,subx2-32,suby1+56,suby1+68) && multiplayer==SERVER ) {
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
			if ( multiplayer==SERVER ) {
				for ( i=0; i<NUM_SERVER_FLAGS; i++ ) {
					if ( mouseInBounds(xres/2+8+6,xres/2+8+30,suby1+80+i*16,suby1+92+i*16) ) {
						mousestatus[SDL_BUTTON_LEFT] = 0;

						// toggle flag
						svFlags ^= power(2,i);

						// update client flags
						strcpy((char *)net_packet->data,"SVFL");
						SDLNet_Write32(svFlags,&net_packet->data[4]);
						net_packet->len = 8;

						int c;
						for ( c=1; c<MAXPLAYERS; c++ ) {
							if ( client_disconnected[c] )
								continue;
							net_packet->address.host = net_clients[c-1].host;
							net_packet->address.port = net_clients[c-1].port;
							sendPacketSafe(net_sock, -1, net_packet, c-1);
						}

						// update lobby data
						#ifdef STEAMWORKS
						if ( !directConnect ) {
							char svFlagsChar[16];
							snprintf(svFlagsChar,15,"%d",svFlags);
							SteamMatchmaking()->SetLobbyData(*static_cast<CSteamID*>(currentLobby), "svFlags", svFlagsChar);
						}
						#endif
					}
				}
			}

			// switch lobby type
			#ifdef STEAMWORKS
			if ( !directConnect ) {
				if ( multiplayer==SERVER ) {
					for ( i=0; i<3; i++ ) {
						if ( mouseInBounds(xres/2+8+6,xres/2+8+30,suby1+256+i*16,suby1+268+i*16) ) {
							mousestatus[SDL_BUTTON_LEFT] = 0;
							switch( i ) {
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
		if( keystatus[SDL_SCANCODE_TAB] ) {
			keystatus[SDL_SCANCODE_TAB] = 0;
			#ifdef STEAMWORKS
			if( inputstr==currentLobbyName ) {
				inputstr = lobbyChatbox;
				inputlen = LOBBY_CHATBOX_LENGTH-1;
			} else {
				inputstr = currentLobbyName;
				inputlen = 31;
			}
			#endif
		}

		// server flag elements
		int i;
		for( i=0; i<NUM_SERVER_FLAGS; i++ ) {
			if( svFlags&power(2,i) ) {
				ttfPrintTextFormatted(ttf12,xres/2+8,suby1+80+16*i,"[x] %s",language[153+i]);
			} else {
				ttfPrintTextFormatted(ttf12,xres/2+8,suby1+80+16*i,"[ ] %s",language[153+i]);
			}
			if (mouseInBounds((xres / 2) + 8 + 6, (xres / 2) + 8 + 30, suby1 + 80 + (i*16), suby1 + 92 + (i*16))) //So many gosh dang magic numbers ._.
			{
				if (strlen(language[1942 + i]) > 0) //Don't bother drawing a tooltip if the file doesn't say anything.
				{
					hovering_selection = i;
					tooltip_box.x = mousex + 16; tooltip_box.y = mousey + 8;
					tooltip_box.w = strlen(language[1942 + i]) * TTF12_WIDTH + 8; //MORE MAGIC NUMBERS. HNNGH. I can guess what they all do, but dang.
					tooltip_box.h = TTF12_HEIGHT + 8;
				}
			}
		}

		// lobby type elements
		#ifdef STEAMWORKS
		if ( !directConnect ) {
			if ( multiplayer==SERVER ) {
				for ( i=0; i<3; i++ ) {
					if ( currentLobbyType == static_cast<ELobbyType>(i) ) {
						ttfPrintTextFormatted(ttf12,xres/2+8,suby1+256+16*i,"[o] %s",language[250+i]);
					} else {
						ttfPrintTextFormatted(ttf12,xres/2+8,suby1+256+16*i,"[ ] %s",language[250+i]);
					}
				}
			}
		}
		#endif
		
		#ifdef STEAMWORKS
		if ( !directConnect ) {
			// server name
			drawDepressed(xres/2, suby1+56, xres/2+388, suby1+72);
			ttfPrintTextFormatted(ttf12, xres/2+2, suby1+58, "%s", currentLobbyName);
			if ( inputstr==currentLobbyName ) {
				if ( (ticks-cursorflash)%TICKS_PER_SECOND<TICKS_PER_SECOND/2 ) {
					int x;
					TTF_SizeUTF8(ttf12,currentLobbyName,&x,NULL);
					ttfPrintTextFormatted(ttf12,xres/2+2+x,suby1+58,"_");
				}
			}

			// update server name
			if ( currentLobby ) {
				const char *lobbyName = SteamMatchmaking()->GetLobbyData( *static_cast<CSteamID*>(currentLobby), "name");
				if ( lobbyName ) {
					if ( strcmp(lobbyName,currentLobbyName) ) {
						if ( multiplayer==CLIENT ) {
							// update the lobby name on our end
							snprintf( currentLobbyName, 31, "%s", lobbyName );
						} else if ( multiplayer==SERVER ) {
							// update the backend's copy of the lobby name
							SteamMatchmaking()->SetLobbyData(*static_cast<CSteamID*>(currentLobby), "name", currentLobbyName);
						}
					}
				}
			}
		}
		#endif

		// chatbox gui elements
		drawDepressed(subx1+16, suby2-256, subx2-16, suby2-48);
		drawDepressed(subx1+16, suby2-48, subx2-16, suby2-32);

		// draw chatbox main text
		int y=suby2-50;
		for( c=0; c<20; c++ ) {
			node_t *node = list_Node(&lobbyChatboxMessages, list_Size(&lobbyChatboxMessages)-c-1);
			if( node ) {
				string_t *str = (string_t *)node->element;
				y -= str->lines*12;
				if( y<suby2-254 ) // there were some tall messages and we're out of space
					break;
				ttfPrintTextFormatted(ttf12, subx1+18, y, str->data);
			} else {
				break;
			}
		}
		while( list_Size(&lobbyChatboxMessages)>20 ) {
			// if there are too many messages to fit the chatbox, just cull them
			list_RemoveNode(lobbyChatboxMessages.first);
		}

		// handle chatbox text entry
		if( !SDL_IsTextInputActive() ) {
			// this is the default text entry box in this window.
			inputstr = lobbyChatbox;
			inputlen = LOBBY_CHATBOX_LENGTH-1;
			SDL_StartTextInput();
		}
		if( keystatus[SDL_SCANCODE_RETURN] && strlen(lobbyChatbox)>0 ) {
			keystatus[SDL_SCANCODE_RETURN] = 0;
			if( multiplayer!=CLIENT )
				playSound(238,64);

			char shortname[11] = {0};
			strncpy(shortname,stats[clientnum].name,10);

			char msg[LOBBY_CHATBOX_LENGTH+32] = { 0 };
			snprintf(msg,LOBBY_CHATBOX_LENGTH,"%s: %s",shortname,lobbyChatbox);
			if( strlen(lobbyChatbox)>LOBBY_CHATBOX_LENGTH-strlen(shortname)-2 ) {
				msg[strlen(msg)] = '\n';
				int i;
				for( i=0; i<strlen(shortname)+2; i++ )
					snprintf((char *)(msg+strlen(msg)),(LOBBY_CHATBOX_LENGTH+31)-strlen(msg)," ");
				snprintf((char *)(msg+strlen(msg)),(LOBBY_CHATBOX_LENGTH+31)-strlen(msg),"%s",(char *)(lobbyChatbox+LOBBY_CHATBOX_LENGTH-strlen(shortname)-2));
			}
			if( multiplayer!=CLIENT )
				newString(&lobbyChatboxMessages,0xFFFFFFFF,msg); // servers print their messages right away
			strcpy(lobbyChatbox,"");

			// send the message
			strcpy((char *)net_packet->data,"CMSG");
			strcat((char *)(net_packet->data),msg);
			net_packet->len = 4+strlen(msg)+1;
			net_packet->data[net_packet->len-1] = 0;
			if( multiplayer==CLIENT ) {
				net_packet->address.host = net_server.host;
				net_packet->address.port = net_server.port;
				sendPacketSafe(net_sock, -1, net_packet, 0);
			} else if( multiplayer==SERVER ) {
				int i;
				for( i=1; i<MAXPLAYERS; i++ ) {
					if( client_disconnected[i] )
						continue;
					net_packet->address.host = net_clients[i-1].host;
					net_packet->address.port = net_clients[i-1].port;
					sendPacketSafe(net_sock, -1, net_packet, i-1);
				}
			}
		}
		
		// draw chatbox entry text and cursor
		ttfPrintTextFormatted(ttf12, subx1+18, suby2-46, ">%s", lobbyChatbox);
		if( inputstr==lobbyChatbox ) {
			if( (ticks-cursorflash)%TICKS_PER_SECOND<TICKS_PER_SECOND/2 ) {
				int x;
				TTF_SizeUTF8(ttf12,lobbyChatbox,&x,NULL);
				ttfPrintTextFormatted(ttf12,subx1+18+x+TTF12_WIDTH,suby2-46,"_");
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
		if( multiplayer==SERVER ) {
			int i;
			for( i=1; i<MAXPLAYERS; i++ ) {
				if( client_disconnected[i] )
					continue;
				if( ticks-client_keepalive[i] > TICKS_PER_SECOND*30 ) {
					client_disconnected[i] = TRUE;
					strncpy((char *)(net_packet->data),"PLAYERDISCONNECT",16);
					net_packet->data[16] = i;
					net_packet->len = 17;
					for( c=1; c<MAXPLAYERS; c++ ) {
						if( client_disconnected[c] )
							continue;
						net_packet->address.host = net_clients[c-1].host;
						net_packet->address.port = net_clients[c-1].port;
						sendPacketSafe(net_sock, -1, net_packet, c-1);
					}
					char shortname[11] = { 0 };
					strncpy(shortname,stats[i].name,10);
					newString(&lobbyChatboxMessages,0xFFFFFFFF,language[1376],shortname);
					continue;
				}
			}
		} else if( multiplayer==CLIENT ) {
			if( ticks-client_keepalive[0] > TICKS_PER_SECOND*30 ) {
				buttonDisconnect(NULL);
				openFailedConnectionWindow(3); // lost connection to server box
			}
		}
		
		// send keepalive messages every second
		if( ticks%(TICKS_PER_SECOND*1)==0 && multiplayer!=SINGLE ) {
			strcpy((char *)net_packet->data,"KEEPALIVE");
			net_packet->data[9] = clientnum;
			net_packet->len = 10;
			if( multiplayer==CLIENT ) {
				net_packet->address.host = net_server.host;
				net_packet->address.port = net_server.port;
				sendPacketSafe(net_sock, -1, net_packet, 0);
			} else if( multiplayer==SERVER ) {
				int i;
				for( i=1; i<MAXPLAYERS; i++ ) {
					if( client_disconnected[i] )
						continue;
					net_packet->address.host = net_clients[i-1].host;
					net_packet->address.port = net_clients[i-1].port;
					sendPacketSafe(net_sock, -1, net_packet, i-1);
				}
			}
		}
	}

	// statistics window
	if( score_window ) {
		if( !list_Size(&topscores) ) {
			#define NOSCORESSTR language[1389]
			ttfPrintTextFormatted(ttf16, xres/2-strlen(NOSCORESSTR)*9, yres/2-9, NOSCORESSTR);
		} else {
			ttfPrintTextFormatted(ttf16, subx1+8, suby1+8, "%s - #%d",language[1390],score_window);

			// draw character window
			if( players[clientnum] != NULL ) {
				camera_charsheet.x=players[clientnum]->x/16.0+1;
				camera_charsheet.y=players[clientnum]->y/16.0-.5;
				camera_charsheet.z=players[clientnum]->z*2;
				camera_charsheet.ang=atan2(players[clientnum]->y/16.0-camera_charsheet.y,players[clientnum]->x/16.0-camera_charsheet.x);
				camera_charsheet.vang=PI/24;
				camera_charsheet.winw=400;
				camera_charsheet.winy=suby1+32;
				camera_charsheet.winh=suby2-96-camera_charsheet.winy;
				camera_charsheet.winx=subx1+32;
				pos.x = camera_charsheet.winx; pos.y = camera_charsheet.winy;
				pos.w = camera_charsheet.winw; pos.h = camera_charsheet.winh;
				drawRect(&pos,0,255);
				b=players[clientnum]->flags[BRIGHT];
				players[clientnum]->flags[BRIGHT]=TRUE;
				if( !players[clientnum]->flags[INVISIBLE] ) {
					double ofov = fov;
					fov = 50;
					glDrawVoxel(&camera_charsheet,players[clientnum],REALCOLORS);
					fov = ofov;
				}
				players[clientnum]->flags[BRIGHT]=b;
				c=0;
				for( node=players[clientnum]->children.first; node!=NULL; node=node->next ) {
					if( c==0 ) {
						c++;
					}
					entity = (Entity *) node->element;
					if( !entity->flags[INVISIBLE] ) {
						b=entity->flags[BRIGHT];
						entity->flags[BRIGHT]=TRUE;
						double ofov = fov;
						fov = 50;
						glDrawVoxel(&camera_charsheet,entity,REALCOLORS);
						fov = ofov;
						entity->flags[BRIGHT]=b;
					}
					c++;
				}
			}

			// print name and class
			if( victory ) {
				ttfPrintTextFormatted(ttf16,subx1+448,suby1+40,language[1391]);
				ttfPrintTextFormatted(ttf16,subx1+448,suby1+56,"%s",stats[clientnum].name);
				if( victory==1 )
					ttfPrintTextFormatted(ttf16,subx1+448,suby1+72,language[1392]);
				else if( victory==2 )
					ttfPrintTextFormatted(ttf16,subx1+448,suby1+72,language[1393]);
			} else {
				ttfPrintTextFormatted(ttf16,subx1+448,suby1+40,language[1394]);
				ttfPrintTextFormatted(ttf16,subx1+448,suby1+56,"%s",stats[clientnum].name);

				char classname[32];
				strcpy(classname,language[1900+client_classes[0]]);
				classname[0]-=32;
				ttfPrintTextFormatted(ttf16,subx1+448,suby1+72,language[1395],classname);
			}

			// print total score
			node = list_Node(&topscores,score_window-1);
			if( node ) {
				score_t *score = (score_t *)node->element;
				ttfPrintTextFormatted(ttf16,subx1+448,suby1+104,language[1404],totalScore(score));
			}

			// print character stats
			ttfPrintTextFormatted(ttf12,subx1+456,suby1+128,language[359],stats[clientnum].LVL,language[1900+client_classes[clientnum]]);
			ttfPrintTextFormatted(ttf12,subx1+456,suby1+140,language[1396],stats[clientnum].EXP);
			ttfPrintTextFormatted(ttf12,subx1+456,suby1+152,language[1397],stats[clientnum].GOLD);
			ttfPrintTextFormatted(ttf12,subx1+456,suby1+164,language[361],currentlevel);

			ttfPrintTextFormatted(ttf12,subx1+456,suby1+188,language[1398],statGetSTR(&stats[clientnum]),stats[clientnum].STR);
			ttfPrintTextFormatted(ttf12,subx1+456,suby1+200,language[1399],statGetDEX(&stats[clientnum]),stats[clientnum].DEX);
			ttfPrintTextFormatted(ttf12,subx1+456,suby1+212,language[1400],statGetCON(&stats[clientnum]),stats[clientnum].CON);
			ttfPrintTextFormatted(ttf12,subx1+456,suby1+224,language[1401],statGetINT(&stats[clientnum]),stats[clientnum].INT);
			ttfPrintTextFormatted(ttf12,subx1+456,suby1+236,language[1402],statGetPER(&stats[clientnum]),stats[clientnum].PER);
			ttfPrintTextFormatted(ttf12,subx1+456,suby1+248,language[1403],statGetCHR(&stats[clientnum]),stats[clientnum].CHR);

			// time
			Uint32 sec = (completionTime/TICKS_PER_SECOND)%60;
			Uint32 min = ((completionTime/TICKS_PER_SECOND)/60)%60;
			Uint32 hour = ((completionTime/TICKS_PER_SECOND)/60)/60;
			ttfPrintTextFormatted(ttf12,subx1+32,suby2-80,"%s: %02d:%02d:%02d. %s:",language[1405],hour,min,sec,language[1406]);
			if( !conductPenniless && !conductFoodless && !conductVegetarian && !conductIlliterate ) {
				ttfPrintText(ttf12,subx1+32,suby2-64,language[1407]);
			} else {
				int b = 0;
				strcpy(tempstr," ");
				if( conductPenniless ) {
					strcat(tempstr,language[1408]);
					b++;
				}
				if( conductFoodless ) {
					strcat(tempstr,language[1409]);
					b++;
				}
				if( b==2 )
					strcat(tempstr,"\n ");
				if( conductVegetarian ) {
					strcat(tempstr,language[1410]);
					b++;
				}
				if( b==2 )
					strcat(tempstr,"\n ");
				if( conductIlliterate ) {
					strcat(tempstr,language[1411]);
					b++;
				}
				if( b==2 )
					strcat(tempstr,"\n ");
				if( b!=2 )
					tempstr[strlen(tempstr)-2] = 0;
				else
					tempstr[strlen(tempstr)-4] = 0;
				ttfPrintTextFormatted(ttf12,subx1+20,suby2-64,tempstr);
			}

			// kills
			int x=0, y=0;
			ttfPrintText(ttf12,subx1+456,suby1+272,language[1412]);
			bool nokills=TRUE;
			for( x=0; x<NUMMONSTERS; x++ ) {
				if( kills[x] ) {
					nokills=FALSE;
					if( kills[x] > 1 )
						ttfPrintTextFormatted(ttf12,subx1+456+(y/10)*180,suby1+296+(y%10)*12,"%d %s",kills[x],language[111+x]);
					else
						ttfPrintTextFormatted(ttf12,subx1+456+(y/10)*180,suby1+296+(y%10)*12,"%d %s",kills[x],language[90+x]);
					y++;
				}
			}
			if( nokills ) {
				ttfPrintText(ttf12,subx1+456,suby1+296,language[1413]);
			}
		}
	}
	
	// handle fade actions
	if( fadefinished ) {
		if( introstage==2 ) { // quit game
			introstage=0;
			mainloop = 0;
		}
		else if( introstage==3 ) { // new game
			introstage=1;
			fadefinished=FALSE;
			fadeout=FALSE;
			gamePaused=FALSE;
			multiplayerselect=0;
			intro = TRUE; //Fix items auto-adding to the hotbar on game restart.

			if( !mode ) {
				// restarting game, make a highscore
				saveScore();
				deleteSaveGame();
				loadingsavegame=0;
			}

			// undo shopkeeper grudge
			swornenemies[SHOPKEEPER][HUMAN] = FALSE;
			monsterally[SHOPKEEPER][HUMAN] = TRUE;

			// setup game
			entity_uids=1;
			loading=TRUE;
			darkmap=FALSE;
			selected_spell=NULL;
			shootmode = TRUE;
			currentlevel = 0;
			secretlevel = FALSE;
			victory = 0;
			completionTime = 0;
			conductPenniless = TRUE;
			conductFoodless = TRUE;
			conductVegetarian = TRUE;
			conductIlliterate = TRUE;
			list_FreeAll(&damageIndicators);
			for( c=0; c<NUMMONSTERS; c++ ) {
				kills[c]=0;
			}

			// disable cheats
			noclip = FALSE;
			godmode = FALSE;
			buddhamode = FALSE;
			everybodyfriendly = FALSE;

			#ifdef STEAMWORKS
			if ( !directConnect ) {
				if ( currentLobby ) {
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
			if( multiplayer != CLIENT ) {
				// stop all sounds
				if( sound_group )
					FMOD_ChannelGroup_Stop(sound_group);

				// generate a unique game key (used to identify compatible save games)
				prng_seed_time();
				if( multiplayer==SINGLE ) {
					uniqueGameKey = prng_get_uint();
					if( !uniqueGameKey )
						uniqueGameKey++;
				}

				// reset class loadout
				if( !loadingsavegame ) {
					clearStats(&stats[0]);
					initClass(0);
				} else {
					loadGame(0);
				}

				// hack to fix these things from breaking everything...
				hudarm = NULL;
				hudweapon = NULL;
				magicLeftHand = NULL;
				magicRightHand = NULL;
				
				for( node=map.entities->first; node!=NULL; node=node->next ) {
					entity = (Entity *)node->element;
					entity->flags[NOUPDATE] = TRUE;
				}
				mapseed = 0;
				lastEntityUIDs=entity_uids;
				numplayers=0;
				if( loadingmap==FALSE ) {
					if( !secretlevel )
						fp = fopen(LEVELSFILE,"r");
					else
						fp = fopen(SECRETLEVELSFILE,"r");
					int i;
					for( i=0; i<currentlevel; i++ )
						while( fgetc(fp) != '\n' ) if( feof(fp) ) break;
					fscanf(fp,"%s",tempstr); while( fgetc(fp) != ' ' ) if( feof(fp) ) break;
					if( !strcmp(tempstr,"gen:") ) {
						fscanf(fp,"%s",tempstr); while( fgetc(fp) != '\n' ) if( feof(fp) ) break;
						generateDungeon(tempstr,mapseed);
					} else if( !strcmp(tempstr,"map:") ) {
						fscanf(fp,"%s",tempstr); while( fgetc(fp) != '\n' ) if( feof(fp) ) break;
						loadMap(tempstr,&map,map.entities);
					}
					fclose(fp);
				} else {
					if( genmap==FALSE ) {
						loadMap(maptoload,&map,map.entities);
					} else {
						generateDungeon(maptoload,mapseed);
					}
				}
				assignActions(&map);
				generatePathMaps();

				if( loadingsavegame ) {
					loadingsavegame = 0;

					list_t *followers = loadGameFollowers();
					if( followers ) {
						int c;
						for( c=0; c<MAXPLAYERS; c++ ) {
							node_t *tempNode = list_Node(followers,c);
							if( tempNode ) {
								list_t *tempFollowers = (list_t *)tempNode->element;
								if( players[c] && !client_disconnected[c] ) {
									node_t *node;
									for( node=tempFollowers->first; node!=NULL; node=node->next ) {
										stat_t *tempStats = (stat_t *)node->element;
										Entity *monster = summonMonster(tempStats->type,players[c]->x,players[c]->y);
										if( monster ) {
											monster->skill[3] = 1; // to mark this monster partially initialized
											list_RemoveNode(monster->children.last);

											node_t *newNode = list_AddNodeLast(&monster->children);
											newNode->element = copyStats(tempStats);
											newNode->deconstructor = &statDeconstructor;
											newNode->size = sizeof(stat_t);

											stat_t *monsterStats = (stat_t *)newNode->element;
											monsterStats->leader_uid = players[c]->uid;
											if( !monsterally[HUMAN][monsterStats->type] )
												monster->flags[USERFLAG2]=TRUE;

											newNode = list_AddNodeLast(&stats[c].FOLLOWERS);
											newNode->deconstructor = &defaultDeconstructor;
											Uint32 *myuid = (Uint32 *) malloc(sizeof(Uint32));
											newNode->element = myuid;
											*myuid = monster->uid;
											
											if( c>0 && multiplayer==SERVER ) {
												strcpy((char *)net_packet->data,"LEAD");
												SDLNet_Write32((Uint32)monster->uid,&net_packet->data[4]);
												net_packet->address.host = net_clients[c-1].host;
												net_packet->address.port = net_clients[c-1].port;
												net_packet->len = 8;
												sendPacketSafe(net_sock, -1, net_packet, c-1);
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

				if( multiplayer==SINGLE )
					saveGame();
			} else {
				// hack to fix these things from breaking everything...
				hudarm = NULL;
				hudweapon = NULL;
				magicLeftHand = NULL;
				magicRightHand = NULL;

				client_disconnected[0] = FALSE;
			
				// initialize class
				if( !loadingsavegame ) {
					clearStats(&stats[clientnum]);
					initClass(clientnum);
				} else {
					loadGame(clientnum);
				}

				// stop all sounds
				if( sound_group )
					FMOD_ChannelGroup_Stop(sound_group);
				// load next level
				mapseed = 0;
				entity_uids=1;
				lastEntityUIDs=entity_uids;
				numplayers=0;
				if( loadingmap==FALSE ) {
					if( !secretlevel )
						fp = fopen(LEVELSFILE,"r");
					else
						fp = fopen(SECRETLEVELSFILE,"r");
					int i;
					for( i=0; i<currentlevel; i++ )
						while( fgetc(fp) != '\n' ) if( feof(fp) ) break;
					fscanf(fp,"%s",tempstr); while( fgetc(fp) != ' ' ) if( feof(fp) ) break;
					if( !strcmp(tempstr,"gen:") ) {
						fscanf(fp,"%s",tempstr); while( fgetc(fp) != '\n' ) if( feof(fp) ) break;
						generateDungeon(tempstr,mapseed);
					} else if( !strcmp(tempstr,"map:") ) {
						fscanf(fp,"%s",tempstr); while( fgetc(fp) != '\n' ) if( feof(fp) ) break;
						loadMap(tempstr,&map,map.entities);
					}
					fclose(fp);
				} else {
					if( genmap==FALSE ) {
						loadMap(maptoload,&map,map.entities);
					} else {
						generateDungeon(maptoload,rand());
					}
				}
				assignActions(&map);
				generatePathMaps();
				for( node=map.entities->first; node!=NULL; node=nextnode ) {
					nextnode = node->next;
					Entity *entity = (Entity *)node->element;
					if( entity->flags[NOUPDATE] )
						list_RemoveNode(entity->mynode); // we're anticipating this entity data from server
				}
					
				printlog("Done.\n");
			}

			// spice of life achievement
			usedClass[client_classes[clientnum]] = TRUE;
			bool usedAllClasses = TRUE;
			for( c=0; c<10; c++ )
			if( !usedClass[c] )
				usedAllClasses = FALSE;
			if( usedAllClasses )
				steamAchievement("BARONY_ACH_SPICE_OF_LIFE");

			// delete game data clutter
			list_FreeAll(&messages);
			list_FreeAll(&command_history);
			list_FreeAll(&safePacketsSent);
			for( c=0; c<MAXPLAYERS; c++ )
				list_FreeAll(&safePacketsReceived[c]);
			deleteAllNotificationMessages();
			for(c=0;c<MAXPLAYERS;c++) {
				list_FreeAll(&stats[c].FOLLOWERS);
			}
			list_FreeAll(&removedEntities);
			list_FreeAll(&chestInv);
		
			// make some messages
			startMessages();
		
			// kick off the main loop!
			pauseGame(1,0);
			loading=FALSE;
			intro=FALSE;
		} else if( introstage==4 ) { // credits
			fadefinished=FALSE;
			fadeout=FALSE;
			creditstage++;
			if( creditstage>=14 ) {
				playmusic(intromusic, TRUE, FALSE, FALSE);
				introstage=1;
				credittime=0;
				creditstage=0;
				movie=FALSE;
			} else {
				credittime=0;
				movie=TRUE;
			}
		} else if( introstage==5 ) { // end game
			// in greater numbers achievement
			if( victory ) {
				int k = 0;
				for( c=0; c<MAXPLAYERS; c++ ) {
					if( players[c] )
						k++;
				}
				if( k>=2 )
					steamAchievement("BARONY_ACH_IN_GREATER_NUMBERS");
			}

			// make a highscore!
			saveScore();
			
			// pick a new subtitle :)
			subtitleCurrent = rand()%NUMSUBTITLES;
			subtitleVisible = TRUE;

			for( c=0; c<NUMMONSTERS; c++ ) {
				kills[c]=0;
			}

			// stop all sounds
			if( sound_group )
				FMOD_ChannelGroup_Stop(sound_group);
			
			// send disconnect messages
			if(multiplayer==CLIENT) {
				strcpy((char *)net_packet->data,"DISCONNECT");
				net_packet->data[10] = clientnum;
				net_packet->address.host = net_server.host;
				net_packet->address.port = net_server.port;
				net_packet->len = 11;
				sendPacketSafe(net_sock, -1, net_packet, 0);
				printlog("disconnected from server.\n");
			} else if(multiplayer==SERVER) {
				for(x=1; x<MAXPLAYERS; x++) {
					if( client_disconnected[x]==TRUE )
						continue;
					strcpy((char *)net_packet->data,"DISCONNECT");
					net_packet->data[10] = clientnum;
					net_packet->address.host = net_clients[x-1].host;
					net_packet->address.port = net_clients[x-1].port;
					net_packet->len = 11;
					sendPacketSafe(net_sock, -1, net_packet, x-1);
					client_disconnected[x]=TRUE;
				}
			}
		
			// clean up shopInv
			if( multiplayer==CLIENT ) {
				if( shopInv ) {
					list_FreeAll(shopInv);
					free(shopInv);
					shopInv = NULL;
				}
			}

			// delete save game
			if( !savethisgame ) {
				deleteSaveGame();
			} else {
				savethisgame=FALSE;
			}

			// reset game
			darkmap=FALSE;
			appraisal_timer=0;
			appraisal_item=0;
			multiplayer=0;
			shootmode = TRUE;
			currentlevel = 0;
			secretlevel = FALSE;
			clientnum = 0;
			introstage = 1;
			intro = TRUE;
			selected_spell = NULL; //So you don't start off with a spell when the game restarts.
			client_classes[0] = 0;
			spellcastingAnimationManager_deactivate(&cast_animation);
			SDL_StopTextInput();

			// delete game data clutter
			list_FreeAll(&messages);
			list_FreeAll(&command_history);
			list_FreeAll(&safePacketsSent);
			for( c=0; c<MAXPLAYERS; c++ )
				list_FreeAll(&safePacketsReceived[c]);
			deleteAllNotificationMessages();
			for(c=0;c<MAXPLAYERS;c++) {
				freePlayerEquipment(c);
				list_FreeAll(&stats[c].inventory);
				list_FreeAll(&stats[c].FOLLOWERS);
			}
			list_FreeAll(&removedEntities);
			list_FreeAll(&chestInv);
			
			// default player stats
			for( c=0; c<MAXPLAYERS; c++ ) {
				if( c>0 )
					client_disconnected[c]=TRUE;
				else
					client_disconnected[c]=FALSE;
				players[c]=NULL;
				stats[c].sex=static_cast<sex_t>(0);
				stats[c].appearance=0;
				strcpy(stats[c].name,"");
				stats[c].type = HUMAN;
				clearStats(&stats[c]);
				entitiesToDelete[c].first=NULL;
				entitiesToDelete[c].last=NULL;
				if( c==0 )
					initClass(c);
			}

			// hack to fix these things from breaking everything...
			hudarm = NULL;
			hudweapon = NULL;
			magicLeftHand = NULL;
			magicRightHand = NULL;
			
			// load menu level
			switch( rand()%4 ) {
				case 0:
					loadMap("mainmenu1",&map,map.entities);
					camera.x = 8;
					camera.y = 4.5;
					camera.z = 0;
					camera.ang = 0.6;
					break;
				case 1:
					loadMap("mainmenu2",&map,map.entities);
					camera.x = 7;
					camera.y = 4;
					camera.z = -4;
					camera.ang = 1.0;
					break;
				case 2:
					loadMap("mainmenu3",&map,map.entities);
					camera.x = 5;
					camera.y = 3;
					camera.z = 0;
					camera.ang = 1.0;
					break;
				case 3:
					loadMap("mainmenu4",&map,map.entities);
					camera.x = 6;
					camera.y = 14.5;
					camera.z = -24;
					camera.ang = 5.0;
					break;
			}
			camera.vang=0;
			numplayers=0;
			assignActions(&map);
			generatePathMaps();
			gamePaused = FALSE;
			if( !victory ) {
				fadefinished=FALSE;
				fadeout=FALSE;
				playmusic(intromusic, TRUE, FALSE, FALSE);
			} else {
				// conduct achievements
				if( conductPenniless )
					steamAchievement("BARONY_ACH_PENNILESS_CONDUCT");
				if( conductFoodless )
					steamAchievement("BARONY_ACH_FOODLESS_CONDUCT");
				if( conductVegetarian )
					steamAchievement("BARONY_ACH_VEGETARIAN_CONDUCT");
				if( conductIlliterate )
					steamAchievement("BARONY_ACH_ILLITERATE_CONDUCT");

				if( completionTime < 20*60*TICKS_PER_SECOND )
					steamAchievement("BARONY_ACH_BOOTS_OF_SPEED");

				if( victory==1 )
					introstage=7;
				else
					introstage=8;
			}

			// finish handling invite
			#ifdef STEAMWORKS
			if( stillConnectingToLobby ) {
				processLobbyInvite();
			}
			#endif
		} else if( introstage==6 ) { // introduction cutscene
			fadefinished=FALSE;
			fadeout=FALSE;
			intromoviestage++;
			if( intromoviestage>=9 ) {
				playmusic(intromusic, TRUE, FALSE, FALSE);
				introstage=1;
				intromovietime=0;
				intromoviestage=0;
				int c;
				for( c=0; c<30; c++ ) {
					intromoviealpha[c] = 0;
				}
				movie=FALSE;
			} else {
				intromovietime=0;
				movie=TRUE;
			}
		} else if( introstage==7 ) { // win game sequence (herx)
			if( firstendmoviestage==0 )
				playmusic(endgamemusic, TRUE, TRUE, FALSE);
			firstendmoviestage++;
			if( firstendmoviestage>=5 ) {
				introstage=4;
				firstendmovietime=0;
				firstendmoviestage=0;
				int c;
				for( c=0; c<30; c++ ) {
					firstendmoviealpha[c] = 0;
				}
				fadeout=TRUE;
			} else {
				fadefinished=FALSE;
				fadeout=FALSE;
				firstendmovietime=0;
				movie=TRUE;
			}
		} else if( introstage==8 ) { // win game sequence (devil)
			if( secondendmoviestage==0 )
				playmusic(endgamemusic, TRUE, TRUE, FALSE);
			secondendmoviestage++;
			if( secondendmoviestage>=5 ) {
				introstage=4;
				secondendmovietime=0;
				secondendmoviestage=0;
				int c;
				for( c=0; c<30; c++ ) {
					secondendmoviealpha[c] = 0;
				}
				fadeout=TRUE;
			} else {
				fadefinished=FALSE;
				fadeout=FALSE;
				secondendmovietime=0;
				movie=TRUE;
			}
		}
	}
	
	// credits sequence
	if( creditstage>0 ) {
		if( (credittime>=300 && (creditstage<=10 ||creditstage>12)) || (credittime>=180 && creditstage==11) ||
			(credittime>=480 && creditstage==12) || mousestatus[SDL_BUTTON_LEFT] ) {
			mousestatus[SDL_BUTTON_LEFT]=0;
			introstage=4;
			fadeout=TRUE;
		}
		
		// stages
		Uint32 colorBlue = SDL_MapRGBA(mainsurface->format,0,92,255,255);
		if( creditstage==1 ) {
			ttfPrintTextFormattedColor(ttf16,xres/2-(TTF16_WIDTH/2)*strlen(language[56]),yres/2-9-18,colorBlue,language[56]);
			ttfPrintTextFormatted(ttf16,xres/2-(TTF16_WIDTH/2)*strlen(CREDITSLINE02),yres/2-9+18,CREDITSLINE02);
		}
		else if( creditstage==2 ) {
			ttfPrintTextFormattedColor(ttf16,xres/2-(TTF16_WIDTH/2)*strlen(language[57]),yres/2-9-18,colorBlue,language[57]);
			ttfPrintTextFormatted(ttf16,xres/2-(TTF16_WIDTH/2)*strlen(CREDITSLINE04),yres/2-9+18,CREDITSLINE04);
		}
		else if( creditstage==3 ) {
			ttfPrintTextFormattedColor(ttf16,xres/2-(TTF16_WIDTH/2)*strlen(language[58]),yres/2-9-18,colorBlue,language[58]);
			ttfPrintTextFormatted(ttf16,xres/2-(TTF16_WIDTH/2)*strlen(CREDITSLINE06),yres/2-9+18,CREDITSLINE06);
		}
		else if( creditstage==4 ) {
			ttfPrintTextFormattedColor(ttf16,xres/2-(TTF16_WIDTH/2)*strlen(language[59]),yres/2-9-18*2,colorBlue,language[59]);
			ttfPrintTextFormatted(ttf16,xres/2-(TTF16_WIDTH/2)*strlen(CREDITSLINE39),yres/2+9,CREDITSLINE39);
		}
		else if( creditstage==5 ) {
			ttfPrintTextFormattedColor(ttf16,xres/2-(TTF16_WIDTH/2)*strlen(language[60]),yres/2-9-18,colorBlue,language[60]);
			ttfPrintTextFormatted(ttf16,xres/2-(TTF16_WIDTH/2)*strlen(CREDITSLINE11),yres/2-9,CREDITSLINE11);
			ttfPrintTextFormatted(ttf16,xres/2-(TTF16_WIDTH/2)*strlen(CREDITSLINE08),yres/2-9+18,CREDITSLINE08);
			ttfPrintTextFormatted(ttf16,xres/2-(TTF16_WIDTH/2)*strlen(CREDITSLINE09),yres/2+9+18*1,CREDITSLINE09);
		}
		else if( creditstage==6 ) {
			ttfPrintTextFormattedColor(ttf16,xres/2-(TTF16_WIDTH/2)*strlen(language[61]),yres/2-9-18,colorBlue,language[61]);
			ttfPrintTextFormatted(ttf16,xres/2-(TTF16_WIDTH/2)*strlen(CREDITSLINE13),yres/2-9+18,CREDITSLINE13);
		}
		else if( creditstage==7 ) {
			ttfPrintTextFormattedColor(ttf16,xres/2-(TTF16_WIDTH/2)*strlen(language[62]),yres/2-9-18*4,colorBlue,language[62]);
			ttfPrintTextFormatted(ttf16,xres/2-(TTF16_WIDTH/2)*strlen(CREDITSLINE15),yres/2-9-18*2,CREDITSLINE15);
			ttfPrintTextFormatted(ttf16,xres/2-(TTF16_WIDTH/2)*strlen(CREDITSLINE16),yres/2-9-18*1,CREDITSLINE16);
			ttfPrintTextFormatted(ttf16,xres/2-(TTF16_WIDTH/2)*strlen(CREDITSLINE17),yres/2-9,CREDITSLINE17);
			ttfPrintTextFormatted(ttf16,xres/2-(TTF16_WIDTH/2)*strlen(CREDITSLINE18),yres/2+9,CREDITSLINE18);
			ttfPrintTextFormatted(ttf16,xres/2-(TTF16_WIDTH/2)*strlen(CREDITSLINE19),yres/2+9+18*1,CREDITSLINE19);
			ttfPrintTextFormatted(ttf16,xres/2-(TTF16_WIDTH/2)*strlen(CREDITSLINE20),yres/2+9+18*2,CREDITSLINE20);
			ttfPrintTextFormatted(ttf16,xres/2-(TTF16_WIDTH/2)*strlen(CREDITSLINE21),yres/2+9+18*3,CREDITSLINE21);
		}
		else if( creditstage==8 ) {
			ttfPrintTextFormattedColor(ttf16,xres/2-(TTF16_WIDTH/2)*strlen(language[63]),yres/2-9-18*4,colorBlue,language[63]);
			ttfPrintTextFormatted(ttf16,xres/2-(TTF16_WIDTH/2)*strlen(CREDITSLINE23),yres/2-9-18*2,CREDITSLINE23);
			ttfPrintTextFormatted(ttf16,xres/2-(TTF16_WIDTH/2)*strlen(CREDITSLINE24),yres/2-9-18*1,CREDITSLINE24);
			ttfPrintTextFormatted(ttf16,xres/2-(TTF16_WIDTH/2)*strlen(CREDITSLINE25),yres/2-9,CREDITSLINE25);
			ttfPrintTextFormatted(ttf16,xres/2-(TTF16_WIDTH/2)*strlen(CREDITSLINE26),yres/2+9,CREDITSLINE26);
			ttfPrintTextFormatted(ttf16,xres/2-(TTF16_WIDTH/2)*strlen(CREDITSLINE27),yres/2+9+18*1,CREDITSLINE27);
			ttfPrintTextFormatted(ttf16,xres/2-(TTF16_WIDTH/2)*strlen(CREDITSLINE28),yres/2+9+18*2,CREDITSLINE28);
			ttfPrintTextFormatted(ttf16,xres/2-(TTF16_WIDTH/2)*strlen(CREDITSLINE29),yres/2+9+18*3,CREDITSLINE29);
			ttfPrintTextFormatted(ttf16,xres/2-(TTF16_WIDTH/2)*strlen(CREDITSLINE30),yres/2+9+18*4,CREDITSLINE30);
		}
		else if( creditstage==9 ) {
			ttfPrintTextFormattedColor(ttf16,xres/2-(TTF16_WIDTH/2)*strlen(language[64]),yres/2-9-18,colorBlue,language[64]);
			ttfPrintTextFormatted(ttf16,xres/2-(TTF16_WIDTH/2)*strlen(language[65]),yres/2-9+18,language[65]);
		}
		else if( creditstage==10 ) {
			// logo
			src.x = 0; src.y = 0;
			src.w = logo_bmp->w; src.h = logo_bmp->h;
			dest.x = xres/2-(logo_bmp->w+title_bmp->w)/2-16; dest.y = yres/2-logo_bmp->h/2;
			dest.w = xres; dest.h = yres;
			drawImage(logo_bmp, &src, &dest);
			// title
			src.x = 0; src.y = 0;
			src.w = title_bmp->w; src.h = title_bmp->h;
			dest.x = xres/2-(logo_bmp->w+title_bmp->w)/2+logo_bmp->w+16; dest.y = yres/2-title_bmp->h/2;
			dest.w = xres; dest.h = yres;
			drawImage(title_bmp, &src, &dest);
			// text
			ttfPrintTextFormatted(ttf16,xres/2-(TTF16_WIDTH/2)*strlen(language[66]),yres/2+96,language[66]);
			ttfPrintTextFormatted(ttf16,xres/2-(TTF16_WIDTH/2)*strlen(language[67]),yres/2+116,language[67]);
			ttfPrintTextFormatted(ttf16,xres/2-(TTF16_WIDTH/2)*strlen(language[68]),yres/2+136,language[68]);
			ttfPrintTextFormattedColor(ttf16,xres/2-(TTF16_WIDTH/2)*strlen(language[69]),yres/2+156,colorBlue,language[69]);
		}
		else if( creditstage==12 ) {
			ttfPrintTextFormatted(ttf16,xres/2-(TTF16_WIDTH/2)*strlen(CREDITSLINE37),yres/2-9,CREDITSLINE37);
			//ttfPrintTextFormattedColor(ttf16,xres/2-(TTF16_WIDTH/2)*strlen(CREDITSLINE37),yres/2+9,colorBlue,CREDITSLINE38);
		}
	}
	
	// intro sequence
	if( intromoviestage>0 ) {
		SDL_Rect pos;
		pos.x = 0; pos.y = 0;
		pos.w = xres; pos.h = (((double)xres)/backdrop_bmp->w)*backdrop_bmp->h;
		drawImageScaled(backdrop_bmp, NULL, &pos);

		if( intromovietime>=600 || mousestatus[SDL_BUTTON_LEFT] || keystatus[SDL_SCANCODE_ESCAPE] ||
			keystatus[SDL_SCANCODE_SPACE] || keystatus[SDL_SCANCODE_RETURN] || (intromovietime>=120 && intromoviestage==1) ) {
			intromovietime=0;
			mousestatus[SDL_BUTTON_LEFT]=0;
			if( intromoviestage!=9 ) {
				intromoviestage++;
			} else {
				introstage=6;
				fadeout=TRUE;
			}
		}
		
		if( intromoviestage>=1 ) {
			intromoviealpha[8] = std::min(intromoviealpha[8]+2,255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0,intromoviealpha[8]),255)<<24;
			ttfPrintTextColor(ttf16,16,yres-32,color,TRUE,language[1414]);
		}
		if( intromoviestage>=2 ) {
			intromoviealpha[0] = std::min(intromoviealpha[0]+2,255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0,intromoviealpha[0]),255)<<24;
			ttfPrintTextColor(ttf16,16+(xres-960)/2,16+(yres-600)/2,color,TRUE,language[1415]);
		}
		if( intromoviestage>=3 ) {
			intromoviealpha[1] = std::min(intromoviealpha[1]+2,255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0,intromoviealpha[1]),255)<<24;
			ttfPrintTextColor(ttf16,16+(xres-960)/2,16+(yres-600)/2,color,TRUE,language[1416]);
		}
		if( intromoviestage>=4 ) {
			intromoviealpha[2] = std::min(intromoviealpha[2]+2,255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0,intromoviealpha[2]),255)<<24;
			ttfPrintTextColor(ttf16,16+(xres-960)/2,16+(yres-600)/2,color,TRUE,language[1417]);
		}
		if( intromoviestage>=5 ) {
			intromoviealpha[3] = std::min(intromoviealpha[3]+2,255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0,intromoviealpha[3]),255)<<24;
			ttfPrintTextColor(ttf16,16+(xres-960)/2,16+(yres-600)/2,color,TRUE,language[1418]);
		}
		if( intromoviestage>=6 ) {
			intromoviealpha[4] = std::min(intromoviealpha[4]+2,255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0,intromoviealpha[4]),255)<<24;
			ttfPrintTextColor(ttf16,16+(xres-960)/2,16+(yres-600)/2,color,TRUE,language[1419]);
		}
		if( intromoviestage>=7 ) {
			intromoviealpha[5] = std::min(intromoviealpha[5]+2,255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0,intromoviealpha[5]),255)<<24;
			ttfPrintTextColor(ttf16,16+(xres-960)/2,16+(yres-600)/2,color,TRUE,language[1420]);
		}
		if( intromoviestage>=8 ) {
			intromoviealpha[6] = std::min(intromoviealpha[6]+2,255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0,intromoviealpha[6]),255)<<24;
			ttfPrintTextColor(ttf16,16+(xres-960)/2,16+(yres-600)/2,color,TRUE,language[1421]);
		}
		if( intromoviestage==9 ) {
			intromoviealpha[7] = std::min(intromoviealpha[7]+2,255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0,intromoviealpha[7]),255)<<24;
			ttfPrintTextColor(ttf16,16+(xres-960)/2,16+(yres-600)/2,color,TRUE,language[1422]);
		}
	}

	// first end sequence (defeating herx)
	if( firstendmoviestage>0 ) {
		SDL_Rect pos;
		pos.x = 0; pos.y = 0;
		pos.w = xres; pos.h = (((double)xres)/backdrop_bmp->w)*backdrop_bmp->h;
		drawImageScaled(backdrop_bmp, NULL, &pos);

		if( firstendmovietime>=600 || mousestatus[SDL_BUTTON_LEFT] || keystatus[SDL_SCANCODE_ESCAPE] ||
			keystatus[SDL_SCANCODE_SPACE] || keystatus[SDL_SCANCODE_RETURN] || (firstendmovietime>=120 && firstendmoviestage==1) ) {
			firstendmovietime=0;
			mousestatus[SDL_BUTTON_LEFT]=0;
			if( firstendmoviestage!=5 ) {
				firstendmoviestage++;
			} else {
				introstage=7;
				fadeout=TRUE;
			}
		}
		
		if( firstendmoviestage>=1 ) {
			firstendmoviealpha[8] = std::min(firstendmoviealpha[8]+2,255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0,firstendmoviealpha[8]),255)<<24;
			ttfPrintTextColor(ttf16,16,yres-32,color,TRUE,language[1414]);
		}
		if( firstendmoviestage>=2 ) {
			firstendmoviealpha[0] = std::min(firstendmoviealpha[0]+2,255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0,firstendmoviealpha[0]),255)<<24;
			ttfPrintTextColor(ttf16,16+(xres-960)/2,16+(yres-600)/2,color,TRUE,language[1423]);
		}
		if( firstendmoviestage>=3 ) {
			firstendmoviealpha[1] = std::min(firstendmoviealpha[1]+2,255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0,firstendmoviealpha[1]),255)<<24;
			ttfPrintTextColor(ttf16,16+(xres-960)/2,16+(yres-600)/2,color,TRUE,language[1424]);
		}
		if( firstendmoviestage>=4 ) {
			firstendmoviealpha[2] = std::min(firstendmoviealpha[2]+2,255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0,firstendmoviealpha[2]),255)<<24;
			ttfPrintTextColor(ttf16,16+(xres-960)/2,16+(yres-600)/2,color,TRUE,language[1425]);
		}
		if( firstendmoviestage==5 ) {
			firstendmoviealpha[3] = std::min(firstendmoviealpha[3]+2,255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0,firstendmoviealpha[3]),255)<<24;
			ttfPrintTextColor(ttf16,16+(xres-960)/2,16+(yres-600)/2,color,TRUE,language[1426]);
		}
	}

	// second end sequence (defeating the devil)
	if( secondendmoviestage>0 ) {
		SDL_Rect pos;
		pos.x = 0; pos.y = 0;
		pos.w = xres; pos.h = (((double)xres)/backdrop_bmp->w)*backdrop_bmp->h;
		drawImageScaled(backdrop_bmp, NULL, &pos);

		if( secondendmovietime>=600 || mousestatus[SDL_BUTTON_LEFT] || keystatus[SDL_SCANCODE_ESCAPE] ||
			keystatus[SDL_SCANCODE_SPACE] || keystatus[SDL_SCANCODE_RETURN] || (secondendmovietime>=120 && secondendmoviestage==1) ) {
			secondendmovietime=0;
			mousestatus[SDL_BUTTON_LEFT]=0;
			if( secondendmoviestage!=7 ) {
				secondendmoviestage++;
			} else {
				introstage=8;
				fadeout=TRUE;
			}
		}
		
		if( secondendmoviestage>=1 ) {
			secondendmoviealpha[8] = std::min(secondendmoviealpha[8]+2,255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0,secondendmoviealpha[8]),255)<<24;
			ttfPrintTextColor(ttf16,16,yres-32,color,TRUE,language[1414]);
		}
		if( secondendmoviestage>=2 ) {
			secondendmoviealpha[0] = std::min(secondendmoviealpha[0]+2,255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0,secondendmoviealpha[0]),255)<<24;
			ttfPrintTextColor(ttf16,16+(xres-960)/2,16+(yres-600)/22,color,TRUE,language[1427]);
		}
		if( secondendmoviestage>=3 ) {
			secondendmoviealpha[1] = std::min(secondendmoviealpha[1]+2,255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0,secondendmoviealpha[1]),255)<<24;
			ttfPrintTextColor(ttf16,16+(xres-960)/2,16+(yres-600)/2,color,TRUE,language[1428]);
		}
		if( secondendmoviestage>=4 ) {
			secondendmoviealpha[2] = std::min(secondendmoviealpha[2]+2,255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0,secondendmoviealpha[2]),255)<<24;
			ttfPrintTextColor(ttf16,16+(xres-960)/2,16+(yres-600)/2,color,TRUE,language[1429]);
		}
		if( secondendmoviestage>=5 ) {
			secondendmoviealpha[3] = std::min(secondendmoviealpha[3]+2,255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0,secondendmoviealpha[3]),255)<<24;
			ttfPrintTextColor(ttf16,16+(xres-960)/2,16+(yres-600)/2,color,TRUE,language[1430]);
		}
		if( secondendmoviestage>=6 ) {
			secondendmoviealpha[4] = std::min(secondendmoviealpha[4]+2,255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0,secondendmoviealpha[4]),255)<<24;
			ttfPrintTextColor(ttf16,16+(xres-960)/2,16+(yres-600)/2,color,TRUE,language[1431]);
		}
		if( secondendmoviestage==7 ) {
			secondendmoviealpha[5] = std::min(secondendmoviealpha[5]+2,255);
			Uint32 color = 0x00FFFFFF;
			color += std::min(std::max(0,secondendmoviealpha[5]),255)<<24;
			ttfPrintTextColor(ttf16,16+(xres-960)/2,16+(yres-600)/2,color,TRUE,language[1432]);
		}
	}
}

/*-------------------------------------------------------------------------------

	button functions
	
	this section contains numerous button functions for the game

-------------------------------------------------------------------------------*/

// opens the gameover window
void openGameoverWindow() {
	node_t *node;
	
	subwindow = 1;
	subx1 = xres/2-288;
	subx2 = xres/2+288;
	suby1 = yres/2-160;
	suby2 = yres/2+160;
	button_t *button;

	// calculate player score
	char scorenum[16];
	score_t *score = scoreConstructor();
	Uint32 total = totalScore(score);
	snprintf(scorenum, 16, "%d\n\n", total);
	scoreDeconstructor((void *)score);

	bool madetop=FALSE;
	if( !list_Size(&topscores) ) {
		madetop=TRUE;
	} else if( list_Size(&topscores)<MAXTOPSCORES ) {
		madetop=TRUE;
	} else if( totalScore((score_t *)topscores.last->element)<total ) {
		madetop=TRUE;
	}

	shootmode = FALSE;
	if( multiplayer==SINGLE ) {
		strcpy(subtext,language[1133]);

		strcat(subtext,language[1134]);

		strcat(subtext,language[1135]);
		strcat(subtext,scorenum);

		if( madetop )
			strcat(subtext,language[1136]);
		else
			strcat(subtext,language[1137]);

		// identify all inventory items
		for( node=stats[clientnum].inventory.first; node!=NULL; node=node->next ) {
			Item *item = (Item *)node->element;
			item->identified = TRUE;
		}

		// Restart
		button = newButton();
		strcpy(button->label,language[1138]);
		button->x=subx2-strlen(language[1138])*12-16; button->y=suby2-28;
		button->sizex=strlen(language[1138])*12+8; button->sizey=20;
		button->action=&buttonStartSingleplayer;
		button->visible=1;
		button->focused=1;

		// Return to Main Menu
		button = newButton();
		strcpy(button->label,language[1139]);
		button->x=subx1+8; button->y=suby2-28;
		button->sizex=strlen(language[1139])*12+8; button->sizey=20;
		button->action=&buttonEndGameConfirm;
		button->visible=1;
		button->focused=1;
	} else {
		strcpy(subtext,language[1140]);
		
		bool survivingPlayer=FALSE;
		int c;
		for( c=0; c<MAXPLAYERS; c++ ) {
			if( !client_disconnected[c] && players[c] ) {
				survivingPlayer=TRUE;
				break;
			}
		}
		if( survivingPlayer ) {
			strcat(subtext,language[1141]);
		} else {
			strcat(subtext,language[1142]);
		}

		strcat(subtext,language[1143]);
		strcat(subtext,scorenum);

		strcat(subtext,"\n\n");

		// Okay
		button = newButton();
		strcpy(button->label,language[1144]);
		button->sizex=strlen(language[1144])*12+8; button->sizey=20;
		button->x=subx1+(subx2-subx1)/2-button->sizex/2; button->y=suby2-28;
		button->action=&buttonCloseSubwindow;
		button->visible=1;
		button->focused=1;
	}

	// death hints
	if( currentlevel/LENGTH_OF_LEVEL_REGION<1 ) {
		strcat(subtext,language[1145+rand()%15]);
	}

	// close button
	button = newButton();
	strcpy(button->label,"x");
	button->x=subx2-20; button->y=suby1;
	button->sizex=20; button->sizey=20;
	button->action=&buttonCloseSubwindow;
	button->visible=1;
	button->focused=1;
	button->key=SDL_SCANCODE_ESCAPE;
}

// sets up the settings window
void openSettingsWindow() {
	button_t *button;
	int c;

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
	settings_sfxvolume = sfxvolume;
	settings_musvolume = musvolume;
	for( c=0; c<NUMIMPULSES; c++ )
		settings_impulses[c] = impulses[c];
	settings_reversemouse = reversemouse;
	settings_smoothmouse = smoothmouse;
	settings_mousespeed = mousespeed;
	settings_broadcast = broadcast;
	settings_nohud = nohud;
	settings_auto_hotbar_new_items = auto_hotbar_new_items;
	settings_disable_messages = disable_messages;
	
	// create settings window
	settings_window = TRUE;
	subwindow = 1;
	subx1 = xres/2-256;
	subx2 = xres/2+256;
	suby1 = yres/2-192;
	suby2 = yres/2+192;
	strcpy(subtext,language[1306]);

	// close button
	button = newButton();
	strcpy(button->label,"x");
	button->x=subx2-20; button->y=suby1;
	button->sizex=20; button->sizey=20;
	button->action=&buttonCloseSubwindow;
	button->visible=1;
	button->focused=1;
	button->key=SDL_SCANCODE_ESCAPE;
	
	// cancel button
	button = newButton();
	strcpy(button->label,language[1316]);
	button->x=subx1+8; button->y=suby2-28;
	button->sizex=strlen(language[1316])*12+8; button->sizey=20;
	button->action=&buttonCloseSubwindow;
	button->visible=1;
	button->focused=1;
	
	// ok button
	button = newButton();
	strcpy(button->label,language[1433]);
	button->x=subx2-strlen(language[1433])*12-16; button->y=suby2-28;
	button->sizex=strlen(language[1433])*12+8; button->sizey=20;
	button->action=&buttonSettingsOK;
	button->visible=1;
	button->focused=1;
	button->key=SDL_SCANCODE_RETURN;
	
	// accept button
	button = newButton();
	strcpy(button->label,language[1317]);
	button->x=subx2-strlen(language[1317])*12-16-strlen(language[1317])*12-16; button->y=suby2-28;
	button->sizex=strlen(language[1317])*12+8; button->sizey=20;
	button->action=&buttonSettingsAccept;
	button->visible=1;
	button->focused=1;
	
	// video tab
	button = newButton();
	strcpy(button->label,language[1434]);
	button->x=subx1+16; button->y=suby1+24;
	button->sizex=strlen(language[1434])*12+8; button->sizey=20;
	button->action=&buttonVideoTab;
	button->visible=1;
	button->focused=1;
	
	// audio tab
	button = newButton();
	strcpy(button->label,language[1435]);
	button->x=subx1+16+strlen(language[1434])*12+8; button->y=suby1+24;
	button->sizex=strlen(language[1435])*12+8; button->sizey=20;
	button->action=&buttonAudioTab;
	button->visible=1;
	button->focused=1;
	
	// keyboard tab
	button = newButton();
	strcpy(button->label,language[1436]);
	button->x=subx1+16+strlen(language[1434])*12+8+strlen(language[1435])*12+8; button->y=suby1+24;
	button->sizex=strlen(language[1436])*12+8; button->sizey=20;
	button->action=&buttonKeyboardTab;
	button->visible=1;
	button->focused=1;
	
	// mouse tab
	button = newButton();
	strcpy(button->label,language[1437]);
	button->x=subx1+16+strlen(language[1434])*12+8+strlen(language[1435])*12+8+strlen(language[1436])*12+8; button->y=suby1+24;
	button->sizex=strlen(language[1437])*12+8; button->sizey=20;
	button->action=&buttonMouseTab;
	button->visible=1;
	button->focused=1;

	// misc tab
	button = newButton();
	strcpy(button->label,language[1438]);
	button->x=subx1+16+strlen(language[1434])*12+8+strlen(language[1435])*12+8+strlen(language[1436])*12+8+strlen(language[1437])*12+8; button->y=suby1+24;
	button->sizex=strlen(language[1438])*12+8; button->sizey=20;
	button->action=&buttonMiscTab;
	button->visible=1;
	button->focused=1;
}

void openSteamLobbyWaitWindow(button_t *my);

// "failed to connect" message
void openFailedConnectionWindow(int mode) {
	button_t *button;
	
	// close current window
	buttonCloseSubwindow(NULL);
	list_FreeAll(&button_l);
	deleteallbuttons=TRUE;
	
	// create new window
	subwindow = 1;
	subx1 = xres/2-256;
	subx2 = xres/2+256;
	suby1 = yres/2-64;
	suby2 = yres/2+64;
	if( directConnect ) {
		if( mode==CLIENT ) {
			strcpy(subtext,language[1439]);
			strcat(subtext,SDLNet_GetError());
		} else if( mode==SERVER ) {
			strcpy(subtext,language[1440]);
			strcat(subtext,SDLNet_GetError());
		} else {
			strcpy(subtext,language[1443]);
		}
	} else {
		if( mode==CLIENT ) {
			strcpy(subtext,language[1441]);
		} else if( mode==SERVER ) {
			strcpy(subtext,language[1442]);
		} else {
			strcpy(subtext,language[1443]);
		}
	}

	// close button
	button = newButton();
	strcpy(button->label,"x");
	button->x=subx2-20; button->y=suby1;
	button->sizex=20; button->sizey=20;
	button->action=&buttonCloseSubwindow;
	button->visible=1;
	button->focused=1;
	button->key=SDL_SCANCODE_ESCAPE;
	
	// okay button
	button = newButton();
	strcpy(button->label,language[732]);
	button->x=subx2-(subx2-subx1)/2-strlen(language[732])*6; button->y=suby2-24;
	button->sizex=strlen(language[732])*12+8; button->sizey=20;
	button->visible=1;
	button->focused=1;
	button->key=SDL_SCANCODE_RETURN;
	
	if( directConnect ) {
		if( mode==CLIENT )
			button->action=&buttonJoinMultiplayer;
		else if( mode==SERVER )
			button->action=&buttonHostMultiplayer;
		else
			button->action=&buttonCloseSubwindow;
	} else {
		if( mode==CLIENT )
			button->action=&openSteamLobbyWaitWindow;
		else if( mode==SERVER )
			button->action=&buttonCloseSubwindow;
		else
			button->action=&buttonCloseSubwindow;
	}

	multiplayer=SINGLE;
	clientnum=0;
}

// opens the wait window for steam lobby (getting lobby list, etc.)
void openSteamLobbyWaitWindow(button_t *my) {
	button_t *button;

	// close current window
	buttonCloseSubwindow(NULL);
	list_FreeAll(&button_l);
	deleteallbuttons=TRUE;
	
	// create new window
	subwindow = 1;
	#ifdef STEAMWORKS
	requestingLobbies = TRUE;
	#endif
	subx1 = xres/2-256;
	subx2 = xres/2+256;
	suby1 = yres/2-64;
	suby2 = yres/2+64;
	strcpy(subtext,language[1444]);
	#ifdef STEAMWORKS
	//c_SteamMatchmaking_RequestLobbyList();
	//SteamMatchmaking()->RequestLobbyList(); //TODO: Is this sufficient for it to work?
	cpp_SteamMatchmaking_RequestLobbyList();
	#endif

	// close button
	button = newButton();
	strcpy(button->label,"x");
	button->x=subx2-20; button->y=suby1;
	button->sizex=20; button->sizey=20;
	button->action=&buttonCloseSubwindow;
	button->visible=1;
	button->focused=1;
	button->key=SDL_SCANCODE_ESCAPE;

	// cancel button
	button = newButton();
	strcpy(button->label,language[1316]);
	button->sizex=strlen(language[1316])*12+8; button->sizey=20;
	button->x=subx1 + (subx2-subx1)/2 - button->sizex/2; button->y=suby2-28;
	button->action=&buttonCloseSubwindow;
	button->visible=1;
	button->focused=1;
}

// opens the lobby browser window (steam client only)
void openSteamLobbyBrowserWindow(button_t *my) {
	button_t *button;
	
	// close current window
	buttonCloseSubwindow(NULL);
	list_FreeAll(&button_l);
	deleteallbuttons=TRUE;
	
	// create new window
	subwindow = 1;
	subx1 = xres/2-280;
	subx2 = xres/2+280;
	suby1 = yres/2-192;
	suby2 = yres/2+192;
	strcpy(subtext,language[1334]);

	// setup lobby browser
	#ifdef STEAMWORKS //TODO: Should this whole function be ifdeffed?
	selectedSteamLobby=0;
	#endif
	slidery=0;
	oslidery=0;

	// close button
	button = newButton();
	strcpy(button->label,"x");
	button->x=subx2-20; button->y=suby1;
	button->sizex=20; button->sizey=20;
	button->action=&buttonCloseSubwindow;
	button->visible=1;
	button->focused=1;
	button->key=SDL_SCANCODE_ESCAPE;
	
	// join button
	button = newButton();
	strcpy(button->label,language[1445]);
	button->x=subx1+8; button->y=suby2-56;
	button->sizex=strlen(language[1445])*12+8; button->sizey=20;
	#ifdef STEAMWORKS
	button->action=&buttonSteamLobbyBrowserJoinGame;
	#endif
	button->visible=1;
	button->focused=1;
	button->key=SDL_SCANCODE_RETURN;
	
	// refresh button
	button = newButton();
	strcpy(button->label,language[1446]);
	button->x=subx1+8; button->y=suby2-28;
	button->sizex=strlen(language[1446])*12+8; button->sizey=20;
	#ifdef STEAMWORKS
	button->action=&buttonSteamLobbyBrowserRefresh;
	#endif
	button->visible=1;
	button->focused=1;
}

// steam lobby browser join game
void buttonSteamLobbyBrowserJoinGame(button_t *my) {
	#ifndef STEAMWORKS
	return;
	#else

	button_t *button;
	int lobbyIndex = std::min(std::max(0,selectedSteamLobby),MAX_STEAM_LOBBIES-1);
	if( lobbyIDs[lobbyIndex] ) {
		// close current window
		int temp1 = connectingToLobby;
		int temp2 = connectingToLobbyWindow;
		//buttonCloseSubwindow(my);
		list_FreeAll(&button_l);
		deleteallbuttons=TRUE;
		connectingToLobby = temp1;
		connectingToLobbyWindow = temp2;
				
		// create new window
		subwindow = 1;
		subx1 = xres/2-256;
		subx2 = xres/2+256;
		suby1 = yres/2-64;
		suby2 = yres/2+64;
		strcpy(subtext,language[1447]);

		// close button
		button = newButton();
		strcpy(button->label,"x");
		button->x=subx2-20; button->y=suby1;
		button->sizex=20; button->sizey=20;
		button->action=&openSteamLobbyWaitWindow;
		button->visible=1;
		button->focused=1;
		button->key=SDL_SCANCODE_ESCAPE;

		// cancel button
		button = newButton();
		strcpy(button->label,language[1316]);
		button->sizex=strlen(language[1316])*12+8; button->sizey=20;
		button->x=subx1 + (subx2-subx1)/2 - button->sizex/2; button->y=suby2-28;
		button->action=&openSteamLobbyWaitWindow;
		button->visible=1;
		button->focused=1;
		
		connectingToLobby = TRUE;
		connectingToLobbyWindow = TRUE;
		strncpy( currentLobbyName, lobbyText[lobbyIndex], 31 );
		cpp_SteamMatchmaking_JoinLobby(*static_cast<CSteamID* >(lobbyIDs[lobbyIndex]));
	}
	#endif
}

// steam lobby browser refresh
void buttonSteamLobbyBrowserRefresh(button_t *my) {
	#ifndef STEAMWORKS
	return;
	#else
	openSteamLobbyWaitWindow(my);
	#endif
}

// quit game button
void buttonQuitConfirm(button_t *my) {
	subwindow=0;
	introstage=2; // prepares to quit the whole game
	fadeout=TRUE;
}

// quit game button (no save)
void buttonQuitNoSaveConfirm(button_t *my) {
	buttonQuitConfirm(my);
	deleteSaveGame();

	// make a highscore!
	saveScore();
}

// end game button
bool savethisgame=FALSE;
void buttonEndGameConfirm(button_t *my) {
	savethisgame=FALSE;
	subwindow=0;
	introstage=5; // prepares to end the current game (throws to main menu)
	fadeout=TRUE;
	//Edge case for freeing channeled spells on a client.
	if (multiplayer == CLIENT) {
		list_FreeAll(&channeledSpells[clientnum]);
	}
	if( !intro )
		pauseGame(2,FALSE);
}

void buttonEndGameConfirmSave(button_t *my) {
	subwindow=0;
	introstage=5; // prepares to end the current game (throws to main menu)
	fadeout=TRUE;
	savethisgame=TRUE;
	if( !intro )
		pauseGame(2,FALSE);
}

// generic close window button
void buttonCloseSubwindow(button_t *my) {
	int c;
	for( c=0; c<512; c++ ) {
		keystatus[c] = 0;
	}
	if( !subwindow )
		return;
	if( score_window ) {
		// reset class loadout
		stats[0].sex=static_cast<sex_t>(0);
		stats[0].appearance=0;
		strcpy(stats[0].name,"");
		stats[0].type = HUMAN;
		client_classes[0] = 0;
		clearStats(&stats[0]);
		initClass(0);
	}
	rebindkey=-1;
	#ifdef STEAMWORKS
	requestingLobbies=FALSE;
	#endif
	score_window=0;
	lobby_window=FALSE;
	settings_window=FALSE;
	connect_window=0;
	#ifdef STEAMWORKS
	if( charcreation_step ) {
		if( lobbyToConnectTo ) {
			// cancel lobby invitation acceptance
			cpp_Free_CSteamID(lobbyToConnectTo); //TODO: Bugger this.
			lobbyToConnectTo = NULL;
		}
	}
	connectingToLobbyWindow=FALSE;
	connectingToLobby=FALSE;
	#endif
	charcreation_step=0;
	subwindow=0;
	if( SDL_IsTextInputActive() )
		SDL_StopTextInput();
	playSound(138,64);
}

void buttonCloseAndEndGameConfirm(button_t *my) {
	//Edge case for freeing channeled spells on a client.
	if (multiplayer == CLIENT) {
		list_FreeAll(&channeledSpells[clientnum]);
	}
	buttonCloseSubwindow(my);
	buttonEndGameConfirmSave(my);
}

Uint32 charcreation_ticks=0;

// move player forward through creation dialogue
void buttonContinue(button_t *my) {
	button_t *button;

	if( ticks-charcreation_ticks<TICKS_PER_SECOND/10 )
		return;
	charcreation_ticks = ticks;
	if( charcreation_step==4 && !strcmp(stats[0].name,"") )
		return;
	
	charcreation_step++;
	if( charcreation_step==4 ) {
		inputstr = stats[0].name;
		SDL_StartTextInput();
	} else if( charcreation_step==5 ) {
		if( SDL_IsTextInputActive() ) {
			SDL_StopTextInput();
		}
		#ifdef STEAMWORKS
		if( lobbyToConnectTo ) {
			charcreation_step=0;
			
			// close current window
			int temp1 = connectingToLobby;
			int temp2 = connectingToLobbyWindow;
			//buttonCloseSubwindow(my);
			list_FreeAll(&button_l);
			deleteallbuttons=TRUE;
			connectingToLobby = temp1;
			connectingToLobbyWindow = temp2;
			
			// create new window
			subwindow = 1;
			subx1 = xres/2-256;
			subx2 = xres/2+256;
			suby1 = yres/2-64;
			suby2 = yres/2+64;
			strcpy(subtext,language[1447]);

			// close button
			button = newButton();
			strcpy(button->label,"x");
			button->x=subx2-20; button->y=suby1;
			button->sizex=20; button->sizey=20;
			button->action=&openSteamLobbyWaitWindow;
			button->visible=1;
			button->focused=1;
			button->key=SDL_SCANCODE_ESCAPE;

			// cancel button
			button = newButton();
			strcpy(button->label,language[1316]);
			button->sizex=strlen(language[1316])*12+8; button->sizey=20;
			button->x=subx1 + (subx2-subx1)/2 - button->sizex/2; button->y=suby2-28;
			button->action=&openSteamLobbyWaitWindow;
			button->visible=1;
			button->focused=1;
			
			connectingToLobby = TRUE;
			connectingToLobbyWindow = TRUE;
			strncpy( currentLobbyName, "", 31 );
			cpp_SteamMatchmaking_JoinLobby(*static_cast<CSteamID *>(lobbyToConnectTo));
			cpp_Free_CSteamID(lobbyToConnectTo); //TODO: Bugger this.
			lobbyToConnectTo = NULL;
		}
		#endif
	} else if( charcreation_step==6 ) {
		if( multiplayerselect==SINGLE ) {
			buttonStartSingleplayer(my);
		} else if( multiplayerselect==SERVER ) {
			#ifdef STEAMWORKS
			directConnect=FALSE;
			#else
			directConnect=TRUE;
			#endif
			buttonHostMultiplayer(my);
		} else if( multiplayerselect==CLIENT ) {
			#ifndef STEAMWORKS
			directConnect=TRUE;
			buttonJoinMultiplayer(my);
			#else
			directConnect=FALSE;
			openSteamLobbyWaitWindow(my);
			#endif
		} else if( multiplayerselect==DIRECTSERVER ) {
			directConnect=TRUE;
			buttonHostMultiplayer(my);
		} else if( multiplayerselect==DIRECTCLIENT ) {
			directConnect=TRUE;
			buttonJoinMultiplayer(my);
		}
	}
}

// move player backward through creation dialogue
void buttonBack(button_t *my) {
	charcreation_step--;
	if (charcreation_step < 4)
		playing_random_char = FALSE;
	if( charcreation_step==3 )
		SDL_StopTextInput();
	else if( charcreation_step==0 )
		buttonCloseSubwindow(my);
}

// start a singleplayer game
void buttonStartSingleplayer(button_t *my) {
	buttonCloseSubwindow(my);
	multiplayer=SINGLE;
	numplayers=0;
	introstage=3;
	fadeout=TRUE;
	if( !intro )
		pauseGame(2,FALSE);
}

// host a multiplayer game
void buttonHostMultiplayer(button_t *my) {
	button_t *button;

	// refresh keepalive
	int c;
	for( c=0; c<MAXPLAYERS; c++ )
		client_keepalive[c] = ticks;

	if( !directConnect ) {
		snprintf(portnumber_char, 6, "%d", DEFAULT_PORT);
		buttonHostLobby(my);
	} else {
		// close current window
		buttonCloseSubwindow(my);
		list_FreeAll(&button_l);
		deleteallbuttons=TRUE;
	
		// open port window
		connect_window = SERVER;
		subwindow = 1;
		subx1 = xres/2-128;
		subx2 = xres/2+128;
		suby1 = yres/2-56;
		suby2 = yres/2+56;
		strcpy(subtext,language[1448]);

		// close button
		button = newButton();
		strcpy(button->label,"x");
		button->x=subx2-20; button->y=suby1;
		button->sizex=20; button->sizey=20;
		button->action=&buttonCloseSubwindow;
		button->visible=1;
		button->focused=1;
		button->key=SDL_SCANCODE_ESCAPE;
	
		// host button
		button = newButton();
		strcpy(button->label,language[1449]);
		button->sizex=strlen(language[1449])*12+8; button->sizey=20;
		button->x=subx2-button->sizex-4; button->y=suby2-24;
		button->action=&buttonHostLobby;
		button->visible=1;
		button->focused=1;
		button->key=SDL_SCANCODE_RETURN;
	
		// cancel button
		button = newButton();
		strcpy(button->label,language[1316]);
		button->sizex=strlen(language[1316])*12+8; button->sizey=20;
		button->x=subx1+4; button->y=suby2-24;
		button->action=&buttonCloseSubwindow;
		button->visible=1;
		button->focused=1;

		strcpy(portnumber_char, last_port); //Copy the last used port.
	}
}

// join a multiplayer game
void buttonJoinMultiplayer(button_t *my) {
	button_t *button;
	
	// close current window
	buttonCloseSubwindow(my);
	list_FreeAll(&button_l);
	deleteallbuttons=TRUE;
	
	// open port window
	connect_window = CLIENT;
	subwindow = 1;
	subx1 = xres/2-210;
	subx2 = xres/2+210;
	suby1 = yres/2-56;
	suby2 = yres/2+56;
	strcpy(subtext,language[1450]);

	// close button
	button = newButton();
	strcpy(button->label,"x");
	button->x=subx2-20; button->y=suby1;
	button->sizex=20; button->sizey=20;
	button->action=&buttonCloseSubwindow;
	button->visible=1;
	button->focused=1;
	button->key=SDL_SCANCODE_ESCAPE;
	
	// join button
	button = newButton();
	strcpy(button->label,language[1451]);
	button->sizex=strlen(language[1451])*12+8; button->sizey=20;
	button->x=subx2-button->sizex-4; button->y=suby2-24;
	button->action=&buttonJoinLobby;
	button->visible=1;
	button->focused=1;
	button->key=SDL_SCANCODE_RETURN;
	
	// cancel button
	button = newButton();
	strcpy(button->label,language[1316]);
	button->x=subx1+4; button->y=suby2-24;
	button->sizex=strlen(language[1316])*12+8; button->sizey=20;
	button->action=&buttonCloseSubwindow;
	button->visible=1;
	button->focused=1;

	strcpy(connectaddress, last_ip); //Copy the last used IP.
}

// starts a lobby as host
void buttonHostLobby(button_t *my) {
	button_t *button;
	int c;

	// close current window
	buttonCloseSubwindow(my);
	list_FreeAll(&button_l);
	deleteallbuttons=TRUE;
	portnumber = atoi(portnumber_char); // get the port number from the text field
	list_FreeAll(&lobbyChatboxMessages);
	newString(&lobbyChatboxMessages,0xFFFFFFFF,language[1452]);
	if( loadingsavegame )
		newString(&lobbyChatboxMessages,0xFFFFFFFF,language[1453]);

	// close any existing net interfaces
	closeNetworkInterfaces();

	if( !directConnect ) {
		#ifdef STEAMWORKS
		for( c=0; c<MAXPLAYERS; c++ ) {
			if( steamIDRemote[c] ) {
				cpp_Free_CSteamID( steamIDRemote[c] ); //TODO: Bugger this.
				steamIDRemote[c] = NULL;
			}
		}
		currentLobbyType = k_ELobbyTypePrivate;
		cpp_SteamMatchmaking_CreateLobby(currentLobbyType, 4);
		#endif
	} else {
		// resolve host's address
		if(SDLNet_ResolveHost(&net_server, NULL, portnumber) == -1) {
			printlog( "warning: resolving host at localhost:%d has failed.\n", portnumber);
			openFailedConnectionWindow(SERVER);
			return;
		}
	
		// open sockets
		if(!(net_sock = SDLNet_UDP_Open(portnumber))) {
			printlog( "warning: SDLNet_UDP_open has failed: %s\n", SDLNet_GetError());
			openFailedConnectionWindow(SERVER);
			return;
		}
		if(!(net_tcpsock = SDLNet_TCP_Open(&net_server))) {
			printlog( "warning: SDLNet_TCP_open has failed: %s\n", SDLNet_GetError());
			openFailedConnectionWindow(SERVER);
			return;
		}
		tcpset = SDLNet_AllocSocketSet(4);
		SDLNet_TCP_AddSocket(tcpset, net_tcpsock);
	}
		
	// allocate data for client connections
	net_clients = (IPaddress *) malloc(sizeof(IPaddress)*MAXPLAYERS);
	net_tcpclients = (TCPsocket *) malloc(sizeof(TCPsocket)*MAXPLAYERS);
	for( c=0; c<MAXPLAYERS; c++ )
		net_tcpclients[c]=NULL;

	// allocate packet data
	if(!(net_packet = SDLNet_AllocPacket(NET_PACKET_SIZE))) {
		printlog( "warning: packet allocation failed: %s\n", SDLNet_GetError());
		openFailedConnectionWindow(SERVER);
		return;
	}
	
	if( directConnect ) {
		printlog( "server initialized successfully.\n");
	} else {
		printlog( "steam lobby opened successfully.\n");
	}
		
	// open lobby window
	multiplayer = SERVER;
	lobby_window = TRUE;
	subwindow = 1;
	subx1 = xres/2-400;
	subx2 = xres/2+400;
	suby1 = yres/2-300;
	suby2 = yres/2+300;
	if( directConnect ) {
		strcpy(subtext, language[1454]);
		strcat(subtext, portnumber_char);
		strcat(subtext, language[1456]);
	} else {
		strcpy(subtext, language[1455]);
		strcat(subtext, language[1456]);
	}
	
	// start game button
	button = newButton();
	strcpy(button->label,language[1457]);
	button->sizex=strlen(language[1457])*12+8; button->sizey=20;
	button->x=subx2-button->sizex-4; button->y=suby2-24;
	button->action=&buttonStartServer;
	button->visible=1;
	button->focused=1;
	
	// disconnect button
	button = newButton();
	strcpy(button->label,language[1311]);
	button->sizex=strlen(language[1311])*12+8; button->sizey=20;
	button->x=subx1+4; button->y=suby2-24;
	button->action=&buttonDisconnect;
	button->visible=1;
	button->focused=1;
	button->key=SDL_SCANCODE_ESCAPE;
	c = button->x+button->sizex + 4;
	
	// invite friends button
	if( !directConnect ) {
		#ifdef STEAMWORKS
		button = newButton();
		strcpy(button->label,language[1458]);
		button->sizex=strlen(language[1458])*12+8; button->sizey=20;
		button->x=c; button->y=suby2-24;
		button->action=&buttonInviteFriends;
		button->visible=1;
		button->focused=1;
		#endif
	}

	if( loadingsavegame )
		loadGame(clientnum);

	strcpy(last_port, portnumber_char);
	saveConfig("default.cfg");
}

// joins a lobby as client
void buttonJoinLobby(button_t *my) {
	button_t *button;
	int c;

	// refresh keepalive
	client_keepalive[0] = ticks;

	// close current window
	#ifdef STEAMWORKS
	int temp1 = connectingToLobby;
	int temp2 = connectingToLobbyWindow;
	#endif
	if( directConnect )
		buttonCloseSubwindow(my);
	list_FreeAll(&button_l);
	deleteallbuttons=TRUE;
	#ifdef STEAMWORKS
	connectingToLobby = temp1;
	connectingToLobbyWindow = temp2;
	#endif

	if( loadingsavegame )
		loadGame(getSaveGameClientnum());
		
	// open wait window
	list_FreeAll(&lobbyChatboxMessages);
	newString(&lobbyChatboxMessages,0xFFFFFFFF,language[1452]);
	multiplayer = CLIENT;
	subwindow = 1;
	subx1 = xres/2-256;
	subx2 = xres/2+256;
	suby1 = yres/2-64;
	suby2 = yres/2+64;
	strcpy(subtext,language[1459]);

	// close button
	button = newButton();
	strcpy(button->label,"x");
	button->x=subx2-20; button->y=suby1;
	button->sizex=20; button->sizey=20;
	button->action=&openSteamLobbyWaitWindow;
	button->visible=1;
	button->focused=1;
	button->key=SDL_SCANCODE_ESCAPE;

	// cancel button
	button = newButton();
	strcpy(button->label,language[1316]);
	button->sizex=strlen(language[1316])*12+8; button->sizey=20;
	button->x=subx1 + (subx2-subx1)/2 - button->sizex/2; button->y=suby2-28;
	button->action=&openSteamLobbyWaitWindow;
	button->visible=1;
	button->focused=1;
	
	if( directConnect ) {
		for( c=0; c<sizeof(connectaddress); c++ ) {
			if( connectaddress[c] == ':' )
				break;
		}
		strncpy(address, connectaddress, c); // get the address from the text field
		portnumber = atoi(&connectaddress[c+1]); // get the port number from the text field
		strcpy(last_ip, connectaddress);
		saveConfig("default.cfg");
	}
	
	// close any existing net interfaces
	closeNetworkInterfaces();

	if( directConnect ) {
		// resolve host's address
		printlog("resolving host's address at %s:%d...\n", address, portnumber);
		if(SDLNet_ResolveHost(&net_server, address, portnumber) == -1) {
			printlog( "warning: resolving host at %s:%d has failed.\n", address, portnumber);
			openFailedConnectionWindow(CLIENT);
			return;
		}
		
		// open sockets
		printlog("opening TCP and UDP sockets...\n");
		if(!(net_sock = SDLNet_UDP_Open(0))) {
			printlog( "warning: SDLNet_UDP_open has failed.\n");
			openFailedConnectionWindow(CLIENT);
			return;
		}
		if(!(net_tcpsock = SDLNet_TCP_Open(&net_server))) {
			printlog( "warning: SDLNet_TCP_open has failed.\n");
			openFailedConnectionWindow(CLIENT);
			return;
		}
		tcpset = SDLNet_AllocSocketSet(4);
		SDLNet_TCP_AddSocket(tcpset, net_tcpsock);
	}
		
	// allocate packet data
	if(!(net_packet = SDLNet_AllocPacket(NET_PACKET_SIZE))) {
		printlog( "warning: packet allocation failed.\n");
		openFailedConnectionWindow(CLIENT);
		return;
	}

	if( directConnect )
		printlog( "successfully contacted server at %s:%d.\n", address, portnumber);

	printlog( "submitting join request...\n");
	
	// send join request
	strcpy((char *)net_packet->data,"BARONY_JOIN_REQUEST");
	if( loadingsavegame ) {
		strncpy((char *)net_packet->data+19,stats[getSaveGameClientnum()].name,22);
		SDLNet_Write32((Uint32)client_classes[getSaveGameClientnum()],&net_packet->data[42]);
		SDLNet_Write32((Uint32)stats[getSaveGameClientnum()].sex,&net_packet->data[46]);
		SDLNet_Write32((Uint32)stats[getSaveGameClientnum()].appearance,&net_packet->data[50]);
		strcpy((char *)net_packet->data+54,VERSION);
		net_packet->data[62] = 0;
		net_packet->data[63] = getSaveGameClientnum();
	} else {
		strncpy((char *)net_packet->data+19,stats[0].name,22);
		SDLNet_Write32((Uint32)client_classes[0],&net_packet->data[42]);
		SDLNet_Write32((Uint32)stats[0].sex,&net_packet->data[46]);
		SDLNet_Write32((Uint32)stats[0].appearance,&net_packet->data[50]);
		strcpy((char *)net_packet->data+54,VERSION);
		net_packet->data[62] = 0;
		net_packet->data[63] = 0;
	}
	if( loadingsavegame ) {
		// send over the map seed being used
		SDLNet_Write32(getSaveGameMapSeed(),&net_packet->data[64]);
	} else {
		SDLNet_Write32(0,&net_packet->data[64]);
	}
	SDLNet_Write32(loadingsavegame,&net_packet->data[68]); // send unique game key
	net_packet->address.host = net_server.host;
	net_packet->address.port = net_server.port;
	net_packet->len = 72;
	if( !directConnect ) {
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
	} else {
		sendPacket(net_sock, -1, net_packet, 0);
	}
}

// starts the game as server
void buttonStartServer(button_t *my) {
	int c;
	
	// close window
	buttonCloseSubwindow(my);

	multiplayer = SERVER;
	intro=TRUE;
	introstage=3;
	numplayers=0;
	fadeout=TRUE;

	// send the ok to start
	for( c=1; c<MAXPLAYERS; c++ ) {
		if( !client_disconnected[c] ) {
			if( !loadingsavegame || !intro ) {
				clearStats(&stats[c]);
				initClass(c);
			} else {
				loadGame(c);
			}
		}
	}
	uniqueGameKey = prng_get_uint();
	if( !uniqueGameKey )
		uniqueGameKey++;
	for( c=1; c<MAXPLAYERS; c++ ) {
		if( client_disconnected[c] )
			continue;
		strcpy((char *)net_packet->data,"BARONY_GAME_START");
		SDLNet_Write32(svFlags,&net_packet->data[17]);
		SDLNet_Write32(uniqueGameKey,&net_packet->data[21]);
		net_packet->address.host = net_clients[c-1].host;
		net_packet->address.port = net_clients[c-1].port;
		net_packet->len = 25;
		sendPacketSafe(net_sock, -1, net_packet, c-1);
	}
}

// opens the steam dialog to invite friends
#ifdef STEAMWORKS
void buttonInviteFriends(button_t *my) {
	if (SteamUser()->BLoggedOn())
		SteamFriends()->ActivateGameOverlayInviteDialog(*static_cast<CSteamID*>(currentLobby));
	return;
}
#endif

// disconnects from whatever lobby the game is connected to
void buttonDisconnect(button_t *my) {
	int c;
	
	if( multiplayer==SERVER ) {
		// send disconnect message to clients
		for( c=1; c<MAXPLAYERS; c++ ) {
			if( client_disconnected[c] )
				continue;
			strcpy((char *)net_packet->data,"PLAYERDISCONNECT");
			net_packet->data[16] = clientnum;
			net_packet->address.host = net_clients[c-1].host;
			net_packet->address.port = net_clients[c-1].port;
			net_packet->len = 17;
			sendPacketSafe(net_sock, -1, net_packet, c-1);
		}
	} else {
		// send disconnect message to server
		strcpy((char *)net_packet->data,"PLAYERDISCONNECT");
		net_packet->data[16] = clientnum;
		net_packet->address.host = net_server.host;
		net_packet->address.port = net_server.port;
		net_packet->len = 17;
		sendPacketSafe(net_sock, -1, net_packet, 0);
	}
	
	// reset multiplayer status
	multiplayer = SINGLE;
	stats[0].sex = stats[clientnum].sex;
	client_classes[0] = client_classes[clientnum];
	strcpy(stats[0].name,stats[clientnum].name);
	clientnum = 0;
	client_disconnected[0]=FALSE;
	for( c=1; c<MAXPLAYERS; c++ ) {
		client_disconnected[c]=TRUE;
	}

	// close any existing net interfaces
	closeNetworkInterfaces();
	#ifdef STEAMWORKS
	if( currentLobby ) {
		SteamMatchmaking()->LeaveLobby(*static_cast<CSteamID*>(currentLobby));
		cpp_Free_CSteamID(currentLobby); //TODO: Bugger this.
		currentLobby = NULL;
	}
	#endif
	
	// close lobby window
	buttonCloseSubwindow(my);
}

// open the video tab in the settings window
void buttonVideoTab(button_t *my) {
	settings_tab=0;
}

// open the audio tab in the settings window
void buttonAudioTab(button_t *my) {
	settings_tab=1;
}

// open the keyboard tab in the settings window
void buttonKeyboardTab(button_t *my) {
	settings_tab=2;
}

// open the mouse tab in the settings window
void buttonMouseTab(button_t *my) {
	settings_tab=3;
}

// open the misc tab in the settings window
void buttonMiscTab(button_t *my) {
	settings_tab=4;
}

// settings accept button
void buttonSettingsAccept(button_t *my) {
	int c;

	// set video options
	fov = settings_fov;
	smoothlighting = settings_smoothlighting;
	fullscreen = settings_fullscreen;
	shaking = settings_shaking;
	bobbing = settings_bobbing;
	spawn_blood = settings_spawn_blood;
	colorblind = settings_colorblind;
	vidgamma = settings_gamma;
	xres = settings_xres;
	yres = settings_yres;
	camera.winx = 0;
	camera.winy = 0;
	camera.winw = std::min(camera.winw,xres);
	camera.winh = std::min(camera.winh,yres);
	if( !changeVideoMode() ) {
		printlog("critical error! Attempting to abort safely...\n");
		mainloop=0;
	}
	if( zbuffer != NULL )
		free(zbuffer);
	zbuffer=(double *) malloc(sizeof(double)*xres*yres);
	if( clickmap != NULL )
		free(clickmap);
	clickmap=(Entity **) malloc(sizeof(Entity *)*xres*yres);
	
	// set audio options
	sfxvolume = settings_sfxvolume;
	musvolume = settings_musvolume;
	FMOD_ChannelGroup_SetVolume(music_group, musvolume / 128.f);
	FMOD_ChannelGroup_SetVolume(sound_group, sfxvolume / 128.f);
	
	// set keyboard options
	for( c=0; c<NUMIMPULSES; c++ ) {
		impulses[c] = settings_impulses[c];
	}

	// set mouse options
	reversemouse = settings_reversemouse;
	smoothmouse = settings_smoothmouse;
	mousespeed = settings_mousespeed;

	// set misc options
	broadcast = settings_broadcast;
	nohud = settings_nohud;

	auto_hotbar_new_items = settings_auto_hotbar_new_items;
	disable_messages = settings_disable_messages;

	// we need to reposition the settings window now.
	buttonCloseSubwindow(my);
	list_FreeAll(&button_l);
	deleteallbuttons=TRUE;
	openSettingsWindow();
	saveConfig("default.cfg");
}

// settings okay button
void buttonSettingsOK(button_t *my) {
	buttonSettingsAccept(my);
	buttonCloseSubwindow(my);
}

// next score button (statistics window)
void buttonScoreNext(button_t *my) {
	score_window = std::min<int>(score_window+1,std::max<Uint32>(1,list_Size(&topscores)));
	loadScore(score_window-1);
}

// previous score button (statistics window)
void buttonScorePrev(button_t *my) {
	score_window = std::max(score_window-1,1);
	loadScore(score_window-1);
}

// handles slider
void doSlider(int x, int y, int dots, int minvalue, int maxvalue, int increment, int *var) {
	int c;

	// build bar
	strcpy(tempstr,"| ");
	for( c=0; c<dots; c++ ) {
		strcat(tempstr,". ");
	}
	strcat(tempstr,"| %d");
	printTextFormatted(SLIDERFONT, x, y, tempstr, *var);
	
	// control
	int range = maxvalue-minvalue;
	int sliderLength = ((strlen(tempstr)-4)*(SLIDERFONT->w/16));
	if( mousestatus[SDL_BUTTON_LEFT] ) {
		if( omousex >= x && omousex < x+sliderLength+(SLIDERFONT->w/16) ) {
			if( omousey >= y-(SLIDERFONT->h/16)/2 && omousey < y+((SLIDERFONT->h/16)/2)*3 ) {
				*var = ((double)(mousex-x-(SLIDERFONT->w/16)/2)/sliderLength)*range+minvalue;
				if( increment ) {
					*var += increment/2;
					*var /= increment;
					*var *= increment;
				}
				*var = std::min(std::max(minvalue,*var),maxvalue);
			}
		}
	}

	// draw slider
	int sliderx = x+(SLIDERFONT->w/16)/2;
	sliderx += (((double)(*var)-minvalue)/range)*sliderLength;
	drawWindowFancy( sliderx-(SLIDERFONT->w/16)/2, y-(SLIDERFONT->h/16)/2, sliderx+(SLIDERFONT->w/16)/2, y+((SLIDERFONT->h/16)/2)*3);
}

// handles slider (float)
void doSliderF(int x, int y, int dots, double minvalue, double maxvalue, double increment, double *var) {
	int c;

	// build bar
	strcpy(tempstr,"| ");
	for( c=0; c<dots; c++ ) {
		strcat(tempstr,". ");
	}
	strcat(tempstr,"| %.3f");
	printTextFormatted(SLIDERFONT, x, y, tempstr, *var);
	
	// control
	double range = maxvalue-minvalue;
	int sliderLength = ((strlen(tempstr)-6)*(SLIDERFONT->w/16));
	if( mousestatus[SDL_BUTTON_LEFT] ) {
		if( omousex >= x && omousex < x+sliderLength+(SLIDERFONT->w/16) ) {
			if( omousey >= y-(SLIDERFONT->h/16)/2 && omousey < y+((SLIDERFONT->h/16)/2)*3 ) {
				*var = ((double)(mousex-x-(SLIDERFONT->w/16)/2)/sliderLength)*range+minvalue;
				if( increment ) {
					*var += increment/2;
					*var /= increment;
					*var = floor(*var);
					*var *= increment;
				}
				*var = fmin(fmax(minvalue,*var),maxvalue);
			}
		}
	}

	// draw slider
	int sliderx = x+(SLIDERFONT->w/16)/2;
	sliderx += (((*var)-minvalue)/range)*sliderLength;
	drawWindowFancy( sliderx-(SLIDERFONT->w/16)/2, y-(SLIDERFONT->h/16)/2, sliderx+(SLIDERFONT->w/16)/2, y+((SLIDERFONT->h/16)/2)*3);
}

void openLoadGameWindow(button_t *my) {
	button_t *button;
	
	// close current window
	buttonCloseSubwindow(NULL);
	list_FreeAll(&button_l);
	deleteallbuttons=TRUE;

	// create confirmation window
	subwindow = 1;
	subx1 = xres/2-256;
	subx2 = xres/2+256;
	suby1 = yres/2-128;
	suby2 = yres/2+128;
	strcpy(subtext,language[1460]);
	char *saveGameName = getSaveGameName();
	strcat(subtext,saveGameName);
	free(saveGameName);
	strcat(subtext,language[1461]);

	// close button
	button = newButton();
	strcpy(button->label,"x");
	button->x=subx2-20; button->y=suby1;
	button->sizex=20; button->sizey=20;
	button->action=&buttonCloseSubwindow;
	button->visible=1;
	button->focused=1;
	button->key=SDL_SCANCODE_ESCAPE;
						
	// yes button
	button = newButton();
	strcpy(button->label,language[1462]);
	button->sizex=strlen(language[1462])*12+8; button->sizey=20;
	button->x=subx1+(subx2-subx1)/2-button->sizex/2; button->y=suby2-52;
	button->action=&buttonLoadGame;
	button->visible=1;
	button->focused=1;
						
	// no button
	button = newButton();
	strcpy(button->label,language[1463]);
	button->sizex=strlen(language[1463])*12+8; button->sizey=20;
	button->x=subx1+(subx2-subx1)/2-button->sizex/2; button->y=suby2-28;
	button->action=&buttonOpenCharacterCreationWindow;
	button->visible=1;
	button->focused=1;
	button->key=SDL_SCANCODE_RETURN;
}

void buttonOpenCharacterCreationWindow(button_t *my) {
	button_t *button;

	playing_random_char = FALSE;
	loadingsavegame = 0;

	// reset class loadout
	clientnum=0;
	stats[0].sex=static_cast<sex_t>(0);
	stats[0].appearance=0;
	strcpy(stats[0].name,"");
	stats[0].type = HUMAN;
	client_classes[0] = 0;
	clearStats(&stats[0]);
	initClass(0);
	
	// close current window
	if( subwindow ) {
		buttonCloseSubwindow(NULL);
		list_FreeAll(&button_l);
		deleteallbuttons=TRUE;
	}

	// create character creation window
	charcreation_step=1;
	subwindow = 1;
	subx1 = xres/2-400;
	subx2 = xres/2+400;
	suby1 = yres/2-240;
	suby2 = yres/2+240;
	strcpy(subtext,"");

	// close button
	button = newButton();
	strcpy(button->label,"x");
	button->x=subx2-20; button->y=suby1;
	button->sizex=20; button->sizey=20;
	button->action=&buttonCloseSubwindow;
	button->visible=1;
	button->focused=1;
	
	// Continue ...
	button = newButton();
	strcpy(button->label,language[1464]);
	button->sizex=strlen(language[1464])*12+8; button->sizey=20;
	button->x=subx2-button->sizex-4; button->y=suby2-24;
	button->action=&buttonContinue;
	button->visible=1;
	button->focused=1;
	button->key=SDL_SCANCODE_RETURN;
	
	// Back ...
	button = newButton();
	strcpy(button->label,language[1465]);
	button->x=subx1+4; button->y=suby2-24;
	button->sizex=strlen(language[1465])*12+8; button->sizey=20;
	button->action=&buttonBack;
	button->visible=1;
	button->focused=1;
	button->key=SDL_SCANCODE_ESCAPE;
	int button_back_x = button->x;
	int button_back_width = button->sizex;
	
	// Random Character ...
	button = newButton();
	strcpy(button->label,language[1466]);
	button->x=button_back_x+button_back_width+4; button->y=suby2-24;
	button->sizex=strlen(language[1466])*12+8; button->sizey=20;
	button->action=&buttonRandomCharacter;
	button->visible=1;
	button->focused=1;
	button->key=SDL_SCANCODE_R;
}

void buttonLoadGame(button_t *button) {
	loadingsavegame = getSaveGameUniqueGameKey();
	int mul = getSaveGameType();

	if( mul==DIRECTSERVER ) {
		buttonHostMultiplayer(button);
	} else if( mul==DIRECTCLIENT ) {
		buttonJoinMultiplayer(button);
	} else if( mul==SINGLE ) {
		buttonStartSingleplayer(button);
	} else {
		#ifdef STEAMWORKS
		if( mul==SERVER ) {
			buttonHostMultiplayer(button);
		} else if( mul==CLIENT ) {
			if( !lobbyToConnectTo ) {
				openSteamLobbyBrowserWindow(button);
			} else {
				// close current window
				int temp1 = connectingToLobby;
				int temp2 = connectingToLobbyWindow;
				//buttonCloseSubwindow(button);
				list_FreeAll(&button_l);
				deleteallbuttons=TRUE;
				connectingToLobby = temp1;
				connectingToLobbyWindow = temp2;
			
				// create new window
				subwindow = 1;
				subx1 = xres/2-256;
				subx2 = xres/2+256;
				suby1 = yres/2-64;
				suby2 = yres/2+64;
				strcpy(subtext,language[1467]);

				// close button
				button = newButton();
				strcpy(button->label,"x");
				button->x=subx2-20; button->y=suby1;
				button->sizex=20; button->sizey=20;
				button->action=&openSteamLobbyWaitWindow;
				button->visible=1;
				button->focused=1;
				button->key=SDL_SCANCODE_ESCAPE;

				// cancel button
				button = newButton();
				strcpy(button->label,language[1316]);
				button->sizex=strlen(language[1316])*12+8; button->sizey=20;
				button->x=subx1 + (subx2-subx1)/2 - button->sizex/2; button->y=suby2-28;
				button->action=&openSteamLobbyWaitWindow;
				button->visible=1;
				button->focused=1;
			
				connectingToLobby = TRUE;
				connectingToLobbyWindow = TRUE;
				strncpy( currentLobbyName, "", 31 );
				cpp_SteamMatchmaking_JoinLobby(*static_cast<CSteamID* >(lobbyToConnectTo));
				cpp_Free_CSteamID(lobbyToConnectTo);
				lobbyToConnectTo = NULL;
			}
		} else {
			buttonStartSingleplayer(button);
		}
		#endif
	}
}

void buttonRandomCharacter(button_t *my) {
	playing_random_char = TRUE;
	charcreation_step = 4;
	stats[0].sex = static_cast<sex_t>(rand()%2);
	client_classes[0] = rand()%NUMCLASSES;
	clearStats(&stats[0]);
	initClass(0);
	stats[0].appearance = rand()%NUMAPPEARANCES;
}