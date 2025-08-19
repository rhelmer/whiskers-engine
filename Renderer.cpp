// Renderer.cpp
#include "Renderer.h"

#include <SDL2/SDL.h>
#include <glad/glad.h>

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <string>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

const char *vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 vertexColor;
out vec2 TexCoord;  // Add this line

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    vertexColor = aPos * 0.5 + 0.5;
    TexCoord = aTexCoord;  // Add this line
}
)";

const char *fragmentShaderSource = R"(
#version 330 core
in vec3 vertexColor;
in vec2 TexCoord;  // Add this line
out vec4 FragColor;

uniform vec3 overrideColor;
uniform sampler2D spaceshipTexture;  // Add this line

void main()
{
    if (overrideColor == vec3(0.0)) {
        // Use texture instead of vertex color
        vec4 texColor = texture(spaceshipTexture, TexCoord);
        FragColor = texColor;
    } else {
        FragColor = vec4(overrideColor, 1.0);
    }
}
)";

Renderer::Renderer(int width, int height) : windowWidth(width), windowHeight(height) {
}

Renderer::~Renderer() {
  glDeleteBuffers(1, &shipVBO);
  glDeleteVertexArrays(1, &shipVAO);
  for (int i = 0; i < 3; i++) {
    glDeleteBuffers(1, &flameLayers[i].VBO);
    glDeleteVertexArrays(1, &flameLayers[i].VAO);
  }
  glDeleteProgram(shaderProgram);
}

GLuint Renderer::loadTexture(const std::string &filepath) {
  GLuint texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);

  int width, height, channels;
  unsigned char *data = stbi_load(filepath.c_str(), &width, &height, &channels, 0);

  if (data) {
    GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    std::cout << "Loaded texture: " << filepath << std::endl;
  } else {
    std::cout << "Failed to load texture: " << filepath << std::endl;
  }

  stbi_image_free(data);
  return texture;
}

GLuint Renderer::compileShader(GLenum type, const char *source) {
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, nullptr);
  glCompileShader(shader);
  GLint success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetShaderInfoLog(shader, 512, nullptr, infoLog);
    std::cerr << "Shader compilation failed: " << infoLog << "\n";
    return 0;
  }
  return shader;
}

GLuint Renderer::createShaderProgram() {
  GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
  GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
  GLuint program = glCreateProgram();

  glAttachShader(program, vertexShader);
  glAttachShader(program, fragmentShader);
  glLinkProgram(program);

  GLint success;
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetProgramInfoLog(program, 512, nullptr, infoLog);
    std::cerr << "Shader linking failed: " << infoLog << "\n";
    return 0;
  }

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  return program;
}

void Renderer::setupShip() {
  // Ship triangle vertices with rotated texture coordinates
  float shipVertices[] = {
      // Position (x,y,z)    Texture coords (u,v) - rotated 180°
      0.0f,  0.2f,  0.0f, 0.5f, 0.0f,  // apex -> bottom center of texture
      0.2f,  -0.2f, 0.0f, 0.0f, 1.0f,  // right base -> top left of texture
      -0.2f, -0.2f, 0.0f, 1.0f, 1.0f   // left base -> top right of texture
  };

  glGenVertexArrays(1, &shipVAO);
  glGenBuffers(1, &shipVBO);

  glBindVertexArray(shipVAO);
  glBindBuffer(GL_ARRAY_BUFFER, shipVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(shipVertices), shipVertices, GL_STATIC_DRAW);

  // Position attribute (location = 0)
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  // Texture coordinate attribute (location = 1)
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

void Renderer::setupFlames() {
  auto setupFlameLayer = [&](FlameLayer &layer, float *vertices, size_t size, float scaleBase,
                             glm::vec3 color, float flickerSpeed, float flickerMagnitude,
                             float flickerPosMagnitude) {
    glGenVertexArrays(1, &layer.VAO);
    glGenBuffers(1, &layer.VBO);

    glBindVertexArray(layer.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, layer.VBO);
    glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    layer.scaleBase = scaleBase;
    layer.color = color;
    layer.flickerSpeed = flickerSpeed;
    layer.flickerMagnitude = flickerMagnitude;
    layer.flickerPosMagnitude = flickerPosMagnitude;
  };

  setupFlameLayer(flameLayers[0], flame1, sizeof(flame1), 0.45f, glm::vec3(1.0f, 0.2f, 0.0f), 7.0f,
                  0.1f, 0.02f);
  setupFlameLayer(flameLayers[1], flame2, sizeof(flame2), 0.35f, glm::vec3(1.0f, 0.6f, 0.0f), 12.0f,
                  0.12f, 0.03f);
  setupFlameLayer(flameLayers[2], flame3, sizeof(flame3), 0.25f, glm::vec3(1.0f, 1.0f, 0.3f), 20.0f,
                  0.15f, 0.04f);
}

bool Renderer::init() {
  shaderProgram = createShaderProgram();
  if (!shaderProgram) return false;

  setupShip();
  setupFlames();

  glEnable(GL_DEPTH_TEST);

  // Add these lines for transparency:
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  modelLoc = glGetUniformLocation(shaderProgram, "model");
  viewLoc = glGetUniformLocation(shaderProgram, "view");
  projLoc = glGetUniformLocation(shaderProgram, "projection");
  overrideColorLoc = glGetUniformLocation(shaderProgram, "overrideColor");

  spaceshipTexture = loadTexture("stellar_whiskers_spaceship.png");

  return true;
}

void Renderer::clear() {
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);  // Pure black background
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::present() {
  // SDL_GL_SwapWindow should be called in main loop after renderShip
  // so this can be empty or you can add a pointer to SDL_Window if you want
}

void Renderer::renderShip(const Entity &ship, bool thrusting) {
  glUseProgram(shaderProgram);

  glm::mat4 view = glm::mat4(1.0f);
  glm::mat4 projection = glm::ortho(-1.f, 1.f, -1.f, 1.f, -1.f, 1.f);

  glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
  glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

  float scale = shipScale;

  glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(ship.position, 0.0f)) *
                    glm::rotate(glm::mat4(1.0f), glm::radians(ship.angle), glm::vec3(0, 0, 1)) *
                    glm::scale(glm::mat4(1.0f), glm::vec3(scale, scale, 1.0f));

  glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
  glUniform3f(overrideColorLoc, 0.0f, 0.0f, 0.0f);  // no override color

  glBindVertexArray(shipVAO);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, spaceshipTexture);
  glUniform1i(glGetUniformLocation(shaderProgram, "spaceshipTexture"), 0);

  glBindVertexArray(shipVAO);
  glDrawArrays(GL_TRIANGLES, 0, 3);
  glDrawArrays(GL_TRIANGLES, 0, 3);

  if (thrusting) {
    float rad = glm::radians(ship.angle + 90.0f);
    glm::vec2 backwardDir = glm::vec2(cos(rad), sin(rad)) * -1.0f;

    float baseOffset = 0.15f;  // REDUCE from 0.3f - moves flames closer to ship

    Uint32 currentTicks = SDL_GetTicks();

    for (int i = 0; i < 3; i++) {
      const FlameLayer &flame = flameLayers[i];
      float flickerTime = currentTicks * flame.flickerSpeed * 0.001f;

      // REDUCE the scale multiplier to make flames smaller
      float scaleFlicker = (flame.scaleBase * 0.6f) +
                           flame.flickerMagnitude * std::sin(flickerTime * 3.14159f * 2.0f);

      float posFlicker = flame.flickerPosMagnitude * std::sin(flickerTime * 7.0f);

      glm::vec2 flickerOffset(posFlicker, 0.0f);

      // REDUCE the spacing between flame layers
      glm::vec2 flamePos = ship.position + backwardDir * (baseOffset + i * 0.02f) + flickerOffset;

      glm::mat4 flameModel =
          glm::translate(glm::mat4(1.0f), glm::vec3(flamePos, 0.0f)) *
          glm::rotate(glm::mat4(1.0f), glm::radians(ship.angle), glm::vec3(0, 0, 1)) *
          glm::scale(glm::mat4(1.0f), glm::vec3(scaleFlicker, scaleFlicker, 1.0f));

      glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(flameModel));
      glUniform3f(overrideColorLoc, flame.color.r, flame.color.g, flame.color.b);

      glBindVertexArray(flame.VAO);
      glDrawArrays(GL_TRIANGLES, 0, 3);
    }
  }
}

void Renderer::renderAsteroid(const Entity &asteroid) {
  // draw simple circle or textured quad
}

void Renderer::renderBullet(const Entity &bullet) {
  glUseProgram(shaderProgram);

  glm::mat4 view = glm::mat4(1.0f);
  glm::mat4 projection = glm::ortho(-1.f, 1.f, -1.f, 1.f, -1.f, 1.f);
  glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
  glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

  glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(bullet.position, 0.0f)) *
                    glm::scale(glm::mat4(1.0f), glm::vec3(0.02f, 0.02f, 1.0f));
  glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
  glUniform3f(overrideColorLoc, 1.0f, 1.0f, 0.0f);  // yellow bullets

  // Draw as a simple triangle (reuse ship VAO for now)
  glBindVertexArray(shipVAO);
  glDrawArrays(GL_TRIANGLES, 0, 3);
}
