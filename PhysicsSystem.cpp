#include "PhysicsSystem.h"

#include "EntityManager.h"

void PhysicsSystem::update(EntityManager &em, float deltaTime) {
  Entity &ship = em.getShip();
  ship.position += ship.velocity * deltaTime;
  ship.angle += ship.angularVelocity * deltaTime;

  if (ship.angle >= 360.0f)
    ship.angle -= 360.0f;
  else if (ship.angle < 0.0f)
    ship.angle += 360.0f;

  // --- Screen wrapping ---
  const float bound = 1.05f;  // slightly larger than 1 so it wraps off-screen
  if (ship.position.x > bound) ship.position.x = -bound;
  if (ship.position.x < -bound) ship.position.x = bound;
  if (ship.position.y > bound) ship.position.y = -bound;
  if (ship.position.y < -bound) ship.position.y = bound;

  for (Entity &e : em.getEntities()) {
    e.position += e.velocity * deltaTime;
    e.angle += e.angularVelocity * deltaTime;

    if (e.angle >= 360.0f)
      e.angle -= 360.0f;
    else if (e.angle < 0.0f)
      e.angle += 360.0f;

    if (e.position.x > bound) e.position.x = -bound;
    if (e.position.x < -bound) e.position.x = bound;
    if (e.position.y > bound) e.position.y = -bound;
    if (e.position.y < -bound) e.position.y = bound;
  }
}
