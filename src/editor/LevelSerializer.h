// LevelSerializer.h
// Serialize/deserialize EntityManager state to/from JSON with CRUD storage.
#pragma once

#include <string>
#include <vector>
#include "../core/EntityManager.h"

namespace LevelSerializer {

  // Serialize all alive entities to a JSON string
  std::string serialize(EntityManager& em, const std::string& name = "untitled");

  // Deserialize JSON and recreate entities in the EntityManager
  bool deserialize(const std::string& json, EntityManager& em);

  // Save level to persistent storage (file on native, localStorage on web)
  bool save(EntityManager& em, const std::string& name);

  // Load level from persistent storage
  std::string load(const std::string& name);

  // List all saved level names
  std::vector<std::string> list();

  // Delete a saved level
  bool remove(const std::string& name);
}
