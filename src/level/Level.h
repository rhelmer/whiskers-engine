// Level.h
// Level loading and management.
#pragma once

#include <string>
#include <vector>

#include "Tilemap.h"
#include "../core/EntityManager.h"

class Level {
 public:
  // Load a level from a JSON file, spawning entities into the EntityManager.
  bool load(const std::string& filepath, EntityManager& em);

  // Unload the current level (destroy all spawned entities).
  void unload(EntityManager& em);

  const LevelData& getData() const { return data; }
  bool isLoaded() const { return loaded; }

  // Get the tilemap layer by name (returns nullptr if not found).
  const TilemapLayer* getLayer(const std::string& name) const;

  // Query the collision layer at a world position.
  bool isSolidAtWorldPos(glm::vec2 worldPos) const;
  TileDef getTileAtWorldPos(glm::vec2 worldPos) const;

  // Convert world coordinates to tile coordinates.
  glm::ivec2 worldToTile(glm::vec2 worldPos) const;
  glm::vec2 tileToWorld(glm::ivec2 tilePos) const;

 private:
  LevelData data;
  bool loaded{false};

  // Track entity handles spawned by this level for unload.
  std::vector<EntityHandle> spawnedHandles;
};
