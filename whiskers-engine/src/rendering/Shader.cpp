#include "whiskers/rendering/Shader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

namespace whiskers {

Shader::Shader() : m_program(0) {}

Shader::~Shader() {
    if (m_program != 0) {
        glDeleteProgram(m_program);
    }
}

bool Shader::loadFromSource(const std::string& vertexSource, const std::string& fragmentSource) {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    if (vertexShader == 0) {
        return false;
    }
    
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (fragmentShader == 0) {
        glDeleteShader(vertexShader);
        return false;
    }
    
    m_program = linkShaders(vertexShader, fragmentShader);
    
    // Clean up individual shaders as they're linked into program now
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return m_program != 0;
}

bool Shader::loadFromFile(const std::string& vertexPath, const std::string& fragmentPath) {
    std::string vertexSource, fragmentSource;
    
    // Read vertex shader
    std::ifstream vertexFile(vertexPath);
    if (!vertexFile.is_open()) {
        std::cerr << "Failed to open vertex shader file: " << vertexPath << std::endl;
        return false;
    }
    
    std::stringstream vertexStream;
    vertexStream << vertexFile.rdbuf();
    vertexSource = vertexStream.str();
    vertexFile.close();
    
    // Read fragment shader
    std::ifstream fragmentFile(fragmentPath);
    if (!fragmentFile.is_open()) {
        std::cerr << "Failed to open fragment shader file: " << fragmentPath << std::endl;
        return false;
    }
    
    std::stringstream fragmentStream;
    fragmentStream << fragmentFile.rdbuf();
    fragmentSource = fragmentStream.str();
    fragmentFile.close();
    
    return loadFromSource(vertexSource, fragmentSource);
}

void Shader::use() const {
    glUseProgram(m_program);
}

void Shader::unuse() const {
    glUseProgram(0);
}

void Shader::setInt(const std::string& name, int value) const {
    glUniform1i(glGetUniformLocation(m_program, name.c_str()), value);
}

void Shader::setFloat(const std::string& name, float value) const {
    glUniform1f(glGetUniformLocation(m_program, name.c_str()), value);
}

void Shader::setVec2(const std::string& name, const glm::vec2& value) const {
    glUniform2fv(glGetUniformLocation(m_program, name.c_str()), 1, &value[0]);
}

void Shader::setVec3(const std::string& name, const glm::vec3& value) const {
    glUniform3fv(glGetUniformLocation(m_program, name.c_str()), 1, &value[0]);
}

void Shader::setVec4(const std::string& name, const glm::vec4& value) const {
    glUniform4fv(glGetUniformLocation(m_program, name.c_str()), 1, &value[0]);
}

void Shader::setMat4(const std::string& name, const glm::mat4& value) const {
    glUniformMatrix4fv(glGetUniformLocation(m_program, name.c_str()), 1, GL_FALSE, glm::value_ptr(value));
}

GLuint Shader::compileShader(GLenum type, const std::string& source) {
    GLuint shader = glCreateShader(type);
    const char* sourceCStr = source.c_str();
    glShaderSource(shader, 1, &sourceCStr, nullptr);
    glCompileShader(shader);
    
    if (!checkCompileErrors(shader, type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT")) {
        glDeleteShader(shader);
        return 0;
    }
    
    return shader;
}

GLuint Shader::linkShaders(GLuint vertexShader, GLuint fragmentShader) {
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    
    if (!checkCompileErrors(program, "PROGRAM")) {
        glDeleteProgram(program);
        return 0;
    }
    
    return program;
}

bool Shader::checkCompileErrors(GLuint shader, const std::string& type) {
    GLint success;
    GLchar infoLog[1024];
    
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
            std::cerr << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" 
                      << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            return false;
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, nullptr, infoLog);
            std::cerr << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" 
                      << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            return false;
        }
    }
    
    return true;
}

}  // namespace whiskers
