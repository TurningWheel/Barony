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
 * They automatically define the func to be a setter function,
 * And define a data member which is public.
 * Make SURE your ConsoleVariable is static so its data member
 * doesn't fall out of scope!!!
 *
 * ex:
 * static ConsoleVariable my_var("/my_var", "some_value", "a variable players can mess with");
 */

class ConsoleVariable : ConsoleCommand {
public:
    ConsoleVariable(const char* _name, const char* _default = "", const char* _desc = ""):
        ConsoleCommand(_name, _desc, &ConsoleVariable::setter)
    {
        data = _default;
        add_to_map();
    }

    void operator()(const char* arg) {
        const char* args[2] = {
            name,
            arg,
        };
        setter(2, args);
    }

    std::string data;

private:
    static void setter(int argc, const char** argv);
    void add_to_map();
};
