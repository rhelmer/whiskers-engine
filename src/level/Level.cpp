// Level.cpp
#include "Level.h"

#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

static TileType parseTileType(const std::string& s) {
  if (s == "solid") return TileType::Solid;
  if (s == "oneWay") return TileType::OneWay;
  if (s == "slopeUp") return TileType::SlopeUp;
  if (s == "slopeDown") return TileType::SlopeDown;
  if (s == "hazard") return TileType::Hazard;
  if (s == "decorative") return TileType::Decorative;
  if (s == "conveyor") return TileType::Conveyor;
  return TileType::Empty;
}

LevelData loadLevelFromJson(const std::string& filepath) {
  LevelData data;

  std::ifstream file(filepath);
  if (!file.is_open()) {
    std::cerr << "Failed to open level file: " << filepath << "\n";
    return data;
  }

  try {
    json j;
    file >> j;

    if (j.contains("name")) data.name = j["name"].get<std::string>();

    if (j.contains("spawn")) {
      data.spawn.x = j["spawn"].value("x", 0.0f);
      data.spawn.y = j["spawn"].value("y", 0.0f);
    }

    if (j.contains("bounds")) {
      data.bounds.left = j["bounds"].value("left", 0.0f);
      data.bounds.right = j["bounds"].value("right", 0.0f);
      data.bounds.bottom = j["bounds"].value("bottom", 0.0f);
      data.bounds.top = j["bounds"].value("top", 0.0f);
      data.bounds.enabled = true;
    }

    if (j.contains("gravity")) {
      data.gravity.x = j["gravity"].value("x", 0.0f);
      data.gravity.y = j["gravity"].value("y", -25.0f);
    }

    if (j.contains("tileSize")) data.tileSize = j["tileSize"].get<float>();

    // Parallax layers
    if (j.contains("parallax") && j["parallax"].contains("layers")) {
      for (const auto& pl : j["parallax"]["layers"]) {
        ParallaxLayer layer;
        if (pl.contains("texture")) layer.texture = pl["texture"].get<std::string>();
        if (pl.contains("depth")) layer.depth = pl["depth"].get<float>();
        if (pl.contains("color")) {
          std::string hex = pl["color"].get<std::string>();
          // Parse "#RRGGBB"
          if (hex.size() == 7 && hex[0] == '#') {
            layer.color.r = std::stoi(hex.substr(1, 2), nullptr, 16) / 255.0f;
            layer.color.g = std::stoi(hex.substr(3, 2), nullptr, 16) / 255.0f;
            layer.color.b = std::stoi(hex.substr(5, 2), nullptr, 16) / 255.0f;
          }
        }
        data.parallaxLayers.push_back(layer);
      }
    }

    // Tilemap
    if (j.contains("tilemap")) {
      const auto& tm = j["tilemap"];
      TilemapLayer layer;
      layer.name = "collision";
      layer.columns = tm.value("columns", 0);
      layer.rows = tm.value("rows", 0);
      layer.tiles.resize(layer.columns * layer.rows);

      if (tm.contains("tiles")) {
        const auto& tilesJson = tm["tiles"];
        // Support two formats:
        // 1. Flat array of integers (texture indices), row-major
        // 2. Array of objects with "type", "textureIndex", etc.
        if (tilesJson.is_array() && !tilesJson.empty()) {
          if (tilesJson[0].is_number()) {
            // Flat integer array
            for (int i = 0; i < tilesJson.size() && i < (int)layer.tiles.size(); ++i) {
              int val = tilesJson[i].get<int>();
              if (val > 0) {
                layer.tiles[i].type = TileType::Solid;
                layer.tiles[i].textureIndex = val - 1;  // 0-based
              }
            }
          } else if (tilesJson[0].is_object()) {
            // Object array
            for (int i = 0; i < tilesJson.size() && i < (int)layer.tiles.size(); ++i) {
              const auto& tj = tilesJson[i];
              TileDef def;
              if (tj.contains("type")) def.type = parseTileType(tj["type"].get<std::string>());
              if (tj.contains("textureIndex")) def.textureIndex = tj["textureIndex"].get<int>();
              if (tj.contains("conveyorSpeed")) def.conveyorSpeed = tj["conveyorSpeed"].get<float>();
              layer.tiles[i] = def;
            }
          }
        }
      }

      data.layers.push_back(layer);
    }

    // Spawned entities
    if (j.contains("entities")) {
      for (const auto& ej : j["entities"]) {
        LevelData::SpawnedEntity se;
        if (ej.contains("type")) se.type = ej["type"].get<std::string>();
        if (ej.contains("position")) {
          se.position.x = ej["position"].value("x", 0.0f);
          se.position.y = ej["position"].value("y", 0.0f);
        }
        if (ej.contains("properties") && ej["properties"].is_object()) {
          for (const auto& [key, val] : ej["properties"].items()) {
            std::string s;
            if (val.is_string()) {
              s = val.get<std::string>();
            } else if (val.is_number()) {
              s = std::to_string(val.get<double>());
            } else {
              s = val.dump();
            }
            se.properties.emplace_back(key, s);
          }
        }
        data.spawnedEntities.push_back(se);
      }
    }

    if (j.contains("music")) data.music = j["music"].get<std::string>();

  } catch (const json::exception& e) {
    std::cerr << "JSON parse error in " << filepath << ": " << e.what() << "\n";
  }

  return data;
}

bool Level::load(const std::string& filepath, EntityManager& em) {
  if (loaded) unload(em);

  data = loadLevelFromJson(filepath);
  if (data.name.empty() && data.layers.empty()) {
    std::cerr << "Level appears empty or failed to load: " << filepath << "\n";
    return false;
  }

  loaded = true;
  std::cout << "Loaded level: " << data.name << "\n";

  // Spawn entities defined in the level
  for (const auto& se : data.spawnedEntities) {
    EntityHandle h = em.createEntity();
    auto& e = em.get(h.index);
    e.hasTransform = true;
    e.transform.position = glm::vec3(se.position, 0.0f);
    e.hasRender = true;

    if (se.type == "player") {
      e.hasPhysics = true;
      e.hasCollider = true;
      e.hasInput = true;
      e.hasCamera = true;
      e.collider.shape = ColliderShape::AABB;
      e.collider.bounds = AABB::fromCenterAndHalfSize(glm::vec2(0), glm::vec2(0.3f, 0.5f));
      e.collider.layer = Layer_Player;
      e.collider.mask = Layer_Platform | Layer_Enemy | Layer_Pickup;
      e.render.color = glm::vec3(0.2f, 0.6f, 1.0f);
      e.render.use3DGeometry = false;
      e.physics.gravityScale = 1.0f;
      e.camera.followTarget = -1;  // will be set by camera system
    } else if (se.type == "platform") {
      e.hasCollider = true;
      e.collider.shape = ColliderShape::AABB;
      e.collider.bounds = AABB::fromCenterAndHalfSize(glm::vec2(0), glm::vec2(1.0f, 0.2f));
      e.collider.layer = Layer_Platform;
      e.collider.mask = Layer_None;
      e.collider.isTrigger = false;
      e.render.color = glm::vec3(0.5f, 0.35f, 0.2f);
      e.render.use3DGeometry = true;
    } else if (se.type == "enemy_patrol") {
      e.hasPhysics = true;
      e.hasCollider = true;
      e.hasEnemyAI = true;
      e.collider.shape = ColliderShape::AABB;
      e.collider.bounds = AABB::fromCenterAndHalfSize(glm::vec2(0), glm::vec2(0.3f, 0.4f));
      e.collider.layer = Layer_Enemy;
      e.collider.mask = Layer_Platform | Layer_Player;
      e.render.color = glm::vec3(0.9f, 0.2f, 0.2f);
      e.physics.gravityScale = 1.0f;
      e.physics.affectedByGravity = true;

      e.hasEnemyAI = true;
      for (const auto& [k, v] : se.properties) {
        if (k == "patrolStart") e.enemyAI.patrolStart = std::stof(v);
        else if (k == "patrolEnd") e.enemyAI.patrolEnd = std::stof(v);
        else if (k == "speed") e.enemyAI.speed = std::stof(v);
      }
    } else if (se.type == "collectible_coin") {
      e.hasCollider = true;
      e.collider.shape = ColliderShape::Circle;
      e.collider.bounds = AABB::fromCenterAndHalfSize(glm::vec2(0), glm::vec2(0.2f, 0.2f));
      e.collider.layer = Layer_Pickup;
      e.collider.mask = Layer_Player;
      e.collider.isTrigger = true;
      e.render.color = glm::vec3(1.0f, 0.85f, 0.0f);
    } else if (se.type == "moving_platform") {
      e.hasCollider = true;
      e.hasPlatform = true;
      e.platform.moveType = PlatformComponent::Moving;
      e.collider.shape = ColliderShape::AABB;
      e.collider.bounds = AABB::fromCenterAndHalfSize(glm::vec2(0), glm::vec2(1.0f, 0.2f));
      e.collider.layer = Layer_Platform;
      e.collider.mask = Layer_None;
      e.render.color = glm::vec3(0.4f, 0.3f, 0.5f);
      e.render.use3DGeometry = true;

      // Parse path from properties
      std::vector<glm::vec2> pathPoints;
      for (const auto& [k, v] : se.properties) {
        if (k == "speed") e.platform.speed = std::stof(v);
        else if (k.find("wp") == 0) {  // wp0, wp1, ...
          // format: "x,y"
          size_t comma = v.find(',');
          if (comma != std::string::npos) {
            glm::vec2 wp;
            wp.x = std::stof(v.substr(0, comma));
            wp.y = std::stof(v.substr(comma + 1));
            pathPoints.push_back(wp);
          }
        }
      }
      if (!pathPoints.empty()) {
        e.platform.path = pathPoints;
        e.transform.position = glm::vec3(pathPoints[0], 0.0f);
        e.platform.lastPosition = pathPoints[0];
      }
    }

    spawnedHandles.push_back(h);
  }

  return true;
}

void Level::unload(EntityManager& em) {
  for (auto& h : spawnedHandles) {
    em.destroyEntity(h);
  }
  em.flushDestroyed();
  spawnedHandles.clear();
  loaded = false;
  data = {};
}

const TilemapLayer* Level::getLayer(const std::string& name) const {
  for (const auto& layer : data.layers) {
    if (layer.name == name) return &layer;
  }
  // Default: return the first layer
  if (!data.layers.empty()) return &data.layers[0];
  return nullptr;
}

glm::ivec2 Level::worldToTile(glm::vec2 worldPos) const {
  return glm::ivec2(
    static_cast<int>(std::floor(worldPos.x / data.tileSize)),
    static_cast<int>(std::floor(-worldPos.y / data.tileSize))  // flip Y for tile coords
  );
}

glm::vec2 Level::tileToWorld(glm::ivec2 tilePos) const {
  return glm::vec2(
    (tilePos.x + 0.5f) * data.tileSize,
    -(tilePos.y + 0.5f) * data.tileSize
  );
}

bool Level::isSolidAtWorldPos(glm::vec2 worldPos) const {
  if (!loaded || data.layers.empty()) return false;
  const auto& layer = data.layers[0];  // collision layer
  glm::ivec2 tile = worldToTile(worldPos);
  TileDef t = layer.getTile(tile.x, tile.y);
  return t.type == TileType::Solid || t.type == TileType::OneWay ||
         t.type == TileType::SlopeUp || t.type == TileType::SlopeDown;
}

TileDef Level::getTileAtWorldPos(glm::vec2 worldPos) const {
  if (!loaded || data.layers.empty()) return {};
  const auto& layer = data.layers[0];
  glm::ivec2 tile = worldToTile(worldPos);
  return layer.getTile(tile.x, tile.y);
}
