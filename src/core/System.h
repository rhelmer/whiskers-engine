// System.h
// Abstract base class for all systems in the ECS architecture.
#pragma once

class EntityManager;  // forward-declare

class System {
 public:
  virtual ~System() = default;

  // Called once before the main loop.
  virtual void init() {}

  // Called every frame *before* physics (for input, AI, etc.).
  virtual void earlyUpdate(EntityManager& em, float deltaTime) { (void)em; (void)deltaTime; }

  // Called every frame during the physics/update phase.
  virtual void update(EntityManager& em, float deltaTime) { (void)em; (void)deltaTime; }

  // Called after physics, before rendering (for camera, animation, etc.).
  virtual void lateUpdate(EntityManager& em, float deltaTime) { (void)em; (void)deltaTime; }
};
