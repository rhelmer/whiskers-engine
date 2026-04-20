#include "whiskers/core/Application.h"
#include "whiskers/core/EntityManager.h"
#include "whiskers/rendering/Renderer.h"
#include <SDL2/SDL.h>
#include <iostream>
#include <chrono>

namespace whiskers {

class Window {
public:
    Window() : m_window(nullptr), m_width(0), m_height(0) {}
    
    ~Window() {
        if (m_window) {
            SDL_DestroyWindow(m_window);
        }
    }
    
    bool initialize(int width, int height, const char* title) {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
            return false;
        }
        
        // Set OpenGL attributes
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        
        m_window = SDL_CreateWindow(title,
                                   SDL_WINDOWPOS_UNDEFINED,
                                   SDL_WINDOWPOS_UNDEFINED,
                                   width, height,
                                   SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
        
        if (m_window == nullptr) {
            std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
            return false;
        }
        
        m_context = SDL_GL_CreateContext(m_window);
        if (m_context == nullptr) {
            std::cerr << "OpenGL context could not be created! SDL_Error: " << SDL_GetError() << std::endl;
            return false;
        }
        
        m_width = width;
        m_height = height;
        return true;
    }
    
    void swapBuffers() {
        SDL_GL_SwapWindow(m_window);
    }
    
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    
private:
    SDL_Window* m_window;
    SDL_GLContext m_context;
    int m_width, m_height;
};

class Event {
public:
    enum Type {
        Quit,
        KeyPress,
        KeyRelease,
        Unknown
    };
    
    Type type;
    int key;
};

Application::Application() : m_running(false) {}

Application::~Application() = default;

bool Application::initialize(int width, int height, const char* title) {
    m_window = std::make_unique<Window>();
    if (!m_window->initialize(width, height, title)) {
        return false;
    }
    
    m_renderer = std::make_unique<Renderer>(width, height);
    if (!m_renderer->init()) {
        std::cerr << "Failed to initialize renderer" << std::endl;
        return false;
    }
    
    m_entityManager = std::make_unique<EntityManager>();
    
    if (!onInitialize()) {
        return false;
    }
    
    return true;
}

void Application::run() {
    m_running = true;
    
    auto lastTime = std::chrono::high_resolution_clock::now();
    
    while (m_running) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;
        
        processEvents();
        
        if (m_running) {
            onUpdate(deltaTime);
            
            m_renderer->clear();
            onRender();
            m_renderer->present();
            m_window->swapBuffers();
        }
    }
}

void Application::shutdown() {
    onShutdown();
    m_renderer.reset();
    m_window.reset();
    SDL_Quit();
}

void Application::processEvents() {
    SDL_Event sdlEvent;
    while (SDL_PollEvent(&sdlEvent)) {
        Event event;
        event.type = Event::Unknown;
        
        switch (sdlEvent.type) {
            case SDL_QUIT:
                event.type = Event::Quit;
                m_running = false;
                break;
            case SDL_KEYDOWN:
                event.type = Event::KeyPress;
                event.key = sdlEvent.key.keysym.sym;
                break;
            case SDL_KEYUP:
                event.type = Event::KeyRelease;
                event.key = sdlEvent.key.keysym.sym;
                break;
        }
        
        if (event.type != Event::Unknown) {
            onEvent(event);
        }
    }
}

}  // namespace whiskers
