#pragma once
#include <string>

struct SDL_Window;
typedef void* SDL_GLContext;

namespace whiskers {

class Window {
public:
    Window();
    ~Window();

    bool initialize(int width, int height, const char* title);
    void swapBuffers();
    void shutdown();
    
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    bool shouldClose() const { return m_shouldClose; }
    void setShouldClose(bool shouldClose) { m_shouldClose = shouldClose; }
    
    SDL_Window* getSDLWindow() const { return m_window; }

private:
    SDL_Window* m_window;
    SDL_GLContext m_context;
    int m_width;
    int m_height;
    bool m_shouldClose;
};

}  // namespace whiskers
