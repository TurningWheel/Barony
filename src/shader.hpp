#pragma once

#include "main.hpp"

// A shader is composed of multiple sources, compiled and linked into a single program.
// Each source can be a particular type: vertex, geometry, or fragment.
// By composing multiple types and linking them together, you can construct a shader pipeline.
// When you are finished, bind a shader, vertex array (mesh), and some textures, then call glDrawArrays()
// In a nutshell, this is how you draw graphics in modern OpenGL 4.6

class Shader {
public:
    void init();    // initialize shader program
    void destroy(); // delete shader program

    bool bind();                        // bind the shader program
    static void unbind();               // unbind the shader program
    GLuint uniform(const char* name);   // return a handle to a shader variable within the compiled program

    enum class Type {
        Vertex,
        Geometry,
        Fragment,
    };

    bool compile(const char* source, size_t len, Type type);
    bool link();

private:
    std::vector<GLuint> shaders;
    std::unordered_map<std::string, GLuint> uniforms;
    GLuint program = 0;
};