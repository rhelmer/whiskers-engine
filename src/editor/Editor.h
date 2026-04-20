// Editor.h
// Level and character editor overlay using Dear ImGui.
#pragma once

#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "../core/EntityManager.h"
#include "PixelEditor.h"

class CameraSystem;
class SpriteAnimator;

// Entity template types for character/level creation
enum class EntityTemplate {
  None,
  Player,
  Enemy_Blob,
  Enemy_ShieldGuard,
  Enemy_FireWisp,
  Enemy_Chaser,
  Enemy_Boss,
  Platform_Static,
  Platform_Moving,
  Platform_Falling,
  Pickup_Coin,
  Pickup_Heart,
  Pickup_Mana,
  Pickup_Key,
  Projectile_Fireball
};

class Editor {
 public:
  Editor();
  ~Editor();

  bool init(SDL_Window* window, void* glContext);
  void shutdown();

  // Process SDL events for ImGui
  void processEvent(const SDL_Event& event);

  // Set the camera for screen→world coordinate conversion
  void setCamera(const CameraSystem* cam) { camera = cam; }

  // Update mouse state for viewport picking (call every frame before render)
  void updateMouseState(int mouseX, int mouseY, int windowWidth, int windowHeight);

  // Check if a viewport click should select an entity and return the picked index (-1 if none)
  int processViewportClick(SDL_Window* window, EntityManager& em);

  // Render the editor UI
  void render(SDL_Window* window, EntityManager& em, SpriteAnimator& anim);

  int getSelectedEntityIndex() const { return selectedEntityIndex; }
  void setSelectedEntityIndex(int idx) { selectedEntityIndex = idx; }

  bool isEnabled() const { return enabled; }
  void setEnabled(bool e) { enabled = e; }

 private:
  bool enabled{false};
  const CameraSystem* camera{nullptr};

  // Mouse state for viewport picking
  int mouseScreenX{0}, mouseScreenY{0};
  int windowWidth{1280}, windowHeight{720};
  bool mouseClicked{false};
  bool mouseReleased{false};

  // UI panels
  void drawMainMenuBar();
  void drawEntityInspector(EntityManager& em);
  void drawTilePalette();
  void drawCharacterEditor(EntityManager& em);
  void drawLevelTools(EntityManager& em);
  void drawHierarchyPanel(EntityManager& em);
  void drawToolbar();
  void drawLevelBrowser(EntityManager& em);
  void drawPixelEditor(EntityManager& em, SpriteAnimator& anim);

  // Render selection highlight in the 3D viewport (integrated into render)
  void renderSelectionHighlight(SDL_Window* window, EntityManager& em);

  // Entity creation helpers
  void drawCreateEntityMenu(EntityManager& em);
  EntityHandle createEntityFromTemplate(EntityManager& em, EntityTemplate type);

  // Viewport picking helpers
  glm::vec2 screenToWorld(SDL_Window* window, int screenX, int screenY) const;
  int pickEntityAtPosition(EntityManager& em, glm::vec2 worldPos) const;

  // State
  int selectedEntityIndex{-1};
  int hoveredEntityIndex{-1};
  EntityTemplate newEntityType{EntityTemplate::None};
  bool showCharacterEditor{false};
  bool showLevelTools{false};
  bool showHierarchy{true};
  bool showTilePalette{false};
  bool showEntityInspector{true};
  bool showLevelBrowser{false};
  bool showPixelEditor{false};

  // Level editing state
  glm::vec2 spawnPosition{5.0f, 3.0f};
  float gridSnapSize{0.25f};
  bool gridSnapEnabled{true};

  // Character editing state
  int editingCharacterIndex{-1};
  std::string characterName{"Hero"};
  int characterHealth{6};
  int characterMana{10};
  float characterSpeed{8.0f};
  float characterJumpForce{10.0f};
  float characterMeleeDamage{1.0f};
  float characterMeleeRange{0.8f};
  float characterFireballDamage{2.0f};

  // Level tools state
  std::vector<glm::vec2> platformPath;
  bool drawingPlatform{false};

  // Pixel editor
  PixelEditor pixelEditor;
};
