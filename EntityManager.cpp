#include "EntityManager.h"

size_t EntityManager::createEntity(const Entity& e) {
  entities.push_back(e);
  return entities.size() - 1;
}
std::vector<Entity>& EntityManager::getEntities() {
  return entities;
}
