#include "EntityManager.h"

void EntityManager::addEntity(const Entity &e) { entities.push_back(e); }

std::vector<Entity> &EntityManager::getEntities() { return entities; }

Entity &EntityManager::getShip() { return ship; }

void EntityManager::setShip(const Entity &entity) { ship = entity; }
