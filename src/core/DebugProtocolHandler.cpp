// DebugProtocolHandler.cpp
#ifdef WHISKERS_DEBUG_PROTOCOL

#include "DebugProtocolHandler.h"

#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <nlohmann/json.hpp>

#include "../level/Level.h"
#include "../physics/PhysicsSystem.h"
#include "../rendering/Renderer.h"
#include "Component.h"
#include "EntityManager.h"
#include "GameLoop.h"

namespace {

std::string base64Encode(const std::vector<unsigned char>& data) {
  static const char tbl[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string out;
  out.reserve(((data.size() + 2) / 3) * 4);
  for (size_t i = 0; i < data.size(); i += 3) {
    uint32_t v = static_cast<uint32_t>(data[i]) << 16;
    if (i + 1 < data.size()) v |= static_cast<uint32_t>(data[i + 1]) << 8;
    if (i + 2 < data.size()) v |= static_cast<uint32_t>(data[i + 2]);
    size_t nbytes = std::min<size_t>(3, data.size() - i);
    out += tbl[(v >> 18) & 63];
    out += tbl[(v >> 12) & 63];
    if (nbytes >= 2)
      out += tbl[(v >> 6) & 63];
    else
      out += '=';
    if (nbytes >= 3)
      out += tbl[v & 63];
    else
      out += '=';
  }
  return out;
}

AABB worldAabbFor(const EntityRecord& e) {
  glm::vec2 c(e.transform.position.x, e.transform.position.y);
  return AABB::fromCenterAndHalfSize(c, e.collider.bounds.halfSize());
}

std::string inferType(const EntityRecord& e) {
  if (!e.alive) return "dead";
  if (e.hasInput) return "player";
  if (e.hasEnemyAI) return "enemy_patrol";
  if (e.hasPlatform) return "moving_platform";
  if (e.hasCollider && !e.hasPhysics) return "platform";
  if (e.hasProjectile) return "projectile";
  return "entity";
}

}  // namespace

void DebugProtocolHandler::setContext(EntityManager* em, PhysicsSystem* physics, Level* level,
                                     Renderer* renderer, GameLoop* loop,
                                     DebugInputBridge* bridge) {
  em_ = em;
  physics_ = physics;
  level_ = level;
  renderer_ = renderer;
  loop_ = loop;
  bridge_ = bridge;
}

void DebugProtocolHandler::writeJsonLine(const nlohmann::json& obj) {
  std::lock_guard<std::mutex> lock(out_mutex_);
  std::cout << obj.dump() << '\n';
  std::cout.flush();
}

void DebugProtocolHandler::writeResponseOk(const nlohmann::json& result) {
  writeJsonLine(nlohmann::json{{"status", "ok"}, {"result", result}});
}

void DebugProtocolHandler::writeResponseError(const std::string& message) {
  writeJsonLine(nlohmann::json{{"status", "error"}, {"result", {{"message", message}}}});
}

nlohmann::json DebugProtocolHandler::buildStatePayload() const {
  nlohmann::json ents = nlohmann::json::array();
  if (!em_) return ents;

  const auto& all = em_->getAll();
  for (size_t i = 0; i < all.size(); ++i) {
    const EntityRecord& e = all[i];
    if (!e.alive) continue;

    nlohmann::json ej;
    ej["index"] = i;
    ej["type"] = inferType(e);
    ej["position"] = {{"x", e.transform.position.x}, {"y", e.transform.position.y},
                      {"z", e.transform.position.z}};
    if (e.hasPhysics) {
      ej["velocity"] = {{"x", e.physics.velocity.x}, {"y", e.physics.velocity.y}};
      ej["onGround"] = e.physics.onGround;
    }
    if (e.hasHealth) {
      ej["health"] = {{"current", e.health.current}, {"max", e.health.max}};
    }
    if (e.hasCollider) {
      AABB wa = worldAabbFor(e);
      ej["collider"] = {{"min", {{"x", wa.min.x}, {"y", wa.min.y}}},
                        {"max", {{"x", wa.max.x}, {"y", wa.max.y}}}};
    }
    ents.push_back(std::move(ej));
  }

  return ents;
}

void DebugProtocolHandler::onFixedStepBegin(EntityManager& em) {
  if (bridge_) bridge_->beginFixedStep(em);
}

void DebugProtocolHandler::onAfterRender(EntityManager& /*em*/) {
  if (!screenshotPending_ || !renderer_) return;
  auto png = renderer_->captureFramebufferPng(screenshotW_, screenshotH_);
  screenshotPending_ = false;
  std::string b64 = base64Encode(png);
  writeResponseOk({{"png", b64}});
}

void DebugProtocolHandler::processLines(const std::vector<std::string>& lines) {
  for (const auto& line : lines) {
    if (line.empty()) continue;
    try {
      nlohmann::json req = nlohmann::json::parse(line);
      handleOne(req);
    } catch (const std::exception& ex) {
      writeResponseError(std::string("parse: ") + ex.what());
    }
  }
}

bool DebugProtocolHandler::spawnEntity(const std::string& type, float x, float y,
                                       const nlohmann::json& props,
                                       nlohmann::json& outResult) {
  if (!em_) return false;
  EntityHandle h = em_->createEntity();
  EntityRecord& e = em_->get(h.index);
  e.hasTransform = true;
  e.transform.position = glm::vec3(x, y, 0.0f);
  e.hasRender = true;

  if (type == "player") {
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
  } else if (type == "platform") {
    e.hasCollider = true;
    e.collider.shape = ColliderShape::AABB;
    e.collider.bounds = AABB::fromCenterAndHalfSize(glm::vec2(0), glm::vec2(1.0f, 0.2f));
    e.collider.layer = Layer_Platform;
    e.collider.mask = Layer_None;
    e.collider.isTrigger = false;
    e.render.color = glm::vec3(0.5f, 0.35f, 0.2f);
    e.render.use3DGeometry = true;
  } else if (type == "enemy_patrol") {
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
    if (props.contains("patrolStart")) e.enemyAI.patrolStart = props["patrolStart"].get<float>();
    if (props.contains("patrolEnd")) e.enemyAI.patrolEnd = props["patrolEnd"].get<float>();
    if (props.contains("speed")) e.enemyAI.speed = props["speed"].get<float>();
  } else {
    em_->destroyEntity(h);
    return false;
  }
  outResult["entity"] = static_cast<uint64_t>(h.index);
  return true;
}

void DebugProtocolHandler::handleOne(const nlohmann::json& req) {
  if (!req.contains("cmd") || !req["cmd"].is_string()) {
    writeResponseError("missing cmd");
    return;
  }
  std::string cmd = req["cmd"].get<std::string>();
  nlohmann::json args = req.value("args", nlohmann::json::object());

  if (cmd == "quit") {
    if (loop_) loop_->running = false;
    writeResponseOk({});
    return;
  }

  if (cmd == "get_state") {
    uint64_t f = loop_ ? loop_->getSimulationFrame() : 0;
    float t = loop_ ? static_cast<float>(f) * loop_->getFixedDeltaTime() * loop_->timeScale : 0.f;
    nlohmann::json result;
    result["frame"] = f;
    result["time"] = t;
    result["entities"] = buildStatePayload();
    writeResponseOk(result);
    return;
  }

  if (cmd == "set_input") {
    if (!bridge_) {
      writeResponseError("no bridge");
      return;
    }
    size_t ent = args.value("entity", 0u);
    bridge_->setInput.active = true;
    bridge_->setInput.entity = ent;
    bridge_->setInput.move = args.value("move", 0.0f);
    bridge_->setInput.jump = args.value("jump", false);
    bridge_->setInput.attack = args.value("attack", false);
    bridge_->inputFrame.active = false;
    writeResponseOk({});
    return;
  }

  if (cmd == "input_frame") {
    if (!bridge_) {
      writeResponseError("no bridge");
      return;
    }
    int frames = args.value("frames", 1);
    bridge_->setInput.active = false;
    bridge_->inputFrame.active = true;
    bridge_->inputFrame.framesLeft = std::max(1, frames);
    bridge_->inputFrame.entity = args.value("entity", 0u);
    bridge_->inputFrame.keys.clear();
    if (args.contains("keys") && args["keys"].is_array()) {
      for (const auto& k : args["keys"]) {
        if (k.is_string()) bridge_->inputFrame.keys.push_back(k.get<std::string>());
      }
    }
    bridge_->prevJumpHeld = false;
    writeResponseOk({});
    return;
  }

  if (cmd == "step") {
    if (!em_ || !loop_) {
      writeResponseError("no context");
      return;
    }
    int frames = args.value("frames", 1);
    frames = std::max(1, frames);
    loop_->runFixedSteps(*em_, frames);
    writeResponseOk({{"frame", loop_->getSimulationFrame()}});
    return;
  }

  if (cmd == "load_level") {
    if (!em_ || !level_ || !physics_) {
      writeResponseError("no level context");
      return;
    }
    std::string path = args.value("path", "");
    if (path.empty()) {
      writeResponseError("missing path");
      return;
    }
    em_->clear();
    if (!level_->load(path, *em_)) {
      writeResponseError("load failed");
      return;
    }
    physics_->setGravity(level_->getData().gravity);
    writeResponseOk({{"entities", static_cast<int>(em_->aliveCount())}});
    return;
  }

  if (cmd == "screenshot") {
    screenshotW_ = args.value("width", 640);
    screenshotH_ = args.value("height", 480);
    screenshotPending_ = true;
    return;
  }

  if (cmd == "teleport") {
    if (!em_) {
      writeResponseError("no em");
      return;
    }
    size_t ent = args.value("entity", 0u);
    if (ent >= em_->getAll().size() || !em_->get(ent).alive) {
      writeResponseError("bad entity");
      return;
    }
    EntityRecord& e = em_->get(ent);
    e.transform.position.x = args.value("x", 0.0f);
    e.transform.position.y = args.value("y", 0.0f);
    if (args.contains("z")) e.transform.position.z = args["z"].get<float>();
    writeResponseOk({});
    return;
  }

  if (cmd == "spawn") {
    nlohmann::json res;
    std::string t = args.value("type", "");
    float x = args.value("x", 0.0f);
    float y = args.value("y", 0.0f);
    nlohmann::json props = args.value("properties", nlohmann::json::object());
    if (!spawnEntity(t, x, y, props, res)) {
      writeResponseError("spawn failed");
      return;
    }
    writeResponseOk(res);
    return;
  }

  if (cmd == "destroy") {
    if (!em_) {
      writeResponseError("no em");
      return;
    }
    size_t idx = args.value("entity", 0u);
    if (idx >= em_->getAll().size()) {
      writeResponseError("bad entity");
      return;
    }
    EntityHandle h{static_cast<uint32_t>(idx), em_->get(idx).generation};
    em_->destroyEntity(h);
    writeResponseOk({});
    return;
  }

  if (cmd == "pause") {
    if (loop_) loop_->paused = true;
    writeResponseOk({});
    return;
  }

  if (cmd == "resume") {
    if (loop_) loop_->paused = false;
    writeResponseOk({});
    return;
  }

  if (cmd == "get_physics_debug") {
    if (!em_) {
      writeResponseError("no em");
      return;
    }
    nlohmann::json colliders = nlohmann::json::array();
    nlohmann::json contacts = nlohmann::json::array();
    const auto& all = em_->getAll();
    for (size_t i = 0; i < all.size(); ++i) {
      if (!all[i].alive || !all[i].hasCollider) continue;
      AABB wa = worldAabbFor(all[i]);
      colliders.push_back({{"entity", i},
                           {"min", {{"x", wa.min.x}, {"y", wa.min.y}}},
                           {"max", {{"x", wa.max.x}, {"y", wa.max.y}}}});
    }
    for (size_t i = 0; i < all.size(); ++i) {
      if (!all[i].alive || !all[i].hasCollider) continue;
      for (size_t j = i + 1; j < all.size(); ++j) {
        if (!all[j].alive || !all[j].hasCollider) continue;
        AABB a = worldAabbFor(all[i]);
        AABB b = worldAabbFor(all[j]);
        if (aabbOverlap(a, b)) {
          contacts.push_back(nlohmann::json::array({i, j}));
        }
      }
    }
    writeResponseOk({{"colliders", colliders}, {"contacts", contacts}});
    return;
  }

  if (cmd == "set_gravity") {
    if (!physics_) {
      writeResponseError("no physics");
      return;
    }
    float gx = args.value("x", 0.0f);
    float gy = args.value("y", -25.0f);
    physics_->setGravity({gx, gy});
    writeResponseOk({});
    return;
  }

  if (cmd == "set_speed") {
    if (!loop_) {
      writeResponseError("no loop");
      return;
    }
    loop_->timeScale = args.value("time_scale", 1.0f);
    writeResponseOk({});
    return;
  }

  writeResponseError(std::string("unknown cmd: ") + cmd);
}

#endif  // WHISKERS_DEBUG_PROTOCOL
