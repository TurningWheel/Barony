#include "shader.hpp"
#include "main.hpp"

void Shader::init(const char* name) {
    if (isInitialized()) {
        return;
    }
    this->name = name;
    program = GL_CHECK_ERR_RET(glCreateProgram());
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
                GL_CHECK_ERR(glDetachShader(program, shader));
                GL_CHECK_ERR(glDeleteShader(shader));
            }
        }
        GL_CHECK_ERR(glDeleteProgram(program));
        uniforms.clear();
        shaders.clear();
        program = 0;
    }
}

static unsigned int currentActiveShader = 0;

bool Shader::bind() {
    if (currentActiveShader != program) {
        GL_CHECK_ERR(glUseProgram(program));
        currentActiveShader = program;
    }
    return program != 0;
}

void Shader::unbind() {
    if (currentActiveShader) {
        GL_CHECK_ERR(glUseProgram(0));
        currentActiveShader = 0;
    }
}

int Shader::uniform(const char* name) {
    auto find = uniforms.find(name);
    if (find == uniforms.end()) {
        int handle = GL_CHECK_ERR_RET(glGetUniformLocation(program, (GLchar*)name));
        if (handle == -1) {
            printlog("uniform %s not found!", name);
        }
        uniforms.emplace(std::string(name), handle);
        return handle;
    } else {
        return find->second;
    }
}

void Shader::bindAttribLocation(const char* attribute, int location) {
    GL_CHECK_ERR(glBindAttribLocation(program, location, attribute));
}

bool Shader::compile(const char* source, size_t len, Shader::Type type) {
    GLenum glType;
    switch (type) {
    default: return false;
    case Shader::Type::Vertex: glType = GL_VERTEX_SHADER; break;
    case Shader::Type::Geometry: glType = GL_GEOMETRY_SHADER; break;
    case Shader::Type::Fragment: glType = GL_FRAGMENT_SHADER; break;
    }

    // For all versions of OpenGL 3.3 and above, the corresponding
    // GLSL version matches the OpenGL version. So GL 4.1 uses GLSL 4.10.
    // see https://www.khronos.org/opengl/wiki/Core_Language_(GLSL)
    // for more details
    
    const char version[] = "#version 150 core\n";
    const char* sources[2] = {version, source};
    const int lens[2] = {(int)sizeof(version) - 1, (int)len};
    
    auto shader = GL_CHECK_ERR_RET(glCreateShader(glType));
    GL_CHECK_ERR(glShaderSource(shader, 2, sources, lens));
    GL_CHECK_ERR(glCompileShader(shader));

    GLint status;
    GL_CHECK_ERR(glGetShaderiv(shader, GL_COMPILE_STATUS, &status));
    if (status) {
        GL_CHECK_ERR(glAttachShader(program, shader));
        shaders.push_back(shader);
        printlog("compiled shader %d successfully", (int)shaders.size());
        return true;
    } else {
        char log[1024];
        GL_CHECK_ERR(glGetShaderInfoLog(shader, (GLint)sizeof(log), nullptr, (GLchar*)log));
        printlog("failed to compile shader: %s", log);
        GL_CHECK_ERR(glDeleteShader(shader));
        return false;
    }
}

bool Shader::link() {
    uniforms.clear();
    GL_CHECK_ERR(glLinkProgram(program));

    GLint status;
    GL_CHECK_ERR(glGetProgramiv(program, GL_LINK_STATUS, &status));
    if (status) {
        printlog("linked shader program '%s' successfully", name);
        return true;
    } else {
        char log[1024];
        GL_CHECK_ERR(glGetProgramInfoLog(program, sizeof(log), nullptr, (GLchar*)log));
        printlog("failed to link shaders for '%s': %s", name, log);
        return false;
    }
}
