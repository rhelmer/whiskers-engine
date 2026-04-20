// Editor.cpp
#include "Editor.h"

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>
#include <iostream>
#include <cmath>
#include <cstdio>

#ifdef WHISKERS_WASM_BUILD
#include <whiskers/glad_wasm.h>
#else
#include <glad/glad.h>
#endif

#include <glm/gtc/matrix_transform.hpp>
#include "../rendering/CameraSystem.h"
#include "LevelSerializer.h"

Editor::Editor() {}
Editor::~Editor() {}

bool Editor::init(SDL_Window* window, void* glContext) {
  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO(); (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigWindowsMoveFromTitleBarOnly = true;

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();

  // Setup Platform/Renderer backends
  if (!ImGui_ImplSDL2_InitForOpenGL(window, glContext)) {
    std::cerr << "ImGui_ImplSDL2_InitForOpenGL failed\n";
    return false;
  }

#ifdef WHISKERS_WASM_BUILD
  const char* glsl_version = "#version 300 es";
#else
  const char* glsl_version = "#version 330 core";
#endif

  if (!ImGui_ImplOpenGL3_Init(glsl_version)) {
    std::cerr << "ImGui_ImplOpenGL3_Init failed\n";
    return false;
  }

  enabled = true;
  return true;
}

void Editor::shutdown() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();
}

void Editor::processEvent(const SDL_Event& event) {
  ImGui_ImplSDL2_ProcessEvent(&event);

  // Track mouse button state for viewport picking
  if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
    mouseClicked = true;
  }
  if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
    mouseReleased = true;
  }
}

void Editor::updateMouseState(int mouseX, int mouseY, int winW, int winH) {
  mouseScreenX = mouseX;
  mouseScreenY = mouseY;
  windowWidth = winW;
  windowHeight = winH;
}

int Editor::processViewportClick(SDL_Window* window, EntityManager& em) {
  if (!mouseReleased) return -1;
  mouseReleased = false;

  // Don't pick if ImGui is capturing the mouse (click was on a UI element)
  ImGuiIO& io = ImGui::GetIO();
  if (io.WantCaptureMouse) return -1;

  glm::vec2 worldPos = screenToWorld(window, mouseScreenX, mouseScreenY);
  int picked = pickEntityAtPosition(em, worldPos);
  
  std::cout << "Viewport click at (" << mouseScreenX << ", " << mouseScreenY 
            << ") -> World (" << worldPos.x << ", " << worldPos.y << ")"
            << " -> Picked: " << picked << std::endl;

  return picked;
}

void Editor::render(SDL_Window* window, EntityManager& em, SpriteAnimator& anim) {
  if (!enabled) return;

  // Start the Dear ImGui frame
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();

  // Draw viewport highlights first (background list)
  renderSelectionHighlight(window, em);

  drawMainMenuBar();
  drawToolbar();

  if (showHierarchy) drawHierarchyPanel(em);
  if (showEntityInspector) drawEntityInspector(em);
  if (showCharacterEditor) drawCharacterEditor(em);
  if (showLevelTools) drawLevelTools(em);
  if (showTilePalette) drawTilePalette();
  if (showLevelBrowser) drawLevelBrowser(em);
  if (showPixelEditor) drawPixelEditor(em, anim);

  // Rendering
  ImGui::Render();
  
  // Ensure viewport is correct for HighDPI before drawing ImGui and highlights
  int drawW, drawH;
  SDL_GL_GetDrawableSize(window, &drawW, &drawH);
  glViewport(0, 0, drawW, drawH);

  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Editor::drawMainMenuBar() {
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("New Level")) {}
      if (ImGui::MenuItem("Open Level...")) { showLevelBrowser = true; }
      if (ImGui::MenuItem("Save Level")) {
        // Handled in drawLevelBrowser for now
      }
      ImGui::Separator();
      if (ImGui::MenuItem("Level Browser")) { showLevelBrowser = !showLevelBrowser; }
      if (ImGui::MenuItem("Pixel Editor")) { showPixelEditor = !showPixelEditor; }
      ImGui::Separator();
      if (ImGui::MenuItem("Exit")) {}
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Edit")) {
      if (ImGui::MenuItem("Undo", "Ctrl+Z", false, false)) {}
      if (ImGui::MenuItem("Redo", "Ctrl+Y", false, false)) {}
      ImGui::Separator();
      if (ImGui::MenuItem("Duplicate Entity", "Ctrl+D", false, selectedEntityIndex >= 0)) {}
      if (ImGui::MenuItem("Delete Entity", "Del", false, selectedEntityIndex >= 0)) {}
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("View")) {
      ImGui::MenuItem("Hierarchy", NULL, &showHierarchy);
      ImGui::MenuItem("Entity Inspector", NULL, &showEntityInspector);
      ImGui::MenuItem("Character Editor", NULL, &showCharacterEditor);
      ImGui::MenuItem("Level Tools", NULL, &showLevelTools);
      ImGui::MenuItem("Tile Palette", NULL, &showTilePalette);
      ImGui::MenuItem("Level Browser", NULL, &showLevelBrowser);
      ImGui::MenuItem("Pixel Editor", NULL, &showPixelEditor);
      ImGui::Separator();
      static bool showDemo = false;
      ImGui::MenuItem("ImGui Demo", NULL, &showDemo);
      if (showDemo) ImGui::ShowDemoWindow(&showDemo);
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Create")) {
      if (ImGui::MenuItem("Player Character")) {
        // Will be handled when EntityManager is available in render loop
      }
      ImGui::Separator();
      if (ImGui::BeginMenu("Enemies")) {
        if (ImGui::MenuItem("Blob enemy")) {}
        if (ImGui::MenuItem("Shield Guard")) {}
        if (ImGui::MenuItem("Fire Wisp")) {}
        if (ImGui::MenuItem("Chaser")) {}
        if (ImGui::MenuItem("Boss")) {}
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("Platforms")) {
        if (ImGui::MenuItem("Static Platform")) {}
        if (ImGui::MenuItem("Moving Platform")) {}
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("Pickups")) {
        if (ImGui::MenuItem("Coin")) {}
        if (ImGui::MenuItem("Heart")) {}
        if (ImGui::MenuItem("Mana Orb")) {}
        if (ImGui::MenuItem("Key")) {}
        ImGui::EndMenu();
      }
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Help")) {
      if (ImGui::MenuItem("Controls")) {}
      if (ImGui::MenuItem("About")) {}
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }
}

void Editor::drawToolbar() {
  ImGui::Begin("Toolbar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse);
  ImGui::SetWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, 40));

  ImGui::Text("Spawn: ");
  ImGui::SameLine();
  ImGui::DragFloat2("Pos", &spawnPosition.x, 0.1f);

  ImGui::SameLine();
  if (ImGui::Button("Snap to Grid")) gridSnapEnabled = !gridSnapEnabled;

  ImGui::SameLine();
  ImGui::Text("Grid: ");
  ImGui::SameLine();
  ImGui::DragFloat("##grid", &gridSnapSize, 0.05f, 0.05f, 2.0f, "%.2f");

  ImGui::SameLine();
  ImGui::Separator();

  ImGui::SameLine();
  if (ImGui::Button("Play")) { setEnabled(false); }
  ImGui::SameLine();
  if (ImGui::Button("Stop")) { setEnabled(true); }

  ImGui::End();
}

void Editor::drawHierarchyPanel(EntityManager& em) {
  ImGui::Begin("Hierarchy");

  // Entity tree grouped by type
  if (ImGui::TreeNode("Player")) {
    auto inputIndices = em.queryWithInput();
    for (size_t idx : inputIndices) {
      char label[32];
      snprintf(label, sizeof(label), "Player (%zu)", idx);
      if (ImGui::Selectable(label, selectedEntityIndex == (int)idx))
        selectedEntityIndex = (int)idx;
    }
    ImGui::TreePop();
  }

  if (ImGui::TreeNode("Enemies")) {
    auto enemyIndices = em.queryWithEnemyAI();
    for (size_t idx : enemyIndices) {
      char label[32];
      snprintf(label, sizeof(label), "Enemy %zu", idx);
      if (ImGui::Selectable(label, selectedEntityIndex == (int)idx))
        selectedEntityIndex = (int)idx;
    }
    ImGui::TreePop();
  }

  if (ImGui::TreeNode("Platforms")) {
    auto platformIndices = em.queryWithPlatform();
    for (size_t idx : platformIndices) {
      char label[32];
      snprintf(label, sizeof(label), "Platform %zu", idx);
      if (ImGui::Selectable(label, selectedEntityIndex == (int)idx))
        selectedEntityIndex = (int)idx;
    }
    ImGui::TreePop();
  }

  if (ImGui::TreeNode("Pickups")) {
    auto pickupIndices = em.queryWithPickup();
    for (size_t idx : pickupIndices) {
      char label[32];
      snprintf(label, sizeof(label), "Pickup %zu", idx);
      if (ImGui::Selectable(label, selectedEntityIndex == (int)idx))
        selectedEntityIndex = (int)idx;
    }
    ImGui::TreePop();
  }

  if (ImGui::TreeNode("Other")) {
    auto allTransforms = em.queryWithTransform();
    auto inputIndices = em.queryWithInput();
    auto enemyIndices = em.queryWithEnemyAI();
    auto platformIndices = em.queryWithPlatform();
    auto pickupIndices = em.queryWithPickup();

    // Helper lambda to check if entity is already categorized
    auto isCategorized = [&](size_t idx) {
      for (size_t i : inputIndices) if (i == idx) return true;
      for (size_t i : enemyIndices) if (i == idx) return true;
      for (size_t i : platformIndices) if (i == idx) return true;
      for (size_t i : pickupIndices) if (i == idx) return true;
      return false;
    };

    for (size_t idx : allTransforms) {
      if (isCategorized(idx)) continue;
      char label[32];
      snprintf(label, sizeof(label), "Entity %zu", idx);
      if (ImGui::Selectable(label, selectedEntityIndex == (int)idx))
        selectedEntityIndex = (int)idx;
    }
    ImGui::TreePop();
  }

  ImGui::Separator();
  if (ImGui::Button("Delete Selected", ImVec2(-1, 0))) {
    if (selectedEntityIndex >= 0 && selectedEntityIndex < (int)em.getAll().size()) {
      em.destroyEntity({(uint32_t)selectedEntityIndex, em.get(selectedEntityIndex).generation});
      selectedEntityIndex = -1;
    }
  }

  ImGui::End();
}

void Editor::drawEntityInspector(EntityManager& em) {
  ImGui::Begin("Entity Inspector");

  auto entities = em.queryWithTransform();
  if (selectedEntityIndex == -1 || selectedEntityIndex >= (int)em.getAll().size()) {
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No entity selected");
    ImGui::End();
    return;
  }

  auto& e = em.get(selectedEntityIndex);
  if (!e.alive) {
    selectedEntityIndex = -1;
    ImGui::End();
    return;
  }

  ImGui::Text("Entity %d", selectedEntityIndex);
  ImGui::Separator();

  // Component flags
  ImGui::Text("Components:");
  ImGui::Checkbox("Transform", &e.hasTransform);
  ImGui::SameLine();
  ImGui::Checkbox("Physics", &e.hasPhysics);
  ImGui::SameLine();
  ImGui::Checkbox("Render", &e.hasRender);
  ImGui::SameLine();
  ImGui::Checkbox("Collider", &e.hasCollider);
  ImGui::SameLine();
  ImGui::Checkbox("Health", &e.hasHealth);
  ImGui::SameLine();
  ImGui::Checkbox("Input", &e.hasInput);
  ImGui::SameLine();
  ImGui::Checkbox("Camera", &e.hasCamera);
  ImGui::SameLine();
  ImGui::Checkbox("AI", &e.hasEnemyAI);
  ImGui::SameLine();
  ImGui::Checkbox("Platform", &e.hasPlatform);
  ImGui::SameLine();
  ImGui::Checkbox("Attack", &e.hasAttack);
  ImGui::SameLine();
  ImGui::Checkbox("Projectile", &e.hasProjectile);
  ImGui::SameLine();
  ImGui::Checkbox("Pickup", &e.hasPickup);

  ImGui::Separator();

  if (e.hasTransform) {
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::DragFloat3("Position", &e.transform.position.x, 0.1f);
      ImGui::DragFloat3("Rotation", &e.transform.rotation.x, 1.0f);
      ImGui::DragFloat3("Scale", &e.transform.scale.x, 0.1f, 0.01f);
    }
  }

  if (e.hasRender) {
    if (ImGui::CollapsingHeader("Render", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::ColorEdit3("Color", &e.render.color.x);
      ImGui::Checkbox("Visible", &e.render.visible);
      ImGui::Checkbox("Use 3D Geometry", &e.render.use3DGeometry);
      ImGui::Checkbox("Use Pixel Art Sprite", &e.render.usePixelArtSprite);
      ImGui::DragFloat("Sprite Scale", &e.render.spriteScale, 0.01f, 0.01f);
      ImGui::Checkbox("Flip X", &e.render.flipX);
      ImGui::DragInt("Sort Order", &e.render.sortOrder);
    }
  }

  if (e.hasPhysics) {
    if (ImGui::CollapsingHeader("Physics")) {
      ImGui::DragFloat2("Velocity", &e.physics.velocity.x, 0.1f);
      ImGui::DragFloat("Gravity Scale", &e.physics.gravityScale, 0.1f, 0.0f, 5.0f);
      ImGui::Checkbox("Affected By Gravity", &e.physics.affectedByGravity);
      ImGui::DragFloat("Mass", &e.physics.mass, 0.1f, 0.1f, 100.0f);
    }
  }

  if (e.hasCollider) {
    if (ImGui::CollapsingHeader("Collider")) {
      const char* shapeNames[] = {"AABB", "Circle"};
      int currentShape = (int)e.collider.shape;
      ImGui::Combo("Shape", &currentShape, shapeNames, 2);
      e.collider.shape = (ColliderShape)currentShape;

      ImGui::DragFloat2("Half Size", &e.collider.bounds.max.x, 0.1f, 0.01f);
      ImGui::Checkbox("Is Trigger", &e.collider.isTrigger);
      int layer = (int)e.collider.layer;
      int mask = (int)e.collider.mask;
      ImGui::DragInt("Layer", &layer);
      ImGui::DragInt("Mask", &mask);
      e.collider.layer = (uint8_t)layer;
      e.collider.mask = (uint8_t)mask;
    }
  }

  if (e.hasHealth) {
    if (ImGui::CollapsingHeader("Health")) {
      ImGui::DragInt("Current HP", &e.health.current, 1, 0);
      ImGui::DragInt("Max HP", &e.health.max, 1, 1);
      ImGui::DragInt("Mana", &e.health.mana, 1, 0, e.health.maxMana);
      ImGui::DragInt("Max Mana", &e.health.maxMana, 1, 1);
      ImGui::DragInt("Coins", &e.health.coins, 1, 0);
      ImGui::DragInt("Keys", &e.health.keys, 1, 0);
    }
  }

  if (e.hasEnemyAI) {
    if (ImGui::CollapsingHeader("Enemy AI")) {
      const char* stateNames[] = {"Patrol", "Chase", "Stunned", "Dead"};
      int currentState = (int)e.enemyAI.state;
      ImGui::Combo("State", &currentState, stateNames, 4);
      e.enemyAI.state = (EnemyAIComponent::State)currentState;

      ImGui::DragFloat("Patrol Start", &e.enemyAI.patrolStart, 0.1f);
      ImGui::DragFloat("Patrol End", &e.enemyAI.patrolEnd, 0.1f);
      ImGui::DragFloat("Detection Range", &e.enemyAI.detectionRange, 0.1f);
      ImGui::DragFloat("Speed", &e.enemyAI.speed, 0.1f, 0.1f);
      ImGui::Checkbox("Facing Right", &e.enemyAI.facingRight);
      ImGui::DragFloat("Attack Cooldown", &e.enemyAI.attackCooldown, 0.1f);
    }
  }

  if (e.hasAttack) {
    if (ImGui::CollapsingHeader("Attack")) {
      const char* typeNames[] = {"Melee", "Ranged"};
      int currentType = (int)e.attack.type;
      ImGui::Combo("Type", &currentType, typeNames, 2);
      e.attack.type = (AttackComponent::AttackType)currentType;

      ImGui::DragFloat("Damage", &e.attack.damage, 0.1f);
      ImGui::DragFloat("Range", &e.attack.range, 0.1f);
      ImGui::DragFloat("Cooldown", &e.attack.cooldown, 0.01f);
      ImGui::DragFloat("Duration", &e.attack.duration, 0.01f);
      ImGui::DragInt("Max Combo", &e.attack.maxCombo, 1, 1, 10);
      ImGui::DragFloat2("Facing Dir", &e.attack.facingDir.x, 0.1f);
    }
  }

  if (e.hasPlatform) {
    if (ImGui::CollapsingHeader("Platform")) {
      const char* moveTypeNames[] = {"Static", "Moving", "Falling", "Conveyor"};
      int currentType = (int)e.platform.moveType;
      ImGui::Combo("Move Type", &currentType, moveTypeNames, 4);
      e.platform.moveType = (PlatformComponent::MoveType)currentType;

      ImGui::DragFloat("Speed", &e.platform.speed, 0.1f);
      ImGui::DragFloat("Path Progress", &e.platform.pathProgress, 0.01f, 0.0f, 1.0f);
      ImGui::DragInt("Current Waypoint", &e.platform.currentWaypoint);
    }
  }

  if (e.hasPickup) {
    if (ImGui::CollapsingHeader("Pickup")) {
      const char* typeNames[] = {"Coin", "Heart", "Mana", "Key"};
      int currentType = (int)e.pickup.type;
      ImGui::Combo("Type", &currentType, typeNames, 4);
      e.pickup.type = (PickupComponent::PickupType)currentType;

      ImGui::DragInt("Value", &e.pickup.value, 1, 0);
      ImGui::DragFloat("Bob Amount", &e.pickup.bobAmount, 0.01f, 0.0f, 1.0f);
      ImGui::Checkbox("Collected", &e.pickup.collected);
    }
  }

  if (e.hasCamera) {
    if (ImGui::CollapsingHeader("Camera")) {
      ImGui::DragInt("Follow Target", &e.camera.followTarget);
      ImGui::DragFloat2("Offset", &e.camera.offset.x, 0.1f);
      ImGui::DragFloat("Smooth Speed", &e.camera.smoothSpeed, 0.1f);
      ImGui::Checkbox("Bounds Enabled", &e.camera.bounds.enabled);
      if (e.camera.bounds.enabled) {
        ImGui::DragFloat("Left", &e.camera.bounds.left, 0.1f);
        ImGui::DragFloat("Right", &e.camera.bounds.right, 0.1f);
        ImGui::DragFloat("Bottom", &e.camera.bounds.bottom, 0.1f);
        ImGui::DragFloat("Top", &e.camera.bounds.top, 0.1f);
      }
    }
  }

  ImGui::End();
}

void Editor::drawCharacterEditor(EntityManager& em) {
  ImGui::Begin("Character Editor");

  ImGui::Text("Character Configuration");
  ImGui::Separator();

  // Character selection
  auto inputIndices = em.queryWithInput();
  if (!inputIndices.empty()) {
    static int charIdx = 0;
    if (charIdx >= (int)inputIndices.size()) charIdx = 0;

    size_t entityIdx = inputIndices[charIdx];
    auto& player = em.get(entityIdx);

    ImGui::Text("Selected: Player (Entity %zu)", entityIdx);

    if (player.hasTransform) {
      if (ImGui::CollapsingHeader("Player Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
        char nameBuf[128];
        strncpy(nameBuf, characterName.c_str(), sizeof(nameBuf) - 1);
        nameBuf[sizeof(nameBuf) - 1] = '\0';
        if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) {
          characterName = nameBuf;
        }
        ImGui::DragFloat2("Spawn Position", &player.transform.position.x, 0.1f);
      }
    }

    if (player.hasPhysics) {
      if (ImGui::CollapsingHeader("Movement Stats", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::DragFloat("Move Speed", &characterSpeed, 0.1f, 1.0f, 20.0f, "%.1f");
        ImGui::DragFloat("Jump Force", &characterJumpForce, 0.1f, 1.0f, 30.0f, "%.1f");
        ImGui::DragFloat("Gravity Scale", &player.physics.gravityScale, 0.1f, 0.0f, 5.0f);
      }
    }

    if (player.hasHealth) {
      if (ImGui::CollapsingHeader("Health & Resources", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::DragInt("Max Health", &characterHealth, 1, 1, 100);
        player.health.max = characterHealth;
        player.health.current = std::min(player.health.current, characterHealth);

        ImGui::DragInt("Max Mana", &characterMana, 1, 1, 100);
        player.health.maxMana = characterMana;
        player.health.mana = std::min(player.health.mana, characterMana);

        ImGui::SliderInt("Current Health", &player.health.current, 0, characterHealth);
        ImGui::SliderInt("Current Mana", &player.health.mana, 0, characterMana);
      }
    }

    if (player.hasAttack) {
      if (ImGui::CollapsingHeader("Combat Stats", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::DragFloat("Melee Damage", &characterMeleeDamage, 0.1f, 0.1f, 50.0f);
        player.attack.damage = characterMeleeDamage;

        ImGui::DragFloat("Melee Range", &characterMeleeRange, 0.1f, 0.1f, 10.0f);
        player.attack.range = characterMeleeRange;

        ImGui::DragFloat("Attack Cooldown", &player.attack.cooldown, 0.01f, 0.01f, 5.0f);
        ImGui::DragFloat("Attack Duration", &player.attack.duration, 0.01f, 0.01f, 2.0f);
        ImGui::DragInt("Max Combo", &player.attack.maxCombo, 1, 1, 10);

        ImGui::Separator();
        ImGui::Text("Fireball (Ranged)");
        ImGui::DragFloat("Fireball Damage", &characterFireballDamage, 0.1f, 0.1f, 50.0f);
      }
    }

    if (player.hasCollider) {
      if (ImGui::CollapsingHeader("Collision")) {
        ImGui::DragFloat2("Collider Half Size", &player.collider.bounds.max.x, 0.01f, 0.01f);
        ImGui::Checkbox("Is Trigger", &player.collider.isTrigger);
      }
    }
  } else {
    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "No player character found!");
    if (ImGui::Button("Create Player")) {
      createEntityFromTemplate(em, EntityTemplate::Player);
    }
  }

  ImGui::Separator();

  // Preset templates
  if (ImGui::CollapsingHeader("Enemy Presets")) {
    if (ImGui::Button("Add blob enemy")) createEntityFromTemplate(em, EntityTemplate::Enemy_Blob);
    if (ImGui::Button("Add Shield Guard")) createEntityFromTemplate(em, EntityTemplate::Enemy_ShieldGuard);
    if (ImGui::Button("Add Fire Wisp")) createEntityFromTemplate(em, EntityTemplate::Enemy_FireWisp);
    if (ImGui::Button("Add chaser")) createEntityFromTemplate(em, EntityTemplate::Enemy_Chaser);
    if (ImGui::Button("Add Boss")) createEntityFromTemplate(em, EntityTemplate::Enemy_Boss);
  }

  ImGui::End();
}

void Editor::drawLevelTools(EntityManager& em) {
  ImGui::Begin("Level Tools");

  ImGui::Text("Platform Creation");
  ImGui::Separator();

  ImGui::Text("Spawn Position:");
  ImGui::DragFloat2("Position", &spawnPosition.x, 0.1f);

  ImGui::Separator();
  ImGui::Text("Platform Type:");
  static int platType = 0;
  const char* platTypes[] = {"Static", "Moving", "Falling", "Conveyor"};
  ImGui::Combo("##platType", &platType, platTypes, 4);

  if (platType == 1) {
    ImGui::Text("Waypoints (click to add):");
    if (ImGui::Button("Add Waypoint at Spawn")) {
      platformPath.push_back(spawnPosition);
    }
    ImGui::Text("%zu waypoints", platformPath.size());
    for (size_t i = 0; i < platformPath.size(); i++) {
      char label[32];
      snprintf(label, sizeof(label), "WP %zu", i);
      ImGui::DragFloat2(label, &platformPath[i].x, 0.1f);
    }
    if (ImGui::Button("Clear Waypoints")) platformPath.clear();
  }

  ImGui::Separator();
  if (ImGui::Button("Create Platform", ImVec2(-1, 0))) {
    // Create platform entity
    auto h = em.createEntity();
    auto& e = em.get(h.index);
    e.hasTransform = true;
    e.transform.position = glm::vec3(spawnPosition, 0.0f);
    e.hasCollider = true;
    e.collider.shape = ColliderShape::AABB;
    e.collider.bounds = AABB::fromCenterAndHalfSize(glm::vec2(0), glm::vec2(1.5f, 0.2f));
    e.transform.scale = glm::vec3(3.0f, 0.4f, 1.0f);
    e.collider.layer = Layer_Platform;
    e.collider.mask = Layer_None;
    e.hasRender = true;
    e.render.color = glm::vec3(0.3f, 0.25f, 0.2f);
    e.render.use3DGeometry = true;
    e.render.sortOrder = 0;
    e.hasPlatform = true;
    e.platform.moveType = (PlatformComponent::MoveType)platType;
    e.platform.path = platformPath;
    e.platform.speed = 1.5f;
  }

  ImGui::Separator();
  ImGui::Text("Quick Add Pickups:");
  if (ImGui::Button("Coin Row")) {
    for (int i = 0; i < 5; i++) {
      auto h = em.createEntity();
      auto& e = em.get(h.index);
      e.hasTransform = true;
      e.transform.position = glm::vec3(spawnPosition.x + i * 1.0f, spawnPosition.y, 0.0f);
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
    }
  }

  ImGui::End();
}

void Editor::drawTilePalette() {
  ImGui::Begin("Tile Palette");
  ImGui::Text("Tile Palette — Coming Soon");
  ImGui::Separator();
  ImGui::Text("Future features:");
  ImGui::Bullet(); ImGui::SameLine(); ImGui::Text("Tile type definitions");
  ImGui::Bullet(); ImGui::SameLine(); ImGui::Text("Tile atlas editor");
  ImGui::Bullet(); ImGui::SameLine(); ImGui::Text("Tilemap painting tools");
  ImGui::Bullet(); ImGui::SameLine(); ImGui::Text("Tile collision mapping");
  ImGui::End();
}

// Entity creation from templates
EntityHandle Editor::createEntityFromTemplate(EntityManager& em, EntityTemplate type) {
  EntityHandle h = em.createEntity();
  auto& e = em.get(h.index);

  switch (type) {
    case EntityTemplate::Player:
      e.hasTransform = true;
      e.transform.position = glm::vec3(2.0f, 2.0f, 0.0f);
      e.hasPhysics = true;
      e.physics.gravityScale = characterSpeed > 0 ? 1.0f : 0.0f;
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
      e.hasInput = true;
      e.hasCamera = true;
      e.camera.followTarget = (int)h.index;
      e.camera.offset = glm::vec2(0.0f, 2.0f);
      e.camera.smoothSpeed = 8.0f;
      e.hasHealth = true;
      e.health.current = characterHealth;
      e.health.max = characterHealth;
      e.health.mana = characterMana;
      e.health.maxMana = characterMana;
      e.hasAttack = true;
      e.attack.type = AttackComponent::Melee;
      e.attack.damage = characterMeleeDamage;
      e.attack.range = characterMeleeRange;
      e.attack.cooldown = 0.3f;
      e.attack.duration = 0.15f;
      e.attack.facingDir = glm::vec2(1.0f, 0.0f);
      break;

    case EntityTemplate::Enemy_Blob:
      e.hasTransform = true;
      e.transform.position = glm::vec3(spawnPosition, 0.0f);
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
      e.hasHealth = true;
      e.health.current = 2;
      e.health.max = 2;
      e.hasEnemyAI = true;
      e.enemyAI.state = EnemyAIComponent::Patrol;
      e.enemyAI.patrolStart = spawnPosition.x - 2.0f;
      e.enemyAI.patrolEnd = spawnPosition.x + 2.0f;
      e.enemyAI.speed = 1.5f;
      e.enemyAI.facingRight = true;
      break;

    case EntityTemplate::Enemy_ShieldGuard:
      e.hasTransform = true;
      e.transform.position = glm::vec3(spawnPosition, 0.0f);
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
      e.hasHealth = true;
      e.health.current = 4;
      e.health.max = 4;
      e.hasEnemyAI = true;
      e.enemyAI.state = EnemyAIComponent::Patrol;
      e.enemyAI.patrolStart = spawnPosition.x - 2.0f;
      e.enemyAI.patrolEnd = spawnPosition.x + 2.0f;
      e.enemyAI.speed = 1.0f;
      e.enemyAI.facingRight = false;
      break;

    case EntityTemplate::Enemy_FireWisp:
      e.hasTransform = true;
      e.transform.position = glm::vec3(spawnPosition, 0.0f);
      e.hasCollider = true;
      e.collider.shape = ColliderShape::AABB;
      e.collider.bounds = AABB::fromCenterAndHalfSize(glm::vec2(0), glm::vec2(0.3f, 0.3f));
      e.collider.layer = Layer_Enemy;
      e.collider.mask = Layer_Player;
      e.hasRender = true;
      e.render.sortOrder = 10;
      e.render.spriteScale = 0.06f;
      e.render.usePixelArtSprite = true;
      e.hasHealth = true;
      e.health.current = 1;
      e.health.max = 1;
      break;

    case EntityTemplate::Enemy_Chaser:
      e.hasTransform = true;
      e.transform.position = glm::vec3(spawnPosition, 0.0f);
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
      e.hasHealth = true;
      e.health.current = 3;
      e.health.max = 3;
      e.hasEnemyAI = true;
      e.enemyAI.state = EnemyAIComponent::Patrol;
      e.enemyAI.patrolStart = spawnPosition.x - 2.0f;
      e.enemyAI.patrolEnd = spawnPosition.x + 2.0f;
      e.enemyAI.speed = 3.0f;
      e.enemyAI.facingRight = false;
      break;

    case EntityTemplate::Enemy_Boss:
      e.hasTransform = true;
      e.transform.position = glm::vec3(spawnPosition, 0.0f);
      e.hasCollider = true;
      e.collider.shape = ColliderShape::AABB;
      e.collider.bounds = AABB::fromCenterAndHalfSize(glm::vec2(0), glm::vec2(0.8f, 1.0f));
      e.collider.layer = Layer_Enemy;
      e.collider.mask = Layer_Player;
      e.hasRender = true;
      e.render.sortOrder = 10;
      e.render.spriteScale = 0.06f;
      e.render.usePixelArtSprite = true;
      e.hasHealth = true;
      e.health.current = 12;
      e.health.max = 12;
      break;

    case EntityTemplate::Platform_Static:
    case EntityTemplate::Platform_Moving:
    case EntityTemplate::Platform_Falling:
      e.hasTransform = true;
      e.transform.position = glm::vec3(spawnPosition, 0.0f);
      e.hasCollider = true;
      e.collider.shape = ColliderShape::AABB;
      e.collider.bounds = AABB::fromCenterAndHalfSize(glm::vec2(0), glm::vec2(1.5f, 0.2f));
      e.transform.scale = glm::vec3(3.0f, 0.4f, 1.0f);
      e.collider.layer = Layer_Platform;
      e.collider.mask = Layer_None;
      e.hasRender = true;
      e.render.color = glm::vec3(0.3f, 0.25f, 0.2f);
      e.render.use3DGeometry = true;
      e.render.sortOrder = 0;
      e.hasPlatform = true;
      e.platform.moveType = (type == EntityTemplate::Platform_Moving) ? PlatformComponent::Moving :
                            (type == EntityTemplate::Platform_Falling) ? PlatformComponent::Falling :
                            PlatformComponent::Static;
      break;

    case EntityTemplate::Pickup_Coin:
      e.hasTransform = true;
      e.transform.position = glm::vec3(spawnPosition, 0.0f);
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
      e.pickup.bobTimer = 0.0f;
      break;

    case EntityTemplate::Pickup_Heart:
      e.hasTransform = true;
      e.transform.position = glm::vec3(spawnPosition, 0.0f);
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
      e.pickup.bobTimer = 0.0f;
      break;

    case EntityTemplate::Pickup_Mana:
      e.hasTransform = true;
      e.transform.position = glm::vec3(spawnPosition, 0.0f);
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
      e.pickup.bobTimer = 0.0f;
      break;

    default:
      break;
  }

  return h;
}

// ---------------------------------------------------------------------------
// Level Browser — CRUD for saved levels
// ---------------------------------------------------------------------------
void Editor::drawLevelBrowser(EntityManager& em) {
  ImGui::Begin("Level Browser");

  ImGui::Text("Saved Levels");
  ImGui::Separator();

  auto levels = LevelSerializer::list();
  if (levels.empty()) {
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No saved levels yet.");
  } else {
    for (auto& name : levels) {
      ImGui::PushID(name.c_str());
      ImGui::Text("📄 %s", name.c_str());
      ImGui::SameLine();
      if (ImGui::Button("Load")) {
        std::string json = LevelSerializer::load(name);
        if (!json.empty()) {
          LevelSerializer::deserialize(json, em);
          PixelEditor::regenerateTextures(em);
          selectedEntityIndex = -1;
        }
      }
      ImGui::SameLine();
      if (ImGui::Button("Delete")) {
        LevelSerializer::remove(name);
      }
      ImGui::PopID();
    }
  }

  ImGui::Separator();
  ImGui::Text("Current Level");

  char levelName[128];
  snprintf(levelName, sizeof(levelName), "%s", "autosave");
  ImGui::InputText("Name", levelName, sizeof(levelName));

  if (ImGui::Button("Save")) {
    LevelSerializer::save(em, levelName);
  }
  ImGui::SameLine();
  if (ImGui::Button("Save As New...")) {
    std::string newName = std::string(levelName) + "_copy";
    LevelSerializer::save(em, newName);
  }

  ImGui::SameLine();
  if (ImGui::Button("Export JSON")) {
    std::string json = LevelSerializer::serialize(em, levelName);
    ImGui::SetClipboardText(json.c_str());
  }

  ImGui::Separator();
  ImGui::Text("Entity count: %zu", em.aliveCount());

  ImGui::End();
}

// ---------------------------------------------------------------------------
// Pixel Editor panel wrapper
// ---------------------------------------------------------------------------
void Editor::drawPixelEditor(EntityManager& em, SpriteAnimator& anim) {
  pixelEditor.render();

  ImGui::SetNextWindowSize(ImVec2(300, 80), ImGuiCond_FirstUseEver);
  if (ImGui::Begin("Pixel Editor Actions", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::Text("Sprite: %s", pixelEditor.getName().c_str());
    if (ImGui::Button("Assign to Selected Entity")) {
      if (selectedEntityIndex >= 0 && selectedEntityIndex < (int)em.getAll().size()) {
        pixelEditor.assignToEntity(em, anim, selectedEntityIndex);
      }
    }
    ImGui::SameLine();
    if (ImGui::Button("Load from Selected Entity")) {
      if (selectedEntityIndex >= 0 && selectedEntityIndex < (int)em.getAll().size()) {
        pixelEditor.loadFromEntity(em, anim, selectedEntityIndex);
      }
    }
    ImGui::SameLine();
    if (ImGui::Button("New Sprite")) {
      pixelEditor.newSprite(16, 16);
    }
    ImGui::End();
  }
}

// ---------------------------------------------------------------------------
// Viewport picking
// ---------------------------------------------------------------------------
glm::vec2 Editor::screenToWorld(SDL_Window* window, int screenX, int screenY) const {
  if (!camera) return glm::vec2(0.0f);

  int logW, logH;
  SDL_GetWindowSize(window, &logW, &logH);
  
  // Normalize screen coordinates to [-1, 1]
  float ndcX = (2.0f * (float)screenX) / (float)logW - 1.0f;
  float ndcY = 1.0f - (2.0f * (float)screenY) / (float)logH;

  float aspect = (float)windowWidth / (float)windowHeight;
  float orthoHalfHeight = camera->getOrthoHalfWidth() / aspect;

  glm::vec2 worldPos;
  worldPos.x = camera->getCameraPos().x + ndcX * camera->getOrthoHalfWidth();
  worldPos.y = camera->getCameraPos().y + ndcY * orthoHalfHeight;

  return worldPos;
}

int Editor::pickEntityAtPosition(EntityManager& em, glm::vec2 worldPos) const {
  int bestIdx = -1;
  float bestDist = 1e9f;

  auto colliderIndices = em.queryWithCollider();
  for (size_t idx : colliderIndices) {
    auto& e = em.get(idx);
    if (!e.alive || !e.hasTransform) continue;

    glm::vec2 entityCenter = glm::vec2(e.transform.position);
    glm::vec2 halfSize = e.collider.bounds.halfSize();

    // Simple AABB test
    bool hit = (worldPos.x >= entityCenter.x - halfSize.x &&
                worldPos.x <= entityCenter.x + halfSize.x &&
                worldPos.y >= entityCenter.y - halfSize.y &&
                worldPos.y <= entityCenter.y + halfSize.y);

    if (hit) {
      float dist = glm::distance(worldPos, entityCenter);
      if (dist < bestDist) {
        bestDist = dist;
        bestIdx = (int)idx;
      }
    }
  }

  if (bestIdx == -1 && !colliderIndices.empty()) {
    // Log the first entity for debug
    auto& e = em.get(colliderIndices[0]);
    glm::vec2 center = glm::vec2(e.transform.position);
    glm::vec2 hs = e.collider.bounds.halfSize();
    std::cout << "  Debug (Entity " << colliderIndices[0] << "): pos=" << center.x << "," << center.y 
              << " hs=" << hs.x << "," << hs.y 
              << " bounds=[" << center.x-hs.x << "," << center.x+hs.x << "] x [" << center.y-hs.y << "," << center.y+hs.y << "]"
              << std::endl;
  }

  return bestIdx;
}

void Editor::renderSelectionHighlight(SDL_Window* window, EntityManager& em) {
  if (!camera || !enabled) return;

  // Ensure we have current logical and drawable sizes
  int logW, logH;
  SDL_GetWindowSize(window, &logW, &logH);
  int drawW, drawH;
  SDL_GL_GetDrawableSize(window, &drawW, &drawH);
  windowWidth = drawW;
  windowHeight = drawH;

  // Update hovered entity based on current mouse position
  glm::vec2 worldPos = screenToWorld(window, mouseScreenX, mouseScreenY);
  hoveredEntityIndex = pickEntityAtPosition(em, worldPos);

  auto worldToScreen = [&](glm::vec2 wPos) -> ImVec2 {
    float aspect = (float)windowWidth / (float)windowHeight;
    float orthoHalfHeight = camera->getOrthoHalfWidth() / aspect;
    
    float ndcX = (wPos.x - camera->getCameraPos().x) / camera->getOrthoHalfWidth();
    float ndcY = (wPos.y - camera->getCameraPos().y) / orthoHalfHeight;
    
    return ImVec2((ndcX + 1.0f) * 0.5f * (float)logW, (1.0f - ndcY) * 0.5f * (float)logH);
  };

  auto drawHighlight = [&](int idx, ImU32 color, float thickness) {
    if (idx < 0 || idx >= (int)em.getAll().size()) return;
    auto& e = em.get(idx);
    if (!e.alive || !e.hasTransform) return;

    glm::vec2 center = glm::vec2(e.transform.position);
    glm::vec2 hs;
    if (e.hasCollider) {
      hs = e.collider.bounds.halfSize();
    } else if (e.hasRender && e.render.spriteScale > 0 && e.render.usePixelArtSprite) {
      hs = glm::vec2(e.render.spriteFrameWidth * 0.5f * e.render.spriteScale,
                     e.render.spriteFrameHeight * 0.5f * e.render.spriteScale);
    } else {
      hs = glm::vec2(0.5f, 0.5f);
    }
    hs = glm::max(hs, glm::vec2(0.1f));

    ImVec2 p1 = worldToScreen(center + glm::vec2(-hs.x, -hs.y));
    ImVec2 p2 = worldToScreen(center + glm::vec2(hs.x, -hs.y));
    ImVec2 p3 = worldToScreen(center + glm::vec2(hs.x, hs.y));
    ImVec2 p4 = worldToScreen(center + glm::vec2(-hs.x, hs.y));

    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    drawList->AddQuad(p1, p2, p3, p4, color, thickness);
    
    // Add a very subtle fill
    ImU32 fillColor = (color & 0x00FFFFFF) | 0x22000000;
    drawList->AddQuadFilled(p1, p2, p3, p4, fillColor);
  };

  // Draw hover box (yellow)
  if (hoveredEntityIndex >= 0 && hoveredEntityIndex != selectedEntityIndex) {
    drawHighlight(hoveredEntityIndex, IM_COL32(255, 255, 100, 180), 2.0f);
  }

  // Draw selected box (gold)
  if (selectedEntityIndex >= 0) {
    drawHighlight(selectedEntityIndex, IM_COL32(255, 215, 0, 255), 4.0f);
  }
}
