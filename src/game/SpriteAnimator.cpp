// SpriteAnimator.cpp
#include "SpriteAnimator.h"

// EntityAnimator::play definition should be inline in SpriteAnimator.h
// Removed the duplicate definition from here.

void SpriteAnimator::update(EntityManager& em, float deltaTime) {
  auto renderIndices = em.queryWithRender();
  for (size_t idx : renderIndices) {
    auto& e = em.get(idx);
    if (!e.render.usePixelArtSprite) continue;

    auto it = animators.find(idx);
    if (it == animators.end()) continue;

    auto& animator = it->second;
    if (animator.currentState.empty()) continue;

    auto stateIt = animator.states.find(animator.currentState);
    if (stateIt == animator.states.end()) continue;

    const auto& state = stateIt->second;
    animator.currentTime += deltaTime;

    if (animator.currentTime >= state.frameDuration) {
      animator.currentTime -= state.frameDuration;
      animator.currentFrame++;

      if (animator.currentFrame >= state.atlas.frameCount) {
        if (state.loop) {
          animator.currentFrame = 0;
        } else {
          animator.currentFrame = state.atlas.frameCount - 1;
        }
      }
    }

    // Sync to RenderComponent
    e.render.spriteAtlas = state.atlas.texture;
    e.render.spriteFrameWidth = state.atlas.frameWidth;
    e.render.spriteFrameHeight = state.atlas.frameHeight;
    e.render.spriteFrameCount = state.atlas.frameCount;
    e.render.spriteCurrentFrame = animator.currentFrame;
  }
}

void SpriteAnimator::addState(size_t entityIndex, const std::string& name, const AnimState& state) {
  animators[entityIndex].states[name] = state;
}

void SpriteAnimator::play(size_t entityIndex, const std::string& name, bool force) {
  animators[entityIndex].play(name, force);
}

int SpriteAnimator::getCurrentFrame(size_t entityIndex) const {
  auto it = animators.find(entityIndex);
  if (it == animators.end()) return 0;
  return it->second.currentFrame;
}
