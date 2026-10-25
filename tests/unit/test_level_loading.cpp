#include <catch2/catch_test_macros.hpp>

#include <string>

#include "level/Level.h"
#include "level/Tilemap.h"

static std::string fixturePath(const char* name) {
#ifdef WHISKERS_TEST_DATA_DIR
  return std::string(WHISKERS_TEST_DATA_DIR) + "/" + name;
#else
  return std::string("tests/fixtures/") + name;
#endif
}

TEST_CASE("Level JSON parses correctly") {
  LevelData data = loadLevelFromJson(fixturePath("level_test_basic.json"));

  CHECK(data.name == "Test Level");
  CHECK(data.spawn == glm::vec2{2.0f, 3.0f});
  CHECK(data.gravity == glm::vec2{0.0f, -25.0f});
  CHECK(data.tileSize == 1.0f);
  CHECK(data.layers.size() == 0);
  REQUIRE(data.spawnedEntities.size() >= 3);
}

TEST_CASE("Level spawns entities into EntityManager") {
  EntityManager em;
  Level level;

  REQUIRE(level.load(fixturePath("level_test_basic.json"), em));

  auto playerIndices = em.queryWithInput();
  REQUIRE(playerIndices.size() == 1);

  auto platformIndices = em.queryWithCollider();
  REQUIRE(platformIndices.size() >= 2);
}
