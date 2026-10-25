// PhysicsSystem.h
// Gravity, integration, and collision resolution for the platformer.
#pragma once

#include <glm/glm.hpp>

#include "../core/EntityManager.h"
#include "../core/System.h"

class PhysicsSystem : public System {
 public:
  PhysicsSystem();

  void update(EntityManager& em, float deltaTime) override;

  // Configure global gravity.
  void setGravity(glm::vec2 g) { gravity = g; }
  glm::vec2 getGravity() const { return gravity; }

 private:
  glm::vec2 gravity{0.0f, -25.0f};

  // Semi-implicit Euler integration for a single entity.
  void integrate(EntityRecord& e, float dt);

  // AABB collision detection and resolution between two entities.
  void resolveCollision(EntityRecord& a, EntityManager& emA, size_t idxA,
                        EntityRecord& b, EntityManager& emB, size_t idxB);

  // Check if two entities' colliders overlap in world space.
  bool aabbTest(const EntityRecord& a, const EntityRecord& b) const;

  // Compute world-space AABB for an entity's collider.
  AABB worldAABB(const EntityRecord& e) const;
};
