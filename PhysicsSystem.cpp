#include "PhysicsSystem.h"

#include "EntityManager.h"

void PhysicsSystem::update(EntityManager& em, float dt) {
  auto& entities = em.getEntities();
  const float bound = 1.05f;

  for (Entity& e : entities) {
    e.position += e.velocity * dt;
    e.angle += e.angularVelocity * dt;

    if (e.angle >= 360.0f) e.angle -= 360.0f;
    if (e.angle < 0.0f) e.angle += 360.0f;

    if (e.position.x > bound) e.position.x = -bound;
    if (e.position.x < -bound) e.position.x = bound;
    if (e.position.y > bound) e.position.y = -bound;
    if (e.position.y < -bound) e.position.y = bound;

    // bullet lifetime
    if (e.type == EntityType::Bullet) {
      e.ttl -= dt;
      if (e.ttl <= 0) {
        e.radius = -1;  // mark as dead
      }
    }
  }
}