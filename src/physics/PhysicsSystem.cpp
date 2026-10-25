// PhysicsSystem.cpp
#include "PhysicsSystem.h"

#include <algorithm>
#include <cmath>

PhysicsSystem::PhysicsSystem() {}

void PhysicsSystem::update(EntityManager& em, float dt) {
  auto& entities = em.getAll();

  // 1. Integration: apply gravity + velocity to position
  for (auto& e : entities) {
    if (!e.alive) continue;
    if (!e.hasPhysics) continue;

    // Reset ground detection each frame
    e.physics.onGround = false;

    integrate(e, dt);
  }

  // 2. Collision detection and resolution (O(n^2) brute force — fine for Phase 1)
  //    Only entities with hasPhysics move, but colliders without physics (static
  //    platforms) still participate as collision targets.
  for (size_t i = 0; i < entities.size(); ++i) {
    if (!entities[i].alive || !entities[i].hasCollider || !entities[i].hasPhysics) continue;

    for (size_t j = 0; j < entities.size(); ++j) {
      if (i == j) continue;
      if (!entities[j].alive || !entities[j].hasCollider) continue;

      // Check collision layer masks
      if (!(entities[i].collider.layer & entities[j].collider.mask) &&
          !(entities[j].collider.layer & entities[i].collider.mask)) {
        continue;
      }

      if (aabbTest(entities[i], entities[j])) {
        resolveCollision(entities[i], em, i, entities[j], em, j);
      }
    }
  }
}

void PhysicsSystem::integrate(EntityRecord& e, float dt) {
  // Apply gravity
  if (e.physics.affectedByGravity) {
    e.physics.acceleration = gravity * e.physics.gravityScale;
  }

  // Semi-implicit Euler
  e.physics.velocity += e.physics.acceleration * dt;

  // Clamp terminal velocity
  const float maxFallSpeed = 20.0f;
  if (e.physics.velocity.y < -maxFallSpeed) {
    e.physics.velocity.y = -maxFallSpeed;
  }

  e.transform.position += glm::vec3(e.physics.velocity * dt, 0.0f);

  // Reset acceleration
  e.physics.acceleration = glm::vec2(0.0f);
}

bool PhysicsSystem::aabbTest(const EntityRecord& a, const EntityRecord& b) const {
  AABB aabbA = worldAABB(a);
  AABB aabbB = worldAABB(b);
  return aabbOverlap(aabbA, aabbB);
}

AABB PhysicsSystem::worldAABB(const EntityRecord& e) const {
  glm::vec2 center(e.transform.position.x, e.transform.position.y);
  glm::vec2 halfSize = e.collider.bounds.halfSize();
  return AABB::fromCenterAndHalfSize(center, halfSize);
}

void PhysicsSystem::resolveCollision(EntityRecord& a, EntityManager& /*emA*/, size_t /*idxA*/,
                                     EntityRecord& b, EntityManager& /*emB*/, size_t /*idxB*/) {
  AABB aabbA = worldAABB(a);
  AABB aabbB = worldAABB(b);

  // Compute overlap on each axis
  float overlapX = std::min(aabbA.max.x, aabbB.max.x) - std::max(aabbA.min.x, aabbB.min.x);
  float overlapY = std::min(aabbA.max.y, aabbB.max.y) - std::max(aabbA.min.y, aabbB.min.y);

  bool bIsStatic = !b.hasPhysics;  // static platform, wall, etc.

  // Resolve along the axis of minimum overlap
  if (overlapX < overlapY) {
    // Horizontal resolution
    glm::vec2 centerA(a.transform.position.x, a.transform.position.y);
    glm::vec2 centerB(b.transform.position.x, b.transform.position.y);
    float sign = (centerA.x < centerB.x) ? -1.0f : 1.0f;

    if (!a.collider.isTrigger && !b.collider.isTrigger) {
      if (bIsStatic) {
        // Push dynamic entity A out entirely
        a.transform.position.x += sign * overlapX;
      } else {
        // Push both apart equally
        a.transform.position.x += sign * overlapX * 0.5f;
        b.transform.position.x -= sign * overlapX * 0.5f;
      }
    }

    // Zero out horizontal velocity on collision
    if (!a.collider.isTrigger) a.physics.velocity.x = 0.0f;
    if (!bIsStatic && !b.collider.isTrigger) b.physics.velocity.x = 0.0f;
  } else {
    // Vertical resolution
    glm::vec2 centerA(a.transform.position.x, a.transform.position.y);
    glm::vec2 centerB(b.transform.position.x, b.transform.position.y);

    if (!a.collider.isTrigger && !b.collider.isTrigger) {
      if (centerA.y < centerB.y) {
        // A is below B — push A down
        if (bIsStatic) {
          a.transform.position.y -= overlapY;
        } else {
          a.transform.position.y -= overlapY * 0.5f;
          b.transform.position.y += overlapY * 0.5f;
        }
        if (!a.collider.isTrigger) a.physics.velocity.y = 0.0f;
        if (!bIsStatic && !b.collider.isTrigger) b.physics.velocity.y = 0.0f;
      } else {
        // A is above B — push A up (landing)
        if (bIsStatic) {
          a.transform.position.y += overlapY;
        } else {
          a.transform.position.y += overlapY * 0.5f;
          b.transform.position.y -= overlapY * 0.5f;
        }
        if (!a.collider.isTrigger) {
          a.physics.velocity.y = 0.0f;
          a.physics.onGround = true;  // A landed on top of B
        }
        if (!bIsStatic && !b.collider.isTrigger) b.physics.velocity.y = 0.0f;
      }
    }
  }
}
