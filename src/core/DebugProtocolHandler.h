// DebugProtocolHandler.h — Execute JSON debug commands (see specs/testing-debug-protocol.md).
#pragma once

#ifdef WHISKERS_DEBUG_PROTOCOL

#include <cstdint>
#include <mutex>
#include <string>

#include <nlohmann/json.hpp>

#include "DebugInputBridge.h"

class EntityManager;
class PhysicsSystem;
class Level;
class Renderer;
class GameLoop;

class DebugProtocolHandler {
 public:
  void setContext(EntityManager* em, PhysicsSystem* physics, Level* level, Renderer* renderer,
                  GameLoop* loop, DebugInputBridge* bridge);

  void processLines(const std::vector<std::string>& lines);
  void onFixedStepBegin(EntityManager& em);
  void onAfterRender(EntityManager& em);

  DebugInputBridge* getBridge() { return bridge_; }

  nlohmann::json buildStatePayload() const;

 private:
  void handleOne(const nlohmann::json& req);
  void writeResponseOk(const nlohmann::json& result);
  void writeResponseError(const std::string& message);
  void writeJsonLine(const nlohmann::json& obj);

  bool spawnEntity(const std::string& type, float x, float y, const nlohmann::json& props,
                   nlohmann::json& outResult);

  EntityManager* em_{nullptr};
  PhysicsSystem* physics_{nullptr};
  Level* level_{nullptr};
  Renderer* renderer_{nullptr};
  GameLoop* loop_{nullptr};
  DebugInputBridge* bridge_{nullptr};

  bool screenshotPending_{false};
  int screenshotW_{640};
  int screenshotH_{480};

  std::mutex out_mutex_;
};

#endif  // WHISKERS_DEBUG_PROTOCOL
