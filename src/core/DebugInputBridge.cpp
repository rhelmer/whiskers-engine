// DebugInputBridge.cpp
#ifdef WHISKERS_DEBUG_PROTOCOL

#include "DebugInputBridge.h"

#include <algorithm>
#include <cmath>

#include "EntityManager.h"

static void applyKeyList(PlayerInputComponent& in, const std::vector<std::string>& keys,
                         bool& prevJump) {
  in.moveAxis = 0.0f;
  bool wantJump = false;
  bool wantAttack = false;
  for (const auto& k : keys) {
    if (k == "A" || k == "a" || k == "Left") in.moveAxis -= 1.0f;
    if (k == "D" || k == "d" || k == "Right") in.moveAxis += 1.0f;
    if (k == "Space" || k == "space" || k == "W" || k == "w" || k == "Up") wantJump = true;
    if (k == "X" || k == "x" || k == "J" || k == "j") wantAttack = true;
  }
  in.moveAxis = std::max(-1.0f, std::min(1.0f, in.moveAxis));
  in.jumpHeld = wantJump;
  in.jumpPressed = wantJump && !prevJump;
  in.attackPressed = wantAttack;
  prevJump = wantJump;
}

void DebugInputBridge::beginFixedStep(EntityManager& em) {
  if (!inputFrame.active) return;
  if (inputFrame.framesLeft <= 0) {
    inputFrame.active = false;
    return;
  }

  auto indices = em.queryWithInput();
  if (indices.empty()) return;
  size_t idx = inputFrame.entity;
  if (std::find(indices.begin(), indices.end(), idx) == indices.end()) {
    idx = indices[0];
    inputFrame.entity = idx;
  }

  EntityRecord& e = em.get(idx);
  if (!e.hasInput) return;

  applyKeyList(e.input, inputFrame.keys, prevJumpHeld);
  --inputFrame.framesLeft;
  if (inputFrame.framesLeft <= 0) {
    inputFrame.active = false;
    e.input.moveAxis = 0.0f;
    e.input.jumpHeld = false;
    e.input.jumpPressed = false;
  }
}

bool DebugInputBridge::shouldSkipSdl(size_t entityIndex) const {
  if (inputFrame.active && entityIndex == inputFrame.entity) return true;
  if (setInput.active && entityIndex == setInput.entity) return true;
  return false;
}

void DebugInputBridge::applySetOverride(EntityRecord& e) {
  if (!setInput.active || inputFrame.active) return;
  e.input.moveAxis = setInput.move;
  e.input.jumpHeld = setInput.jump;
  e.input.jumpPressed = setInput.jump && !prevJumpHeld;
  e.input.attackPressed = setInput.attack;
  prevJumpHeld = setInput.jump;
}

#endif  // WHISKERS_DEBUG_PROTOCOL
