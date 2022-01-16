/*-------------------------------------------------------------------------------

	BARONY
	File: consolecommand.hpp
	Desc: console command class

	Copyright 2022 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include <string>
#include <unordered_map>

/*
 * How to define a console command:
 *
 * ConsoleCommand myCmd("/dothing", "put a helpful description here",
 *     [](int argc, const char** argv){
 *
 *     });
 *
 * They can be defined anywhere.
 */

typedef void (*ccmd_function)(int argc, const char **argv);

class ConsoleCommand {
public:
    ConsoleCommand(const char* _name, const char* _desc, const ccmd_function _func) :
        name(_name),
        desc(_desc),
        func(_func)
    {
        add_to_map();
    }
    const char* name;
    const char* desc;
    const ccmd_function func;

    void operator()(int argc, const char** argv) {
        (*func)(argc, argv);
    }

private:
    void add_to_map();
};
