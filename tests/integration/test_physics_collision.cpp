#include <catch2/catch_test_macros.hpp>

#include "core/Component.h"
#include "core/EntityManager.h"
#include "physics/PhysicsSystem.h"

TEST_CASE("Dynamic entity lands on static platform") {
  EntityManager em;
  PhysicsSystem physics;

  auto floor = em.createEntity();
  auto& fe = em.get(floor.index);
  fe.hasTransform = true;
  fe.transform.position = glm::vec3(0, 0, 0);
  fe.hasCollider = true;
  fe.collider.bounds = AABB::fromCenterAndHalfSize({0, 0}, {10.f, 0.2f});
  fe.collider.layer = Layer_Platform;
  fe.collider.mask = Layer_Player;

  auto box = em.createEntity();
  auto& be = em.get(box.index);
  be.hasTransform = true;
  be.transform.position = glm::vec3(0, 2.0f, 0);
  be.hasPhysics = true;
  be.physics.affectedByGravity = true;
  be.physics.gravityScale = 1.0f;
  be.hasCollider = true;
  be.collider.bounds = AABB::fromCenterAndHalfSize({0, 0}, {0.3f, 0.5f});
  be.collider.layer = Layer_Player;
  be.collider.mask = Layer_Platform;

  for (int i = 0; i < 600; ++i) {
    physics.update(em, 1.0f / 120.0f);
  }

  CHECK(be.physics.onGround);
}
