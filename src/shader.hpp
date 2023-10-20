#pragma once

#include <stdint.h>
#include <vector>
#include <unordered_map>
#include <string>

// A shader is composed of multiple sources, compiled and linked into a single program.
// Each source can be a particular type: vertex, geometry, or fragment.
// By composing multiple types and linking them together, you can construct a shader pipeline.
// When you are finished, bind a shader, vertex array (mesh), and some textures, then call glDrawArrays()
// In a nutshell, this is how you draw graphics in modern OpenGL 4.6

class Shader {
public:
    Shader() = default;
    Shader(Shader&&) = default;
    Shader(const Shader&) = default;
    ~Shader() = default;
    
    void init(const char* name); // initialize shader program
    void destroy(); // delete shader program

    bool bind();                        // bind the shader program
    static void unbind();               // unbind the shader program
    int uniform(const char* name);      // return a handle to a shader variable within the compiled program
    bool isInitialized() const { return program != 0; }

    enum class Type {
        Vertex,
        Geometry,
        Fragment,
    };

    void bindAttribLocation(const char* attribute, int location);
    bool compile(const char* source, size_t len, Type type);
    bool link();
    
    bool operator==(const Shader& rhs) const {
        return program == rhs.program;
    }

private:
    const char* name = "untitled";
    std::vector<uint32_t> shaders;
    std::unordered_map<std::string, int> uniforms;
    uint32_t program = 0;
};
