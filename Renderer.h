// Renderer.h
#pragma once
#include <glm/glm.hpp>
#include <string>

#include "Entity.h"

#ifndef RENDERER_H
#define RENDERER_H

#include "glad/glad.h"  // for GLuint, GLenum, GLADloadproc, etc.

#endif  // RENDERER_H

class Renderer {
 public:
  Renderer(int width, int height);
  ~Renderer();

  bool init();
  void clear();
  void present();
  void renderShip(const Entity &ship, bool thrusting);
  void renderAsteroid(const Entity &asteroid);
  void renderBullet(const Entity &bullet);

 private:
  GLuint spaceshipTexture = 0;
  GLuint loadTexture(const std::string &filepath);
  void setupShip();
  void setupFlames();

  GLuint compileShader(GLenum type, const char *source);
  GLuint createShaderProgram();

  GLuint shaderProgram = 0;

  GLuint shipVAO = 0, shipVBO = 0;

  struct FlameLayer {
    GLuint VAO = 0, VBO = 0;
    float scaleBase;
    glm::vec3 color;
    float flickerSpeed;
    float flickerMagnitude;
    float flickerPosMagnitude;
  };

  FlameLayer flameLayers[3];

  int windowWidth;
  int windowHeight;

  // Flame vertices (copied from your original)
  float flame1[9] = {0.0f, 0.0f, 0.0f, -0.15f, -0.4f, 0.0f, 0.15f, -0.4f, 0.0f};
  float flame2[9] = {0.0f, 0.0f, 0.0f, -0.10f, -0.3f, 0.0f, 0.10f, -0.3f, 0.0f};
  float flame3[9] = {0.0f, 0.0f, 0.0f, -0.05f, -0.15f, 0.0f, 0.05f, -0.15f, 0.0f};

  // ship scale used in clamp
  const float shipScale = 0.5f;

  GLuint modelLoc = 0, viewLoc = 0, projLoc = 0, overrideColorLoc = 0;
};
