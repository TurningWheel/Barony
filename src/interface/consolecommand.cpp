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
#include "../engine/audio/sound.hpp"
#include "../menu.hpp"
#include "../monster.hpp"
#include "../net.hpp"
#include "../paths.hpp"
#include "../player.hpp"
#include "interface.hpp"
#include "../scores.hpp"
#include "../magic/magic.hpp"
#include "../mod_tools.hpp"
#include "../collision.hpp"
#include "../player.hpp"
#include "../ui/GameUI.hpp"
#include "../classdescriptions.hpp"

bool spamming = false;
bool showfirst = false;
bool logCheckObstacle = false;
int logCheckObstacleCount = 0;
bool logCheckMainLoopTimers = false;
bool autoLimbReload = false;

#define CCMD (int argc, const char **argv)
typedef void (*ccmd_function)CCMD;
typedef std::unordered_map<std::string, ccmd_function> ccmd_map_t;
static ccmd_map_t& getConsoleCommands()
{
	static ccmd_map_t ccmd_map;
	return ccmd_map;
}

class ConsoleCommand {
    ConsoleCommand(const char* name, ccmd_function func) {
        auto& map = getConsoleCommands();
        map.emplace(name, func);
    }
};

/*-------------------------------------------------------------------------------

	consoleCommand

	Takes a string and executes it as a game command

-------------------------------------------------------------------------------*/

void consoleCommand(char const * const command_str)
{
	if ( !command_str || command_str[0] == '\0' )
	{
		return;
	}

	char buf[1024];
	size_t size = strlen(command_str);
	size = std::min(size, sizeof(buf) - 1);
	memcpy(buf, command_str, size);
	buf[size] = '\0';

    const char* command = NULL;
    std::vector<const char*> tokens;
    auto token = strtok(buf, " ");
	auto command = token;
	while (token) {
	    tokens.push_back(token);
	    strtok(token, " ");
	}

	auto& map = getConsoleCommands();
	auto find = map.find(command);
	if (find == map.end()) {
	    // invalid command
		messagePlayer(clientnum, MESSAGE_MISC, language[305], command_str);
	} else {
	    auto ccmd = find->second;
	    (*ccmd)(tokens.size(), tokens.data());
	}
}

namespace ConsoleCommands {
    static ConsoleCommand ccmd_ping("/ping", []CCMD{
	    if ( multiplayer != CLIENT )
	    {
		    messagePlayer(clientnum, MESSAGE_MISC, language[1117], 0);
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
        });

    static ConsoleCommand ccmd_usemodelcache("/usemodelcache", []CCMD{
        useModelCache = true;
        });

    static ConsoleCommand ccmd_disablemodelcache("/disablemodelcache", []CCMD{
	    useModelCache = false;
        });

    static ConsoleCommand ccmd_fov("/fov", []CCMD{
        if (argc < 2) {
            return;
        }
	    fov = atoi(argv[1]);
	    fov = std::min(std::max<Uint32>(40, fov), 100u);
        });

    static ConsoleCommand ccmd_fps("/fps", []CCMD{
        if (argc < 2) {
            return;
        }
		fpsLimit = atoi(argv[1]);
		fpsLimit = std::min(std::max<Uint32>(30, fpsLimit), 300u);
        });

    static ConsoleCommand ccmd_svflags("/svflags", []CCMD{
		if ( multiplayer == CLIENT )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[275]);
		}
		else
		{
		    if (argc < 2)
		    {
		        return;
		    }
			svFlags = atoi(argv[1]);
			messagePlayer(clientnum, MESSAGE_MISC, language[276]);

			if ( multiplayer == SERVER )
			{
				// update client flags
				strcpy((char*)net_packet->data, "SVFL");
				SDLNet_Write32(svFlags, &net_packet->data[4]);
				net_packet->len = 8;

				for ( int c = 1; c < MAXPLAYERS; c++ )
				{
					if ( client_disconnected[c] || players[c]->isLocalPlayer() )
					{
						continue;
					}
					net_packet->address.host = net_clients[c - 1].host;
					net_packet->address.port = net_clients[c - 1].port;
					sendPacketSafe(net_sock, -1, net_packet, c - 1);
					messagePlayer(c, MESSAGE_MISC, language[276]);
				}
			}
		}
        });

    static ConsoleCommand ccmd_lastname("/lastname", []CCMD{
        if (argc < 2)
        {
            return;
        }
		lastname = argv[1];
		});

    static ConsoleCommand ccmd_lastchar("/lastcharacter", []CCMD{
		for ( int c = 1; c < argc; ++c )
		{
			switch ( c )
			{
				case 1:
					lastCreatedCharacterSex = atoi(argv[c]);
					break;
				case 2:
					lastCreatedCharacterClass = atoi(argv[c]);
					break;
				case 3:
					lastCreatedCharacterAppearance = atoi(argv[c]);
					break;
				case 4:
					lastCreatedCharacterRace = atoi(argv[c]);
					break;
				default:
					break;
			}
		}
		});

	static ConsoleCommand ccmd_spawnitem("/spawnitem", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}
		if (argc < 2)
		{
		    return;
		}
		auto name = argv[1];

        int c;
		for ( c = 0; c < NUMITEMS; c++ )
		{
			if ( strcmp(items[c].name_identified, name) == 0 )
			{
				dropItem(newItem(static_cast<ItemType>(c), EXCELLENT, 0, 1, rand(), true, &stats[clientnum]->inventory), 0);
				break;
			}
		}
		if ( c == NUMITEMS )
		{
			for ( c = 0; c < NUMITEMS; c++ )
			{
				if ( strstr(items[c].name_identified, name) )
				{
					dropItem(newItem(static_cast<ItemType>(c), EXCELLENT, 0, 1, rand(), true, &stats[clientnum]->inventory), 0);
					break;
				}
			}
		}
		if ( c == NUMITEMS )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[278], name);
		}
	    });

	static ConsoleCommand ccmd_spawncursed("/spawncursed", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}
		if (argc < 2)
		{
		    return;
		}
		auto name = argv[1];

		int c;
		for ( c = 0; c < NUMITEMS; c++ )
		{
			if ( strcmp(items[c].name_identified, name) == 0 )
			{
				dropItem(newItem(static_cast<ItemType>(c), WORN, -2, 1, rand(), false, &stats[clientnum]->inventory), 0);
				break;
			}
		}
		if ( c == NUMITEMS )
		{
			for ( c = 0; c < NUMITEMS; c++ )
			{
				if ( strstr(items[c].name_identified, name) )
				{
					dropItem(newItem(static_cast<ItemType>(c), WORN, -2, 1, rand(), false, &stats[clientnum]->inventory), 0);
					break;
				}
			}
		}
		if ( c == NUMITEMS )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[278], name);
		}
	    });

	static ConsoleCommand ccmd_spawnblessed("/spawnblessed", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}
		if (argc < 2)
		{
		    return;
		}
		auto name = argv[1];

		int c;
		for ( c = 0; c < NUMITEMS; ++c )
		{
			if ( strcmp(items[c].name_identified, name) == 0 )
			{
				dropItem(newItem(static_cast<ItemType>(c), WORN, 2, 1, rand(), false, &stats[clientnum]->inventory), 0);
				break;
			}
		}
		if ( c == NUMITEMS )
		{
			for ( c = 0; c < NUMITEMS; ++c )
			{
				if ( strstr(items[c].name_identified, name) )
				{
					dropItem(newItem(static_cast<ItemType>(c), WORN, 2, 1, rand(), false, &stats[clientnum]->inventory), 0);
					break;
				}
			}
		}
		if ( c == NUMITEMS )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[278], name);
		}
	    });

	static ConsoleCommand ccmd_kick("/kick", []CCMD{
		if (argc < 2)
		{
		    return;
		}
		auto name = argv[1];
		if ( multiplayer == SERVER )
		{
			for ( int c = 1; c < MAXPLAYERS; c++ )
			{
				if ( !client_disconnected[c] && !strncmp(name, stats[c]->name, 128) && !players[c]->isLocalPlayer() )
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
						messagePlayer(i, MESSAGE_MISC, language[279], c, stats[c]->name);
					}
					break;
				}
			}
			if ( c == MAXPLAYERS )
			{
				messagePlayer(clientnum, MESSAGE_MISC, language[280]);
			}
		}
		else if ( multiplayer == CLIENT )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[281]);
		}
		else
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[282]);
		}
	    });

	static ConsoleCommand ccmd_spawnbook("/spawnbook", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}
        if (argc < 2)
        {
            return;
        }
		auto name = argv[1];
		dropItem(newItem(READABLE_BOOK, EXCELLENT, 0, 1, getBook(name), true, &stats[clientnum]->inventory), 0);
	    });

	static ConsoleCommand ccmd_savemap("/savemap", []CCMD{
		if ( argc > 1 )
		{
			saveMap(argv[1]);
			messagePlayer(clientnum, MESSAGE_MISC, language[283], argv[1]);
		}
	    });

	static ConsoleCommand ccmd_nextlevel("/nextlevel", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}
		if ( multiplayer == CLIENT )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[284]);
		}
		else
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[285]);
			loadnextlevel = true;
		}
	    });

	static ConsoleCommand ccmd_pos("/pos", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}
		messagePlayer(clientnum, language[286],
		    (int)cameras[0].x,
		    (int)cameras[0].y,
		    (int)cameras[0].z,
		    cameras[0].ang,
		    cameras[0].vang);
	    });

	static ConsoleCommand ccmd_asdf("/asdf", []CCMD{
	    });

	static ConsoleCommand ccmd_pathmap("/pathmap", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}
		if (players[clientnum] && players[clientnum]->entity)
		{
			int x = std::min<int>(std::max<int>(0, floor(players[clientnum]->entity->x / 16)), map.width - 1);
			int y = std::min<int>(std::max<int>(0, floor(players[clientnum]->entity->y / 16)), map.height - 1);
			messagePlayer(clientnum, MESSAGE_MISC, "pathMapGrounded value: %d", pathMapGrounded[y + x * map.height]);
			messagePlayer(clientnum, MESSAGE_MISC, "pathMapFlying value: %d", pathMapFlying[y + x * map.height]);
		}
	    });

	static ConsoleCommand ccmd_exit("/exit", []CCMD{
		mainloop = 0;
		});

	static ConsoleCommand ccmd_showfps("/showfps", []CCMD{
		showfps = (showfps == false);
		});

	static ConsoleCommand ccmd_noclip("/noclip", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}
		if ( multiplayer != SINGLE )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[287]);
		}
		else
		{
			noclip = (noclip == false);
			if ( noclip )
			{
				messagePlayer(clientnum, MESSAGE_MISC, language[288]);
			}
			else
			{
				messagePlayer(clientnum, MESSAGE_MISC, language[289]);
			}
		}
		});

	static ConsoleCommand ccmd_god("/god", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}
		if ( multiplayer != SINGLE )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[290]);
		}
		else
		{
			godmode = (godmode == false);
			if ( godmode )
			{
				messagePlayer(clientnum, MESSAGE_MISC, language[291]);
			}
			else
			{
				messagePlayer(clientnum, MESSAGE_MISC, language[292]);
			}
		}
		});

	static ConsoleCommand ccmd_spam("/spam", []CCMD{
		spamming = !(spamming);
		});

	static ConsoleCommand ccmd_logobstacle("/logobstacle", []CCMD{
		logCheckObstacle = !(logCheckObstacle);
		});

	static ConsoleCommand ccmd_showfirst("/showfirst", []CCMD{
		showfirst = !(showfirst);
		});

	static ConsoleCommand ccmd_buddha("/buddha", []CCMD{
		if ( multiplayer != SINGLE )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[293]);
		}
		else
		{
			buddhamode = (buddhamode == false);
			if ( buddhamode )
			{
				messagePlayer(clientnum, MESSAGE_MISC, language[294]);
			}
			else
			{
				messagePlayer(clientnum, MESSAGE_MISC, language[295]);
			}
		}
		});

	static ConsoleCommand ccmd_friendly("/friendly", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}
		if ( multiplayer == CLIENT )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[284]);
			return;
		}
		everybodyfriendly = (everybodyfriendly == false);
		if ( everybodyfriendly )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[296]);
		}
		else
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[297]);
		}
		});

	static ConsoleCommand ccmd_dowse("/dowse", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}
		for ( node = map.entities->first; node != NULL; node = node->next )
		{
			entity = (Entity*)node->element;
			if ( entity->behavior == &actLadder )
			{
				messagePlayer(clientnum, MESSAGE_MISC, language[298], (int)(entity->x / 16), (int)(entity->y / 16));
			}
		}
		});

	static ConsoleCommand ccmd_thirdperson("/thirdperson", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
		    // this is definitely considered a cheat.
		    // otherwise it's a major gameplay exploit.
		    // do not disable this code block.
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}
		if (players[clientnum] != nullptr && players[clientnum]->entity != nullptr)
		{
			players[clientnum]->entity->skill[3] = (players[clientnum]->entity->skill[3] == 0);
			if (players[clientnum]->entity->skill[3] == 1)
			{
				messagePlayer(clientnum, MESSAGE_MISC, "thirdperson ON");
			}
			else
			{
				messagePlayer(clientnum, MESSAGE_MISC, "thirdperson OFF");
			}
		}
		});

	static ConsoleCommand ccmd_res("/res", []CCMD{
	    if (argc < 2)
	    {
	        return;
	    }
		xres = atoi(argv[1]);
		for ( int c = 0; c < strlen(argv[1]); c++ )
		{
			if ( argv[1][c] == 'x' )
			{
				yres = atoi(&argv[1][c + 1]);
				break;
			}
		}
		if (initialized)
		{
			if ( !changeVideoMode() )
			{
				printlog("critical error! Attempting to abort safely...\n");
				mainloop = 0;
			}
		}
		});

	static ConsoleCommand ccmd_rscale("/rscale", []CCMD{
	    if (argc < 2)
	    {
	        return;
	    }
		rscale = atoi(argv[1]);
		});

	static ConsoleCommand ccmd_fullscreen("/fullscreen", []CCMD{
		fullscreen = (fullscreen == 0);
		});

	static ConsoleCommand ccmd_shaking("/shaking", []CCMD{
		shaking = (shaking == 0);
		});

	static ConsoleCommand ccmd_bobbing("/bobbing", []CCMD{
		bobbing = (bobbing == 0);
		});

	static ConsoleCommand ccmd_sfxvolume("/sfxvolume", []CCMD{
	    if (argc > 1)
		    sfxvolume = strtof(argv[1], nullptr);
		});

	static ConsoleCommand ccmd_musvolume("/musvolume", []CCMD{
	    if (argc > 1)
		    musvolume = strtof(argv[1], nullptr);
		});

	static ConsoleCommand ccmd_bind("/bind", []CCMD{
		printlog("Note: /bind is now deprecated.\n");
		});

	static ConsoleCommand ccmd_joybind("/joybind", []CCMD{
		printlog("Note: /joybind is now deprecated.\n");
		});

	static ConsoleCommand ccmd_mousespeed("/mousespeed", []CCMD{
	    if (argc > 1)
		    mousespeed = atoi(argv[1]);
		});

	static ConsoleCommand ccmd_reversemouse("/reversemouse", []CCMD{
		reversemouse = (reversemouse == 0);
		});

	static ConsoleCommand ccmd_smoothmouse("/smoothmouse", []CCMD{
		smoothmouse = (smoothmouse == false);
		});

	static ConsoleCommand ccmd_mana("/mana", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}

		if ( multiplayer == SINGLE )
		{
			stats[clientnum]->MP = stats[clientnum]->MAXMP;
		}
		else
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[299]);
		}
		});

	static ConsoleCommand ccmd_heal("/heal", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}

		if ( multiplayer == SINGLE )
		{
			stats[clientnum]->HP = stats[clientnum]->MAXHP;
		}
		else
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[299]);
		}
		});

	static ConsoleCommand ccmd_damage("/damage", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}
		if ( multiplayer != SINGLE )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[299]);
			return;
		}
        if (argc < 2) {
            return;
        }

		int amount = atoi(argv[1]);

		players[clientnum]->entity->modHP(-amount);

		messagePlayer(clientnum, MESSAGE_MISC, "Damaging you by %d. New health: %d", amount, stats[clientnum]->HP);
		});

	static ConsoleCommand ccmd_ip("/ip", []CCMD{
		if ( argc > 1 )
		{
		    size_t size = strlen(argv[1]);
		    size = std::min(size, sizeof(last_ip) - 1);
		    memcpy(last_ip, argv[1], size);
		    last_ip[size] = '\0';
		}
		});

	static ConsoleCommand ccmd_port("/port", []CCMD{
		if ( argc > 1 )
		{
		    size_t size = strlen(argv[1]);
		    size = std::min(size, sizeof(last_port) - 1);
		    memcpy(last_port, argv[1], size);
		    last_port[size] = '\0';
		}
		});

	static ConsoleCommand ccmd_noblood("/noblood", []CCMD{
		spawn_blood = (spawn_blood == false);
		});

	static ConsoleCommand ccmd_nolightflicker("/nolightflicker", []CCMD{
		flickerLights = (flickerLights == false);
		});

	static ConsoleCommand ccmd_vsync("/vsync", []CCMD{
		verticalSync = (verticalSync == false);
		});

	static ConsoleCommand ccmd_hidestatusicons("/hidestatusicons", []CCMD{
		showStatusEffectIcons = (showStatusEffectIcons == false);
		});

	static ConsoleCommand ccmd_muteping("/muteping", []CCMD{
		minimapPingMute = (minimapPingMute == false);
		});

	static ConsoleCommand ccmd_colorblind("/colorblind", []CCMD{
		colorblind = (colorblind == false);
		});

	static ConsoleCommand ccmd_gamma("/gamma", []CCMD{
        if (argc < 2) {
            return;
        }
		vidgamma = strtof(argv[1], nullptr);
		});

	static ConsoleCommand ccmd_capturemouse("/capturemouse", []CCMD{
		capture_mouse = (capture_mouse == false);
		});

	static ConsoleCommand ccmd_levelup("/levelup", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
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
			//messagePlayer(clientnum, MESSAGE_MISC, language[299]);
		}
		});

	static ConsoleCommand ccmd_maxout2("/maxout2", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
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
				auto& hotbar = players[clientnum]->hotbar.slots();
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
			//messagePlayer(clientnum, MESSAGE_MISC, language[299]);
		}
		});

	static ConsoleCommand ccmd_jumplevel("/jumplevel", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}
        if (argc < 2) {
            return;
        }
		skipLevelsOnLoad = atoi(argv[1]);
		if ( skipLevelsOnLoad == -1 )
		{
			loadingSameLevelAsCurrent = true;
		}
		consoleCommand("/nextlevel");
		});

	static ConsoleCommand ccmd_maxout3("/maxout3", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
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
				auto& hotbar = players[clientnum]->hotbar.slots();
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
			messagePlayer(clientnum, MESSAGE_MISC, language[299]);
		}
		});

	static ConsoleCommand ccmd_maxout4("/maxout4", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
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
				auto& hotbar = players[clientnum]->hotbar.slots();
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
			messagePlayer(clientnum, MESSAGE_MISC, language[299]);
		}
		});

	static ConsoleCommand ccmd_maxout("/maxout", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
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
			messagePlayer(clientnum, MESSAGE_MISC, language[299]);
		}
		});

	static ConsoleCommand ccmd_hunger("/hunger", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
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
			messagePlayer(clientnum, MESSAGE_MISC, language[299]);
		}
		});

	static ConsoleCommand ccmd_poison("/poison", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
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
			messagePlayer(clientnum, MESSAGE_MISC, language[299]);
		}
		});

	static ConsoleCommand ccmd_testsound("/testsound", []CCMD{
        if (argc < 2) {
            return;
        }
		int num = atoi(argv[1]);
		playSound(num, 256);
		});

	static ConsoleCommand ccmd_skipintro("/skipintro", []CCMD{
		skipintro = (skipintro == false);
		});

	static ConsoleCommand ccmd_levelmagic("/levelmagic", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
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
			messagePlayer(clientnum, MESSAGE_MISC, language[299]);
		}
		});

	static ConsoleCommand ccmd_numentities("/numentities", []CCMD{
		messagePlayer(clientnum, MESSAGE_MISC, language[300], list_Size(map.entities));
		});

	static ConsoleCommand ccmd_nummonsters2("/nummonsters2", []CCMD{
		messagePlayer(clientnum, MESSAGE_MISC, language[2353], list_Size(map.creatures));
		});

	static ConsoleCommand ccmd_nummonsters("/nummonsters", []CCMD{
		messagePlayer(clientnum, MESSAGE_MISC, language[2353], nummonsters);
		});

	static ConsoleCommand ccmd_verifycreaturelist("/verifycreaturelist", []CCMD{
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

		messagePlayer(clientnum, MESSAGE_MISC, "ent count = %d, creatures list size = %d", entcount, list_Size(map.creatures));

		if ( entcount == list_Size(map.creatures) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, "Yes, list is verified correct.");
		}
		else
		{
			messagePlayer(clientnum, MESSAGE_MISC, "Nope, much problemo!");
		}
		});

	static ConsoleCommand ccmd_loadmodels("/loadmodels", []CCMD{
		char name2[128];
		char buf[16] = "";
		int startIndex = 0;
		int endIndex = nummodels;
		int i = 0;
        if (argc < 2) {
            return;
        }
		auto name = argv[1];
		for ( int c = 0; name[c] != '\0'; c++ )
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
		File *fp = openDataFile(modelsDirectory.c_str(), "r");
		for ( int c = 0; !fp->eof(); c++ )
		{
			fp->gets2(name2, sizeof(name2));
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
					if ( polymodels[c].grayscale_colors )
					{
						SDL_glDeleteBuffers(1, &polymodels[c].grayscale_colors);
					}
					if ( polymodels[c].grayscale_colors_shifted )
					{
						SDL_glDeleteBuffers(1, &polymodels[c].grayscale_colors_shifted);
					}
				}
				models[c] = loadVoxel(name2);
			}
		}
		FileIO::close(fp);
		//messagePlayer(clientnum, language[2354]);
		messagePlayer(clientnum, language[2355], startIndex, endIndex);
		generatePolyModels(startIndex, endIndex, true);
		});

	static ConsoleCommand ccmd_killmonsters("/killmonsters", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}
		if ( multiplayer == CLIENT )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[284]);
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
			messagePlayer(clientnum, MESSAGE_MISC, language[301], c);
		}
		});

	static ConsoleCommand ccmd_die("/die", []CCMD{
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
			if ( players[clientnum] && players[clientnum]->entity )
			{
				players[clientnum]->entity->setHP(0);
			}
		}
		});

	static ConsoleCommand ccmd_segfault("/segfault", []CCMD{
		int* potato = NULL;
		(*potato) = 322; //Crash the game!
		});

	static ConsoleCommand ccmd_flames("/flames", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}

		if ( multiplayer != SINGLE )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[299]);
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
		});

	static ConsoleCommand ccmd_cure("/cure", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}

		if ( multiplayer != SINGLE )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[299]);
			return;
		}

		for ( c = 0; c < NUMEFFECTS; c++ )   //This does a whole lot more than just cure ailments.
		{
			if ( !(c == EFF_VAMPIRICAURA && players[clientnum]->entity->getStats()->EFFECTS_TIMERS[c] == -2) 
				&& c != EFF_WITHDRAWAL && c != EFF_SHAPESHIFT )
			{
				players[clientnum]->entity->getStats()->EFFECTS[c] = false;
				players[clientnum]->entity->getStats()->EFFECTS_TIMERS[c] = 0;
			}
		}
		if ( players[clientnum]->entity->getStats()->EFFECTS[EFF_WITHDRAWAL] )
		{
			players[clientnum]->entity->setEffect(EFF_WITHDRAWAL, false, EFFECT_WITHDRAWAL_BASE_TIME, true);
		}
		});

	static ConsoleCommand ccmd_summonall("/summonall", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}
		if (multiplayer == CLIENT)
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[284]);
		}
		else if (players[clientnum] && players[clientnum]->entity)
		{
            if (argc < 2) {
                return;
            }
			auto name = argv[1];
			int i, creature;
			bool found = false;

			for (i = 1; i < NUMMONSTERS; ++i)   //Start at 1 because 0 is a nothing.
			{
				if ( strstr(getMonsterLocalizedName((Monster)i).c_str(), name) )
				{
					creature = i;
					found = true;
					break;
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
				messagePlayer(clientnum, MESSAGE_MISC, language[304], name);
			}
		}
		});

	static ConsoleCommand ccmd_summon("/summon", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}
		if (multiplayer == CLIENT)
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[284]);
		}
		else if (players[clientnum] && players[clientnum]->entity)
		{
            if (argc < 2) {
                return;
            }
			auto name = argv[1];
			int i, creature;
			bool found = false;

			for (i = 1; i < NUMMONSTERS; ++i)   //Start at 1 because 0 is a nothing.
			{
				if ( strstr(getMonsterLocalizedName((Monster)i).c_str(), name) )
				{
					creature = i;
					found = true;
					break;
				}
			}
			if ( !found )
			{
				MonsterStatCustomManager::StatEntry* statEntry = monsterStatCustomManager.readFromFile(name);
				if ( statEntry )
				{
					Entity* monster = summonMonster(static_cast<Monster>(statEntry->type), players[clientnum]->entity->x + 32 * cos(players[clientnum]->entity->yaw), players[clientnum]->entity->y + 32 * sin(players[clientnum]->entity->yaw));
					if ( monster )
					{
						messagePlayer(clientnum, MESSAGE_MISC, language[302], getMonsterLocalizedName(static_cast<Monster>(statEntry->type)).c_str());
						if ( monster->getStats() )
						{
							statEntry->setStatsAndEquipmentToMonster(monster->getStats());
							while ( statEntry->numFollowers > 0 )
							{
								std::string followerName = statEntry->getFollowerVariant();
								if ( followerName.compare("") && followerName.compare("none") )
								{
									MonsterStatCustomManager::StatEntry* followerEntry = monsterStatCustomManager.readFromFile(followerName.c_str());
									if ( followerEntry )
									{
										Entity* summonedFollower = summonMonster(static_cast<Monster>(followerEntry->type), monster->x, monster->y);
										if ( summonedFollower )
										{
											if ( summonedFollower->getStats() )
											{
												followerEntry->setStatsAndEquipmentToMonster(summonedFollower->getStats());
												summonedFollower->getStats()->leader_uid = monster->getUID();
											}
										}
										delete followerEntry;
									}
									else
									{
										Entity* summonedFollower = summonMonster(monster->getStats()->type, monster->x, monster->y);
										if ( summonedFollower )
										{
											if ( summonedFollower->getStats() )
											{
												summonedFollower->getStats()->leader_uid = monster->getUID();
											}
										}
									}
								}
								--statEntry->numFollowers;
							}
						}
					}
					delete statEntry;
					return;
				}
			}

			if (found)
			{
				playSoundEntity(players[clientnum]->entity, 153, 64);

				//Spawn monster
				Entity* monster = summonMonster(static_cast<Monster>(creature), players[clientnum]->entity->x + 32 * cos(players[clientnum]->entity->yaw), players[clientnum]->entity->y + 32 * sin(players[clientnum]->entity->yaw));
				if (monster)
				{
					messagePlayer(clientnum, MESSAGE_MISC, language[302], getMonsterLocalizedName((Monster)creature).c_str());
				}
				else
				{
					messagePlayer(clientnum, MESSAGE_MISC, language[303], getMonsterLocalizedName((Monster)creature).c_str());
				}
			}
			else
			{
				messagePlayer(clientnum, MESSAGE_MISC, language[304], name);
			}
		}
		});

	static ConsoleCommand ccmd_summonchest("/summonchest", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}
		if (multiplayer == CLIENT)
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[284]);
		}
		else if (players[clientnum] && players[clientnum]->entity)
		{
		    auto player = players[clientnum]->entity;

			playSoundEntity(player, 153, 64);

            real_t x = player->x + 32.0 * cos(player->yaw);
            real_t y = player->y + 32.0 * sin(player->yaw);
			Entity* chest = summonChest(x, y);
		}
		});

	static ConsoleCommand ccmd_broadcast("/broadcast", []CCMD{
		broadcast = (broadcast == false);
		});

	static ConsoleCommand ccmd_nohud("/nohud", []CCMD{
		nohud = (nohud == false);
		});

	static ConsoleCommand ccmd_disablehotbarnewitems("/disablehotbarnewitems", []CCMD{
		auto_hotbar_new_items = (auto_hotbar_new_items == false);
		});

	static ConsoleCommand ccmd_hotbarenablecategory("/hotbarenablecategory", []CCMD{
        if (argc < 3) {
            return;
        }
		int catIndex = atoi(argv[1]);
		int value = atoi(argv[2]);
		auto_hotbar_categories[catIndex] = value;
		printlog("Hotbar auto add category %d, value %d.", catIndex, value);
		});

	static ConsoleCommand ccmd_autosortcategory("/autosortcategory", []CCMD{
        if (argc < 3) {
            return;
        }
		int catIndex = atoi(argv[1]);
		int value = atoi(argv[2]);
		autosort_inventory_categories[catIndex] = value;
		printlog("Autosort inventory category %d, priority %d.", catIndex, value);
		});

	static ConsoleCommand ccmd_quickaddtohotbar("/quickaddtohotbar", []CCMD{
		hotbar_numkey_quick_add = !hotbar_numkey_quick_add;
		});

	static ConsoleCommand ccmd_locksidebar("/locksidebar", []CCMD{
		if ( players[clientnum] ) // warning - this doesn't exist when loadConfig() is called on init.
		{
			players[clientnum]->characterSheet.lock_right_sidebar = (players[clientnum]->characterSheet.lock_right_sidebar == false);
			if ( players[clientnum]->characterSheet.lock_right_sidebar )
			{
				players[clientnum]->characterSheet.proficienciesPage = 1;
			}
		}
		});

	static ConsoleCommand ccmd_showgametimer("/showgametimer", []CCMD{
		show_game_timer_always = (show_game_timer_always == false);
		});

	static ConsoleCommand ccmd_lang("/lang", []CCMD{
	    if (argc > 1) {
		    loadLanguage(argv[1]);
	    }
		});

	static ConsoleCommand ccmd_mapseed("/mapseed", []CCMD{
		messagePlayer(clientnum, MESSAGE_MISC, "%d", mapseed);
		});

	static ConsoleCommand ccmd_reloadlang("/reloadlang", []CCMD{
		reloadLanguage();
		});

	static ConsoleCommand ccmd_disablemessages("/disablemessages", []CCMD{
		disable_messages = true;
		});

	static ConsoleCommand ccmd_right_click_protect("/right_click_protect", []CCMD{
		right_click_protect = (right_click_protect == false);
		});

	static ConsoleCommand ccmd_autoappraisenewitems("/autoappraisenewitems", []CCMD{
		auto_appraise_new_items = true;
		});

	static ConsoleCommand ccmd_startfloor("/startfloor", []CCMD{
	    if (argc > 1)
	    {
		    startfloor = atoi(argv[1]);
		    //Ensure its value is in range.
		    startfloor = std::max(startfloor, 0);
		    //startfloor = std::min(startfloor, numlevels);
		    printlog("Start floor is %d.", startfloor);
		}
		});

	static ConsoleCommand ccmd_splitscreen("/splitscreen", []CCMD{
		int numPlayers = 4;
		splitscreen = !splitscreen;

		if ( splitscreen )
		{
			for ( int i = 1; i < MAXPLAYERS; ++i )
			{
				if ( i < numPlayers )
				{
					client_disconnected[i] = false;
				}
			}
		}
		else
		{
			client_disconnected[1] = true;
			client_disconnected[2] = true;
			client_disconnected[3] = true;
		}

		int playercount = 1;
		for ( int i = 1; i < MAXPLAYERS; ++i )
		{
			if ( client_disconnected[i] )
			{
				players[i]->bSplitscreen = false;
				players[i]->splitScreenType = Player::SPLITSCREEN_DEFAULT;
			}
			else
			{
				players[i]->bSplitscreen = true;
			}

			if ( players[i]->isLocalPlayer() )
			{
				++playercount;
			}
		}


		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			if ( verticalSplitscreen )
			{
				players[i]->splitScreenType = Player::SPLITSCREEN_VERTICAL;
			}
			else
			{
				players[i]->splitScreenType = Player::SPLITSCREEN_DEFAULT;
			}

			if ( !splitscreen )
			{
				players[i]->camera().winx = 0;
				players[i]->camera().winy = 0;
				players[i]->camera().winw = xres;
				players[i]->camera().winh = yres;
			}
			else
			{
				if ( playercount == 1 )
				{
					players[i]->camera().winx = 0;
					players[i]->camera().winy = 0;
					players[i]->camera().winw = xres;
					players[i]->camera().winh = yres;
				}
				else if ( playercount == 2 )
				{
					if ( players[i]->splitScreenType == Player::SPLITSCREEN_VERTICAL )
					{
						// divide screen vertically
						players[i]->camera().winx = i * xres / 2;
						players[i]->camera().winy = 0;
						players[i]->camera().winw = xres / 2;
						players[i]->camera().winh = yres;
					}
					else
					{
						// divide screen horizontally
						players[i]->camera().winx = 0;
						players[i]->camera().winy = i * yres / 2;
						players[i]->camera().winw = xres;
						players[i]->camera().winh = yres / 2;
					}
				}
				else if ( playercount >= 3 )
				{
					// divide screen into quadrants
					players[i]->camera().winx = (i % 2) * xres / 2;
					players[i]->camera().winy = (i / 2) * yres / 2;
					players[i]->camera().winw = xres / 2;
					players[i]->camera().winh = yres / 2;
				}
			}

			inputs.getVirtualMouse(i)->x = players[i]->camera_x1() + players[i]->camera_width() / 2;
			inputs.getVirtualMouse(i)->y = players[i]->camera_y1() + players[i]->camera_height() / 2;

			if ( i > 0 )
			{
				stats[i]->sex = static_cast<sex_t>(rand() % 2);
				stats[i]->appearance = rand() % 18;
				stats[i]->clearStats();
				client_classes[i] = rand() % (CLASS_MONK + 1);//NUMCLASSES;
				stats[i]->playerRace = RACE_HUMAN;
				if ( enabledDLCPack1 || enabledDLCPack2 )
				{
					stats[i]->playerRace = rand() % NUMPLAYABLERACES;
					if ( !enabledDLCPack1 )
					{
						while ( stats[i]->playerRace == RACE_SKELETON || stats[i]->playerRace == RACE_VAMPIRE
							|| stats[i]->playerRace == RACE_SUCCUBUS || stats[i]->playerRace == RACE_GOATMAN )
						{
							stats[i]->playerRace = rand() % NUMPLAYABLERACES;
						}
					}
					else if ( !enabledDLCPack2 )
					{
						while ( stats[i]->playerRace == RACE_AUTOMATON || stats[i]->playerRace == RACE_GOBLIN
							|| stats[i]->playerRace == RACE_INCUBUS || stats[i]->playerRace == RACE_INSECTOID )
						{
							stats[i]->playerRace = rand() % NUMPLAYABLERACES;
						}
					}
					if ( stats[i]->playerRace == RACE_INCUBUS )
					{
						stats[i]->sex = MALE;
					}
					else if ( stats[i]->playerRace == RACE_SUCCUBUS )
					{
						stats[i]->sex = FEMALE;
					}

					if ( stats[i]->playerRace == RACE_HUMAN )
					{
						client_classes[i] = rand() % (NUMCLASSES);
						if ( !enabledDLCPack1 )
						{
							while ( client_classes[i] == CLASS_CONJURER || client_classes[i] == CLASS_ACCURSED
								|| client_classes[i] == CLASS_MESMER || client_classes[i] == CLASS_BREWER )
							{
								client_classes[i] = rand() % (NUMCLASSES);
							}
						}
						else if ( !enabledDLCPack2 )
						{
							while ( client_classes[i] == CLASS_HUNTER || client_classes[i] == CLASS_SHAMAN
								|| client_classes[i] == CLASS_PUNISHER || client_classes[i] == CLASS_MACHINIST )
							{
								client_classes[i] = rand() % (NUMCLASSES);
							}
						}
						stats[i]->appearance = rand() % 18;
					}
					else
					{
						client_classes[i] = rand() % (CLASS_MONK + 2);
						if ( client_classes[i] > CLASS_MONK )
						{
							client_classes[i] = CLASS_MONK + stats[i]->playerRace; // monster specific classes.
						}
						stats[i]->appearance = 0;
					}
				}
				else
				{
					stats[i]->playerRace = RACE_HUMAN;
					stats[i]->appearance = rand() % 18;
				}
				strcpy(stats[i]->name, randomPlayerNamesFemale[rand() % randomPlayerNamesFemale.size()].c_str());
				bool oldIntro = intro;
				intro = true; // so initClass doesn't add items to hotbar.
				initClass(i);
				intro = oldIntro;
			}
		}
		});

	static ConsoleCommand ccmd_gamepad_deadzone("/gamepad_deadzone", []CCMD{
	    if (argc < 2)
	    {
	        return;
	    }
		gamepad_deadzone = atoi(argv[1]);
		//Ensure its value is in range.
		gamepad_deadzone = std::max(gamepad_deadzone, 0);
		printlog("Controller deadzone is %d.", gamepad_deadzone);
		});

	static ConsoleCommand ccmd_gamepad_trigger_deadzone("/gamepad_trigger_deadzone", []CCMD{
	    if (argc < 2)
	    {
	        return;
	    }
		gamepad_trigger_deadzone = atoi(argv[1]);
		//Ensure its value is in range.
		gamepad_trigger_deadzone = std::max(gamepad_trigger_deadzone, 0);
		printlog("Controller trigger deadzone is %d.", gamepad_trigger_deadzone);
		});

	static ConsoleCommand ccmd_gamepad_leftx_sensitivity("/gamepad_leftx_sensitivity", []CCMD{
	    if (argc < 2)
	    {
	        return;
	    }
		gamepad_leftx_sensitivity = atoi(argv[1]);
		//Ensure its value is in range.
		gamepad_leftx_sensitivity = std::max(gamepad_leftx_sensitivity, 1);
		printlog("Controller leftx sensitivity is %d.", gamepad_leftx_sensitivity);
		});

	static ConsoleCommand ccmd_gamepad_lefty_sensitivity("/gamepad_lefty_sensitivity", []CCMD{
	    if (argc < 2)
	    {
	        return;
	    }
		gamepad_lefty_sensitivity = atoi(argv[1]);
		//Ensure its value is in range.
		gamepad_lefty_sensitivity = std::max(gamepad_lefty_sensitivity, 1);
		printlog("Controller lefty sensitivity is %d.", gamepad_lefty_sensitivity);
		});

	static ConsoleCommand ccmd_gamepad_rightx_sensitivity("/gamepad_rightx_sensitivity", []CCMD{
	    if (argc < 2)
	    {
	        return;
	    }
		gamepad_rightx_sensitivity = atoi(argv[1]);
		//Ensure its value is in range.
		gamepad_rightx_sensitivity = std::max(gamepad_rightx_sensitivity, 1);
		printlog("Controller rightx sensitivity is %d.", gamepad_rightx_sensitivity);
		});

	static ConsoleCommand ccmd_gamepad_righty_sensitivity("/gamepad_righty_sensitivity", []CCMD{
	    if (argc < 2)
	    {
	        return;
	    }
		gamepad_righty_sensitivity = atoi(argv[1]);
		//Ensure its value is in range.
		gamepad_righty_sensitivity = std::max(gamepad_righty_sensitivity, 1);
		printlog("Controller righty sensitivity is %d.", gamepad_righty_sensitivity);
		});

	static ConsoleCommand ccmd_gamepad_menux_sensitivity("/gamepad_menux_sensitivity", []CCMD{
	    if (argc < 2)
	    {
	        return;
	    }
		gamepad_menux_sensitivity = atoi(argv[1]);
		//Ensure its value is in range.
		gamepad_menux_sensitivity = std::max(gamepad_menux_sensitivity, 1);
		printlog("Controller menux sensitivity is %d.", gamepad_menux_sensitivity);
		});

	static ConsoleCommand ccmd_gamepad_menuy_sensitivity("/gamepad_menuy_sensitivity", []CCMD{
	    if (argc < 2)
	    {
	        return;
	    }
		gamepad_menuy_sensitivity = atoi(argv[1]);
		//Ensure its value is in range.
		gamepad_menuy_sensitivity = std::max(gamepad_menuy_sensitivity, 1);
		printlog("Controller menuy sensitivity is %d.", gamepad_menuy_sensitivity);
		});

	static ConsoleCommand ccmd_gamepad_leftx_invert("/gamepad_leftx_invert", []CCMD{
		gamepad_leftx_invert = true;
		});

	static ConsoleCommand ccmd_gamepad_lefty_invert("/gamepad_lefty_invert", []CCMD{
		gamepad_lefty_invert = true;
		});

	static ConsoleCommand ccmd_gamepad_rightx_invert("/gamepad_rightx_invert", []CCMD{
		gamepad_rightx_invert = true;
		});

	static ConsoleCommand ccmd_gamepad_righty_invert("/gamepad_righty_invert", []CCMD{
		gamepad_righty_invert = true;
		});

	static ConsoleCommand ccmd_gamepad_menux_invert("/gamepad_menux_invert", []CCMD{
		gamepad_menux_invert = true;
		});

	static ConsoleCommand ccmd_gamepad_menuy_invert("/gamepad_menuy_invert", []CCMD{
		gamepad_menuy_invert = true;
		});

	static ConsoleCommand ccmd_numgold("/numgold", []CCMD{
		for ( unsigned i = 0; i < MAXPLAYERS; ++i )
		{
			if ( client_disconnected[i] )
			{
				continue;
			}
			messagePlayer(clientnum, MESSAGE_MISC, "Player %d has %d gold.", i, stats[i]->GOLD);
		}
		});

	static ConsoleCommand ccmd_gold("/gold", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}
		if ( multiplayer != SINGLE )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[299]);
			return;
		}

        if (argc < 2)
        {
            return;
        }
		int amount = atoi(argv[1]);
		stats[clientnum]->GOLD += amount;
		stats[clientnum]->GOLD = std::max(stats[clientnum]->GOLD, 0);

		messagePlayer(clientnum, MESSAGE_MISC, "Giving %d gold pieces.", amount);
		});

	static ConsoleCommand ccmd_dropgold("/dropgold", []CCMD{
		if (argc < 2)
        {
            return;
        }
		int amount = atoi(argv[1]);
		if ( !stats[clientnum] )
		{
			return;
		}
		else if ( stats[clientnum]->HP <= 0 || !players[clientnum] || !players[clientnum]->entity )
		{
			return;
		}
		if ( stats[clientnum]->GOLD < 0 )
		{
			stats[clientnum]->GOLD = 0;
		}

		//Drop gold.
		int x = std::min<int>(std::max(0, (int)(players[clientnum]->entity->x / 16)), map.width - 1);
		int y = std::min<int>(std::max(0, (int)(players[clientnum]->entity->y / 16)), map.height - 1);
		if ( map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height] )
		{
			if ( stats[clientnum]->GOLD - amount < 0 )
			{
				amount = stats[clientnum]->GOLD;
			}
			if ( amount == 0 )
			{
				messagePlayer(clientnum, MESSAGE_INVENTORY, language[2593]);
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
				playSoundEntity(players[clientnum]->entity, 242 + rand() % 4, 64);
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
			messagePlayer(clientnum, MESSAGE_INVENTORY, language[2594], amount);
		}
		else
		{
			messagePlayer(clientnum, MESSAGE_INVENTORY | MESSAGE_MISC, language[4085]); // invalid location to drop gold
		}
		});

	static ConsoleCommand ccmd_minotaurlevel("/minotaurlevel", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}
		if ( multiplayer != SINGLE )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[299]);
			return;
		}

		if ( !minotaurlevel )
		{
			minotaurlevel = 1;
			createMinotaurTimer(players[0]->entity, &map);
		}
		});

	static ConsoleCommand ccmd_minotaurnow("/minotaurnow", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}
		if ( multiplayer != SINGLE )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[299]);
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
		});

	static ConsoleCommand ccmd_levelskill("/levelskill", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}
		if ( multiplayer != SINGLE )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[299]);
			return;
		}

		if (argc < 2)
        {
            return;
        }
		int skill = atoi(argv[1]);
		if ( skill >= NUMPROFICIENCIES )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[3239]); //Skill out of range.
		}
		else
		{
			for ( int i = 0; i < 10; ++i )
			{
				players[clientnum]->entity->increaseSkill(skill);
			}
		}
		});

	static ConsoleCommand ccmd_maplevel("/maplevel", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}
		if ( multiplayer != SINGLE )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[299]);
			return;
		}

		messagePlayer(clientnum, MESSAGE_MISC, language[412]);

		mapLevel(clientnum);
		});

	static ConsoleCommand ccmd_drunky("/drunky", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}
		if ( multiplayer != SINGLE )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[299]);
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
		});

	static ConsoleCommand ccmd_maxskill("/maxskill", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}
		if ( multiplayer != SINGLE )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[299]);
			return;
		}

		if (argc < 2)
        {
            return;
        }
		int skill = atoi(argv[1]);
		if ( skill >= NUMPROFICIENCIES )
		{
			messagePlayer(clientnum, MESSAGE_MISC, "Invalid skill ID"); //Skill out of range.
		}
		else
		{
			for ( int i = players[clientnum]->entity->getStats()->PROFICIENCIES[skill]; i < 100; ++i )
			{
				players[clientnum]->entity->increaseSkill(skill);
			}
		}
		});

	static ConsoleCommand ccmd_reloadlimbs("/reloadlimbs", []CCMD{
		int x;
		File* fp;
		bool success = true;

		if ( !autoLimbReload )
		{
			messagePlayer(clientnum, MESSAGE_MISC, "Reloading limb offsets from limbs.txt files...");
		}

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
			for ( line = 1; !fp->eof(); line++ )
			{
				char data[256];
				int limb = 20;
				int dummy;

				// read line from file
				fp->gets(data, 256);

				// skip blank and comment lines
				if ( data[0] == '\n' || data[0] == '\r' || data[0] == '#' )
				{
					continue;
				}

				// process line
				if ( sscanf(data, "%d", &limb) != 1 || limb >= 20 || limb < 0 )
				{
					messagePlayer(clientnum, MESSAGE_MISC, "warning: syntax error in '%s':%d\n invalid limb index!", filename, line);
					printlog("warning: syntax error in '%s':%d\n invalid limb index!\n", filename, line);
					success = false;
					continue;
				}
				if ( sscanf(data, "%d %f %f %f\n", &dummy, &limbs[c][limb][0], &limbs[c][limb][1], &limbs[c][limb][2]) != 4 )
				{
					messagePlayer(clientnum, MESSAGE_MISC, "warning: syntax error in '%s':%d\n invalid limb offsets!", filename, line);
					printlog("warning: syntax error in '%s':%d\n invalid limb offsets!\n", filename, line);
					success = false;
					continue;
				}
			}

			// close file
			FileIO::close(fp);
		}
		if ( success && !autoLimbReload )
		{
			messagePlayer(clientnum, MESSAGE_MISC, "Successfully reloaded all limbs.txt!");
		}
		});

	static ConsoleCommand ccmd_animspeed("/animspeed", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}
		if ( multiplayer != SINGLE )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[299]);
			return;
		}

		if (argc < 2)
        {
            return;
        }
		int speed = atoi(argv[1]);
		monsterGlobalAnimationMultiplier = speed;
		messagePlayer(clientnum, MESSAGE_MISC, "Changed animation speed multiplier to %f.", speed / 10.0);
		});

	static ConsoleCommand ccmd_atkspeed("/atkspeed", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}
		if ( multiplayer != SINGLE )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[299]);
			return;
		}

		if (argc < 2)
        {
            return;
        }
		int speed = atoi(argv[1]);
		monsterGlobalAttackTimeMultiplier = speed;
		messagePlayer(clientnum, MESSAGE_MISC, "Changed attack speed multiplier to %d.", speed);
		});

	static ConsoleCommand ccmd_loadmod("/loadmod", []CCMD{
	    for (int c = 1; c < argc; ++c)
	    {
		    std::string cmd = argv[c];
		    std::size_t dirfind = cmd.find("dir:");
		    std::size_t namefind = cmd.find("name:");
		    std::size_t fileidFind = cmd.find("fileid:");
	    }
	    std::string modname;
	    if ( dirfind != std::string::npos && namefind != std::string::npos &&  )
	    {
	        if ( fileidFind == std::string::npos )
	        {
		        std::string directory = cmd.substr(dirfind + 4, namefind - (dirfind + 5));
		        modname = cmd.substr(namefind + 5);
		        modname = modname.substr(0, modname.length() - 1);
		        printlog("[Mods]: Adding mod \"%s\" in path \"%s\"", directory.c_str(), modname.c_str());
		        gamemods_mountedFilepaths.push_back(std::make_pair(directory, modname));
		        gamemods_modelsListRequiresReload = true;
		        gamemods_soundListRequiresReload = true;
	        }
#ifdef STEAMWORKS
	        else
	        {
		        std::string directory = cmd.substr(dirfind + 4, namefind - (dirfind + 5));
		        modname = cmd.substr(namefind + 5, fileidFind - (namefind + 6));
		        printlog("[Mods]: Adding mod \"%s\" in path \"%s\"", directory.c_str(), modname.c_str());
		        gamemods_mountedFilepaths.push_back(std::make_pair(directory, modname));
		        gamemods_modelsListRequiresReload = true;
		        gamemods_soundListRequiresReload = true;

		        uint64 id = atoi(cmd.substr(fileidFind + 7).c_str());
		        gamemods_workshopLoadedFileIDMap.push_back(std::make_pair(modname, id));
		        printlog("[Mods]: Steam Workshop mod file ID added for previous entry:%lld", id);
	        }
#endif
	    }
		});

	static ConsoleCommand ccmd_muteaudiofocuslost("/muteaudiofocuslost", []CCMD{
		mute_audio_on_focus_lost = (mute_audio_on_focus_lost == false);
		});

	static ConsoleCommand ccmd_muteplayermonstersounds("/muteplayermonstersounds", []CCMD{
		mute_player_monster_sounds = (mute_player_monster_sounds == false);
		});

	static ConsoleCommand ccmd_minimaptransparencyfg("/minimaptransparencyfg", []CCMD{
		if (argc < 2)
        {
            return;
        }
		minimapTransparencyForeground = atoi(argv[1]);
		minimapTransparencyForeground = std::min(std::max<int>(0, minimapTransparencyForeground), 100);

		});

	static ConsoleCommand ccmd_minimaptransparencybg("/minimaptransparencybg", []CCMD{
		minimapTransparencyBackground = atoi(argv[1]);
		minimapTransparencyBackground = std::min(std::max<int>(0, minimapTransparencyBackground), 100);

		});

	static ConsoleCommand ccmd_minimapscale("/minimapscale", []CCMD{
		if (argc < 2)
        {
            return;
        }
		minimapScale = atoi(argv[1]);
		minimapScale = std::min(std::max<int>(2, minimapScale), 16);
		});

	static ConsoleCommand ccmd_minimapobjectzoom("/minimapobjectzoom", []CCMD{
		if (argc < 2)
        {
            return;
        }
		minimapObjectZoom = atoi(argv[1]);
		minimapObjectZoom = std::min(std::max<int>(0, minimapObjectZoom), 4);
		});

	static ConsoleCommand ccmd_uiscale_charsheet("/uiscale_charsheet", []CCMD{
		uiscale_charactersheet = !uiscale_charactersheet;
		});

	static ConsoleCommand ccmd_uiscale_skillsheet("/uiscale_skillsheet", []CCMD{
		uiscale_skillspage = !uiscale_skillspage;
		});

	static ConsoleCommand ccmd_hidestatusbar("/hidestatusbar", []CCMD{
		//hide_statusbar = !hide_statusbar;
		});

	static ConsoleCommand ccmd_hideplayertags("/hideplayertags", []CCMD{
		hide_playertags = !hide_playertags;
		});

	static ConsoleCommand ccmd_showskillvalues("/showskillvalues", []CCMD{
		show_skill_values = !show_skill_values;
		});

	static ConsoleCommand ccmd_disablenetworkmultithreading("/disablenetworkmultithreading", []CCMD{
		disableMultithreadedSteamNetworking = true;// !disableMultithreadedSteamNetworking;
		});

	static ConsoleCommand ccmd_autolimbreload("/autolimbreload", []CCMD{
		autoLimbReload = !autoLimbReload;
		});

	static ConsoleCommand ccmd_togglesecretlevel("/togglesecretlevel", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}
		if ( multiplayer != SINGLE )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[299]);
			return;
		}
		secretlevel = (secretlevel == false);
		});

	static ConsoleCommand ccmd_seteffect("/seteffect", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}
		if ( multiplayer != SINGLE )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[299]);
			return;
		}

		if (argc < 2)
        {
            return;
        }
		int effect = atoi(argv[1]);
		if ( effect >= NUMEFFECTS || effect < 0 )
		{
			return;
		}
		else
		{
			players[clientnum]->entity->setEffect(effect, true, 500, true);
		}
		});

	static ConsoleCommand ccmd_levelsummon("/levelsummon", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}
		for ( node_t* node = map.creatures->first; node != nullptr; node = node->next )
		{
			Entity* entity = (Entity*)node->element;
			if ( entity && entity->behavior == &actMonster && entity->monsterAllySummonRank != 0 )
			{
				Stat* entityStats = entity->getStats();
				if ( entityStats )
				{
					entityStats->EXP += 100;
				}
			}
		}
		return;
		});

	static ConsoleCommand ccmd_brawlermode("/brawlermode", []CCMD{
		achievementBrawlerMode = !achievementBrawlerMode;
		if ( achievementBrawlerMode && conductGameChallenges[CONDUCT_BRAWLER] )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[2995]);
		}
		else if ( achievementBrawlerMode && !conductGameChallenges[CONDUCT_BRAWLER] )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[2998]);
		}
		else if ( !achievementBrawlerMode )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[2996]);
		}
		});

	static ConsoleCommand ccmd_rangermode("/rangermode", []CCMD{
		int player = -1;
		if ( argc > 1 )
		{
			player = atoi(argv[1]);
		}
		else
		{
			player = 0;
		}

		if ( multiplayer == CLIENT )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[284]);
			return;
		}

		achievementRangedMode[player] = !achievementRangedMode[player];
		if ( multiplayer == SERVER )
		{
			if ( player != clientnum )
			{
				if ( achievementRangedMode[player] )
				{
					messagePlayer(clientnum, MESSAGE_MISC, language[3926], player);
				}
				else
				{
					messagePlayer(clientnum, MESSAGE_MISC, language[3925], player);
				}
			}
		}
		if ( achievementRangedMode[player] && !playerFailedRangedOnlyConduct[player] )
		{
			messagePlayer(player, MESSAGE_MISC, language[3921]);
		}
		else if ( achievementRangedMode[player] && playerFailedRangedOnlyConduct[player] )
		{
			messagePlayer(player, MESSAGE_MISC, language[3924]);
		}
		else if ( !achievementRangedMode[player] )
		{
			messagePlayer(player, MESSAGE_MISC, language[3922]);
		}
		});

	static ConsoleCommand ccmd_gimmepotions("/gimmepotions", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}

		if ( multiplayer != SINGLE )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[299]);
			return;
		}

		std::vector<int> potionChances =
		{
			1,	//POTION_WATER,
			1,	//POTION_BOOZE,
			1,	//POTION_JUICE,
			1,	//POTION_SICKNESS,
			1,	//POTION_CONFUSION,
			1,	//POTION_EXTRAHEALING,
			1,	//POTION_HEALING,
			1,	//POTION_CUREAILMENT,
			1,	//POTION_BLINDNESS,
			1,	//POTION_RESTOREMAGIC,
			1,	//POTION_INVISIBILITY,
			1,	//POTION_LEVITATION,
			1,	//POTION_SPEED,
			1,	//POTION_ACID,
			1,	//POTION_PARALYSIS,
			1,	//POTION_POLYMORPH
		};

		std::discrete_distribution<> potionDistribution(potionChances.begin(), potionChances.end());
		for ( int i = 0; i < 10; ++i )
		{
			auto generatedPotion = potionStandardAppearanceMap.at(potionDistribution(fountainSeed));
			Item* potion = newItem(static_cast<ItemType>(generatedPotion.first), static_cast<Status>(SERVICABLE + rand() % 2),
				0, 1, generatedPotion.second, true, nullptr);
			itemPickup(clientnum, potion);
			//free(potion);
		}
		});

	static ConsoleCommand ccmd_hungoverstats("/hungoverstats", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}

		if ( multiplayer != SINGLE )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[299]);
			return;
		}

		messagePlayer(clientnum, MESSAGE_MISC, "Hungover Active: %d, Time to go: %d, Drunk Active: %d, Drunk time: %d",
			stats[clientnum]->EFFECTS[EFF_WITHDRAWAL], stats[clientnum]->EFFECTS_TIMERS[EFF_WITHDRAWAL],
			stats[clientnum]->EFFECTS[EFF_DRUNK], stats[clientnum]->EFFECTS_TIMERS[EFF_DRUNK]);
		return;
		});

	static ConsoleCommand ccmd_debugtimers("/debugtimers", []CCMD{
		logCheckMainLoopTimers = !logCheckMainLoopTimers;
		});

	static ConsoleCommand ccmd_entityfreeze("/entityfreeze", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}
		gameloopFreezeEntities = !gameloopFreezeEntities;
		});

	static ConsoleCommand ccmd_tickrate("/tickrate", []CCMD{
		if (argc < 2)
        {
            return;
        }
		networkTickrate = atoi(argv[1]);
		networkTickrate = std::max<Uint32>(1, networkTickrate);
		messagePlayer(clientnum, MESSAGE_MISC, "Set tickrate to %d, network processing allowed %3.0f percent of frame limit interval. Default value 2.",
			networkTickrate, 100.f / networkTickrate);
		});

	static ConsoleCommand ccmd_disablenetcodefpslimit("/disablenetcodefpslimit", []CCMD{
		disableFPSLimitOnNetworkMessages = !disableFPSLimitOnNetworkMessages;
		});

	static ConsoleCommand ccmd_allspells1("/allspells1", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}

		for ( auto it = allGameSpells.begin(); it != allGameSpells.begin() + 29; ++it )
		{
			spell_t* spell = *it;
			bool learned = addSpell(spell->ID, clientnum, true);
		}
		return;
		});

	static ConsoleCommand ccmd_setmapseed("/setmapseed", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}
		if ( multiplayer == CLIENT )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[284]);
			return;
		}

		if (argc < 2)
        {
            return;
        }
		Uint32 newseed = atoi(argv[1]);
		forceMapSeed = newseed;
		messagePlayer(clientnum, MESSAGE_MISC, "Set next map seed to: %d", forceMapSeed);
		return;
		});

	static ConsoleCommand ccmd_greaseme("/greaseme", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}
		if ( multiplayer == CLIENT )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[284]);
			return;
		}
		if ( players[clientnum] && players[clientnum]->entity )
		{
			players[clientnum]->entity->setEffect(EFF_GREASY, true, TICKS_PER_SECOND * 20, false);
		}
		});

	static ConsoleCommand ccmd_gimmearrows("/gimmearrows", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}
		for ( int i = QUIVER_SILVER; i <= QUIVER_HUNTING; ++i )
		{
			dropItem(newItem(static_cast<ItemType>(i), EXCELLENT, 0, 25 + rand() % 26, rand(), true, &stats[clientnum]->inventory), 0);
		}
		});

	static ConsoleCommand ccmd_gimmescrap("/gimmescrap", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}
		dropItem(newItem(TOOL_METAL_SCRAP, EXCELLENT, 0, 100, rand(), true, &stats[clientnum]->inventory), 0);
		dropItem(newItem(TOOL_MAGIC_SCRAP, EXCELLENT, 0, 100, rand(), true, &stats[clientnum]->inventory), 0);
		dropItem(newItem(TOOL_TINKERING_KIT, EXCELLENT, 0, 1, rand(), true, &stats[clientnum]->inventory), 0);
		});

	static ConsoleCommand ccmd_gimmerobots("/gimmerobots", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}
		dropItem(newItem(TOOL_GYROBOT, EXCELLENT, 0, 10, rand(), true, &stats[clientnum]->inventory), 0);
		dropItem(newItem(TOOL_DUMMYBOT, EXCELLENT, 0, 10, rand(), true, &stats[clientnum]->inventory), 0);
		dropItem(newItem(TOOL_SENTRYBOT, EXCELLENT, 0, 10, rand(), true, &stats[clientnum]->inventory), 0);
		dropItem(newItem(TOOL_SPELLBOT, EXCELLENT, 0, 10, rand(), true, &stats[clientnum]->inventory), 0);
		});

	static ConsoleCommand ccmd_toggletinkeringlimits("/toggletinkeringlimits", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}
		overrideTinkeringLimit = !overrideTinkeringLimit;
		if ( overrideTinkeringLimit )
		{
			messagePlayer(clientnum, MESSAGE_MISC, "Disabled tinkering bot limit");
		}
		else
		{
			messagePlayer(clientnum, MESSAGE_MISC, "Re-enabled tinkering bot limit");
		}
		});

	static ConsoleCommand ccmd_setdecoyrange("/setdecoyrange", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}
		if ( multiplayer == CLIENT )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[284]);
			return;
		}
		if (argc < 2)
        {
            return;
        }
		decoyBoxRange = atoi(argv[1]);
		messagePlayer(clientnum, MESSAGE_MISC, "Set decoy range to %d", decoyBoxRange);
		});

	static ConsoleCommand ccmd_gimmegoblinbooks("/gimmegoblinbooks", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}
		for ( int i = 0; i < NUM_SPELLS; ++i )
		{
			int spellbook = getSpellbookFromSpellID(i);
			dropItem(newItem(static_cast<ItemType>(spellbook), DECREPIT, -1, 1, rand(), true, &stats[clientnum]->inventory), 0);
		}
		});

	static ConsoleCommand ccmd_unsetdlc2achievements("/unsetdlc2achievements", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}
#ifdef STEAMWORKS
		steamUnsetAchievement("BARONY_ACH_TAKING_WITH");
		steamUnsetAchievement("BARONY_ACH_TELEFRAG");
		steamUnsetAchievement("BARONY_ACH_FASCIST");
		steamUnsetAchievement("BARONY_ACH_REAL_BOY");
		steamUnsetAchievement("BARONY_ACH_OVERCLOCKED");
		steamUnsetAchievement("BARONY_ACH_TRASH_COMPACTOR");
		steamUnsetAchievement("BARONY_ACH_BOILERPLATE_BARON");
		steamUnsetAchievement("BARONY_ACH_PIMPIN");
		steamUnsetAchievement("BARONY_ACH_BAD_BEAUTIFUL");
		steamUnsetAchievement("BARONY_ACH_SERIAL_THRILLA");
		steamUnsetAchievement("BARONY_ACH_TRADITION");
		steamUnsetAchievement("BARONY_ACH_BAD_BOY_BARON");
		steamUnsetAchievement("BARONY_ACH_POP_QUIZ");
		steamUnsetAchievement("BARONY_ACH_DYSLEXIA");
		steamUnsetAchievement("BARONY_ACH_SAVAGE");
		steamUnsetAchievement("BARONY_ACH_TRIBE_SUBSCRIBE");
		steamUnsetAchievement("BARONY_ACH_BAYOU_BARON");
		steamUnsetAchievement("BARONY_ACH_GASTRIC_BYPASS");
		steamUnsetAchievement("BARONY_ACH_BOOKWORM");
		steamUnsetAchievement("BARONY_ACH_FLUTTERSHY");
		steamUnsetAchievement("BARONY_ACH_MONARCH");
		steamUnsetAchievement("BARONY_ACH_BUGGAR_BARON");
		steamUnsetAchievement("BARONY_ACH_TIME_TO_PLAN");
		steamUnsetAchievement("BARONY_ACH_WONDERFUL_TOYS");
		steamUnsetAchievement("BARONY_ACH_SUPER_SHREDDER");
		steamUnsetAchievement("BARONY_ACH_UTILITY_BELT");
		steamUnsetAchievement("BARONY_ACH_FIXER_UPPER");
		steamUnsetAchievement("BARONY_ACH_TORCHERER");
		steamUnsetAchievement("BARONY_ACH_LEVITANT_LACKEY");
		steamUnsetAchievement("BARONY_ACH_GOODNIGHT_SWEET_PRINCE");
		steamUnsetAchievement("BARONY_ACH_MANY_PEDI_PALP");
		steamUnsetAchievement("BARONY_ACH_5000_SECOND_RULE");
		steamUnsetAchievement("BARONY_ACH_FORUM_TROLL");
		steamUnsetAchievement("BARONY_ACH_SOCIAL_BUTTERFLY");
		steamUnsetAchievement("BARONY_ACH_ROLL_THE_BONES");
		steamUnsetAchievement("BARONY_ACH_COWBOY_FROM_HELL");
		steamUnsetAchievement("BARONY_ACH_IRONIC_PUNISHMENT");
		steamUnsetAchievement("BARONY_ACH_SELF_FLAGELLATION");
		steamUnsetAchievement("BARONY_ACH_OHAI_MARK");
		steamUnsetAchievement("BARONY_ACH_CHOPPING_BLOCK");
		steamUnsetAchievement("BARONY_ACH_ITS_A_LIVING");
		steamUnsetAchievement("BARONY_ACH_ARSENAL");
		steamUnsetAchievement("BARONY_ACH_IF_YOU_LOVE_SOMETHING");
		steamUnsetAchievement("BARONY_ACH_GUDIPARIAN_BAZI");
		steamUnsetAchievement("BARONY_ACH_STRUNG_OUT");
		steamUnsetAchievement("BARONY_ACH_FELL_BEAST");
		steamUnsetAchievement("BARONY_ACH_PLEASE_HOLD");
		steamUnsetAchievement("BARONY_ACH_SWINGERS");
		steamUnsetAchievement("BARONY_ACH_COLD_BLOODED");
		steamUnsetAchievement("BARONY_ACH_SOULLESS");
		steamUnsetAchievement("BARONY_ACH_TRIBAL");
		steamUnsetAchievement("BARONY_ACH_MANAGEMENT_TEAM");
		steamUnsetAchievement("BARONY_ACH_SOCIOPATHS");
		steamUnsetAchievement("BARONY_ACH_FACES_OF_DEATH");
		steamUnsetAchievement("BARONY_ACH_SURVIVALISTS");
		steamUnsetAchievement("BARONY_ACH_I_WANT_IT_ALL");
		steamUnsetAchievement("BARONY_ACH_RUST_IN_PEACE");
		steamUnsetAchievement("BARONY_ACH_MACHINE_HEAD");
		steamUnsetAchievement("BARONY_ACH_RAGE_AGAINST");
		steamUnsetAchievement("BARONY_ACH_GUERILLA_RADIO");
		steamUnsetAchievement("BARONY_ACH_BOMBTRACK");
		steamUnsetAchievement("BARONY_ACH_CALM_LIKE_A_BOMB");
		steamUnsetAchievement("BARONY_ACH_CAUGHT_IN_A_MOSH");
		steamUnsetAchievement("BARONY_ACH_SPICY");
		for ( int i = STEAM_STAT_TRASH_COMPACTOR; i < 43; ++i )
		{
			g_SteamStats[i].m_iValue = 0;
			SteamUserStats()->SetStat(g_SteamStats[i].m_pchStatName, 0);
		}
		SteamUserStats()->StoreStats();
#endif // STEAMWORKS
		});

	static ConsoleCommand ccmd_gimmebombs("/gimmebombs", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}
		dropItem(newItem(TOOL_BOMB, EXCELLENT, 0, 10, rand(), true, &stats[clientnum]->inventory), 0);
		dropItem(newItem(TOOL_FREEZE_BOMB, EXCELLENT, 0, 10, rand(), true, &stats[clientnum]->inventory), 0);
		dropItem(newItem(TOOL_TELEPORT_BOMB, EXCELLENT, 0, 10, rand(), true, &stats[clientnum]->inventory), 0);
		dropItem(newItem(TOOL_SLEEP_BOMB, EXCELLENT, 0, 10, rand(), true, &stats[clientnum]->inventory), 0);
		});

	static ConsoleCommand ccmd_showhunger("/showhunger", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}
		messagePlayer(clientnum, MESSAGE_MISC, "Hunger value: %d", stats[clientnum]->HUNGER);
		});

	static ConsoleCommand ccmd_disablemouserotationlimit("/disablemouserotationlimit", []CCMD{
		disablemouserotationlimit = (disablemouserotationlimit == false);
		});

	static ConsoleCommand ccmd_usecamerasmoothing("/usecamerasmoothing", []CCMD{
		usecamerasmoothing = (usecamerasmoothing == false);
		});

	static ConsoleCommand ccmd_lightupdate("/lightupdate", []CCMD{
		if (argc < 2)
        {
            return;
        }
		globalLightSmoothingRate = atoi(argv[1]);
		});

	static ConsoleCommand ccmd_dumpnetworkdata("/dumpnetworkdata", []CCMD{
		for ( auto element : DebugStats.networkPackets )
		{
			printlog("Packet: %s | %d", element.second.first.c_str(), element.second.second);
		}
		});

	static ConsoleCommand ccmd_dumpentudata("/dumpentudata", []CCMD{
		for ( auto element : DebugStats.entityUpdatePackets )
		{
			printlog("Sprite: %d | %d", element.first, element.second);
		}
		});

	static ConsoleCommand ccmd_borderless("/borderless", []CCMD{
		borderless = (!borderless);
		});

	static ConsoleCommand ccmd_jsonexportmonster("/jsonexportmonster", []CCMD{
		if (argc < 2)
        {
            return;
        }
		auto name = argv[1];
		int creature = NOTHING;

		for ( int i = 1; i < NUMMONSTERS; ++i )   //Start at 1 because 0 is a nothing.
		{
			if ( strstr(getMonsterLocalizedName((Monster)i).c_str(), name) )
			{
				creature = i;
				break;
			}
		}

		if ( creature != NOTHING )
		{
			Stat* monsterStats = new Stat(1000 + creature);
			monsterStatCustomManager.writeAllFromStats(monsterStats);
			delete monsterStats;
		}
		});

	static ConsoleCommand ccmd_jsonexportfromcursor("/jsonexportfromcursor", []CCMD{
		Entity* target = entityClicked(nullptr, true, clientnum, EntityClickType::ENTITY_CLICK_USE);
		if ( target )
		{
			Entity* parent = uidToEntity(target->skill[2]);
			if ( target->behavior == &actMonster || (parent && parent->behavior == &actMonster) )
			{
				// see if we selected a limb
				if ( parent )
				{
					target = parent;
				}
			}
			monsterStatCustomManager.writeAllFromStats(target->getStats());
		}
		});

	static ConsoleCommand ccmd_newui("/newui", []CCMD{
		});

	static ConsoleCommand ccmd_jsonexportgameplaymodifiers("/jsonexportgameplaymodifiers", []CCMD{
		gameplayCustomManager.writeAllToDocument();
		});

	static ConsoleCommand ccmd_jsonexportmonstercurve("/jsonexportmonstercurve", []CCMD{
		monsterCurveCustomManager.writeSampleToDocument();
		});

	static ConsoleCommand ccmd_crossplay("/crossplay", []CCMD{
#if (defined STEAMWORKS && defined USE_EOS)
		EOS.CrossplayAccountManager.autologin = true;
#endif // USE_EOS
	}
#if (defined SOUND)
	static ConsoleCommand ccmd_sfxambientvolume("/sfxambientvolume", []CCMD{
		if (argc < 2)
        {
            return;
        }
		sfxAmbientVolume = srtof(argv[1], nullptr);
		});

	static ConsoleCommand ccmd_sfxambientdynamic("/sfxambientdynamic", []CCMD{
		sfxUseDynamicAmbientVolume = !sfxUseDynamicAmbientVolume;
		if ( sfxUseDynamicAmbientVolume )
		{
			messagePlayer(clientnum, MESSAGE_MISC, "Dynamic ambient volume ON");
		}
		else
		{
			messagePlayer(clientnum, MESSAGE_MISC, "Dynamic ambient volume OFF");
		}
		});

	static ConsoleCommand ccmd_sfxenvironmentdynamic("/sfxenvironmentdynamic", []CCMD{
		sfxUseDynamicEnvironmentVolume = !sfxUseDynamicEnvironmentVolume;
		if ( sfxUseDynamicEnvironmentVolume )
		{
			messagePlayer(clientnum, MESSAGE_MISC, "Dynamic environment volume ON");
		}
		else
		{
			messagePlayer(clientnum, MESSAGE_MISC, "Dynamic environment volume OFF");
		}
		});

	static ConsoleCommand ccmd_sfxenvironmentvolume("/sfxenvironmentvolume", []CCMD{
		if (argc < 2)
        {
            return;
        }
		sfxEnvironmentVolume = strtof(argv[1], nullptr);
	}
#endif
	static ConsoleCommand ccmd_cyclekeyboard("/cyclekeyboard", []CCMD{
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			if ( inputs.bPlayerUsingKeyboardControl(i) )
			{
				if ( i + 1 >= MAXPLAYERS )
				{
					inputs.setPlayerIDAllowedKeyboard(0);
					messagePlayer(clientnum, MESSAGE_MISC, "Keyboard controlled by player %d", 0);
				}
				else
				{
					inputs.setPlayerIDAllowedKeyboard(i + 1);
					messagePlayer(clientnum, MESSAGE_MISC, "Keyboard controlled by player %d", i + 1);
				}
				break;
			}
		}
		});

	static ConsoleCommand ccmd_cyclegamepad("/cyclegamepad", []CCMD{
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			if ( inputs.hasController(i) )
			{
				int id = inputs.getControllerID(i);
				inputs.removeControllerWithDeviceID(id);
				if ( i + 1 >= MAXPLAYERS )
				{
					inputs.setControllerID(0, id);
				}
				else
				{
					inputs.setControllerID(i + 1, id);
				}
				break;
			}
		}
		});

	static ConsoleCommand ccmd_cycledeadzoneleft("/cycledeadzoneleft", []CCMD{
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			if ( inputs.hasController(i) )
			{
				switch ( inputs.getController(i)->leftStickDeadzoneType )
				{
					case GameController::DEADZONE_PER_AXIS:
						inputs.getController(i)->leftStickDeadzoneType = GameController::DEADZONE_MAGNITUDE_LINEAR;
						messagePlayer(i, MESSAGE_MISC, "Using radial deadzone on left stick.");
						break;
					case GameController::DEADZONE_MAGNITUDE_LINEAR:
						inputs.getController(i)->leftStickDeadzoneType = GameController::DEADZONE_MAGNITUDE_HALFPIPE;
						messagePlayer(i, MESSAGE_MISC, "Using curved radial deadzone on left stick.");
						break;
					case GameController::DEADZONE_MAGNITUDE_HALFPIPE:
						inputs.getController(i)->leftStickDeadzoneType = GameController::DEADZONE_PER_AXIS;
						messagePlayer(i, MESSAGE_MISC, "Using per-axis deadzone on left stick.");
						break;
				}
			}
		}
		});

	static ConsoleCommand ccmd_cycledeadzoneright("/cycledeadzoneright", []CCMD{
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			if ( inputs.hasController(i) )
			{
				switch ( inputs.getController(i)->rightStickDeadzoneType )
				{
					case GameController::DEADZONE_PER_AXIS:
						inputs.getController(i)->rightStickDeadzoneType = GameController::DEADZONE_MAGNITUDE_LINEAR;
						messagePlayer(i, MESSAGE_MISC, "Using radial deadzone on right stick.");
						break;
					case GameController::DEADZONE_MAGNITUDE_LINEAR:
						inputs.getController(i)->rightStickDeadzoneType = GameController::DEADZONE_MAGNITUDE_HALFPIPE;
						messagePlayer(i, MESSAGE_MISC, "Using curved radial deadzone on right stick.");
						break;
					case GameController::DEADZONE_MAGNITUDE_HALFPIPE:
						inputs.getController(i)->rightStickDeadzoneType = GameController::DEADZONE_PER_AXIS;
						messagePlayer(i, MESSAGE_MISC, "Using per-axis deadzone on right stick.");
						break;
				}
			}
		}
		});

	static ConsoleCommand ccmd_vibration("/vibration", []CCMD{
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			if ( inputs.hasController(i) )
			{
				inputs.getController(i)->haptics.vibrationEnabled = !inputs.getController(i)->haptics.vibrationEnabled;
				if ( inputs.getController(i)->haptics.vibrationEnabled )
				{
					messagePlayer(i, MESSAGE_MISC, "Controller vibration is enabled.");
				}
				else
				{
					messagePlayer(i, MESSAGE_MISC, "Controller vibration is disabled.");
				}
			}
		}
		});

	static ConsoleCommand ccmd_tooltipoffset("/tooltipoffset", []CCMD{
	    if (argc < 2)
	    {
	        return;
	    }
		int offset = atoi(argv[1]);
		Player::WorldUI_t::tooltipHeightOffsetZ = static_cast<real_t>(offset) / 10.0;
		messagePlayer(clientnum, MESSAGE_MISC, "Tooltip Z offset set to: %.1f", Player::WorldUI_t::tooltipHeightOffsetZ);
		});

	static ConsoleCommand ccmd_radialhotbar("/radialhotbar", []CCMD{
		players[clientnum]->hotbar.useHotbarRadialMenu = !players[clientnum]->hotbar.useHotbarRadialMenu;
		});

	static ConsoleCommand ccmd_radialhotslots("/radialhotslots", []CCMD{
	    if (argc < 2)
	    {
	        return;
	    }
		int slots = atoi(argv[1]);
		players[clientnum]->hotbar.radialHotbarSlots = slots;
		messagePlayer(clientnum, MESSAGE_MISC, "Slots in use: %d", slots);
		});

	static ConsoleCommand ccmd_facehotbar("/facehotbar", []CCMD{
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			if ( inputs.bPlayerUsingKeyboardControl(i) )
			{
				players[i]->hotbar.useHotbarFaceMenu = !players[i]->hotbar.useHotbarFaceMenu;
				messagePlayer(i, MESSAGE_MISC, "Face button hotbar: %d", players[i]->hotbar.useHotbarFaceMenu ? 1 : 0);
			}
		}
		});

	static ConsoleCommand ccmd_facebarinvert("/facebarinvert", []CCMD{
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			if ( inputs.bPlayerUsingKeyboardControl(i) )
			{
				players[i]->hotbar.faceMenuInvertLayout = !players[i]->hotbar.faceMenuInvertLayout;
				messagePlayer(i, MESSAGE_MISC, "Face button invert position: %d", players[i]->hotbar.faceMenuInvertLayout ? 1 : 0);
			}
		}
		});

	static ConsoleCommand ccmd_facebarquickcast("/facebarquickcast", []CCMD{
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			if ( inputs.bPlayerUsingKeyboardControl(i) )
			{
				players[i]->hotbar.faceMenuQuickCastEnabled = !players[i]->hotbar.faceMenuQuickCastEnabled;
				messagePlayer(i, MESSAGE_MISC, "Face button quickcast: %d", players[i]->hotbar.faceMenuQuickCastEnabled ? 1 : 0);
			}
		}
		});

	static ConsoleCommand ccmd_paperdoll("/paperdoll", []CCMD{
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			if ( inputs.bPlayerUsingKeyboardControl(i) )
			{
				players[i]->paperDoll.enabled = !players[i]->paperDoll.enabled;
				messagePlayer(i, MESSAGE_MISC, "Paper doll: %d", players[i]->paperDoll.enabled ? 1 : 0);
			}
		}
		});

	static ConsoleCommand ccmd_facebaralternate("/facebaralternate", []CCMD{
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			if ( inputs.bPlayerUsingKeyboardControl(i) )
			{
				players[i]->hotbar.faceMenuAlternateLayout = !players[i]->hotbar.faceMenuAlternateLayout;
				messagePlayer(i, MESSAGE_MISC, "Face button alternate: %d", players[i]->hotbar.faceMenuAlternateLayout ? 1 : 0);
			}
		}
		});

	static ConsoleCommand ccmd_inventorynew("/inventorynew", []CCMD{
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			if ( inputs.bPlayerUsingKeyboardControl(i) )
			{
				players[i]->inventoryUI.bNewInventoryLayout = !players[i]->inventoryUI.bNewInventoryLayout;
				players[i]->inventoryUI.resetInventory();
				messagePlayer(i, MESSAGE_MISC, "New Inventory layout: %d", players[i]->inventoryUI.bNewInventoryLayout ? 1 : 0);
			}
		}
		});

	static ConsoleCommand ccmd_worldui("/worldui", []CCMD{
		for ( int i = 0; i < MAXPLAYERS; ++i )
		{
			if ( inputs.bPlayerUsingKeyboardControl(i) )
			{
				if ( players[i]->worldUI.isEnabled() )
				{
					players[i]->worldUI.disable();
				}
				else
				{
					players[i]->worldUI.enable();
				}
			}
		}
	}

#ifndef NINTENDO
	static ConsoleCommand ccmd_ircconnect("/ircconnect", []CCMD{
		if ( IRCHandler.connect() )
		{
			messagePlayer(clientnum, MESSAGE_MISC, "[IRC]: Connected.");
		}
		else
		{
			IRCHandler.disconnect();
			messagePlayer(clientnum, MESSAGE_MISC, "[IRC]: Error connecting.");
		}
		});

	static ConsoleCommand ccmd_ircdisconnect("/ircdisconnect", []CCMD{
		IRCHandler.disconnect();
		messagePlayer(clientnum, MESSAGE_MISC, "[IRC]: Disconnected.");
		});

	static ConsoleCommand ccmd_irc("/irc", []CCMD{
	    if (argc < 2)
	    {
	        return;
	    }
		std::string message = argv[1];
		message.append("\r\n");
		IRCHandler.packetSend(message);
		messagePlayer(clientnum, MESSAGE_MISC, "[IRC]: Sent message.");
	}
#endif // !NINTENDO

	static ConsoleCommand ccmd_loadtooltips("/loadtooltips", []CCMD{
		ItemTooltips.readTooltipsFromFile();
		messagePlayer(clientnum, MESSAGE_MISC, "Reloaded item_tooltips.json");
		});

	static ConsoleCommand ccmd_reflowtext("/reflowtext", []CCMD{
		bUsePreciseFieldTextReflow = !bUsePreciseFieldTextReflow;
		messagePlayer(clientnum, MESSAGE_MISC, "Set bUsePreciseFieldTextReflow to %d", bUsePreciseFieldTextReflow);
		});

	static ConsoleCommand ccmd_selectedanimcycle("/selectedanimcycle", []CCMD{
		bUseSelectedSlotCycleAnimation = !bUseSelectedSlotCycleAnimation;
		messagePlayer(clientnum, MESSAGE_MISC, "Set bUseSelectedSlotCycleAnimation to %d", bUseSelectedSlotCycleAnimation);
		});

	static ConsoleCommand ccmd_autoloadtooltips("/autoloadtooltips", []CCMD{
		ItemTooltips.autoReload = !ItemTooltips.autoReload;
		messagePlayer(clientnum, MESSAGE_MISC, "Set auto-reload to %d for item_tooltips.json", ItemTooltips.autoReload);
		});

	static ConsoleCommand ccmd_debugtooltips("/debugtooltips", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}

		if ( multiplayer != SINGLE )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[299]);
			return;
		}
		ItemTooltips.itemDebug = !ItemTooltips.itemDebug;
		messagePlayer(clientnum, MESSAGE_MISC, "Set item-debug to %d for item_tooltips.json", ItemTooltips.itemDebug);
		});

	static ConsoleCommand ccmd_loaditems("/loaditems", []CCMD{
		ItemTooltips.readItemsFromFile();
		messagePlayer(clientnum, MESSAGE_MISC, "Reloaded items.json");
		});

	static ConsoleCommand ccmd_gimmeallpotions("/gimmeallpotions", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}

		if ( multiplayer != SINGLE )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[299]);
			return;
		}
		for ( int i = 0; i < potionStandardAppearanceMap.size(); ++i )
		{
			auto generatedPotion = potionStandardAppearanceMap.at(i);
			Item* potion = newItem(static_cast<ItemType>(generatedPotion.first), static_cast<Status>(SERVICABLE + rand() % 2),
				0, 1, generatedPotion.second, true, nullptr);
			itemPickup(clientnum, potion);
			//free(potion);
		}
		});

	static ConsoleCommand ccmd_gimmeblessedpotions("/gimmeblessedpotions", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}

		if ( multiplayer != SINGLE )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[299]);
			return;
		}
		for ( int i = 0; i < potionStandardAppearanceMap.size(); ++i )
		{
			auto generatedPotion = potionStandardAppearanceMap.at(i);
			Item* potion = newItem(static_cast<ItemType>(generatedPotion.first), static_cast<Status>(SERVICABLE + rand() % 2),
				1 + rand() % 2, 1, generatedPotion.second, true, nullptr);
			itemPickup(clientnum, potion);
			//free(potion);
		}
		});

	static ConsoleCommand ccmd_gimmecursedpotions("/gimmecursedpotions", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}

		if ( multiplayer != SINGLE )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[299]);
			return;
		}
		for ( int i = 0; i < potionStandardAppearanceMap.size(); ++i )
		{
			auto generatedPotion = potionStandardAppearanceMap.at(i);
			Item* potion = newItem(static_cast<ItemType>(generatedPotion.first), static_cast<Status>(SERVICABLE + rand() % 2),
				-2 + rand() % 2, 1, generatedPotion.second, true, nullptr);
			itemPickup(clientnum, potion);
			//free(potion);
		}
		});

	static ConsoleCommand ccmd_allspells2("/allspells2", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}

		for ( auto it = allGameSpells.begin() + 29; it != allGameSpells.end(); ++it )
		{
			spell_t* spell = *it;
			bool learned = addSpell(spell->ID, clientnum, true);
		}
		return;
		});

	static ConsoleCommand ccmd_allspells3("/allspells3", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}

		for ( auto it = allGameSpells.begin(); it != allGameSpells.end(); ++it )
		{
			spell_t* spell = *it;
			bool learned = addSpell(spell->ID, clientnum, true);
		}
		return;
		});

	static ConsoleCommand ccmd_gimmexp("/gimmexp", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}

		if ( players[clientnum] && players[clientnum]->entity )
		{
			players[clientnum]->entity->getStats()->EXP += 1 + rand() % 50;
		}
		});

	static ConsoleCommand ccmd_loadhudsettings("/loadhudsettings", []CCMD{
		loadHUDSettingsJSON();
		messagePlayer(clientnum, MESSAGE_MISC, "Reloaded HUD_settings.json");
		});

	static ConsoleCommand ccmd_loadskillsheet("/loadskillsheet", []CCMD{
		Player::SkillSheet_t::loadSkillSheetJSON();
		messagePlayer(clientnum, MESSAGE_MISC, "Reloaded skillsheet_entries.json");
		});

	static ConsoleCommand ccmd_loadcharsheet("/loadcharsheet", []CCMD{
		Player::CharacterSheet_t::loadCharacterSheetJSON();
		messagePlayer(clientnum, MESSAGE_MISC, "Reloaded charsheet_settings.json");
		});

	static ConsoleCommand ccmd_printleaderlist("/printleaderlist", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}
		Player::SkillSheet_t::generateFollowerTableForSkillsheet = true;
		messagePlayer(clientnum, MESSAGE_MISC, "On next human right click leader list will be generated.");
		});

	static ConsoleCommand ccmd_poly("/poly", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}
		if ( players[clientnum]->entity )
		{
			spellEffectPolymorph(players[clientnum]->entity, players[clientnum]->entity, true, TICKS_PER_SECOND * 60 * 2);
		}
		});

	static ConsoleCommand ccmd_sexchange("/sexchange", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}
		stats[clientnum]->sex = stats[clientnum]->sex == sex_t::MALE ? sex_t::FEMALE : sex_t::MALE;
		});

	static ConsoleCommand ccmd_appearances("/appearances", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}
		++stats[clientnum]->appearance;
		if ( stats[clientnum]->appearance >= NUMAPPEARANCES )
		{
			stats[clientnum]->appearance = 0;
		}
		});

	static ConsoleCommand ccmd_classdebug("/classdebug", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}
		client_classes[clientnum] = rand() % (CLASS_MONK + 1);//NUMCLASSES;
		});

	static ConsoleCommand ccmd_unpoly("/unpoly", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, language[277]);
			return;
		}
		if ( players[clientnum]->entity )
		{
			players[clientnum]->entity->setEffect(EFF_POLYMORPH, false, 0, true);
		}
		});

	static ConsoleCommand ccmd_usepaperdollmovement("/usepaperdollmovement", []CCMD{
		restrictPaperDollMovement = !restrictPaperDollMovement;
		messagePlayer(clientnum, MESSAGE_MISC, "Set restrictPaperDollMovement to %d", restrictPaperDollMovement);
		});

	static ConsoleCommand ccmd_exportstatue("/exportstatue", []CCMD{
		StatueManager.exportActive = true;
		});

	static ConsoleCommand ccmd_importstatue("/importstatue", []CCMD{
	    if (argc < 2)
	    {
	        return;
	    }
		int index = atoi(argv[1]);
		StatueManager.readStatueFromFile(index);
		});

	static ConsoleCommand ccmd_timertests("/timertests", []CCMD{
		TimerExperiments::bUseTimerInterpolation = !TimerExperiments::bUseTimerInterpolation;
		messagePlayer(clientnum, MESSAGE_MISC, "Set bUseTimerInterpolation to %d", TimerExperiments::bUseTimerInterpolation);
		});

	static ConsoleCommand ccmd_timertestsdebug("/timertestsdebug", []CCMD{
		TimerExperiments::bDebug = !TimerExperiments::bDebug;
		messagePlayer(clientnum, MESSAGE_MISC, "Set TimerExperiments::bDebug to %d", TimerExperiments::bDebug);
		});

	static ConsoleCommand ccmd_framesearchdebug("/framesearchdebug", []CCMD{
		Frame::findFrameDefaultSearchType =
			Frame::findFrameDefaultSearchType == Frame::FRAME_SEARCH_DEPTH_FIRST
			? Frame::FRAME_SEARCH_BREADTH_FIRST
			: Frame::FRAME_SEARCH_DEPTH_FIRST;
		if ( Frame::findFrameDefaultSearchType == Frame::FRAME_SEARCH_DEPTH_FIRST )
		{
			messagePlayer(clientnum, MESSAGE_MISC, "Set Frame::findFrameDefaultSearchType to depth first");
		}
		else
		{
			messagePlayer(clientnum, MESSAGE_MISC, "Set Frame::findFrameDefaultSearchType to breadth first");
		}
		});
}
