// TilemapRenderer.h
// Renders tilemap layers as 3D box geometry.
#pragma once

#ifdef WHISKERS_WASM_BUILD
#include <whiskers/glad_wasm.h>
#else
#include <glad/glad.h>
#endif
#include <glm/glm.hpp>
#include <vector>
#include <string>

#include "../level/Tilemap.h"

class TilemapRenderer {
 public:
  void init();
  void render(const TilemapLayer& layer, float tileSize, glm::vec2 cameraOffset);

 private:
  GLuint tileVAO = 0, tileVBO = 0;
  GLuint shaderProgram = 0;
  GLuint modelLoc = 0, viewLoc = 0, projLoc = 0, colorLoc = 0;

  // Unit cube vertices (centered at origin, size 1x1x1)
  float cubeVertices[108] = {
    // Front face
    -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
     0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f,
    // Back face
    -0.5f, -0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,
     0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
    // Left face
    -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
    -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,
    // Right face
     0.5f,  0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,
     0.5f, -0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,
    // Top face
    -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,
     0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f,
    // Bottom face
    -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,
     0.5f, -0.5f, -0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,
  };

  GLuint compileShader(GLenum type, const char* source);
  GLuint createShaderProgram();
};
