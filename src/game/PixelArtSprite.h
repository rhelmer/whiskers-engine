// PixelArtSprite.h
// Procedural pixel art sprite generator — creates texture data at runtime
// so we have character art without needing external files.
#pragma once

#ifdef WHISKERS_WASM_BUILD
#include <whiskers/glad_wasm.h>
#else
#include <glad/glad.h>
#endif
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <cstdint>

// Pixel art is defined as a small grid where each cell is a palette index.
// Palette index 0 = transparent.

struct PixelArtFrame {
  int width{0};
  int height{0};
  std::vector<uint8_t> pixels;  // palette indices, row-major
};

struct PixelArtPalette {
  struct Color { uint8_t r, g, b, a; };
  std::vector<Color> colors;  // index 0 = transparent

  // Add a color, return its index.
  int add(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
    colors.push_back({r, g, b, a});
    return (int)colors.size() - 1;
  }

  // Get color by index (returns transparent if out of range).
  Color get(int i) const {
    if (i <= 0 || i >= (int)colors.size()) return {0, 0, 0, 0};
    return colors[i];
  }

  int size() const { return (int)colors.size(); }
};

struct PixelArtSpriteDef {
  std::string name;
  PixelArtPalette palette;
  std::vector<PixelArtFrame> frames;
  float frameDuration{0.1f};
  bool loop{true};
};

// Procedurally generated hero + enemies (Whiskers platformer demo).
PixelArtSpriteDef createHeroIdleSprite();
PixelArtSpriteDef createHeroRunSprite();
PixelArtSpriteDef createHeroJumpSprite();
PixelArtSpriteDef createHeroAttackSprite();

// Procedurally generated enemy sprites.
PixelArtSpriteDef createBlobEnemySprite();
PixelArtSpriteDef createShieldGuardSprite();
PixelArtSpriteDef createFireWispSprite();
PixelArtSpriteDef createChaserSprite();
PixelArtSpriteDef createBossGatekeeperSprite();

// Procedurally generated item sprites.
PixelArtSpriteDef createHeartSprite();
PixelArtSpriteDef createManaOrbSprite();
PixelArtSpriteDef createCoinSprite();
PixelArtSpriteDef createKeySprite();
PixelArtSpriteDef createCheckpointSprite();

// Procedurally generated projectile sprites.
PixelArtSpriteDef createFireballSprite();
PixelArtSpriteDef createMeleeSwingSprite();

// Convert a PixelArtSpriteDef to an OpenGL texture (RGBA8, nearest-neighbor).
uint32_t createTextureFromSpriteDef(const PixelArtSpriteDef& def, int frameIndex);

// Create an atlas texture from all frames of a sprite def.
// Returns the texture ID and frame dimensions.
struct SpriteAtlas {
  uint32_t texture{0};
  int frameWidth{0};
  int frameHeight{0};
  int frameCount{0};
};
SpriteAtlas createAtlasFromSpriteDef(const PixelArtSpriteDef& def);

// Export a sprite definition to JSON format compatible with PixelEditor.
std::string exportSpriteDefToJSON(const PixelArtSpriteDef& def);
