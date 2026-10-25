// Tilemap.h
// Tilemap data structure with JSON loading support.
#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <cstdint>

// Tile properties
enum class TileType : uint8_t {
  Empty,
  Solid,       // full collision block
  OneWay,      // collide only from above
  SlopeUp,     // angled surface (/)
  SlopeDown,   // angled surface (\)
  Hazard,      // damages on contact
  Decorative,  // visual only
  Conveyor,    // applies horizontal force
};

struct TileDef {
  TileType type{TileType::Empty};
  int textureIndex{-1};   // index into the tileset texture array
  float conveyorSpeed{0}; // only used for Conveyor tiles
};

struct TilemapLayer {
  std::string name;
  int columns{0};
  int rows{0};
  std::vector<TileDef> tiles;  // row-major: tiles[y * columns + x]

  TileDef getTile(int x, int y) const {
    if (x < 0 || x >= columns || y < 0 || y >= rows) return {};
    return tiles[y * columns + x];
  }

  void setTile(int x, int y, const TileDef& t) {
    if (x < 0 || x >= columns || y < 0 || y >= rows) return;
    tiles[y * columns + x] = t;
  }

  bool hasCollision(int x, int y, bool fromAbove = false) const {
    TileDef t = getTile(x, y);
    switch (t.type) {
      case TileType::Solid: return true;
      case TileType::OneWay: return fromAbove;  // caller must verify direction
      case TileType::SlopeUp:
      case TileType::SlopeDown: return true;
      case TileType::Hazard: return true;
      default: return false;
    }
  }
};

struct ParallaxLayer {
  std::string texture;
  float depth{0.0f};         // parallax factor (0 = static, 1 = full scroll)
  glm::vec3 color{1.0f};     // tint color
  std::vector<std::string> textureFiles;  // for multi-tile backgrounds
};

struct LevelData {
  std::string name;
  glm::vec2 spawn{0.0f, 0.0f};

  // Camera bounds
  struct {
    float left{0}, right{0}, bottom{0}, top{0};
    bool enabled{false};
  } bounds;

  glm::vec2 gravity{0.0f, -25.0f};

  std::vector<ParallaxLayer> parallaxLayers;

  // Multi-layer tilemaps
  std::vector<TilemapLayer> layers;

  float tileSize{1.0f};

  // Entity spawn definitions (parsed by Level manager)
  struct SpawnedEntity {
    std::string type;
    glm::vec2 position{0.0f, 0.0f};
    // Generic key-value properties (parsed per-type)
    std::vector<std::pair<std::string, std::string>> properties;
  };
  std::vector<SpawnedEntity> spawnedEntities;

  std::string music;
};

// Forward-declare the loader (implemented in Level.cpp).
LevelData loadLevelFromJson(const std::string& filepath);
