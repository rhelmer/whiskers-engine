#include "whiskers/rendering/Texture.h"
#include <iostream>

// STB Image will be included through the STB directory from CMake
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace whiskers {

Texture::Texture() : m_texture(0), m_width(0), m_height(0), m_channels(0) {}

Texture::~Texture() {
    if (m_texture != 0) {
        glDeleteTextures(1, &m_texture);
    }
}

bool Texture::loadFromFile(const std::string& filepath) {
    // Load image data
    unsigned char* data = stbi_load(filepath.c_str(), &m_width, &m_height, &m_channels, 0);
    if (!data) {
        std::cerr << "Failed to load texture: " << filepath << std::endl;
        return false;
    }
    
    bool success = loadFromMemory(data, m_width, m_height, m_channels);
    
    // Free the image data
    stbi_image_free(data);
    
    return success;
}

bool Texture::loadFromMemory(const unsigned char* data, int width, int height, int channels) {
    if (!data) {
        std::cerr << "Invalid texture data" << std::endl;
        return false;
    }
    
    m_width = width;
    m_height = height;
    m_channels = channels;
    
    // Generate texture
    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    
    // Set texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    // Set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // Determine format based on number of channels
    GLenum format;
    switch (channels) {
        case 1:
            format = GL_RED;
            break;
        case 3:
            format = GL_RGB;
            break;
        case 4:
            format = GL_RGBA;
            break;
        default:
            std::cerr << "Unsupported texture format with " << channels << " channels" << std::endl;
            glDeleteTextures(1, &m_texture);
            m_texture = 0;
            return false;
    }
    
    // Upload texture data
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    
    // Unbind texture
    glBindTexture(GL_TEXTURE_2D, 0);
    
    return true;
}

void Texture::bind(unsigned int slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_texture);
}

void Texture::unbind() const {
    glBindTexture(GL_TEXTURE_2D, 0);
}

}  // namespace whiskers
