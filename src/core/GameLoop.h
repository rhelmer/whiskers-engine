// GameLoop.h
// Fixed-timestep game loop with variable-rate rendering.
#pragma once

#include <SDL2/SDL.h>
#include <cstdint>
#include <functional>
#include <vector>

#include "EntityManager.h"
#include "System.h"

class GameLoop {
 public:
  GameLoop();

  // Register systems. They are called in registration order within each phase.
  void addSystem(System* system);

  // Main loop — runs until `running` is set to false or SDL_QUIT.
  int run(SDL_Window* window, class Renderer* renderer, EntityManager& em);

  std::function<void(const SDL_Event&)> onEvent;
  std::function<void(EntityManager&)> onRender;

  // Expose deltaTime for systems that need it.
  float getDeltaTime() const { return deltaTime; }
  float getFixedDeltaTime() const { return fixedDeltaTime; }

  // Simulation frame counter (incremented once per fixed update).
  uint64_t getSimulationFrame() const { return simulationFrame; }

  // When true, fixed-step simulation does not advance (events + render still run if applicable).
  bool paused{false};

  // Scales fixed-step dt (1.0 = normal speed).
  float timeScale{1.0f};

  // Run N fixed physics steps synchronously (for debug protocol / tests).
  void runFixedSteps(EntityManager& em, int steps);

  // Called at the start of each main-loop iteration (e.g. debug protocol drain).
  std::function<void(EntityManager&)> onBeforeFrame;

  // Called at the start of each fixed update (before earlyUpdate).
  std::function<void(EntityManager&, float)> onFixedStepStart;

  // After main scene render + optional overlay (e.g. debug screenshot readback).
  std::function<void(EntityManager&)> onAfterSceneRender;

  // Flag to break the loop from outside.
  bool running{true};

 private:
  void runOneFixedStep(EntityManager& em, float dt);

  std::vector<System*> systems;
  float deltaTime{0.0f};
  const float fixedDeltaTime{1.0f / 120.0f};  // 120 Hz physics
  uint64_t simulationFrame{0};
};
