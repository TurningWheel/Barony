#include "shader.hpp"

void Shader::init(const char* name) {
    this->name = name;
    program = glCreateProgram();
    if (program) {
        printlog("initialized shader program '%s' successfully", name);
    } else {
        printlog("failed to initialize shader program '%s'", name);
    }
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

static GLuint currentActiveShader = 0;

bool Shader::bind() {
    if (currentActiveShader != program) {
        glUseProgram(program);
        currentActiveShader = program;
    }
    return program != 0;
}

void Shader::unbind() {
    if (currentActiveShader) {
        glUseProgram(0);
        currentActiveShader = 0;
    }
}

GLint Shader::uniform(const char* name) {
    auto find = uniforms.find(name);
    if (find == uniforms.end()) {
        GLint handle = glGetUniformLocation(program, (GLchar*)name);
        if (handle == -1) {
            printlog("uniform %s not found!", name);
        }
        uniforms.emplace(name, handle);
        return handle;
    } else {
        return find->second;
    }
}

void Shader::bindAttribLocation(const char* attribute, int location) {
    glBindAttribLocation(program, location, attribute);
}

bool Shader::compile(const char* source, size_t len, Shader::Type type) {
    GLenum glType;
    switch (type) {
    default: return false;
    case Shader::Type::Vertex: glType = GL_VERTEX_SHADER; break;
    case Shader::Type::Geometry: glType = GL_GEOMETRY_SHADER; break;
    case Shader::Type::Fragment: glType = GL_FRAGMENT_SHADER; break;
    }

    const GLint glLen = (GLint)len;
    auto shader = glCreateShader(glType);
    glShaderSource(shader, 1, &source, &glLen);
    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status) {
        glAttachShader(program, shader);
        shaders.push_back(shader);
        printlog("compiled shader %d successfully", (int)shaders.size());
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
        printlog("linked shader program '%s' successfully", name);
        return true;
    } else {
        char log[1024];
        glGetProgramInfoLog(program, sizeof(log), nullptr, (GLchar*)log);
        printlog("failed to link shaders for '%s': %s", name, log);
        return false;
    }
}
