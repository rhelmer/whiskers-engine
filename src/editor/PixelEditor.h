// PixelEditor.h
// Pixel art sprite editor using Dear ImGui.
#pragma once

#include <imgui.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include "../core/EntityManager.h"

class SpriteAnimator;

#ifdef WHISKERS_WASM_BUILD
#include <whiskers/glad_wasm.h>
#else
#include <glad/glad.h>
#endif

class PixelEditor {
 public:
  PixelEditor();
  ~PixelEditor();

  void newSprite(int width = 16, int height = 16);
  void render();
  GLuint generateAtlas() const;
  void assignToEntity(EntityManager& em, SpriteAnimator& anim, int entityIndex) const;
  void loadFromEntity(EntityManager& em, SpriteAnimator& anim, int entityIndex);
  static void regenerateTextures(EntityManager& em);
  std::string exportJSON() const;
  void importJSON(const std::string& json);

  const std::string& getName() const { return spriteName; }
  void setName(const std::string& name) { spriteName = name; }
  bool hasUnsavedChanges() const { return unsavedChanges; }

 private:
  // Canvas state
  int canvasWidth{16}, canvasHeight{16};
  int pixelSize{12};
  bool showGrid{true};

  // Frame data: frames[row][col] = palette index, -1 = transparent
  struct Frame {
    std::vector<std::vector<int>> pixels;
    std::string name;
  };
  std::vector<Frame> frames;
  int currentFrameIndex{0};

  // Palette (ImVec4 = RGBA float)
  std::vector<ImVec4> palette;
  int selectedColorIndex{1};
  bool showColorPicker{false};

  // Tools
  enum class Tool { Pencil, Eraser, Fill, Line, Rect, Picker };
  Tool currentTool{Tool::Pencil};

  // Drawing state
  bool isDrawing{false};
  int lastMouseX{-1}, lastMouseY{-1};
  int lineStartX{-1}, lineStartY{-1};
  int rectStartX{-1}, rectStartY{-1};

  // Undo/redo (stores 2D pixel arrays)
  std::vector<std::vector<std::vector<int>>> undoStack;
  std::vector<std::vector<std::vector<int>>> redoStack;

  // Preview
  bool previewPlaying{true};
  float previewTimer{0.0f};
  int previewFrameIndex{0};
  float frameDuration{0.1f};
  bool loopAnimation{true};

  // Sprite name
  std::string spriteName{"untitled"};
  bool unsavedChanges{false};

  // Helpers
  void drawCanvas();
  void drawPalette();
  void drawFrames();
  void drawPreview(float deltaTime);
  void drawToolbar();

  void setPixel(int x, int y, int color);
  int getPixel(int x, int y) const;
  void floodFill(int startX, int startY, int newColor);
  void drawLineBresenham(int x0, int y0, int x1, int y1, int color);
  void drawFilledRect(int x0, int y0, int x1, int y1, int color);

  void pushUndo();
  void undo();
  void redo();

  void addFrame();
  void deleteFrame(int index);
  void duplicateFrame(int index);

  ImU32 getPaletteColor(int index) const;
};
