/*-------------------------------------------------------------------------------

	BARONY
	File: consolecommand.cpp
	Desc: contains consoleCommand()

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "consolecommand.hpp"

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
#include "../ui/LoadingScreen.hpp"
#include "../classdescriptions.hpp"
#include "../ui/MainMenu.hpp"

bool spamming = false;
bool showfirst = false;
bool logCheckObstacle = false;
int logCheckObstacleCount = 0;
bool logCheckMainLoopTimers = false;
bool autoLimbReload = false;

/*******************************************************************************
	std::string cvars
*******************************************************************************/

template ConsoleVariable<std::string>::ConsoleVariable(const char*, std::string const&, const char*);
template<> void ConsoleVariable<std::string>::set(const char* arg)
{
	data = arg;
	messagePlayer(clientnum, MESSAGE_MISC, "\"%s\" is \"%s\"",
		name + 1, data.c_str());
}

/*******************************************************************************
	int cvars
*******************************************************************************/

template ConsoleVariable<int>::ConsoleVariable(const char*, int const&, const char*);
template<> void ConsoleVariable<int>::set(const char* arg)
{
	if (arg && arg[0] != '\0') {
		data = (int)strtol(arg, nullptr, 10);
	}
	messagePlayer(clientnum, MESSAGE_MISC, "\"%s\" is \"%d\"",
		name + 1, data);
}

/*******************************************************************************
	float cvars
*******************************************************************************/

template ConsoleVariable<float>::ConsoleVariable(const char*, float const&, const char*);
template<> void ConsoleVariable<float>::set(const char* arg)
{
	if (arg && arg[0] != '\0') {
		data = strtof(arg, nullptr);
	}
	messagePlayer(clientnum, MESSAGE_MISC, "\"%s\" is \"%f\"",
		name + 1, data);
}

/*******************************************************************************
	bool cvars
*******************************************************************************/

template ConsoleVariable<bool>::ConsoleVariable(const char*, bool const&, const char*);
template<> void ConsoleVariable<bool>::set(const char* arg)
{
	if (arg && arg[0] != '\0') {
		data = !(!strcmp(arg, "false") || !strcmp(arg, "0"));
	}
	messagePlayer(clientnum, MESSAGE_MISC, "\"%s\" is \"%s\"",
		name + 1, data ? "true" : "false");
}

/*******************************************************************************
	Vector4 cvars
*******************************************************************************/

template ConsoleVariable<Vector4>::ConsoleVariable(const char*, Vector4 const&, const char*);
template<> void ConsoleVariable<Vector4>::set(const char* arg)
{
	if (arg && arg[0] != '\0') {
		char* ptr = const_cast<char*>(arg);
		data.x = strtof(ptr, &ptr);
		data.y = strtof(ptr, &ptr);
		data.z = strtof(ptr, &ptr);
		data.w = strtof(ptr, &ptr);
	}
	messagePlayer(clientnum, MESSAGE_MISC, "\"%s\" is \"%f %f %f %f\"",
		name + 1, data.x, data.y, data.z, data.w);
}

/******************************************************************************/

template<typename T>
ConsoleVariable<T>::ConsoleVariable(const char* _name, const T& _default, const char* _desc) :
	ConsoleCommand(_name, _desc, &ConsoleVariable<T>::setter)
{
	add_to_map();
	data = _default;
}

template<typename T>
void ConsoleVariable<T>::setter(int argc, const char** argv)
{
	auto& map = getConsoleVariables();
	auto find = map.find(argv[0]);
	if (find != map.end()) {
		auto& cvar = find->second;
		if (argc >= 2) {
			std::string data;
			data = argv[1];
			for (int c = 2; c < argc; ++c) {
				data.append(" ");
				data.append(argv[c]);
			}
			cvar.set(data.c_str());
		}
		else {
			cvar.set("");
		}
	}
}

template<typename T>
void ConsoleVariable<T>::add_to_map()
{
	auto& map = getConsoleVariables();
	(void)map.emplace(name, *this);
}

template <typename T>
typename ConsoleVariable<T>::cvar_map_t& ConsoleVariable<T>::getConsoleVariables()
{
	static ConsoleVariable<T>::cvar_map_t cvar_map;
	return cvar_map;
}

/******************************************************************************/

ConsoleCommand::ConsoleCommand(const char* _name, const char* _desc, const ccmd_function _func) :
	name(_name),
	desc(_desc),
	func(_func)
{
	add_to_map();
}

typedef std::map<std::string, ConsoleCommand> ccmd_map_t;
static ccmd_map_t& getConsoleCommands()
{
	static ccmd_map_t ccmd_map;
	return ccmd_map;
}

void ConsoleCommand::add_to_map()
{
	auto& map = getConsoleCommands();
	auto result = map.emplace(name, *this);
	if (result.second == false) {
		printlog("A ConsoleCommand by the name \"%s\" already exists! Aborting\n", name);
		assert(0 && "A ConsoleCommand with a duplicate name was found. Aborting");
		exit(1);
	}
}

/*-------------------------------------------------------------------------------

	consoleCommand

	Takes a string and executes it as a game command

-------------------------------------------------------------------------------*/

void consoleCommand(char const* const command_str)
{
	if (!command_str || command_str[0] == '\0')
	{
		return;
	}

	char buf[1024];
	size_t size = strlen(command_str);
	size = std::min(size, sizeof(buf) - 1);
	memcpy(buf, command_str, size);
	buf[size] = '\0';

	std::vector<const char*> tokens;
	auto token = strtok(buf, " ");
	auto command = token;
	while (token)
	{
		tokens.push_back(token);
		token = strtok(nullptr, " ");
	}

	auto& map = getConsoleCommands();
	auto find = map.find(command);
	if (find == map.end())
	{
		// invalid command
		if (intro || !initialized)
		{
			printlog("Unknown command: '%s'\n", command_str);
		}
		else
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(305), command_str);
		}
	}
	else
	{
		auto& ccmd = find->second;
		ccmd(tokens.size(), tokens.data());
	}
}

const char* FindConsoleCommand(const char* str, int index) {
	if (!str || str[0] == '\0') {
		return nullptr;
	}
	size_t len = strlen(str);
	int count = 0;
	auto& map = getConsoleCommands();
	auto lower = map.lower_bound(str);
	auto it = lower;
	for (; count < index; ++it, ++count);
	auto cmd = it == map.end() ? nullptr : it->first.c_str();
	if (cmd && strncmp(str, cmd, len) == 0) {
		return cmd;
	}
	else {
		return nullptr;
	}
}

namespace Test {
	static ConsoleVariable<bool> cvar_bool("/cvar_test_bool", true, "test bools in cvars");
	static ConsoleVariable<float> cvar_float("/cvar_test_float", 1.f, "test floats in cvars");
	static ConsoleVariable<int> cvar_int("/cvar_test_int", 1, "test ints in cvars");
	static ConsoleVariable<std::string> cvar_string("/cvar_test_string", "Hello world", "test strings in cvars");

	static ConsoleCommand print_bool("/test_print_bool", "print contents of cvar_test_bool",
		[](int argc, const char** argv) {
			messagePlayer(clientnum, MESSAGE_MISC, "%s", cvar_bool.data ? "true" : "false");
		});

	static ConsoleCommand print_float("/test_print_float", "print contents of cvar_test_float",
		[](int argc, const char** argv) {
			messagePlayer(clientnum, MESSAGE_MISC, "%f", cvar_float.data);
		});

	static ConsoleCommand print_int("/test_print_int", "print contents of cvar_test_int",
		[](int argc, const char** argv) {
			messagePlayer(clientnum, MESSAGE_MISC, "%d", cvar_int.data);
		});

	static ConsoleCommand print_string("/test_print_string", "print contents of cvar_test_string",
		[](int argc, const char** argv) {
			messagePlayer(clientnum, MESSAGE_MISC, "%s", cvar_string.data.c_str());
		});
}

#define CCMD (int argc, const char **argv)

namespace ConsoleCommands {
	static ConsoleCommand ccmd_help("/help", "get help for a command (eg: /help listcmds)", []CCMD{
		const char* cmd = argc == 1 ? "help" : argv[1];
		auto& map = getConsoleCommands();
		auto find = map.find(std::string("/") + cmd);
		if (find != map.end()) {
			messagePlayer(clientnum, MESSAGE_MISC, "%s", find->second.desc);
		}
 else {
  messagePlayer(clientnum, MESSAGE_MISC, "command '%s' not found", cmd);
}
		});

	static ConsoleCommand ccmd_listcmds("/listcmds", "list all console commands", []CCMD{
		auto & map = getConsoleCommands();
		int pagenum = argc > 1 ? atoi(argv[1]) : 0;
		int index = 0;
		const int num_per_page = 5;
		for (auto& pair : map) {
			auto& cmd = pair.second;
			++index;
			const int cur_page = index / num_per_page;
			if (cur_page == pagenum) {
				messagePlayer(clientnum, MESSAGE_MISC, "%s", cmd.name);
			}
			else if (cur_page > pagenum) {
				break;
			}
		}
		messagePlayer(clientnum, MESSAGE_MISC, "Type \"/listcmds %d\" for more", pagenum + 1);
		});

	static ConsoleCommand ccmd_mousecapture("/mousecapture", "toggle mouse capture enabled", []CCMD{
		if (EnableMouseCapture == SDL_TRUE) {
			EnableMouseCapture = SDL_FALSE;
			messagePlayer(clientnum, MESSAGE_MISC, "Mouse capture is disabled.");
		}
		else if (EnableMouseCapture == SDL_FALSE) {
			EnableMouseCapture = SDL_TRUE;
			messagePlayer(clientnum, MESSAGE_MISC, "Mouse capture is enabled.");
		}
		});

	static ConsoleCommand ccmd_ping("/ping", "ping the remote server", []CCMD{
		if (multiplayer != CLIENT)
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(1117), 0);
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

	static ConsoleCommand ccmd_usemodelcache("/usemodelcache", "use the model cache saved on disk", []CCMD{
		useModelCache = true;
		});

	static ConsoleCommand ccmd_disablemodelcache("/disablemodelcache", "disables use of model cache", []CCMD{
		useModelCache = false;
		});

	static ConsoleCommand ccmd_fov("/fov", "change field-of-view", []CCMD{
		if (argc < 2) {
			return;
		}
		fov = atoi(argv[1]);
		fov = std::min(std::max<Uint32>(40, fov), 100u);
		});

	static ConsoleCommand ccmd_fps("/fps", "set frame rate limit", []CCMD{
		if (argc < 2) {
			return;
		}
		fpsLimit = atoi(argv[1]);
		fpsLimit = std::min(std::max<Uint32>(30, fpsLimit), 1200u);
		});

	static ConsoleCommand ccmd_svflags("/svflags", "set server flags", []CCMD{
		if (multiplayer == CLIENT)
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(275));
		}
		else
		{
			if (argc < 2)
			{
				return;
			}
			svFlags = atoi(argv[1]);
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(276));

			if (multiplayer == SERVER)
			{
				// update client flags
				strcpy((char*)net_packet->data, "SVFL");
				SDLNet_Write32(svFlags, &net_packet->data[4]);
				net_packet->len = 8;

				for (int c = 1; c < MAXPLAYERS; c++)
				{
					if (client_disconnected[c] || players[c]->isLocalPlayer())
					{
						continue;
					}
					net_packet->address.host = net_clients[c - 1].host;
					net_packet->address.port = net_clients[c - 1].port;
					sendPacketSafe(net_sock, -1, net_packet, c - 1);
					messagePlayer(c, MESSAGE_MISC, Language::get(276));
				}
			}
		}
		});

	static ConsoleCommand ccmd_lastname("/lastname", "set cached last name", []CCMD{
		if (argc < 2)
		{
			return;
		}
		std::string name = argv[1];
		for (int arg = 2; arg < argc; ++arg) {
			name.append(" ");
			name.append(argv[arg]);
		}
		lastname = name.c_str();
		});

	static ConsoleCommand ccmd_spawnitem("/spawnitem", "spawn an item (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		if (argc < 2)
		{
			return;
		}
		std::string name = argv[1];
		for (int arg = 2; arg < argc; ++arg) {
			name.append(" ");
			name.append(argv[arg]);
		}

		int c;
		for (c = 0; c < NUMITEMS; c++)
		{
			if (strcmp(items[c].getIdentifiedName(), name.c_str()) == 0)
			{
				dropItem(newItem(static_cast<ItemType>(c), EXCELLENT, 0, 1, local_rng.rand(), true, &stats[clientnum]->inventory), 0);
				break;
			}
		}
		if (c == NUMITEMS)
		{
			for (c = 0; c < NUMITEMS; c++)
			{
				if (strstr(items[c].getIdentifiedName(), name.c_str()))
				{
					dropItem(newItem(static_cast<ItemType>(c), EXCELLENT, 0, 1, local_rng.rand(), true, &stats[clientnum]->inventory), 0);
					break;
				}
			}
		}
		if (c == NUMITEMS)
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(278), name.c_str());
		}
		});

	static ConsoleCommand ccmd_spawncursed("/spawncursed", "spawn a cursed item (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		if (argc < 2)
		{
			return;
		}
		std::string name = argv[1];
		for (int arg = 2; arg < argc; ++arg) {
			name.append(" ");
			name.append(argv[arg]);
		}

		int c;
		for (c = 0; c < NUMITEMS; c++)
		{
			if (strcmp(items[c].getIdentifiedName(), name.c_str()) == 0)
			{
				dropItem(newItem(static_cast<ItemType>(c), WORN, -2, 1, local_rng.rand(), false, &stats[clientnum]->inventory), 0);
				break;
			}
		}
		if (c == NUMITEMS)
		{
			for (c = 0; c < NUMITEMS; c++)
			{
				if (strstr(items[c].getIdentifiedName(), name.c_str()))
				{
					dropItem(newItem(static_cast<ItemType>(c), WORN, -2, 1, local_rng.rand(), false, &stats[clientnum]->inventory), 0);
					break;
				}
			}
		}
		if (c == NUMITEMS)
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(278), name.c_str());
		}
		});

	static ConsoleCommand ccmd_spawnblessed("/spawnblessed", "spawn a blessed item (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		if (argc < 2)
		{
			return;
		}
		std::string name = argv[1];
		for (int arg = 2; arg < argc; ++arg) {
			name.append(" ");
			name.append(argv[arg]);
		}

		int c;
		for (c = 0; c < NUMITEMS; ++c)
		{
			if (strcmp(items[c].getIdentifiedName(), name.c_str()) == 0)
			{
				dropItem(newItem(static_cast<ItemType>(c), WORN, 2, 1, local_rng.rand(), false, &stats[clientnum]->inventory), 0);
				break;
			}
		}
		if (c == NUMITEMS)
		{
			for (c = 0; c < NUMITEMS; ++c)
			{
				if (strstr(items[c].getIdentifiedName(), name.c_str()))
				{
					dropItem(newItem(static_cast<ItemType>(c), WORN, 2, 1, local_rng.rand(), false, &stats[clientnum]->inventory), 0);
					break;
				}
			}
		}
		if (c == NUMITEMS)
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(278), name.c_str());
		}
		});

	static ConsoleCommand ccmd_kick("/kick", "remove a player from the server by name", []CCMD{
		if (argc < 2)
		{
			return;
		}
		std::string name = argv[1];
		for (int arg = 2; arg < argc; ++arg) {
			name.append(" ");
			name.append(argv[arg]);
		}
		if (multiplayer == SERVER)
		{
			int c;
			for (c = 1; c < MAXPLAYERS; c++)
			{
				if (!client_disconnected[c] && !strncmp(name.c_str(), stats[c]->name, 128) && !players[c]->isLocalPlayer())
				{
					client_disconnected[c] = true;
					strcpy((char*)net_packet->data, "KICK");
					net_packet->address.host = net_clients[c - 1].host;
					net_packet->address.port = net_clients[c - 1].port;
					net_packet->len = 4;
					sendPacketSafe(net_sock, -1, net_packet, c - 1);
					int i;
					for (i = 0; i < MAXPLAYERS; i++)
					{
						messagePlayer(i, MESSAGE_MISC, Language::get(279), c, stats[c]->name);
					}
					break;
				}
			}
			if (c == MAXPLAYERS)
			{
				messagePlayer(clientnum, MESSAGE_MISC, Language::get(280));
			}
		}
		else if (multiplayer == CLIENT)
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(281));
		}
		else
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(282));
		}
		});

	static ConsoleCommand ccmd_spawnbook("/spawnbook", "spawn a readable book (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		if (argc < 2)
		{
			return;
		}
		std::string name = argv[1];
		for (int arg = 2; arg < argc; ++arg) {
			name.append(" ");
			name.append(argv[arg]);
		}

		int i = 0;
		for ( i = 0; i < numbooks; ++i )
		{
			if ( strcmp(getBookDefaultNameFromIndex(i).c_str(), name.c_str()) == 0 )
			{
				dropItem(newItem(READABLE_BOOK, EXCELLENT, 0, 1, getBook(getBookDefaultNameFromIndex(i)), true, &stats[clientnum]->inventory), 0);
				break;
			}
		}

		if ( i == numbooks )
		{
			for ( i = 0; i < numbooks; ++i )
			{
				if ( strstr(getBookDefaultNameFromIndex(i).c_str(), name.c_str()) )
				{
					dropItem(newItem(READABLE_BOOK, EXCELLENT, 0, 1, getBook(getBookDefaultNameFromIndex(i)), true, &stats[clientnum]->inventory), 0);
					break;
				}
			}
		}
	});

	static ConsoleCommand ccmd_spawnallbooks("/spawnallbooks", "spawn all readable books (cheat)", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}

		for ( int i = 0; i < numbooks; ++i )
		{
			dropItem(newItem(READABLE_BOOK, EXCELLENT, 0, 1, getBook(getBookDefaultNameFromIndex(i)), true, &stats[clientnum]->inventory), 0);
		}
	});

	static ConsoleCommand ccmd_savemap("/savemap", "save the current level to disk", []CCMD{
		if (argc > 1)
		{
			saveMap(argv[1]);
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(283), argv[1]);
		}
		});

	static ConsoleCommand ccmd_nextlevel("/nextlevel", "advance to the next dungeon level (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		if (multiplayer == CLIENT)
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(284));
		}
		else
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(285));
			loadnextlevel = true;
			Compendium_t::Events_t::previousCurrentLevel = currentlevel;
			Compendium_t::Events_t::previousSecretlevel = secretlevel;
		}
		});

	static ConsoleCommand ccmd_pos("/pos", "show the camera coordinates", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		messagePlayer(clientnum, MESSAGE_MISC, Language::get(286),
			(int)cameras[0].x,
			(int)cameras[0].y,
			(int)cameras[0].z,
			cameras[0].ang,
			cameras[0].vang);
		});

	static ConsoleCommand ccmd_pathmap("/pathmap", "display pathmap values at player coords", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
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

	static ConsoleCommand ccmd_exit("/exit", "exit the game", []CCMD{
		mainloop = 0;
		});

	static ConsoleCommand ccmd_showfps("/showfps", "display FPS counter", []CCMD{
		showfps = (showfps == false);
		});

	static ConsoleCommand ccmd_noclip("/noclip", "toggle noclip mode (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		if (multiplayer != SINGLE)
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(287));
		}
		else
		{
			noclip = (noclip == false);
			if (noclip)
			{
				messagePlayer(clientnum, MESSAGE_MISC, Language::get(288));
			}
			else
			{
				messagePlayer(clientnum, MESSAGE_MISC, Language::get(289));
			}
		}
		});

	static ConsoleCommand ccmd_god("/god", "toggle god mode (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		if (multiplayer != SINGLE)
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(290));
		}
		else
		{
			godmode = (godmode == false);
			if (godmode)
			{
				messagePlayer(clientnum, MESSAGE_MISC, Language::get(291));
			}
			else
			{
				messagePlayer(clientnum, MESSAGE_MISC, Language::get(292));
			}
		}
		});

	static ConsoleCommand ccmd_spam("/spam", "", []CCMD{
		spamming = !(spamming);
		});

	static ConsoleCommand ccmd_logobstacle("/logobstacle", "", []CCMD{
		logCheckObstacle = !(logCheckObstacle);
		});

	static ConsoleCommand ccmd_showfirst("/showfirst", "", []CCMD{
		showfirst = !(showfirst);
		});

	static ConsoleCommand ccmd_buddha("/buddha", "toggle buddha mode (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		if (multiplayer != SINGLE)
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(293));
		}
		else
		{
			buddhamode = (buddhamode == false);
			if (buddhamode)
			{
				messagePlayer(clientnum, MESSAGE_MISC, Language::get(294));
			}
			else
			{
				messagePlayer(clientnum, MESSAGE_MISC, Language::get(295));
			}
		}
		});

	static ConsoleCommand ccmd_friendly("/friendly", "make all NPCs friendly (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		if (multiplayer == CLIENT)
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(284));
			return;
		}
		everybodyfriendly = (everybodyfriendly == false);
		if (everybodyfriendly)
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(296));
		}
		else
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(297));
		}
		});

	static ConsoleCommand ccmd_dowse("/dowse", "print the down stairs coords (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		for (auto node = map.entities->first; node != NULL; node = node->next)
		{
			auto entity = (Entity*)node->element;
			if (entity->behavior == &actLadder)
			{
				messagePlayer(clientnum, MESSAGE_MISC, Language::get(298), (int)(entity->x / 16), (int)(entity->y / 16));
			}
		}
		});

	static ConsoleCommand ccmd_thirdperson("/thirdperson", "toggle thirdperson mode (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			// this is definitely considered a cheat.
			// otherwise it's a major gameplay exploit.
			// do not disable this code block.
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
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

	static ConsoleCommand ccmd_res("/res", "change the window resolution", []CCMD{
		if (argc < 2)
		{
			return;
		}
		xres = atoi(argv[1]);
		for (int c = 0; c < strlen(argv[1]); c++)
		{
			if (argv[1][c] == 'x')
			{
				yres = atoi(&argv[1][c + 1]);
				break;
			}
		}
		if (initialized)
		{
			if (!changeVideoMode())
			{
				printlog("critical error! Attempting to abort safely...\n");
				mainloop = 0;
			}
		}
		});

	static ConsoleCommand ccmd_rscale("/rscale", "", []CCMD{
		if (argc < 2)
		{
			return;
		}
		rscale = atoi(argv[1]);
		});

	static ConsoleCommand ccmd_fullscreen("/fullscreen", "toggle fullscreen mode", []CCMD{
		fullscreen = (fullscreen == 0);
		});

	static ConsoleCommand ccmd_shaking("/shaking", "toggle camera shaking", []CCMD{
		shaking = (shaking == 0);
		});

	static ConsoleCommand ccmd_bobbing("/bobbing", "toggle camera bobbing", []CCMD{
		bobbing = (bobbing == 0);
		});

	static ConsoleCommand ccmd_sfxvolume("/sfxvolume", "set sfx volume", []CCMD{
		if (argc > 1)
			sfxvolume = strtof(argv[1], nullptr);
		});

	static ConsoleCommand ccmd_musvolume("/musvolume", "set music volume", []CCMD{
		if (argc > 1)
			musvolume = strtof(argv[1], nullptr);
		});

	static ConsoleCommand ccmd_bind("/bind", "bind input to game action", []CCMD{
		printlog("Note: /bind is now deprecated.\n");
		});

	static ConsoleCommand ccmd_mana("/mana", "give player mana (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}

		if (multiplayer == SINGLE)
		{
			stats[clientnum]->MP = stats[clientnum]->MAXMP;
		}
		else
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(299));
		}
		});

	static ConsoleCommand ccmd_heal("/heal", "heal the player (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}

		if (multiplayer == SINGLE)
		{
			stats[clientnum]->HP = stats[clientnum]->MAXHP;
		}
		else
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(299));
		}
		});

	static ConsoleCommand ccmd_damage("/damage", "damage the player (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		if (multiplayer != SINGLE)
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(299));
			return;
		}
		if (argc < 2) {
			return;
		}

		int amount = atoi(argv[1]);

		players[clientnum]->entity->modHP(-amount);

		messagePlayer(clientnum, MESSAGE_MISC, "Damaging you by %d. New health: %d", amount, stats[clientnum]->HP);
		});

	static ConsoleCommand ccmd_ip("/ip", "set the saved ip address", []CCMD{
		if (argc > 1)
		{
			size_t size = strlen(argv[1]);
			size = std::min(size, sizeof(last_ip) - 1);
			memcpy(last_ip, argv[1], size);
			last_ip[size] = '\0';
		}
		});

	static ConsoleCommand ccmd_port("/port", "set the saved port number", []CCMD{
		if (argc > 1)
		{
			size_t size = strlen(argv[1]);
			size = std::min(size, sizeof(last_port) - 1);
			memcpy(last_port, argv[1], size);
			last_port[size] = '\0';
		}
		});

	static ConsoleCommand ccmd_noblood("/noblood", "toggle content control", []CCMD{
		spawn_blood = (spawn_blood == false);
		});

	static ConsoleCommand ccmd_nolightflicker("/nolightflicker", "toggle flickering lights", []CCMD{
		flickerLights = (flickerLights == false);
		});

	static ConsoleCommand ccmd_vsync("/vsync", "toggle vertical sync", []CCMD{
		verticalSync = (verticalSync == false);
		});

	static ConsoleCommand ccmd_hidestatusicons("/hidestatusicons", "toggle status icons", []CCMD{
		showStatusEffectIcons = (showStatusEffectIcons == false);
		});

	static ConsoleCommand ccmd_muteping("/muteping", "toggle minimap ping audio", []CCMD{
		minimapPingMute = (minimapPingMute == false);
		});

	static ConsoleCommand ccmd_colorblind("/colorblind", "toggle colorblind mode", []CCMD{
		colorblind = (colorblind == false);
		});

	static ConsoleCommand ccmd_gamma("/gamma", "set gamma value", []CCMD{
		if (argc < 2) {
			return;
		}
		vidgamma = strtof(argv[1], nullptr);
		});

	static ConsoleCommand ccmd_capturemouse("/capturemouse", "toggle mouse capture", []CCMD{
		capture_mouse = (capture_mouse == false);
		});

	static ConsoleCommand ccmd_levelup("/levelup", "level up the player character (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}

		if (multiplayer != CLIENT)
		{
			if (players[clientnum] && players[clientnum]->entity)
			{
				players[clientnum]->entity->getStats()->EXP += 100;
			}
		}
		else if (multiplayer == CLIENT)
		{
			// request level up
			strcpy((char*)net_packet->data, "CLVL");
			net_packet->data[4] = clientnum;
			net_packet->address.host = net_server.host;
			net_packet->address.port = net_server.port;
			net_packet->len = 5;
			sendPacketSafe(net_sock, -1, net_packet, 0);
			//messagePlayer(clientnum, MESSAGE_MISC, Language::get(299));
		}
		});

	static ConsoleCommand ccmd_maxout2("/maxout2", "give player lots of stuff (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}

		if (multiplayer != CLIENT)
		{
			int c;
			Stat* myStats = stats[0];
			for (c = 0; c < 24; c++)
			{
				consoleCommand("/levelup");
			}
			for (c = 0; c < NUM_HOTBAR_SLOTS; c++)
			{
				auto& hotbar = players[clientnum]->hotbar.slots();
				hotbar[c].item = 0;
			}
			myStats->weapon = newItem(STEEL_SWORD, SERVICABLE, 0, 1, local_rng.rand(), true, &myStats->inventory);
			newItem(CROSSBOW, SERVICABLE, 0, 1, local_rng.rand(), true, &myStats->inventory);
			newItem(MAGICSTAFF_LIGHT, SERVICABLE, 0, 1, local_rng.rand(), true, &myStats->inventory);
			myStats->shield = newItem(STEEL_SHIELD, SERVICABLE, 0, 1, local_rng.rand(), true, &myStats->inventory);
			myStats->helmet = newItem(HAT_HOOD, SERVICABLE, 0, 1, 2, true, &myStats->inventory);
			myStats->shoes = newItem(STEEL_BOOTS, SERVICABLE, 0, 1, local_rng.rand(), true, &myStats->inventory);
			myStats->breastplate = newItem(STEEL_BREASTPIECE, SERVICABLE, 0, 1, local_rng.rand(), true, &myStats->inventory);
			myStats->gloves = newItem(GAUNTLETS, SERVICABLE, 0, 1, local_rng.rand(), true, &myStats->inventory);
			myStats->cloak = newItem(CLOAK_BLACK, SERVICABLE, 0, 1, local_rng.rand(), true, &myStats->inventory);
		}
		else
		{
			for (int c = 0; c < 24; c++)
			{
				consoleCommand("/levelup");
			}
			//messagePlayer(clientnum, MESSAGE_MISC, Language::get(299));
		}
		});

	static ConsoleCommand ccmd_jumplevel("/jumplevel", "advance several levels", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		if (argc < 2) {
			return;
		}
		skipLevelsOnLoad = atoi(argv[1]);
		if (skipLevelsOnLoad == -1)
		{
			loadingSameLevelAsCurrent = true;
		}
		consoleCommand("/nextlevel");
		});

	static ConsoleCommand ccmd_maxout3("/maxout3", "give player lots of stuff (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}

		if (multiplayer == SINGLE)
		{
			int c;
			Stat* myStats = stats[0];
			//skipLevelsOnLoad = 31;
			for (c = 0; c < 24; c++)
			{
				consoleCommand("/levelup");
			}
			for (c = 0; c < NUM_HOTBAR_SLOTS; c++)
			{
				auto& hotbar = players[clientnum]->hotbar.slots();
				hotbar[c].item = 0;
			}
			myStats->weapon = newItem(STEEL_SWORD, SERVICABLE, 0, 1, local_rng.rand(), true, &myStats->inventory);
			newItem(CROSSBOW, SERVICABLE, 0, 1, local_rng.rand(), true, &myStats->inventory);
			newItem(MAGICSTAFF_LIGHT, SERVICABLE, 0, 1, local_rng.rand(), true, &myStats->inventory);
			myStats->shield = newItem(STEEL_SHIELD, SERVICABLE, 0, 1, local_rng.rand(), true, &myStats->inventory);
			myStats->helmet = newItem(HAT_HOOD, SERVICABLE, 0, 1, 2, true, &myStats->inventory);
			myStats->shoes = newItem(STEEL_BOOTS, SERVICABLE, 0, 1, local_rng.rand(), true, &myStats->inventory);
			myStats->breastplate = newItem(STEEL_BREASTPIECE, SERVICABLE, 0, 1, local_rng.rand(), true, &myStats->inventory);
			myStats->gloves = newItem(GAUNTLETS, SERVICABLE, 0, 1, local_rng.rand(), true, &myStats->inventory);
			myStats->cloak = newItem(CLOAK_BLACK, SERVICABLE, 0, 1, local_rng.rand(), true, &myStats->inventory);
			consoleCommand("/levelskill 9");
			//consoleCommand("/nextlevel");
			while (myStats->getProficiency(PRO_APPRAISAL) < 50)
			{
				consoleCommand("/levelskill 3");
			}
		}
		else
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(299));
		}
		});

	static ConsoleCommand ccmd_maxout4("/maxout4", "give player lots of stuff (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}

		if (multiplayer == SINGLE)
		{
			int c;
			Stat* myStats = stats[0];
			for (c = 0; c < 35; c++)
			{
				consoleCommand("/levelup");
			}
			for (c = 0; c < NUM_HOTBAR_SLOTS; c++)
			{
				auto& hotbar = players[clientnum]->hotbar.slots();
				hotbar[c].item = 0;
			}
			myStats->weapon = newItem(STEEL_SWORD, SERVICABLE, 0, 1, local_rng.rand(), true, &myStats->inventory);
			newItem(CROSSBOW, SERVICABLE, 0, 1, local_rng.rand(), true, &myStats->inventory);
			newItem(MAGICSTAFF_LIGHT, SERVICABLE, 0, 1, local_rng.rand(), true, &myStats->inventory);
			myStats->shield = newItem(STEEL_SHIELD, SERVICABLE, 0, 1, local_rng.rand(), true, &myStats->inventory);
			myStats->helmet = newItem(HAT_HOOD, SERVICABLE, 0, 1, 2, true, &myStats->inventory);
			myStats->shoes = newItem(STEEL_BOOTS, SERVICABLE, 0, 1, local_rng.rand(), true, &myStats->inventory);
			myStats->breastplate = newItem(STEEL_BREASTPIECE, SERVICABLE, 0, 1, local_rng.rand(), true, &myStats->inventory);
			myStats->gloves = newItem(GAUNTLETS, SERVICABLE, 0, 1, local_rng.rand(), true, &myStats->inventory);
			myStats->cloak = newItem(CLOAK_BLACK, SERVICABLE, 0, 1, local_rng.rand(), true, &myStats->inventory);
			//consoleCommand("/nextlevel");
			for (c = 0; c < NUMPROFICIENCIES; c++)
			{
				if (c != PRO_STEALTH)
				{
					while (stats[clientnum]->getProficiency(c) < 100 )
					{
						players[clientnum]->entity->increaseSkill(c, false);
					}
				}
			}
		}
		else
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(299));
		}
		});

	static ConsoleCommand ccmd_maxout("/maxout", "give player lots of stuff (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}

		if (multiplayer == SINGLE)
		{
			int c;
			for (c = 0; c < 14; c++)
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
			for (c = 0; c < NUMPROFICIENCIES; c++)
			{
				while (stats[clientnum]->getProficiency(c) < 100)
				{
					players[clientnum]->entity->increaseSkill(c, false);
				}
			}
		}
		else
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(299));
		}
		});

	static ConsoleCommand ccmd_hunger("/hunger", "set player hunger (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}

		int player = clientnum;
		if ( argc == 2 ) {
			player = atoi(argv[1]);
		}

		if (multiplayer == SINGLE)
		{
			Stat* tempStats = players[clientnum]->entity->getStats();
			if (tempStats)
			{
				tempStats->HUNGER = std::max(0, tempStats->HUNGER - 100);
			}
		}
		else if ( multiplayer == SERVER )
		{
			if ( player >= 0 && player < MAXPLAYERS )
			{
				Stat* tempStats = players[player]->entity->getStats();
				if ( tempStats )
				{
					tempStats->HUNGER = std::max(0, tempStats->HUNGER - 100);
				}
				serverUpdateHunger(player);
			}
		}
		else
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(284));
		}
		});

	static ConsoleCommand ccmd_poison("/poison", "poison the player (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}

		if (multiplayer == SINGLE)
		{
			Stat* tempStats = players[clientnum]->entity->getStats();
			if (tempStats)
			{
				tempStats->EFFECTS[EFF_POISONED] = true;
				tempStats->EFFECTS_TIMERS[EFF_POISONED] = 600;
			}
		}
		else
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(299));
		}
		});

	static ConsoleCommand ccmd_testsound("/testsound", "test a sound effect", []CCMD{
		if (argc < 2) {
			return;
		}
		int num = atoi(argv[1]);
		playSound(num, 255);
		});

	static ConsoleCommand ccmd_skipintro("/skipintro", "toggle skipping the opening cutscene", []CCMD{
		skipintro = (skipintro == false);
		});

	static ConsoleCommand ccmd_levelmagic("/levelmagic", "level up magic skills (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
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
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(299));
		}
		});

	static ConsoleCommand ccmd_numentities("/numentities", "display number of entities in the level", []CCMD{
		messagePlayer(clientnum, MESSAGE_MISC, Language::get(300), list_Size(map.entities));
		});

	static ConsoleCommand ccmd_nummonsters2("/nummonsters2", "display number of NPCs in the level", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		messagePlayer(clientnum, MESSAGE_MISC, Language::get(2353), list_Size(map.creatures));
		});

	static ConsoleCommand ccmd_nummonsters("/nummonsters", "display number of monsters in the level", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		messagePlayer(clientnum, MESSAGE_MISC, Language::get(2353), nummonsters);
		});

	static ConsoleCommand ccmd_verifycreaturelist("/verifycreaturelist", "", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		//Make sure that the number of creatures in the creature list are the real count in the game world.
		unsigned entcount = 0;

		for (node_t* node = map.entities->first; node; node = node->next)
		{
			if (node->element)
			{
				Entity* ent = static_cast<Entity*>(node->element);
				if (ent->behavior == actMonster || ent->behavior == actPlayer)
				{
					++entcount;
				}
			}
		}

		messagePlayer(clientnum, MESSAGE_MISC, "ent count = %d, creatures list size = %d", entcount, list_Size(map.creatures));

		if (entcount == list_Size(map.creatures))
		{
			messagePlayer(clientnum, MESSAGE_MISC, "Yes, list is verified correct.");
		}
		else
		{
			messagePlayer(clientnum, MESSAGE_MISC, "Nope, much problemo!");
		}
		});

	static ConsoleCommand ccmd_loadmodels("/loadmodels", "", []CCMD{
		const int startIndex = argc >= 2 ? atoi(argv[1]) : 0;
		const int endIndex = argc >= 3 ? atoi(argv[2]) : nummodels;
		reloadModels(startIndex, endIndex);
		});

	static ConsoleCommand ccmd_killmonsters("/killmonsters", "kill all monsters (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		if (multiplayer == CLIENT)
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(284));
		}
		else
		{
			int c = 0;
			node_t* node,* nextnode;
			for (node = map.entities->first; node != NULL; node = nextnode)
			{
				nextnode = node->next;
				Entity* entity = (Entity*)node->element;
				if (entity->behavior == &actMonster)
				{
					entity->setHP(0);
					c++;
				}
			}
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(301), c);
		}
		});

	static ConsoleCommand ccmd_cleanfloor("/cleanfloor", "remove floor items (cheat)", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		if ( multiplayer == CLIENT )
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(284));
		}
		else
		{
			int c = 0;
			node_t* node,* nextnode;
			for ( node = map.entities->first; node != NULL; node = nextnode )
			{
				nextnode = node->next;
				Entity* entity = (Entity*)node->element;
				if ( entity->behavior == &actItem )
				{
					list_RemoveNode(entity->mynode);
					c++;
				}
			}
			messagePlayer(clientnum, MESSAGE_MISC, "Cleared %d items", c);
		}
	});

	static void suicide(int player) {
		if (player < 0 || player >= MAXPLAYERS) {
			return;
		}
		if (multiplayer != SINGLE && player != clientnum) {
			// don't let internet jokers kill other players with 1 command
			return;
		}
		if (multiplayer == CLIENT) {
			// request sweet release.
			strcpy((char*)net_packet->data, "IDIE");
			net_packet->data[4] = player;
			net_packet->address.host = net_server.host;
			net_packet->address.port = net_server.port;
			net_packet->len = 5;
			sendPacketSafe(net_sock, -1, net_packet, 0);
		}
		else {
			if (players[player] && players[player]->entity) {
				players[player]->entity->setHP(0);
			}
		}
	}

	static ConsoleCommand ccmd_die("/die", "suicide the player", []CCMD{
		suicide(clientnum);
		});

	static ConsoleCommand ccmd_killme("/killme", "suicide the player", []CCMD{
		suicide(clientnum);
		});

	static ConsoleCommand ccmd_suicide("/suicide", "kill the given player", []CCMD{
		if (argc < 2) {
			suicide(clientnum);
		}
 else {
  int player = (int)strtol(argv[1], nullptr, 10);
  suicide(player);
}
		});

	static ConsoleCommand ccmd_segfault("/segfault", "don't try this at home", []CCMD{
		int* potato = NULL;
		(*potato) = 322; //Crash the game!
		});

	static ConsoleCommand ccmd_flames("/flames", "ignite the player (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}

		if (multiplayer != SINGLE)
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(299));
			return;
		}

		// Attempt to set the Player on fire
		players[clientnum]->entity->SetEntityOnFire();

		for (int c = 0; c < 100; c++)
		{
			if ( auto entity = spawnFlame(players[clientnum]->entity, SPRITE_FLAME) )
			{
				entity->sprite = 16;
				double vel = local_rng.rand() % 10;
				entity->vel_x = vel * cos(entity->yaw) * cos(entity->pitch) * .1;
				entity->vel_y = vel * sin(entity->yaw) * cos(entity->pitch) * .1;
				entity->vel_z = vel * sin(entity->pitch) * .2;
				entity->skill[0] = 5 + local_rng.rand() % 10;
			}
		}
		});

	static ConsoleCommand ccmd_cure("/cure", "cure the player of ailments (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}

		if (multiplayer != SINGLE)
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(299));
			return;
		}

		if (!players[clientnum]->entity)
		{
			return;
		}

		for (int c = 0; c < NUMEFFECTS; c++)   //This does a whole lot more than just cure ailments.
		{
			if (!(c == EFF_VAMPIRICAURA && players[clientnum]->entity->getStats()->EFFECTS_TIMERS[c] == -2)
				&& c != EFF_WITHDRAWAL && c != EFF_SHAPESHIFT)
			{
				players[clientnum]->entity->getStats()->EFFECTS[c] = false;
				players[clientnum]->entity->getStats()->EFFECTS_TIMERS[c] = 0;
			}
		}
		if (players[clientnum]->entity->getStats()->EFFECTS[EFF_WITHDRAWAL])
		{
			players[clientnum]->entity->setEffect(EFF_WITHDRAWAL, false, EFFECT_WITHDRAWAL_BASE_TIME, true);
		}
		});

	static ConsoleCommand ccmd_summonall("/summonall", "summon a bunch of monsters (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		if (multiplayer == CLIENT)
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(284));
		}
		else if (players[clientnum] && players[clientnum]->entity)
		{
			if (argc < 2) {
				return;
			}
			std::string name = argv[1];
			for (int arg = 2; arg < argc; ++arg) {
				name.append(" ");
				name.append(argv[arg]);
			}
			int i, creature;
			bool found = false;

			for (i = 1; i < NUMMONSTERS; ++i)   //Start at 1 because 0 is a nothing.
			{
				if (strstr(monstertypename[i], name.c_str()))
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
				messagePlayer(clientnum, MESSAGE_MISC, Language::get(304), name.c_str());
			}
		}
		});

	static ConsoleCommand ccmd_summonshop("/summonshop", "summon a shop (cheat)", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		if ( multiplayer == CLIENT )
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(284));
		}
		else if ( players[clientnum] && players[clientnum]->entity )
		{
			if ( argc < 2 ) {
				return;
			}
			int type = atoi(argv[1]);

			playSoundEntity(players[clientnum]->entity, 153, 64);

			//Spawn monster
			Entity* monster = summonMonster(SHOPKEEPER, players[clientnum]->entity->x + 32 * cos(players[clientnum]->entity->yaw), players[clientnum]->entity->y + 32 * sin(players[clientnum]->entity->yaw));
			if ( monster )
			{
				messagePlayer(clientnum, MESSAGE_MISC, Language::get(302), getMonsterLocalizedName(SHOPKEEPER).c_str());
				if ( auto stat = monster->getStats() )
				{
					stat->MISC_FLAGS[STAT_FLAG_NPC] = 1 + std::max(0, std::min(9, type));
				}
			}
			else
			{
				messagePlayer(clientnum, MESSAGE_MISC, Language::get(303), getMonsterLocalizedName(SHOPKEEPER).c_str());
			}
		}
	});
	static ConsoleCommand ccmd_summon("/summon", "summon a monster (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		if (multiplayer == CLIENT)
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(284));
		}
		else if (players[clientnum] && players[clientnum]->entity)
		{
			if (argc < 2) {
				return;
			}
			std::string name = argv[1];
			for (int arg = 2; arg < argc; ++arg) {
				name.append(" ");
				name.append(argv[arg]);
			}
			int i, creature;
			bool found = false;

			for (i = 1; i < NUMMONSTERS; ++i)   //Start at 1 because 0 is a nothing.
			{
				if (strstr(monstertypename[i], name.c_str()))
				{
					creature = i;
					found = true;
					break;
				}
			}
			if (!found)
			{
				MonsterStatCustomManager::StatEntry* statEntry = monsterStatCustomManager.readFromFile(name.c_str());
				if (statEntry)
				{
					Entity* monster = summonMonster(static_cast<Monster>(statEntry->type), players[clientnum]->entity->x + 32 * cos(players[clientnum]->entity->yaw), players[clientnum]->entity->y + 32 * sin(players[clientnum]->entity->yaw));
					if (monster)
					{
						messagePlayer(clientnum, MESSAGE_MISC, Language::get(302), monstertypename[static_cast<Monster>(statEntry->type)]);
						if (monster->getStats())
						{
							statEntry->setStatsAndEquipmentToMonster(monster->getStats());
							while (statEntry->numFollowers > 0)
							{
								std::string followerName = statEntry->getFollowerVariant();
								if (followerName.compare("") && followerName.compare("none"))
								{
									MonsterStatCustomManager::StatEntry* followerEntry = monsterStatCustomManager.readFromFile(followerName.c_str());
									if (followerEntry)
									{
										Entity* summonedFollower = summonMonster(static_cast<Monster>(followerEntry->type), monster->x, monster->y);
										if (summonedFollower)
										{
											if (summonedFollower->getStats())
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
										if (summonedFollower)
										{
											if (summonedFollower->getStats())
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
					messagePlayer(clientnum, MESSAGE_MISC, Language::get(302), getMonsterLocalizedName((Monster)creature).c_str());
				}
				else
				{
					messagePlayer(clientnum, MESSAGE_MISC, Language::get(303), getMonsterLocalizedName((Monster)creature).c_str());
				}
			}
			else
			{
				messagePlayer(clientnum, MESSAGE_MISC, Language::get(304), name.c_str());
			}
		}
		});

	static ConsoleCommand ccmd_summonchest("/summonchest", "spawn a chest (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		if (multiplayer == CLIENT)
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(284));
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

	static ConsoleCommand ccmd_broadcast("/broadcast", "", []CCMD{
		broadcast = (broadcast == false);
		});

	static ConsoleCommand ccmd_nohud("/nohud", "disable in-game hud", []CCMD{
		nohud = (nohud == false);
		});

	static ConsoleCommand ccmd_disablehotbarnewitems("/disablehotbarnewitems", "", []CCMD{
		auto_hotbar_new_items = (auto_hotbar_new_items == false);
		});

	static ConsoleCommand ccmd_hotbarenablecategory("/hotbarenablecategory", "", []CCMD{
		if (argc < 3) {
			return;
		}
		int catIndex = atoi(argv[1]);
		int value = atoi(argv[2]);
		auto_hotbar_categories[catIndex] = value;
		printlog("Hotbar auto add category %d, value %d.", catIndex, value);
		});

	static ConsoleCommand ccmd_autosortcategory("/autosortcategory", "", []CCMD{
		if (argc < 3) {
			return;
		}
		int catIndex = atoi(argv[1]);
		int value = atoi(argv[2]);
		autosort_inventory_categories[catIndex] = value;
		printlog("Autosort inventory category %d, priority %d.", catIndex, value);
		});

	static ConsoleCommand ccmd_locksidebar("/locksidebar", "", []CCMD{
		if (players[clientnum]) // warning - this doesn't exist when loadConfig() is called on init.
		{
			players[clientnum]->characterSheet.lock_right_sidebar = (players[clientnum]->characterSheet.lock_right_sidebar == false);
			if (players[clientnum]->characterSheet.lock_right_sidebar)
			{
				players[clientnum]->characterSheet.proficienciesPage = 1;
			}
		}
		});

	static ConsoleCommand ccmd_showgametimer("/showgametimer", "display in-game timer", []CCMD{
		show_game_timer_always = (show_game_timer_always == false);
		});

	static ConsoleCommand ccmd_lang("/lang", "load specified language file (eg: /lang en)", []CCMD{
		if (argc > 1) {
			Language::loadLanguage(argv[1], false);
		}
		});

	static ConsoleCommand ccmd_mapseed("/mapseed", "display map seed", []CCMD{
		messagePlayer(clientnum, MESSAGE_MISC, "Mapseed: %d | Gamekey: %lu | Lobby: %lu", mapseed, uniqueGameKey, uniqueLobbyKey);
		});

	static ConsoleCommand ccmd_seedgame("/seedgame", "set custom seed", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		if ( multiplayer == CLIENT )
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(284));
		}

		if ( argc > 1 ) {
			gameModeManager.currentSession.seededRun.setup(argv[1]);
		}
		messagePlayer(clientnum, MESSAGE_DEBUG, "Seed is %lu | name: %s | key: %lu", 
			gameModeManager.currentSession.seededRun.seed,
			gameModeManager.currentSession.seededRun.seedString.c_str(),
			uniqueGameKey);
		});

	static ConsoleCommand ccmd_reloadlang("/reloadlang", "reload language file", []CCMD{
		Language::reloadLanguage();
		});

	static ConsoleCommand ccmd_disablemessages("/disablemessages", "disable all messages", []CCMD{
		disable_messages = true;
		});

	static ConsoleCommand ccmd_right_click_protect("/right_click_protect", "toggle right-click protection", []CCMD{
		right_click_protect = (right_click_protect == false);
		});

	static ConsoleCommand ccmd_autoappraisenewitems("/autoappraisenewitems", "auto appraise new items", []CCMD{
		auto_appraise_new_items = true;
		});

	static ConsoleCommand ccmd_startfloor("/startfloor", "set the start floor", []CCMD{
		if (argc > 1)
		{
			startfloor = atoi(argv[1]);
			//Ensure its value is in range.
			startfloor = std::max(startfloor, 0);
			//startfloor = std::min(startfloor, numlevels);
			printlog("Start floor is %d.", startfloor);
		}
		});

	static ConsoleCommand ccmd_splitscreen("/splitscreen", "enable splitscreen mode (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		int numPlayers = 4;
		if ( argc > 1 )
		{
			numPlayers = std::max(std::min(atoi(argv[1]), MAXPLAYERS), 2);
		}
		splitscreen = !splitscreen;

		if (splitscreen)
		{
			for (int i = 1; i < MAXPLAYERS; ++i)
			{
				if (i < numPlayers)
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
		for (int i = 1; i < MAXPLAYERS; ++i)
		{
			if (client_disconnected[i])
			{
				players[i]->bSplitscreen = false;
				players[i]->splitScreenType = Player::SPLITSCREEN_DEFAULT;
			}
			else
			{
				players[i]->bSplitscreen = true;
			}

			if (players[i]->isLocalPlayer())
			{
				++playercount;
			}
		}


		for (int i = 0; i < MAXPLAYERS; ++i)
		{
			players[i]->splitScreenType = Player::SPLITSCREEN_DEFAULT;
			players[i]->messageZone.deleteAllNotificationMessages();
			if (!splitscreen)
			{
				players[i]->camera().winx = 0;
				players[i]->camera().winy = 0;
				players[i]->camera().winw = xres;
				players[i]->camera().winh = yres;
			}
			else
			{
				if (playercount == 1)
				{
					players[i]->camera().winx = 0;
					players[i]->camera().winy = 0;
					players[i]->camera().winw = xres;
					players[i]->camera().winh = yres;
				}
				else if (playercount == 2)
				{
					if (players[i]->splitScreenType == Player::SPLITSCREEN_VERTICAL)
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
				else if (playercount >= 3)
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

			if (i > 0)
			{
				stats[i]->sex = static_cast<sex_t>(local_rng.rand() % 2);
				stats[i]->stat_appearance = local_rng.rand() % 18;
				stats[i]->clearStats();
				client_classes[i] = local_rng.rand() % (CLASS_MONK + 1);//NUMCLASSES;
				stats[i]->playerRace = RACE_HUMAN;
				if (enabledDLCPack1 || enabledDLCPack2)
				{
					stats[i]->playerRace = local_rng.rand() % NUMPLAYABLERACES;
					if (!enabledDLCPack1)
					{
						while (stats[i]->playerRace == RACE_SKELETON || stats[i]->playerRace == RACE_VAMPIRE
							|| stats[i]->playerRace == RACE_SUCCUBUS || stats[i]->playerRace == RACE_GOATMAN)
						{
							stats[i]->playerRace = local_rng.rand() % NUMPLAYABLERACES;
						}
					}
					else if (!enabledDLCPack2)
					{
						while (stats[i]->playerRace == RACE_AUTOMATON || stats[i]->playerRace == RACE_GOBLIN
							|| stats[i]->playerRace == RACE_INCUBUS || stats[i]->playerRace == RACE_INSECTOID)
						{
							stats[i]->playerRace = local_rng.rand() % NUMPLAYABLERACES;
						}
					}
					if (stats[i]->playerRace == RACE_INCUBUS)
					{
						stats[i]->sex = MALE;
					}
					else if (stats[i]->playerRace == RACE_SUCCUBUS)
					{
						stats[i]->sex = FEMALE;
					}

					if (stats[i]->playerRace == RACE_HUMAN)
					{
						client_classes[i] = local_rng.rand() % (NUMCLASSES);
						if (!enabledDLCPack1)
						{
							while (client_classes[i] == CLASS_CONJURER || client_classes[i] == CLASS_ACCURSED
								|| client_classes[i] == CLASS_MESMER || client_classes[i] == CLASS_BREWER)
							{
								client_classes[i] = local_rng.rand() % (NUMCLASSES);
							}
						}
						else if (!enabledDLCPack2)
						{
							while (client_classes[i] == CLASS_HUNTER || client_classes[i] == CLASS_SHAMAN
								|| client_classes[i] == CLASS_PUNISHER || client_classes[i] == CLASS_MACHINIST)
							{
								client_classes[i] = local_rng.rand() % (NUMCLASSES);
							}
						}
						stats[i]->stat_appearance = local_rng.rand() % 18;
					}
					else
					{
						client_classes[i] = local_rng.rand() % (CLASS_MONK + 2);
						if (client_classes[i] > CLASS_MONK)
						{
							client_classes[i] = CLASS_MONK + stats[i]->playerRace; // monster specific classes.
						}
						stats[i]->stat_appearance = 0;
					}
				}
				else
				{
					stats[i]->playerRace = RACE_HUMAN;
					stats[i]->stat_appearance = local_rng.rand() % 18;
				}
				strcpy(stats[i]->name, randomPlayerNamesFemale[local_rng.rand() % randomPlayerNamesFemale.size()].c_str());
				bool oldIntro = intro;
				intro = true; // so initClass doesn't add items to hotbar.
				initClass(i);
				intro = oldIntro;
			}
		}
		for (auto& input : Input::inputs) {
			input.refresh();
		}
		});

#ifndef NDEBUG
	static ConsoleCommand ccmd_unlock_achievement("/unlockachievement", "", []CCMD{
		if (argc > 1) {
			steamAchievement(argv[1]);
		}
		});
#endif

	static ConsoleCommand ccmd_gamepad_deadzone("/gamepad_deadzone", "", []CCMD{
		if (argc < 2)
		{
			return;
		}
		gamepad_deadzone = atoi(argv[1]);
		//Ensure its value is in range.
		gamepad_deadzone = std::max(gamepad_deadzone, 0);
		printlog("Controller deadzone is %d.", gamepad_deadzone);
		});

	static ConsoleCommand ccmd_gamepad_trigger_deadzone("/gamepad_trigger_deadzone", "", []CCMD{
		if (argc < 2)
		{
			return;
		}
		gamepad_trigger_deadzone = atoi(argv[1]);
		//Ensure its value is in range.
		gamepad_trigger_deadzone = std::max(gamepad_trigger_deadzone, 0);
		printlog("Controller trigger deadzone is %d.", gamepad_trigger_deadzone);
		});

	static ConsoleCommand ccmd_gamepad_leftx_sensitivity("/gamepad_leftx_sensitivity", "", []CCMD{
		if (argc < 2)
		{
			return;
		}
		gamepad_leftx_sensitivity = strtof(argv[1], nullptr);
		//Ensure its value is in range.
		gamepad_leftx_sensitivity = std::max(gamepad_leftx_sensitivity, 1.0);
		printlog("Controller leftx sensitivity is %.1f.", gamepad_leftx_sensitivity);
		});

	static ConsoleCommand ccmd_gamepad_lefty_sensitivity("/gamepad_lefty_sensitivity", "", []CCMD{
		if (argc < 2)
		{
			return;
		}
		gamepad_lefty_sensitivity = strtof(argv[1], nullptr);
		//Ensure its value is in range.
		gamepad_lefty_sensitivity = std::max(gamepad_lefty_sensitivity, 1.0);
		printlog("Controller lefty sensitivity is %.1f.", gamepad_lefty_sensitivity);
		});

	static ConsoleCommand ccmd_gamepad_rightx_sensitivity("/gamepad_rightx_sensitivity", "", []CCMD{
		if (argc < 2)
		{
			return;
		}
		gamepad_rightx_sensitivity = strtof(argv[1], nullptr);
		//Ensure its value is in range.
		gamepad_rightx_sensitivity = std::max(gamepad_rightx_sensitivity, 1.0);
		printlog("Controller rightx sensitivity is %.1f.", gamepad_rightx_sensitivity);
		});

	static ConsoleCommand ccmd_gamepad_righty_sensitivity("/gamepad_righty_sensitivity", "", []CCMD{
		if (argc < 2)
		{
			return;
		}
		gamepad_righty_sensitivity = strtof(argv[1], nullptr);
		//Ensure its value is in range.
		gamepad_righty_sensitivity = std::max(gamepad_righty_sensitivity, 1.0);
		printlog("Controller righty sensitivity is %.1f.", gamepad_righty_sensitivity);
		});

	static ConsoleCommand ccmd_gamepad_menux_sensitivity("/gamepad_menux_sensitivity", "", []CCMD{
		if (argc < 2)
		{
			return;
		}
		gamepad_menux_sensitivity = strtof(argv[1], nullptr);
		//Ensure its value is in range.
		gamepad_menux_sensitivity = std::max(gamepad_menux_sensitivity, 1.0);
		printlog("Controller menux sensitivity is %.1f.", gamepad_menux_sensitivity);
		});

	static ConsoleCommand ccmd_gamepad_menuy_sensitivity("/gamepad_menuy_sensitivity", "", []CCMD{
		if (argc < 2)
		{
			return;
		}
		gamepad_menuy_sensitivity = strtof(argv[1], nullptr);
		//Ensure its value is in range.
		gamepad_menuy_sensitivity = std::max(gamepad_menuy_sensitivity, 1.0);
		printlog("Controller menuy sensitivity is %.1f.", gamepad_menuy_sensitivity);
		});

	static ConsoleCommand ccmd_gamepad_leftx_invert("/gamepad_leftx_invert", "", []CCMD{
		gamepad_leftx_invert = true;
		});

	static ConsoleCommand ccmd_gamepad_lefty_invert("/gamepad_lefty_invert", "", []CCMD{
		gamepad_lefty_invert = true;
		});

	static ConsoleCommand ccmd_gamepad_rightx_invert("/gamepad_rightx_invert", "", []CCMD{
		gamepad_rightx_invert = true;
		});

	static ConsoleCommand ccmd_gamepad_righty_invert("/gamepad_righty_invert", "", []CCMD{
		gamepad_righty_invert = true;
		});

	static ConsoleCommand ccmd_gamepad_menux_invert("/gamepad_menux_invert", "", []CCMD{
		gamepad_menux_invert = true;
		});

	static ConsoleCommand ccmd_gamepad_menuy_invert("/gamepad_menuy_invert", "", []CCMD{
		gamepad_menuy_invert = true;
		});

	static ConsoleCommand ccmd_numgold("/numgold", "tell how much gold the given player has (eg: /numgold 0)", []CCMD{
		for (unsigned i = 0; i < MAXPLAYERS; ++i)
		{
			if (client_disconnected[i])
			{
				continue;
			}
			messagePlayer(clientnum, MESSAGE_MISC, "Player %d has %d gold.", i, stats[i]->GOLD);
		}
		});

	static ConsoleCommand ccmd_gold("/gold", "give the player gold (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		if (multiplayer != SINGLE)
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(299));
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

	static ConsoleCommand ccmd_dropgold("/dropgold", "drop some gold", []CCMD{
		if (argc < 3)
		{
			messagePlayer(clientnum, MESSAGE_MISC, "Please include the amount of gold to drop and player num. (eg: /dropgold 0 10)");
			return;
		}
        
        // select player
		const int player = (int)strtol(argv[1], nullptr, 10);
        if (player < 0 || player >= MAXPLAYERS)
        {
            return;
        }
        if (!players[player]->isLocalPlayerAlive())
        {
			return;
		}
		if (stats[player]->GOLD < 0)
		{
			stats[player]->GOLD = 0;
		}
        
        // select gold
        int amount = (int)strtol(argv[2], nullptr, 10);
        if (amount > stats[player]->GOLD)
        {
            amount = stats[player]->GOLD;
        }
        if (amount < 0)
        {
            amount = 0;
        }

		// drop gold
		int x = std::min<int>(std::max(0, (int)(players[player]->entity->x / 16)), map.width - 1);
		int y = std::min<int>(std::max(0, (int)(players[player]->entity->y / 16)), map.height - 1);
		if (map.tiles[y * MAPLAYERS + x * MAPLAYERS * map.height])
		{
			if (amount == 0)
			{
				messagePlayer(player, MESSAGE_INVENTORY, Language::get(2593));
				return;
			}
			stats[player]->GOLD -= amount;
			stats[player]->GOLD = std::max(stats[player]->GOLD, 0);
			if (multiplayer == CLIENT)
			{
				//Tell the server we dropped some gold.
				strcpy((char*)net_packet->data, "DGLD");
				net_packet->data[4] = player;
				SDLNet_Write32(amount, &net_packet->data[5]);
				net_packet->address.host = net_server.host;
				net_packet->address.port = net_server.port;
				net_packet->len = 9;
				sendPacketSafe(net_sock, -1, net_packet, 0);
			}
			else
			{
				playSoundEntity(players[player]->entity, 242 + local_rng.rand() % 4, 64);
				auto entity = newEntity(amount < 5 ? 1379 : 130, 0, map.entities, nullptr); // 130 = goldbag model
				entity->goldAmount = amount; // amount
				entity->sizex = 4;
				entity->sizey = 4;
				entity->x = players[player]->entity->x;
				entity->y = players[player]->entity->y;
				entity->z = 0;
				entity->vel_z = (-40 - local_rng.rand() % 5) * .01;
				entity->goldBouncing = 0;
				entity->yaw = (local_rng.rand() % 360) * PI / 180.0;
				entity->flags[PASSABLE] = true;
				entity->flags[UPDATENEEDED] = true;
				entity->behavior = &actGoldBag;
			}
			messagePlayer(player, MESSAGE_INVENTORY, Language::get(2594), amount);
		}
		else
		{
			messagePlayer(player, MESSAGE_INVENTORY | MESSAGE_MISC, Language::get(4085)); // invalid location to drop gold
		}
		});

	static ConsoleCommand ccmd_minotaurlevel("/minotaurlevel", "create a minotaur timer (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		if (multiplayer != SINGLE)
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(299));
			return;
		}

		if (!minotaurlevel)
		{
			minotaurlevel = 1;
			createMinotaurTimer(players[0]->entity, &map, local_rng.getU32());
		}
		});

	static ConsoleCommand ccmd_minotaurnow("/minotaurnow", "summon the minotaur (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		if (multiplayer != SINGLE)
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(299));
			return;
		}

		if (minotaurlevel)
		{
			node_t* tmpNode = NULL;
			Entity* tmpEnt = NULL;
			for (tmpNode = map.entities->first; tmpNode != NULL; tmpNode = tmpNode->next)
			{
				tmpEnt = (Entity*)tmpNode->element;
				if (tmpEnt->sprite == 37)
				{
					tmpEnt->skill[0] += TICKS_PER_SECOND * 210;
					return;
				}
			}
		}
		});

	static ConsoleCommand ccmd_levelskill("/levelskill", "increase a skill (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		if (multiplayer != SINGLE)
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(299));
			return;
		}

		if (argc < 2)
		{
			return;
		}
		int skill = atoi(argv[1]);
		if (skill >= NUMPROFICIENCIES)
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(3239)); //Skill out of range.
		}
		else
		{
			for (int i = 0; i < 10; ++i)
			{
				players[clientnum]->entity->increaseSkill(skill);
			}
		}
		});

	static ConsoleCommand ccmd_maplevel("/maplevel", "magic mapping for the level (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		if (multiplayer != SINGLE)
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(299));
			return;
		}

		messagePlayer(clientnum, MESSAGE_MISC, Language::get(412));

		mapLevel(clientnum);
		});

	static ConsoleCommand ccmd_maplevel2("/maplevel2", "magic mapping for the level (cheat)", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		if ( multiplayer != SINGLE )
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(299));
			return;
		}

		messagePlayer(clientnum, MESSAGE_MISC, Language::get(412));

		mapLevel2(clientnum);
		});

	static ConsoleCommand ccmd_maplevel3("/maplevel3", "magic mapping for the level (cheat)", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		if ( multiplayer != SINGLE )
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(299));
			return;
		}

		if ( Player::getPlayerInteractEntity(clientnum) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(412));
			shrineDaedalusRevealMap(*Player::getPlayerInteractEntity(clientnum));
		}
		});

	static ConsoleCommand ccmd_drunky("/drunky", "make me drunk (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		if (multiplayer != SINGLE)
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(299));
			return;
		}

		if (!players[clientnum]->entity->getStats()->EFFECTS[EFF_DRUNK])
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

	static ConsoleCommand ccmd_maxskill("/maxskill", "max out player skills (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		if (multiplayer != SINGLE)
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(299));
			return;
		}

		if (argc < 2)
		{
			return;
		}
		int skill = atoi(argv[1]);
		if (skill >= NUMPROFICIENCIES)
		{
			messagePlayer(clientnum, MESSAGE_MISC, "Invalid skill ID"); //Skill out of range.
		}
		else
		{
			for (int i = players[clientnum]->entity->getStats()->getProficiency(skill); i < 100; ++i)
			{
				players[clientnum]->entity->increaseSkill(skill);
			}
		}
		});

	static ConsoleCommand ccmd_reloadlimbs("/reloadlimbs", "reload limb files", []CCMD{
		int x;
		File* fp;
		bool success = true;

		if (!autoLimbReload)
		{
			messagePlayer(clientnum, MESSAGE_MISC, "Reloading limb offsets from limbs.txt files...");
		}

		for (int c = 1; c < NUMMONSTERS; c++)
		{
			// initialize all offsets to zero
			for (x = 0; x < 20; x++)
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
			if ((fp = openDataFile(filename, "rb")) == NULL)
			{
				continue;
			}

			// read file
			int line;
			for (line = 1; !fp->eof(); line++)
			{
				char data[256];
				int limb = 20;
				int dummy;

				// read line from file
				fp->gets(data, 256);

				// skip blank and comment lines
				if (data[0] == '\n' || data[0] == '\r' || data[0] == '#')
				{
					continue;
				}

				// process line
				if (sscanf(data, "%d", &limb) != 1 || limb >= 20 || limb < 0)
				{
					messagePlayer(clientnum, MESSAGE_MISC, "warning: syntax error in '%s':%d\n invalid limb index!", filename, line);
					printlog("warning: syntax error in '%s':%d\n invalid limb index!\n", filename, line);
					success = false;
					continue;
				}
				if (sscanf(data, "%d %f %f %f\n", &dummy, &limbs[c][limb][0], &limbs[c][limb][1], &limbs[c][limb][2]) != 4)
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
		if (success && !autoLimbReload)
		{
			messagePlayer(clientnum, MESSAGE_MISC, "Successfully reloaded all limbs.txt!");
		}
		});

	static ConsoleCommand ccmd_animspeed("/animspeed", "change animation speed (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		if (multiplayer != SINGLE)
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(299));
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

	static ConsoleCommand ccmd_atkspeed("/atkspeed", "change attack speed (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		if (multiplayer != SINGLE)
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(299));
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

	static ConsoleCommand ccmd_loadmod("/loadmod", "load the specified mod", []CCMD{
		std::string dir, name, fileid;
		for (int c = 1; c < argc; ++c)
		{
			std::string cmd = argv[c];
			std::size_t dirfind = cmd.find("dir:");
			if (dirfind != std::string::npos) {
				dir = cmd.substr(dirfind + 4);
				continue;
			}
			std::size_t namefind = cmd.find("name:");
			if (namefind != std::string::npos) {
				name = cmd.substr(namefind + 5);
				continue;
			}
			std::size_t fileidFind = cmd.find("fileid:");
			if (fileidFind != std::string::npos) {
				fileid = cmd.substr(fileidFind + 7);
				continue;
			}
		}
		std::string modname;
		if (!dir.empty() && !name.empty())
		{
			if (fileid.empty())
			{
				std::string directory = dir;
				modname = name;
				//TODO is this still necessary?
				//modname = modname.substr(0, modname.length() - 1);
				printlog("[Mods]: Adding mod \"%s\" in path \"%s\"", directory.c_str(), modname.c_str());
				Mods::mountedFilepaths.push_back(std::make_pair(directory, modname));
			}
#ifdef STEAMWORKS
			else
			{
				std::string directory = dir;
				modname = name;
				printlog("[Mods]: Adding mod \"%s\" in path \"%s\"", directory.c_str(), modname.c_str());
				Mods::mountedFilepaths.push_back(std::make_pair(directory, modname));

				uint64 id = atoi(fileid.c_str());
				Mods::workshopLoadedFileIDMap.push_back(std::make_pair(modname, id));
				printlog("[Mods]: Steam Workshop mod file ID added for previous entry:%lld", id);
			}
#endif
		}
		});

	static ConsoleCommand ccmd_muteaudiofocuslost("/muteaudiofocuslost", "", []CCMD{
		mute_audio_on_focus_lost = (mute_audio_on_focus_lost == false);
		});

	static ConsoleCommand ccmd_muteplayermonstersounds("/muteplayermonstersounds", "", []CCMD{
		mute_player_monster_sounds = (mute_player_monster_sounds == false);
		});

	static ConsoleCommand ccmd_minimaptransparencyfg("/minimaptransparencyfg", "", []CCMD{
		if (argc < 2)
		{
			return;
		}
		minimapTransparencyForeground = atoi(argv[1]);
		minimapTransparencyForeground = std::min(std::max<int>(0, minimapTransparencyForeground), 100);

		});

	static ConsoleCommand ccmd_minimaptransparencybg("/minimaptransparencybg", "", []CCMD{
		minimapTransparencyBackground = atoi(argv[1]);
		minimapTransparencyBackground = std::min(std::max<int>(0, minimapTransparencyBackground), 100);

		});

	static ConsoleCommand ccmd_minimapscale("/minimapscale", "", []CCMD{
		if (argc < 2)
		{
			return;
		}
		minimapScale = atoi(argv[1]);
		minimapScale = std::min(std::max<int>(2, minimapScale), 16);
		});

	static ConsoleCommand ccmd_minimapobjectzoom("/minimapobjectzoom", "", []CCMD{
		if (argc < 2)
		{
			return;
		}
		minimapObjectZoom = atoi(argv[1]);
		minimapObjectZoom = std::min(std::max<int>(0, minimapObjectZoom), 4);
		});

	static ConsoleCommand ccmd_uiscale_charsheet("/uiscale_charsheet", "", []CCMD{
		uiscale_charactersheet = !uiscale_charactersheet;
		});

	static ConsoleCommand ccmd_uiscale_skillsheet("/uiscale_skillsheet", "", []CCMD{
		uiscale_skillspage = !uiscale_skillspage;
		});

	static ConsoleCommand ccmd_hidestatusbar("/hidestatusbar", "", []CCMD{
		//hide_statusbar = !hide_statusbar;
		});

	static ConsoleCommand ccmd_hideplayertags("/hideplayertags", "", []CCMD{
		hide_playertags = !hide_playertags;
		});

	static ConsoleCommand ccmd_showskillvalues("/showskillvalues", "", []CCMD{
		show_skill_values = !show_skill_values;
		});

	static ConsoleCommand ccmd_disablenetworkmultithreading("/disablenetworkmultithreading", "", []CCMD{
		disableMultithreadedSteamNetworking = true;// !disableMultithreadedSteamNetworking;
		});

	static ConsoleCommand ccmd_autolimbreload("/autolimbreload", "", []CCMD{
		autoLimbReload = !autoLimbReload;
		});

	static ConsoleCommand ccmd_togglesecretlevel("/togglesecretlevel", "put the player on the secret level track (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		if (multiplayer != SINGLE)
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(299));
			return;
		}
		Compendium_t::Events_t::previousSecretlevel = secretlevel;
		secretlevel = (secretlevel == false);
		});

	static ConsoleCommand ccmd_seteffect("/seteffect", "give the player the specified effect (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		if (multiplayer != SINGLE)
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(299));
			return;
		}

		if (argc < 2)
		{
			return;
		}
		int effect = atoi(argv[1]);
		if (effect >= NUMEFFECTS || effect < 0 || !players[clientnum]->entity)
		{
			return;
		}
		else
		{
			int duration = 500;
			if ( argc >= 3 )
			{
				duration = atoi(argv[2]);
			}
			players[clientnum]->entity->setEffect(effect, true, duration, true);
		}
		});

	static ConsoleCommand ccmd_seteffect_rand("/seteffect_rand", "give assortment of effects (cheat)", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		if ( multiplayer != SINGLE )
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(299));
			return;
		}

		int num = local_rng.rand() % 10;
		std::vector<unsigned int> effects;
		for ( int i = 0; i < NUMEFFECTS; ++i )
		{
			effects.push_back(1);
		}
		while ( num > 0 && players[clientnum]->entity )
		{
			--num;
			auto picked = local_rng.discrete(effects.data(), effects.size());
			effects[picked] = 0;
			players[clientnum]->entity->setEffect(picked, true, TICKS_PER_SECOND * 60, true);
		}
		});

	static ConsoleCommand ccmd_levelsummon("/levelsummon", "level up monster summons (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		for (node_t* node = map.creatures->first; node != nullptr; node = node->next)
		{
			Entity* entity = (Entity*)node->element;
			if (entity && entity->behavior == &actMonster && entity->monsterAllySummonRank != 0)
			{
				Stat* entityStats = entity->getStats();
				if (entityStats)
				{
					entityStats->EXP += 100;
				}
			}
		}
		return;
		});

	static ConsoleCommand ccmd_brawlermode("/brawlermode", "activate brawler mode", []CCMD{
		achievementBrawlerMode = !achievementBrawlerMode;
		if (achievementBrawlerMode && conductGameChallenges[CONDUCT_BRAWLER])
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(2995));
		}
		else if (achievementBrawlerMode && !conductGameChallenges[CONDUCT_BRAWLER])
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(2998));
		}
		else if (!achievementBrawlerMode)
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(2996));
		}
		});

	static ConsoleCommand ccmd_pennilessmode("/pennilessmode", "activate penniless mode", []CCMD{
		achievementPenniless = !achievementPenniless;
		if ( achievementPenniless && conductPenniless )
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(6058));
		}
		else if ( achievementPenniless && !conductPenniless )
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(6060));
		}
		else if ( !achievementPenniless )
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(6059));
		}
		});

	static ConsoleCommand ccmd_rangermode("/rangermode", "activate ranger mode", []CCMD{
		int player = -1;
		if (argc > 1)
		{
			player = atoi(argv[1]);
		}
		else
		{
			player = 0;
		}

		if (multiplayer == CLIENT)
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(284));
			return;
		}

		achievementRangedMode[player] = !achievementRangedMode[player];
		if (multiplayer == SERVER)
		{
			if (player != clientnum)
			{
				if (achievementRangedMode[player])
				{
					messagePlayer(clientnum, MESSAGE_MISC, Language::get(3926), player);
				}
				else
				{
					messagePlayer(clientnum, MESSAGE_MISC, Language::get(3925), player);
				}
			}
		}
		if (achievementRangedMode[player] && !playerFailedRangedOnlyConduct[player])
		{
			messagePlayer(player, MESSAGE_MISC, Language::get(3921));
		}
		else if (achievementRangedMode[player] && playerFailedRangedOnlyConduct[player])
		{
			messagePlayer(player, MESSAGE_MISC, Language::get(3924));
		}
		else if (!achievementRangedMode[player])
		{
			messagePlayer(player, MESSAGE_MISC, Language::get(3922));
		}
		});

	static ConsoleCommand ccmd_gimmevictory("/gimmevictory", "win without trying", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}

		if (multiplayer != SINGLE)
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(299));
			return;
		}

		if (argc > 1) {
			victory = (int)strtol(argv[1], nullptr, 10);
		}

		messagePlayer(clientnum, MESSAGE_MISC, "Victory is %d", victory);
		});

	static ConsoleCommand ccmd_gimmeconducts("/gimmeconducts", "inflate your ego", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}

		if (multiplayer != SINGLE)
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(299));
			return;
		}

		for (int c = 0; c < NUM_CONDUCT_CHALLENGES; ++c) {
			++conductGameChallenges[c];
		}

		messagePlayer(clientnum, MESSAGE_MISC, "Gave you some conducts");
		});

	static ConsoleCommand ccmd_gimmekills("/gimmekills", "inflate your kill stats", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}

		if (multiplayer != SINGLE)
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(299));
			return;
		}

		for (int c = 0; c < NUMMONSTERS; ++c) {
			++kills[c];
		}

		messagePlayer(clientnum, MESSAGE_MISC, "Gave you some kills");
		});

	static ConsoleCommand ccmd_gimmepotions2("/gimmepotions2", "give the player some potions (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}

		if (multiplayer != SINGLE)
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(299));
			return;
		}

		ItemType item1 = POTION_BOOZE;
		ItemType item2 = POTION_BLINDNESS;

		Item* potion = newItem(item1, EXCELLENT, 0, 1, 0, true, nullptr);
		itemPickup(clientnum, potion);
		potion = newItem(item1, SERVICABLE, 0, 3, 0, true, nullptr);
		itemPickup(clientnum, potion);
		potion = newItem(item1, WORN, 0, 1, 0, true, nullptr);
		itemPickup(clientnum, potion);
		potion = newItem(item1, DECREPIT,	0, 1, 0, true, nullptr);
		itemPickup(clientnum, potion);
		potion = newItem(item1, SERVICABLE, 1, 2, 0, true, nullptr);
		itemPickup(clientnum, potion);
		potion = newItem(item1, SERVICABLE, 2, 2, 0, true, nullptr);
		itemPickup(clientnum, potion);
		potion = newItem(item1, EXCELLENT, -1, 2, 0, true, nullptr);
		itemPickup(clientnum, potion);
		potion = newItem(item1, EXCELLENT, -2, 2, 0, true, nullptr);
		itemPickup(clientnum, potion);

		potion = newItem(item2, EXCELLENT, 0, 2, 0, true, nullptr);
		itemPickup(clientnum, potion);
		potion = newItem(item2, WORN, 0, 1, 0, true, nullptr);
		itemPickup(clientnum, potion);
		potion = newItem(item2, SERVICABLE, 0, 1, 0, true, nullptr);
		itemPickup(clientnum, potion);
		potion = newItem(item2, DECREPIT, 0, 4, 0, true, nullptr);
		itemPickup(clientnum, potion);
		potion = newItem(item2, SERVICABLE, 1, 2, 0, true, nullptr);
		itemPickup(clientnum, potion);
		potion = newItem(item2, SERVICABLE, 2, 2, 0, true, nullptr);
		itemPickup(clientnum, potion);
		potion = newItem(item2, EXCELLENT, -1, 2, 0, true, nullptr);
		itemPickup(clientnum, potion);
		potion = newItem(item2, EXCELLENT, -2, 2, 0, true, nullptr);
		itemPickup(clientnum, potion);
		});

	static ConsoleCommand ccmd_gimmepotions("/gimmepotions", "give the player some potions (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}

		if (multiplayer != SINGLE)
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(299));
			return;
		}

		if (argc >= 2)
		{
			int count = std::max(1, atoi(argv[1]));
			for (int i = 0; i < 17; ++i)
			{
				auto generatedPotion = potionStandardAppearanceMap.at(i);
				Item* potion = newItem(static_cast<ItemType>(generatedPotion.first), static_cast<Status>(SERVICABLE + local_rng.rand() % 2),
					0, count, generatedPotion.second, true, nullptr);
				itemPickup(clientnum, potion);
			}
			return;
		}

		std::vector<unsigned int> potionChances =
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

		for (int i = 0; i < 10; ++i)
		{
			auto generatedPotion = potionStandardAppearanceMap.at(
				local_rng.discrete(potionChances.data(), potionChances.size()));
			Item* potion = newItem(static_cast<ItemType>(generatedPotion.first), static_cast<Status>(SERVICABLE + local_rng.rand() % 2),
				0, 1, generatedPotion.second, true, nullptr);
			itemPickup(clientnum, potion);
			//free(potion);
		}
		});

	static ConsoleCommand ccmd_hungoverstats("/hungoverstats", "display stats on drunkenness (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}

		if (multiplayer != SINGLE)
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(299));
			return;
		}

		messagePlayer(clientnum, MESSAGE_MISC, "Hungover Active: %d, Time to go: %d, Drunk Active: %d, Drunk time: %d",
			stats[clientnum]->EFFECTS[EFF_WITHDRAWAL], stats[clientnum]->EFFECTS_TIMERS[EFF_WITHDRAWAL],
			stats[clientnum]->EFFECTS[EFF_DRUNK], stats[clientnum]->EFFECTS_TIMERS[EFF_DRUNK]);
		return;
		});

	static ConsoleCommand ccmd_debugtimers("/debugtimers", "", []CCMD{
		logCheckMainLoopTimers = !logCheckMainLoopTimers;
		});

	static ConsoleCommand ccmd_entityfreeze("/entityfreeze", "freeze all entities (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		gameloopFreezeEntities = !gameloopFreezeEntities;
		});

	static ConsoleCommand ccmd_tickrate("/tickrate", "set game tick rate", []CCMD{
		if (argc < 2)
		{
			return;
		}
		networkTickrate = atoi(argv[1]);
		networkTickrate = std::max<Uint32>(1, networkTickrate);
		messagePlayer(clientnum, MESSAGE_MISC, "Set tickrate to %d, network processing allowed %3.0f percent of frame limit interval. Default value 2.",
			networkTickrate, 100.f / networkTickrate);
		});

	static ConsoleCommand ccmd_disablenetcodefpslimit("/disablenetcodefpslimit", "", []CCMD{
		disableFPSLimitOnNetworkMessages = !disableFPSLimitOnNetworkMessages;
		});

	static ConsoleCommand ccmd_allspells1("/allspells1", "teach player some spells (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}

		for (auto it = allGameSpells.begin(); it != allGameSpells.begin() + 29; ++it)
		{
			spell_t* spell = *it;
			bool oldIntro = intro;
			intro = true;
			bool learned = addSpell(spell->ID, clientnum, true);
			intro = oldIntro;
		}
		return;
		});

	static ConsoleCommand ccmd_setmapseed("/setmapseed", "set the next map seed (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		if (multiplayer == CLIENT)
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(284));
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

	static ConsoleCommand ccmd_greaseme("/greaseme", "make the player greasy (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		if (multiplayer == CLIENT)
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(284));
			return;
		}
		if (players[clientnum] && players[clientnum]->entity)
		{
			players[clientnum]->entity->setEffect(EFF_GREASY, true, TICKS_PER_SECOND * 20, false);
		}
		});

	static ConsoleCommand ccmd_gimmearrows("/gimmearrows", "give the player some arrows (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		for (int i = QUIVER_SILVER; i <= QUIVER_HUNTING; ++i)
		{
			dropItem(newItem(static_cast<ItemType>(i), EXCELLENT, 0, 25 + local_rng.rand() % 26, local_rng.rand(), true, &stats[clientnum]->inventory), 0);
		}
		});

	static ConsoleCommand ccmd_gimmescrap("/gimmescrap", "give the player some scrap metal (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		dropItem(newItem(TOOL_METAL_SCRAP, EXCELLENT, 0, 100, local_rng.rand(), true, &stats[clientnum]->inventory), 0);
		dropItem(newItem(TOOL_MAGIC_SCRAP, EXCELLENT, 0, 100, local_rng.rand(), true, &stats[clientnum]->inventory), 0);
		dropItem(newItem(TOOL_TINKERING_KIT, EXCELLENT, 0, 1, local_rng.rand(), true, &stats[clientnum]->inventory), 0);
		});

	static ConsoleCommand ccmd_gimmerobots("/gimmerobots", "give the player some robots (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		dropItem(newItem(TOOL_GYROBOT, EXCELLENT, 0, 10, local_rng.rand(), true, &stats[clientnum]->inventory), 0);
		dropItem(newItem(TOOL_DUMMYBOT, EXCELLENT, 0, 10, local_rng.rand(), true, &stats[clientnum]->inventory), 0);
		dropItem(newItem(TOOL_SENTRYBOT, EXCELLENT, 0, 10, local_rng.rand(), true, &stats[clientnum]->inventory), 0);
		dropItem(newItem(TOOL_SPELLBOT, EXCELLENT, 0, 10, local_rng.rand(), true, &stats[clientnum]->inventory), 0);
		});

	static ConsoleCommand ccmd_toggletinkeringlimits("/toggletinkeringlimits", "", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		overrideTinkeringLimit = !overrideTinkeringLimit;
		if (overrideTinkeringLimit)
		{
			messagePlayer(clientnum, MESSAGE_MISC, "Disabled tinkering bot limit");
		}
		else
		{
			messagePlayer(clientnum, MESSAGE_MISC, "Re-enabled tinkering bot limit");
		}
		});

	static ConsoleCommand ccmd_setdecoyrange("/setdecoyrange", "", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		if (multiplayer == CLIENT)
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(284));
			return;
		}
		if (argc < 2)
		{
			return;
		}
		decoyBoxRange = atoi(argv[1]);
		messagePlayer(clientnum, MESSAGE_MISC, "Set decoy range to %d", decoyBoxRange);
		});

	static ConsoleCommand ccmd_gimmegoblinbooks("/gimmegoblinbooks", "give the player some spellbooks (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		for (int i = 0; i < NUM_SPELLS; ++i)
		{
			int spellbook = getSpellbookFromSpellID(i);
			dropItem(newItem(static_cast<ItemType>(spellbook), DECREPIT, -1, 1, local_rng.rand(), true, &stats[clientnum]->inventory), 0);
		}
		});

	static ConsoleCommand ccmd_unsetdlc2achievements("/unsetdlc2achievements", "", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
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
		for (int i = STEAM_STAT_TRASH_COMPACTOR; i < 43; ++i)
		{
			g_SteamStats[i].m_iValue = 0;
			SteamUserStats()->SetStat(g_SteamStats[i].m_pchStatName, 0);
		}
		SteamUserStats()->StoreStats();
#endif // STEAMWORKS
		});

	static ConsoleCommand ccmd_gimmebombs("/gimmebombs", "give the player some bombs (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		dropItem(newItem(TOOL_BOMB, EXCELLENT, 0, 10, local_rng.rand(), true, &stats[clientnum]->inventory), 0);
		dropItem(newItem(TOOL_FREEZE_BOMB, EXCELLENT, 0, 10, local_rng.rand(), true, &stats[clientnum]->inventory), 0);
		dropItem(newItem(TOOL_TELEPORT_BOMB, EXCELLENT, 0, 10, local_rng.rand(), true, &stats[clientnum]->inventory), 0);
		dropItem(newItem(TOOL_SLEEP_BOMB, EXCELLENT, 0, 10, local_rng.rand(), true, &stats[clientnum]->inventory), 0);
		});

	static ConsoleCommand ccmd_showhunger("/showhunger", "show the player's hunger value (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		messagePlayer(clientnum, MESSAGE_MISC, "Hunger value: %d", stats[clientnum]->HUNGER);
		});

	static ConsoleCommand ccmd_disablemouserotationlimit("/disablemouserotationlimit", "", []CCMD{
		disablemouserotationlimit = (disablemouserotationlimit == false);
		});

	static ConsoleCommand ccmd_usecamerasmoothing("/usecamerasmoothing", "", []CCMD{
		usecamerasmoothing = (usecamerasmoothing == false);
		});

	static ConsoleCommand ccmd_dumpnetworkdata("/dumpnetworkdata", "", []CCMD{
		for (auto element : DebugStats.networkPackets)
		{
			printlog("Packet: %s | %d", element.second.first.c_str(), element.second.second);
		}
		});

	static ConsoleCommand ccmd_dumpentudata("/dumpentudata", "", []CCMD{
		for (auto element : DebugStats.entityUpdatePackets)
		{
			printlog("Sprite: %d | %d", element.first, element.second);
		}
		});

	static ConsoleCommand ccmd_borderless("/borderless", "toggle borderless mode", []CCMD{
		borderless = (!borderless);
		});

	static ConsoleCommand ccmd_jsonexportmonster("/jsonexportmonster", "", []CCMD{
		if (argc < 2)
		{
			return;
		}
		std::string name = argv[1];
		for (int arg = 2; arg < argc; ++arg) {
			name.append(" ");
			name.append(argv[arg]);
		}
		int creature = NOTHING;

		for (int i = 1; i < NUMMONSTERS; ++i)   //Start at 1 because 0 is a nothing.
		{
			if (strstr(monstertypename[i], name.c_str()))
			{
				creature = i;
				break;
			}
		}

		if (creature != NOTHING)
		{
			Stat* monsterStats = new Stat(1000 + creature);
			monsterStatCustomManager.writeAllFromStats(monsterStats);
			delete monsterStats;
		}
		});

	static ConsoleCommand ccmd_jsonexportfromcursor("/jsonexportfromcursor", "", []CCMD{
		Entity * target = entityClicked(nullptr, true, clientnum, EntityClickType::ENTITY_CLICK_USE);
		if (target)
		{
			Entity* parent = uidToEntity(target->skill[2]);
			if (target->behavior == &actMonster || (parent && parent->behavior == &actMonster))
			{
				// see if we selected a limb
				if (parent)
				{
					target = parent;
				}
			}
			monsterStatCustomManager.writeAllFromStats(target->getStats());
		}
		});

	static ConsoleCommand ccmd_jsonexportgameplaymodifiers("/jsonexportgameplaymodifiers", "", []CCMD{
		gameplayCustomManager.writeAllToDocument();
		});

	static ConsoleCommand ccmd_jsonexportmonstercurve("/jsonexportmonstercurve", "", []CCMD{
		monsterCurveCustomManager.writeSampleToDocument();
		});

	static ConsoleCommand ccmd_crossplay("/crossplay", "", []CCMD{
#if (defined STEAMWORKS && defined USE_EOS)
		EOS.CrossplayAccountManager.autologin = true;
#endif // USE_EOS
		});
#if (defined SOUND)
	static ConsoleCommand ccmd_sfxambientvolume("/sfxambientvolume", "set ambient sfx volume", []CCMD{
		if (argc < 2)
		{
			return;
		}
		sfxAmbientVolume = strtof(argv[1], nullptr);
		});

	static ConsoleCommand ccmd_sfxambientdynamic("/sfxambientdynamic", "", []CCMD{
		sfxUseDynamicAmbientVolume = !sfxUseDynamicAmbientVolume;
		if (sfxUseDynamicAmbientVolume)
		{
			messagePlayer(clientnum, MESSAGE_MISC, "Dynamic ambient volume ON");
		}
		else
		{
			messagePlayer(clientnum, MESSAGE_MISC, "Dynamic ambient volume OFF");
		}
		});

	static ConsoleCommand ccmd_sfxenvironmentdynamic("/sfxenvironmentdynamic", "", []CCMD{
		sfxUseDynamicEnvironmentVolume = !sfxUseDynamicEnvironmentVolume;
		if (sfxUseDynamicEnvironmentVolume)
		{
			messagePlayer(clientnum, MESSAGE_MISC, "Dynamic environment volume ON");
		}
		else
		{
			messagePlayer(clientnum, MESSAGE_MISC, "Dynamic environment volume OFF");
		}
		});

	static ConsoleCommand ccmd_sfxenvironmentvolume("/sfxenvironmentvolume", "set environment sfx volume", []CCMD{
		if (argc < 2)
		{
			return;
		}
		sfxEnvironmentVolume = strtof(argv[1], nullptr);
		});
#endif
	static ConsoleCommand ccmd_cyclekeyboard("/cyclekeyboard", "assign the keyboard to another player", []CCMD{
		bool found = false;
		for (int i = 0; i < MAXPLAYERS; ++i)
		{
			if (inputs.bPlayerUsingKeyboardControl(i))
			{
				found = true;
				if (i + 1 >= MAXPLAYERS)
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
		if (!found)
		{
			inputs.setPlayerIDAllowedKeyboard(0);
		}
		for (int i = 0; i < MAXPLAYERS; ++i)
		{
			Input::inputs[i].refresh();
		}
		});

	static ConsoleCommand ccmd_cyclegamepad("/cyclegamepad", "", []CCMD{
		for (int i = 0; i < MAXPLAYERS; ++i)
		{
			if (inputs.hasController(i))
			{
				int id = inputs.getControllerID(i);
				inputs.removeControllerWithDeviceID(id);
				if (i + 1 >= MAXPLAYERS)
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
		for (int c = 0; c < MAXPLAYERS; ++c)
		{
			Input::inputs[c].refresh();
		}
		});

	static ConsoleCommand ccmd_cycledeadzoneleft("/cycledeadzoneleft", "", []CCMD{
		for (int i = 0; i < MAXPLAYERS; ++i)
		{
			if (inputs.hasController(i))
			{
				switch (inputs.getController(i)->leftStickDeadzoneType)
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

	static ConsoleCommand ccmd_cycledeadzoneright("/cycledeadzoneright", "", []CCMD{
		for (int i = 0; i < MAXPLAYERS; ++i)
		{
			if (inputs.hasController(i))
			{
				switch (inputs.getController(i)->rightStickDeadzoneType)
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

	static ConsoleCommand ccmd_vibration("/vibration", "", []CCMD{
		for (int i = 0; i < MAXPLAYERS; ++i)
		{
			if (inputs.hasController(i))
			{
				inputs.getController(i)->haptics.vibrationEnabled = !inputs.getController(i)->haptics.vibrationEnabled;
				if (inputs.getController(i)->haptics.vibrationEnabled)
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

	static ConsoleCommand ccmd_vibecheck("/vibecheck", "", []CCMD{
		for (int i = 0; i < MAXPLAYERS; ++i)
		{
			if (inputs.hasController(i))
			{
				inputs.getController(i)->addRumble(GameController::Haptic_t::RUMBLE_NORMAL,
					8000, 0, TICKS_PER_SECOND, 0);
			}
		}
		});

	static ConsoleCommand ccmd_vibecheck2("/vibecheck2", "", []CCMD{
		for (int i = 0; i < MAXPLAYERS; ++i)
		{
			if (inputs.hasController(i))
			{
#ifndef DISABLE_RUMBLE
				SDL_GameControllerRumble(inputs.getController(i)->getControllerDevice(), 8000, 0, 500);
#endif
			}
		}
		});

	static ConsoleCommand ccmd_tooltipoffset("/tooltipoffset", "", []CCMD{
		if (argc < 2)
		{
			return;
		}
		int offset = atoi(argv[1]);
		Player::WorldUI_t::tooltipHeightOffsetZ = static_cast<real_t>(offset) / 10.0;
		messagePlayer(clientnum, MESSAGE_MISC, "Tooltip Z offset set to: %.1f", Player::WorldUI_t::tooltipHeightOffsetZ);
		});

	static ConsoleCommand ccmd_radialhotbar("/radialhotbar", "", []CCMD{
		players[clientnum]->hotbar.useHotbarRadialMenu = !players[clientnum]->hotbar.useHotbarRadialMenu;
		});

	static ConsoleCommand ccmd_radialhotslots("/radialhotslots", "", []CCMD{
		if (argc < 2)
		{
			return;
		}
		int slots = atoi(argv[1]);
		players[clientnum]->hotbar.radialHotbarSlots = slots;
		messagePlayer(clientnum, MESSAGE_MISC, "Slots in use: %d", slots);
		});

	static ConsoleCommand ccmd_facehotbar("/facehotbar", "", []CCMD{
		for (int i = 0; i < MAXPLAYERS; ++i)
		{
			if (inputs.bPlayerUsingKeyboardControl(i))
			{
				players[i]->hotbar.useHotbarFaceMenu = !players[i]->hotbar.useHotbarFaceMenu;
				messagePlayer(i, MESSAGE_MISC, "Face button hotbar: %d", players[i]->hotbar.useHotbarFaceMenu ? 1 : 0);
			}
		}
		});

	static ConsoleCommand ccmd_facebarinvert("/facebarinvert", "", []CCMD{
		for (int i = 0; i < MAXPLAYERS; ++i)
		{
			if (inputs.bPlayerUsingKeyboardControl(i))
			{
				players[i]->hotbar.faceMenuInvertLayout = !players[i]->hotbar.faceMenuInvertLayout;
				messagePlayer(i, MESSAGE_MISC, "Face button invert position: %d", players[i]->hotbar.faceMenuInvertLayout ? 1 : 0);
			}
		}
		});

	static ConsoleCommand ccmd_facebarquickcast("/facebarquickcast", "", []CCMD{
		for (int i = 0; i < MAXPLAYERS; ++i)
		{
			if (inputs.bPlayerUsingKeyboardControl(i))
			{
				players[i]->hotbar.faceMenuQuickCastEnabled = !players[i]->hotbar.faceMenuQuickCastEnabled;
				messagePlayer(i, MESSAGE_MISC, "Face button quickcast: %d", players[i]->hotbar.faceMenuQuickCastEnabled ? 1 : 0);
			}
		}
		});

	static ConsoleCommand ccmd_paperdoll("/paperdoll", "", []CCMD{
		for (int i = 0; i < MAXPLAYERS; ++i)
		{
			if (inputs.bPlayerUsingKeyboardControl(i))
			{
				players[i]->paperDoll.enabled = !players[i]->paperDoll.enabled;
				messagePlayer(i, MESSAGE_MISC, "Paper doll: %d", players[i]->paperDoll.enabled ? 1 : 0);
			}
		}
		});

	static ConsoleCommand ccmd_facebaralternate("/facebaralternate", "", []CCMD{
		for (int i = 0; i < MAXPLAYERS; ++i)
		{
			if (inputs.bPlayerUsingKeyboardControl(i))
			{
				players[i]->hotbar.faceMenuAlternateLayout = !players[i]->hotbar.faceMenuAlternateLayout;
				messagePlayer(i, MESSAGE_MISC, "Face button alternate: %d", players[i]->hotbar.faceMenuAlternateLayout ? 1 : 0);
			}
		}
		});

	static ConsoleCommand ccmd_inventorynew("/inventorynew", "", []CCMD{
		for (int i = 0; i < MAXPLAYERS; ++i)
		{
			if (inputs.bPlayerUsingKeyboardControl(i))
			{
				players[i]->inventoryUI.bNewInventoryLayout = !players[i]->inventoryUI.bNewInventoryLayout;
				players[i]->inventoryUI.resetInventory();
				messagePlayer(i, MESSAGE_MISC, "New Inventory layout: %d", players[i]->inventoryUI.bNewInventoryLayout ? 1 : 0);
			}
		}
		});

	static ConsoleCommand ccmd_worldui("/worldui", "", []CCMD{
		for (int i = 0; i < MAXPLAYERS; ++i)
		{
			if (inputs.bPlayerUsingKeyboardControl(i))
			{
				if (players[i]->worldUI.isEnabled())
				{
					players[i]->worldUI.disable();
				}
				else
				{
					players[i]->worldUI.enable();
				}
			}
		}
		});

#ifndef NINTENDO
	static ConsoleCommand ccmd_ircconnect("/ircconnect", "", []CCMD{
		if (IRCHandler.connect())
		{
			messagePlayer(clientnum, MESSAGE_MISC, "[IRC]: Connected.");
		}
		else
		{
			IRCHandler.disconnect();
			messagePlayer(clientnum, MESSAGE_MISC, "[IRC]: Error connecting.");
		}
		});

	static ConsoleCommand ccmd_ircdisconnect("/ircdisconnect", "", []CCMD{
		IRCHandler.disconnect();
		messagePlayer(clientnum, MESSAGE_MISC, "[IRC]: Disconnected.");
		});

	static ConsoleCommand ccmd_irc("/irc", "", []CCMD{
		if (argc < 2)
		{
			return;
		}
		std::string message = argv[1];
		message.append("\r\n");
		IRCHandler.packetSend(message);
		messagePlayer(clientnum, MESSAGE_MISC, "[IRC]: Sent message.");
		});
#endif // !NINTENDO

	static ConsoleCommand ccmd_loadtooltips("/loadtooltips", "", []CCMD{
		ItemTooltips.readTooltipsFromFile();
		messagePlayer(clientnum, MESSAGE_MISC, "Reloaded item_tooltips.json");
		});

	static ConsoleCommand ccmd_reflowtext("/reflowtext", "", []CCMD{
		bUsePreciseFieldTextReflow = !bUsePreciseFieldTextReflow;
		messagePlayer(clientnum, MESSAGE_MISC, "Set bUsePreciseFieldTextReflow to %d", bUsePreciseFieldTextReflow);
		});

	static ConsoleCommand ccmd_selectedanimcycle("/selectedanimcycle", "", []CCMD{
		bUseSelectedSlotCycleAnimation = !bUseSelectedSlotCycleAnimation;
		messagePlayer(clientnum, MESSAGE_MISC, "Set bUseSelectedSlotCycleAnimation to %d", bUseSelectedSlotCycleAnimation);
		});

	static ConsoleCommand ccmd_autoloadtooltips("/autoloadtooltips", "", []CCMD{
		ItemTooltips.autoReload = !ItemTooltips.autoReload;
		messagePlayer(clientnum, MESSAGE_MISC, "Set auto-reload to %d for item_tooltips.json", ItemTooltips.autoReload);
		});

	static ConsoleCommand ccmd_debugtooltips("/debugtooltips", "", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}

		if (multiplayer != SINGLE)
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(299));
			return;
		}
		ItemTooltips.itemDebug = !ItemTooltips.itemDebug;
		messagePlayer(clientnum, MESSAGE_MISC, "Set item-debug to %d for item_tooltips.json", ItemTooltips.itemDebug);
		});

	static ConsoleCommand ccmd_loaditems("/loaditems", "", []CCMD{
		ItemTooltips.readItemsFromFile();
		messagePlayer(clientnum, MESSAGE_MISC, "Reloaded items.json");
		});

	static ConsoleCommand ccmd_gimmeallpotions("/gimmeallpotions", "give all potions (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}

		if (multiplayer != SINGLE)
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(299));
			return;
		}
		for (int i = 0; i < potionStandardAppearanceMap.size(); ++i)
		{
			auto generatedPotion = potionStandardAppearanceMap.at(i);
			Item* potion = newItem(static_cast<ItemType>(generatedPotion.first), static_cast<Status>(SERVICABLE + local_rng.rand() % 2),
				0, 1, generatedPotion.second, true, nullptr);
			itemPickup(clientnum, potion);
			//free(potion);
		}
		});

	static ConsoleCommand ccmd_gimmeblessedpotions("/gimmeblessedpotions", "give blessed potions (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}

		if (multiplayer != SINGLE)
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(299));
			return;
		}
		for (int i = 0; i < potionStandardAppearanceMap.size(); ++i)
		{
			auto generatedPotion = potionStandardAppearanceMap.at(i);
			Item* potion = newItem(static_cast<ItemType>(generatedPotion.first), static_cast<Status>(SERVICABLE + local_rng.rand() % 2),
				1 + local_rng.rand() % 2, 1, generatedPotion.second, true, nullptr);
			itemPickup(clientnum, potion);
			//free(potion);
		}
		});

	static ConsoleCommand ccmd_gimmecursedpotions("/gimmecursedpotions", "give cursed potions (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}

		if (multiplayer != SINGLE)
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(299));
			return;
		}
		for (int i = 0; i < potionStandardAppearanceMap.size(); ++i)
		{
			auto generatedPotion = potionStandardAppearanceMap.at(i);
			Item* potion = newItem(static_cast<ItemType>(generatedPotion.first), static_cast<Status>(SERVICABLE + local_rng.rand() % 2),
				-2 + local_rng.rand() % 2, 1, generatedPotion.second, true, nullptr);
			itemPickup(clientnum, potion);
			//free(potion);
		}
		});

	static ConsoleCommand ccmd_allspells2("/allspells2", "teach the player some spells (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}

		for (auto it = allGameSpells.begin() + 29; it != allGameSpells.end(); ++it)
		{
			spell_t* spell = *it;
			bool oldIntro = intro;
			intro = true;
			bool learned = addSpell(spell->ID, clientnum, true);
			intro = oldIntro;
		}
		return;
		});

	static ConsoleCommand ccmd_allspells3("/allspells3", "teach the player some spells (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}

		for (auto it = allGameSpells.begin(); it != allGameSpells.end(); ++it)
		{
			spell_t* spell = *it;
			bool oldIntro = intro;
			intro = true;
			bool learned = addSpell(spell->ID, clientnum, true);
			intro = oldIntro;
		}
		return;
		});

	static ConsoleCommand ccmd_gimmexp("/gimmexp", "give the player some XP (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}

		if (players[clientnum] && players[clientnum]->entity)
		{
			players[clientnum]->entity->getStats()->EXP += 1 + local_rng.rand() % 50;
		}
		});

	static ConsoleCommand ccmd_loadhudsettings("/loadhudsettings", "", []CCMD{
		loadHUDSettingsJSON();
		messagePlayer(clientnum, MESSAGE_MISC, "Reloaded HUD_settings.json");
		});

	static ConsoleCommand ccmd_loadskillsheet("/loadskillsheet", "", []CCMD{
		Player::SkillSheet_t::loadSkillSheetJSON();
		messagePlayer(clientnum, MESSAGE_MISC, "Reloaded skillsheet_entries.json");
		});

	static ConsoleCommand ccmd_loadcharsheet("/loadcharsheet", "", []CCMD{
		Player::CharacterSheet_t::loadCharacterSheetJSON();
		messagePlayer(clientnum, MESSAGE_MISC, "Reloaded charsheet_settings.json");
		});

	static ConsoleCommand ccmd_loadfollowerwheel("/loadfollowerwheel", "", []CCMD{
		FollowerRadialMenu::loadFollowerJSON();
		messagePlayer(clientnum, MESSAGE_MISC, "Reloaded follower_wheel.json");
		});

	static ConsoleCommand ccmd_loadcalloutwheel("/loadcalloutwheel", "", []CCMD{
		CalloutRadialMenu::loadCalloutJSON();
		messagePlayer(clientnum, MESSAGE_MISC, "Reloaded callout_wheel.json");
		});

	static ConsoleCommand ccmd_printleaderlist("/printleaderlist", "", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		Player::SkillSheet_t::generateFollowerTableForSkillsheet = true;
		messagePlayer(clientnum, MESSAGE_MISC, "On next human right click leader list will be generated.");
		});

	static ConsoleCommand ccmd_poly("/poly", "polymorph the player (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		if (players[clientnum]->entity)
		{
			spellEffectPolymorph(players[clientnum]->entity, players[clientnum]->entity, true, TICKS_PER_SECOND * 60 * 2);
		}
		});

	static ConsoleCommand ccmd_sexchange("/sexchange", "fix yourself (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		stats[clientnum]->sex = stats[clientnum]->sex == sex_t::MALE ? sex_t::FEMALE : sex_t::MALE;
		});

	static ConsoleCommand ccmd_appearances("/appearances", "", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		++stats[clientnum]->stat_appearance;
		if (stats[clientnum]->stat_appearance >= NUMAPPEARANCES)
		{
			stats[clientnum]->stat_appearance = 0;
		}
		});

	static ConsoleCommand ccmd_classdebug("/classdebug", "", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		if ( argc == 2 )
		{
#ifndef NDEBUG
			client_classes[clientnum] = std::min(NUMCLASSES - 1, std::max((int)CLASS_BARBARIAN, atoi(argv[1])));
#else
			client_classes[clientnum] = std::min((int)CLASS_MONK, std::max((int)CLASS_BARBARIAN, atoi(argv[1])));
#endif
		}
		else
		{
			client_classes[clientnum] = local_rng.rand() % (CLASS_MONK + 1);
		}
		});

	static ConsoleCommand ccmd_unpoly("/unpoly", "unpolymorph the player (cheat)", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		if (players[clientnum]->entity)
		{
			players[clientnum]->entity->setEffect(EFF_POLYMORPH, false, 0, true);
		}
		});

	static ConsoleCommand ccmd_usepaperdollmovement("/usepaperdollmovement", "", []CCMD{
		restrictPaperDollMovement = !restrictPaperDollMovement;
		messagePlayer(clientnum, MESSAGE_MISC, "Set restrictPaperDollMovement to %d", restrictPaperDollMovement);
		});

	static ConsoleCommand ccmd_exportstatue("/exportstatue", "", []CCMD{
		StatueManager.exportActive = true;
		});

	static ConsoleCommand ccmd_importstatue("/importstatue", "", []CCMD{
		if (argc < 2)
		{
			return;
		}
		int index = atoi(argv[1]);
		StatueManager.readStatueFromFile(index, "");
		});

	static ConsoleCommand ccmd_importallstatues("/importallstatues", "", []CCMD{
		StatueManager.readAllStatues();
		});

	static ConsoleCommand ccmd_refreshstatues("/refreshstatues", "", []CCMD{
		StatueManager.refreshAllStatues();
		});

	static ConsoleCommand ccmd_importandrefreshstatues("/importandrefreshstatues", "", []CCMD{
		consoleCommand("/importallstatues");
		StatueManager.refreshAllStatues();
		});

	static ConsoleCommand ccmd_resetstatueeditor("/resetstatueeditor", "", []CCMD{
		StatueManager.resetStatueEditor();
		});

	static ConsoleCommand ccmd_timertests("/timertests", "", []CCMD{
		TimerExperiments::bUseTimerInterpolation = !TimerExperiments::bUseTimerInterpolation;
		messagePlayer(clientnum, MESSAGE_MISC, "Set bUseTimerInterpolation to %d", TimerExperiments::bUseTimerInterpolation);
		});

	static ConsoleCommand ccmd_timertestsdebug("/timertestsdebug", "", []CCMD{
		TimerExperiments::bDebug = !TimerExperiments::bDebug;
		messagePlayer(clientnum, MESSAGE_MISC, "Set TimerExperiments::bDebug to %d", TimerExperiments::bDebug);
		});

	static ConsoleCommand ccmd_framesearchdebug("/framesearchdebug", "", []CCMD{
		Frame::findFrameDefaultSearchType =
			Frame::findFrameDefaultSearchType == Frame::FRAME_SEARCH_DEPTH_FIRST
			? Frame::FRAME_SEARCH_BREADTH_FIRST
			: Frame::FRAME_SEARCH_DEPTH_FIRST;
		if (Frame::findFrameDefaultSearchType == Frame::FRAME_SEARCH_DEPTH_FIRST)
		{
			messagePlayer(clientnum, MESSAGE_MISC, "Set Frame::findFrameDefaultSearchType to depth first");
		}
		else
		{
			messagePlayer(clientnum, MESSAGE_MISC, "Set Frame::findFrameDefaultSearchType to breadth first");
		}
		});

	static ConsoleCommand ccmd_debugkeys("/debugkeys", "", []CCMD{
		enableDebugKeys = !enableDebugKeys;
		messagePlayer(clientnum, MESSAGE_MISC, "Set enableDebugKeys to %d", enableDebugKeys);
		});

	static ConsoleCommand ccmd_loadglyphs("/loadglyphs", "", []CCMD{
		GlyphHelper.readFromFile();
		messagePlayer(clientnum, MESSAGE_MISC, "Reloaded keyboard glyph paths from JSON file");
		});

	static ConsoleCommand ccmd_renderglyphs("/renderglyphs", "", []CCMD{
		GlyphHelper.renderGlyphsToPNGs();
		messagePlayer(clientnum, MESSAGE_MISC, "Re-rendering keyboard glyphs...");
		});

	static ConsoleCommand ccmd_loadstatusfx("/loadstatusfx", "", []CCMD{
		StatusEffectQueue_t::loadStatusEffectsJSON();
		messagePlayer(clientnum, MESSAGE_MISC, "Reloaded status_effects.json");
		});

	static void rocksFall(int player) {
		if (!(svFlags & SV_FLAG_CHEATS)) {
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		if (multiplayer == CLIENT) {
			messagePlayer(clientnum, MESSAGE_MISC, "Only the server can do that.");
			return;
		}
		if (player < 0 || player >= MAXPLAYERS) {
			return;
		}
		if (!players[player]->entity) {
			return;
		}
		Entity* entity = newEntity(245, 1, map.entities, nullptr); // boulder
		entity->parent = players[player]->entity->getUID();
		entity->x = players[player]->entity->x;
		entity->y = players[player]->entity->y;
		entity->z = -64;
		entity->yaw = (PI / 2) * (local_rng.rand() % 4);
		entity->sizex = 7;
		entity->sizey = 7;
		entity->behavior = &actBoulder;
		entity->flags[UPDATENEEDED] = true;
		entity->flags[PASSABLE] = true;
	}

	static ConsoleCommand ccmd_rocksfall("/rocksfall", "spawns a boulder over your head", []CCMD{
		if (argc >= 2) {
			rocksFall((int)strtol(argv[1], nullptr, 10));
		}
 else {
  rocksFall(clientnum);
}
		});

	static ConsoleCommand ccmd_smite("/smite", "spawns a boulder over somebody's head", []CCMD{
		if (argc >= 2) {
			rocksFall((int)strtol(argv[1], nullptr, 10));
		}
 else {
  rocksFall(clientnum);
}
		});

	static ConsoleCommand ccmd_listalchemyrecipes("/listalchemyrecipes", "lists known alchemy recipes", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS)) {
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		if (argc >= 2) {
			int player = (int)strtol(argv[1], nullptr, 10);
			if (player < 0 || player >= MAXPLAYERS)
			{
				player = clientnum;
			}
			for (auto& entry : clientLearnedAlchemyRecipes[player])
			{
				messagePlayer(clientnum, MESSAGE_MISC, "[%s]: %s | %s",
					items[entry.first].getIdentifiedName(), items[entry.second.first].getIdentifiedName(),
					items[entry.second.second].getIdentifiedName());
			}
		}
		else {
			for (auto& entry : clientLearnedAlchemyRecipes[clientnum])
			{
				messagePlayer(clientnum, MESSAGE_MISC, "[%s]: %s | %s",
					items[entry.first].getIdentifiedName(), items[entry.second.first].getIdentifiedName(),
					items[entry.second.second].getIdentifiedName());
			}
		}
		});

	static ConsoleCommand ccmd_itemuids("/itemuids", "prints current global itemuids", []CCMD{
		messagePlayer(clientnum, MESSAGE_DEBUG, "itemuids: %d", itemuids);
		});

	static ConsoleCommand ccmd_gamepadDropdown("/gamepad_dropdown", "set gamepad to use dropdown mode: (values of 0, 1 or 2)", []CCMD{
		if (argc < 2)
		{
			return;
		}
		int type = atoi(argv[1]);
		type = std::min(std::max((int)Player::Inventory_t::GAMEPAD_DROPDOWN_DISABLE, type), (int)Player::Inventory_t::GAMEPAD_DROPDOWN_COMPACT);
		for (int i = 0; i < MAXPLAYERS; ++i)
		{
			if (inputs.bPlayerUsingKeyboardControl(i))
			{
				players[i]->inventoryUI.useItemDropdownOnGamepad = static_cast<Player::Inventory_t::GamepadDropdownTypes>(type);
			}
		}
		});

	static ConsoleCommand ccmd_loadscripts("/loadscripts", "loads scripts.json", []CCMD{
		ScriptTextParser.readAllScripts();
		messagePlayer(clientnum, MESSAGE_MISC, "reloaded scripts.json");
		});

	static ConsoleCommand ccmd_dumpsigns("/dumpsigns", "dumps signs on level to signs.json", []CCMD{
		ScriptTextParser.writeWorldSignsToFile();
		messagePlayer(clientnum, MESSAGE_MISC, "dumped data/scripts/scripts.json");
		});

	static ConsoleCommand ccmd_addfollower("/addfollower", "adds a follower to party", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}

		if (multiplayer != SINGLE)
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(299));
			return;
		}

		if (players[clientnum]->entity)
		{
			if (Entity* monster = summonMonster(HUMAN, players[clientnum]->entity->x, players[clientnum]->entity->y))
			{
				if (forceFollower(*players[clientnum]->entity, *monster))
				{
					monster->monsterAllyIndex = clientnum;
					monster->flags[USERFLAG2] = true;
				}
			}
		}
		});

	static ConsoleCommand ccmd_addfollower2("/addfollowers", "adds many followers to party", []CCMD{
		if (!(svFlags & SV_FLAG_CHEATS))
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}

		if (multiplayer != SINGLE)
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(299));
			return;
		}

		if (argc < 2)
		{
			return;
		}
		int setToChoose = atoi(argv[1]);

		if (players[clientnum]->entity)
		{
			std::vector<Monster> set1 = {
				HUMAN,
				RAT,
				GOBLIN,
				SLIME,
				TROLL,
				SPIDER,
				GHOUL,
				SKELETON
			};
			std::vector<Monster> set2 = {
				SCORPION,
				CREATURE_IMP,
				GNOME,
				DEMON,
				SUCCUBUS,
				KOBOLD,
				SCARAB,
				CRYSTALGOLEM
			};
			std::vector<Monster> set3 = {
				INCUBUS,
				VAMPIRE,
				SHADOW,
				COCKATRICE,
				INSECTOID,
				GOATMAN,
				AUTOMATON,
				BUGBEAR
			};
			std::vector<Monster>* set = nullptr;
			if (setToChoose == 1)
			{
				set = &set1;
			}
			else if (setToChoose == 2)
			{
				set = &set2;
			}
			else if (setToChoose == 3)
			{
				set = &set3;
			}
			else
			{
				return;
			}
			for (auto type : *set)
			{
				if (Entity* monster = summonMonster(type, players[clientnum]->entity->x, players[clientnum]->entity->y))
				{
					if (forceFollower(*players[clientnum]->entity, *monster))
					{
						monster->monsterAllyIndex = clientnum;
						monster->flags[USERFLAG2] = true;
					}
				}
			}
		}
		});

	static ConsoleCommand ccmd_loadmonsterdata("/loadmonsterdata", "", []CCMD{
		MonsterData_t::loadMonsterDataJSON();
		});

	static ConsoleCommand ccmd_itemlevelcurve("/itemlevelcurve", "generate item level curve drop", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}

		if ( argc < 2 )
		{
			return;
		}

		int cat = atoi(argv[1]);
		cat = std::min(std::max(0, cat), NUMCATEGORIES - 1);
		ItemType type = itemLevelCurve((Category)cat, 0, currentlevel, local_rng);
		dropItem(newItem(type, EXCELLENT, 0, 1, local_rng.rand(), true, &stats[clientnum]->inventory), 0);
	});

	static ConsoleCommand ccmd_spawnitem2("/spawnitem2", "spawn an item with beatitude and status (/spawnitem -2 5 wooden shield) (cheat)", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}

		if ( argc < 4 )
		{
			return;
		}

		int beatitude = atoi(argv[1]);
		beatitude = std::max(std::min(99, beatitude), -99);
		int status = atoi(argv[2]);
		status = std::max(std::min(static_cast<int>(EXCELLENT), status), static_cast<int>(BROKEN));

		std::string name = argv[3];
		for ( int arg = 4; arg < argc; ++arg ) {
			name.append(" ");
			name.append(argv[arg]);
		}

		int c;
		for ( c = 0; c < NUMITEMS; c++ )
		{
			if ( strcmp(items[c].getIdentifiedName(), name.c_str()) == 0 )
			{
				dropItem(newItem(static_cast<ItemType>(c), static_cast<Status>(status), beatitude, 1, local_rng.rand(), true, &stats[clientnum]->inventory), 0);
				break;
			}
		}
		if ( c == NUMITEMS )
		{
			for ( c = 0; c < NUMITEMS; c++ )
			{
				if ( strstr(items[c].getIdentifiedName(), name.c_str()) )
				{
					dropItem(newItem(static_cast<ItemType>(c), static_cast<Status>(status), beatitude, 1, local_rng.rand(), true, &stats[clientnum]->inventory), 0);
					break;
				}
			}
		}
		if ( c == NUMITEMS )
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(278), name.c_str());
		}
	});

	static ConsoleCommand ccmd_imgui("/devmenu", "", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}

#ifdef USE_IMGUI
		if ( ImGui_t::isInit )
		{
			ImGui_t::queueDeinit = true;
		}
		else
		{
			ImGui_t::queueInit = true;
		}
#endif
	});

	static ConsoleCommand ccmd_loadshopkeeperconsumables("/loadshopconsumables", "", []CCMD{
#ifndef EDITOR
		ShopkeeperConsumables_t::readFromFile();
#endif
	});

	static ConsoleCommand ccmd_writedefaultclasshotbars("/writedefaultclasshotbars", "", []CCMD{
#ifndef NINTENDO
#ifndef EDITOR
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		bool oldIntro = intro;
		intro = true;
		ClassHotbarConfig_t::writeToFile(ClassHotbarConfig_t::HOTBAR_LAYOUT_DEFAULT_CONFIG, ClassHotbarConfig_t::HOTBAR_CONFIG_WRITE);
		intro = oldIntro;
		ClassHotbarConfig_t::init();
#endif
#endif
	});

	static ConsoleCommand ccmd_saveclasshotbar("/saveclasshotbar", "", []CCMD{
#ifndef EDITOR
		ClassHotbarConfig_t::writeToFile(ClassHotbarConfig_t::HOTBAR_LAYOUT_CUSTOM_CONFIG, ClassHotbarConfig_t::HOTBAR_CONFIG_WRITE);
	ClassHotbarConfig_t::init();
#endif
	});

	static ConsoleCommand ccmd_deleteclasshotbar("/deleteclasshotbar", "", []CCMD{
#ifndef EDITOR
		ClassHotbarConfig_t::writeToFile(ClassHotbarConfig_t::HOTBAR_LAYOUT_CUSTOM_CONFIG, ClassHotbarConfig_t::HOTBAR_CONFIG_DELETE);
	ClassHotbarConfig_t::init();
#endif
	});

	static ConsoleCommand ccmd_loadclasshotbars("/loadclasshotbars", "", []CCMD{
#ifndef EDITOR
		ClassHotbarConfig_t::init();
#endif
	});

	static ConsoleCommand ccmd_assignclasshotbars("/assignhotbarslots", "", []CCMD{
#ifndef EDITOR
		ClassHotbarConfig_t::assignHotbarSlots(clientnum);
#endif
	});

	static ConsoleCommand ccmd_maphashcheck("/maphashcheck", "", []CCMD{
#ifndef NINTENDO
		const char* outpath = nullptr;
		if (argc > 1) {
			outpath = argv[1];
		}
		std::map<std::string, int> newMapHashes;
		for (auto f : directoryContents("maps/", false, true))
		{
			map_t m;
			m.tiles = nullptr;
			m.entities = (list_t*)malloc(sizeof(list_t));
			m.entities->first = nullptr;
			m.entities->last = nullptr;
			m.creatures = new list_t;
			m.creatures->first = nullptr;
			m.creatures->last = nullptr;
			m.worldUI = new list_t;
			m.worldUI->first = nullptr;
			m.worldUI->last = nullptr;
			const std::string mapPath = "maps/" + f;
			auto path = PHYSFS_getRealDir(mapPath.c_str());
			if (path)
			{
				int maphash = 0;
				const std::string fullMapPath = path + (PHYSFS_getDirSeparator() + mapPath);
				int result = loadMap(fullMapPath.c_str(), &m, m.entities, m.creatures, &maphash);
				if (result >= 0) {
					(void)verifyMapHash(fullMapPath.c_str(), maphash);
					if (outpath) {
						newMapHashes[f] = maphash;
					}
				}
			}
			if (m.entities) {
				list_FreeAll(m.entities);
				free(m.entities);
			}
			if (m.creatures) {
				list_FreeAll(m.creatures);
				delete m.creatures;
			}
			if (m.worldUI) {
				list_FreeAll(m.worldUI);
				delete m.worldUI;
			}
			if (m.tiles) {
				free(m.tiles);
			}
		}
		if (outpath) {
			char buf[16];
			File* fp = FileIO::open(outpath, "wb");
            if (fp) {
                fp->write("{\n", sizeof(char), 2);
                for (auto& pair : newMapHashes) {
                    fp->write("\t{ \"", sizeof(char), 4);
                    fp->write(pair.first.c_str(), sizeof(char), pair.first.size());
                    fp->write("\", ", sizeof(char), 3);
                    int len = snprintf(buf, sizeof(buf), "%d", pair.second);
                    fp->write(buf, sizeof(char), len);
                    fp->write(" },\n", sizeof(char), 4);
                }
                fp->write("};", sizeof(char), 2);
                FileIO::close(fp);
            }
		}
#endif
	});

	static ConsoleCommand ccmd_mapwirecheck("/mapwirecheck", "", []CCMD{
#ifndef NINTENDO
		for ( auto f : directoryContents(".\\maps\\", false, true) )
		{
			std::string mapPath = "maps/";
			mapPath += f;
			bool foundNumber = std::find_if(f.begin(), f.end(), ::isdigit) != f.end();
			if ( foundNumber && PHYSFS_getRealDir(mapPath.c_str()) )
			{
				int maphash = 0;
				std::string fullMapPath = PHYSFS_getRealDir(mapPath.c_str());
				fullMapPath += PHYSFS_getDirSeparator();
				fullMapPath += mapPath;
				loadMap(fullMapPath.c_str(), &map, map.entities, map.creatures, nullptr);
				int gate = 0;
				int invertedGate = 0;
				int gateOnEdge = 0;
				int invertedGateOnEdge = 0;
				for ( node_t* node = map.entities->first; node; node = node->next )
				{
					if ( Entity* entity = (Entity*)node->element )
					{
						if ( entity->sprite == 19 || entity->sprite == 20 )
						{
							++gate;
							if ( entity->x == 0 || entity->y == 0
								|| entity->x == map.width - 1 || entity->y == map.height - 1 )
							{
								++gateOnEdge;
							}
						}
						else if ( entity->sprite == 113 || entity->sprite == 114 )
						{
							++invertedGate;
							if ( entity->x == 0 || entity->y == 0
								|| entity->x == map.width - 1 || entity->y == map.height - 1 )
							{
								++invertedGateOnEdge;
							}
						}
					}
				}
				if ( gate || invertedGate )
				{
					char buf[1024];
					if ( gateOnEdge || invertedGateOnEdge )
					{
						snprintf(buf, sizeof(buf), "[Map Wiring]: File %s | Disable Traps: %d | Found %d gates (%d perimeter) %d inverted gates (%d perimeter)", f.c_str(),
							map.flags[MAP_FLAG_DISABLETRAPS], gate, gateOnEdge, invertedGate, invertedGateOnEdge);
						printlog(buf);
					}
					else
					{
						snprintf(buf, sizeof(buf), "[Map Wiring]: File %s | Disable Traps: %d | Found %d gates (NO perimeter) %d inverted gates (NO perimeter)", f.c_str(),
							map.flags[MAP_FLAG_DISABLETRAPS], gate, invertedGate);
						printlog(buf);
					}
				}
				// will crash the game but will show results of every map load :)
			}
		}
#endif
	});

	static ConsoleCommand ccmd_exportitemlang("/exportitemlang", "", []CCMD{
#ifndef EDITOR
#ifndef NINTENDO
		/*rapidjson::Document d;
		d.SetObject();
		CustomHelpers::addMemberToRoot(d, "version", rapidjson::Value(1));
		CustomHelpers::addMemberToRoot(d, "items", rapidjson::Value(rapidjson::kObjectType));
		for ( int i = 0; i < NUMITEMS; ++i )
		{
			d["item_names"].AddMember(rapidjson::Value(ItemTooltips.tmpItems[i].itemName.c_str(), d.GetAllocator()), rapidjson::Value(rapidjson::kObjectType),
				d.GetAllocator());
			d["item_names"][ItemTooltips.tmpItems[i].itemName.c_str()].AddMember("name_identified", rapidjson::Value(items[i].name_identified, d.GetAllocator()), d.GetAllocator());
			d["item_names"][ItemTooltips.tmpItems[i].itemName.c_str()].AddMember("name_unidentified", rapidjson::Value(items[i].name_unidentified, d.GetAllocator()), d.GetAllocator());
		}
		File* fp = FileIO::open("lang/item_names.json", "wb");
		if ( !fp )
		{
			return;
		}
		rapidjson::StringBuffer os;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(os);
		d.Accept(writer);
		fp->write(os.GetString(), sizeof(char), os.GetSize());
		FileIO::close(fp);*/
#endif
#endif
	});

	static ConsoleCommand ccmd_exportspelllang("/exportspelllang", "", []CCMD{
#ifndef EDITOR
#ifndef NINTENDO
		/*rapidjson::Document d;
		d.SetObject();
		CustomHelpers::addMemberToRoot(d, "version", rapidjson::Value(1));
		CustomHelpers::addMemberToRoot(d, "spells", rapidjson::Value(rapidjson::kObjectType));
		for ( int i = 0; i < NUM_SPELLS; ++i )
		{
			if ( spell_t* spell = getSpellFromID(i) )
			{
				d["spell_names"].AddMember(rapidjson::Value(ItemTooltips.spellItems[i].internalName.c_str(), d.GetAllocator()), rapidjson::Value(rapidjson::kObjectType),
					d.GetAllocator());
				d["spell_names"][ItemTooltips.spellItems[i].internalName.c_str()].AddMember("name", rapidjson::Value(spell->getSpellName(), d.GetAllocator()), d.GetAllocator());
			}
		}
		File* fp = FileIO::open("lang/spell_names.json", "wb");
		if ( !fp )
		{
			return;
		}
		rapidjson::StringBuffer os;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(os);
		d.Accept(writer);
		fp->write(os.GetString(), sizeof(char), os.GetSize());
		FileIO::close(fp);*/
#endif
#endif
	});

	static ConsoleCommand ccmd_spawndummy("/spawndummy", "", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		if ( multiplayer == CLIENT )
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(284));
			return;
		}
		if ( players[clientnum]->entity )
		{
			if ( Entity* monster = summonMonster(DUMMYBOT, players[clientnum]->entity->x, players[clientnum]->entity->y) )
			{
				if ( Stat* stat = monster->getStats() )
				{
					stat->HP = 5000;
					stat->MAXHP = 5000;
					stat->CON = 0;
					stat->LVL = 50;
					stat->monsterForceAllegiance = Stat::MONSTER_FORCE_PLAYER_ENEMY;
					serverUpdateEntityStatFlag(monster, 20);
				}
			}
		}
	});

	static ConsoleCommand ccmd_spawndummyhuman("/spawndummyhuman", "", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		if ( multiplayer == CLIENT )
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(284));
			return;
		}
		if ( players[clientnum]->entity )
		{
			if ( Entity* monster = summonMonster(HUMAN, players[clientnum]->entity->x, players[clientnum]->entity->y) )
			{
				if ( Stat* stat = monster->getStats() )
				{
					stat->HP = 5000;
					stat->MAXHP = 5000;
					stat->CON = 0;
					stat->RANDOM_CON = 0;
					stat->LVL = 50;
					stat->EFFECTS[EFF_STUNNED] = true;
					stat->monsterForceAllegiance = Stat::MONSTER_FORCE_PLAYER_ENEMY;
					serverUpdateEntityStatFlag(monster, 20);
					stat->EDITOR_ITEMS[ITEM_SLOT_HELM] = 0;
					stat->EDITOR_ITEMS[ITEM_SLOT_WEAPON] = 0;
					stat->EDITOR_ITEMS[ITEM_SLOT_SHIELD] = 0;
					stat->EDITOR_ITEMS[ITEM_SLOT_ARMOR] = 0;
					stat->EDITOR_ITEMS[ITEM_SLOT_BOOTS] = 0;
					stat->EDITOR_ITEMS[ITEM_SLOT_RING] = 0;
					stat->EDITOR_ITEMS[ITEM_SLOT_AMULET] = 0;
					stat->EDITOR_ITEMS[ITEM_SLOT_CLOAK] = 0;
					stat->EDITOR_ITEMS[ITEM_SLOT_MASK] = 0;
					stat->EDITOR_ITEMS[ITEM_SLOT_GLOVES] = 0;
				}
			}
		}
	});

	static ConsoleCommand ccmd_mesh_collider_debug("/mesh_collider_debug", "", []CCMD{
		node_t* tmpNode = NULL;
		Entity* tmpEnt = NULL;
		for ( tmpNode = map.entities->first; tmpNode != NULL; tmpNode = tmpNode->next )
		{
			tmpEnt = (Entity*)tmpNode->element;
			if ( tmpEnt->behavior == &actColliderDecoration )
			{
				if ( tmpEnt->colliderHasCollision != 0 )
				{
					messagePlayer(clientnum, MESSAGE_DEBUG, "Collider: %d | z: %4.2f | pos: x: %d y: %d", 
						tmpEnt->sprite, tmpEnt->z, (int)tmpEnt->x / 16, (int)tmpEnt->y / 16);
				}
			}
		}
	});

	static ConsoleCommand ccmd_mesh_collider_verify_and_crash_game("/mesh_collider_verify_and_crash_game", "", []CCMD{
#ifndef NINTENDO
		for ( auto f : directoryContents(".\\maps\\", false, true) )
		{
			std::string mapPath = "maps/";
			mapPath += f;
			bool foundNumber = std::find_if(f.begin(), f.end(), ::isdigit) != f.end();
			if ( /*foundNumber &&*/ PHYSFS_getRealDir(mapPath.c_str()) )
			{
				int maphash = 0;
				std::string fullMapPath = PHYSFS_getRealDir(mapPath.c_str());
				fullMapPath += PHYSFS_getDirSeparator();
				fullMapPath += mapPath;
				loadMap(fullMapPath.c_str(), &map, map.entities, map.creatures, nullptr);
				for ( node_t* node = map.entities->first; node; node = node->next )
				{
					if ( Entity* entity = (Entity*)node->element )
					{
						if ( entity->sprite == 179 )
						{
							int x = (int)(entity->x) / 16;
							int y = (int)(entity->y) / 16;
							if ( entity->colliderDecorationModel == 1203
								|| entity->colliderDecorationModel == 1204 )
							{
								real_t z = entity->z = 7.5 - entity->colliderDecorationHeightOffset * 0.25;
								if ( z > -8.51 && z < -8.49 )
								{
									if ( entity->colliderHasCollision == 0 )
									{
										printlog("[Collider Verify]: x: %d y: %d has no collision in map %s", x, y, f.c_str());
									}
								}
							}
							else if ( entity->colliderDecorationModel == 1197
								|| entity->colliderDecorationModel == 1198 )
							{
								real_t z = entity->z = 7.5 - entity->colliderDecorationHeightOffset * 0.25;
								if ( z > 7.49 || z < 7.51 )
								{
									if ( entity->colliderHasCollision == 0 )
									{
										printlog("[Collider Verify]: x: %d y: %d has no collision in map %s", x, y, f.c_str());
									}
								}
							}
							else if ( entity->colliderDecorationModel > 1206 )
							{
								printlog("[Collider Verify]: x: %d y: %d has wrong mesh: %d in map %s", x, y, entity->colliderDecorationModel, f.c_str());
							}
							if ( entity->colliderHasCollision != 0 && (entity->colliderSizeX == 0 || entity->colliderSizeY == 0) )
							{
								printlog("[Collider Verify]: x: %d y: %d has 0 collision size (x: %d, y: %d), mesh: %d in map %s", 
									x, y, entity->colliderSizeX, entity->colliderSizeY, entity->colliderDecorationModel, f.c_str());
							}
						}
					}
				}
				// will crash the game but will show results of every map load :)
			}
		}
#endif
	});

    static ConsoleCommand ccmd_test_light("/test_light", "spawn a test light (r, g, b)", []CCMD{
        const auto r = argc >= 2 ? strtof(argv[1], nullptr) : 1.f;
        const auto g = argc >= 3 ? strtof(argv[2], nullptr) : 1.f;
        const auto b = argc >= 4 ? strtof(argv[3], nullptr) : 1.f;
        (void)lightSphereShadow(0, cameras[0].x, cameras[0].y, 4, r, g, b, 0.5f);
    });

    static ConsoleCommand ccmd_test_model("/test_model", "spawn an entity using a specific model", []CCMD{
        auto sprite = argc >= 2 ? (int)strtol(argv[1], nullptr, 10) : 1;
        auto entity = newEntity(sprite, 1, map.entities, nullptr);
        entity->flags[PASSABLE] = true;
        entity->x = cameras[0].x * 16;
        entity->y = cameras[0].y * 16;
        entity->z = cameras[0].z;
    });

    static ConsoleCommand ccmd_load_map("/loadmap", "load specified map file", []CCMD{
        if ( !(svFlags & SV_FLAG_CHEATS) )
        {
            messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
            return;
        }
        if ( multiplayer != SINGLE )
        {
            messagePlayer(clientnum, MESSAGE_MISC, "Can only be done in singleplayer.");
            return;
        }
        if (argc < 2) {
            return;
        }
        loadMap(argv[1], &map, map.entities, map.creatures, nullptr);
        numplayers = 0;
        assignActions(&map);
    });

    static ConsoleCommand ccmd_enable_cheats("/enablecheats", "enables cheats", []CCMD{
		if ( gameModeManager.getMode() == GameModeManager_t::GAME_MODE_CUSTOM_RUN
			|| gameModeManager.getMode() == GameModeManager_t::GAME_MODE_CUSTOM_RUN )
		{
#ifdef NDEBUG
			return;
#endif
		}
        if ( multiplayer == CLIENT )
        {
            messagePlayer(clientnum, MESSAGE_MISC, "Can only be done by the server.");
            return;
        }
        svFlags = svFlags | SV_FLAG_CHEATS;
        if (multiplayer == SERVER)
        {
            // update client flags
            strcpy((char*)net_packet->data, "SVFL");
            SDLNet_Write32(svFlags, &net_packet->data[4]);
            net_packet->len = 8;

            for (int c = 1; c < MAXPLAYERS; c++)
            {
                if (client_disconnected[c] || players[c]->isLocalPlayer())
                {
                    continue;
                }
                net_packet->address.host = net_clients[c - 1].host;
                net_packet->address.port = net_clients[c - 1].port;
                sendPacketSafe(net_sock, -1, net_packet, c - 1);
                messagePlayer(c, MESSAGE_MISC, Language::get(276));
            }
        }
    });

    //static ConsoleCommand ccmd_quickstart("/quickstart", "quickly starts a new game (eg /quickstart monk)", []CCMD{
    //    if (multiplayer != SINGLE) {
    //        messagePlayer(clientnum, MESSAGE_MISC, "Can only be done in singleplayer.");
    //        return;
    //    }
    //    
    //    // choose class
    //    const char* classtoquickstart = argc > 1 ? argv[1] : "barbarian";
    //    for (int c = 0; c <= CLASS_MONK; ++c) {
    //        if (!strcmp(classtoquickstart, playerClassLangEntry(c, clientnum))) {
    //            client_classes[clientnum] = c;
    //            break;
    //        }
    //    }

    //    // initialize class
    //    strcpy(stats[clientnum]->name, "Avatar");
    //    stats[clientnum]->playerRace = RACE_HUMAN;
    //    stats[clientnum]->sex = static_cast<sex_t>(local_rng.rand() % 2);
    //    stats[clientnum]->appearance = local_rng.rand() % NUMAPPEARANCES;
    //    stats[clientnum]->clearStats();
    //    initClass(clientnum);

    //    // generate unique game key
    //    local_rng.seedTime();
    //    local_rng.getSeed(&uniqueGameKey, sizeof(uniqueGameKey));
    //    net_rng.seedBytes(&uniqueGameKey, sizeof(uniqueGameKey));
    //    doNewGame(false);
    //    
    //    // this just fixes the command buffer coming up again immediately after doNewGame()
    //    Input::inputs[clientnum].consumeBinary("Chat");
    //});

	static ConsoleCommand ccmd_cast_spell_debug("/cast_spell_debug", "shoot every spell", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}

		if ( multiplayer != SINGLE )
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(299));
			return;
		}
		if ( !players[clientnum]->entity ) { return; }

		for ( int i = SPELL_FORCEBOLT; i < NUM_SPELLS; ++i )
		{
			auto spell = getSpellFromID(i);
			std::string tags;
			for ( auto tag : ItemTooltips.spellItems[i].spellTagsStr )
			{
				tags += tag;
				tags += ' ';
			}
			messagePlayer(clientnum, MESSAGE_DEBUG, "[%s]: Type: %s", ItemTooltips.spellItems[i].internalName.c_str(), tags.c_str());
			castSpell(players[clientnum]->entity->getUID(),
				spell, false, false, false);
		}
	});

	static ConsoleCommand ccmd_load_entity_data("/loadentitydata", "reloads entity_data.json", []CCMD{
		EditorEntityData_t::readFromFile();
	});

	static ConsoleCommand ccmd_load_race_descriptions("/loadracedescriptions", "reloads race_descriptions.json", []CCMD{
		MainMenu::RaceDescriptions::readFromFile();
	});

	static ConsoleCommand ccmd_load_class_descriptions("/loadclassdescriptions", "reloads class_descriptions.json", []CCMD{
		MainMenu::ClassDescriptions::readFromFile();
	});

	static ConsoleCommand ccmd_crosshair("/crosshair", "cycles crosshair type", []CCMD{
		if ( argc >= 2 )
		{
			playerSettings[0].shootmodeCrosshair = (int)strtol(argv[1], nullptr, 10);
		}
		else
		{
			++playerSettings[0].shootmodeCrosshair;
		}
	});

	static ConsoleCommand ccmd_reloadtiles("/reloadtiles", "reloads tile textures", []CCMD{
		generateTileTextures();
	});

	static ConsoleCommand ccmd_spawnghost2("/respawnasghost2", "respawn", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}

		if ( players[clientnum]->entity )
		{
			return;
		}

		players[clientnum]->ghost.respawn();
		});

	static ConsoleCommand ccmd_spawnghost("/respawnasghost", "respawn as a ghost", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		
		if ( players[clientnum]->ghost.my )
		{
			players[clientnum]->ghost.setActive(!players[clientnum]->ghost.isActive());
			return;
		}

		if ( stats[clientnum]->HP > 0 )
		{
			stats[clientnum]->HP = 0;
		}

		if ( players[clientnum]->entity )
		{
			players[clientnum]->ghost.initTeleportLocations(players[clientnum]->entity->x / 16, players[clientnum]->entity->y / 16);
		}
		else
		{
			players[clientnum]->ghost.initTeleportLocations(players[clientnum]->ghost.startRoomX, players[clientnum]->ghost.startRoomY);
		}
		players[clientnum]->ghost.spawnGhost();
	});

	static ConsoleCommand ccmd_reloadequipmentoffsets("/reloadequipmentoffsets", "reloads equipment model offsets", []CCMD{
		EquipmentModelOffsets.readFromFile(monstertypename[stats[clientnum]->type]);
	});

	static ConsoleCommand ccmd_classstatrolls("/classstatrolls", "debug class stats", []CCMD{
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}

		if ( multiplayer != SINGLE )
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(299));
			return;
		}
		if ( !players[clientnum]->entity ) { return; }

		for ( int i = 0; i < NUMCLASSES; ++i )
		{
			stats[clientnum]->STR = 0;
			stats[clientnum]->DEX = 0;
			stats[clientnum]->CON = 0;
			stats[clientnum]->INT = 0;
			stats[clientnum]->PER = 0;
			stats[clientnum]->CHR = 0;
			for ( int lv = 0; lv < 50; ++lv )
			{
				int increasestat[3] = { 0, 0, 0 };
				players[clientnum]->entity->playerStatIncrease(i, increasestat);
				for ( int i = 0; i < 3; i++ )
				{
					switch ( increasestat[i] )
					{
					case STAT_STR:
						stats[clientnum]->STR++;
						break;
					case STAT_DEX:
						stats[clientnum]->DEX++;
						break;
					case STAT_CON:
						stats[clientnum]->CON++;
						break;
					case STAT_INT:
						stats[clientnum]->INT++;
						break;
					case STAT_PER:
						stats[clientnum]->PER++;
						break;
					case STAT_CHR:
						stats[clientnum]->CHR++;
						break;
					default:
						break;
					}
				}
			}
			printlog("%d: %d %d %d %d %d %d",
				i,
				stats[clientnum]->STR,
				stats[clientnum]->DEX,
				stats[clientnum]->CON,
				stats[clientnum]->INT,
				stats[clientnum]->PER,
				stats[clientnum]->CHR);
		}
		stats[clientnum]->STR = 0;
		stats[clientnum]->DEX = 0;
		stats[clientnum]->CON = 0;
		stats[clientnum]->INT = 0;
		stats[clientnum]->PER = 0;
		stats[clientnum]->CHR = 0;
	});

	static ConsoleCommand ccmd_reloadequipmentoffsets_all("/reloadequipmentoffsets_all", "reloads all equipment model offsets", []CCMD{
		for ( int c = 1; c < NUMMONSTERS; ++c )
		{
			EquipmentModelOffsets.readFromFile(monstertypename[c], c);
		}
	});

	static ConsoleCommand ccmd_reloadcompendiumlimbs("/reloadcompendiumlimbs", "reloads compendium entries", []CCMD{
			CompendiumEntries.compendiumObjectLimbs.clear();
			CompendiumEntries.readModelLimbsFromFile("monster");
			CompendiumEntries.readModelLimbsFromFile("world");
			CompendiumEntries.readModelLimbsFromFile("codex");
		});

	static ConsoleCommand ccmd_reloadcompendiummonsters("/reloadcompendiummonsters", "reloads compendium entries", []CCMD{
		CompendiumEntries.readMonstersFromFile();
		CompendiumEntries.readMonstersTranslationsFromFile();
		});

	static ConsoleCommand ccmd_reloadcompendiumworld("/reloadcompendiumworld", "reloads compendium entries", []CCMD{
		CompendiumEntries.readWorldFromFile();
		CompendiumEntries.readWorldTranslationsFromFile();

		});

	static ConsoleCommand ccmd_reloadcompendiumcodex("/reloadcompendiumcodex", "reloads compendium entries", []CCMD{
		CompendiumEntries.readCodexFromFile();
		CompendiumEntries.readCodexTranslationsFromFile();
		});

	static ConsoleCommand ccmd_reloadcompendiumitems("/reloadcompendiumitems", "reloads compendium entries", []CCMD{
		CompendiumEntries.readItemsFromFile();
		CompendiumEntries.readMagicFromFile();
		CompendiumEntries.readItemsTranslationsFromFile();
		CompendiumEntries.readMagicTranslationsFromFile();
		});

	static ConsoleCommand ccmd_reloadcompendiumevents("/reloadcompendiumevents", "reloads compendium entries", []CCMD{
		Compendium_t::Events_t::readEventsFromFile();
		Compendium_t::Events_t::readEventsTranslations();
		});

	static ConsoleCommand ccmd_mapdebugfixedmonsters("/mapdebugfixedmonsters", "prints fixed monster spawns", []CCMD{
	#ifndef NINTENDO
		if ( !(svFlags & SV_FLAG_CHEATS) )
		{
			messagePlayer(clientnum, MESSAGE_MISC, Language::get(277));
			return;
		}
		for ( auto f : directoryContents(".\\maps\\", false, true) )
		{
			std::string mapPath = "maps/";
			mapPath += f;
			bool foundNumber = std::find_if(f.begin(), f.end(), ::isdigit) != f.end();
			if ( /*foundNumber &&*/ PHYSFS_getRealDir(mapPath.c_str()) )
			{
				std::string fullMapPath = PHYSFS_getRealDir(mapPath.c_str());
				fullMapPath += PHYSFS_getDirSeparator();
				fullMapPath += mapPath;

				if ( fullMapPath.find(".txt") != std::string::npos )
				{
					continue;
				}

				loadMap(fullMapPath.c_str(), &map, map.entities, map.creatures, nullptr);
				for ( node_t* node = map.entities->first; node; node = node->next )
				{
					if ( Entity* entity = (Entity*)node->element )
					{
						/*if ( entity->sprite == 119 || entity->sprite == 179 || entity->sprite == 127 )
						{
							if ( map.tiles && map.tiles[1 + ((int)entity->y / 16) * MAPLAYERS + ((int)entity->x / 16) * MAPLAYERS * map.height] )
							{
								printlog("Map [%s] ceiling collider: %d sprite", f.c_str(), entity->sprite);
							}
						}*/

						Monster monsterType = NOTHING;
						switch ( entity->sprite ) {
						case 27: monsterType = HUMAN; break;
						case 30: monsterType = TROLL; break;
						case 35: monsterType = SHOPKEEPER; break;
						case 36: monsterType = GOBLIN; break;
						case 48: monsterType = SPIDER; break;
						case 62: monsterType = LICH; break;
						case 70: monsterType = GNOME; break;
						case 71: monsterType = DEVIL; break;
						case 75: monsterType = DEMON; break;
						case 76: monsterType = CREATURE_IMP; break;
						case 77: monsterType = MINOTAUR; break;
						case 78: monsterType = SCORPION; break;
						case 79: monsterType = SLIME; break;
						case 80: monsterType = SUCCUBUS; break;
						case 81: monsterType = RAT; break;
						case 82: monsterType = GHOUL; break;
						case 83: monsterType = SKELETON; break;
						case 84: monsterType = KOBOLD; break;
						case 85: monsterType = SCARAB; break;
						case 86: monsterType = CRYSTALGOLEM; break;
						case 87: monsterType = INCUBUS; break;
						case 88: monsterType = VAMPIRE; break;
						case 89: monsterType = SHADOW; break;
						case 90: monsterType = COCKATRICE; break;
						case 91: monsterType = INSECTOID; break;
						case 92: monsterType = GOATMAN; break;
						case 93: monsterType = AUTOMATON; break;
						case 94: monsterType = LICH_ICE; break;
						case 95: monsterType = LICH_FIRE; break;
						case 163: monsterType = SENTRYBOT; break;
						case 164: monsterType = SPELLBOT; break;
						case 165: monsterType = DUMMYBOT; break;
						case 166: monsterType = GYROBOT; break;
						case 188: monsterType = BAT_SMALL; break;
						case 189: monsterType = BUGBEAR; break;
						default:
							break;
						}
						if ( monsterType != NOTHING )
						{
							printlog("Map [%s]: Monster: %s", f.c_str(), monstertypename[monsterType]);
						}
					}
				}
				// will crash the game but will show results of every map load :)
			}
		}
#endif
		});
}

