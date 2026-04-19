// CameraSystem.cpp
#include "CameraSystem.h"

#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

void CameraSystem::lateUpdate(EntityManager& em, float deltaTime) {
  auto cameraIndices = em.queryWithCamera();
  if (cameraIndices.empty()) return;

  // Find the first camera entity and follow its target
  const auto& camEntity = em.get(cameraIndices[0]);
  const CameraComponent& cam = camEntity.camera;

  if (cam.followTarget >= 0 && cam.followTarget < (int)em.getAll().size()) {
    const auto& target = em.get(cam.followTarget);
    targetPos = glm::vec2(target.transform.position) + cam.offset;
  }

  // Smooth interpolation
  float t = 1.0f - std::exp(-cam.smoothSpeed * deltaTime);
  cameraPos = glm::mix(cameraPos, targetPos, t);

  // Apply bounds
  if (cam.bounds.enabled) {
    cameraPos.x = std::clamp(cameraPos.x, cam.bounds.left, cam.bounds.right);
    cameraPos.y = std::clamp(cameraPos.y, cam.bounds.bottom, cam.bounds.top);
  }
}
