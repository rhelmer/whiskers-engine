// PixelArtSprite.cpp
#include "PixelArtSprite.h"

#include <cstring>

#ifdef WHISKERS_WASM_BUILD
#include <whiskers/glad_wasm.h>
#else
#include <glad/glad.h>
#endif
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Helper: build a frame from a string (compact pixel art notation).
// Each character maps to a palette index:
//   ' ' = 0 (transparent)  'a'=1  'b'=2  'c'=3  'd'=4  'e'=5
//   'f'=6  'g'=7  'h'=8  'i'=9  'j'=10 'k'=11 'l'=12 'm'=13 'n'=14 'o'=15
// ---------------------------------------------------------------------------
static int charToIndex(char c) {
  if (c == ' ') return 0;
  if (c >= 'a' && c <= 'o') return c - 'a' + 1;
  return 0;
}

static PixelArtFrame frameFromString(int w, int h, const std::string& art) {
  PixelArtFrame f;
  f.width = w;
  f.height = h;
  f.pixels.resize(w * h, 0);

  // Read exactly w*h characters, skipping only newlines.
  // Spaces ARE transparent pixels (index 0).
  int pixelIdx = 0;
  for (char c : art) {
    if (c == '\n' || c == '\r') continue;
    if (pixelIdx >= w * h) break;
    f.pixels[pixelIdx++] = charToIndex(c);
  }
  return f;
}

// ---------------------------------------------------------------------------
// Whiskers hero — 16×16 simple tabby cat in a jacket (palette a–j).
// ---------------------------------------------------------------------------

static void addHeroPalette(PixelArtPalette& p) {
  p.add(0, 0, 0, 0);
  p.add(0xE8, 0x92, 0x56);  // a fur
  p.add(0xFF, 0xEA, 0xD5);  // b muzzle
  p.add(0x7A, 0x4A, 0x2A);  // c ears / outline
  p.add(0x24, 0x24, 0x28);  // d eyes
  p.add(0x3D, 0x7D, 0xD9);  // e jacket
  p.add(0xFF, 0xFF, 0xFF);  // f highlight
  p.add(0x6B, 0x53, 0x44);  // g boots
  p.add(0xC4, 0xCC, 0xE0);  // h baton / glint
  p.add(0x2A, 0x4A, 0x6E);  // i pants
  p.add(0xFF, 0xB6, 0x80);  // j accent
}

PixelArtSpriteDef createHeroIdleSprite() {
  PixelArtSpriteDef s;
  s.name = "hero_idle";
  addHeroPalette(s.palette);

  const char* art =
      "                "
      "       cc       "
      "      cbbc      "
      "      cbdcb     "
      "      bbbbb     "
      "      eejee     "
      "     eeeeeee    "
      "     ee   ee    "
      "      eeeee     "
      "      eiiee     "
      "     gg  gg     "
      "                "
      "                "
      "                "
      "                "
      "                ";

  s.frames.push_back(frameFromString(16, 16, art));
  s.frameDuration = 0.5f;
  s.loop = false;

  return s;
}

PixelArtSpriteDef createHeroRunSprite() {
  PixelArtSpriteDef s;
  s.name = "hero_run";
  addHeroPalette(s.palette);

  const char* head =
      "                "
      "       cc       "
      "      cbbc      "
      "      cbdcb     "
      "      bbbbb     "
      "      eejee     "
      "     eeeeeee    "
      "     ee   ee    "
      "      eeeee     "
      "      eiiee     ";

  const char* frames[] = {
      "     g  g       "
      "    e     e     "
      "                "
      "                "
      "                "
      "                ",
      "    gg  gg      "
      "    ee  ee      "
      "                "
      "                "
      "                "
      "                ",
      "      g g       "
      "   ee   ee      "
      "                "
      "                "
      "                "
      "                ",
      "    gg  gg      "
      "   e    e       "
      "                "
      "                "
      "                "
      "                ",
  };

  char buf[257];
  buf[256] = '\0';
  for (int i = 0; i < 4; i++) {
    std::memcpy(buf, head, 160);
    std::memcpy(buf + 160, frames[i], 96);
    s.frames.push_back(frameFromString(16, 16, buf));
  }
  s.frameDuration = 0.1f;
  s.loop = true;

  return s;
}

PixelArtSpriteDef createHeroJumpSprite() {
  PixelArtSpriteDef s;
  s.name = "hero_jump";
  addHeroPalette(s.palette);

  const char* art =
      "                "
      "       cc       "
      "      cbbc      "
      "      cbdcb     "
      "      bbbbb     "
      "      eejee     "
      "     eehh ee    "
      "      eeee      "
      "      eeee      "
      "       ee       "
      "      eiie      "
      "     g    g     "
      "                "
      "                "
      "                "
      "                ";

  s.frames.push_back(frameFromString(16, 16, art));
  s.frameDuration = 0.5f;
  s.loop = false;

  return s;
}

PixelArtSpriteDef createHeroAttackSprite() {
  PixelArtSpriteDef s;
  s.name = "hero_attack";
  addHeroPalette(s.palette);

  const char* frames[] = {
      "                "
      "       cc       "
      "      cbbc      "
      "      cbdcb     "
      "      bbbbb     "
      "      eejee     "
      "     eeeeeee    "
      "     ee   ee    "
      "      eeee      "
      "     eeee h     "
      "     eeee h     "
      "     gg  gg     "
      "                "
      "                "
      "                "
      "                ",
      "                "
      "       cc       "
      "      cbbc      "
      "      cbdcb     "
      "      bbbbb     "
      "      eejee     "
      "     eeeeeee    "
      "    eeee   h    "
      "    eeee   h    "
      "     eeeee      "
      "      eiiee     "
      "     gg  gg     "
      "                "
      "                "
      "                "
      "                ",
  };

  for (int i = 0; i < 2; i++) {
    s.frames.push_back(frameFromString(16, 16, frames[i]));
  }
  s.frameDuration = 0.12f;
  s.loop = false;

  return s;
}

// ---------------------------------------------------------------------------
// Enemy sprites
// ---------------------------------------------------------------------------

// Blob enemy — 16×16, small bouncy critter
PixelArtSpriteDef createBlobEnemySprite() {
  PixelArtSpriteDef s;
  s.name = "blob_enemy";

  auto& p = s.palette;
  p.add(0, 0, 0, 0);
  p.add(0x99, 0x22, 0x22);  // dark red body
  p.add(0xCC, 0x44, 0x44);  // red highlights
  p.add(0xFF, 0xDD, 0x00);  // yellow eyes
  p.add(0x44, 0x11, 0x11);  // very dark red (shadows)
  p.add(0x88, 0x44, 0x22);  // brown horns

  const char* art =
    "                "
    "                "
    "   e   e        "  // horns
    "   aaa a        "  // head
    "   abb a        "  // face + yellow eyes
    "   aaaaa        "
    "    ccc         "  // body
    "   ccccc        "
    "   cc cc        "
    "   c  c         "
    "  cc  cc        "  // legs
    "  dd  dd        "  // feet
    "                "
    "                "
    "                "
    "                ";

  s.frames.push_back(frameFromString(16, 16, art));
  s.frameDuration = 0.5f;
  s.loop = false;
  return s;
}

// Shield Guard — 16×16, armored enemy
PixelArtSpriteDef createShieldGuardSprite() {
  PixelArtSpriteDef s;
  s.name = "shield_guard";

  auto& p = s.palette;
  p.add(0, 0, 0, 0);
  p.add(0x66, 0x66, 0x77);  // grey armor
  p.add(0x88, 0x88, 0x99);  // light armor
  p.add(0x44, 0x44, 0x55);  // dark armor
  p.add(0xCC, 0x33, 0x33);  // red eyes
  p.add(0x88, 0x44, 0x22);  // brown shield
  p.add(0xAA, 0x55, 0x33);  // light brown shield

  const char* art =
    "                "
    "    dddd        "  // helmet
    "    daad        "
    "    dcc a       "  // face + red eyes
    "    aaaa        "
    "    aaaa   f    "  // body + shield
    "    aaa   ff    "
    "    aaa  fff    "
    "    aaa         "
    "    aaa         "
    "   aa aa        "
    "  dd  dd        "
    "                "
    "                "
    "                "
    "                ";

  s.frames.push_back(frameFromString(16, 16, art));
  s.frameDuration = 0.5f;
  s.loop = false;
  return s;
}

// Fire Wisp — 16×16, floating flame enemy
PixelArtSpriteDef createFireWispSprite() {
  PixelArtSpriteDef s;
  s.name = "fire_wisp";

  auto& p = s.palette;
  p.add(0, 0, 0, 0);
  p.add(0xFF, 0x44, 0x00);  // orange
  p.add(0xFF, 0x88, 0x00);  // light orange
  p.add(0xFF, 0xCC, 0x00);  // yellow
  p.add(0xFF, 0xFF, 0xCC);  // white-yellow center
  p.add(0xCC, 0x22, 0x00);  // dark orange

  // 2-frame flicker animation
  const char* frames[] = {
    "                "
    "                "
    "       c        "
    "      dcd       "
    "     dbbbd      "
    "     dbbbd      "
    "      bbb       "
    "     aeeea      "
    "    aeeeeea     "
    "    aeeeeea     "
    "     aeeea      "
    "      aaa       "
    "                "
    "                "
    "                "
    "                ",

    "                "
    "                "
    "                "
    "       c        "
    "      dcd       "
    "     dbbbd      "
    "     dbbbd      "
    "      bbb       "
    "     aeeea      "
    "    aeeeeea     "
    "    aeeeeea     "
    "     aeeea      "
    "      aaa       "
    "                "
    "                "
    "                ",
  };

  for (int i = 0; i < 2; i++) {
    s.frames.push_back(frameFromString(16, 16, frames[i]));
  }
  s.frameDuration = 0.2f;
  s.loop = true;
  return s;
}

// Chaser — 16×16, fast ground pursuer
PixelArtSpriteDef createChaserSprite() {
  PixelArtSpriteDef s;
  s.name = "chaser";

  auto& p = s.palette;
  p.add(0, 0, 0, 0);
  p.add(0x33, 0x33, 0x44);  // dark grey body
  p.add(0x55, 0x55, 0x66);  // medium grey
  p.add(0xFF, 0x44, 0x00);  // orange eyes
  p.add(0xFF, 0x22, 0x00);  // red mouth
  p.add(0x66, 0x66, 0x77);  // light grey

  const char* art =
    "                "
    "                "
    "   a            "  // ear
    "  abbc          "  // head + orange eye
    "  abbbc         "
    "   bbdd         "  // mouth
    "  cbbb          "  // body
    " cccbb          "
    "  cbbb    c     "  // tail
    "  c cc          "
    "  cc  c         "  // legs
    "  cc  c         "
    "                "
    "                "
    "                "
    "                ";

  s.frames.push_back(frameFromString(16, 16, art));
  s.frameDuration = 0.5f;
  s.loop = false;
  return s;
}

// Boss brute — 32×32, large armored enemy
PixelArtSpriteDef createBossGatekeeperSprite() {
  PixelArtSpriteDef s;
  s.name = "boss_gatekeeper";

  auto& p = s.palette;
  p.add(0, 0, 0, 0);
  p.add(0x55, 0x44, 0x33);  // brown skin
  p.add(0x77, 0x66, 0x55);  // light brown
  p.add(0xFF, 0x44, 0x00);  // orange eyes
  p.add(0x88, 0x88, 0x88);  // grey armor
  p.add(0xCC, 0xCC, 0xCC);  // light grey armor
  p.add(0x33, 0x22, 0x11);  // dark brown
  p.add(0xAA, 0x33, 0x00);  // dark orange

  // 32x32 boss sprite
  const char* art =
    "                                "
    "                                "
    "           dd dd                "  // horns
    "          dddddd               "
    "          bbbbbaa              "  // head
    "         bbcccbbaa             "  // face + orange eyes
    "         bbcccbbaaa            "
    "          bbbbaaaa             "
    "          bbbbbaaa             "
    "           ddddaaaa            "  // armor top
    "          ddddddaaaa           "
    "    eeeeeeeeee  aaaa           "  // shoulders
    "   eeeeeeffeeee  aa            "
    "   eeeefffeeee   aa            "  // chest
    "    eeeeeeeeee   a             "
    "    eeeeeeeeee   a             "
    "    eeeeeeeeee                  "
    "     eeeeeeee                  "
    "     ee    ee                  "
    "     ee    ee                  "
    "    eee    eee                 "
    "   eee      eee                "
    "   ee        ee                "
    "  dd          dd               "  // feet
    "                                "
    "                                "
    "                                "
    "                                "
    "                                "
    "                                "
    "                                "
    "                                ";

  s.frames.push_back(frameFromString(32, 32, art));
  s.frameDuration = 0.5f;
  s.loop = false;
  return s;
}

// ---------------------------------------------------------------------------
// Item sprites — 16×16
// ---------------------------------------------------------------------------

PixelArtSpriteDef createHeartSprite() {
  PixelArtSpriteDef s;
  s.name = "heart";

  auto& p = s.palette;
  p.add(0, 0, 0, 0);
  p.add(0xFF, 0x33, 0x55);  // red
  p.add(0xFF, 0x88, 0xAA);  // pink highlight
  p.add(0xFF, 0xFF, 0xFF);  // white sparkle

  const char* art =
    "                "
    "                "
    "   aa    aa     "
    "  aaaa  aaaa    "
    "  aabbaabbaa    "
    "  aa ca ca a    "  // c = white sparkle
    "   aaaaaaaa     "
    "    aaaaaa      "
    "     aaaa       "
    "      aa        "
    "                "
    "                "
    "                "
    "                "
    "                "
    "                ";

  // Fix: c maps to white, need to use a valid char
  // The heart art above uses 'c' which maps to palette index 3 (transparent).
  // Let me redo this properly.
  s.frames.clear();
  const char* art2 =
    "                "
    "                "
    "   aa    aa     "
    "  aaaa  aaaa    "
    "  aabbaabbaa    "
    "  aaccaaccaa    "  // c=pink highlight
    "   aaaaaaaa     "
    "    aaaaaa      "
    "     aaaa       "
    "      aa        "
    "                "
    "                "
    "                "
    "                "
    "                "
    "                ";

  s.frames.push_back(frameFromString(16, 16, art2));
  s.frameDuration = 0.5f;
  s.loop = false;
  return s;
}

PixelArtSpriteDef createManaOrbSprite() {
  PixelArtSpriteDef s;
  s.name = "mana_orb";

  auto& p = s.palette;
  p.add(0, 0, 0, 0);
  p.add(0x33, 0x66, 0xFF);  // blue
  p.add(0x66, 0x99, 0xFF);  // light blue
  p.add(0xAA, 0xCC, 0xFF);  // pale blue
  p.add(0xFF, 0xFF, 0xFF);  // white

  // 2-frame shimmer
  const char* frames[] = {
    "                "
    "                "
    "                "
    "      bb        "
    "    bcccb       "
    "   bcddcb       "
    "   bcddcb       "
    "    bcccb       "
    "      bb        "
    "                "
    "                "
    "                "
    "                "
    "                "
    "                "
    "                ",

    "                "
    "                "
    "       b        "
    "     bccb       "
    "    bcddcb      "
    "   bcddcb       "
    "   bcddcb       "
    "    bcddcb      "
    "     bccb       "
    "       b        "
    "                "
    "                "
    "                "
    "                "
    "                "
    "                ",
  };

  for (int i = 0; i < 2; i++) {
    s.frames.push_back(frameFromString(16, 16, frames[i]));
  }
  s.frameDuration = 0.3f;
  s.loop = true;
  return s;
}

PixelArtSpriteDef createCoinSprite() {
  PixelArtSpriteDef s;
  s.name = "coin";

  auto& p = s.palette;
  p.add(0, 0, 0, 0);
  p.add(0xFF, 0xCC, 0x00);  // gold
  p.add(0xFF, 0xEE, 0x66);  // light gold
  p.add(0xCC, 0x99, 0x00);  // dark gold

  // 4-frame spin
  const char* frames[] = {
    "                "
    "                "
    "                "
    "      aa        "
    "    aabbaa      "
    "   abcccbba     "
    "   abcccbba     "
    "    aabbaa      "
    "      aa        "
    "                "
    "                "
    "                "
    "                "
    "                "
    "                "
    "                ",

    "                "
    "                "
    "                "
    "      aa        "
    "     abba       "
    "     accb       "
    "     accb       "
    "     abba       "
    "      aa        "
    "                "
    "                "
    "                "
    "                "
    "                "
    "                "
    "                ",

    "                "
    "                "
    "                "
    "       a        "
    "      aba       "
    "      ab        "
    "      ab        "
    "      aba       "
    "       a        "
    "                "
    "                "
    "                "
    "                "
    "                "
    "                "
    "                ",

    "                "
    "                "
    "                "
    "      aa        "
    "     abba       "
    "     accb       "
    "     accb       "
    "     abba       "
    "      aa        "
    "                "
    "                "
    "                "
    "                "
    "                "
    "                "
    "                ",
  };

  for (int i = 0; i < 4; i++) {
    s.frames.push_back(frameFromString(16, 16, frames[i]));
  }
  s.frameDuration = 0.12f;
  s.loop = true;
  return s;
}

PixelArtSpriteDef createKeySprite() {
  PixelArtSpriteDef s;
  s.name = "key";

  auto& p = s.palette;
  p.add(0, 0, 0, 0);
  p.add(0xFF, 0xCC, 0x00);  // gold
  p.add(0xFF, 0xEE, 0x66);  // light gold

  const char* art =
    "                "
    "                "
    "                "
    "      aa        "
    "    aabbaa      "
    "    abccba      "
    "    aabbaa      "
    "      aa        "
    "      aa        "
    "     aaa        "
    "    aaa aa      "
    "     a  aa      "
    "                "
    "                "
    "                "
    "                ";

  s.frames.push_back(frameFromString(16, 16, art));
  s.frameDuration = 0.5f;
  s.loop = false;
  return s;
}

PixelArtSpriteDef createCheckpointSprite() {
  PixelArtSpriteDef s;
  s.name = "checkpoint";

  auto& p = s.palette;
  p.add(0, 0, 0, 0);
  p.add(0x66, 0x66, 0x77);  // grey pole
  p.add(0xFF, 0x33, 0x55);  // red flag
  p.add(0xFF, 0x88, 0xAA);  // pink flag highlight

  const char* art =
    "                "
    "                "
    "                "
    "      aaa       "
    "      aab       "
    "      abb       "
    "      aaa       "
    "      a         "
    "      a         "
    "      a         "
    "      a         "
    "      a         "
    "     aa         "
    "    aaa         "
    "                "
    "                ";

  s.frames.push_back(frameFromString(16, 16, art));
  s.frameDuration = 0.5f;
  s.loop = false;
  return s;
}

// ---------------------------------------------------------------------------
// Projectile sprites
// ---------------------------------------------------------------------------

PixelArtSpriteDef createFireballSprite() {
  PixelArtSpriteDef s;
  s.name = "fireball";

  auto& p = s.palette;
  p.add(0, 0, 0, 0);
  p.add(0xFF, 0x44, 0x00);  // orange
  p.add(0xFF, 0x88, 0x00);  // light orange
  p.add(0xFF, 0xCC, 0x00);  // yellow
  p.add(0xFF, 0xFF, 0xCC);  // white-yellow

  // 2-frame flicker
  const char* frames[] = {
    "                "
    "                "
    "                "
    "                "
    "      cc        "
    "    cdddc       "
    "   cdbbbdc      "
    "   cdbbbdc      "
    "    cdddc       "
    "      cc        "
    "                "
    "                "
    "                "
    "                "
    "                "
    "                ",

    "                "
    "                "
    "                "
    "                "
    "       c        "
    "     cddc       "
    "    cdbbdc      "
    "    cdbbdc      "
    "    cdbbdc      "
    "     cddc       "
    "       c        "
    "                "
    "                "
    "                "
    "                "
    "                ",
  };

  for (int i = 0; i < 2; i++) {
    s.frames.push_back(frameFromString(16, 16, frames[i]));
  }
  s.frameDuration = 0.08f;
  s.loop = true;
  return s;
}

PixelArtSpriteDef createMeleeSwingSprite() {
  PixelArtSpriteDef s;
  s.name = "melee_swing";

  auto& p = s.palette;
  p.add(0, 0, 0, 0);
  p.add(0x8B, 0x45, 0x13);  // brown handle
  p.add(0xAA, 0xAA, 0xAA);  // silver prongs
  p.add(0xFF, 0xCC, 0x00);  // yellow slash effect
  p.add(0xFF, 0xFF, 0xCC);  // white-yellow effect

  const char* art =
    "                "
    "                "
    "                "
    "      d         "
    "     dbd        "
    "    ddbbd       "
    "     ccc        "
    "      a         "
    "      a         "
    "      a         "
    "      a         "
    "      a         "
    "      a         "
    "                "
    "                "
    "                ";

  s.frames.push_back(frameFromString(16, 16, art));
  s.frameDuration = 0.15f;
  s.loop = false;
  return s;
}

// ---------------------------------------------------------------------------
// Texture generation
// ---------------------------------------------------------------------------

uint32_t createTextureFromSpriteDef(const PixelArtSpriteDef& def, int frameIndex) {
  if (frameIndex < 0 || frameIndex >= (int)def.frames.size()) return 0;
  const auto& frame = def.frames[frameIndex];

  std::vector<uint8_t> rgba(frame.width * frame.height * 4);
  for (int i = 0; i < frame.width * frame.height; ++i) {
    auto color = def.palette.get(frame.pixels[i]);
    rgba[i * 4 + 0] = color.r;
    rgba[i * 4 + 1] = color.g;
    rgba[i * 4 + 2] = color.b;
    rgba[i * 4 + 3] = color.a;
  }

  uint32_t tex;
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, frame.width, frame.height, 0,
               GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());

  // Nearest-neighbor filtering for pixel art
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  return (uint32_t)tex;
}

SpriteAtlas createAtlasFromSpriteDef(const PixelArtSpriteDef& def) {
  SpriteAtlas atlas;
  if (def.frames.empty()) return atlas;

  atlas.frameWidth = def.frames[0].width;
  atlas.frameHeight = def.frames[0].height;
  atlas.frameCount = (int)def.frames.size();

  // Create horizontal strip: all frames side by side
  int totalWidth = atlas.frameWidth * atlas.frameCount;
  int totalHeight = atlas.frameHeight;
  std::vector<uint8_t> rgba(totalWidth * totalHeight * 4);

  for (int f = 0; f < atlas.frameCount; ++f) {
    const auto& frame = def.frames[f];
    for (int y = 0; y < atlas.frameHeight; ++y) {
      for (int x = 0; x < atlas.frameWidth; ++x) {
        int srcIdx = y * frame.width + x;
        int dstX = f * atlas.frameWidth + x;
        int dstIdx = y * totalWidth + dstX;

        auto color = def.palette.get(
          srcIdx < (int)frame.pixels.size() ? frame.pixels[srcIdx] : 0);
        rgba[(dstIdx) * 4 + 0] = color.r;
        rgba[(dstIdx) * 4 + 1] = color.g;
        rgba[(dstIdx) * 4 + 2] = color.b;
        rgba[(dstIdx) * 4 + 3] = color.a;
      }
    }
  }

  glGenTextures(1, &atlas.texture);
  glBindTexture(GL_TEXTURE_2D, atlas.texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, totalWidth, totalHeight, 0,
               GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  return atlas;
}

std::string exportSpriteDefToJSON(const PixelArtSpriteDef& def) {
  json root;
  root["name"] = def.name;
  root["width"] = def.frames.empty() ? 16 : def.frames[0].width;
  root["height"] = def.frames.empty() ? 16 : def.frames[0].height;
  root["frameDuration"] = def.frameDuration;
  root["loop"] = def.loop;

  root["palette"] = json::array();
  for (const auto& c : def.palette.colors) {
    root["palette"].push_back({(float)c.r / 255.0f, (float)c.g / 255.0f, (float)c.b / 255.0f, (float)c.a / 255.0f});
  }

  root["frames"] = json::array();
  for (size_t i = 0; i < def.frames.size(); ++i) {
    const auto& f = def.frames[i];
    json fj;
    fj["name"] = "Frame " + std::to_string(i + 1);
    fj["pixels"] = json::array();
    for (uint8_t p : f.pixels) {
      fj["pixels"].push_back(p);
    }
    root["frames"].push_back(fj);
  }

  return root.dump(2);
}
