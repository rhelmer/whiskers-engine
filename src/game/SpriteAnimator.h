// SpriteAnimator.h
// Manages sprite animation playback — updates current frame, handles
// state transitions, and communicates with RenderComponent.
#pragma once

#include <string>
#include <unordered_map>

#include "PixelArtSprite.h"  // defines SpriteAtlas struct
#include "../core/EntityManager.h"
#include "../core/System.h"

// An animation state: name + sprite atlas.
struct AnimState {
  std::string name;
  SpriteAtlas atlas;
  float frameDuration{0.1f};
  bool loop{true};
};

// An entity's animation state machine.
struct EntityAnimator {
  std::unordered_map<std::string, AnimState> states;
  std::string currentState;
  float currentTime{0.0f};
  int currentFrame{0};

  // Transition to a named state. If already in this state, does nothing.
  // If `force` is true, resets to frame 0.
  void play(const std::string& stateName, bool force = false) { // Inline definition
    if (!force && currentState == stateName) return;
    auto it = states.find(stateName);
    if (it == states.end()) return;
    currentState = stateName;
    currentTime = 0.0f;
    currentFrame = 0;
  }

  // Reset all animations.
  void clear() {
    states.clear();
    currentState = "";
    currentTime = 0.0f;
    currentFrame = 0;
  }
};

class SpriteAnimator : public System {
 public:
  void update(EntityManager& em, float deltaTime) override;

  // Register an animation state for an entity.
  void addState(size_t entityIndex, const std::string& name, const AnimState& state);

  // Play a named animation on an entity.
  void play(size_t entityIndex, const std::string& name, bool force = false);

  // Get current frame index for an entity.
  int getCurrentFrame(size_t entityIndex) const;

  // Reset all animators.
  void clear() {
    animators.clear();
  }

  // Remove animator for an entity (stops animation)
  void remove(size_t entityIndex) {
    animators.erase(entityIndex);
  }

 private:
  std::unordered_map<size_t, EntityAnimator> animators;
};
