#include <catch2/catch_test_macros.hpp>

#include "core/Component.h"

TEST_CASE("AABB overlap detection") {
  AABB a = AABB::fromCenterAndHalfSize({0, 0}, {1, 1});
  AABB b = AABB::fromCenterAndHalfSize({0.5f, 0.5f}, {1, 1});
  AABB c = AABB::fromCenterAndHalfSize({5, 5}, {1, 1});

  REQUIRE(aabbOverlap(a, b));
  REQUIRE_FALSE(aabbOverlap(a, c));
}

TEST_CASE("AABB from center and half-size") {
  AABB box = AABB::fromCenterAndHalfSize({3, 4}, {1, 2});
  CHECK(box.min == glm::vec2{2, 2});
  CHECK(box.max == glm::vec2{4, 6});
  CHECK(box.center() == glm::vec2{3, 4});
  CHECK(box.halfSize() == glm::vec2{1, 2});
}
