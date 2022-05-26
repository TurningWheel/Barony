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

struct Vector4 {
    float x;
    float y;
    float z;
    float w;
};

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
        data = _default;
    }

    void operator=(const char* arg);
    T& operator*() {
        return data;
    }
    T* operator->() {
        return &data;
    }

    T data;

private:
    static void setter(int argc, const char** argv);
    void add_to_map();
    using cvar_map_t = std::map<std::string, ConsoleVariable<T>&>;
    static cvar_map_t& getConsoleVariables();
};

// Valid ConsoleVariable types:
extern template class ConsoleVariable<std::string>;
extern template class ConsoleVariable<int>;
extern template class ConsoleVariable<float>;
extern template class ConsoleVariable<bool>;
extern template class ConsoleVariable<Vector4>;
