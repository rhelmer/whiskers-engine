#include "PhysicsSystem.h"

#include "EntityManager.h"

void PhysicsSystem::update(EntityManager &em, float deltaTime) {
  // Update the ship
  Entity &ship = em.getShip();
  ship.position += ship.velocity * deltaTime;

  ship.angle += ship.angularVelocity * deltaTime;
  if (ship.angle >= 360.0f)
    ship.angle -= 360.0f;
  else if (ship.angle < 0.0f)
    ship.angle += 360.0f;

  // Update other entities (if any)
  for (Entity &e : em.getEntities()) {
    e.position += e.velocity * deltaTime;

    e.angle += e.angularVelocity * deltaTime;
    if (e.angle >= 360.0f)
      e.angle -= 360.0f;
    else if (e.angle < 0.0f)
      e.angle += 360.0f;
  }
}
