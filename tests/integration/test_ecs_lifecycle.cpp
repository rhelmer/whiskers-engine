#include <catch2/catch_test_macros.hpp>

#include "core/EntityManager.h"

TEST_CASE("Stale handle rejected") {
  EntityManager em;
  auto h = em.createEntity();
  em.get(h.index).hasTransform = true;
  em.destroyEntity(h);

  auto resolved = em.tryGet(h);
  CHECK_FALSE(resolved.has_value());
}
