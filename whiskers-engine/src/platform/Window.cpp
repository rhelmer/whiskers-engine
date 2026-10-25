#include "whiskers/platform/Window.h"
#include <SDL2/SDL.h>
#include <iostream>
#include <glad/glad.h>

namespace whiskers {

Window::Window() 
    : m_window(nullptr)
    , m_context(nullptr)
    , m_width(0)
    , m_height(0)
    , m_shouldClose(false) 
{
}

Window::~Window() {
    shutdown();
}

bool Window::initialize(int width, int height, const char* title) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // Set OpenGL attributes
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    
    // Create window
    m_window = SDL_CreateWindow(title,
                               SDL_WINDOWPOS_UNDEFINED,
                               SDL_WINDOWPOS_UNDEFINED,
                               width, height,
                               SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    
    if (m_window == nullptr) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return false;
    }
    
    // Create OpenGL context
    m_context = SDL_GL_CreateContext(m_window);
    if (m_context == nullptr) {
        std::cerr << "OpenGL context could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }
    
    // Enable VSync
    SDL_GL_SetSwapInterval(1);
    
    // Set viewport
    glViewport(0, 0, width, height);
    
    m_width = width;
    m_height = height;
    m_shouldClose = false;
    
    std::cout << "Window initialized successfully" << std::endl;
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    
    return true;
}

void Window::swapBuffers() {
    SDL_GL_SwapWindow(m_window);
}

void Window::shutdown() {
    if (m_context) {
        SDL_GL_DeleteContext(m_context);
        m_context = nullptr;
    }
    
    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
    
    SDL_Quit();
}

}  // namespace whiskers
