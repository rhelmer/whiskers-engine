#include <catch2/catch_test_macros.hpp>

#include "core/EntityManager.h"
#include "physics/PhysicsSystem.h"

TEST_CASE("Gravity integration applies velocity") {
  EntityManager em;
  PhysicsSystem physics;

  auto h = em.createEntity();
  auto& e = em.get(h.index);
  e.hasPhysics = true;
  e.physics.affectedByGravity = true;
  e.physics.gravityScale = 1.0f;

  physics.update(em, 0.01f);

  CHECK(e.physics.velocity.y < -0.2f);
  CHECK(e.physics.velocity.y > -0.3f);
}

TEST_CASE("Static platform does not move under gravity") {
  EntityManager em;
  PhysicsSystem physics;

  auto h = em.createEntity();
  auto& e = em.get(h.index);
  e.hasCollider = true;
  e.hasTransform = true;
  e.collider.bounds = AABB::fromCenterAndHalfSize({0, 0}, {5, 0.5f});

  auto posBefore = e.transform.position;
  physics.update(em, 0.1f);

  CHECK(e.transform.position == posBefore);
}
