// EntityManager.h
// Refactored ECS manager with stable entity handles, destruction, and component queries.
#pragma once

#include <optional>
#include <vector>
#include <cstdint>

#include "Component.h"

// Stable entity handle: index + generation counter.
// When an entity is destroyed and a new entity reuses its slot, the generation
// increments so stale handles are detected as invalid.
struct EntityHandle {
  uint32_t index{0};
  uint32_t generation{0};

  bool operator==(const EntityHandle& o) const { return index == o.index && generation == o.generation; }
  bool operator!=(const EntityHandle& o) const { return !(*this == o); }
};

// A sentinel value for "no entity".
inline constexpr EntityHandle NullEntity{UINT32_MAX, 0};

// Internal entity record (not exposed publicly).
struct EntityRecord {
  TransformComponent transform;
  PhysicsComponent physics;
  ColliderComponent collider;
  RenderComponent render;
  SpriteAnimComponent anim;
  HealthComponent health;
  PlayerInputComponent input;
  CameraComponent camera;
  PlatformComponent platform;
  EnemyAIComponent enemyAI;
  AttackComponent attack;
  ProjectileComponent projectile;
  PickupComponent pickup;
  MessengerComponent messenger;
  LetterComponent letter;
  DeliveryZoneComponent deliveryZone;
  NPCComponent npc;

  // Magicor-specific components
  MagicorPlayerComponent magicorPlayer;
  OrbComponent orb;
  HazardComponent hazard;
  MagicorEnemyComponent magicorEnemy;
  ParticleComponent particle;

  // Flags for which components are "active" on this entity.
  bool hasTransform{true};
  bool hasPhysics{false};
  bool hasCollider{false};
  bool hasRender{false};
  bool hasAnim{false};
  bool hasHealth{false};
  bool hasInput{false};
  bool hasCamera{false};
  bool hasPlatform{false};
  bool hasEnemyAI{false};
  bool hasAttack{false};
  bool hasProjectile{false};
  bool hasPickup{false};
  bool hasMessenger{false};
  bool hasLetter{false};
  bool hasDeliveryZone{false};
  bool hasNPC{false};
  // Magicor
  bool hasMagicorPlayer{false};
  bool hasOrb{false};
  bool hasHazard{false};
  bool hasMagicorEnemy{false};
  bool hasParticle{false};

  bool alive{true};
  uint32_t generation{0};
};

class EntityManager {
 public:
  // Create a new entity, returning a stable handle.
  EntityHandle createEntity();

  // Destroy an entity by handle (marks for deferred removal).
  void destroyEntity(EntityHandle handle);

  // Purge all destroyed entities from storage. Call once per frame.
  void flushDestroyed();

  // Access the internal record by index. Caller must ensure index is valid.
  EntityRecord& get(size_t index);
  const EntityRecord& get(size_t index) const;

  // Resolve a handle to an entity record. Returns nullopt if handle is stale.
  std::optional<std::reference_wrapper<EntityRecord>> tryGet(EntityHandle handle);
  std::optional<std::reference_wrapper<const EntityRecord>> tryGet(EntityHandle handle) const;

  // Access all alive entities (for range-based for loops).
  const std::vector<EntityRecord>& getAll() const { return entities; }
  std::vector<EntityRecord>& getAll() { return entities; }

  // Query: return indices of all alive entities that have a given component.
  std::vector<size_t> queryWithTransform() const;
  std::vector<size_t> queryWithPhysics() const;
  std::vector<size_t> queryWithCollider() const;
  std::vector<size_t> queryWithRender() const;
  std::vector<size_t> queryWithAnim() const;
  std::vector<size_t> queryWithHealth() const;
  std::vector<size_t> queryWithInput() const;
  std::vector<size_t> queryWithCamera() const;
  std::vector<size_t> queryWithPlatform() const;
  std::vector<size_t> queryWithEnemyAI() const;
  std::vector<size_t> queryWithAttack() const;
  std::vector<size_t> queryWithProjectile() const;
  std::vector<size_t> queryWithPickup() const;
  std::vector<size_t> queryWithMessenger() const;
  std::vector<size_t> queryWithLetter() const;
  std::vector<size_t> queryWithDeliveryZone() const;
  std::vector<size_t> queryWithNPC() const;
  // Magicor queries
  std::vector<size_t> queryWithMagicorPlayer() const;
  std::vector<size_t> queryWithOrb() const;
  std::vector<size_t> queryWithHazard() const;
  std::vector<size_t> queryWithMagicorEnemy() const;
  std::vector<size_t> queryWithParticle() const;

  // Count of alive entities.
  size_t aliveCount() const;

  // Reset all entities.
  void clear() {
    entities.clear();
    generations.clear();
    freeIndices.clear();
  }

 private:
  std::vector<EntityRecord> entities;
  std::vector<uint32_t> generations;  // parallel generation counters
  std::vector<size_t> freeIndices;    // recycled slots
};

// ---- Inline implementations ----

inline EntityHandle EntityManager::createEntity() {
  EntityRecord e;
  EntityHandle handle;

  if (!freeIndices.empty()) {
    handle.index = freeIndices.back();
    freeIndices.pop_back();
    handle.generation = ++generations[handle.index];
    entities[handle.index] = e;
    entities[handle.index].alive = true;
    entities[handle.index].generation = handle.generation;
  } else {
    handle.index = static_cast<uint32_t>(entities.size());
    handle.generation = 0;
    entities.push_back(e);
    generations.push_back(0);
  }

  return handle;
}

inline void EntityManager::destroyEntity(EntityHandle handle) {
  if (handle.index >= entities.size()) return;
  EntityRecord& e = entities[handle.index];
  if (!e.alive) return;
  if (e.generation != handle.generation) return;  // stale handle
  e.alive = false;
}

inline void EntityManager::flushDestroyed() {
  // Compact: move alive entities from the end into holes left by destroyed ones.
  // For simplicity in Phase 1 we just erase in-place; an archetype approach
  // would do this more efficiently.
  size_t write = 0;
  for (size_t read = 0; read < entities.size(); ++read) {
    if (entities[read].alive) {
      if (write != read) {
        entities[write] = std::move(entities[read]);
        generations[write] = generations[read];
        // Update handle generations are implicit — we don't reassign handles here.
      }
      ++write;
    }
  }
  // Note: this simple compaction invalidates indices. For Phase 1, entities
  // are sparse enough that we can skip compaction and just tombstone.
  // A production ECS would use proper sparse-set or archetype storage.
}

inline EntityRecord& EntityManager::get(size_t index) {
  return entities[index];
}

inline const EntityRecord& EntityManager::get(size_t index) const {
  return entities[index];
}

inline std::optional<std::reference_wrapper<EntityRecord>> EntityManager::tryGet(EntityHandle handle) {
  if (handle.index >= entities.size()) return std::nullopt;
  EntityRecord& e = entities[handle.index];
  if (!e.alive || e.generation != handle.generation) return std::nullopt;
  return e;
}

inline std::optional<std::reference_wrapper<const EntityRecord>> EntityManager::tryGet(EntityHandle handle) const {
  if (handle.index >= entities.size()) return std::nullopt;
  const EntityRecord& e = entities[handle.index];
  if (!e.alive || e.generation != handle.generation) return std::nullopt;
  return e;
}

inline std::vector<size_t> EntityManager::queryWithTransform() const {
  std::vector<size_t> out;
  for (size_t i = 0; i < entities.size(); ++i)
    if (entities[i].alive && entities[i].hasTransform) out.push_back(i);
  return out;
}

inline std::vector<size_t> EntityManager::queryWithPhysics() const {
  std::vector<size_t> out;
  for (size_t i = 0; i < entities.size(); ++i)
    if (entities[i].alive && entities[i].hasPhysics) out.push_back(i);
  return out;
}

inline std::vector<size_t> EntityManager::queryWithCollider() const {
  std::vector<size_t> out;
  for (size_t i = 0; i < entities.size(); ++i)
    if (entities[i].alive && entities[i].hasCollider) out.push_back(i);
  return out;
}

inline std::vector<size_t> EntityManager::queryWithRender() const {
  std::vector<size_t> out;
  for (size_t i = 0; i < entities.size(); ++i)
    if (entities[i].alive && entities[i].hasRender) out.push_back(i);
  return out;
}

inline std::vector<size_t> EntityManager::queryWithAnim() const {
  std::vector<size_t> out;
  for (size_t i = 0; i < entities.size(); ++i)
    if (entities[i].alive && entities[i].hasAnim) out.push_back(i);
  return out;
}

inline std::vector<size_t> EntityManager::queryWithHealth() const {
  std::vector<size_t> out;
  for (size_t i = 0; i < entities.size(); ++i)
    if (entities[i].alive && entities[i].hasHealth) out.push_back(i);
  return out;
}

inline std::vector<size_t> EntityManager::queryWithInput() const {
  std::vector<size_t> out;
  for (size_t i = 0; i < entities.size(); ++i)
    if (entities[i].alive && entities[i].hasInput) out.push_back(i);
  return out;
}

inline std::vector<size_t> EntityManager::queryWithCamera() const {
  std::vector<size_t> out;
  for (size_t i = 0; i < entities.size(); ++i)
    if (entities[i].alive && entities[i].hasCamera) out.push_back(i);
  return out;
}

inline std::vector<size_t> EntityManager::queryWithPlatform() const {
  std::vector<size_t> out;
  for (size_t i = 0; i < entities.size(); ++i)
    if (entities[i].alive && entities[i].hasPlatform) out.push_back(i);
  return out;
}

inline std::vector<size_t> EntityManager::queryWithEnemyAI() const {
  std::vector<size_t> out;
  for (size_t i = 0; i < entities.size(); ++i)
    if (entities[i].alive && entities[i].hasEnemyAI) out.push_back(i);
  return out;
}

inline std::vector<size_t> EntityManager::queryWithAttack() const {
  std::vector<size_t> out;
  for (size_t i = 0; i < entities.size(); ++i)
    if (entities[i].alive && entities[i].hasAttack) out.push_back(i);
  return out;
}

inline std::vector<size_t> EntityManager::queryWithProjectile() const {
  std::vector<size_t> out;
  for (size_t i = 0; i < entities.size(); ++i)
    if (entities[i].alive && entities[i].hasProjectile) out.push_back(i);
  return out;
}

inline std::vector<size_t> EntityManager::queryWithPickup() const {
  std::vector<size_t> out;
  for (size_t i = 0; i < entities.size(); ++i)
    if (entities[i].alive && entities[i].hasPickup) out.push_back(i);
  return out;
}

inline std::vector<size_t> EntityManager::queryWithMessenger() const {
  std::vector<size_t> out;
  for (size_t i = 0; i < entities.size(); ++i)
    if (entities[i].alive && entities[i].hasMessenger) out.push_back(i);
  return out;
}

inline std::vector<size_t> EntityManager::queryWithLetter() const {
  std::vector<size_t> out;
  for (size_t i = 0; i < entities.size(); ++i)
    if (entities[i].alive && entities[i].hasLetter) out.push_back(i);
  return out;
}

inline std::vector<size_t> EntityManager::queryWithDeliveryZone() const {
  std::vector<size_t> out;
  for (size_t i = 0; i < entities.size(); ++i)
    if (entities[i].alive && entities[i].hasDeliveryZone) out.push_back(i);
  return out;
}

inline std::vector<size_t> EntityManager::queryWithNPC() const {
  std::vector<size_t> out;
  for (size_t i = 0; i < entities.size(); ++i)
    if (entities[i].alive && entities[i].hasNPC) out.push_back(i);
  return out;
}

// Magicor component queries
inline std::vector<size_t> EntityManager::queryWithMagicorPlayer() const {
  std::vector<size_t> out;
  for (size_t i = 0; i < entities.size(); ++i)
    if (entities[i].alive && entities[i].hasMagicorPlayer) out.push_back(i);
  return out;
}

inline std::vector<size_t> EntityManager::queryWithOrb() const {
  std::vector<size_t> out;
  for (size_t i = 0; i < entities.size(); ++i)
    if (entities[i].alive && entities[i].hasOrb) out.push_back(i);
  return out;
}

inline std::vector<size_t> EntityManager::queryWithHazard() const {
  std::vector<size_t> out;
  for (size_t i = 0; i < entities.size(); ++i)
    if (entities[i].alive && entities[i].hasHazard) out.push_back(i);
  return out;
}

inline std::vector<size_t> EntityManager::queryWithMagicorEnemy() const {
  std::vector<size_t> out;
  for (size_t i = 0; i < entities.size(); ++i)
    if (entities[i].alive && entities[i].hasMagicorEnemy) out.push_back(i);
  return out;
}

inline std::vector<size_t> EntityManager::queryWithParticle() const {
  std::vector<size_t> out;
  for (size_t i = 0; i < entities.size(); ++i)
    if (entities[i].alive && entities[i].hasParticle) out.push_back(i);
  return out;
}

inline size_t EntityManager::aliveCount() const {
  size_t count = 0;
  for (auto& e : entities)
    if (e.alive) ++count;
  return count;
}
