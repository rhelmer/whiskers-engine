// Renderer.cpp
#include "Renderer.h"

#include <SDL2/SDL.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <algorithm>
#include <cstdlib>

#define STB_IMAGE_IMPLEMENTATION
#include "../../stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../../include/stb_image_write.h"

// ---------------------------------------------------------------------------
// Shaders
// ---------------------------------------------------------------------------

static const char* entityVertSrc = R"(
layout (location = 0) in vec3 aPos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
void main() {
  gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

static const char* entityFragSrc = R"(
out vec4 FragColor;
uniform vec3 entityColor;
void main() {
  FragColor = vec4(entityColor, 1.0);
}
)";

// Sprite shader — takes a texture atlas (horizontal strip) and a frame index.
static const char* spriteVertSrc = R"(
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform int frameIndex;      // which frame in the atlas strip
uniform int frameCount;      // total frames
uniform bool flipX;          // horizontal flip
out vec2 TexCoord;
void main() {
  gl_Position = projection * view * model * vec4(aPos, 1.0);

  // Compute UVs for the correct frame in the atlas
  float frameWidth = 1.0 / float(frameCount);
  float u = aTexCoord.x * frameWidth + float(frameIndex) * frameWidth;

  // Apply horizontal flip
  if (flipX) {
    u = (1.0 - aTexCoord.x) * frameWidth + float(frameIndex) * frameWidth;
  }

  // Flip Y — our pixel art data is stored top-to-bottom but OpenGL
  // texture origin is bottom-left.
  TexCoord = vec2(u, 1.0 - aTexCoord.y);
}
)";

static const char* spriteFragSrc = R"(
in vec2 TexCoord;
out vec4 FragColor;
uniform sampler2D spriteTexture;
void main() {
  vec4 color = texture(spriteTexture, TexCoord);
  if (color.a < 0.1) discard;
  FragColor = color;
}
)";

// HUD shader — screen-space NDC coordinates (-1 to 1), no camera transform
static const char* hudVertSrc = R"(
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
out vec2 TexCoord;
void main() {
  gl_Position = vec4(aPos, 0.0, 1.0);
  TexCoord = aTexCoord;
}
)";

static const char* hudFragSrc = R"(
in vec2 TexCoord;
out vec4 FragColor;
uniform vec4 hudColor;
uniform sampler2D hudTexture;
uniform bool useTexture;
void main() {
  if (useTexture) {
    FragColor = texture(hudTexture, TexCoord);
  } else {
    FragColor = hudColor;
  }
}
)";

// ---------------------------------------------------------------------------
// Renderer implementation
// ---------------------------------------------------------------------------

Renderer::Renderer(int width, int height)
    : windowWidth(width), windowHeight(height) {}

Renderer::~Renderer() {
  glDeleteVertexArrays(1, &entityVAO);
  glDeleteBuffers(1, &entityVBO);
  glDeleteVertexArrays(1, &spriteVAO);
  glDeleteBuffers(1, &spriteVBO);
  glDeleteVertexArrays(1, &hudVAO);
  glDeleteBuffers(1, &hudVBO);
  if (shaderProgram) glDeleteProgram(shaderProgram);
  if (spriteShader) glDeleteProgram(spriteShader);
  if (hudShader) glDeleteProgram(hudShader);
  for (auto& tex : textures) {
    if (tex) glDeleteTextures(1, &tex);
  }
}

GLuint Renderer::loadTexture(const std::string& filepath) {
  GLuint texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);

  int width, height, channels;
  unsigned char* data = stbi_load(filepath.c_str(), &width, &height, &channels, 0);

  if (data) {
    GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    textures.push_back(texture);
  } else {
    std::cerr << "Failed to load texture: " << filepath << "\n";
    glDeleteTextures(1, &texture);
    return 0;
  }

  stbi_image_free(data);
  return texture;
}

GLuint Renderer::compileShader(GLenum type, const char* source) {
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

GLuint Renderer::createShaderProgram() {
  GLuint vs = compileShader(GL_VERTEX_SHADER, entityVertSrc);
  GLuint fs = compileShader(GL_FRAGMENT_SHADER, entityFragSrc);
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

GLuint Renderer::createSpriteShader() {
  GLuint vs = compileShader(GL_VERTEX_SHADER, spriteVertSrc);
  GLuint fs = compileShader(GL_FRAGMENT_SHADER, spriteFragSrc);
  GLuint program = glCreateProgram();
  glAttachShader(program, vs);
  glAttachShader(program, fs);
  glLinkProgram(program);

  GLint success;
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetProgramInfoLog(program, 512, nullptr, infoLog);
    std::cerr << "Sprite shader linking failed: " << infoLog << "\n";
    return 0;
  }

  glDeleteShader(vs);
  glDeleteShader(fs);
  return program;
}

GLuint Renderer::createHUDShader() {
  GLuint vs = compileShader(GL_VERTEX_SHADER, hudVertSrc);
  GLuint fs = compileShader(GL_FRAGMENT_SHADER, hudFragSrc);
  GLuint program = glCreateProgram();
  glAttachShader(program, vs);
  glAttachShader(program, fs);
  glLinkProgram(program);

  GLint success;
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetProgramInfoLog(program, 512, nullptr, infoLog);
    std::cerr << "HUD shader linking failed: " << infoLog << "\n";
    return 0;
  }

  glDeleteShader(vs);
  glDeleteShader(fs);
  return program;
}

bool Renderer::init() {
  shaderProgram = createShaderProgram();
  if (!shaderProgram) return false;

  spriteShader = createSpriteShader();
  hudShader = createHUDShader();

  // Entity VAO (colored geometry — cube)
  glGenVertexArrays(1, &entityVAO);
  glGenBuffers(1, &entityVBO);
  glBindVertexArray(entityVAO);
  glBindBuffer(GL_ARRAY_BUFFER, entityVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  // Sprite VAO (textured quad with texcoords)
  glGenVertexArrays(1, &spriteVAO);
  glGenBuffers(1, &spriteVBO);
  glBindVertexArray(spriteVAO);
  glBindBuffer(GL_ARRAY_BUFFER, spriteVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  // HUD VAO (screen-space quad, NDC coords)
  float hudQuad[24] = {
    // pos (x,y)    texcoord (u,v)   — NDC -1 to 1
    -1.0f, -1.0f,   0.0f, 0.0f,
     1.0f, -1.0f,   1.0f, 0.0f,
     1.0f,  1.0f,   1.0f, 1.0f,
     1.0f,  1.0f,   1.0f, 1.0f,
    -1.0f,  1.0f,   0.0f, 1.0f,
    -1.0f, -1.0f,   0.0f, 0.0f,
  };
  glGenVertexArrays(1, &hudVAO);
  glGenBuffers(1, &hudVBO);
  glBindVertexArray(hudVAO);
  glBindBuffer(GL_ARRAY_BUFFER, hudVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(hudQuad), hudQuad, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
  glEnableVertexAttribArray(1);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  modelLoc = glGetUniformLocation(shaderProgram, "model");
  viewLoc = glGetUniformLocation(shaderProgram, "view");
  projLoc = glGetUniformLocation(shaderProgram, "projection");
  colorLoc = glGetUniformLocation(shaderProgram, "entityColor");

  spriteModelLoc = glGetUniformLocation(spriteShader, "model");
  spriteViewLoc = glGetUniformLocation(spriteShader, "view");
  spriteProjLoc = glGetUniformLocation(spriteShader, "projection");
  spriteTexLoc = glGetUniformLocation(spriteShader, "spriteTexture");
  spriteFrameLoc = glGetUniformLocation(spriteShader, "frameIndex");
  spriteFrameCountLoc = glGetUniformLocation(spriteShader, "frameCount");
  spriteFlipXLoc = glGetUniformLocation(spriteShader, "flipX");

  tilemapRenderer.init();

  glViewport(0, 0, windowWidth, windowHeight);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  return true;
}

void Renderer::clear() {
  glClearColor(0.04f, 0.03f, 0.09f, 1.0f);  // deep underworld indigo
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::setViewportSize(int width, int height) {
  windowWidth = width;
  windowHeight = height;
  glViewport(0, 0, width, height);
}

void Renderer::render(EntityManager& em) {
  // Render entities (sorted by sortOrder)
  auto renderIndices = em.queryWithRender();

  std::sort(renderIndices.begin(), renderIndices.end(),
    [&em](size_t a, size_t b) {
      return em.get(a).render.sortOrder < em.get(b).render.sortOrder;
    });

  for (size_t idx : renderIndices) {
    const auto& e = em.get(idx);
    if (!e.render.visible) continue;
    renderEntity(e);
  }
}

void Renderer::renderEntity(const EntityRecord& e) {
  if (e.render.use3DGeometry) {
    // Render as 3D box
    glUseProgram(shaderProgram);

    float aspect = (float)windowWidth / (float)windowHeight;
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 projection = camera.getProjectionMatrix(aspect);

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3f(colorLoc, e.render.color.r, e.render.color.g, e.render.color.b);

    glm::mat4 model = glm::translate(glm::mat4(1.0f), e.transform.position) *
                      glm::rotate(glm::mat4(1.0f), glm::radians(e.transform.rotation.z), glm::vec3(0, 0, 1)) *
                      glm::scale(glm::mat4(1.0f), e.transform.scale);

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glBindVertexArray(entityVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
  } else if (e.render.usePixelArtSprite && e.render.spriteAtlas != 0) {
    // Render as pixel art sprite from atlas
    glUseProgram(spriteShader);

    float aspect = (float)windowWidth / (float)windowHeight;
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 projection = camera.getProjectionMatrix(aspect);

    glUniformMatrix4fv(spriteViewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(spriteProjLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Sprite size in world units
    float spriteWorldWidth = (float)e.render.spriteFrameWidth * e.render.spriteScale;
    float spriteWorldHeight = (float)e.render.spriteFrameHeight * e.render.spriteScale;

    glm::mat4 model = glm::translate(glm::mat4(1.0f), e.transform.position) *
                      glm::scale(glm::mat4(1.0f), glm::vec3(spriteWorldWidth, spriteWorldHeight, 1.0f));

    glUniformMatrix4fv(spriteModelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(spriteFrameLoc, e.render.spriteCurrentFrame);
    glUniform1i(spriteFrameCountLoc, std::max(1, e.render.spriteFrameCount));
    glUniform1i(spriteFlipXLoc, e.render.flipX ? 1 : 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, (GLuint)e.render.spriteAtlas);
    glUniform1i(spriteTexLoc, 0);

    glBindVertexArray(spriteVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
  } else {
    // Fallback: colored quad (no sprite texture)
    glUseProgram(shaderProgram);

    float aspect = (float)windowWidth / (float)windowHeight;
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 projection = camera.getProjectionMatrix(aspect);

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3f(colorLoc, e.render.color.r, e.render.color.g, e.render.color.b);

    glm::mat4 model = glm::translate(glm::mat4(1.0f), e.transform.position) *
                      glm::scale(glm::mat4(1.0f), glm::vec3(e.transform.scale.x, e.transform.scale.y, 1.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glBindVertexArray(entityVAO);
    // Draw just the front face (6 vertices)
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
  }
}

void Renderer::renderParallax(const std::vector<ParallaxLayer>& layers, glm::vec2 camPos) {
  (void)layers;
  (void)camPos;
}

// ---------------------------------------------------------------------------
// HUD Rendering
// ---------------------------------------------------------------------------

// Helper: draw a filled rect in screen-space NDC coords.
static void drawHUDRect(float x, float y, float w, float h, glm::vec3 color, GLuint hudVAO, GLuint hudShader) {
  // Convert world-ish coords to NDC (-1 to 1)
  float ndcX = -1.0f + x * 2.0f;
  float ndcY = 1.0f - y * 2.0f;
  float ndcW = w * 2.0f;
  float ndcH = h * 2.0f;

  float verts[24] = {
    ndcX,      ndcY,       0.0f, 0.0f,
    ndcX+ndcW, ndcY,       1.0f, 0.0f,
    ndcX+ndcW, ndcY+ndcH,  1.0f, 1.0f,
    ndcX+ndcW, ndcY+ndcH,  1.0f, 1.0f,
    ndcX,      ndcY+ndcH,  0.0f, 1.0f,
    ndcX,      ndcY,       0.0f, 0.0f,
  };

  glUseProgram(hudShader);
  GLuint hudColorLoc = glGetUniformLocation(hudShader, "hudColor");
  glUniform4f(hudColorLoc, color.r, color.g, color.b, 1.0f);
  glUniform1i(glGetUniformLocation(hudShader, "useTexture"), 0);

  glDisable(GL_DEPTH_TEST);

  glBindVertexArray(hudVAO);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glBindVertexArray(0);

  glEnable(GL_DEPTH_TEST);
}

// Helper: draw a textured quad in screen-space NDC coords.
static void drawHUDTexture(float x, float y, float w, float h, GLuint tex, GLuint hudVAO, GLuint hudShader) {
  float ndcX = -1.0f + x * 2.0f;
  float ndcY = 1.0f - y * 2.0f;
  float ndcW = w * 2.0f;
  float ndcH = h * 2.0f;

  float verts[24] = {
    ndcX,      ndcY,       0.0f, 0.0f,
    ndcX+ndcW, ndcY,       1.0f, 0.0f,
    ndcX+ndcW, ndcY+ndcH,  1.0f, 1.0f,
    ndcX+ndcW, ndcY+ndcH,  1.0f, 1.0f,
    ndcX,      ndcY+ndcH,  0.0f, 1.0f,
    ndcX,      ndcY,       0.0f, 0.0f,
  };

  glUseProgram(hudShader);
  glUniform1i(glGetUniformLocation(hudShader, "useTexture"), 1);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, tex);
  glUniform1i(glGetUniformLocation(hudShader, "hudTexture"), 0);

  glDisable(GL_DEPTH_TEST);

  glBindVertexArray(hudVAO);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glBindVertexArray(0);

  glEnable(GL_DEPTH_TEST);
}

void Renderer::renderHUD(EntityManager& em) {
  // Find the player entity (first one with input component)
  auto inputIndices = em.queryWithInput();
  if (inputIndices.empty()) return;

  const auto& player = em.get(inputIndices[0]);
  if (!player.hasHealth) return;

  glDisable(GL_DEPTH_TEST);

  // HUD layout (NDC 0..1): parcels, bonus icons, mana, coins

  float hudY = 0.03f;      // top of screen
  float iconSize = 0.04f;  // 4% of screen height
  float iconGap = 0.005f;
  float barHeight = 0.02f;

  float hudX = 0.02f;

  // ========== Messenger / parcel HUD ==========
  
  // Draw held letters (envelope icons with borders)
  if (player.hasMessenger) {
    const auto& messenger = player.messenger;
    
    // Letter icons
    for (size_t i = 0; i < messenger.heldLetters.size(); i++) {
      drawHUDRect(hudX - 0.002f, hudY - 0.002f, iconSize + 0.004f, iconSize + 0.004f,
                 glm::vec3(0.5f, 0.4f, 0.7f), hudVAO, hudShader);
      drawHUDRect(hudX, hudY, iconSize, iconSize, glm::vec3(0.8f, 0.6f, 1.0f), hudVAO, hudShader);
      hudX += iconSize + iconGap;
    }
    
    // Empty slot
    if (messenger.heldLetters.empty()) {
      drawHUDRect(hudX, hudY, iconSize, iconSize, glm::vec3(0.25f, 0.22f, 0.35f), hudVAO, hudShader);
      hudX += iconSize + iconGap;
    }
    
    // Bonus stars
    float dfX = hudX + 0.01f;
    for (int i = 0; i < messenger.divineFavor; i++) {
      drawHUDRect(dfX + i * (iconSize + iconGap) - 0.002f, hudY - 0.002f,
                 iconSize + 0.004f, iconSize + 0.004f, glm::vec3(1.0f, 0.6f, 0.0f), hudVAO, hudShader);
      drawHUDRect(dfX + i * (iconSize + iconGap), hudY, iconSize, iconSize,
                  glm::vec3(1.0f, 0.9f, 0.1f), hudVAO, hudShader);
    }
    
    hudX = dfX + messenger.divineFavor * (iconSize + iconGap) + 0.01f;
  }

  // Draw health hearts with borders
  for (int i = 0; i < player.health.max; i++) {
    glm::vec3 heartColor = (i < player.health.current)
        ? glm::vec3(0.95f, 0.20f, 0.30f) : glm::vec3(0.25f, 0.20f, 0.28f);
    drawHUDRect(hudX - 0.002f, hudY - 0.002f, iconSize + 0.004f, iconSize + 0.004f,
               glm::vec3(0.5f, 0.1f, 0.15f), hudVAO, hudShader);
    drawHUDRect(hudX, hudY, iconSize, iconSize, heartColor, hudVAO, hudShader);
    hudX += iconSize + iconGap;
  }

  // Draw mana bar
  float manaWidth = 0.15f;
  float manaX = hudX + 0.01f;
  drawHUDRect(manaX, hudY + barHeight * 0.2f, manaWidth, barHeight * 0.6f,
              glm::vec3(0.12f, 0.12f, 0.18f), hudVAO, hudShader);
  float manaFill = (float)player.health.mana / (float)player.health.maxMana;
  drawHUDRect(manaX, hudY + barHeight * 0.2f, manaWidth * manaFill, barHeight * 0.6f,
              glm::vec3(0.25f, 0.45f, 0.95f), hudVAO, hudShader);

  // Draw coins with borders
  float coinX = 0.45f;
  for (int i = 0; i < std::min(player.health.coins, 5); i++) {
    float cx = coinX + i * (iconSize + iconGap * 2);
    drawHUDRect(cx - 0.0015f, hudY - 0.0015f, iconSize + 0.003f, iconSize + 0.003f,
               glm::vec3(0.8f, 0.6f, 0.15f), hudVAO, hudShader);
    drawHUDRect(cx, hudY, iconSize, iconSize, glm::vec3(0.95f, 0.82f, 0.0f), hudVAO, hudShader);
  }

  // Draw keys with borders
  float keyX = 0.95f - iconSize;
  for (int i = 0; i < player.health.keys; i++) {
    float kx = keyX - i * (iconSize + iconGap);
    drawHUDRect(kx - 0.0015f, hudY - 0.0015f, iconSize + 0.003f, iconSize + 0.003f,
               glm::vec3(0.5f, 0.58f, 0.7f), hudVAO, hudShader);
    drawHUDRect(kx, hudY, iconSize, iconSize, 
                glm::vec3(0.85f, 0.88f, 0.95f), hudVAO, hudShader);
  }

  // Whiskers watermark (decorative blocks) bottom-right
  float tagY = 0.95f;
  float tagW = 0.1f;
  float tagH = 0.025f;
  float tagX = 0.88f;
  drawHUDRect(tagX - 0.003f, tagY - 0.003f, tagW + 0.006f, tagH + 0.006f,
              glm::vec3(0.35f, 0.25f, 0.5f), hudVAO, hudShader);
  drawHUDRect(tagX, tagY, tagW, tagH, glm::vec3(0.65f, 0.45f, 0.85f), hudVAO, hudShader);

  glEnable(GL_DEPTH_TEST);
}

std::vector<unsigned char> Renderer::captureFramebufferPng(int reqWidth, int reqHeight) {
  int w = (reqWidth > 0) ? std::min(reqWidth, windowWidth) : windowWidth;
  int h = (reqHeight > 0) ? std::min(reqHeight, windowHeight) : windowHeight;
  w = std::max(1, w);
  h = std::max(1, h);

  std::vector<unsigned char> rgba(static_cast<size_t>(w) * static_cast<size_t>(h) * 4u);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadBuffer(GL_BACK);
  glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());

  const int stride = w * 4;
  for (int y = 0; y < h / 2; ++y) {
    int y2 = h - 1 - y;
    for (int x = 0; x < stride; ++x) {
      std::swap(rgba[static_cast<size_t>(y) * stride + x],
                rgba[static_cast<size_t>(y2) * stride + x]);
    }
  }

  int outLen = 0;
  unsigned char* png = stbi_write_png_to_mem(rgba.data(), stride, w, h, 4, &outLen);
  if (!png) return {};
  std::vector<unsigned char> out(png, png + outLen);
  std::free(png);
  return out;
}
