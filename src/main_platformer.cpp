// main_platformer.cpp
// Whiskers Engine — sample 2.5D platformer (ECS demo + ImGui editor)
#include <SDL2/SDL.h>
#ifdef WHISKERS_WASM_BUILD
#include <whiskers/glad_wasm.h>
#else
#include <glad/glad.h>
#endif

#include <iostream>
#include <memory>
#include <cmath>

#include "core/EntityManager.h"
#include "core/GameLoop.h"
#include "core/System.h"
#include "physics/PhysicsSystem.h"
#include "rendering/Renderer.h"
#include "rendering/CameraSystem.h"
#include "level/Level.h"
#include "game/PixelArtSprite.h"
#include "game/SpriteAnimator.h"
#include "editor/Editor.h"

#ifdef WHISKERS_DEBUG_PROTOCOL
#include "core/DebugProtocol.h"
#include "core/DebugProtocolHandler.h"
#include "core/DebugInputBridge.h"
#endif

// ---------------------------------------------------------------------------
// PlatformerInputSystem — reads SDL keyboard state and writes to input
// components.
// ---------------------------------------------------------------------------
class PlatformerInputSystem : public System {
 public:
  PlatformerInputSystem(GameLoop* gl) : gameLoop(gl) {}

#ifdef WHISKERS_DEBUG_PROTOCOL
  void setDebugBridge(DebugInputBridge* b) { debugBridge_ = b; }
#endif

  void update(EntityManager& em, float deltaTime) override {
    (void)deltaTime;

    const Uint8* state = SDL_GetKeyboardState(NULL);

    auto inputIndices = em.queryWithInput();
    for (size_t idx : inputIndices) {
      auto& e = em.get(idx);

#ifdef WHISKERS_DEBUG_PROTOCOL
      if (debugBridge_) {
        if (debugBridge_->shouldSkipSdl(idx)) {
          if (debugBridge_->setInput.active && !debugBridge_->inputFrame.active) {
            debugBridge_->applySetOverride(e);
          }
          continue;
        }
      }
#endif

      // Previous frame state for edge detection
      bool prevJump = e.input.jumpPressed;
      bool prevAttack = e.input.attackPressed;
      bool prevFire = e.input.firePressed;

      // Movement (A/D or Left/Right arrows)
      e.input.moveAxis = 0.0f;
      if (state[SDL_SCANCODE_A] || state[SDL_SCANCODE_LEFT]) e.input.moveAxis -= 1.0f;
      if (state[SDL_SCANCODE_D] || state[SDL_SCANCODE_RIGHT]) e.input.moveAxis += 1.0f;

      // Jump (Space or Up/W)
      e.input.jumpHeld = state[SDL_SCANCODE_SPACE] || state[SDL_SCANCODE_W] || state[SDL_SCANCODE_UP];
      e.input.jumpPressed = e.input.jumpHeld && !prevJump;

      // Attack (X or J — melee)
      e.input.attackPressed = state[SDL_SCANCODE_X] || state[SDL_SCANCODE_J];

      // Fire (C or K â€” fireball ranged)
      e.input.firePressed = (state[SDL_SCANCODE_C] || state[SDL_SCANCODE_K]) && !prevFire;
      prevFire = state[SDL_SCANCODE_C] || state[SDL_SCANCODE_K];

      // Crouch (S or Down)
      e.input.crouchHeld = state[SDL_SCANCODE_S] || state[SDL_SCANCODE_DOWN];

      // Debug: R to restart
      if (state[SDL_SCANCODE_R] && !rWasPressed) {
        restartRequested = true;
        if (gameLoop) gameLoop->running = false;
      }
      rWasPressed = state[SDL_SCANCODE_R];
    }
  }

  bool restartRequested{false};
  bool rWasPressed{false};

 private:
  GameLoop* gameLoop{nullptr};
#ifdef WHISKERS_DEBUG_PROTOCOL
  DebugInputBridge* debugBridge_{nullptr};
#endif
};

// ---------------------------------------------------------------------------
// PlayerMovementSystem — reads input and applies forces to physics.
// ---------------------------------------------------------------------------
class PlayerMovementSystem : public System {
 public:
  void update(EntityManager& em, float deltaTime) override {
    auto inputIndices = em.queryWithInput();
    for (size_t idx : inputIndices) {
      auto& e = em.get(idx);
      if (!e.hasPhysics) continue;

      const float moveSpeed = 8.0f;
      const float groundAccel = 40.0f;
      const float airAccel = 24.0f;
      const float decel = 30.0f;
      const float jumpVelocity = 10.0f;

      // Horizontal movement
      float accel = e.physics.onGround ? groundAccel : airAccel;
      float targetVel = e.input.moveAxis * moveSpeed;

      if (e.input.moveAxis != 0.0f) {
        if (e.physics.velocity.x < targetVel) {
          e.physics.velocity.x = std::min(e.physics.velocity.x + accel * deltaTime, targetVel);
        } else {
          e.physics.velocity.x = std::max(e.physics.velocity.x - decel * deltaTime, targetVel);
        }
      } else {
        if (std::abs(e.physics.velocity.x) < decel * deltaTime) {
          e.physics.velocity.x = 0.0f;
        } else {
          e.physics.velocity.x -= std::copysign(decel * deltaTime, e.physics.velocity.x);
        }
      }

      // Coyote time
      if (e.physics.onGround) {
        coyoteTimer = 0.1f;
        e.transform.lastSafePosition = e.transform.position;
      } else {
        coyoteTimer -= deltaTime;
      }

      // Jump buffer
      if (e.input.jumpPressed) {
        jumpBufferTimer = 0.1f;
      } else {
        jumpBufferTimer -= deltaTime;
      }

      // Jump
      if (jumpBufferTimer > 0.0f && coyoteTimer > 0.0f) {
        e.physics.velocity.y = jumpVelocity;
        jumpBufferTimer = 0.0f;
        coyoteTimer = 0.0f;
      }

      // Variable jump height
      if (!e.input.jumpHeld && e.physics.velocity.y > jumpVelocity * 0.4f) {
        e.physics.velocity.y = jumpVelocity * 0.4f;
      }
    }
  }

 private:
  float coyoteTimer{0.0f};
  float jumpBufferTimer{0.0f};
};

// ---------------------------------------------------------------------------
// HazardSystem — handles enemy-to-player collisions and pitfalls (No Death).
// ---------------------------------------------------------------------------
class HazardSystem : public System {
 public:
  void update(EntityManager& em, float deltaTime) override {
    auto inputIndices = em.queryWithInput();
    auto enemyIndices = em.queryWithEnemyAI();

    for (size_t pIdx : inputIndices) {
      auto& player = em.get(pIdx);
      if (!player.hasCollider || !player.hasTransform) continue;

      // Update stun timer
      if (player.hasMessenger) {
        player.messenger.stunTimer = std::max(0.0f, player.messenger.stunTimer - deltaTime);
      }

      AABB playerWorld = AABB::fromCenterAndHalfSize(
          glm::vec2(player.transform.position), player.collider.bounds.halfSize());

      // 1. Check enemy collisions
      bool hitByEnemy = false;
      for (size_t eIdx : enemyIndices) {
        auto& enemy = em.get(eIdx);
        if (!enemy.alive || !enemy.hasCollider) continue;

        AABB enemyWorld = AABB::fromCenterAndHalfSize(
            glm::vec2(enemy.transform.position), enemy.collider.bounds.halfSize());

        if (aabbOverlap(playerWorld, enemyWorld)) {
          hitByEnemy = true;
          break;
        }
      }

      // 2. Check pitfall (Y < -10 for example, or use camera bounds)
      bool fellInPit = (player.transform.position.y < -5.0f);

      if (hitByEnemy || fellInPit) {
        respawnPlayer(player);
      }
    }
  }

 private:
  void respawnPlayer(EntityRecord& player) {
    player.transform.position = player.transform.lastSafePosition;
    if (player.hasPhysics) {
      player.physics.velocity = glm::vec2(0.0f);
    }
    if (player.hasMessenger) {
      player.messenger.stunCount++;
      player.messenger.stunTimer = 0.5f; // briefly stun
    }
    // TODO: Play "poof" sound/effect
    std::cout << "Player respawned at safe position. Stun count: " 
              << (player.hasMessenger ? player.messenger.stunCount : 0) << std::endl;
  }
};

// ---------------------------------------------------------------------------
// CombatSystem — handles melee attacks, fireballs, and damage.
// ---------------------------------------------------------------------------
class CombatSystem : public System {
 public:
  CombatSystem(SpriteAnimator* animator) : animator(animator) {}

  void init() override {
    // Create sprite atlases for projectiles and effects
    fireballAtlas = createAtlasFromSpriteDef(createFireballSprite());
    swingAtlas = createAtlasFromSpriteDef(createMeleeSwingSprite());
  }

  void update(EntityManager& em, float deltaTime) override {
    auto inputIndices = em.queryWithInput();
    for (size_t idx : inputIndices) {
      auto& e = em.get(idx);
      if (!e.hasAttack) continue;

      // Cooldowns
      e.attack.cooldownTimer = std::max(0.0f, e.attack.cooldownTimer - deltaTime);
      e.attack.durationTimer = std::max(0.0f, e.attack.durationTimer - deltaTime);
      if (e.attack.durationTimer <= 0.0f) e.attack.attacking = false;

      // Melee attack
      if (e.input.attackPressed && e.attack.cooldownTimer <= 0.0f) {
        e.attack.attacking = true;
        e.attack.durationTimer = e.attack.duration;
        e.attack.cooldownTimer = e.attack.cooldown;
        e.attack.comboStep = (e.attack.comboStep + 1) % e.attack.maxCombo;

        // Check for enemies in range
        performMeleeAttack(em, e);
      }

      // Ranged attack (fireball) — costs mana
      if (e.input.firePressed && e.attack.cooldownTimer <= 0.0f && e.hasHealth && e.health.mana >= 2) {
        e.health.mana -= 2;
        e.attack.cooldownTimer = e.attack.cooldown * 1.5f;  // slightly longer cooldown for fireballs
        spawnFireball(em, e);
      }

      // Mana regen
      if (e.hasHealth) {
        e.health.manaRegenTimer += deltaTime;
        if (e.health.manaRegenTimer >= 1.0f) {
          e.health.manaRegenTimer -= 1.0f;
          e.health.mana = std::min(e.health.mana + 1, e.health.maxMana);
        }
      }
    }

    // Update projectiles
    updateProjectiles(em, deltaTime);
  }

  void clear() {
    // Projectiles are now entities, so they are cleared by em.clear()
  }

 private:
  SpriteAnimator* animator;
  SpriteAtlas fireballAtlas;
  SpriteAtlas swingAtlas;

  void performMeleeAttack(EntityManager& em, EntityRecord& player) {
    // ... (rest of performMeleeAttack remains the same)
    // Create hitbox in facing direction
    glm::vec2 hitboxCenter = glm::vec2(player.transform.position) + player.attack.facingDir * player.attack.range * 0.5f;
    AABB hitbox = AABB::fromCenterAndHalfSize(hitboxCenter, glm::vec2(player.attack.range * 0.5f, 0.4f));

    auto enemyIndices = em.queryWithEnemyAI();
    for (size_t idx : enemyIndices) {
      auto& enemy = em.get(idx);
      if (!enemy.alive || !enemy.hasCollider) continue;

      AABB enemyWorld = AABB::fromCenterAndHalfSize(
        glm::vec2(enemy.transform.position), enemy.collider.bounds.halfSize());

      if (aabbOverlap(hitbox, enemyWorld)) {
        // Deal damage
        if (enemy.hasHealth) {
          enemy.health.current -= (int)player.attack.damage;
          if (enemy.health.current <= 0) {
            enemy.alive = false;  // enemy dies
            // Drop coins
            dropCoins(em, glm::vec2(enemy.transform.position), 3 + (rand() % 4));
          }
        } else {
          enemy.alive = false;
          dropCoins(em, glm::vec2(enemy.transform.position), 2 + (rand() % 3));
        }
      }
    }
  }

  void spawnFireball(EntityManager& em, const EntityRecord& player) {
    glm::vec2 dir = player.attack.facingDir;
    glm::vec2 spawnPos = glm::vec2(player.transform.position) + dir * 0.5f;

    auto h = em.createEntity();
    auto& e = em.get(h.index);
    e.hasTransform = true;
    e.transform.position = glm::vec3(spawnPos, 0.1f); // slightly in front
    e.hasRender = true;
    e.render.usePixelArtSprite = true;
    e.render.spriteScale = 0.05f;
    e.render.sortOrder = 12;
    e.render.flipX = (dir.x < 0);
    
    e.hasProjectile = true;
    e.projectile.damage = 2.0f;
    e.projectile.direction = dir;
    e.projectile.speed = 10.0f;
    e.projectile.lifetime = 2.0f;
    e.projectile.lifetimeTimer = 0.0f;
    e.projectile.isPlayerOwned = true;

    e.hasCollider = true;
    e.collider.shape = ColliderShape::AABB;
    e.collider.bounds = AABB::fromCenterAndHalfSize(glm::vec2(0), glm::vec2(0.15f, 0.15f));
    e.collider.layer = Layer_Projectile;
    e.collider.mask = Layer_Enemy;

    if (animator) {
      animator->addState(h.index, "idle", { "idle", fireballAtlas, 0.1f, true });
      animator->play(h.index, "idle");
    }
  }

  void updateProjectiles(EntityManager& em, float deltaTime) {
    auto projIndices = em.queryWithProjectile();
    for (size_t idx : projIndices) {
      auto& e = em.get(idx);
      if (!e.alive) continue;

      auto& p = e.projectile;
      e.transform.position.x += p.direction.x * p.speed * deltaTime;
      e.transform.position.y += p.direction.y * p.speed * deltaTime;
      
      p.lifetimeTimer += deltaTime;
      if (p.lifetimeTimer >= p.lifetime) {
        e.alive = false;
        continue;
      }

      // Check collision
      if (p.isPlayerOwned) {
        auto enemyIndices = em.queryWithEnemyAI();
        for (size_t eIdx : enemyIndices) {
          auto& enemy = em.get(eIdx);
          if (!enemy.alive || !enemy.hasCollider) continue;

          AABB enemyWorld = AABB::fromCenterAndHalfSize(
            glm::vec2(enemy.transform.position), enemy.collider.bounds.halfSize());
          AABB fireballBB = AABB::fromCenterAndHalfSize(
            glm::vec2(e.transform.position), e.collider.bounds.halfSize());

          if (aabbOverlap(fireballBB, enemyWorld)) {
            if (enemy.hasHealth) {
              enemy.health.current -= (int)p.damage;
              if (enemy.health.current <= 0) {
                enemy.alive = false;
                dropCoins(em, glm::vec2(enemy.transform.position), 3 + (rand() % 4));
              }
            } else {
              enemy.alive = false;
              dropCoins(em, glm::vec2(enemy.transform.position), 2 + (rand() % 3));
            }
            e.alive = false; // destroy projectile
            break;
          }
        }
      }
    }
  }

  void dropCoins(EntityManager& em, glm::vec2 pos, int count) {
    for (int i = 0; i < count; i++) {
      // We don't spawn actual entities for coins here — that's done in the demo level.
      // In a full implementation, we'd create coin pickup entities here.
      (void)em; (void)pos; (void)i; (void)count;
    }
  }
};

// ---------------------------------------------------------------------------
// PickupSystem — handles coin/heart/mana pickups.
// ---------------------------------------------------------------------------
class PickupSystem : public System {
 public:
  void init() override {
    coinAtlas = createAtlasFromSpriteDef(createCoinSprite());
    heartAtlas = createAtlasFromSpriteDef(createHeartSprite());
    manaOrbAtlas = createAtlasFromSpriteDef(createManaOrbSprite());
    keyAtlas = createAtlasFromSpriteDef(createKeySprite());
  }

  void update(EntityManager& em, float deltaTime) override {
    auto pickupIndices = em.queryWithPickup();
    auto inputIndices = em.queryWithInput();

    for (size_t idx : pickupIndices) {
      auto& e = em.get(idx);
      if (!e.hasPickup || !e.hasCollider || e.pickup.collected) continue;

      // Bob animation
      e.pickup.bobTimer += deltaTime * 3.0f;
      float bobOffset = std::sin(e.pickup.bobTimer) * e.pickup.bobAmount;
      e.transform.position.z = bobOffset * 0.01f;  // subtle Z-offset for depth

      // Check pickup collision with player
      for (size_t playerIdx : inputIndices) {
        auto& player = em.get(playerIdx);
        if (!player.hasCollider) continue;

        AABB pickupWorld = AABB::fromCenterAndHalfSize(
          glm::vec2(e.transform.position), e.collider.bounds.halfSize());
        AABB playerWorld = AABB::fromCenterAndHalfSize(
          glm::vec2(player.transform.position), player.collider.bounds.halfSize());

        if (aabbOverlap(pickupWorld, playerWorld)) {
          e.pickup.collected = true;
          e.alive = false;

          // Apply pickup effect
          applyPickup(player, e.pickup);
        }
      }
    }
  }

 private:
  SpriteAtlas coinAtlas;
  SpriteAtlas heartAtlas;
  SpriteAtlas manaOrbAtlas;
  SpriteAtlas keyAtlas;

  void applyPickup(EntityRecord& player, const PickupComponent& pickup) {
    switch (pickup.type) {
      case PickupComponent::Coin:
        if (player.hasHealth) player.health.coins += pickup.value;
        break;
      case PickupComponent::Heart:
        if (player.hasHealth) player.health.current = std::min(player.health.current + pickup.value, player.health.max);
        break;
      case PickupComponent::Mana:
        if (player.hasHealth) player.health.mana = std::min(player.health.mana + pickup.value, player.health.maxMana);
        break;
      case PickupComponent::Key:
        if (player.hasHealth) player.health.keys += pickup.value;
        break;
      case PickupComponent::Letter:
        if (player.hasMessenger) {
          player.messenger.heldLetters.push_back(pickup.letterId);
          std::cout << "Collected letter: " << pickup.letterId << std::endl;
        }
        break;
    }
  }

 public:
  // Expose atlases for rendering
  const SpriteAtlas& getCoinAtlas() const { return coinAtlas; }
  const SpriteAtlas& getHeartAtlas() const { return heartAtlas; }
  const SpriteAtlas& getManaOrbAtlas() const { return manaOrbAtlas; }
  const SpriteAtlas& getKeyAtlas() const { return keyAtlas; }

  void clear() {
    // Atlases are persistent, nothing to clear for now
  }
};

// ---------------------------------------------------------------------------
// Level state
// ---------------------------------------------------------------------------
enum class GameLevel { Level1, Level2 };
static GameLevel currentLevel = GameLevel::Level1;

// ---------------------------------------------------------------------------
// DeliveryZoneSystem — handles letter delivery when player enters trigger zone.
// ---------------------------------------------------------------------------
class DeliveryZoneSystem : public System {
 public:
  void update(EntityManager& em, float deltaTime) override {
    auto zoneIndices = em.queryWithDeliveryZone();
    auto messengerIndices = em.queryWithMessenger();

    for (size_t zoneIdx : zoneIndices) {
      auto& zone = em.get(zoneIdx);
      if (!zone.hasDeliveryZone || !zone.hasCollider || zone.deliveryZone.triggered) continue;

      // Update cooldown
      zone.deliveryZone.triggerCooldown = std::max(0.0f, zone.deliveryZone.triggerCooldown - deltaTime);
      if (zone.deliveryZone.triggerCooldown > 0.0f) continue;

      // Check all messengers (players) for collision
      for (size_t msgIdx : messengerIndices) {
        auto& messenger = em.get(msgIdx);
        if (!messenger.hasCollider) continue;

        AABB zoneWorld = AABB::fromCenterAndHalfSize(
          glm::vec2(zone.transform.position), zone.collider.bounds.halfSize());
        AABB playerWorld = AABB::fromCenterAndHalfSize(
          glm::vec2(messenger.transform.position), messenger.collider.bounds.halfSize());

        if (aabbOverlap(zoneWorld, playerWorld)) {
          // Check if player has the expected letter
          bool hasLetter = false;
          if (messenger.hasMessenger) {
            for (const auto& letterId : messenger.messenger.heldLetters) {
              if (letterId == zone.deliveryZone.expectedLetterId) {
                hasLetter = true;
                break;
              }
            }
          }

          // Handle delivery
          if (hasLetter) {
            // Remove the letter from player
            auto& letters = messenger.messenger.heldLetters;
            letters.erase(std::remove(letters.begin(), letters.end(), zone.deliveryZone.expectedLetterId), letters.end());
            
            // Award divine favor
            if (messenger.hasMessenger) {
              messenger.messenger.divineFavor += 1;
            }

            // Mark zone as triggered
            zone.deliveryZone.triggered = true;
            zone.deliveryZone.triggerCooldown = 2.0f; // cooldown before re-triggering

            std::cout << "[" << zone.deliveryZone.npcName << "]: " 
                      << zone.deliveryZone.dialogOnSuccess << std::endl;
            std::cout << "+1 bonus! (Total: " << messenger.messenger.divineFavor << ")" << std::endl;
          } else if (messenger.messenger.heldLetters.empty()) {
            std::cout << "[" << zone.deliveryZone.npcName << "]: " 
                      << zone.deliveryZone.dialogOnEmpty << std::endl;
          } else {
            std::cout << "[" << zone.deliveryZone.npcName << "]: " 
                      << zone.deliveryZone.dialogOnWrong << std::endl;
          }
        }
      }
    }
  }
};

// ---------------------------------------------------------------------------
// LevelTransitionSystem — handles advancing to next level on delivery success.
// ---------------------------------------------------------------------------
class LevelTransitionSystem : public System {
 public:
  LevelTransitionSystem(GameLoop* gl) : gameLoop(gl) {}

  void update(EntityManager& em, float deltaTime) override {
    auto zoneIndices = em.queryWithDeliveryZone();
    
    for (size_t zoneIdx : zoneIndices) {
      auto& zone = em.get(zoneIdx);
      if (!zone.hasDeliveryZone) continue;
      
      // Check if this zone was just triggered
      if (zone.deliveryZone.triggered && !zone.deliveryZone.triggeredLastFrame) {
        // Check which zone was triggered
        if (zone.deliveryZone.expectedLetterId == "demo_pkg_a_to_b") {
          // Level 1 complete - advance to Level 2
          nextLevel = GameLevel::Level2;
          transitionRequested = true;
          gameLoop->running = false;
        } else if (zone.deliveryZone.expectedLetterId == "demo_pkg_b_to_c") {
          // Level 2 complete - show completion
          std::cout << "\n=== LEVEL 2 COMPLETE ===" << std::endl;
          std::cout << "All deliveries for this demo are complete!" << std::endl;
        }
      }
      
      zone.deliveryZone.triggeredLastFrame = zone.deliveryZone.triggered;
    }
  }

  void reset() {
    transitionRequested = false;
    nextLevel = GameLevel::Level1;
  }

  bool transitionRequested{false};
  GameLevel nextLevel{GameLevel::Level1};

 private:
  GameLoop* gameLoop{nullptr};
};

// ---------------------------------------------------------------------------
// NPCInteractionSystem — handles NPC dialog and letter pickup.
// ---------------------------------------------------------------------------
class NPCInteractionSystem : public System {
 public:
  void update(EntityManager& em, float deltaTime) override {
    auto npcIndices = em.queryWithNPC();
    auto messengerIndices = em.queryWithMessenger();

    for (size_t npcIdx : npcIndices) {
      auto& npc = em.get(npcIdx);
      if (!npc.hasNPC || !npc.hasCollider) continue;

      // Check all messengers (players) for collision
      for (size_t msgIdx : messengerIndices) {
        auto& messenger = em.get(msgIdx);
        if (!messenger.hasCollider) continue;

        AABB npcWorld = AABB::fromCenterAndHalfSize(
          glm::vec2(npc.transform.position), npc.collider.bounds.halfSize());
        AABB playerWorld = AABB::fromCenterAndHalfSize(
          glm::vec2(messenger.transform.position), messenger.collider.bounds.halfSize());

        if (aabbOverlap(npcWorld, playerWorld)) {
          // Check if NPC has a letter to give and hasn't given it yet
          if (npc.npc.hasLetterForPlayer && !npc.npc.letterGiven) {
            if (messenger.hasMessenger) {
              messenger.messenger.heldLetters.push_back(npc.npc.letterIdToGive);
              npc.npc.letterGiven = true;
              std::cout << "[" << npc.npc.name << "]: 'Take this letter to " 
                        << (npc.npc.letterIdToGive.find("to_") != std::string::npos ? 
                            npc.npc.letterIdToGive.substr(npc.npc.letterIdToGive.find("to_") + 3) : "the destination")
                        << "!'" << std::endl;
            }
          } else if (!npc.npc.dialog.empty()) {
            // Regular dialog
            std::cout << "[" << npc.npc.name << "]: " << npc.npc.dialog << std::endl;
          }
        }
      }
    }
  }
};

// ---------------------------------------------------------------------------
// EnemyMovementSystem — patrol behavior with facing direction.
// ---------------------------------------------------------------------------
class EnemyMovementSystem : public System {
 public:
  void update(EntityManager& em, float deltaTime) override {
    auto aiIndices = em.queryWithEnemyAI();
    for (size_t idx : aiIndices) {
      auto& e = em.get(idx);
      if (!e.alive || !e.hasPhysics) continue;

      // Skip dead/stunned enemies
      if (e.enemyAI.state == EnemyAIComponent::Dead) continue;
      if (e.enemyAI.state == EnemyAIComponent::Stunned) {
        e.enemyAI.stunTimer -= deltaTime;
        if (e.enemyAI.stunTimer <= 0.0f) {
          e.enemyAI.state = EnemyAIComponent::Patrol;
        }
        continue;
      }

      // Update attack cooldown
      e.enemyAI.attackCooldown = std::max(0.0f, e.enemyAI.attackCooldown - deltaTime);

      float dir = e.enemyAI.facingRight ? 1.0f : -1.0f;
      e.physics.velocity.x = dir * e.enemyAI.speed;

      // Sync facing direction with sprite flip
      if (e.hasRender) {
        e.render.flipX = !e.enemyAI.facingRight;
      }

      // Turn around at patrol bounds
      if (e.transform.position.x >= e.enemyAI.patrolEnd) {
        e.enemyAI.facingRight = false;
      } else if (e.transform.position.x <= e.enemyAI.patrolStart) {
        e.enemyAI.facingRight = true;
      }

      // Sync attack facing direction
      if (e.hasAttack) {
        e.attack.facingDir = glm::vec2(dir, 0.0f);
      }
    }
  }
};

// ---------------------------------------------------------------------------
// MovingPlatformSystem — updates moving platform positions.
// ---------------------------------------------------------------------------
class MovingPlatformSystem : public System {
 public:
  void update(EntityManager& em, float deltaTime) override {
    auto platformIndices = em.queryWithPlatform();
    for (size_t idx : platformIndices) {
      auto& e = em.get(idx);
      if (e.platform.moveType != PlatformComponent::Moving) continue;
      if (e.platform.path.size() < 2) continue;

      e.platform.lastPosition = glm::vec2(e.transform.position);

      int current = e.platform.currentWaypoint;
      int next = (current + 1) % e.platform.path.size();

      glm::vec2 from = e.platform.path[current];
      glm::vec2 to = e.platform.path[next];
      glm::vec2 dir = to - from;
      float dist = glm::length(dir);

      if (dist < 0.001f) {
        e.platform.currentWaypoint = next;
        continue;
      }

      e.platform.pathProgress += (e.platform.speed / dist) * deltaTime;
      if (e.platform.pathProgress >= 1.0f) {
        e.platform.pathProgress = 0.0f;
        e.platform.currentWaypoint = next;
        from = e.platform.path[current];
        to = e.platform.path[next];
        dir = to - from;
      }

      glm::vec2 newPos = glm::mix(from, to, e.platform.pathProgress);
      e.transform.position = glm::vec3(newPos, e.transform.position.z);
    }
  }
};

// ---------------------------------------------------------------------------
// SpriteRenderSystem — syncs sprite animator output to render components.
// ---------------------------------------------------------------------------
class SpriteRenderSystem : public System {
 public:
  SpriteRenderSystem(SpriteAnimator* animator) : animator(animator) {}

  void lateUpdate(EntityManager& em, float deltaTime) override {
    animator->update(em, deltaTime);
  }

 private:
  SpriteAnimator* animator;
};

// ---------------------------------------------------------------------------
// ParallaxRenderSystem — renders background parallax layers.
// ---------------------------------------------------------------------------
class ParallaxRenderSystem : public System {
 public:
  void lateUpdate(EntityManager& em, float deltaTime) override {
    (void)em;
    (void)deltaTime;
  }
};

// ---------------------------------------------------------------------------
// Demo level 2 — moving platforms + second delivery
// ---------------------------------------------------------------------------
static void createDemoLevelTwo(EntityManager& em, SpriteAnimator& spriteAnimator,
                          PickupSystem& pickupSystem) {
  // ---- Player ----
  {
    auto h = em.createEntity();
    auto& e = em.get(h.index);
    e.hasTransform = true;
    e.transform.position = glm::vec3(2.0f, 2.0f, 0.0f);
    e.transform.lastSafePosition = e.transform.position;
    e.hasPhysics = true;
    e.physics.gravityScale = 1.0f;
    e.physics.affectedByGravity = true;
    e.hasCollider = true;
    e.collider.shape = ColliderShape::AABB;
    e.collider.bounds = AABB::fromCenterAndHalfSize(glm::vec2(0), glm::vec2(0.3f, 0.5f));
    e.collider.layer = Layer_Player;
    e.collider.mask = Layer_Platform | Layer_Enemy | Layer_Pickup;
    e.hasRender = true;
    e.render.sortOrder = 10;
    e.render.spriteScale = 0.06f;
    e.render.usePixelArtSprite = true;
    e.render.spriteSourceJSON = exportSpriteDefToJSON(createHeroIdleSprite());
    e.hasInput = true;
    e.hasCamera = true;
    e.camera.followTarget = (int)h.index;
    e.camera.offset = glm::vec2(0.0f, 2.0f);
    e.camera.smoothSpeed = 8.0f;
    e.camera.bounds.enabled = false;
    
    e.hasMessenger = true;
    e.messenger.stunCount = 0;
    e.messenger.stunTimer = 0.0f;
    e.messenger.divineFavor = 0;

    e.hasHealth = true;
    e.health.current = 6;
    e.health.max = 6;
    e.health.mana = 10;
    e.health.maxMana = 10;
    e.health.coins = 0;
    
    e.hasAttack = true;
    e.attack.type = AttackComponent::Melee;
    e.attack.damage = 1.0f;
    e.attack.range = 0.8f;
    e.attack.cooldown = 0.3f;
    e.attack.duration = 0.15f;
    e.attack.facingDir = glm::vec2(1.0f, 0.0f);

    // Register sprite animations
    auto idleAtlas = createAtlasFromSpriteDef(createHeroIdleSprite());
    auto runAtlas = createAtlasFromSpriteDef(createHeroRunSprite());
    auto jumpAtlas = createAtlasFromSpriteDef(createHeroJumpSprite());
    auto attackAtlas = createAtlasFromSpriteDef(createHeroAttackSprite());

    spriteAnimator.addState(h.index, "idle", { "idle", idleAtlas, 0.5f, false });
    spriteAnimator.addState(h.index, "run", { "run", runAtlas, 0.1f, true });
    spriteAnimator.addState(h.index, "jump", { "jump", jumpAtlas, 0.5f, false });
    spriteAnimator.addState(h.index, "attack", { "attack", attackAtlas, 0.12f, false });
    spriteAnimator.play(h.index, "idle");
  }

  // ---- Dock NPC (gives second parcel) ----
  {
    auto h = em.createEntity();
    auto& e = em.get(h.index);
    e.hasTransform = true;
    e.transform.position = glm::vec3(2.0f, 1.0f, 0.0f);
    e.hasRender = true;
    e.render.color = glm::vec3(0.3f, 0.3f, 0.4f);
    e.render.use3DGeometry = true;
    e.transform.scale = glm::vec3(0.7f, 1.4f, 0.7f);
    e.render.sortOrder = 5;
    e.hasCollider = true;
    e.collider.shape = ColliderShape::AABB;
    e.collider.bounds = AABB::fromCenterAndHalfSize(glm::vec2(0), glm::vec2(0.6f, 1.0f));
    e.collider.layer = Layer_None;
    e.collider.isTrigger = true;
    e.hasNPC = true;
    e.npc.type = NPCComponent::QuestGiver;
    e.npc.name = "Dock worker";
    e.npc.dialog = "Parcel delivered! Take this next one to the clerk past the moving platforms.";
    e.npc.hasLetterForPlayer = true;
    e.npc.letterIdToGive = "demo_pkg_b_to_c";
    e.npc.letterGiven = false;
  }

  // ---- Moving ferry platform ----
  {
    auto h = em.createEntity();
    auto& e = em.get(h.index);
    e.hasTransform = true;
    e.transform.position = glm::vec3(5.0f, 0.0f, 0.0f);
    e.hasCollider = true;
    e.collider.shape = ColliderShape::AABB;
    e.collider.bounds = AABB::fromCenterAndHalfSize(glm::vec2(0), glm::vec2(3.0f, 0.3f));
    e.transform.scale = glm::vec3(6.0f, 0.6f, 1.5f); // Match collider
    e.collider.layer = Layer_Platform;
    e.collider.mask = Layer_None;
    e.hasRender = true;
    e.render.color = glm::vec3(0.25f, 0.18f, 0.12f);  // dark wood
    e.render.use3DGeometry = true;
    e.render.sortOrder = 0;
    e.hasPlatform = true;
    e.platform.moveType = PlatformComponent::Moving;
    e.platform.path = {glm::vec2(5.0f, 0.0f), glm::vec2(15.0f, 0.0f), glm::vec2(15.0f, 0.0f), glm::vec2(5.0f, 0.0f)};
    e.platform.speed = 1.0f;
  }

  // ---- Floating fog platforms (moving slightly) ----
  for (int i = 0; i < 3; i++) {
    auto h = em.createEntity();
    auto& e = em.get(h.index);
    e.hasTransform = true;
    float x = 8.0f + i * 4.0f;
    float y = 1.0f + (i % 2) * 0.5f;
    e.transform.position = glm::vec3(x, y, 0.0f);
    e.hasCollider = true;
    e.collider.shape = ColliderShape::AABB;
    e.collider.bounds = AABB::fromCenterAndHalfSize(glm::vec2(0), glm::vec2(1.5f, 0.2f));
    e.transform.scale = glm::vec3(3.0f, 0.4f, 1.0f);
    e.collider.layer = Layer_Platform;
    e.collider.mask = Layer_None;
    e.hasRender = true;
    e.render.color = glm::vec3(0.4f, 0.4f, 0.5f); // foggy gray
    e.render.use3DGeometry = true;
    e.render.sortOrder = 0;
    e.hasPlatform = true;
    e.platform.moveType = PlatformComponent::Moving;
    e.platform.path = {glm::vec2(x, y), glm::vec2(x, y + 0.3f), glm::vec2(x, y)};
    e.platform.speed = 0.5f;
  }

  // ---- Clerk NPC (Level 2 drop-off) ----
  {
    auto h = em.createEntity();
    auto& e = em.get(h.index);
    e.hasTransform = true;
    e.transform.position = glm::vec3(18.0f, 1.0f, 0.0f);
    e.hasRender = true;
    e.render.color = glm::vec3(0.8f, 0.6f, 0.4f);
    e.render.use3DGeometry = true;
    e.transform.scale = glm::vec3(0.7f, 1.4f, 0.7f);
    e.render.sortOrder = 5;
    e.hasCollider = true;
    e.collider.shape = ColliderShape::AABB;
    e.collider.bounds = AABB::fromCenterAndHalfSize(glm::vec2(0), glm::vec2(0.6f, 1.0f));
    e.collider.layer = Layer_None;
    e.collider.isTrigger = true;
    e.hasNPC = true;
    e.npc.type = NPCComponent::QuestGiver;
    e.npc.name = "Clerk";
    e.npc.dialog = "Ah, a message from the underworld! The Elysian Fields await your next delivery.";
    e.npc.hasLetterForPlayer = false;
  }

  // ---- Clerk delivery zone ----
  {
    auto h = em.createEntity();
    auto& e = em.get(h.index);
    e.hasTransform = true;
    e.transform.position = glm::vec3(18.0f, 1.0f, 0.0f);
    e.hasCollider = true;
    e.collider.shape = ColliderShape::AABB;
    e.collider.bounds = AABB::fromCenterAndHalfSize(glm::vec2(0), glm::vec2(1.5f, 1.5f));
    e.collider.layer = Layer_None;
    e.collider.isTrigger = true;
    e.hasDeliveryZone = true;
    e.deliveryZone.npcName = "Clerk";
    e.deliveryZone.expectedLetterId = "demo_pkg_b_to_c";
    e.deliveryZone.triggered = false;
    e.deliveryZone.triggerCooldown = 0.0f;
    e.deliveryZone.dialogOnSuccess = "Thanks! Demo run complete — that's both deliveries.";
    e.deliveryZone.dialogOnEmpty = "You need the parcel from the dock worker first.";
    e.deliveryZone.dialogOnWrong = "This letter is not addressed to me. Please check the recipient.";
  }

  // ---- Ground platform ----
  {
    auto h = em.createEntity();
    auto& e = em.get(h.index);
    e.hasTransform = true;
    e.transform.position = glm::vec3(10.0f, -0.5f, 0.0f);
    e.hasCollider = true;
    e.collider.shape = ColliderShape::AABB;
    e.collider.bounds = AABB::fromCenterAndHalfSize(glm::vec2(0), glm::vec2(15.0f, 0.5f));
    e.transform.scale = glm::vec3(30.0f, 1.0f, 1.0f);
    e.collider.layer = Layer_Platform;
    e.collider.mask = Layer_None;
    e.hasRender = true;
    e.render.color = glm::vec3(0.15f, 0.12f, 0.1f);  // dark stone
    e.render.use3DGeometry = true;
    e.render.sortOrder = 0;
  }

  // ---- Coins ----
  for (int i = 0; i < 8; i++) {
    auto h = em.createEntity();
    auto& e = em.get(h.index);
    e.hasTransform = true;
    float x = 4.0f + i * 2.0f;
    float y = 1.5f + (i % 2) * 0.5f;
    e.transform.position = glm::vec3(x, y, 0.0f);
    e.hasCollider = true;
    e.collider.shape = ColliderShape::AABB;
    e.collider.bounds = AABB::fromCenterAndHalfSize(glm::vec2(0), glm::vec2(0.2f, 0.2f));
    e.collider.layer = Layer_Pickup;
    e.collider.mask = Layer_Player;
    e.collider.isTrigger = true;
    e.hasRender = true;
    e.render.sortOrder = 15;
    e.render.spriteScale = 0.05f;
    e.render.usePixelArtSprite = true;
    e.hasPickup = true;
    e.pickup.type = PickupComponent::Coin;
    e.pickup.value = 1;
    e.pickup.bobTimer = (float)i * 0.5f;

    spriteAnimator.addState(h.index, "idle", { "coin", pickupSystem.getCoinAtlas(), 0.12f, true });
    spriteAnimator.play(h.index, "idle");
  }
}

// ---------------------------------------------------------------------------
// Demo level 1 — first parcel delivery
// ---------------------------------------------------------------------------
static void createDemoLevelOne(EntityManager& em, SpriteAnimator& spriteAnimator,
                           PickupSystem& pickupSystem) {
  // ---- Player ----
  {
    auto h = em.createEntity();
    auto& e = em.get(h.index);
    e.hasTransform = true;
    e.transform.position = glm::vec3(2.0f, 2.0f, 0.0f);
    e.transform.lastSafePosition = e.transform.position;
    e.hasPhysics = true;
    e.physics.gravityScale = 1.0f;
    e.physics.affectedByGravity = true;
    e.hasCollider = true;
    e.collider.shape = ColliderShape::AABB;
    e.collider.bounds = AABB::fromCenterAndHalfSize(glm::vec2(0), glm::vec2(0.3f, 0.5f));
    e.collider.layer = Layer_Player;
    e.collider.mask = Layer_Platform | Layer_Enemy | Layer_Pickup;
    e.hasRender = true;
    e.render.sortOrder = 10;
    e.render.spriteScale = 0.06f;  // 16px sprite = ~1 world unit
    e.render.usePixelArtSprite = true;
    e.render.spriteSourceJSON = exportSpriteDefToJSON(createHeroIdleSprite());
    e.hasInput = true;
    e.hasCamera = true;
    e.camera.followTarget = (int)h.index;
    e.camera.offset = glm::vec2(0.0f, 2.0f);
    e.camera.smoothSpeed = 8.0f;
    e.camera.bounds.enabled = false;
    
    e.hasMessenger = true; // Messenger God!
    e.messenger.stunCount = 0;
    e.messenger.stunTimer = 0.0f;

    e.hasHealth = true; // Keeping for mana/coins/keys for now
    e.health.current = 6;
    e.health.max = 6;
    e.health.mana = 10;
    e.health.maxMana = 10;
    e.health.coins = 0;
    
    e.hasAttack = true;
    e.attack.type = AttackComponent::Melee;
    e.attack.damage = 1.0f;
    e.attack.range = 0.8f;
    e.attack.cooldown = 0.3f;
    e.attack.duration = 0.15f;
    e.attack.facingDir = glm::vec2(1.0f, 0.0f);

    // Register sprite animations
    auto idleAtlas = createAtlasFromSpriteDef(createHeroIdleSprite());
    auto runAtlas = createAtlasFromSpriteDef(createHeroRunSprite());
    auto jumpAtlas = createAtlasFromSpriteDef(createHeroJumpSprite());
    auto attackAtlas = createAtlasFromSpriteDef(createHeroAttackSprite());

    spriteAnimator.addState(h.index, "idle", { "idle", idleAtlas, 0.5f, false });
    spriteAnimator.addState(h.index, "run", { "run", runAtlas, 0.1f, true });
    spriteAnimator.addState(h.index, "jump", { "jump", jumpAtlas, 0.5f, false });
    spriteAnimator.addState(h.index, "attack", { "attack", attackAtlas, 0.12f, false });
    spriteAnimator.play(h.index, "idle");
  }

  // ---- Vendor NPC ----
  {
    auto h = em.createEntity();
    auto& e = em.get(h.index);
    e.hasTransform = true;
    e.transform.position = glm::vec3(0.0f, 1.0f, 0.0f);
    e.hasRender = true;
    e.render.color = glm::vec3(0.75f, 0.45f, 0.95f);
    e.render.use3DGeometry = true;
    e.transform.scale = glm::vec3(0.8f, 1.5f, 0.8f);
    e.render.sortOrder = 5;
    e.hasCollider = true;
    e.collider.shape = ColliderShape::AABB;
    e.collider.bounds = AABB::fromCenterAndHalfSize(glm::vec2(0), glm::vec2(0.6f, 1.0f));
    e.collider.layer = Layer_None; // NPC trigger zone
    e.collider.isTrigger = true;
    e.hasNPC = true;
    e.npc.type = NPCComponent::QuestGiver;
    e.npc.name = "Vendor";
    e.npc.dialog = "Hi! Deliver this parcel to the foreman by the ferry dock.";
    e.npc.hasLetterForPlayer = true;
    e.npc.letterIdToGive = "demo_pkg_a_to_b";
    e.npc.letterGiven = false;
  }

  // ---- Letter Visual Indicator (pickup-style, but given by NPC) ----
  // Visual cue above vendor
  {
    auto h = em.createEntity();
    auto& e = em.get(h.index);
    e.hasTransform = true;
    e.transform.position = glm::vec3(0.0f, 1.8f, 0.0f);
    e.hasRender = true;
    e.render.sortOrder = 15;
    e.render.spriteScale = 0.05f;
    e.render.usePixelArtSprite = true;
    e.hasPickup = true;
    // This is just visual - the actual letter is given by the NPC component
    e.pickup.type = PickupComponent::Letter;
    e.pickup.bobTimer = 0.0f;

    spriteAnimator.addState(h.index, "idle", { "key", pickupSystem.getKeyAtlas(), 0.5f, false });
    spriteAnimator.play(h.index, "idle");
  }

  // ---- Dock foreman (delivery target) ----
  {
    auto h = em.createEntity();
    auto& e = em.get(h.index);
    e.hasTransform = true;
    e.transform.position = glm::vec3(18.0f, 1.0f, 0.0f);
    e.hasRender = true;
    e.render.color = glm::vec3(0.15f, 0.20f, 0.35f); // Deep nautical blue for Ferryman
    e.render.use3DGeometry = true;
    e.transform.scale = glm::vec3(0.7f, 1.4f, 0.7f);
    e.render.sortOrder = 5;
    e.hasCollider = true;
    e.collider.shape = ColliderShape::AABB;
    e.collider.bounds = AABB::fromCenterAndHalfSize(glm::vec2(0), glm::vec2(0.6f, 1.0f));
    e.collider.layer = Layer_None;
    e.collider.isTrigger = true;
    e.hasNPC = true;
    e.npc.type = NPCComponent::QuestGiver;
    e.npc.name = "Foreman";
    e.npc.dialog = "I'm waiting for the vendor's parcel before the ferry can leave.";
    e.npc.hasLetterForPlayer = false;
  }

  // ---- Foreman delivery zone ----
  {
    auto h = em.createEntity();
    auto& e = em.get(h.index);
    e.hasTransform = true;
    e.transform.position = glm::vec3(18.0f, 1.0f, 0.0f);
    e.hasCollider = true;
    e.collider.shape = ColliderShape::AABB;
    e.collider.bounds = AABB::fromCenterAndHalfSize(glm::vec2(0), glm::vec2(1.5f, 1.5f));
    e.collider.layer = Layer_None;
    e.collider.isTrigger = true;
    e.hasDeliveryZone = true;
    e.deliveryZone.npcName = "Foreman";
    e.deliveryZone.expectedLetterId = "demo_pkg_a_to_b";
    e.deliveryZone.triggered = false;
    e.deliveryZone.triggerCooldown = 0.0f;
    e.deliveryZone.dialogOnSuccess = "That's the one! Ferry's clear — head to level two when you're ready.";
    e.deliveryZone.dialogOnEmpty = "No parcel yet. Talk to the vendor first.";
    e.deliveryZone.dialogOnWrong = "Wrong parcel — I need the one from the vendor.";
  }

  // ---- Player animation state machine ----
  // (handled by a simple system in the game loop below)

  // ---- Ground platform (obsidian) ----
  {
    auto h = em.createEntity();
    auto& e = em.get(h.index);
    e.hasTransform = true;
    e.transform.position = glm::vec3(5.0f, -0.5f, 0.0f);
    e.hasCollider = true;
    e.collider.shape = ColliderShape::AABB;
    e.collider.bounds = AABB::fromCenterAndHalfSize(glm::vec2(0), glm::vec2(10.0f, 0.5f));
    e.transform.scale = glm::vec3(20.0f, 1.0f, 1.0f); // Match collider (2 * halfSize)
    e.collider.layer = Layer_Platform;
    e.collider.mask = Layer_None;
    e.hasRender = true;
    e.render.color = glm::vec3(0.15f, 0.12f, 0.18f);  // dark obsidian with blue tint
    e.render.use3DGeometry = true;
    e.render.sortOrder = 0;
  }

  // ---- Floating ice platforms ----
  for (int i = 0; i < 5; i++) {
    auto h = em.createEntity();
    auto& e = em.get(h.index);
    e.hasTransform = true;
    float x = 4.0f + i * 3.5f;
    float y = 1.0f + (i % 2) * 1.0f;
    e.transform.position = glm::vec3(x, y, 0.0f);
    e.hasCollider = true;
    e.collider.shape = ColliderShape::AABB;
    e.collider.bounds = AABB::fromCenterAndHalfSize(glm::vec2(0), glm::vec2(1.5f, 0.2f));
    e.transform.scale = glm::vec3(3.0f, 0.4f, 1.0f); // 2 * halfSize
    e.collider.layer = Layer_Platform;
    e.collider.mask = Layer_None;
    e.hasRender = true;
    // Vary ice tones
    float tint = 0.75f - (i * 0.08f);
    e.render.color = glm::vec3(tint * 0.85f, tint * 0.92f, tint);  // frosty blue-white
    e.render.use3DGeometry = true;
    e.render.sortOrder = 0;
  }

  // ---- Moving crystal platform ----
  {
    auto h = em.createEntity();
    auto& e = em.get(h.index);
    e.hasTransform = true;
    e.transform.position = glm::vec3(12.0f, 2.0f, 0.0f);
    e.hasCollider = true;
    e.collider.shape = ColliderShape::AABB;
    e.collider.bounds = AABB::fromCenterAndHalfSize(glm::vec2(0), glm::vec2(1.5f, 0.2f));
    e.transform.scale = glm::vec3(3.0f, 0.4f, 1.0f); // 2 * halfSize
    e.collider.layer = Layer_Platform;
    e.collider.mask = Layer_None;
    e.hasRender = true;
    e.render.color = glm::vec3(0.35f, 0.50f, 0.65f);  // translucent ice crystal
    e.render.use3DGeometry = true;
    e.render.sortOrder = 0;
    e.hasPlatform = true;
    e.platform.moveType = PlatformComponent::Moving;
    e.platform.path = {glm::vec2(12.0f, 2.0f), glm::vec2(16.0f, 2.0f), glm::vec2(16.0f, 4.0f), glm::vec2(12.0f, 4.0f)};
    e.platform.speed = 1.5f;
  }

  // ---- Enemies ----
  // Blob enemy 1
  {
    auto h = em.createEntity();
    auto& e = em.get(h.index);
    e.hasTransform = true;
    e.transform.position = glm::vec3(8.0f, 1.0f, 0.0f);
    e.hasPhysics = true;
    e.physics.gravityScale = 1.0f;
    e.physics.affectedByGravity = true;
    e.hasCollider = true;
    e.collider.shape = ColliderShape::AABB;
    e.collider.bounds = AABB::fromCenterAndHalfSize(glm::vec2(0), glm::vec2(0.25f, 0.35f));
    e.collider.layer = Layer_Enemy;
    e.collider.mask = Layer_Platform | Layer_Player;
    e.hasRender = true;
    e.render.sortOrder = 10;
    e.render.spriteScale = 0.06f;
    e.render.usePixelArtSprite = true;
    e.render.spriteSourceJSON = exportSpriteDefToJSON(createBlobEnemySprite());
    e.render.flipX = false;
    e.hasHealth = true;
    e.health.current = 2;
    e.health.max = 2;
    e.hasEnemyAI = true;
    e.enemyAI.state = EnemyAIComponent::Patrol;
    e.enemyAI.patrolStart = 6.0f;
    e.enemyAI.patrolEnd = 10.0f;
    e.enemyAI.speed = 1.5f;
    e.enemyAI.facingRight = true;

    auto blobAtlas = createAtlasFromSpriteDef(createBlobEnemySprite());
    spriteAnimator.addState(h.index, "idle", { "idle", blobAtlas, 0.5f, false });
    spriteAnimator.play(h.index, "idle");
  }

  // Shield Guard
  {
    auto h = em.createEntity();
    auto& e = em.get(h.index);
    e.hasTransform = true;
    e.transform.position = glm::vec3(14.0f, 1.0f, 0.0f);
    e.hasPhysics = true;
    e.physics.gravityScale = 1.0f;
    e.physics.affectedByGravity = true;
    e.hasCollider = true;
    e.collider.shape = ColliderShape::AABB;
    e.collider.bounds = AABB::fromCenterAndHalfSize(glm::vec2(0), glm::vec2(0.3f, 0.4f));
    e.collider.layer = Layer_Enemy;
    e.collider.mask = Layer_Platform | Layer_Player;
    e.hasRender = true;
    e.render.sortOrder = 10;
    e.render.spriteScale = 0.06f;
    e.render.usePixelArtSprite = true;
    e.render.spriteSourceJSON = exportSpriteDefToJSON(createShieldGuardSprite());
    e.hasHealth = true;
    e.health.current = 4;
    e.health.max = 4;
    e.hasEnemyAI = true;
    e.enemyAI.state = EnemyAIComponent::Patrol;
    e.enemyAI.patrolStart = 12.0f;
    e.enemyAI.patrolEnd = 16.0f;
    e.enemyAI.speed = 1.0f;
    e.enemyAI.facingRight = false;

    auto guardAtlas = createAtlasFromSpriteDef(createShieldGuardSprite());
    spriteAnimator.addState(h.index, "idle", { "idle", guardAtlas, 0.5f, false });
    spriteAnimator.play(h.index, "idle");
  }

  // Fire Wisp (floating, no gravity)
  {
    auto h = em.createEntity();
    auto& e = em.get(h.index);
    e.hasTransform = true;
    e.transform.position = glm::vec3(10.0f, 3.5f, 0.0f);
    e.hasCollider = true;
    e.collider.shape = ColliderShape::AABB;
    e.collider.bounds = AABB::fromCenterAndHalfSize(glm::vec2(0), glm::vec2(0.3f, 0.3f));
    e.collider.layer = Layer_Enemy;
    e.collider.mask = Layer_Player;
    e.hasRender = true;
    e.render.sortOrder = 10;
    e.render.spriteScale = 0.06f;
    e.render.usePixelArtSprite = true;
    e.render.spriteSourceJSON = exportSpriteDefToJSON(createFireWispSprite());
    e.hasHealth = true;
    e.health.current = 1;
    e.health.max = 1;

    auto wispAtlas = createAtlasFromSpriteDef(createFireWispSprite());
    spriteAnimator.addState(h.index, "idle", { "idle", wispAtlas, 0.2f, true });
    spriteAnimator.play(h.index, "idle");
  }

  // Chaser enemy
  {
    auto h = em.createEntity();
    auto& e = em.get(h.index);
    e.hasTransform = true;
    e.transform.position = glm::vec3(18.0f, 1.0f, 0.0f);
    e.hasPhysics = true;
    e.physics.gravityScale = 1.0f;
    e.physics.affectedByGravity = true;
    e.hasCollider = true;
    e.collider.shape = ColliderShape::AABB;
    e.collider.bounds = AABB::fromCenterAndHalfSize(glm::vec2(0), glm::vec2(0.35f, 0.3f));
    e.collider.layer = Layer_Enemy;
    e.collider.mask = Layer_Platform | Layer_Player;
    e.hasRender = true;
    e.render.sortOrder = 10;
    e.render.spriteScale = 0.06f;
    e.render.usePixelArtSprite = true;
    e.render.spriteSourceJSON = exportSpriteDefToJSON(createChaserSprite());
    e.hasHealth = true;
    e.health.current = 3;
    e.health.max = 3;
    e.hasEnemyAI = true;
    e.enemyAI.state = EnemyAIComponent::Patrol;
    e.enemyAI.patrolStart = 16.0f;
    e.enemyAI.patrolEnd = 20.0f;
    e.enemyAI.speed = 3.0f;
    e.enemyAI.facingRight = false;

    auto chaserAtlas = createAtlasFromSpriteDef(createChaserSprite());
    spriteAnimator.addState(h.index, "idle", { "idle", chaserAtlas, 0.5f, false });
    spriteAnimator.play(h.index, "idle");
  }

  // ---- Gatekeeper Boss ----
  {
    auto h = em.createEntity();
    auto& e = em.get(h.index);
    e.hasTransform = true;
    e.transform.position = glm::vec3(22.0f, 0.5f, 0.0f);
    e.hasCollider = true;
    e.collider.shape = ColliderShape::AABB;
    e.collider.bounds = AABB::fromCenterAndHalfSize(glm::vec2(0), glm::vec2(0.8f, 1.0f));
    e.collider.layer = Layer_Enemy;
    e.collider.mask = Layer_Player;
    e.hasRender = true;
    e.render.sortOrder = 10;
    e.render.spriteScale = 0.06f;
    e.render.usePixelArtSprite = true;
    e.render.spriteSourceJSON = exportSpriteDefToJSON(createBossGatekeeperSprite());
    e.hasHealth = true;
    e.health.current = 12;
    e.health.max = 12;

    auto bossAtlas = createAtlasFromSpriteDef(createBossGatekeeperSprite());
    spriteAnimator.addState(h.index, "idle", { "idle", bossAtlas, 0.5f, false });
    spriteAnimator.play(h.index, "idle");
  }

  // ---- Pickups: Coins ----
  for (int i = 0; i < 10; i++) {
    auto h = em.createEntity();
    auto& e = em.get(h.index);
    e.hasTransform = true;
    float x = 3.0f + i * 2.2f;
    float y = 1.5f + (i % 3) * 0.5f;
    e.transform.position = glm::vec3(x, y, 0.0f);
    e.hasCollider = true;
    e.collider.shape = ColliderShape::AABB;
    e.collider.bounds = AABB::fromCenterAndHalfSize(glm::vec2(0), glm::vec2(0.2f, 0.2f));
    e.collider.layer = Layer_Pickup;
    e.collider.mask = Layer_Player;
    e.collider.isTrigger = true;
    e.hasRender = true;
    e.render.sortOrder = 15;
    e.render.spriteScale = 0.05f;
    e.render.usePixelArtSprite = true;
    e.hasPickup = true;
    e.pickup.type = PickupComponent::Coin;
    e.pickup.value = 1;
    e.pickup.bobTimer = (float)i * 0.5f;

    spriteAnimator.addState(h.index, "idle", { "coin", pickupSystem.getCoinAtlas(), 0.12f, true });
    spriteAnimator.play(h.index, "idle");
  }

  // ---- Heart pickups ----
  for (int i = 0; i < 3; i++) {
    auto h = em.createEntity();
    auto& e = em.get(h.index);
    e.hasTransform = true;
    float x = 6.0f + i * 6.0f;
    float y = 3.0f;
    e.transform.position = glm::vec3(x, y, 0.0f);
    e.hasCollider = true;
    e.collider.shape = ColliderShape::AABB;
    e.collider.bounds = AABB::fromCenterAndHalfSize(glm::vec2(0), glm::vec2(0.2f, 0.2f));
    e.collider.layer = Layer_Pickup;
    e.collider.mask = Layer_Player;
    e.collider.isTrigger = true;
    e.hasRender = true;
    e.render.sortOrder = 15;
    e.render.spriteScale = 0.05f;
    e.render.usePixelArtSprite = true;
    e.hasPickup = true;
    e.pickup.type = PickupComponent::Heart;
    e.pickup.value = 1;
    e.pickup.bobTimer = (float)i * 0.7f;

    spriteAnimator.addState(h.index, "idle", { "heart", pickupSystem.getHeartAtlas(), 0.5f, false });
    spriteAnimator.play(h.index, "idle");
  }

  // ---- Mana Orb pickups ----
  for (int i = 0; i < 2; i++) {
    auto h = em.createEntity();
    auto& e = em.get(h.index);
    e.hasTransform = true;
    float x = 9.0f + i * 8.0f;
    float y = 3.5f;
    e.transform.position = glm::vec3(x, y, 0.0f);
    e.hasCollider = true;
    e.collider.shape = ColliderShape::AABB;
    e.collider.bounds = AABB::fromCenterAndHalfSize(glm::vec2(0), glm::vec2(0.2f, 0.2f));
    e.collider.layer = Layer_Pickup;
    e.collider.mask = Layer_Player;
    e.collider.isTrigger = true;
    e.hasRender = true;
    e.render.sortOrder = 15;
    e.render.spriteScale = 0.05f;
    e.render.usePixelArtSprite = true;
    e.hasPickup = true;
    e.pickup.type = PickupComponent::Mana;
    e.pickup.value = 3;
    e.pickup.bobTimer = (float)i * 0.9f;

    spriteAnimator.addState(h.index, "idle", { "mana", pickupSystem.getManaOrbAtlas(), 0.3f, true });
    spriteAnimator.play(h.index, "idle");
  }
}

// ---------------------------------------------------------------------------
// PlayerAnimController — syncs input/physics state to sprite animations.
// ---------------------------------------------------------------------------
class PlayerAnimController : public System {
 public:
  PlayerAnimController(SpriteAnimator* animator) : animator(animator) {}

  void lateUpdate(EntityManager& em, float deltaTime) override {
    (void)deltaTime;
    auto inputIndices = em.queryWithInput();
    for (size_t idx : inputIndices) {
      auto& e = em.get(idx);

      // Flip sprite based on movement direction
      if (e.hasRender) {
        if (e.input.moveAxis > 0.01f) e.render.flipX = false;
        else if (e.input.moveAxis < -0.01f) e.render.flipX = true;
      }

      // Update facing direction for attacks
      if (e.hasAttack) {
        if (e.render.flipX) e.attack.facingDir = glm::vec2(-1.0f, 0.0f);
        else e.attack.facingDir = glm::vec2(1.0f, 0.0f);
      }

      // Animation state machine
      if (e.hasAttack && e.attack.attacking) {
        animator->play(idx, "attack");
      } else if (!e.physics.onGround) {
        animator->play(idx, "jump");
      } else if (std::abs(e.physics.velocity.x) > 0.5f) {
        animator->play(idx, "run");
      } else {
        animator->play(idx, "idle");
      }
    }
  }

 private:
  SpriteAnimator* animator;
};

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main(int argc, char* argv[]) {
  bool headless = false;
#ifdef WHISKERS_DEBUG_PROTOCOL
  bool runDebugProtocol = false;
#endif
  for (int i = 1; i < argc; ++i) {
    std::string a(argv[i]);
    if (a == "--headless") headless = true;
#ifdef WHISKERS_DEBUG_PROTOCOL
    if (a == "--debug-protocol") runDebugProtocol = true;
#endif
  }

  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    std::cerr << "SDL_Init error: " << SDL_GetError() << "\n";
    return 1;
  }

#ifdef WHISKERS_WASM_BUILD
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#else
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif

  Uint32 winFlags =
      SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
  if (headless) winFlags |= SDL_WINDOW_HIDDEN;

  SDL_Window* window = SDL_CreateWindow(
    "Whiskers — Platformer Demo",
    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
    1280, 720,
    winFlags
  );

  if (!window) {
    std::cerr << "SDL_CreateWindow error: " << SDL_GetError() << "\n";
    SDL_Quit();
    return 1;
  }

  SDL_GLContext context = SDL_GL_CreateContext(window);
  if (!context) {
    std::cerr << "SDL_GL_CreateContext error: " << SDL_GetError() << "\n";
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

#ifndef WHISKERS_WASM_BUILD
  if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
    std::cerr << "Failed to initialize GLAD\n";
    return 1;
  }
#endif

  int width = 1280, height = 720;
  SDL_GetWindowSize(window, &width, &height);
  int drawW = width, drawH = height;
  SDL_GL_GetDrawableSize(window, &drawW, &drawH);

  auto renderer = std::make_unique<Renderer>(drawW, drawH);
  if (!renderer->init()) {
    std::cerr << "Failed to initialize renderer\n";
    return 1;
  }

  EntityManager entityManager;
  GameLoop gameLoop;

#ifdef WHISKERS_DEBUG_PROTOCOL
  Level debugLevel;
  DebugProtocol debugProtocol;
  DebugProtocolHandler debugHandler;
  DebugInputBridge debugInputBridge;
#endif

  Editor editor;
  if (!editor.init(window, context)) {
    std::cerr << "Failed to initialize editor\n";
  }
  editor.setEnabled(false); // Start disabled
  editor.setCamera(&renderer->getCamera());

  int mouseX = 0, mouseY = 0;

  // Create systems
  auto spriteAnimator = std::make_unique<SpriteAnimator>();
  auto pickupSystem = std::make_unique<PickupSystem>();

  gameLoop.onEvent = [&](const SDL_Event& event) {
    editor.processEvent(event);
    if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_F1) {
      editor.setEnabled(!editor.isEnabled());
    }
    if (event.type == SDL_MOUSEMOTION) {
      mouseX = event.motion.x;
      mouseY = event.motion.y;
    }
    if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
      mouseX = event.button.x;
      mouseY = event.button.y;
    }
    if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED) {
      width = event.window.data1;
      height = event.window.data2;
      SDL_GL_GetDrawableSize(window, &drawW, &drawH);
      renderer->setViewportSize(drawW, drawH);
    }
  };
  gameLoop.onRender = [&](EntityManager& em) {
    // Viewport click-to-select
    if (editor.isEnabled()) {
      editor.updateMouseState(mouseX, mouseY, width, height);
      int picked = editor.processViewportClick(window, em);
      if (picked >= 0) {
        editor.setSelectedEntityIndex(picked);
      }
    }
    editor.render(window, em, *spriteAnimator);
  };

  PlatformerInputSystem inputSystem(&gameLoop);
  PlayerMovementSystem movementSystem;
  PhysicsSystem physicsSystem;
  HazardSystem hazardSystem;
  CombatSystem combatSystem(spriteAnimator.get());
  MovingPlatformSystem movingPlatformSystem;
  EnemyMovementSystem enemyMovementSystem;
  PlayerAnimController animController(spriteAnimator.get());
  SpriteRenderSystem spriteRenderSystem(spriteAnimator.get());
  CameraSystem& cameraSystem = renderer->getCamera();
  ParallaxRenderSystem parallaxSystem;
  DeliveryZoneSystem deliveryZoneSystem;
  NPCInteractionSystem npcInteractionSystem;
  LevelTransitionSystem levelTransitionSystem(&gameLoop);

#ifdef WHISKERS_DEBUG_PROTOCOL
  if (runDebugProtocol) {
    setvbuf(stdout, nullptr, _IOLBF, 0);
    debugHandler.setContext(&entityManager, &physicsSystem, &debugLevel, renderer.get(), &gameLoop,
                            &debugInputBridge);
    inputSystem.setDebugBridge(&debugInputBridge);
    gameLoop.onBeforeFrame = [&](EntityManager&) {
      std::vector<std::string> lines;
      debugProtocol.drainIncoming(lines);
      debugHandler.processLines(lines);
    };
    gameLoop.onFixedStepStart = [&](EntityManager& em, float /*dt*/) {
      debugHandler.onFixedStepBegin(em);
    };
    gameLoop.onAfterSceneRender = [&](EntityManager& em) {
      debugHandler.onAfterRender(em);
    };
    debugProtocol.start();
  }
#endif

  // Register systems
  gameLoop.addSystem(&inputSystem);
  gameLoop.addSystem(&movementSystem);
  gameLoop.addSystem(&physicsSystem);
  gameLoop.addSystem(&hazardSystem);
  gameLoop.addSystem(&combatSystem);
  gameLoop.addSystem(pickupSystem.get());
  gameLoop.addSystem(&movingPlatformSystem);
  gameLoop.addSystem(&enemyMovementSystem);
  gameLoop.addSystem(&npcInteractionSystem);
  gameLoop.addSystem(&deliveryZoneSystem);
  gameLoop.addSystem(&levelTransitionSystem);
  gameLoop.addSystem(&animController);
  gameLoop.addSystem(&spriteRenderSystem);
  gameLoop.addSystem(&cameraSystem);
  gameLoop.addSystem(&parallaxSystem);

  // Load demo level 1
  createDemoLevelOne(entityManager, *spriteAnimator, *pickupSystem);

#ifdef WHISKERS_DEBUG_PROTOCOL
  const bool jsonOnlyStdout = runDebugProtocol;
#else
  const bool jsonOnlyStdout = false;
#endif
  if (!jsonOnlyStdout) {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════╗\n";
    std::cout << "║     Whiskers Engine — Platformer Demo (Lv 1)  ║\n";
    std::cout << "╠══════════════════════════════════════════════════╣\n";
    std::cout << "║  A/D or ←/→   Move                             ║\n";
    std::cout << "║  Space/W/↑    Jump                             ║\n";
    std::cout << "║  X or J       Melee                            ║\n";
    std::cout << "║  C or K       Fireball (costs 2 mana)          ║\n";
    std::cout << "║  R            Restart      F1  Editor         ║\n";
    std::cout << "╠══════════════════════════════════════════════════╣\n";
    std::cout << "║  Goal: Get parcel from vendor → foreman →     ║\n";
    std::cout << "║        unlock level 2                          ║\n";
    std::cout << "╚══════════════════════════════════════════════════╝\n\n";

    std::cout << "Starting — " << entityManager.aliveCount() << " entities\n";
  }

  int result = 0;
  bool quit = false;
  while (!quit) {
    result = gameLoop.run(window, renderer.get(), entityManager);
    
    if (inputSystem.restartRequested) {
      std::cout << "Restarting level...\n";
      inputSystem.restartRequested = false;
      entityManager.clear();
      spriteAnimator->clear();
      pickupSystem->clear();
      combatSystem.clear();
      levelTransitionSystem.reset();
      
      // Restart current level
      if (currentLevel == GameLevel::Level1) {
        createDemoLevelOne(entityManager, *spriteAnimator, *pickupSystem);
      } else {
        createDemoLevelTwo(entityManager, *spriteAnimator, *pickupSystem);
      }
      gameLoop.running = true;  // keep running
    } else if (levelTransitionSystem.transitionRequested) {
      std::cout << "Level complete! Advancing to next level...\n";
      currentLevel = levelTransitionSystem.nextLevel;
      levelTransitionSystem.reset();
      entityManager.clear();
      spriteAnimator->clear();
      pickupSystem->clear();
      combatSystem.clear();
      
      // Carry over player state (divine favor, coins, etc.)
      // In a full implementation, we'd preserve the player entity
      
      // Load next level
      if (currentLevel == GameLevel::Level1) {
        createDemoLevelOne(entityManager, *spriteAnimator, *pickupSystem);
      } else if (currentLevel == GameLevel::Level2) {
        std::cout << "\n"
                  << "╔══════════════════════════════════════════════════╗\n"
                  << "║     Whiskers — Demo Level 2 (moving platforms)  ║\n"
                  << "╠══════════════════════════════════════════════════╣\n"
                  << "║  A/D or ←/→   Move                             ║\n"
                  << "║  Space/W/↑    Jump                             ║\n"
                  << "║  X or J       Melee                            ║\n"
                  << "║  C or K       Fireball (costs 2 mana)          ║\n"
                  << "║  R  Restart   F1  Editor                       ║\n"
                  << "╠══════════════════════════════════════════════════╣\n"
                  << "║  Goal: Carry dock parcel to the clerk          ║\n"
                  << "║        past the moving platforms                ║\n"
                  << "╚══════════════════════════════════════════════════╝\n\n" << std::endl;
        createDemoLevelTwo(entityManager, *spriteAnimator, *pickupSystem);
      }
      gameLoop.running = true;  // keep running
    } else {
      quit = true;
    }
  }

#ifdef WHISKERS_DEBUG_PROTOCOL
  if (runDebugProtocol) {
    debugProtocol.shutdown();
  }
#endif

  editor.shutdown();

#ifndef WHISKERS_WASM_BUILD
  SDL_GL_DeleteContext(context);
#endif
  SDL_DestroyWindow(window);
  SDL_Quit();

  return result;
}
