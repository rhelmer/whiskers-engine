#include <SDL2/SDL.h>
#include <glad/glad.h>

#include <iostream>

#include "EntityManager.h"
#include "PhysicsSystem.h"
#include "Renderer.h"

int main(int argc, char *argv[]) {
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    std::cerr << "SDL_Init error: " << SDL_GetError() << "\n";
    return 1;
  }

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  SDL_Window *window =
      SDL_CreateWindow("SDL2 + OpenGL Rocket with Layered Flame", SDL_WINDOWPOS_CENTERED,
                       SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

  if (!window) {
    std::cerr << "SDL_CreateWindow error: " << SDL_GetError() << "\n";
    SDL_Quit();
    return 1;
  }

  SDL_GLContext context = SDL_GL_CreateContext(window);
  if (!context) {
    std::cerr << "SDL_GL_CreateContext error: " << SDL_GetError() << "\n";
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
    std::cerr << "Failed to initialize GLAD\n";
    return 1;
  }

  int width = 800, height = 600;
  SDL_GetWindowSize(window, &width, &height);

  Renderer renderer(width, height);
  if (!renderer.init()) {
    std::cerr << "Failed to initialize renderer\n";
    return 1;
  }

  EntityManager entityManager;
  PhysicsSystem physicsSystem;

  // Initial ship entity
  Entity shipEntity;
  shipEntity.position = {0.0f, 0.0f};
  shipEntity.velocity = {0.0f, 0.0f};
  shipEntity.angle = 0.0f;
  shipEntity.angularVelocity = 0.0f;
  entityManager.setShip(shipEntity);

  Uint32 lastTicks = SDL_GetTicks();

  bool running = true;
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) running = false;
    }

    Uint32 currentTicks = SDL_GetTicks();
    float deltaTime = (currentTicks - lastTicks) / 1000.0f;
    lastTicks = currentTicks;

    const Uint8 *state = SDL_GetKeyboardState(NULL);
    Entity &ship = entityManager.getShip();

    const float rotationSpeed = 180.0f;  // degrees per second
    const float thrustPower = 3.0f;      // acceleration units per second²
    const float drag = 0.8f;             // friction factor

    // Rotation input
    if (state[SDL_SCANCODE_A]) {
      ship.angularVelocity = rotationSpeed;
    } else if (state[SDL_SCANCODE_D]) {
      ship.angularVelocity = -rotationSpeed;
    } else {
      ship.angularVelocity = 0.0f;
    }

    // Thrust input
    bool thrusting = false;
    if (state[SDL_SCANCODE_W]) {
      thrusting = true;
      float rad = glm::radians(ship.angle + 90.0f);
      glm::vec2 acceleration(cos(rad), sin(rad));
      acceleration *= thrustPower;
      ship.velocity += acceleration * deltaTime;
    } else {
      ship.velocity *= pow(drag, deltaTime * 60.0f);
    }

    physicsSystem.update(entityManager, deltaTime);

    renderer.clear();
    renderer.renderShip(entityManager.getShip(), thrusting);
    SDL_GL_SwapWindow(window);
  }

  SDL_GL_DeleteContext(context);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
