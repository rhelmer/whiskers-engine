#pragma once
#include <vector>

#include "Entity.h"

class EntityManager {
 public:
  size_t createEntity(const Entity& e);
  std::vector<Entity>& getEntities();

 private:
  std::vector<Entity> entities;
};