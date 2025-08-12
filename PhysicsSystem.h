// PhysicsSystem.h
#pragma once
#include "EntityManager.h"
#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp> // for glm::pi

class PhysicsSystem {
public:
  void update(EntityManager &em, float deltaTime);
  bool getThrusting() const { return isThrusting; }

private:
  const float rotationSpeed = 180.0f; // degrees per second
  const float thrustPower = 3.0f;     // acceleration units per second²
  const float drag = 0.8f;            // friction factor
  const float scale = 0.5f;           // ship size scale

  bool isThrusting = false;
};
