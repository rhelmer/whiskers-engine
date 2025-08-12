#pragma once
#include "Entity.h"
#include <vector>

class EntityManager {
public:
  void addEntity(const Entity &entity);
  std::vector<Entity> &getEntities();

  // Add these:
  Entity &getShip();
  void setShip(const Entity &entity);

private:
  Entity ship;
  std::vector<Entity> entities;
};
