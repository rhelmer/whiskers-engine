// PixelEditor.cpp
#include "PixelEditor.h"
#include "../game/SpriteAnimator.h"

#include <imgui.h>
#include <nlohmann/json.hpp>
#include <queue>
#include <algorithm>
#include <cmath>
#include <iostream>

using json = nlohmann::json;

PixelEditor::PixelEditor() {
  // PICO-8 inspired default palette
  palette = {
    {0.000f, 0.000f, 0.000f, 0.000f},  // 0: transparent
    {0.000f, 0.000f, 0.000f, 1.000f},  // 1: black
    {0.114f, 0.169f, 0.325f, 1.000f},  // 2: dark blue
    {0.494f, 0.145f, 0.325f, 1.000f},  // 3: dark purple
    {0.000f, 0.529f, 0.318f, 1.000f},  // 4: dark green
    {0.671f, 0.322f, 0.212f, 1.000f},  // 5: brown
    {0.373f, 0.341f, 0.310f, 1.000f},  // 6: gray
    {0.761f, 0.765f, 0.780f, 1.000f},  // 7: light gray
    {1.000f, 0.945f, 0.910f, 1.000f},  // 8: white
    {1.000f, 0.000f, 0.302f, 1.000f},  // 9: red
    {1.000f, 0.639f, 0.000f, 1.000f},  // 10: orange
    {1.000f, 0.925f, 0.153f, 1.000f},  // 11: yellow
    {0.000f, 0.894f, 0.212f, 1.000f},  // 12: green
    {0.161f, 0.678f, 1.000f, 1.000f},  // 13: light blue
    {0.514f, 0.463f, 0.612f, 1.000f},  // 14: purple
    {1.000f, 0.467f, 0.659f, 1.000f},  // 15: pink
    {1.000f, 0.800f, 0.667f, 1.000f},  // 16: peach
  };
  newSprite(16, 16);
}

PixelEditor::~PixelEditor() {
  // Textures are owned by entities; we don't own any here
}

void PixelEditor::newSprite(int width, int height) {
  canvasWidth = std::max(1, std::min(64, width));
  canvasHeight = std::max(1, std::min(64, height));
  frames.clear();
  undoStack.clear();
  redoStack.clear();
  currentFrameIndex = 0;
  previewFrameIndex = 0;
  previewTimer = 0.0f;
  unsavedChanges = false;
  addFrame();
}

// ---------------------------------------------------------------------------
// Pixel access
// ---------------------------------------------------------------------------
void PixelEditor::setPixel(int x, int y, int color) {
  if (x < 0 || x >= canvasWidth || y < 0 || y >= canvasHeight) return;
  auto& frame = frames[currentFrameIndex];
  if (frame.pixels[y][x] != color) {
    frame.pixels[y][x] = color;
    unsavedChanges = true;
  }
}

int PixelEditor::getPixel(int x, int y) const {
  if (x < 0 || x >= canvasWidth || y < 0 || y >= canvasHeight) return -1;
  return frames[currentFrameIndex].pixels[y][x];
}

ImU32 PixelEditor::getPaletteColor(int index) const {
  if (index < 0 || index >= (int)palette.size()) return IM_COL32(0, 0, 0, 0);
  auto& c = palette[index];
  return IM_COL32(
    (ImU8)(c.x * 255),
    (ImU8)(c.y * 255),
    (ImU8)(c.z * 255),
    (ImU8)(c.w * 255)
  );
}

// ---------------------------------------------------------------------------
// Undo/Redo
// ---------------------------------------------------------------------------
void PixelEditor::pushUndo() {
  if (frames.empty()) return;
  undoStack.push_back(frames[currentFrameIndex].pixels);
  if (undoStack.size() > 100) undoStack.erase(undoStack.begin());
  redoStack.clear();
}

void PixelEditor::undo() {
  if (undoStack.empty() || frames.empty()) return;
  redoStack.push_back(frames[currentFrameIndex].pixels);
  frames[currentFrameIndex].pixels = undoStack.back();
  undoStack.pop_back();
  unsavedChanges = true;
}

void PixelEditor::redo() {
  if (redoStack.empty() || frames.empty()) return;
  undoStack.push_back(frames[currentFrameIndex].pixels);
  frames[currentFrameIndex].pixels = redoStack.back();
  redoStack.pop_back();
  unsavedChanges = true;
}

// ---------------------------------------------------------------------------
// Drawing tools
// ---------------------------------------------------------------------------
void PixelEditor::floodFill(int startX, int startY, int newColor) {
  if (frames.empty()) return;
  int oldColor = getPixel(startX, startY);
  if (oldColor == newColor) return;

  pushUndo();
  std::queue<std::pair<int, int>> q;
  q.push({startX, startY});

  while (!q.empty()) {
    auto [x, y] = q.front(); q.pop();
    if (x < 0 || x >= canvasWidth || y < 0 || y >= canvasHeight) continue;
    if (getPixel(x, y) != oldColor) continue;
    frames[currentFrameIndex].pixels[y][x] = newColor;
    q.push({x - 1, y});
    q.push({x + 1, y});
    q.push({x, y - 1});
    q.push({x, y + 1});
  }
  unsavedChanges = true;
}

void PixelEditor::drawLineBresenham(int x0, int y0, int x1, int y1, int color) {
  int dx = std::abs(x1 - x0), dy = std::abs(y1 - y0);
  int sx = (x0 < x1) ? 1 : -1;
  int sy = (y0 < y1) ? 1 : -1;
  int err = dx - dy;

  while (true) {
    setPixel(x0, y0, color);
    if (x0 == x1 && y0 == y1) break;
    int e2 = 2 * err;
    if (e2 > -dy) { err -= dy; x0 += sx; }
    if (e2 < dx) { err += dx; y0 += sy; }
  }
}

void PixelEditor::drawFilledRect(int x0, int y0, int x1, int y1, int color) {
  int minX = std::max(0, std::min(x0, x1));
  int maxX = std::min(canvasWidth - 1, std::max(x0, x1));
  int minY = std::max(0, std::min(y0, y1));
  int maxY = std::min(canvasHeight - 1, std::max(y0, y1));

  for (int y = minY; y <= maxY; y++)
    for (int x = minX; x <= maxX; x++)
      setPixel(x, y, color);
}

// ---------------------------------------------------------------------------
// Frame management
// ---------------------------------------------------------------------------
void PixelEditor::addFrame() {
  Frame f;
  f.name = "Frame " + std::to_string(frames.size() + 1);
  f.pixels.resize(canvasHeight, std::vector<int>(canvasWidth, -1));
  frames.push_back(std::move(f));
  currentFrameIndex = (int)frames.size() - 1;
  previewFrameIndex = currentFrameIndex;
  unsavedChanges = true;
}

void PixelEditor::deleteFrame(int index) {
  if (frames.size() <= 1) return;
  frames.erase(frames.begin() + index);
  if (currentFrameIndex >= (int)frames.size()) currentFrameIndex = (int)frames.size() - 1;
  previewFrameIndex = currentFrameIndex;
  unsavedChanges = true;
}

void PixelEditor::duplicateFrame(int index) {
  if (index < 0 || index >= (int)frames.size()) return;
  Frame dup = frames[index];
  dup.name = frames[index].name + " copy";
  frames.insert(frames.begin() + index + 1, std::move(dup));
  currentFrameIndex = index + 1;
  unsavedChanges = true;
}

// ---------------------------------------------------------------------------
// Canvas rendering
// ---------------------------------------------------------------------------
void PixelEditor::drawCanvas() {
  if (frames.empty()) return;
  auto& frame = frames[currentFrameIndex];

  ImVec2 canvasSize = ImVec2((float)(canvasWidth * pixelSize), (float)(canvasHeight * pixelSize));
  ImGui::InvisibleButton("##canvas", canvasSize);
  ImVec2 pMin = ImGui::GetItemRectMin();
  ImVec2 pMax = ImGui::GetItemRectMax();
  ImDrawList* drawList = ImGui::GetWindowDrawList();

  // Mouse handling
  bool hovered = ImGui::IsItemHovered();
  bool active = ImGui::IsItemActive();

  if (active && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
    ImVec2 mousePos = ImGui::GetMousePos();
    int px = (int)((mousePos.x - pMin.x) / pixelSize);
    int py = (int)((mousePos.y - pMin.y) / pixelSize);
    px = std::max(0, std::min(canvasWidth - 1, px));
    py = std::max(0, std::min(canvasHeight - 1, py));

    if (currentTool == Tool::Pencil) {
      pushUndo();
      setPixel(px, py, selectedColorIndex);
    } else if (currentTool == Tool::Eraser) {
      pushUndo();
      setPixel(px, py, -1);
    } else if (currentTool == Tool::Fill) {
      floodFill(px, py, selectedColorIndex);
    } else if (currentTool == Tool::Picker) {
      int c = getPixel(px, py);
      if (c >= 0) selectedColorIndex = c;
    } else if (currentTool == Tool::Line) {
      pushUndo();
      lineStartX = px; lineStartY = py;
    } else if (currentTool == Tool::Rect) {
      pushUndo();
      rectStartX = px; rectStartY = py;
    }
    lastMouseX = px; lastMouseY = py;
    isDrawing = true;
  }

  if (active && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
    ImVec2 mousePos = ImGui::GetMousePos();
    int px = (int)((mousePos.x - pMin.x) / pixelSize);
    int py = (int)((mousePos.y - pMin.y) / pixelSize);
    px = std::max(0, std::min(canvasWidth - 1, px));
    py = std::max(0, std::min(canvasHeight - 1, py));

    if (currentTool == Tool::Pencil) {
      drawLineBresenham(lastMouseX, lastMouseY, px, py, selectedColorIndex);
    } else if (currentTool == Tool::Eraser) {
      drawLineBresenham(lastMouseX, lastMouseY, px, py, -1);
    }
    lastMouseX = px; lastMouseY = py;
  }

  if (active && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
    if (isDrawing) {
      ImVec2 mousePos = ImGui::GetMousePos();
      int px = (int)((mousePos.x - pMin.x) / pixelSize);
      int py = (int)((mousePos.y - pMin.y) / pixelSize);
      px = std::max(0, std::min(canvasWidth - 1, px));
      py = std::max(0, std::min(canvasHeight - 1, py));

      if (currentTool == Tool::Line) {
        drawLineBresenham(lineStartX, lineStartY, px, py, selectedColorIndex);
      } else if (currentTool == Tool::Rect) {
        drawFilledRect(rectStartX, rectStartY, px, py, selectedColorIndex);
      }
    }
    isDrawing = false;
    lineStartX = lineStartY = -1;
    rectStartX = rectStartY = -1;
  }

  // Draw checkerboard background
  ImU32 dark = IM_COL32(40, 40, 40, 255);
  ImU32 light = IM_COL32(60, 60, 60, 255);
  for (int y = 0; y < canvasHeight; y++) {
    for (int x = 0; x < canvasWidth; x++) {
      ImU32 c = ((x + y) % 2 == 0) ? dark : light;
      drawList->AddRectFilled(
        ImVec2(pMin.x + x * pixelSize, pMin.y + y * pixelSize),
        ImVec2(pMin.x + (x + 1) * pixelSize, pMin.y + (y + 1) * pixelSize),
        c
      );
    }
  }

  // Draw pixels
  for (int y = 0; y < canvasHeight; y++) {
    for (int x = 0; x < canvasWidth; x++) {
      int colorIdx = frame.pixels[y][x];
      if (colorIdx < 0 || colorIdx >= (int)palette.size()) continue;
      ImU32 col = getPaletteColor(colorIdx);
      drawList->AddRectFilled(
        ImVec2(pMin.x + x * pixelSize, pMin.y + y * pixelSize),
        ImVec2(pMin.x + (x + 1) * pixelSize, pMin.y + (y + 1) * pixelSize),
        col
      );
    }
  }

  // Draw grid
  if (showGrid && pixelSize > 4) {
    ImU32 gridCol = IM_COL32(80, 80, 80, 128);
    for (int x = 0; x <= canvasWidth; x++) {
      drawList->AddLine(
        ImVec2(pMin.x + x * pixelSize, pMin.y),
        ImVec2(pMin.x + x * pixelSize, pMax.y), gridCol
      );
    }
    for (int y = 0; y <= canvasHeight; y++) {
      drawList->AddLine(
        ImVec2(pMin.x, pMin.y + y * pixelSize),
        ImVec2(pMax.x, pMin.y + y * pixelSize), gridCol
      );
    }
  }

  // Draw tool preview (line/rect while dragging)
  if (isDrawing && (currentTool == Tool::Line || currentTool == Tool::Rect) &&
      lineStartX >= 0 && rectStartX >= 0) {
    // This would need current mouse pos; simplified: skip preview for now
  }
}

// ---------------------------------------------------------------------------
// Palette rendering
// ---------------------------------------------------------------------------
void PixelEditor::drawPalette() {
  ImGui::Text("Palette");
  ImGui::Separator();

  // Color swatches in a grid
  int cols = 4;
  float swatchSize = 24.0f;
  for (int i = 0; i < (int)palette.size(); i++) {
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 size = ImVec2(swatchSize, swatchSize);
    ImU32 col = getPaletteColor(i);
    ImGui::GetWindowDrawList()->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), col);

    // Selection border
    if (i == selectedColorIndex) {
      ImGui::GetWindowDrawList()->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y),
        IM_COL32(255, 255, 0, 255), 0, ImDrawFlags_RoundCornersAll, 2.0f);
    }

    if (ImGui::InvisibleButton(("##swatch" + std::to_string(i)).c_str(), size)) {
      selectedColorIndex = i;
    }
    if (i < (int)palette.size() - 1 && (i + 1) % cols != 0) ImGui::SameLine();
  }

  ImGui::Spacing();

  // Selected color editor
  ImGui::Text("Selected:");
  if (selectedColorIndex >= 0 && selectedColorIndex < (int)palette.size()) {
    ImGui::ColorEdit4(("##color" + std::to_string(selectedColorIndex)).c_str(),
      (float*)&palette[selectedColorIndex],
      ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs);
    unsavedChanges = true;
  }

  ImGui::Spacing();
  if (ImGui::Button("Add Color")) {
    palette.push_back({1.0f, 0.0f, 0.0f, 1.0f});
    selectedColorIndex = (int)palette.size() - 1;
    unsavedChanges = true;
  }
  if (ImGui::Button("Remove Color") && palette.size() > 2) {
    palette.erase(palette.begin() + selectedColorIndex);
    selectedColorIndex = std::min(selectedColorIndex, (int)palette.size() - 1);
    unsavedChanges = true;
  }
}

// ---------------------------------------------------------------------------
// Frame management UI
// ---------------------------------------------------------------------------
void PixelEditor::drawFrames() {
  ImGui::Text("Frames");
  ImGui::Separator();

  ImGui::PushItemWidth(100);
  ImGui::DragFloat("Frame Duration", &frameDuration, 0.01f, 0.01f, 2.0f, "%.2fs");
  ImGui::Checkbox("Loop", &loopAnimation);
  ImGui::PopItemWidth();

  ImGui::Spacing();

  // Frame list
  for (int i = 0; i < (int)frames.size(); i++) {
    char label[64];
    snprintf(label, sizeof(label), "%s", frames[i].name.c_str());
    bool selected = (i == currentFrameIndex);
    if (ImGui::Selectable(label, selected)) {
      currentFrameIndex = i;
      previewFrameIndex = i;
    }
  }

  ImGui::Spacing();
  if (ImGui::Button("+ Frame")) { addFrame(); }
  ImGui::SameLine();
  if (ImGui::Button("Dup Frame") && frames.size() > 0) { duplicateFrame(currentFrameIndex); }
  ImGui::SameLine();
  if (ImGui::Button("- Frame") && frames.size() > 1) { deleteFrame(currentFrameIndex); }
}

// ---------------------------------------------------------------------------
// Preview rendering
// ---------------------------------------------------------------------------
void PixelEditor::drawPreview(float deltaTime) {
  ImGui::Text("Preview");
  ImGui::Separator();

  // Update animation
  if (previewPlaying && frames.size() > 1) {
    previewTimer += deltaTime;
    if (previewTimer >= frameDuration) {
      previewTimer -= frameDuration;
      if (loopAnimation) {
        previewFrameIndex = (previewFrameIndex + 1) % (int)frames.size();
      } else if (previewFrameIndex < (int)frames.size() - 1) {
        previewFrameIndex++;
      }
    }
  }

  // Draw preview canvas
  int previewSize = 128;
  float pxSize = (float)previewSize / (float)std::max(canvasWidth, canvasHeight);
  ImVec2 canvasSz = ImVec2((float)canvasWidth * pxSize, (float)canvasHeight * pxSize);

  ImGui::InvisibleButton("##preview", canvasSz);
  ImVec2 pMin = ImGui::GetItemRectMin();
  ImDrawList* drawList = ImGui::GetWindowDrawList();

  auto& frame = frames[previewFrameIndex];
  for (int y = 0; y < canvasHeight; y++) {
    for (int x = 0; x < canvasWidth; x++) {
      int colorIdx = frame.pixels[y][x];
      if (colorIdx < 0 || colorIdx >= (int)palette.size()) continue;
      ImU32 col = getPaletteColor(colorIdx);
      drawList->AddRectFilled(
        ImVec2(pMin.x + x * pxSize, pMin.y + y * pxSize),
        ImVec2(pMin.x + (x + 1) * pxSize, pMin.y + (y + 1) * pxSize),
        col
      );
    }
  }

  ImGui::Spacing();
  if (ImGui::Button(previewPlaying ? "⏸" : "▶")) previewPlaying = !previewPlaying;
  ImGui::SameLine();
  if (ImGui::Button("◼")) { previewPlaying = false; previewFrameIndex = 0; previewTimer = 0.0f; }
  ImGui::SameLine();
  if (ImGui::Button("◄")) {
    previewFrameIndex = (previewFrameIndex - 1 + (int)frames.size()) % (int)frames.size();
    previewTimer = 0.0f;
  }
  ImGui::SameLine();
  if (ImGui::Button("►")) {
    previewFrameIndex = (previewFrameIndex + 1) % (int)frames.size();
    previewTimer = 0.0f;
  }
  ImGui::SameLine();
  ImGui::Text("%d/%d", previewFrameIndex + 1, (int)frames.size());
}

// ---------------------------------------------------------------------------
// Toolbar
// ---------------------------------------------------------------------------
void PixelEditor::drawToolbar() {
  const char* toolNames[] = {"✏ Pencil", "⌫ Eraser", "🪣 Fill", "╱ Line", "▭ Rect", "💧 Picker"};
  Tool tools[] = {Tool::Pencil, Tool::Eraser, Tool::Fill, Tool::Line, Tool::Rect, Tool::Picker};

  for (int i = 0; i < 6; i++) {
    bool active = (currentTool == tools[i]);
    if (ImGui::Button(toolNames[i])) {
      currentTool = tools[i];
    }
    if (i < 5) ImGui::SameLine();
  }

  ImGui::SameLine();
  ImGui::Separator();
  ImGui::SameLine();

  if (ImGui::Button("Undo (Ctrl+Z)")) undo();
  ImGui::SameLine();
  if (ImGui::Button("Redo (Ctrl+Y)")) redo();
}

// ---------------------------------------------------------------------------
// Main render
// ---------------------------------------------------------------------------
void PixelEditor::render() {
  ImGui::Begin("Pixel Editor");

  // Header: name, size, zoom
  char nameBuf[128];
  strncpy(nameBuf, spriteName.c_str(), sizeof(nameBuf) - 1);
  nameBuf[sizeof(nameBuf) - 1] = '\0';
  if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) {
    spriteName = nameBuf;
  }

  ImGui::SameLine();
  ImGui::Text("Size:");
  ImGui::SameLine();
  ImGui::PushItemWidth(50);
  int newW = canvasWidth, newH = canvasHeight;
  if (ImGui::DragInt("##w", &newW, 1, 1, 64)) {
    if (newW != canvasWidth) { newSprite(newW, canvasHeight); }
  }
  ImGui::SameLine();
  ImGui::Text("x");
  ImGui::SameLine();
  if (ImGui::DragInt("##h", &newH, 1, 1, 64)) {
    if (newH != canvasHeight) { newSprite(canvasWidth, newH); }
  }
  ImGui::PopItemWidth();

  ImGui::SameLine();
  ImGui::Text("Zoom:");
  ImGui::SameLine();
  ImGui::PushItemWidth(80);
  ImGui::DragInt("##zoom", &pixelSize, 1, 4, 24);
  ImGui::PopItemWidth();

  ImGui::SameLine();
  ImGui::Checkbox("Grid", &showGrid);

  drawToolbar();

  ImGui::Separator();

  // Main layout: canvas on left, palette+preview+frames on right
  float rightPanelWidth = 220.0f;
  float canvasAvailWidth = ImGui::GetContentRegionAvail().x - rightPanelWidth - 10.0f;
  canvasAvailWidth = std::max(200.0f, canvasAvailWidth);

  // Left: Canvas
  ImGui::BeginChild("CanvasArea", ImVec2(canvasAvailWidth, 0), true);
  drawCanvas();
  ImGui::EndChild();

  ImGui::SameLine();

  // Right: Palette, Preview, Frames
  ImGui::BeginChild("RightPanel", ImVec2(rightPanelWidth, 0), true);
  drawPalette();
  ImGui::Separator();
  drawPreview(ImGui::GetIO().DeltaTime);
  ImGui::Separator();
  drawFrames();
  ImGui::EndChild();

  ImGui::Separator();

  // Footer: actions
  if (ImGui::Button("Assign to Selected Entity")) {
    // This is handled externally via assignToEntity
    ImGui::OpenPopup("##noEntity");
  }
  ImGui::SameLine();
  if (ImGui::Button("Clear Frame")) {
    pushUndo();
    if (!frames.empty()) {
      auto& frame = frames[currentFrameIndex];
      for (auto& row : frame.pixels)
        std::fill(row.begin(), row.end(), -1);
      unsavedChanges = true;
    }
  }
  ImGui::SameLine();
  if (ImGui::Button("Export JSON")) {
    std::string jsonStr = exportJSON();
    // In a real app, this would trigger a file download (web) or save dialog (native)
    // For now, copy to clipboard is the closest ImGui equivalent
    ImGui::SetClipboardText(jsonStr.c_str());
    ImGui::LogToClipboard();
    ImGui::LogText("Sprite JSON copied to clipboard");
    ImGui::LogFinish();
  }
  ImGui::SameLine();
  if (ImGui::Button("Import JSON")) {
    // Would open a file dialog; for now, read from clipboard
    const char* clip = ImGui::GetClipboardText();
    if (clip) importJSON(clip);
  }

  ImGui::End();
}

// ---------------------------------------------------------------------------
// Atlas generation for entity assignment
// ---------------------------------------------------------------------------
GLuint PixelEditor::generateAtlas() const {
  int totalWidth = canvasWidth * (int)frames.size();
  int height = canvasHeight;
  std::vector<uint8_t> rgba(totalWidth * height * 4);

  for (size_t f = 0; f < frames.size(); f++) {
    for (int y = 0; y < canvasHeight; y++) {
      for (int x = 0; x < canvasWidth; x++) {
        int srcX = (int)f * canvasWidth + x;
        int dstIdx = (y * totalWidth + srcX) * 4;
        int colorIdx = frames[f].pixels[y][x];
        if (colorIdx < 0 || colorIdx >= (int)palette.size()) {
          rgba[dstIdx] = 0; rgba[dstIdx + 1] = 0;
          rgba[dstIdx + 2] = 0; rgba[dstIdx + 3] = 0;
        } else {
          auto& c = palette[colorIdx];
          rgba[dstIdx] = (uint8_t)(c.x * 255);
          rgba[dstIdx + 1] = (uint8_t)(c.y * 255);
          rgba[dstIdx + 2] = (uint8_t)(c.z * 255);
          rgba[dstIdx + 3] = (uint8_t)(c.w * 255);
        }
      }
    }
  }

  GLuint tex;
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, totalWidth, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D, 0);
  return tex;
}

void PixelEditor::assignToEntity(EntityManager& em, SpriteAnimator& anim, int entityIndex) const {
  if (entityIndex < 0 || entityIndex >= (int)em.getAll().size()) return;
  auto& e = em.get(entityIndex);
  if (!e.alive) return;

  GLuint tex = generateAtlas();

  e.hasRender = true;
  e.render.usePixelArtSprite = true;
  e.render.spriteAtlas = tex;
  e.render.spriteFrameWidth = canvasWidth;
  e.render.spriteFrameHeight = canvasHeight;
  e.render.spriteFrameCount = (int)frames.size();
  e.render.spriteCurrentFrame = 0;
  e.render.spriteScale = 0.06f;
  e.render.use3DGeometry = false;
  e.render.spriteSourceJSON = exportJSON();

  // Stop animation since we just assigned a static sprite from the editor
  anim.remove(entityIndex);
}

void PixelEditor::loadFromEntity(EntityManager& em, SpriteAnimator& anim, int entityIndex) {
  if (entityIndex < 0 || entityIndex >= (int)em.getAll().size()) return;
  auto& e = em.get(entityIndex);
  if (!e.alive || !e.hasRender || e.render.spriteSourceJSON.empty()) return;

  importJSON(e.render.spriteSourceJSON);
}

void PixelEditor::regenerateTextures(EntityManager& em) {
  auto& allEntities = em.getAll();
  for (size_t i = 0; i < allEntities.size(); i++) {
    auto& e = allEntities[i];
    if (e.alive && e.hasRender && !e.render.spriteSourceJSON.empty()) {
      PixelEditor temp;
      temp.importJSON(e.render.spriteSourceJSON);
      e.render.spriteAtlas = temp.generateAtlas();
      e.render.spriteFrameWidth = temp.canvasWidth;
      e.render.spriteFrameHeight = temp.canvasHeight;
      e.render.spriteFrameCount = (int)temp.frames.size();
      e.render.usePixelArtSprite = true;
    }
  }
}

// ---------------------------------------------------------------------------
// JSON import/export
// ---------------------------------------------------------------------------
std::string PixelEditor::exportJSON() const {
  json root;
  root["name"] = spriteName;
  root["width"] = canvasWidth;
  root["height"] = canvasHeight;
  root["frameDuration"] = frameDuration;
  root["loop"] = loopAnimation;

  root["palette"] = json::array();
  for (auto& c : palette) {
    root["palette"].push_back({c.x, c.y, c.z, c.w});
  }

  root["frames"] = json::array();
  for (auto& f : frames) {
    json fj;
    fj["name"] = f.name;
    fj["pixels"] = json::array();
    for (auto& row : f.pixels)
      for (int p : row)
        fj["pixels"].push_back(p);
    root["frames"].push_back(fj);
  }

  return root.dump(2);
}

void PixelEditor::importJSON(const std::string& jsonStr) {
  try {
    json root = json::parse(jsonStr);

    spriteName = root.value("name", "imported_sprite");
    canvasWidth = root.value("width", 16);
    canvasHeight = root.value("height", 16);
    frameDuration = root.value("frameDuration", 0.1f);
    loopAnimation = root.value("loop", true);

    // Load palette
    palette.clear();
    if (root.contains("palette")) {
      for (auto& pc : root["palette"]) {
        palette.push_back({pc[0].get<float>(), pc[1].get<float>(), pc[2].get<float>(), pc[3].get<float>()});
      }
    }

    // Load frames
    frames.clear();
    if (root.contains("frames")) {
      for (auto& fj : root["frames"]) {
        Frame f;
        f.name = fj.value("name", "Frame");
        f.pixels.resize(canvasHeight, std::vector<int>(canvasWidth, -1));
        if (fj.contains("pixels")) {
          int idx = 0;
          for (auto& p : fj["pixels"]) {
            int x = idx % canvasWidth;
            int y = idx / canvasWidth;
            if (y < canvasHeight) f.pixels[y][x] = p.get<int>();
            idx++;
          }
        }
        frames.push_back(std::move(f));
      }
    }

    currentFrameIndex = 0;
    previewFrameIndex = 0;
    undoStack.clear();
    redoStack.clear();
    unsavedChanges = false;
  } catch (const std::exception& ex) {
    std::cerr << "PixelEditor::importJSON error: " << ex.what() << "\n";
  }
}
