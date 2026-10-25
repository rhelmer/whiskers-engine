// GameLoop.cpp
#include "GameLoop.h"

#include <iostream>

#include "../rendering/Renderer.h"

GameLoop::GameLoop() {}

void GameLoop::addSystem(System* system) {
  systems.push_back(system);
  system->init();
}

void GameLoop::runOneFixedStep(EntityManager& em, float dt) {
  if (onFixedStepStart) {
    onFixedStepStart(em, dt);
  }
  for (auto* sys : systems) {
    sys->earlyUpdate(em, dt);
  }
  for (auto* sys : systems) {
    sys->update(em, dt);
  }
  ++simulationFrame;
}

void GameLoop::runFixedSteps(EntityManager& em, int steps) {
  const float dt = fixedDeltaTime * timeScale;
  for (int i = 0; i < steps; ++i) {
    runOneFixedStep(em, dt);
  }
}

int GameLoop::run(SDL_Window* window, Renderer* renderer, EntityManager& em) {
  Uint32 lastTicks = SDL_GetTicks();
  float accumulator = 0.0f;

  while (running) {
    if (onBeforeFrame) {
      onBeforeFrame(em);
    }

    // --- Event processing ---
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (onEvent) onEvent(event);
      if (event.type == SDL_QUIT) {
        running = false;
        break;
      }
    }
    if (!running) break;

    Uint32 currentTicks = SDL_GetTicks();
    float frameTime = (currentTicks - lastTicks) / 1000.0f;
    lastTicks = currentTicks;

    // Clamp to avoid spiral of death
    if (frameTime > 0.25f) frameTime = 0.25f;

    accumulator += frameTime;
    deltaTime = frameTime;

    // --- Fixed-timestep update ---
    const float simDt = fixedDeltaTime * timeScale;
    while (!paused && accumulator >= fixedDeltaTime) {
      runOneFixedStep(em, simDt);

      accumulator -= fixedDeltaTime;
    }

    // --- Late update (camera, animation) ---
    for (auto* sys : systems) {
      sys->lateUpdate(em, deltaTime);
    }

    // --- Render ---
    if (renderer) {
      renderer->clear();
      renderer->render(em);
      if (onRender) onRender(em);
      if (onAfterSceneRender) onAfterSceneRender(em);
      SDL_GL_SwapWindow(window);
    }
  }

  return 0;
}
