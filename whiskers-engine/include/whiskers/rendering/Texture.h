#pragma once
#include <string>
#include <glad/glad.h>

namespace whiskers {

class Texture {
public:
    Texture();
    ~Texture();

    bool loadFromFile(const std::string& filepath);
    bool loadFromMemory(const unsigned char* data, int width, int height, int channels);
    
    void bind(unsigned int slot = 0) const;
    void unbind() const;
    
    GLuint getTextureId() const { return m_texture; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    int getChannels() const { return m_channels; }

private:
    GLuint m_texture;
    int m_width;
    int m_height;
    int m_channels;
};

}  // namespace whiskers
