#include "shader.hpp"

void Shader::init() {
    program = glCreateProgram();
}

void Shader::destroy() {
    if (program) {
        for (auto shader: shaders) {
            if (shader) {
                glDetachShader(program, shader);
                glDeleteShader(shader);
            }
        }
        glDeleteProgram(program);
    }
}

bool Shader::bind() {
    glUseProgram(program);
    return program;
}

void Shader::unbind() {
    glUseProgram(0);
}

GLuint Shader::uniform(const char* name) {
    auto find = uniforms.find(name);
    if (find == uniforms.end()) {
        GLuint handle = glGetUniformLocation(program, (GLchar*)name);
        uniforms.emplace(name, handle);
        return handle;
    } else {
        return find->second;
    }
}

bool Shader::compile(const char* source, size_t len, Shader::Type type) {
    GLenum glType;
    switch (type) {
    default: return false;
    case Shader::Type::Vertex: glType = GL_VERTEX_SHADER; break;
    case Shader::Type::Geometry: glType = GL_GEOMETRY_SHADER; break;
    case Shader::Type::Fragment: glType = GL_FRAGMENT_SHADER; break;
    }

    const GLint glLen = len;
    auto shader = glCreateShader(glType);
    glShaderSource(shader, 1, &source, &glLen);
    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status) {
        glAttachShader(program, shader);
        shaders.push_back(shader);
        printlog("compiled shader successfully");
        return true;
    } else {
        char log[1024];
        glGetShaderInfoLog(shader, (GLint)sizeof(log), nullptr, (GLchar*)log);
        printlog("failed to compile shader: %s", log);
        glDeleteShader(shader);
        return false;
    }
}

bool Shader::link() {
    uniforms.clear();
    glLinkProgram(program);

    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status) {
        printlog("linked shader program successfully");
        return true;
    } else {
        char log[1024];
        glGetProgramInfoLog(program, sizeof(log), nullptr, (GLchar*)log);
        printlog("failed to link shaders: %s", log);
        return false;
    }
}