// Renderer.h
// Platformer renderer — replaces the old space-shooter Renderer.
// Uses the new ECS components and camera system.
#pragma once

#ifdef WHISKERS_WASM_BUILD
#include <whiskers/glad_wasm.h>
#else
#include <glad/glad.h>
#endif
#include <glm/glm.hpp>
#include <string>
#include <vector>

#include <vector>

#include "../core/EntityManager.h"
#include "CameraSystem.h"
#include "TilemapRenderer.h"

class Renderer {
 public:
  Renderer(int width, int height);
  ~Renderer();

  bool init();
  void clear();
  void setViewportSize(int width, int height);
  void render(EntityManager& em);
  void renderHUD(EntityManager& em);  // new: HUD overlay

  // RGBA framebuffer → PNG bytes (call with GL context bound, after rendering).
  std::vector<unsigned char> captureFramebufferPng(int reqWidth, int reqHeight);

  int getViewportWidth() const { return windowWidth; }
  int getViewportHeight() const { return windowHeight; }

  CameraSystem& getCamera() { return camera; }
  const CameraSystem& getCamera() const { return camera; }

  TilemapRenderer& getTilemapRenderer() { return tilemapRenderer; }

 private:
  int windowWidth, windowHeight;

  CameraSystem camera;
  TilemapRenderer tilemapRenderer;

  // Shader and VAO for rendering entities (3D boxes)
  GLuint shaderProgram = 0;
  GLuint entityVAO = 0, entityVBO = 0;

  // Shader for pixel art sprites (atlas-based rendering)
  GLuint spriteShader = 0;
  GLuint spriteVAO = 0, spriteVBO = 0;

  // HUD shader and VAO (screen-space, no camera transform)
  GLuint hudShader = 0;
  GLuint hudVAO = 0, hudVBO = 0;

  // Uniform locations
  GLuint modelLoc = 0, viewLoc = 0, projLoc = 0, colorLoc = 0;
  GLuint spriteModelLoc = 0, spriteViewLoc = 0, spriteProjLoc = 0;
  GLuint spriteTexLoc = 0, spriteFrameLoc = 0, spriteFrameCountLoc = 0;
  GLuint spriteFlipXLoc = 0;

  // Textures
  std::vector<GLuint> textures;
  GLuint loadTexture(const std::string& filepath);

  // Shaders
  GLuint compileShader(GLenum type, const char* source);
  GLuint createShaderProgram();
  GLuint createSpriteShader();
  GLuint createHUDShader();

  // Render a single entity
  void renderEntity(const EntityRecord& e);

  // Render parallax background layers
  void renderParallax(const std::vector<struct ParallaxLayer>& layers, glm::vec2 camPos);

  // Unit quad vertices (centered, 1x1) — for textured sprites
  float quadVertices[30] = {
    // pos (x,y,z)      texcoord (u,v)
    -0.5f, -0.5f, 0.0f,  0.0f, 0.0f,
     0.5f, -0.5f, 0.0f,  1.0f, 0.0f,
     0.5f,  0.5f, 0.0f,  1.0f, 1.0f,
     0.5f,  0.5f, 0.0f,  1.0f, 1.0f,
    -0.5f,  0.5f, 0.0f,  0.0f, 1.0f,
    -0.5f, -0.5f, 0.0f,  0.0f, 0.0f,
  };

  // Unit cube vertices for 3D geometry
  float cubeVertices[108] = {
    // Front
    -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
     0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f,
    // Back
    -0.5f, -0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,
     0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
    // Left
    -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
    -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,
    // Right
     0.5f,  0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,
     0.5f, -0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,
    // Top
    -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,
     0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f,
    // Bottom
    -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,
     0.5f, -0.5f, -0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,
  };
};
