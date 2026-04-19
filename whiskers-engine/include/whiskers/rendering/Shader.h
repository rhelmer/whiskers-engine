#pragma once
#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>

namespace whiskers {

class Shader {
public:
    Shader();
    ~Shader();

    bool loadFromSource(const std::string& vertexSource, const std::string& fragmentSource);
    bool loadFromFile(const std::string& vertexPath, const std::string& fragmentPath);
    
    void use() const;
    void unuse() const;
    
    // Uniform setters
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setVec2(const std::string& name, const glm::vec2& value) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setVec4(const std::string& name, const glm::vec4& value) const;
    void setMat4(const std::string& name, const glm::mat4& value) const;
    
    GLuint getProgramId() const { return m_program; }

private:
    GLuint compileShader(GLenum type, const std::string& source);
    GLuint linkShaders(GLuint vertexShader, GLuint fragmentShader);
    bool checkCompileErrors(GLuint shader, const std::string& type);
    
    GLuint m_program;
};

}  // namespace whiskers
