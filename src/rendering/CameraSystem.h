// CameraSystem.h
// Follow camera with smoothing and bounds.
#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../core/EntityManager.h"
#include "../core/System.h"

class CameraSystem : public System {
 public:
  void lateUpdate(EntityManager& em, float deltaTime) override;

  // Current camera world-space position (center of view).
  glm::vec2 getCameraPos() const { return cameraPos; }

  // Orthographic half-width (controls zoom).
  float getOrthoHalfWidth() const { return orthoHalfWidth; }
  void setOrthoHalfWidth(float w) { orthoHalfWidth = w; }

  // View matrix (translation only).
  glm::mat4 getViewMatrix() const {
    return glm::translate(glm::mat4(1.0f), glm::vec3(-cameraPos, 0.0f));
  }

  // Projection matrix.
  glm::mat4 getProjectionMatrix(float aspect) const {
    return glm::ortho(
      -orthoHalfWidth, orthoHalfWidth,
      -orthoHalfWidth / aspect, orthoHalfWidth / aspect,
      -10.0f, 10.0f
    );
  }

 private:
  glm::vec2 cameraPos{0.0f, 0.0f};
  glm::vec2 targetPos{0.0f, 0.0f};
  float orthoHalfWidth{10.0f};  // 20 world units visible horizontally
};
