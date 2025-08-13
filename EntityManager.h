#pragma once
#include <vector>

#include "Entity.h"

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
