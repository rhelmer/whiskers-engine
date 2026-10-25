// TilemapRenderer.cpp
#include "TilemapRenderer.h"

#ifdef WHISKERS_WASM_BUILD
#include <whiskers/glad_wasm.h>
#else
#include <glad/glad.h>
#endif
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

static const char* tileVertSrc = R"(
layout (location = 0) in vec3 aPos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
void main() {
  gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

static const char* tileFragSrc = R"(
out vec4 FragColor;
uniform vec3 tileColor;
void main() {
  FragColor = vec4(tileColor, 1.0);
}
)";

void TilemapRenderer::init() {
  shaderProgram = createShaderProgram();
  if (!shaderProgram) {
    std::cerr << "Failed to create tilemap shader program\n";
    return;
  }

  glGenVertexArrays(1, &tileVAO);
  glGenBuffers(1, &tileVBO);

  glBindVertexArray(tileVAO);
  glBindBuffer(GL_ARRAY_BUFFER, tileVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  modelLoc = glGetUniformLocation(shaderProgram, "model");
  viewLoc = glGetUniformLocation(shaderProgram, "view");
  projLoc = glGetUniformLocation(shaderProgram, "projection");
  colorLoc = glGetUniformLocation(shaderProgram, "tileColor");
}

void TilemapRenderer::render(const TilemapLayer& layer, float tileSize, glm::vec2 cameraOffset) {
  if (!shaderProgram || layer.tiles.empty()) return;

  glUseProgram(shaderProgram);

  glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(-cameraOffset, 0.0f));
  // Orthographic projection — match the main renderer
  float orthoWidth = 20.0f;  // will be overridden by camera system
  float aspect = 800.0f / 600.0f;
  glm::mat4 projection = glm::ortho(
    -orthoWidth * 0.5f, orthoWidth * 0.5f,
    -orthoWidth * 0.5f / aspect, orthoWidth * 0.5f / aspect
  );

  glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
  glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

  glBindVertexArray(tileVAO);

  for (int y = 0; y < layer.rows; ++y) {
    for (int x = 0; x < layer.columns; ++x) {
      TileDef t = layer.getTile(x, y);
      if (t.type == TileType::Empty || t.type == TileType::Decorative) continue;

      glm::vec3 color;
      switch (t.type) {
        case TileType::Solid:    color = glm::vec3(0.5f, 0.35f, 0.2f); break;
        case TileType::OneWay:   color = glm::vec3(0.4f, 0.5f, 0.3f); break;
        case TileType::Hazard:   color = glm::vec3(0.9f, 0.2f, 0.2f); break;
        case TileType::Conveyor: color = glm::vec3(0.3f, 0.3f, 0.5f); break;
        case TileType::SlopeUp:
        case TileType::SlopeDown: color = glm::vec3(0.55f, 0.4f, 0.25f); break;
        default: color = glm::vec3(0.7f); break;
      }

      glUniform3f(colorLoc, color.r, color.g, color.b);

      // Tile world position (tile origin at center of tile)
      glm::vec3 worldPos((x + 0.5f) * tileSize, -(y + 0.5f) * tileSize, 0.0f);

      glm::mat4 model = glm::translate(glm::mat4(1.0f), worldPos) *
                        glm::scale(glm::mat4(1.0f), glm::vec3(tileSize, tileSize, tileSize * 0.5f));

      glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
      glDrawArrays(GL_TRIANGLES, 0, 36);
    }
  }

  glBindVertexArray(0);
}

GLuint TilemapRenderer::compileShader(GLenum type, const char* source) {
#ifdef WHISKERS_WASM_BUILD
    const char* header = "#version 300 es\nprecision mediump float;\n";
#else
    const char* header = "#version 330 core\n";
#endif

    const char* sources[] = { header, source };
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 2, sources, nullptr);
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

GLuint TilemapRenderer::createShaderProgram() {
  GLuint vs = compileShader(GL_VERTEX_SHADER, tileVertSrc);
  GLuint fs = compileShader(GL_FRAGMENT_SHADER, tileFragSrc);
  GLuint program = glCreateProgram();
  glAttachShader(program, vs);
  glAttachShader(program, fs);
  glLinkProgram(program);

  GLint success;
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetProgramInfoLog(program, 512, nullptr, infoLog);
    std::cerr << "Shader linking failed: " << infoLog << "\n";
    return 0;
  }

  glDeleteShader(vs);
  glDeleteShader(fs);
  return program;
}
