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

  // Create ship
  Entity ship;
  ship.position = {0, 0};
  ship.radius = 16.0f;
  ship.type = EntityType::Ship;
  size_t shipIdx = entityManager.createEntity(ship);

  Uint32 lastTicks = SDL_GetTicks();

  bool running = true;
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) running = false;
      // Fire bullet on key press
      if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE) {
        Entity &ship = entityManager.getEntities()[shipIdx];  // re-fetch
        Entity bullet;
        bullet.type = EntityType::Bullet;
        bullet.radius = 2.0f;
        bullet.ttl = 1.5f;
        float rad = glm::radians(ship.angle + 90.0f);
        glm::vec2 dir = {cos(rad), sin(rad)};
        bullet.position = ship.position + dir * 0.2f;
        bullet.velocity = dir * 2.0f;
        entityManager.createEntity(bullet);  // may reallocate; safe because we won’t reuse refs
      }
    }

    Uint32 currentTicks = SDL_GetTicks();
    float deltaTime = (currentTicks - lastTicks) / 1000.0f;
    lastTicks = currentTicks;

    const Uint8 *state = SDL_GetKeyboardState(NULL);

    const float rotationSpeed = 180.0f;  // degrees per second
    const float thrustPower = 3.0f;      // acceleration units per second²
    const float drag = 0.995f;           // friction factor

    Entity &ship = entityManager.getEntities()[shipIdx];
    if (state[SDL_SCANCODE_A])
      ship.angularVelocity = 180.0f;
    else if (state[SDL_SCANCODE_D])
      ship.angularVelocity = -180.0f;
    else
      ship.angularVelocity = 0.0f;

    bool thrusting = false;
    if (state[SDL_SCANCODE_W]) {
      thrusting = true;
      float rad = glm::radians(ship.angle + 90.0f);
      glm::vec2 accel(cos(rad), sin(rad));
      accel *= thrustPower;
      ship.velocity += accel * deltaTime;
    } else {
      ship.velocity *= pow(drag, deltaTime * 60.0f);
    }

    physicsSystem.update(entityManager, deltaTime);
    // entityManager.clearDestroyed();

    renderer.clear();
    for (auto &e : entityManager.getEntities()) {
      switch (e.type) {
        case EntityType::Ship:
          renderer.renderShip(e, thrusting);
          break;
        case EntityType::Asteroid:
          renderer.renderAsteroid(e);  // TODO
          break;
        case EntityType::Bullet:
          renderer.renderBullet(e);
          break;
      }
    }
    SDL_GL_SwapWindow(window);
  }

  SDL_GL_DeleteContext(context);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
