/*-------------------------------------------------------------------------------

	BARONY
	File: consolecommand.cpp
	Desc: contains consoleCommand()

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/
#include <sstream>
#include "../main.hpp"
#include "../files.hpp"
#include "../game.hpp"
#include "../stat.hpp"
#include "../items.hpp"
#include "../init.hpp"
#include "../book.hpp"
#include "../sound.hpp"
#include "../menu.hpp"
#include "../monster.hpp"
#include "../net.hpp"
#include "../paths.hpp"
#include "../player.hpp"
#include "interface.hpp"
#include "../scores.hpp"

bool spamming = false;
bool showfirst = false;
bool logCheckObstacle = false;
int logCheckObstacleCount = 0;

/*-------------------------------------------------------------------------------

	consoleCommand

	Takes a string and executes it as a game command

-------------------------------------------------------------------------------*/

void consoleCommand(char* command_str)
{
	node_t* node;
	Entity* entity;
	char name[64];
	int c;
	bool invalidcommand = false;

	if ( !command_str )
	{
		return;
	}

	if ( !strncmp(command_str, "/ping", 5) )
	{
		if ( multiplayer != CLIENT )
		{
			messagePlayer(clientnum, language[1117], 0);
		}
		else
		{
			strcpy((char*)net_packet->data, "PING");
			net_packet->data[4] = clientnum;
			net_packet->address.host = net_server.host;
			net_packet->address.port = net_server.port;
			net_packet->len = 5;
			sendPacketSafe(net_sock, -1, net_packet, 0);
			pingtime = SDL_GetTicks();
		}
	}
	else if ( !strncmp(command_str, "/usemodelcache", 14) )
	{
		useModelCache = true;
	}
	else if ( !strncmp(command_str, "/disablemodelcache", 14) )
	{
		useModelCache = false;
	}
	else if (!strncmp(command_str, "/fov", 4))
	{
		fov = atoi(&command_str[5]);
		fov = std::min(std::max<Uint32>(40, fov), 100u);
	}
	else if ( !strncmp(command_str, "/fps", 4) )
	{
		fpsLimit = atoi(&command_str[5]);
		fpsLimit = std::min(std::max<Uint32>(60, fpsLimit), 144u);
	}
	else if (!strncmp(command_str, "/svflags ", 9))
	{
		if ( multiplayer == CLIENT )
		{
			messagePlayer(clientnum, language[275]);
		}
		else
		{
			svFlags = atoi(&command_str[9]);
			messagePlayer(clientnum, language[276]);

			if ( multiplayer == SERVER )
			{
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
					messagePlayer(c, language[276]);
				}
			}
		}
	}
	else if ( !strncmp(command_str, "/lastname ", 10) )
	{
		strcpy(name, command_str + 10);
		lastname = (string)name;
		lastname = lastname.substr(0, lastname.size() - 1);
	}
	else if ( !strncmp(command_str, "/lastcharacter ", 15) )
	{
		int loadedValues = 0;
		for ( c = 0; c < strlen(command_str); ++c )
		{
			if ( command_str[c] == ' ' )
			{
				switch ( loadedValues )
				{
					case 0:
						lastCreatedCharacterSex = atoi(&command_str[c + 1]);
						break;
					case 1:
						lastCreatedCharacterClass = atoi(&command_str[c + 1]);
						break;
					case 2:
						lastCreatedCharacterAppearance = atoi(&command_str[c + 1]);
						break;
					default:
						break;
				}
				++loadedValues;
			}
		}
	}
	else if ( !strncmp(command_str, "/spawnitem ", 11) )
	{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, language[277]);
			return;
		}
		strcpy(name, command_str + 11);
		for ( c = 0; c < NUMITEMS; c++ )
		{
			if ( strstr(items[c].name_identified, name) )
			{
				dropItem(newItem(static_cast<ItemType>(c), EXCELLENT, 0, 1, rand(), true, &stats[clientnum]->inventory), 0);
				break;
			}
		}
		if ( c == NUMITEMS )
		{
			messagePlayer(clientnum, language[278], name);
		}
	}
	else if ( !strncmp(command_str, "/spawncursed ", 13) )
	{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, language[277]);
			return;
		}
		strcpy(name, command_str + 13);
		for ( c = 0; c < NUMITEMS; c++ )
		{
			if ( strstr(items[c].name_identified, name) )
			{
				dropItem(newItem(static_cast<ItemType>(c), WORN, -2, 1, rand(), false, &stats[clientnum]->inventory), 0);
				break;
			}
		}
		if ( c == NUMITEMS )
		{
			messagePlayer(clientnum, language[278], name);
		}
	}
	else if ( !strncmp(command_str, "/spawnblessed ", 14) )
	{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, language[277]);
			return;
		}
		strcpy(name, command_str + 14);
		for ( c = 0; c < NUMITEMS; ++c )
		{
			if ( strstr(items[c].name_identified, name) )
			{
				dropItem(newItem(static_cast<ItemType>(c), WORN, 2, 1, rand(), false, &stats[clientnum]->inventory), 0);
				break;
			}
		}
		if ( c == NUMITEMS )
		{
			messagePlayer(clientnum, language[278], name);
		}
	}
	else if ( !strncmp(command_str, "/kick ", 6) )
	{
		strcpy(name, command_str + 6);
		if ( multiplayer == SERVER )
		{
			for ( c = 1; c < MAXPLAYERS; c++ )
			{
				if ( !client_disconnected[c] && !strncmp(name, stats[c]->name, 128) )
				{
					client_disconnected[c] = true;
					strcpy((char*)net_packet->data, "KICK");
					net_packet->address.host = net_clients[c - 1].host;
					net_packet->address.port = net_clients[c - 1].port;
					net_packet->len = 4;
					sendPacketSafe(net_sock, -1, net_packet, c - 1);
					int i;
					for ( i = 0; i < MAXPLAYERS; i++ )
					{
						messagePlayer(i, language[279], c, stats[c]->name);
					}
					break;
				}
			}
			if ( c == MAXPLAYERS )
			{
				messagePlayer(clientnum, language[280]);
			}
		}
		else if ( multiplayer == CLIENT )
		{
			messagePlayer(clientnum, language[281]);
		}
		else
		{
			messagePlayer(clientnum, language[282]);
		}
	}
	else if ( !strncmp(command_str, "/spawnbook ", 11) )
	{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, language[277]);
			return;
		}

		strcpy(name, command_str + 11);
		dropItem(newItem(READABLE_BOOK, EXCELLENT, 0, 1, getBook(name), true, &stats[clientnum]->inventory), 0);
	}
	else if ( !strncmp(command_str, "/savemap ", 9) )
	{
		if ( command_str[9] != 0 )
		{
			saveMap(command_str + 9);
			messagePlayer(clientnum, language[283], command_str + 9);
		}
	}
	else if ( !strncmp(command_str, "/nextlevel", 10) )
	{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, language[277]);
			return;
		}
		if ( multiplayer == CLIENT )
		{
			messagePlayer(clientnum, language[284]);
		}
		else
		{
			messagePlayer(clientnum, language[285]);
			loadnextlevel = true;
		}
	}
	else if ( !strncmp(command_str, "/pos", 4) )
	{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, language[277]);
			return;
		}
		messagePlayer(clientnum, language[286], (int)camera.x, (int)camera.y, (int)camera.z, camera.ang, camera.vang);
	}
	else if ( !strncmp(command_str, "/pathmap", 4) )
	{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, language[277]);
			return;
		}
		if (players[clientnum] && players[clientnum]->entity)
		{
			int x = std::min<int>(std::max<int>(0, floor(players[clientnum]->entity->x / 16)), map.width - 1);
			int y = std::min<int>(std::max<int>(0, floor(players[clientnum]->entity->y / 16)), map.height - 1);
			messagePlayer(clientnum, "pathMapGrounded value: %d", pathMapGrounded[y + x * map.height]);
			messagePlayer(clientnum, "pathMapFlying value: %d", pathMapFlying[y + x * map.height]);
		}
	}
	else if ( !strncmp(command_str, "/exit", 5) )
	{
		mainloop = 0;
	}
	else if ( !strncmp(command_str, "/showfps", 8) )
	{
		showfps = (showfps == false);
	}
	else if ( !strncmp(command_str, "/noclip", 7) )
	{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, language[277]);
			return;
		}
		if ( multiplayer != SINGLE )
		{
			messagePlayer(clientnum, language[287]);
		}
		else
		{
			noclip = (noclip == false);
			if ( noclip )
			{
				messagePlayer(clientnum, language[288]);
			}
			else
			{
				messagePlayer(clientnum, language[289]);
			}
		}
	}
	else if ( !strncmp(command_str, "/god", 4) )
	{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, language[277]);
			return;
		}
		if ( multiplayer != SINGLE )
		{
			messagePlayer(clientnum, language[290]);
		}
		else
		{
			godmode = (godmode == false);
			if ( godmode )
			{
				messagePlayer(clientnum, language[291]);
			}
			else
			{
				messagePlayer(clientnum, language[292]);
			}
		}
	}
	else if ( !strncmp(command_str, "/spam", 5) )
	{
		spamming = !(spamming);
	}
	else if ( !strncmp(command_str, "/logobstacle", 12) )
	{
		logCheckObstacle = !(logCheckObstacle);
	}
	else if ( !strncmp(command_str, "/showfirst", 10) )
	{
		showfirst = !(showfirst);
	}
	else if ( !strncmp(command_str, "/buddha", 7) )
	{
		if ( multiplayer != SINGLE )
		{
			messagePlayer(clientnum, language[293]);
		}
		else
		{
			buddhamode = (buddhamode == false);
			if ( buddhamode )
			{
				messagePlayer(clientnum, language[294]);
			}
			else
			{
				messagePlayer(clientnum, language[295]);
			}
		}
	}
	else if ( !strncmp(command_str, "/friendly", 9) )
	{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, language[277]);
			return;
		}
		if ( multiplayer == CLIENT )
		{
			messagePlayer(clientnum, language[284]);
			return;
		}
		everybodyfriendly = (everybodyfriendly == false);
		if ( everybodyfriendly )
		{
			messagePlayer(clientnum, language[296]);
		}
		else
		{
			messagePlayer(clientnum, language[297]);
		}
	}
	else if ( !strncmp(command_str, "/dowse", 6) )
	{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, language[277]);
			return;
		}
		for ( node = map.entities->first; node != NULL; node = node->next )
		{
			entity = (Entity*)node->element;
			if ( entity->behavior == &actLadder )
			{
				messagePlayer(clientnum, language[298], (int)(entity->x / 16), (int)(entity->y / 16));
			}
		}
	}
	else if (!strncmp(command_str, "/thirdperson", 12))
	{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, language[277]);
			return;
		}
		if (players[clientnum] != nullptr && players[clientnum]->entity != nullptr)
		{
			players[clientnum]->entity->skill[3] = (players[clientnum]->entity->skill[3] == 0);
			if (players[clientnum]->entity->skill[3] == 1)
			{
				messagePlayer(clientnum, "thirdperson ON");
			}
			else
			{
				messagePlayer(clientnum, "thirdperson OFF");
			}
		}
	}
	else if ( !strncmp(command_str, "/res ", 5) )
	{
		xres = atoi(&command_str[5]);
		for ( c = 0; c < strlen(command_str); c++ )
		{
			if ( command_str[c] == 'x' )
			{
				yres = atoi(&command_str[c + 1]);
				break;
			}
		}
	}
	else if ( !strncmp(command_str, "/rscale", 7) )
	{
		rscale = atoi(&command_str[8]);
	}
	else if ( !strncmp(command_str, "/smoothlighting", 15) )
	{
		smoothlighting = (smoothlighting == 0);
	}
	else if ( !strncmp(command_str, "/fullscreen", 11) )
	{
		fullscreen = (fullscreen == 0);
	}
	else if ( !strncmp(command_str, "/shaking", 8) )
	{
		shaking = (shaking == 0);
	}
	else if ( !strncmp(command_str, "/bobbing", 8) )
	{
		bobbing = (bobbing == 0);
	}
	else if ( !strncmp(command_str, "/sfxvolume", 10) )
	{
		sfxvolume = atoi(&command_str[11]);
	}
	else if ( !strncmp(command_str, "/musvolume", 10) )
	{
		musvolume = atoi(&command_str[11]);
	}
	else if ( !strncmp(command_str, "/bind", 5) )
	{
		if ( strstr(command_str, "IN_FORWARD") )
		{
			impulses[IN_FORWARD] = atoi(&command_str[6]);
			printlog("Bound IN_FORWARD: %d\n", atoi(&command_str[6]));
		}
		else if ( strstr(command_str, "IN_LEFT") )
		{
			impulses[IN_LEFT] = atoi(&command_str[6]);
			printlog("Bound IN_LEFT: %d\n", atoi(&command_str[6]));
		}
		else if ( strstr(command_str, "IN_BACK") )
		{
			impulses[IN_BACK] = atoi(&command_str[6]);
			printlog("Bound IN_BACK: %d\n", atoi(&command_str[6]));
		}
		else if ( strstr(command_str, "IN_RIGHT") )
		{
			impulses[IN_RIGHT] = atoi(&command_str[6]);
			printlog("Bound IN_RIGHT: %d\n", atoi(&command_str[6]));
		}
		else if ( strstr(command_str, "IN_TURNL") )
		{
			impulses[IN_TURNL] = atoi(&command_str[6]);
			printlog("Bound IN_TURNL: %d\n", atoi(&command_str[6]));
		}
		else if ( strstr(command_str, "IN_TURNR") )
		{
			impulses[IN_TURNR] = atoi(&command_str[6]);
			printlog("Bound IN_TURNR: %d\n", atoi(&command_str[6]));
		}
		else if ( strstr(command_str, "IN_UP") )
		{
			impulses[IN_UP] = atoi(&command_str[6]);
			printlog("Bound IN_UP: %d\n", atoi(&command_str[6]));
		}
		else if ( strstr(command_str, "IN_DOWN") )
		{
			impulses[IN_DOWN] = atoi(&command_str[6]);
			printlog("Bound IN_DOWN: %d\n", atoi(&command_str[6]));
		}
		else if ( strstr(command_str, "IN_CHAT") )
		{
			impulses[IN_CHAT] = atoi(&command_str[6]);
			printlog("Bound IN_CHAT: %d\n", atoi(&command_str[6]));
		}
		else if ( strstr(command_str, "IN_COMMAND") )
		{
			impulses[IN_COMMAND] = atoi(&command_str[6]);
			printlog("Bound IN_COMMAND: %d\n", atoi(&command_str[6]));
		}
		else if ( strstr(command_str, "IN_STATUS") )
		{
			impulses[IN_STATUS] = atoi(&command_str[6]);
			printlog("Bound IN_STATUS: %d\n", atoi(&command_str[6]));
		}
		else if (strstr(command_str, "IN_SPELL_LIST"))
		{
			impulses[IN_SPELL_LIST] = atoi(&command_str[6]);
			printlog( "Bound IN_SPELL_LIST: %d\n", atoi(&command_str[6]));
		}
		else if (strstr(command_str, "IN_CAST_SPELL"))
		{
			impulses[IN_CAST_SPELL] = atoi(&command_str[6]);
			printlog( "Bound IN_CAST_SPELL: %d\n", atoi(&command_str[6]));
		}
		else if (strstr(command_str, "IN_DEFEND"))
		{
			impulses[IN_DEFEND] = atoi(&command_str[6]);
			printlog( "Bound IN_DEFEND: %d\n", atoi(&command_str[6]));
		}
		else if (strstr(command_str, "IN_ATTACK"))
		{
			impulses[IN_ATTACK] = atoi(&command_str[6]);
			printlog( "Bound IN_ATTACK: %d\n", atoi(&command_str[6]));
		}
		else if (strstr(command_str, "IN_USE"))
		{
			impulses[IN_USE] = atoi(&command_str[6]);
			printlog( "Bound IN_USE: %d\n", atoi(&command_str[6]));
		}
		else if ( strstr(command_str, "IN_AUTOSORT") )
		{
			impulses[IN_AUTOSORT] = atoi(&command_str[6]);
			printlog("Bound IN_AUTOSORT: %d\n", atoi(&command_str[6]));
		}
		else if ( strstr(command_str, "IN_MINIMAPSCALE") )
		{
			impulses[IN_MINIMAPSCALE] = atoi(&command_str[6]);
			printlog("Bound IN_MINIMAPSCALE: %d\n", atoi(&command_str[6]));
		}
		else if ( strstr(command_str, "IN_TOGGLECHATLOG") )
		{
			impulses[IN_TOGGLECHATLOG] = atoi(&command_str[6]);
			printlog("Bound IN_TOGGLECHATLOG: %d\n", atoi(&command_str[6]));
		}
		else if ( strstr(command_str, "IN_FOLLOWERMENU_OPEN") )
		{
			impulses[IN_FOLLOWERMENU] = atoi(&command_str[6]);
			printlog("Bound IN_FOLLOWERMENU_OPEN: %d\n", impulses[IN_FOLLOWERMENU]);
		}
		else if ( strstr(command_str, "IN_FOLLOWERMENU_LASTCMD") )
		{
			impulses[IN_FOLLOWERMENU_LASTCMD] = atoi(&command_str[6]);
			printlog("Bound IN_FOLLOWERMENU_LASTCMD: %d\n", impulses[IN_FOLLOWERMENU_LASTCMD]);
		}
		else if ( strstr(command_str, "IN_FOLLOWERMENU_CYCLENEXT") )
		{
			impulses[IN_FOLLOWERMENU_CYCLENEXT] = atoi(&command_str[6]);
			printlog("Bound IN_FOLLOWERMENU_CYCLENEXT: %d\n", impulses[IN_FOLLOWERMENU_CYCLENEXT]);
		}
		else
		{
			messagePlayer(clientnum, "Invalid binding.");
		}
	}
	else if (!strncmp(command_str, "/joybind", 8))
	{
		if (strstr(command_str, "INJOY_STATUS"))
		{
			joyimpulses[INJOY_STATUS] = atoi(&command_str[9]);
			printlog("[GAMEPAD] Bound INJOY_STATUS: %d\n", atoi(&command_str[9]));
		}
		else if (strstr(command_str, "INJOY_SPELL_LIST"))
		{
			joyimpulses[INJOY_SPELL_LIST] = atoi(&command_str[9]);
			printlog("[GAMEPAD] Bound INJOY_SPELL_LIST: %d\n", atoi(&command_str[9]));
		}
		else if (strstr(command_str, "INJOY_GAME_CAST_SPELL"))
		{
			joyimpulses[INJOY_GAME_CAST_SPELL] = atoi(&command_str[9]);
			printlog("[GAMEPAD] Bound INJOY_GAME_CAST_SPELL: %d\n", atoi(&command_str[9]));
		}
		else if (strstr(command_str, "INJOY_GAME_DEFEND"))
		{
			joyimpulses[INJOY_GAME_DEFEND] = atoi(&command_str[9]);
			printlog("[GAMEPAD] Bound INJOY_GAME_DEFEND: %d\n", atoi(&command_str[9]));
		}
		else if (strstr(command_str, "INJOY_GAME_ATTACK"))
		{
			joyimpulses[INJOY_GAME_ATTACK] = atoi(&command_str[9]);
			printlog("[GAMEPAD] Bound INJOY_GAME_ATTACK: %d\n", atoi(&command_str[9]));
		}
		else if (strstr(command_str, "INJOY_GAME_USE"))
		{
			joyimpulses[INJOY_GAME_USE] = atoi(&command_str[9]);
			printlog("[GAMEPAD] Bound INJOY_GAME_USE: %d\n", atoi(&command_str[9]));
		}
		else if (strstr(command_str, "INJOY_MENU_USE"))
		{
			joyimpulses[INJOY_MENU_USE] = atoi(&command_str[9]);
			printlog("[GAMEPAD] Bound INJOY_MENU_USE: %d\n", atoi(&command_str[9]));
		}
		else if (strstr(command_str, "INJOY_PAUSE_MENU"))
		{
			joyimpulses[INJOY_PAUSE_MENU] = atoi(&command_str[9]);
			printlog("[GAMEPAD] Bound INJOY_PAUSE_MENU: %d\n", atoi(&command_str[9]));
		}
		else if (strstr(command_str, "INJOY_MENU_LEFT_CLICK"))
		{
			joyimpulses[INJOY_MENU_LEFT_CLICK] = atoi(&command_str[9]);
			printlog("[GAMEPAD] Bound INJOY_MENU_LEFT_CLICK: %d\n", atoi(&command_str[9]));
		}
		else if (strstr(command_str, "INJOY_DPAD_LEFT"))
		{
			joyimpulses[INJOY_DPAD_LEFT] = atoi(&command_str[9]);
			printlog("[GAMEPAD] Bound INJOY_DPAD_LEFT: %d\n", atoi(&command_str[9]));
		}
		else if (strstr(command_str, "INJOY_DPAD_RIGHT"))
		{
			joyimpulses[INJOY_DPAD_RIGHT] = atoi(&command_str[9]);
			printlog("[GAMEPAD] Bound INJOY_DPAD_RIGHT: %d\n", atoi(&command_str[9]));
		}
		else if (strstr(command_str, "INJOY_DPAD_UP"))
		{
			joyimpulses[INJOY_DPAD_UP] = atoi(&command_str[9]);
			printlog("[GAMEPAD] Bound INJOY_DPAD_UP: %d\n", atoi(&command_str[9]));
		}
		else if (strstr(command_str, "INJOY_DPAD_DOWN"))
		{
			joyimpulses[INJOY_DPAD_DOWN] = atoi(&command_str[9]);
			printlog("[GAMEPAD] Bound INJOY_DPAD_DOWN: %d\n", atoi(&command_str[9]));
		}
		else if (strstr(command_str, "INJOY_MENU_NEXT"))
		{
			joyimpulses[INJOY_MENU_NEXT] = atoi(&command_str[9]);
			printlog("[GAMEPAD] Bound INJOY_MENU_NEXT: %d\n", atoi(&command_str[9]));
		}
		else if (strstr(command_str, "INJOY_GAME_HOTBAR_NEXT"))
		{
			joyimpulses[INJOY_GAME_HOTBAR_NEXT] = atoi(&command_str[9]);
			printlog("[GAMEPAD] Bound INJOY_GAME_HOTBAR_NEXT: %d\n", atoi(&command_str[9]));
		}
		else if (strstr(command_str, "INJOY_GAME_HOTBAR_PREV"))
		{
			joyimpulses[INJOY_GAME_HOTBAR_PREV] = atoi(&command_str[9]);
			printlog("[GAMEPAD] Bound INJOY_GAME_HOTBAR_PREV: %d\n", atoi(&command_str[9]));
		}
		else if (strstr(command_str, "INJOY_GAME_HOTBAR_ACTIVATE"))
		{
			joyimpulses[INJOY_GAME_HOTBAR_ACTIVATE] = atoi(&command_str[9]);
			printlog("[GAMEPAD] Bound INJOY_GAME_HOTBAR_ACTIVATE: %d\n", atoi(&command_str[9]));
		}
		else if ( strstr(command_str, "INJOY_GAME_HOTBAR_ACTIVATE") )
		{
			joyimpulses[INJOY_GAME_HOTBAR_ACTIVATE] = atoi(&command_str[9]);
			printlog("[GAMEPAD] Bound INJOY_GAME_HOTBAR_ACTIVATE: %d\n", atoi(&command_str[9]));
		}
		else if ( strstr(command_str, "INJOY_GAME_GAME_MINIMAPSCALE") )
		{
			joyimpulses[INJOY_GAME_MINIMAPSCALE] = atoi(&command_str[9]);
			printlog("[GAMEPAD] Bound INJOY_GAME_GAME_MINIMAPSCALE: %d\n", atoi(&command_str[9]));
		}
		else if ( strstr(command_str, "INJOY_GAME_GAME_TOGGLECHATLOG") )
		{
			joyimpulses[INJOY_GAME_TOGGLECHATLOG] = atoi(&command_str[9]);
			printlog("[GAMEPAD] Bound INJOY_GAME_GAME_TOGGLECHATLOG: %d\n", atoi(&command_str[9]));
		}
		else if ( strstr(command_str, "INJOY_GAME_GAME_FOLLOWERMENU_OPEN") )
		{
			joyimpulses[INJOY_GAME_FOLLOWERMENU] = atoi(&command_str[9]);
			printlog("[GAMEPAD] Bound INJOY_GAME_GAME_FOLLOWERMENU_OPEN: %d\n", atoi(&command_str[9]));
		}
		else if ( strstr(command_str, "INJOY_GAME_GAME_FOLLOWERMENU_LASTCMD") )
		{
			joyimpulses[INJOY_GAME_FOLLOWERMENU_LASTCMD] = atoi(&command_str[9]);
			printlog("[GAMEPAD] Bound INJOY_GAME_GAME_FOLLOWERMENU_LASTCMD: %d\n", atoi(&command_str[9]));
		}
		else if ( strstr(command_str, "INJOY_GAME_GAME_FOLLOWERMENU_CYCLENEXT") )
		{
			joyimpulses[INJOY_GAME_FOLLOWERMENU_CYCLE] = atoi(&command_str[9]);
			printlog("[GAMEPAD] Bound INJOY_GAME_GAME_FOLLOWERMENU_CYCLENEXT: %d\n", atoi(&command_str[9]));
		}
		else if ( strstr(command_str, "INJOY_MENU_CHEST_GRAB_ALL"))
		{
			joyimpulses[INJOY_MENU_CHEST_GRAB_ALL] = atoi(&command_str[9]);
			printlog("[GAMEPAD] Bound INJOY_MENU_CHEST_GRAB_ALL: %d\n", atoi(&command_str[9]));
		}
		else if (strstr(command_str, "INJOY_MENU_HOTBAR_CLEAR"))
		{
			joyimpulses[INJOY_MENU_HOTBAR_CLEAR] = atoi(&command_str[9]);
			printlog("[GAMEPAD] Bound INJOY_MENU_HOTBAR_CLEAR: %d\n", atoi(&command_str[9]));
		}
		else if (strstr(command_str, "INJOY_MENU_REFRESH_LOBBY"))
		{
			joyimpulses[INJOY_MENU_REFRESH_LOBBY] = atoi(&command_str[9]);
			printlog("[GAMEPAD] Bound INJOY_MENU_REFRESH_LOBBY: %d\n", atoi(&command_str[9]));
		}
		else if (strstr(command_str, "INJOY_MENU_DONT_LOAD_SAVE"))
		{
			joyimpulses[INJOY_MENU_DONT_LOAD_SAVE] = atoi(&command_str[9]);
			printlog("[GAMEPAD] Bound INJOY_MENU_DONT_LOAD_SAVE: %d\n", atoi(&command_str[9]));
		}
		else if (strstr(command_str, "INJOY_MENU_RANDOM_CHAR"))
		{
			joyimpulses[INJOY_MENU_RANDOM_CHAR] = atoi(&command_str[9]);
			printlog("[GAMEPAD] Bound INJOY_MENU_RANDOM_CHAR: %d\n", atoi(&command_str[9]));
		}
		else if (strstr(command_str, "INJOY_MENU_CANCEL"))
		{
			joyimpulses[INJOY_MENU_CANCEL] = atoi(&command_str[9]);
			printlog("[GAMEPAD] Bound INJOY_MENU_CANCEL: %d\n", atoi(&command_str[9]));
		}
		else if (strstr(command_str, "INJOY_MENU_DROP_ITEM"))
		{
			joyimpulses[INJOY_MENU_DROP_ITEM] = atoi(&command_str[9]);
			printlog("[GAMEPAD] Bound INJOY_MENU_DROP_ITEM: %d\n", atoi(&command_str[9]));
		}
		else if (strstr(command_str, "INJOY_MENU_CYCLE_SHOP_LEFT"))
		{
			joyimpulses[INJOY_MENU_CYCLE_SHOP_LEFT] = atoi(&command_str[9]);
			printlog("[GAMEPAD] Bound INJOY_MENU_CYCLE_SHOP_LEFT: %d\n", atoi(&command_str[9]));
		}
		else if (strstr(command_str, "INJOY_MENU_CYCLE_SHOP_RIGHT"))
		{
			joyimpulses[INJOY_MENU_CYCLE_SHOP_RIGHT] = atoi(&command_str[9]);
			printlog("[GAMEPAD] Bound INJOY_MENU_CYCLE_SHOP_RIGHT: %d\n", atoi(&command_str[9]));
		}
		else if (strstr(command_str, "INJOY_MENU_BOOK_NEXT"))
		{
			joyimpulses[INJOY_MENU_BOOK_NEXT] = atoi(&command_str[9]);
			printlog("[GAMEPAD] Bound INJOY_MENU_BOOK_NEXT: %d\n", atoi(&command_str[9]));
		}
		else if (strstr(command_str, "INJOY_MENU_BOOK_PREV"))
		{
			joyimpulses[INJOY_MENU_BOOK_PREV] = atoi(&command_str[9]);
			printlog("[GAMEPAD] Bound INJOY_MENU_BOOK_PREV: %d\n", atoi(&command_str[9]));
		}
		else if (strstr(command_str, "INJOY_MENU_SETTINGS_NEXT"))
		{
			joyimpulses[INJOY_MENU_SETTINGS_NEXT] = atoi(&command_str[9]);
			printlog("[GAMEPAD] Bound INJOY_MENU_SETTINGS_NEXT: %d\n", atoi(&command_str[9]));
		}
		else if (strstr(command_str, "INJOY_MENU_SETTINGS_PREV"))
		{
			joyimpulses[INJOY_MENU_SETTINGS_PREV] = atoi(&command_str[9]);
			printlog("[GAMEPAD] Bound INJOY_MENU_SETTINGS_PREV: %d\n", atoi(&command_str[9]));
		}
		else if (strstr(command_str, "INJOY_MENU_INVENTORY_TAB"))
		{
			joyimpulses[INJOY_MENU_INVENTORY_TAB] = atoi(&command_str[9]);
			printlog("[GAMEPAD] Bound INJOY_MENU_INVENTORY_TAB: %d\n", atoi(&command_str[9]));
		}
		else if (strstr(command_str, "INJOY_MENU_MAGIC_TAB"))
		{
			joyimpulses[INJOY_MENU_MAGIC_TAB] = atoi(&command_str[9]);
			printlog("[GAMEPAD] Bound INJOY_MENU_MAGIC_TAB: %d\n", atoi(&command_str[9]));
		}
		else if ( strstr(command_str, "INJOY_MENU_RANDOM_NAME") )
		{
			joyimpulses[INJOY_MENU_RANDOM_NAME] = atoi(&command_str[9]);
			printlog("[GAMEPAD] Bound INJOY_MENU_RANDOM_NAME: %d\n", atoi(&command_str[9]));
		}
		else
		{
			messagePlayer(clientnum, "Invalid binding.");
		}
	}
	else if ( !strncmp(command_str, "/mousespeed", 11) )
	{
		mousespeed = atoi(&command_str[12]);
	}
	else if ( !strncmp(command_str, "/reversemouse", 13) )
	{
		reversemouse = (reversemouse == 0);
	}
	else if ( !strncmp(command_str, "/smoothmouse", 12) )
	{
		smoothmouse = (smoothmouse == false);
	}
	else if ( !strncmp(command_str, "/mana", 4) )
	{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, language[277]);
			return;
		}

		if ( multiplayer == SINGLE )
		{
			stats[clientnum]->MP = stats[clientnum]->MAXMP;
		}
		else
		{
			messagePlayer(clientnum, language[299]);
		}
	}
	else if ( !strncmp(command_str, "/heal", 4) )
	{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, language[277]);
			return;
		}

		if ( multiplayer == SINGLE )
		{
			stats[clientnum]->HP = stats[clientnum]->MAXHP;
		}
		else
		{
			messagePlayer(clientnum, language[299]);
		}
	}
	else if ( !strncmp(command_str, "/damage ", 8) )
	{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, language[277]);
			return;
		}
		if ( multiplayer != SINGLE )
		{
			messagePlayer(clientnum, language[299]);
			return;
		}

		int amount = atoi(&command_str[8]);

		players[clientnum]->entity->modHP(-amount);

		messagePlayer(clientnum, "Damaging you by %d. New health: %d", amount, stats[clientnum]->HP);
	}
	else if (!strncmp(command_str, "/ip ", 4))
	{
		if ( command_str[4] != 0 )
		{
			strcpy(last_ip, command_str + 4);
			last_ip[strlen(last_ip) - 1] = 0;
		}
	}
	else if (!strncmp(command_str, "/port ", 6))
	{
		if (command_str[6] != 0)
		{
			strcpy(last_port, command_str + 6);
			last_port[strlen(last_port) - 1] = 0;
		}
	}
	else if (!strncmp(command_str, "/noblood", 8))
	{
		spawn_blood = (spawn_blood == false);
	}
	else if (!strncmp(command_str, "/nolightflicker", 15))
	{
		flickerLights = (flickerLights == false);
	}
	else if ( !strncmp(command_str, "/vsync", 6) )
	{
		verticalSync = (verticalSync == false);
	}
	else if ( !strncmp(command_str, "/muteping", 9) )
	{
		minimapPingMute = (minimapPingMute == false);
	}
	else if (!strncmp(command_str, "/colorblind", 11))
	{
		colorblind = (colorblind == false);
	}
	else if (!strncmp(command_str, "/gamma", 6))
	{
		std::stringstream ss;
		ss << command_str + 7;
		ss >> vidgamma;
	}
	else if (!strncmp(command_str, "/capturemouse", 13))
	{
		capture_mouse = (capture_mouse == false);
	}
	else if (!strncmp(command_str, "/levelup", 8))
	{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, language[277]);
			return;
		}

		if (multiplayer != CLIENT)
		{
			if (players[clientnum] && players[clientnum]->entity)
			{
				players[clientnum]->entity->getStats()->EXP += 100;
			}
		}
		else if ( multiplayer == CLIENT )
		{
			// request level up
			strcpy((char*)net_packet->data, "CLVL");
			net_packet->data[4] = clientnum;
			net_packet->address.host = net_server.host;
			net_packet->address.port = net_server.port;
			net_packet->len = 5;
			sendPacketSafe(net_sock, -1, net_packet, 0);
			//messagePlayer(clientnum, language[299]);
		}
	}
	else if ( !strncmp(command_str, "/maxout2", 8) )
	{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, language[277]);
			return;
		}

		if ( multiplayer != CLIENT )
		{
			int c;
			Stat* myStats = stats[0];
			for ( c = 0; c < 24; c++ )
			{
				consoleCommand("/levelup");
			}
			for ( c = 0; c < NUM_HOTBAR_SLOTS; c++ )
			{
				hotbar[c].item = 0;
			}
			myStats->weapon = newItem(STEEL_SWORD, SERVICABLE, 0, 1, rand(), true, &myStats->inventory);
			newItem(CROSSBOW, SERVICABLE, 0, 1, rand(), true, &myStats->inventory);
			newItem(MAGICSTAFF_LIGHT, SERVICABLE, 0, 1, rand(), true, &myStats->inventory);
			myStats->shield = newItem(STEEL_SHIELD, SERVICABLE, 0, 1, rand(), true, &myStats->inventory);
			myStats->helmet = newItem(HAT_HOOD, SERVICABLE, 0, 1, 2, true, &myStats->inventory);
			myStats->shoes = newItem(STEEL_BOOTS, SERVICABLE, 0, 1, rand(), true, &myStats->inventory);
			myStats->breastplate = newItem(STEEL_BREASTPIECE, SERVICABLE, 0, 1, rand(), true, &myStats->inventory);
			myStats->gloves = newItem(GAUNTLETS, SERVICABLE, 0, 1, rand(), true, &myStats->inventory);
			myStats->cloak = newItem(CLOAK_BLACK, SERVICABLE, 0, 1, rand(), true, &myStats->inventory);
		}
		else
		{
			for ( int c = 0; c < 24; c++ )
			{
				consoleCommand("/levelup");
			}
			//messagePlayer(clientnum, language[299]);
		}
	}
	else if ( !strncmp(command_str, "/jumplevel ", 11) )
	{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, language[277]);
			return;
		}
		skipLevelsOnLoad = atoi((char*)(command_str + 11));
		consoleCommand("/nextlevel");
	}
	else if ( !strncmp(command_str, "/maxout3", 8) )
	{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, language[277]);
			return;
		}

		if ( multiplayer == SINGLE )
		{
			int c;
			Stat* myStats = stats[0];
			skipLevelsOnLoad = 31;
			for ( c = 0; c < 24; c++ )
			{
				consoleCommand("/levelup");
			}
			for ( c = 0; c < NUM_HOTBAR_SLOTS; c++ )
			{
				hotbar[c].item = 0;
			}
			myStats->weapon = newItem(STEEL_SWORD, SERVICABLE, 0, 1, rand(), true, &myStats->inventory);
			newItem(CROSSBOW, SERVICABLE, 0, 1, rand(), true, &myStats->inventory);
			newItem(MAGICSTAFF_LIGHT, SERVICABLE, 0, 1, rand(), true, &myStats->inventory);
			myStats->shield = newItem(STEEL_SHIELD, SERVICABLE, 0, 1, rand(), true, &myStats->inventory);
			myStats->helmet = newItem(HAT_HOOD, SERVICABLE, 0, 1, 2, true, &myStats->inventory);
			myStats->shoes = newItem(STEEL_BOOTS, SERVICABLE, 0, 1, rand(), true, &myStats->inventory);
			myStats->breastplate = newItem(STEEL_BREASTPIECE, SERVICABLE, 0, 1, rand(), true, &myStats->inventory);
			myStats->gloves = newItem(GAUNTLETS, SERVICABLE, 0, 1, rand(), true, &myStats->inventory);
			myStats->cloak = newItem(CLOAK_BLACK, SERVICABLE, 0, 1, rand(), true, &myStats->inventory);
			consoleCommand("/levelskill 9");
			//consoleCommand("/nextlevel");
			while ( myStats->PROFICIENCIES[PRO_APPRAISAL] < 50 )
			{
				consoleCommand("/levelskill 3");
			}
		}
		else
		{
			messagePlayer(clientnum, language[299]);
		}
	}
	else if ( !strncmp(command_str, "/maxout4", 8) )
	{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, language[277]);
			return;
		}

		if ( multiplayer == SINGLE )
		{
			int c;
			Stat* myStats = stats[0];
			for ( c = 0; c < 35; c++ )
			{
				consoleCommand("/levelup");
			}
			for ( c = 0; c < NUM_HOTBAR_SLOTS; c++ )
			{
				hotbar[c].item = 0;
			}
			myStats->weapon = newItem(STEEL_SWORD, SERVICABLE, 0, 1, rand(), true, &myStats->inventory);
			newItem(CROSSBOW, SERVICABLE, 0, 1, rand(), true, &myStats->inventory);
			newItem(MAGICSTAFF_LIGHT, SERVICABLE, 0, 1, rand(), true, &myStats->inventory);
			myStats->shield = newItem(STEEL_SHIELD, SERVICABLE, 0, 1, rand(), true, &myStats->inventory);
			myStats->helmet = newItem(HAT_HOOD, SERVICABLE, 0, 1, 2, true, &myStats->inventory);
			myStats->shoes = newItem(STEEL_BOOTS, SERVICABLE, 0, 1, rand(), true, &myStats->inventory);
			myStats->breastplate = newItem(STEEL_BREASTPIECE, SERVICABLE, 0, 1, rand(), true, &myStats->inventory);
			myStats->gloves = newItem(GAUNTLETS, SERVICABLE, 0, 1, rand(), true, &myStats->inventory);
			myStats->cloak = newItem(CLOAK_BLACK, SERVICABLE, 0, 1, rand(), true, &myStats->inventory);
			//consoleCommand("/nextlevel");
			for ( c = 0; c < NUMPROFICIENCIES; c++ )
			{
				if ( c != PRO_STEALTH )
				{
					while ( stats[clientnum]->PROFICIENCIES[c] < 100 )
					{
						//++stats[clientnum]->PROFICIENCIES[c];
						players[clientnum]->entity->increaseSkill(c);
					}
				}
			}
		}
		else
		{
			messagePlayer(clientnum, language[299]);
		}
	}
	else if (!strncmp(command_str, "/maxout", 7))
	{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, language[277]);
			return;
		}

		if ( multiplayer == SINGLE )
		{
			int c;
			for ( c = 0; c < 14; c++ )
			{
				consoleCommand("/levelup");
			}
			consoleCommand("/spawnitem steel breastpiece");
			consoleCommand("/spawnitem steel boots");
			consoleCommand("/spawnitem steel gauntlets");
			consoleCommand("/spawnitem steel helm");
			consoleCommand("/spawnitem cloak of magic");
			consoleCommand("/spawnitem steel shield of magic");
			consoleCommand("/spawnitem steel sword");
			consoleCommand("/spawnitem crossbow");
			consoleCommand("/spawnitem magicstaff of lightning");
			for ( c = 0; c < NUMPROFICIENCIES; c++ )
			{
				//for ( int j = 0; j < 100; ++j )
				while ( stats[clientnum]->PROFICIENCIES[c] < 100 )
				{
					//++stats[clientnum]->PROFICIENCIES[c];
					players[clientnum]->entity->increaseSkill(c);
				}
			}
		}
		else
		{
			messagePlayer(clientnum, language[299]);
		}
	}
	else if (!strncmp(command_str, "/hunger", 7))
	{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, language[277]);
			return;
		}

		if ( multiplayer == SINGLE )
		{
			Stat* tempStats = players[clientnum]->entity->getStats();
			if ( tempStats )
			{
				tempStats->HUNGER = std::max(0, tempStats->HUNGER - 100);
			}
		}
		else
		{
			messagePlayer(clientnum, language[299]);
		}
	}
	else if (!strncmp(command_str, "/poison", 7))
	{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, language[277]);
			return;
		}

		if ( multiplayer == SINGLE )
		{
			Stat* tempStats = players[clientnum]->entity->getStats();
			if ( tempStats )
			{
				tempStats->EFFECTS[EFF_POISONED] = true;
				tempStats->EFFECTS_TIMERS[EFF_POISONED] = 600;
			}
		}
		else
		{
			messagePlayer(clientnum, language[299]);
		}
	}
	else if (!strncmp(command_str, "/testsound ", 11))
	{
		int num = 0;
		//snprintf((char *)(command_str + 11), strlen(command_str)-11, "%d", num);
		//printlog( "Number is %d. Original is: \"%s\"\n", num, (char *)(&command_str[11]));
		num = atoi((char*)(command_str + 11));
		playSound(num, 256);
	}
	else if (!strncmp(command_str, "/skipintro", 10))
	{
		skipintro = (skipintro == false);
	}
	else if (!strncmp(command_str, "/levelmagic", 11))
	{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, language[277]);
			return;
		}

		if (multiplayer == SINGLE)
		{
			int i = 0;
			for (; i < 10; ++i)
			{
				players[clientnum]->entity->increaseSkill(PRO_MAGIC);
				players[clientnum]->entity->increaseSkill(PRO_SPELLCASTING);
			}
		}
		else
		{
			messagePlayer(clientnum, language[299]);
		}
	}
	else if (!strncmp(command_str, "/numentities", 12))
	{
		messagePlayer(clientnum, language[300], list_Size(map.entities));
	}
	else if ( !strncmp(command_str, "/nummonsters2", 13) )
	{
		messagePlayer(clientnum, language[2353], list_Size(map.creatures));
	}
	else if ( !strncmp(command_str, "/nummonsters", 12) )
	{
		messagePlayer(clientnum, language[2353], nummonsters);
	}
	else if ( !strncmp(command_str, "/verifycreaturelist", strlen("/verifycreaturelist")) )
	{
		//Make sure that the number of creatures in the creature list are the real count in the game world.
		unsigned entcount = 0;

		for ( node_t* node = map.entities->first; node; node = node->next )
		{
			if ( node->element )
			{
				Entity* ent = static_cast<Entity*>(node->element);
				if ( ent->behavior == actMonster || ent->behavior == actPlayer )
				{
					++entcount;
				}
			}
		}

		messagePlayer(clientnum, "ent count = %d, creatures list size = %d", entcount, list_Size(map.creatures));

		if ( entcount == list_Size(map.creatures) )
		{
			messagePlayer(clientnum, "Yes, list is verified correct.");
		}
		else
		{
			messagePlayer(clientnum, "Nope, much problemo!");
		}
	}
	else if ( !strncmp(command_str, "/loadmodels ", 12) )
	{
		char name2[128];
		char buf[16] = "";
		int startIndex = 0;
		int endIndex = nummodels;
		int i = 0;
		strcpy(name, command_str + 12);
		for ( c = 0; name[c] != '\0'; c++ )
		{
			if ( name[c] == ' ' && startIndex == 0 )
			{
				startIndex = atoi(buf);
				strcpy(buf, "");
				i = 0;
				continue;
			}
			buf[i] = name[c];
			i++;
		}

		if ( startIndex != 0 )
		{
			endIndex = atoi(buf);
			if ( endIndex > nummodels || endIndex < startIndex )
			{
				endIndex = nummodels;
			}
		}

		std::string modelsDirectory = PHYSFS_getRealDir("models/models.txt");
		modelsDirectory.append(PHYSFS_getDirSeparator()).append("models/models.txt");
		FILE *fp = openDataFile(modelsDirectory.c_str(), "r");
		for ( c = 0; !feof(fp); c++ )
		{
			fscanf(fp, "%s", name2);
			while ( fgetc(fp) != '\n' ) if ( feof(fp) )
			{
				break;
			}
			if ( c >= startIndex && c < endIndex )
			{
				if ( models[c] != NULL )
				{
					if ( models[c]->data )
					{
						free(models[c]->data);
					}
					free(models[c]);
					if ( polymodels[c].faces )
					{
						free(polymodels[c].faces);
					}
					if ( polymodels[c].vbo )
					{
						SDL_glDeleteBuffers(1, &polymodels[c].vbo);
					}
					if ( polymodels[c].colors )
					{
						SDL_glDeleteBuffers(1, &polymodels[c].colors);
					}
					if ( polymodels[c].va )
					{
						SDL_glDeleteVertexArrays(1, &polymodels[c].va);
					}
					if ( polymodels[c].colors_shifted )
					{
						SDL_glDeleteBuffers(1, &polymodels[c].colors_shifted);
					}
				}
				models[c] = loadVoxel(name2);
			}
		}
		fclose(fp);
		//messagePlayer(clientnum, language[2354]);
		messagePlayer(clientnum, language[2355], startIndex, endIndex);
		generatePolyModels(startIndex, endIndex, true);
	}
	else if (!strncmp(command_str, "/killmonsters", 13))
	{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, language[277]);
			return;
		}
		if ( multiplayer == CLIENT )
		{
			messagePlayer(clientnum, language[284]);
		}
		else
		{
			int c = 0;
			node_t* node, *nextnode;
			for ( node = map.entities->first; node != NULL; node = nextnode )
			{
				nextnode = node->next;
				Entity* entity = (Entity*)node->element;
				if ( entity->behavior == &actMonster )
				{
					entity->setHP(0);
					c++;
				}
			}
			messagePlayer(clientnum, language[301], c);
		}
	}
	else if (!strncmp(command_str, "/die", 4))
	{
		if ( multiplayer == CLIENT )
		{
			// request sweet release.
			strcpy((char*)net_packet->data, "IDIE");
			net_packet->data[4] = clientnum;
			net_packet->address.host = net_server.host;
			net_packet->address.port = net_server.port;
			net_packet->len = 5;
			sendPacketSafe(net_sock, -1, net_packet, 0);
		}
		else
		{
			players[clientnum]->entity->setHP(0);
		}
	}
	else if (!strncmp(command_str, "/segfault", 9))
	{
		int* potato = NULL;
		(*potato) = 322; //Crash the game!
	}
	else if ( !(strncmp(command_str, "/flames", 7)) )
	{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, language[277]);
			return;
		}

		if ( multiplayer != SINGLE )
		{
			messagePlayer(clientnum, language[299]);
			return;
		}

		// Attempt to set the Player on fire
		players[clientnum]->entity->SetEntityOnFire();

		for ( c = 0; c < 100; c++ )
		{
			entity = spawnFlame(players[clientnum]->entity, SPRITE_FLAME);
			entity->sprite = 16;
			double vel = rand() % 10;
			entity->vel_x = vel * cos(entity->yaw) * cos(entity->pitch) * .1;
			entity->vel_y = vel * sin(entity->yaw) * cos(entity->pitch) * .1;
			entity->vel_z = vel * sin(entity->pitch) * .2;
			entity->skill[0] = 5 + rand() % 10;
		}
	}
	else if ( !(strncmp(command_str, "/cure", 5)) )
	{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, language[277]);
			return;
		}

		if ( multiplayer != SINGLE )
		{
			messagePlayer(clientnum, language[299]);
			return;
		}

		for ( c = 0; c < NUMEFFECTS; c++ )   //This does a whole lot more than just cure ailments.
		{
			players[clientnum]->entity->getStats()->EFFECTS[c] = false;
			players[clientnum]->entity->getStats()->EFFECTS_TIMERS[c] = 0;
		}
	}
	else if (!strncmp(command_str, "/summonall ", 11))
	{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, language[277]);
			return;
		}
		if (multiplayer == CLIENT)
		{
			messagePlayer(clientnum, language[284]);
		}
		else if (players[clientnum] && players[clientnum]->entity)
		{
			strcpy(name, command_str + 11);
			int i, creature;
			bool found = false;

			for (i = 1; i < NUMMONSTERS; ++i)   //Start at 1 because 0 is a nothing.
			{
				if ( i < KOBOLD ) //Search original monsters
				{
					if ( strstr(language[90 + i], name) )
					{
						creature = i;
						found = true;
						break;
					}
				}
				else if ( i >= KOBOLD ) //Search additional monsters
				{
					if ( strstr(language[2000 + (i - KOBOLD)], name) )
					{
						creature = i;
						found = true;
						break;
					}
				}

			}

			if (found)
			{
				playSoundEntity(players[clientnum]->entity, 153, 64);

				//Spawn monster
				summonManyMonster(static_cast<Monster>(creature));
			}
			else
			{
				messagePlayer(clientnum, language[304], name);
			}
		}
	}
	else if (!strncmp(command_str, "/summon ", 8))
	{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, language[277]);
			return;
		}
		if (multiplayer == CLIENT)
		{
			messagePlayer(clientnum, language[284]);
		}
		else if (players[clientnum] && players[clientnum]->entity)
		{
			strcpy(name, command_str + 8);
			int i, creature;
			bool found = false;

			for (i = 1; i < NUMMONSTERS; ++i)   //Start at 1 because 0 is a nothing.
			{
				if ( i < KOBOLD ) //Search original monsters
				{
					if ( strstr(language[90 + i], name) )
					{
						creature = i;
						found = true;
						break;
					}
				}
				else if ( i >= KOBOLD ) //Search additional monsters
				{
					if ( strstr(language[2000 + (i - KOBOLD)], name) )
					{
						creature = i;
						found = true;
						break;
					}
				}

			}

			if (found)
			{
				playSoundEntity(players[clientnum]->entity, 153, 64);

				//Spawn monster
				Entity* monster = summonMonster(static_cast<Monster>(creature), players[clientnum]->entity->x + 32 * cos(players[clientnum]->entity->yaw), players[clientnum]->entity->y + 32 * sin(players[clientnum]->entity->yaw));
				if (monster)
				{
					if ( i < KOBOLD )
					{
						messagePlayer(clientnum, language[302], language[90 + creature]);
					}
					else if ( i >= KOBOLD )
					{
						messagePlayer(clientnum, language[302], language[2000 + (creature-21)]);
					}
				}
				else
				{
					if ( i < KOBOLD )
					{
						messagePlayer(clientnum, language[303], language[90 + creature]);
					}
					else if ( i >= KOBOLD )
					{
						messagePlayer(clientnum, language[303], language[2000 + (creature - KOBOLD)]);
					}
				}
			}
			else
			{
				messagePlayer(clientnum, language[304], name);
			}
		}
	}
	else if (!strncmp(command_str, "/summonchest", 12)) //MAGIC TEST FUNCTION WE NEEDED LONG AGO.
	{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, language[277]);
			return;
		}
		if (multiplayer == CLIENT)
		{
			messagePlayer(clientnum, language[284]);
		}
		else if (players[clientnum] && players[clientnum]->entity)
		{
			playSoundEntity(players[clientnum]->entity, 153, 64);

			//Spawn monster
			Entity* chest = summonChest(players[clientnum]->entity->x + 32 * cos(players[clientnum]->entity->yaw), players[clientnum]->entity->y + 32 * sin(players[clientnum]->entity->yaw));
		}
	}
	else if (!strncmp(command_str, "/broadcast", 10))
	{
		broadcast = (broadcast == false);
	}
	else if (!strncmp(command_str, "/nohud", 6))
	{
		nohud = (nohud == false);
	}
	else if (!strncmp(command_str, "/disablehotbarnewitems", 15))
	{
		auto_hotbar_new_items = (auto_hotbar_new_items == false);
	}
	else if ( !strncmp(command_str, "/hotbarenablecategory ", 22) )
	{
		int catIndex = atoi(&command_str[22]);
		int value = atoi(&command_str[24]);
		auto_hotbar_categories[catIndex] = value;
		printlog("Hotbar auto add category %d, value %d.", catIndex, value);
	}
	else if ( !strncmp(command_str, "/autosortcategory ", 18) )
	{
		int catIndex = atoi(&command_str[18]);
		int value = atoi(&command_str[20]);
		autosort_inventory_categories[catIndex] = value;
		printlog("Autosort inventory category %d, priority %d.", catIndex, value);
	}
	else if ( !strncmp(command_str, "/quickaddtohotbar", 17) )
	{
		hotbar_numkey_quick_add = !hotbar_numkey_quick_add;
	}
	else if ( !strncmp(command_str, "/locksidebar", 12) )
	{
		lock_right_sidebar = (lock_right_sidebar == false);
		if ( lock_right_sidebar )
		{
			proficienciesPage = 1;
		}
	}
	else if ( !strncmp(command_str, "/showgametimer", 14) )
	{
		show_game_timer_always = (show_game_timer_always == false);
	}
	else if (!strncmp(command_str, "/lang ", 6))
	{
		command_str[8] = 0;
		loadLanguage(command_str + 6);
	}
	else if ( !strncmp(command_str, "/mapseed", 8) )
	{
		messagePlayer(clientnum, "%d", mapseed);
	}
	else if (!strncmp(command_str, "/reloadlang", 11))
	{
		reloadLanguage();
	}
	else if (!strncmp(command_str, "/disablemessages", 15))
	{
		disable_messages = true;
	}
	else if (!strncmp(command_str, "/right_click_protect", 19))
	{
		right_click_protect = true;
	}
	else if (!strncmp(command_str, "/autoappraisenewitems", 21))
	{
		auto_appraise_new_items = true;
	}
	else if (!strncmp(command_str, "/startfloor ", 12))
	{
		startfloor = atoi(&command_str[12]);
		//Ensure its value is in range.
		startfloor = std::max(startfloor, 0);
		//startfloor = std::min(startfloor, numlevels);
		printlog("Start floor is %d.", startfloor);
	}
	else if (!strncmp(command_str, "/splitscreen", 12))
	{
		splitscreen = true;
	}
	else if (!strncmp(command_str, "/gamepad_deadzone ", 18))
	{
		gamepad_deadzone = atoi(&command_str[18]);
		//Ensure its value is in range.
		gamepad_deadzone = std::max(gamepad_deadzone, 0);
		printlog("Controller deadzone is %d.", gamepad_deadzone);
	}
	else if (!strncmp(command_str, "/gamepad_trigger_deadzone ", 26))
	{
		gamepad_trigger_deadzone = atoi(&command_str[26]);
		//Ensure its value is in range.
		gamepad_trigger_deadzone = std::max(gamepad_trigger_deadzone, 0);
		printlog("Controller trigger deadzone is %d.", gamepad_trigger_deadzone);
	}
	else if (!strncmp(command_str, "/gamepad_leftx_sensitivity ", 27))
	{
		gamepad_leftx_sensitivity = atoi(&command_str[27]);
		//Ensure its value is in range.
		gamepad_leftx_sensitivity = std::max(gamepad_leftx_sensitivity, 1);
		printlog("Controller leftx sensitivity is %d.", gamepad_leftx_sensitivity);
	}
	else if (!strncmp(command_str, "/gamepad_lefty_sensitivity ", 27))
	{
		gamepad_lefty_sensitivity = atoi(&command_str[27]);
		//Ensure its value is in range.
		gamepad_lefty_sensitivity = std::max(gamepad_lefty_sensitivity, 1);
		printlog("Controller lefty sensitivity is %d.", gamepad_lefty_sensitivity);
	}
	else if (!strncmp(command_str, "/gamepad_rightx_sensitivity ", 28))
	{
		gamepad_rightx_sensitivity = atoi(&command_str[28]);
		//Ensure its value is in range.
		gamepad_rightx_sensitivity = std::max(gamepad_rightx_sensitivity, 1);
		printlog("Controller rightx sensitivity is %d.", gamepad_rightx_sensitivity);
	}
	else if (!strncmp(command_str, "/gamepad_righty_sensitivity ", 28))
	{
		gamepad_righty_sensitivity = atoi(&command_str[28]);
		//Ensure its value is in range.
		gamepad_righty_sensitivity = std::max(gamepad_righty_sensitivity, 1);
		printlog("Controller righty sensitivity is %d.", gamepad_righty_sensitivity);
	}
	else if (!strncmp(command_str, "/gamepad_menux_sensitivity ", 27))
	{
		gamepad_menux_sensitivity = atoi(&command_str[27]);
		//Ensure its value is in range.
		gamepad_menux_sensitivity = std::max(gamepad_menux_sensitivity, 1);
		printlog("Controller menux sensitivity is %d.", gamepad_menux_sensitivity);
	}
	else if (!strncmp(command_str, "/gamepad_menuy_sensitivity ", 27))
	{
		gamepad_menuy_sensitivity = atoi(&command_str[27]);
		//Ensure its value is in range.
		gamepad_menuy_sensitivity = std::max(gamepad_menuy_sensitivity, 1);
		printlog("Controller menuy sensitivity is %d.", gamepad_menuy_sensitivity);
	}
	else if (!strncmp(command_str, "/gamepad_leftx_invert", 21))
	{
		gamepad_leftx_invert = true;
	}
	else if (!strncmp(command_str, "/gamepad_lefty_invert", 21))
	{
		gamepad_lefty_invert = true;
	}
	else if (!strncmp(command_str, "/gamepad_rightx_invert", 22))
	{
		gamepad_rightx_invert = true;
	}
	else if (!strncmp(command_str, "/gamepad_righty_invert", 22))
	{
		gamepad_righty_invert = true;
	}
	else if (!strncmp(command_str, "/gamepad_menux_invert", 21))
	{
		gamepad_menux_invert = true;
	}
	else if (!strncmp(command_str, "/gamepad_menuy_invert", 21))
	{
		gamepad_menuy_invert = true;
	}
	else if ( !strncmp(command_str, "/numgold", 8) )
	{
		for ( unsigned i = 0; i < numplayers; ++i )
		{
			if ( client_disconnected[i] )
			{
				continue;
			}
			messagePlayer(clientnum, "Player %d has %d gold.", i, stats[i]->GOLD);
		}
	}
	else if ( !strncmp(command_str, "/gold ", 5) )
	{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, language[277]);
			return;
		}
		if ( multiplayer != SINGLE )
		{
			messagePlayer(clientnum, language[299]);
			return;
		}

		int amount = atoi(&command_str[5]);
		stats[clientnum]->GOLD += amount;
		stats[clientnum]->GOLD = std::max(stats[clientnum]->GOLD, 0);

		messagePlayer(clientnum, "Giving %d gold pieces.", amount);
	}
	else if ( !strncmp(command_str, "/dropgold", 9) )
	{
		int amount = 100;
		if ( !stats[clientnum] )
		{
			return;
		}
		else if ( stats[clientnum]->HP <= 0 )
		{
			return;
		}
		if ( stats[clientnum]->GOLD - amount < 0 )
		{
			amount = stats[clientnum]->GOLD;
		}
		if ( amount == 0 )
		{
			messagePlayer(clientnum, language[2593]);
			return;
		}
		stats[clientnum]->GOLD -= amount;
		stats[clientnum]->GOLD = std::max(stats[clientnum]->GOLD, 0);

		if ( multiplayer == CLIENT )
		{
			//Tell the server we dropped some gold.
			strcpy((char*)net_packet->data, "DGLD");
			net_packet->data[4] = clientnum;
			SDLNet_Write32(amount, &net_packet->data[5]);
			net_packet->address.host = net_server.host;
			net_packet->address.port = net_server.port;
			net_packet->len = 9;
			sendPacketSafe(net_sock, -1, net_packet, 0);
		}
		else
		{
			//Drop gold.
			int x = std::min<int>(std::max(0, (int)(players[clientnum]->entity->x / 16)), map.width - 1);
			int y = std::min<int>(std::max(0, (int)(players[clientnum]->entity->y / 16)), map.height - 1);
			if ( map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height] )
			{
				entity = newEntity(130, 0, map.entities, nullptr); // 130 = goldbag model
				entity->sizex = 4;
				entity->sizey = 4;
				entity->x = players[clientnum]->entity->x;
				entity->y = players[clientnum]->entity->y;
				entity->z = 6;
				entity->yaw = (rand() % 360) * PI / 180.0;
				entity->flags[PASSABLE] = true;
				entity->flags[UPDATENEEDED] = true;
				entity->behavior = &actGoldBag;
				entity->goldAmount = amount; // amount
			}
			playSoundEntity(players[clientnum]->entity, 242 + rand() % 4, 64);
		}

		messagePlayer(clientnum, language[2594], amount);
	}
	else if (!strncmp(command_str, "/minotaurlevel", 14))
	{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, language[277]);
			return;
		}
		if ( multiplayer != SINGLE )
		{
			messagePlayer(clientnum, language[299]);
			return;
		}

		if ( !minotaurlevel )
		{
			minotaurlevel = 1;
			createMinotaurTimer(players[0]->entity, &map);
		}
	}
	else if ( !strncmp(command_str, "/minotaurnow", 12) )
	{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, language[277]);
			return;
		}
		if ( multiplayer != SINGLE )
		{
			messagePlayer(clientnum, language[299]);
			return;
		}

		if ( minotaurlevel )
		{
			node_t *tmpNode = NULL;
			Entity *tmpEnt = NULL;
			for ( tmpNode = map.entities->first; tmpNode != NULL; tmpNode = tmpNode->next )
			{
				tmpEnt = (Entity*)tmpNode->element;
				if ( tmpEnt->sprite == 37 )
				{
					tmpEnt->skill[0] += TICKS_PER_SECOND * 210;
					return;
				}
			}
		}
	}
	else if ( !strncmp(command_str, "/levelskill ", 12) )
	{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, language[277]);
			return;
		}
		if ( multiplayer != SINGLE )
		{
			messagePlayer(clientnum, language[299]);
			return;
		}

		int skill = atoi(&command_str[12]);
		if ( skill >= NUMPROFICIENCIES )
		{
			messagePlayer(clientnum, language[2451]); //Skill out of range.
		}
		else
		{
			for ( int i = 0; i < 10; ++i )
			{
				players[clientnum]->entity->increaseSkill(skill);
			}
		}
	}
	else if ( !strncmp(command_str, "/maplevel", 9) )
	{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, language[277]);
			return;
		}
		if ( multiplayer != SINGLE )
		{
			messagePlayer(clientnum, language[299]);
			return;
		}

		messagePlayer(clientnum, language[412]);
		printlog("Made it this far...");

		mapLevel(clientnum);
	}
	else if ( !strncmp(command_str, "/drunky", 7) )
	{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, language[277]);
			return;
		}
		if ( multiplayer != SINGLE )
		{
			messagePlayer(clientnum, language[299]);
			return;
		}

		if ( !players[clientnum]->entity->getStats()->EFFECTS[EFF_DRUNK] )
		{
			players[clientnum]->entity->getStats()->EFFECTS[EFF_DRUNK] = true;
			players[clientnum]->entity->getStats()->EFFECTS_TIMERS[EFF_DRUNK] = -1;
		}
		else
		{
			players[clientnum]->entity->getStats()->EFFECTS[EFF_DRUNK] = false;
			players[clientnum]->entity->getStats()->EFFECTS_TIMERS[EFF_DRUNK] = 0;
		}
	}
	else if ( !strncmp(command_str, "/maxskill ", 9) )
	{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, language[277]);
			return;
		}
		if ( multiplayer != SINGLE )
		{
			messagePlayer(clientnum, language[299]);
			return;
		}

		int skill = atoi(&command_str[12]);
		if ( skill >= NUMPROFICIENCIES )
		{
			messagePlayer(clientnum, language[2451]); //Skill out of range.
		}
		else
		{
			for ( int i = players[clientnum]->entity->getStats()->PROFICIENCIES[skill]; i < 100; ++i )
			{
				players[clientnum]->entity->increaseSkill(skill);
			}
		}
	}
	else if ( !strncmp(command_str, "/reloadlimbs", 12) )
	{
		int x;
		FILE* fp;
		bool success = true;

		messagePlayer(clientnum, "Reloading limb offsets from limbs.txt files...");

		for ( c = 1; c < NUMMONSTERS; c++ )
		{
			// initialize all offsets to zero
			for ( x = 0; x < 20; x++ )
			{
				limbs[c][x][0] = 0;
				limbs[c][x][1] = 0;
				limbs[c][x][2] = 0;
			}

			// open file
			char filename[256];
			strcpy(filename, "models/creatures/");
			strcat(filename, monstertypename[c]);
			strcat(filename, "/limbs.txt");
			if ( (fp = openDataFile(filename, "r")) == NULL )
			{
				continue;
			}

			// read file
			int line;
			for ( line = 1; feof(fp) == 0; line++ )
			{
				char data[256];
				int limb = 20;
				int dummy;

				// read line from file
				fgets(data, 256, fp);

				// skip blank and comment lines
				if ( data[0] == '\n' || data[0] == '\r' || data[0] == '#' )
				{
					continue;
				}

				// process line
				if ( sscanf(data, "%d", &limb) != 1 || limb >= 20 || limb < 0 )
				{
					messagePlayer(clientnum, "warning: syntax error in '%s':%d\n invalid limb index!", filename, line);
					printlog("warning: syntax error in '%s':%d\n invalid limb index!\n", filename, line);
					success = false;
					continue;
				}
				if ( sscanf(data, "%d %f %f %f\n", &dummy, &limbs[c][limb][0], &limbs[c][limb][1], &limbs[c][limb][2]) != 4 )
				{
					messagePlayer(clientnum, "warning: syntax error in '%s':%d\n invalid limb offsets!", filename, line);
					printlog("warning: syntax error in '%s':%d\n invalid limb offsets!\n", filename, line);
					success = false;
					continue;
				}
			}

			// close file
			fclose(fp);
		}
		if ( success )
		{
			messagePlayer(clientnum, "Successfully reloaded all limbs.txt!");
		}
	}
	else if ( !strncmp(command_str, "/animspeed ", 10) )
	{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, language[277]);
			return;
		}
		if ( multiplayer != SINGLE )
		{
			messagePlayer(clientnum, language[299]);
			return;
		}

		int speed = atoi(&command_str[11]);
		monsterGlobalAnimationMultiplier = speed;
		messagePlayer(clientnum, "Changed animation speed multiplier to %f.", speed / 10.0);
	}
	else if ( !strncmp(command_str, "/atkspeed ", 9) )
	{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, language[277]);
			return;
		}
		if ( multiplayer != SINGLE )
		{
			messagePlayer(clientnum, language[299]);
			return;
		}

		int speed = atoi(&command_str[10]);
		monsterGlobalAttackTimeMultiplier = speed;
		messagePlayer(clientnum, "Changed attack speed multiplier to %d.", speed);
	}
	else if ( !strncmp(command_str, "/loadmod ", 9) )
	{
		std::string cmd = command_str;
		std::size_t dirfind = cmd.find("dir:");
		std::size_t namefind = cmd.find("name:");
		std::string modname;
		std::size_t fileidFind = cmd.find("fileid:");
		if ( dirfind != std::string::npos && namefind != std::string::npos && fileidFind == std::string::npos )
		{
			std::string directory = cmd.substr(dirfind + 4, namefind - (dirfind + 5));
			modname = cmd.substr(namefind + 5);
			modname = modname.substr(0, modname.length() - 1);
			printlog("[Mods]: Adding mod \"%s\" in path \"%s\"", directory.c_str(), modname.c_str());
			gamemods_mountedFilepaths.push_back(std::make_pair(directory, modname));
			gamemods_modelsListRequiresReload = true;
			gamemods_soundListRequiresReload = true;
		}
		if ( dirfind != std::string::npos && namefind != std::string::npos && fileidFind != std::string::npos )
		{
#ifdef STEAMWORKS
			std::string directory = cmd.substr(dirfind + 4, namefind - (dirfind + 5));
			modname = cmd.substr(namefind + 5, fileidFind - (namefind + 6));
			printlog("[Mods]: Adding mod \"%s\" in path \"%s\"", directory.c_str(), modname.c_str());
			gamemods_mountedFilepaths.push_back(std::make_pair(directory, modname));
			gamemods_modelsListRequiresReload = true;
			gamemods_soundListRequiresReload = true;

			uint64 id = atoi(cmd.substr(fileidFind + 7).c_str());
			gamemods_workshopLoadedFileIDMap.push_back(std::make_pair(modname, id));
			printlog("[Mods]: Steam Workshop mod file ID added for previous entry:%lld", id);
#endif
		}
	}
	else
	{
		invalidcommand = true;
	}

	if ( invalidcommand ) // starting new if else block to get around compiler >128 statement limit.
	{
		if ( !strncmp(command_str, "/muteaudiofocuslost", 19) )
		{
			mute_audio_on_focus_lost = (mute_audio_on_focus_lost == false);
		}
		else if ( !strncmp(command_str, "/minimaptransparencyfg", 22) )
		{
			minimapTransparencyForeground = atoi(&command_str[23]);
			minimapTransparencyForeground = std::min(std::max<int>(0, minimapTransparencyForeground), 100);

		}
		else if ( !strncmp(command_str, "/minimaptransparencybg", 22) )
		{
			minimapTransparencyBackground = atoi(&command_str[23]);
			minimapTransparencyBackground = std::min(std::max<int>(0, minimapTransparencyBackground), 100);

		}
		else if ( !strncmp(command_str, "/minimapscale", 13) )
		{
			minimapScale = atoi(&command_str[14]);
			minimapScale = std::min(std::max<int>(2, minimapScale), 16);
		}
		else if ( !strncmp(command_str, "/minimapobjectzoom", 18) )
		{
			minimapObjectZoom = atoi(&command_str[19]);
			minimapObjectZoom = std::min(std::max<int>(0, minimapObjectZoom), 4);
		}
		else if ( !strncmp(command_str, "/uiscale_inv", 12) )
		{
			std::stringstream ss;
			ss << command_str + 13;
			ss >> uiscale_inventory;
		}
		else if ( !strncmp(command_str, "/uiscale_hotbar", 15) )
		{
			std::stringstream ss;
			ss << command_str + 16;
			ss >> uiscale_hotbar;
		}
		else if ( !strncmp(command_str, "/uiscale_chatbox", 16) )
		{
			std::stringstream ss;
			ss << command_str + 17;
			ss >> uiscale_chatlog;
		}
		else if ( !strncmp(command_str, "/uiscale_playerbars", 19) )
		{
			std::stringstream ss;
			ss << command_str + 20;
			ss >> uiscale_playerbars;
		}
		else if ( !strncmp(command_str, "/uiscale_charsheet", 18) )
		{
			uiscale_charactersheet = !uiscale_charactersheet;
		}
		else if ( !strncmp(command_str, "/uiscale_skillsheet", 19) )
		{
			uiscale_skillspage = !uiscale_skillspage;
		}
		else if ( !strncmp(command_str, "/hidestatusbar", 14) )
		{
			hide_statusbar = !hide_statusbar;
		}
		else if ( !strncmp(command_str, "/hideplayertags", 15) )
		{
			hide_playertags = !hide_playertags;
		}
		else if ( !strncmp(command_str, "/showskillvalues", 16) )
		{
			show_skill_values = !show_skill_values;
		}
		else if ( !strncmp(command_str, "/disablenetworkmultithreading", 29) )
		{
			disableMultithreadedSteamNetworking = !disableMultithreadedSteamNetworking;
		}
		else if ( !strncmp(command_str, "/togglesecretlevel", 18) )
		{
			if ( !(svFlags & SV_FLAG_CHEATS) )
			{
				messagePlayer(clientnum, language[277]);
				return;
			}
			if ( multiplayer != SINGLE )
			{
				messagePlayer(clientnum, language[299]);
				return;
			}
			secretlevel = (secretlevel == false);
		}
		else if ( !strncmp(command_str, "/seteffect ", 11) )
		{
			if ( !(svFlags & SV_FLAG_CHEATS) )
			{
				messagePlayer(clientnum, language[277]);
				return;
			}
			if ( multiplayer != SINGLE )
			{
				messagePlayer(clientnum, language[299]);
				return;
			}

			int effect = atoi(&command_str[11]);
			if ( effect >= NUMEFFECTS || effect < 0 )
			{
				return;
			}
			else
			{
				players[clientnum]->entity->setEffect(effect, true, 500, true);
			}
		}
		else if ( !strncmp(command_str, "/brawlermode", 12) )
		{
			achievementBrawlerMode = !achievementBrawlerMode;
			if ( achievementBrawlerMode && conductGameChallenges[CONDUCT_BRAWLER] )
			{
				messagePlayer(clientnum, language[2995]);
			}
			else if ( achievementBrawlerMode && !conductGameChallenges[CONDUCT_BRAWLER] )
			{
				messagePlayer(clientnum, language[2998]);
			}
			else if ( !achievementBrawlerMode )
			{
				messagePlayer(clientnum, language[2996]);
			}
		}
		else
		{
			messagePlayer(clientnum, language[305], command_str);
		}
	}
}
