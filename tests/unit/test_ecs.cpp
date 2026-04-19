#include <catch2/catch_test_macros.hpp>

#include "core/EntityManager.h"

TEST_CASE("EntityManager create and query") {
  EntityManager em;
  auto h = em.createEntity();
  REQUIRE(h.index == 0);

  auto& e = em.get(h.index);
  e.hasTransform = true;
  e.hasPhysics = true;

  auto q = em.queryWithPhysics();
  REQUIRE(q.size() == 1);
  REQUIRE(q[0] == 0);
}

TEST_CASE("Destroy entity excludes from queries") {
  EntityManager em;
  auto h = em.createEntity();
  em.get(h.index).hasTransform = true;

  em.destroyEntity(h);
  CHECK(em.aliveCount() == 0);
}
