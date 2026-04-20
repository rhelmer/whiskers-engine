// DebugInputBridge.h — Debug protocol input override (read by PlatformerInputSystem).
#pragma once

#ifdef WHISKERS_DEBUG_PROTOCOL

#include <string>
#include <vector>

#include "Component.h"
#include "EntityManager.h"

struct DebugInputBridge {
  struct SetOverride {
    bool active{false};
    size_t entity{0};
    float move{0.f};
    bool jump{false};
    bool attack{false};
  } setInput;

  struct InputFrameJob {
    bool active{false};
    int framesLeft{0};
    size_t entity{0};
    std::vector<std::string> keys;
  } inputFrame;

  bool prevJumpHeld{false};

  void beginFixedStep(EntityManager& em);
  bool shouldSkipSdl(size_t entityIndex) const;
  void applySetOverride(EntityRecord& e);
};

#endif  // WHISKERS_DEBUG_PROTOCOL
