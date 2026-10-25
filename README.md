# Whiskers Engine

A C++ **2.5D ECS platformer engine** with native (SDL2 + OpenGL) and optional **WebAssembly** builds. The repo ships a **sample platformer** (`platformer_demo` / `platformer_web`): pixel-art hero, combat, pickups, parcel-delivery quest flow, and an **ImGui** in-game editor.

```
╔══════════════════════════════════════════════════╗
║         Whiskers — Platformer Demo             ║
╠══════════════════════════════════════════════════╣
║  A/D or ←/→  Move                              ║
║  Space/W/↑   Jump                              ║
║  X or J       Melee                            ║
║  C or K       Fireball (costs mana)            ║
║  F1           Toggle editor                    ║
╚══════════════════════════════════════════════════╝
```

## Quick Start

### Dependencies

**macOS**
```bash
brew install sdl2 glm cmake pkg-config
```

**Ubuntu/Debian**
```bash
sudo apt update
sudo apt install -y libsdl2-dev libglm-dev cmake build-essential pkg-config
```

**Windows (vcpkg)**
```powershell
./vcpkg/vcpkg install sdl2 glm --triplet=x64-windows
```

### Native Build & Run

```bash
mkdir build && cd build
cmake ..
cmake --build .

./platformer_demo
```

### Web (WASM) Build

Requires the [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html):

```bash
source /path/to/emsdk/emsdk_env.sh
./web/build_wasm.sh
```

Then open or serve `build-web/platformer_web.html`.

## Features

- **ECS** — stable handles, component queries, physics, rendering, camera.
- **Sample content** — two small levels, NPCs, delivery zones, moving platforms, enemies.
- **Editor** — ImGui viewport and entity tools (F1 in the demo).
- **Tests** — Catch2 + optional JSON [debug protocol](specs/testing-debug-protocol.md) for automation (`ctest`).

## Layout

| Path | Role |
|------|------|
| `src/core/` | Game loop, ECS |
| `src/physics/` | Physics |
| `src/rendering/` | OpenGL renderer, HUD |
| `src/editor/` | ImGui editor |
| `src/game/` | Pixel art sprites, animator |
| `src/level/` | Level JSON |
| `tests/` | Unit + integration tests |
| `specs/` | Design + testing docs |

| Target | Description |
|--------|-------------|
| `whiskers_engine` | Static library |
| `platformer_demo` | Sample game (desktop) |
| `platformer_web` | Same demo in the browser (Emscripten) |

## License

See project root for license terms.
