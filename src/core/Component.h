// Component.h
// Plain data components for the ECS architecture.
#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <cstdint>

// ---------------------------------------------------------------------------
// Collision shape types
// ---------------------------------------------------------------------------
enum class ColliderShape { AABB, Circle };

struct AABB {
  glm::vec2 min{0.0f, 0.0f};
  glm::vec2 max{0.0f, 0.0f};

  AABB() = default;
  AABB(glm::vec2 mn, glm::vec2 mx) : min(mn), max(mx) {}

  // Build an AABB centered at `center` with half-extents `halfSize`.
  static AABB fromCenterAndHalfSize(glm::vec2 center, glm::vec2 halfSize) {
    return {center - halfSize, center + halfSize};
  }

  glm::vec2 center() const { return (min + max) * 0.5f; }
  glm::vec2 halfSize() const { return (max - min) * 0.5f; }
};

inline bool aabbOverlap(const AABB& a, const AABB& b) {
  return a.min.x <= b.max.x && a.max.x >= b.min.x && a.min.y <= b.max.y && a.max.y >= b.min.y;
}

// ---------------------------------------------------------------------------
// Collision layer bitmask — extend as needed.
// ---------------------------------------------------------------------------
enum CollisionLayer : uint8_t {
  Layer_None    = 0,
  Layer_Player  = 1 << 0,
  Layer_Enemy   = 1 << 1,
  Layer_Platform = 1 << 2,
  Layer_Pickup  = 1 << 3,
  Layer_Projectile = 1 << 4,
  Layer_Trigger = 1 << 5,
};

// ---------------------------------------------------------------------------
// Components
// ---------------------------------------------------------------------------

struct TransformComponent {
  glm::vec3 position{0.0f, 0.0f, 0.0f};
  glm::vec3 rotation{0.0f, 0.0f, 0.0f};  // degrees, only Z is used for 2D
  glm::vec3 scale{1.0f, 1.0f, 1.0f};
  glm::vec3 lastSafePosition{0.0f, 0.0f, 0.0f};
};

struct PhysicsComponent {
  glm::vec2 velocity{0.0f, 0.0f};
  glm::vec2 acceleration{0.0f, 0.0f};
  bool onGround{false};
  float gravityScale{1.0f};    // 0 = no gravity (e.g. flying enemies)
  float mass{1.0f};
  bool affectedByGravity{true};
};

struct ColliderComponent {
  ColliderShape shape{ColliderShape::AABB};
  AABB bounds{glm::vec2(0), glm::vec2(0)};  // local-space bounds
  bool isTrigger{false};
  uint8_t layer{Layer_Platform};
  uint8_t mask{0xFF};  // bitmask of layers this collider interacts with
};

struct RenderComponent {
  // For the platformer these are indices into texture atlases / mesh pools.
  // In Phase 1 we keep it simple — a color and an optional texture path.
  glm::vec3 color{1.0f, 1.0f, 1.0f};
  std::string texturePath;
  int sortOrder{0};       // higher = drawn on top
  bool visible{true};
  bool use3DGeometry{false};  // when true, render as a 3D box instead of flat sprite

  // Pixel art sprite rendering
  uint32_t spriteAtlas{0};     // OpenGL texture ID (atlas strip)
  int spriteFrameWidth{0};
  int spriteFrameHeight{0};
  int spriteFrameCount{0};
  int spriteCurrentFrame{0};
  float spriteScale{1.0f};     // world units per sprite pixel (e.g., 0.05 = 20px per world unit)
  bool usePixelArtSprite{false};
  bool flipX{false};           // horizontal flip for facing direction
  std::string spriteSourceJSON; // Raw pixel data and palette (JSON format)
};

struct SpriteAnimComponent {
  std::vector<std::string> frames;  // texture file paths per frame
  float frameDuration{0.1f};
  float currentTime{0.0f};
  int currentFrame{0};
  bool loop{true};
  bool playing{false};
};

struct HealthComponent {
  int current{3};
  int max{3};
  float invincibleTimer{0.0f};

  // Extra resources (coins, mana, etc.)
  int mana{10};
  int maxMana{10};
  float manaRegenTimer{0.0f};
  float manaRegenRate{1.0f};  // mana per second

  int coins{0};
  int keys{0};
};

struct PlayerInputComponent {
  float moveAxis{0.0f};     // -1 (left) .. +1 (right)
  bool jumpHeld{false};
  bool jumpPressed{false};  // true on the frame jump is pressed
  bool attackPressed{false};  // melee
  bool firePressed{false};    // ranged (fireball)
  bool crouchHeld{false};
};

struct CameraComponent {
  int followTarget{-1};       // entity index, -1 = no target
  glm::vec2 offset{0.0f, 2.0f};
  float smoothSpeed{8.0f};
  // Camera bounds in world space (0 = unbounded)
  struct Bounds {
    float left{0}, right{0}, bottom{0}, top{0};
    bool enabled{false};
  } bounds;
};

struct PlatformComponent {
  enum MoveType { Static, Moving, Falling, Conveyor };
  MoveType moveType{Static};
  std::vector<glm::vec2> path;  // waypoints for moving platforms
  float speed{1.0f};
  float pathProgress{0.0f};     // 0..1 between current segment waypoints
  int currentWaypoint{0};
  glm::vec2 lastPosition{0.0f, 0.0f};  // used to carry standing entities
};

struct EnemyAIComponent {
  enum State { Patrol, Chase, Stunned, Dead };
  State state{Patrol};
  float patrolStart{0.0f};
  float patrolEnd{0.0f};
  float detectionRange{5.0f};
  float speed{2.0f};
  bool facingRight{true};
  int health{2};
  int maxHealth{2};
  float stunTimer{0.0f};
  float attackCooldown{0.0f};
};

// Attack state for melee/ranged combat.
struct AttackComponent {
  enum AttackType { Melee, Ranged };
  AttackType type{Melee};
  float damage{1.0f};
  float range{0.8f};       // melee range in world units
  float cooldown{0.3f};    // time between attacks
  float cooldownTimer{0.0f};
  float duration{0.15f};   // how long the hitbox is active
  float durationTimer{0.0f};
  bool attacking{false};
  int comboStep{0};        // 0, 1, 2 for 3-hit combo
  int maxCombo{3};
  glm::vec2 facingDir{1.0f, 0.0f};
};

// Projectile component (fireballs, enemy projectiles).
struct ProjectileComponent {
  float damage{1.0f};
  float speed{8.0f};
  float lifetime{2.0f};
  float lifetimeTimer{0.0f};
  glm::vec2 direction{1.0f, 0.0f};
  bool isPlayerOwned{true};
};

// Pickup component (coins, hearts, mana, letters).
struct PickupComponent {
  enum PickupType { Coin, Heart, Mana, Key, Letter };
  PickupType type{Coin};
  int value{1};
  float bobTimer{0.0f};
  float bobAmount{0.1f};
  bool collected{false};
  std::string letterId;  // for PickupType::Letter
};

struct LetterComponent {
  std::string id;
  std::string sender;
  std::string recipient;
  bool delivered{false};
};

struct MessengerComponent {
  std::vector<std::string> heldLetters;
  float stunTimer{0.0f};
  int stunCount{0};
  int divineFavor{0};  // Earned by successful deliveries
};

// Delivery Zone component - triggers when player with correct letter enters
struct DeliveryZoneComponent {
  std::string npcName;             // NPC display name for dialogs
  std::string expectedLetterId;   // The letter ID this zone accepts
  bool triggered{false};          // Has delivery been completed?
  bool triggeredLastFrame{false}; // For detecting edge transitions
  float triggerCooldown{0.0f};    // Prevent re-triggering
  std::string dialogOnSuccess;    // What the NPC says when letter is delivered
  std::string dialogOnEmpty;      // What the NPC says when player has no letter
  std::string dialogOnWrong;      // What the NPC says when player has wrong letter
};

// NPC component for quest-giving characters
struct NPCComponent {
  enum NPCType { QuestGiver, Shopkeeper, Neutral };
  NPCType type{NPCType::Neutral};
  std::string name;
  std::string dialog;
  bool hasLetterForPlayer{false};
  std::string letterIdToGive;     // Letter this NPC gives to player
  bool letterGiven{false};
};

// Health component (was already defined, adding mana field here).
// We add mana to HealthComponent for simplicity.

// ============================================================
// Magicor-Specific Components
// ============================================================

// Player component for Magicor
struct MagicorPlayerComponent {
    // Movement
    bool canDoubleJump{true};
    bool hasDoubleJumped{false};
    bool isWallSliding{false};
    bool facingRight{true};
    
    // Wall jump
    float wallJumpCooldown{0.0f};
    
    // Coyote time - allows jumping shortly after leaving a platform
    float coyoteTime{0.0f};
    static constexpr float maxCoyoteTime{0.15f};
    
    // Jump charging
    float jumpCharge{0.0f};
    static constexpr float maxJumpCharge{1.0f};
    static constexpr float jumpChargeRate{2.0f};
    
    // Stats
    int score{0};
    int lives{3};
    
    // State
    bool isDead{false};
    float deathTimer{0.0f};
    float respawnTimer{0.0f};
    glm::vec2 respawnPosition{0.0f, 0.0f};
};

// Orb component
struct OrbComponent {
    enum Type { Normal, Concentrated, Magic };
    Type type{Magic};
    int points{500};
    bool collected{false};
    float bobOffset{0.0f};
    float bobSpeed{1.0f};
    float bobAmount{0.2f};
};

// Hazard component
struct HazardComponent {
    enum Type { Spikes, Lava, Fire, Pit };
    Type type{Spikes};
    float damage{1.0f};
};

// Enemy component for Magicor
struct MagicorEnemyComponent {
    enum AIType { Patrol, Climb, Turret, Seeker };
    AIType aiType{Patrol};
    float patrolStart{0.0f};
    float patrolEnd{0.0f};
    float speed{2.0f};
    int health{1};
    float attackCooldown{0.0f};
    float attackRange{100.0f};
    bool facingRight{true};
};

// Particle component
struct ParticleComponent {
    glm::vec2 velocity{0.0f, 0.0f};
    glm::vec2 acceleration{0.0f, 0.0f};
    float lifetime{1.0f};
    float age{0.0f};
    bool fadeOut{true};
};
