/*-------------------------------------------------------------------------------

    BARONY
    File: consolecommand.hpp
    Desc: console command class

    Copyright 2022 (c) Turning Wheel LLC, all rights reserved.
    See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once

#include <string>
#include <map>

const char* FindConsoleCommand(const char* str, int index);

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

typedef void (* const ccmd_function)(int argc, const char** argv);

class ConsoleCommand {
public:
    ConsoleCommand(const char* _name, const char* _desc, const ccmd_function _func);

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
    ConsoleVariable(const char* _name, const T& _default, const char* _desc = "");

    T& operator*() {
        return data;
    }
    T* operator->() {
        return &data;
    }

    T data;

private:
    void set(const char* arg);
    void add_to_map();
    static void setter(int argc, const char** argv);
    using cvar_map_t = std::map<std::string, ConsoleVariable<T>&>;
    static cvar_map_t& getConsoleVariables();
};
