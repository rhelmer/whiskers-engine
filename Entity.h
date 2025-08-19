// Entity.h
#pragma once
#include <glm/glm.hpp>

enum class EntityType { Ship, Asteroid, Bullet };

struct Entity {
  glm::vec2 position{0.0f, 0.0f};
  glm::vec2 velocity{0.0f, 0.0f};
  float angle{0.0f};            // degrees
  float angularVelocity{0.0f};  // degrees per second
  float radius{8.0f};           // for collisions
  EntityType type{EntityType::Asteroid};
  float ttl{-1.0f};  // bullet lifetime, -1 = infinite
};
