// LevelSerializer.cpp
#include "LevelSerializer.h"

#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

#ifdef WHISKERS_WASM_BUILD
#include <emscripten/val.h>
#endif

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Storage helpers
// ---------------------------------------------------------------------------
static void storageSet(const std::string& key, const std::string& value) {
#ifdef WHISKERS_WASM_BUILD
  namespace val = emscripten;
  val::val::global("localStorage").call<void>("setItem", key, value);
#else
  std::string path = "levels/" + key + ".json";
  // Ensure directory exists
  std::ofstream test("levels/.keep", std::ios::app);
  test.close();
  std::ofstream ofs(path);
  if (ofs) { ofs << value; ofs.close(); }
  else { std::cerr << "Failed to save level: " << path << "\n"; }
#endif
}

static std::string storageGet(const std::string& key) {
#ifdef WHISKERS_WASM_BUILD
  namespace val = emscripten;
  val::val item = val::val::global("localStorage").call<val::val>("getItem", "whiskers_level_" + key);
  if (item.isUndefined() || item.isNull()) return "";
  return item.as<std::string>();
#else
  std::string path = "levels/" + key + ".json";
  std::ifstream ifs(path);
  if (!ifs) return "";
  return std::string((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
#endif
}

static std::vector<std::string> storageList() {
  std::vector<std::string> names;
#ifdef WHISKERS_WASM_BUILD
  namespace val = emscripten;
  val::val storage = val::val::global("localStorage");
  int len = storage["length"].as<int>();
  for (int i = 0; i < len; i++) {
    val::val k = storage.call<val::val>("key", i);
    std::string key = k.as<std::string>();
    const std::string prefix = "whiskers_level_";
    if (key.rfind(prefix, 0) == 0) {
      names.push_back(key.substr(prefix.size()));
    }
  }
#else
  // Simple directory scan
  std::string dir = "levels/";
  std::ifstream test(dir + ".keep");
  if (!test) {
    // Create directory
    std::ofstream mk(dir + ".keep");
    mk.close();
  }
  // Use popen to list directory (portable enough for demo)
  FILE* pipe = popen("ls levels/*.json 2>/dev/null", "r");
  if (pipe) {
    char buf[256];
    while (fgets(buf, sizeof(buf), pipe)) {
      std::string line(buf);
      // Strip newline and path
      if (!line.empty() && line.back() == '\n') line.pop_back();
      // Extract filename without path and extension
      size_t slash = line.rfind('/');
      std::string fname = (slash != std::string::npos) ? line.substr(slash + 1) : line;
      size_t dot = fname.rfind('.');
      if (dot != std::string::npos) fname = fname.substr(0, dot);
      if (!fname.empty() && fname != ".keep") names.push_back(fname);
    }
    pclose(pipe);
  }
#endif
  return names;
}

static bool storageRemove(const std::string& key) {
#ifdef WHISKERS_WASM_BUILD
  namespace val = emscripten;
  val::val::global("localStorage").call<void>("removeItem", "whiskers_level_" + key);
  return true;
#else
  std::string path = "levels/" + key + ".json";
  return std::remove(path.c_str()) == 0;
#endif
}

// ---------------------------------------------------------------------------
// Entity type inference
// ---------------------------------------------------------------------------
static std::string inferEntityType(const EntityRecord& e) {
  if (e.hasInput) return "player";
  if (e.hasEnemyAI) return "enemy";
  if (e.hasPlatform) {
    switch (e.platform.moveType) {
      case PlatformComponent::Moving: return "platform_moving";
      case PlatformComponent::Falling: return "platform_falling";
      case PlatformComponent::Conveyor: return "platform_conveyor";
      default: return "platform_static";
    }
  }
  if (e.hasPickup) {
    switch (e.pickup.type) {
      case PickupComponent::Coin: return "pickup_coin";
      case PickupComponent::Heart: return "pickup_heart";
      case PickupComponent::Mana: return "pickup_mana";
      case PickupComponent::Key: return "pickup_key";
      case PickupComponent::Letter: return "pickup_letter";
    }
  }
  if (e.hasProjectile) return "projectile";
  return "custom";
}

// ---------------------------------------------------------------------------
// Serialization
// ---------------------------------------------------------------------------
static json serializeEntity(const EntityRecord& e) {
  json j;
  j["type"] = inferEntityType(e);

  if (e.hasTransform) {
    j["transform"]["pos"] = {e.transform.position.x, e.transform.position.y, e.transform.position.z};
    j["transform"]["rot"] = {e.transform.rotation.x, e.transform.rotation.y, e.transform.rotation.z};
    j["transform"]["scale"] = {e.transform.scale.x, e.transform.scale.y, e.transform.scale.z};
  }

  if (e.hasPhysics) {
    j["physics"]["vel"] = {e.physics.velocity.x, e.physics.velocity.y};
    j["physics"]["gravityScale"] = e.physics.gravityScale;
    j["physics"]["mass"] = e.physics.mass;
    j["physics"]["affectedByGravity"] = e.physics.affectedByGravity;
  }

  if (e.hasCollider) {
    auto hs = e.collider.bounds.halfSize();
    j["collider"]["shape"] = (e.collider.shape == ColliderShape::AABB) ? "AABB" : "Circle";
    j["collider"]["halfSize"] = {hs.x, hs.y};
    j["collider"]["layer"] = (int)e.collider.layer;
    j["collider"]["mask"] = (int)e.collider.mask;
    j["collider"]["isTrigger"] = e.collider.isTrigger;
  }

  if (e.hasRender) {
    j["render"]["color"] = {e.render.color.x, e.render.color.y, e.render.color.z};
    j["render"]["spriteScale"] = e.render.spriteScale;
    j["render"]["usePixelArtSprite"] = e.render.usePixelArtSprite;
    j["render"]["use3DGeometry"] = e.render.use3DGeometry;
    j["render"]["sortOrder"] = e.render.sortOrder;
    j["render"]["flipX"] = e.render.flipX;
    j["render"]["spriteSourceJSON"] = e.render.spriteSourceJSON;
  }

  if (e.hasHealth) {
    j["health"]["current"] = e.health.current;
    j["health"]["max"] = e.health.max;
    j["health"]["mana"] = e.health.mana;
    j["health"]["maxMana"] = e.health.maxMana;
  }

  if (e.hasCamera) {
    j["camera"]["offset"] = {e.camera.offset.x, e.camera.offset.y};
    j["camera"]["smoothSpeed"] = e.camera.smoothSpeed;
    j["camera"]["boundsEnabled"] = e.camera.bounds.enabled;
    if (e.camera.bounds.enabled) {
      j["camera"]["bounds"]["left"] = e.camera.bounds.left;
      j["camera"]["bounds"]["right"] = e.camera.bounds.right;
      j["camera"]["bounds"]["bottom"] = e.camera.bounds.bottom;
      j["camera"]["bounds"]["top"] = e.camera.bounds.top;
    }
  }

  if (e.hasEnemyAI) {
    j["enemyAI"]["patrolStart"] = e.enemyAI.patrolStart;
    j["enemyAI"]["patrolEnd"] = e.enemyAI.patrolEnd;
    j["enemyAI"]["speed"] = e.enemyAI.speed;
    j["enemyAI"]["facingRight"] = e.enemyAI.facingRight;
    j["enemyAI"]["detectionRange"] = e.enemyAI.detectionRange;
  }

  if (e.hasAttack) {
    j["attack"]["type"] = (e.attack.type == AttackComponent::Melee) ? "Melee" : "Ranged";
    j["attack"]["damage"] = e.attack.damage;
    j["attack"]["range"] = e.attack.range;
    j["attack"]["cooldown"] = e.attack.cooldown;
    j["attack"]["duration"] = e.attack.duration;
    j["attack"]["maxCombo"] = e.attack.maxCombo;
  }

  if (e.hasPlatform) {
    j["platform"]["moveType"] = (int)e.platform.moveType;
    j["platform"]["speed"] = e.platform.speed;
    j["platform"]["path"] = json::array();
    for (auto& wp : e.platform.path) {
      j["platform"]["path"].push_back({wp.x, wp.y});
    }
  }

  if (e.hasPickup) {
    j["pickup"]["type"] = (int)e.pickup.type;
    j["pickup"]["value"] = e.pickup.value;
    if (!e.pickup.letterId.empty()) j["pickup"]["letterId"] = e.pickup.letterId;
  }

  if (e.hasMessenger) {
    j["messenger"]["heldLetters"] = e.messenger.heldLetters;
    j["messenger"]["stunCount"] = e.messenger.stunCount;
  }

  if (e.hasLetter) {
    j["letter"]["id"] = e.letter.id;
    j["letter"]["sender"] = e.letter.sender;
    j["letter"]["recipient"] = e.letter.recipient;
    j["letter"]["delivered"] = e.letter.delivered;
  }

  return j;
}

std::string LevelSerializer::serialize(EntityManager& em, const std::string& name) {
  json root;
  root["version"] = 1;
  root["name"] = name;
  root["gravity"] = {0.0, -25.0};
  root["tileSize"] = 1.0;
  root["entities"] = json::array();

  auto allEntities = em.getAll();
  for (size_t i = 0; i < allEntities.size(); i++) {
    if (!allEntities[i].alive) continue;
    root["entities"].push_back(serializeEntity(allEntities[i]));
  }

  return root.dump(2);
}

// ---------------------------------------------------------------------------
// Deserialization
// ---------------------------------------------------------------------------
static void applyTransform(EntityRecord& e, const json& j) {
  if (!j.contains("pos")) return;
  e.transform.position = {j["pos"][0].get<float>(), j["pos"][1].get<float>(), j["pos"][2].get<float>()};
  if (j.contains("rot")) e.transform.rotation = {j["rot"][0].get<float>(), j["rot"][1].get<float>(), j["rot"][2].get<float>()};
  if (j.contains("scale")) e.transform.scale = {j["scale"][0].get<float>(), j["scale"][1].get<float>(), j["scale"][2].get<float>()};
}

static void applyPhysics(EntityRecord& e, const json& j) {
  e.hasPhysics = true;
  if (j.contains("vel")) e.physics.velocity = {j["vel"][0].get<float>(), j["vel"][1].get<float>()};
  if (j.contains("gravityScale")) e.physics.gravityScale = j["gravityScale"].get<float>();
  if (j.contains("mass")) e.physics.mass = j["mass"].get<float>();
  if (j.contains("affectedByGravity")) e.physics.affectedByGravity = j["affectedByGravity"].get<bool>();
}

static void applyCollider(EntityRecord& e, const json& j) {
  e.hasCollider = true;
  std::string shape = j.value("shape", "AABB");
  e.collider.shape = (shape == "Circle") ? ColliderShape::Circle : ColliderShape::AABB;
  if (j.contains("halfSize")) {
    float hx = j["halfSize"][0].get<float>();
    float hy = j["halfSize"][1].get<float>();
    e.collider.bounds = AABB::fromCenterAndHalfSize(glm::vec2(0), glm::vec2(hx, hy));
  }
  if (j.contains("layer")) e.collider.layer = (uint8_t)j["layer"].get<int>();
  if (j.contains("mask")) e.collider.mask = (uint8_t)j["mask"].get<int>();
  if (j.contains("isTrigger")) e.collider.isTrigger = j["isTrigger"].get<bool>();
}

static void applyRender(EntityRecord& e, const json& j) {
  e.hasRender = true;
  if (j.contains("color")) e.render.color = {j["color"][0].get<float>(), j["color"][1].get<float>(), j["color"][2].get<float>()};
  if (j.contains("spriteScale")) e.render.spriteScale = j["spriteScale"].get<float>();
  if (j.contains("usePixelArtSprite")) e.render.usePixelArtSprite = j["usePixelArtSprite"].get<bool>();
  if (j.contains("use3DGeometry")) e.render.use3DGeometry = j["use3DGeometry"].get<bool>();
  if (j.contains("sortOrder")) e.render.sortOrder = j["sortOrder"].get<int>();
  if (j.contains("flipX")) e.render.flipX = j["flipX"].get<bool>();
  if (j.contains("spriteSourceJSON")) e.render.spriteSourceJSON = j["spriteSourceJSON"].get<std::string>();
}

static void applyHealth(EntityRecord& e, const json& j) {
  e.hasHealth = true;
  if (j.contains("current")) e.health.current = j["current"].get<int>();
  if (j.contains("max")) e.health.max = j["max"].get<int>();
  if (j.contains("mana")) e.health.mana = j["mana"].get<int>();
  if (j.contains("maxMana")) e.health.maxMana = j["maxMana"].get<int>();
}

static void applyCamera(EntityRecord& e, const json& j) {
  e.hasCamera = true;
  if (j.contains("offset")) e.camera.offset = {j["offset"][0].get<float>(), j["offset"][1].get<float>()};
  if (j.contains("smoothSpeed")) e.camera.smoothSpeed = j["smoothSpeed"].get<float>();
  if (j.contains("boundsEnabled")) {
    e.camera.bounds.enabled = j["boundsEnabled"].get<bool>();
    if (e.camera.bounds.enabled && j.contains("bounds")) {
      auto& b = j["bounds"];
      e.camera.bounds.left = b.value("left", 0.0f);
      e.camera.bounds.right = b.value("right", 0.0f);
      e.camera.bounds.bottom = b.value("bottom", 0.0f);
      e.camera.bounds.top = b.value("top", 0.0f);
    }
  }
}

static void applyEnemyAI(EntityRecord& e, const json& j) {
  e.hasEnemyAI = true;
  if (j.contains("patrolStart")) e.enemyAI.patrolStart = j["patrolStart"].get<float>();
  if (j.contains("patrolEnd")) e.enemyAI.patrolEnd = j["patrolEnd"].get<float>();
  if (j.contains("speed")) e.enemyAI.speed = j["speed"].get<float>();
  if (j.contains("facingRight")) e.enemyAI.facingRight = j["facingRight"].get<bool>();
  if (j.contains("detectionRange")) e.enemyAI.detectionRange = j["detectionRange"].get<float>();
}

static void applyAttack(EntityRecord& e, const json& j) {
  e.hasAttack = true;
  std::string type = j.value("type", "Melee");
  e.attack.type = (type == "Ranged") ? AttackComponent::Ranged : AttackComponent::Melee;
  if (j.contains("damage")) e.attack.damage = j["damage"].get<float>();
  if (j.contains("range")) e.attack.range = j["range"].get<float>();
  if (j.contains("cooldown")) e.attack.cooldown = j["cooldown"].get<float>();
  if (j.contains("duration")) e.attack.duration = j["duration"].get<float>();
  if (j.contains("maxCombo")) e.attack.maxCombo = j["maxCombo"].get<int>();
}

static void applyPlatform(EntityRecord& e, const json& j) {
  e.hasPlatform = true;
  if (j.contains("moveType")) e.platform.moveType = (PlatformComponent::MoveType)j["moveType"].get<int>();
  if (j.contains("speed")) e.platform.speed = j["speed"].get<float>();
  if (j.contains("path")) {
    e.platform.path.clear();
    for (auto& wp : j["path"]) {
      e.platform.path.push_back({wp[0].get<float>(), wp[1].get<float>()});
    }
  }
}

static void applyPickup(EntityRecord& e, const json& j) {
  e.hasPickup = true;
  if (j.contains("type")) e.pickup.type = (PickupComponent::PickupType)j["type"].get<int>();
  if (j.contains("value")) e.pickup.value = j["value"].get<int>();
  if (j.contains("letterId")) e.pickup.letterId = j["letterId"].get<std::string>();
}

static void applyMessenger(EntityRecord& e, const json& j) {
  e.hasMessenger = true;
  if (j.contains("heldLetters")) e.messenger.heldLetters = j["heldLetters"].get<std::vector<std::string>>();
  if (j.contains("stunCount")) e.messenger.stunCount = j["stunCount"].get<int>();
}

static void applyLetter(EntityRecord& e, const json& j) {
  e.hasLetter = true;
  if (j.contains("id")) e.letter.id = j["id"].get<std::string>();
  if (j.contains("sender")) e.letter.sender = j["sender"].get<std::string>();
  if (j.contains("recipient")) e.letter.recipient = j["recipient"].get<std::string>();
  if (j.contains("delivered")) e.letter.delivered = j["delivered"].get<bool>();
}

bool LevelSerializer::deserialize(const std::string& jsonStr, EntityManager& em) {
  try {
    json root = json::parse(jsonStr);
    em.clear();

    if (!root.contains("entities")) return false;

    for (auto& ej : root["entities"]) {
      auto h = em.createEntity();
      auto& e = em.get(h.index);
      std::string type = ej.value("type", "custom");

      // Apply type-specific defaults
      if (type == "player") {
        e.hasInput = true;
        e.hasCamera = true;
        e.camera.followTarget = (int)h.index;
        e.camera.offset = {0.0f, 2.0f};
        e.camera.smoothSpeed = 8.0f;
      } else if (type == "enemy") {
        e.hasEnemyAI = true;
        e.enemyAI.patrolStart = -2.0f;
        e.enemyAI.patrolEnd = 2.0f;
      } else if (type.rfind("platform", 0) == 0) {
        e.hasPlatform = true;
      } else if (type.rfind("pickup", 0) == 0) {
        e.hasPickup = true;
        e.collider.isTrigger = true;
        e.collider.layer = Layer_Pickup;
        e.collider.mask = Layer_Player;
      }

      // Apply component data from JSON
      if (ej.contains("transform")) applyTransform(e, ej["transform"]);
      if (ej.contains("physics")) applyPhysics(e, ej["physics"]);
      if (ej.contains("collider")) applyCollider(e, ej["collider"]);
      if (ej.contains("render")) applyRender(e, ej["render"]);
      if (ej.contains("health")) applyHealth(e, ej["health"]);
      if (ej.contains("camera")) applyCamera(e, ej["camera"]);
      if (ej.contains("enemyAI")) applyEnemyAI(e, ej["enemyAI"]);
      if (ej.contains("attack")) applyAttack(e, ej["attack"]);
      if (ej.contains("platform")) applyPlatform(e, ej["platform"]);
      if (ej.contains("pickup")) applyPickup(e, ej["pickup"]);
      if (ej.contains("messenger")) applyMessenger(e, ej["messenger"]);
      if (ej.contains("letter")) applyLetter(e, ej["letter"]);
    }

    return true;
  } catch (const std::exception& ex) {
    std::cerr << "LevelSerializer::deserialize error: " << ex.what() << "\n";
    return false;
  }
}

// ---------------------------------------------------------------------------
// CRUD operations
// ---------------------------------------------------------------------------
bool LevelSerializer::save(EntityManager& em, const std::string& name) {
  if (name.empty()) return false;
  std::string json = serialize(em, name);
  storageSet(name, json);
  return true;
}

std::string LevelSerializer::load(const std::string& name) {
  return storageGet(name);
}

std::vector<std::string> LevelSerializer::list() {
  return storageList();
}

bool LevelSerializer::remove(const std::string& name) {
  return storageRemove(name);
}
