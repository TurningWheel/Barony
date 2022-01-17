/*-------------------------------------------------------------------------------

	BARONY
	File: consolecommand.hpp
	Desc: console command class

	Copyright 2022 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include <string>
#include <unordered_map>
#include <cstdlib>
#include <cassert>

#include "../main.hpp"
#include "../game.hpp"
#include "../net.hpp"

/*
 * How to define a console command:
 *
 * ConsoleCommand myCmd("/dothing", "put a helpful description here",
 *     [](int argc, const char** argv){
 *     // do something
 *     });
 *
 * They can be defined anywhere.
 */

typedef void (*const ccmd_function)(int argc, const char **argv);

class ConsoleCommand {
public:
    ConsoleCommand(const char* _name, const char* _desc, const ccmd_function _func) :
        name(_name),
        desc(_desc),
        func(_func)
    {
        add_to_map();
    }

    const char* const name;
    const char* const desc;

    void operator()(int argc, const char** argv) {
        (*func)(argc, argv);
    }

private:
    void add_to_map();
    const ccmd_function func;
};

/*
 * ConsoleVariables are defined just like console commands.
 * They automatically define the func to be a setter function, and define a data
 * member which is public. Make SURE your ConsoleVariable is static so its data
 * doesn't fall out of scope!!!
 *
 * ex (1):
 * static ConsoleVariable<std::string> my_string("/my_string", "Hello world");
 *
 * ex (2):
 * static ConsoleVariable<int> my_int("/my_int", 10);
 *
 * Note: due to the way ConsoleCommands tokenize strings, using several
 * successive spaces in a ConsoleVariable<std::string> will result in just one
 * space being recorded.
 *
 * ex:
 * /cvar Hello       World
 *
 * becomes:
 * "cvar" is "Hello World"
 */

template<typename T = std::string>
class ConsoleVariable : ConsoleCommand {
public:
    ConsoleVariable(const char* _name, const T& _default, const char* _desc = ""):
        ConsoleCommand(_name, _desc, &ConsoleVariable::setter)
    {
        add_to_map();
        (*this)(_default);
    }

    void operator()(const T& arg);

    T data;

private:
    static void setter(int argc, const char** argv);
    void add_to_map();
};

template <typename T>
using cvar_map_t = std::unordered_map<std::string, ConsoleVariable<T>&>;

template <typename T>
cvar_map_t<T>& getConsoleVariables()
{
    static cvar_map_t<T> cvar_map;
    return cvar_map;
}

template <typename T>
void ConsoleVariable<T>::add_to_map()
{
    auto& map = getConsoleVariables<T>();
    auto result = map.emplace(name, *this);
    if (result.second == false) {
        printlog("A ConsoleVariable by the name \"%s\" already exists! Aborting\n", name);
        assert(0 && "A ConsoleVariable with a duplicate name was found. Aborting");
        exit(1);
    }
}
