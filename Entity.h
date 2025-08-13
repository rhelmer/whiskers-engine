// Entity.h
#pragma once
#include <glm/glm.hpp>

struct Entity {
  glm::vec2 position;
  glm::vec2 velocity;
  float angle;            // current rotation angle in degrees
  float angularVelocity;  // rotational speed in degrees per second
};
