# Debug Protocol & Automated Testing Specification

## Overview

Add a debug communication protocol and automated testing infrastructure so that:
1. **LLM agents can play-test** the game programmatically
2. **Integration tests can assert game behavior** (physics, collisions, level loading)
3. **CI runs tests on every PR** via `ctest`

---

## 1. Debug Protocol

### 1.1 Design

A **JSON-over-stdio** protocol is compiled in when **`-DWHISKERS_DEBUG_PROTOCOL=ON`** (default **ON** for native builds). At **runtime**, the game only enables the protocol when started with:

```text
platformer_demo --debug-protocol [--headless]
```

- **`--debug-protocol`** — wires stdin/stdout JSON handling, debug input bridge, and suppresses decorative `stdout` so every line is valid JSON for agents.
- **`--headless`** — creates the window with `SDL_WINDOW_HIDDEN` (OpenGL still works; use for CI and automated runs).

When the protocol is **off**, there is no extra stdin thread or JSON processing.

### 1.2 Protocol Format

**Request** (agent → game, one JSON object per line on stdin):

```json
{"cmd": "<command>", "args": { ... }}
```

**Response** (game → agent, one JSON object per line on stdout):

```json
{"status": "ok" | "error", "result": { ... }}
```

Errors look like: `{"status":"error","result":{"message":"..."}}`.

**State snapshots** are returned from **`get_state`** inside `result` (see below). Optional periodic **`frame_state`** streaming is *not* emitted by default; agents should call `get_state` or use the `step` response `result.frame` for the simulation frame counter.

### 1.3 Commands

| Command | Args | Response `result` | Description |
|---------|------|---------------------|-------------|
| `get_state` | none | `frame`, `time`, `entities` | Full entity snapshot |
| `set_input` | `{ "entity": <index>, "move": -1.0, "jump": true, "attack": false }` | `{}` | Override SDL input for that entity; cancels an active `input_frame` |
| `input_frame` | `{ "keys": ["Space", "D"], "frames": 5, "entity": 0 }` | `{}` | Optional `entity` (defaults to first entity with input). Hold keys for N fixed steps, then release. Cancels `set_input` override |
| `step` | `{ "frames": 1 }` | `{ "frame": <uint64> }` | Runs N fixed physics steps synchronously (no wall-clock wait) |
| `load_level` | `{ "path": "tests/fixtures/level_test_basic.json" }` | `{ "entities": <int> }` | Clears `EntityManager`, then loads the level file |
| `screenshot` | `{ "width": 640, "height": 480 }` | `{ "png": "<base64>" }` | After the **next** full scene render, captures framebuffer (clamped to window size) |
| `teleport` | `{ "entity": <index>, "x": 5.0, "y": 3.0, "z": ? }` | `{}` | Teleport entity |
| `spawn` | `{ "type": "...", "x", "y", "properties": {} }` | `{ "entity": <index> }` | Types: `player`, `platform`, `enemy_patrol` (optional `properties` for AI, e.g. patrol range) |
| `destroy` | `{ "entity": <index> }` | `{}` | Destroy by slot index |
| `pause` | none | `{}` | Pauses fixed-step simulation (events/render may still run) |
| `resume` | none | `{}` | Resumes simulation |
| `get_physics_debug` | none | `{ "colliders": [...], "contacts": [[i,j],...] }` | World-space AABBs and overlapping pairs |
| `set_gravity` | `{ "x": 0, "y": -20 }` | `{}` | Sets `PhysicsSystem` gravity |
| `set_speed` | `{ "time_scale": 0.5 }` | `{}` | Multiplier on fixed-step `dt` in the main loop |
| `quit` | none | `{}` | Stops the main loop |

### 1.4 State snapshot format (`get_state` → `result`)

```json
{
  "status": "ok",
  "result": {
    "frame": 142,
    "time": 1.183,
    "entities": [
      {
        "index": 0,
        "type": "player",
        "position": { "x": 2.5, "y": 3.1, "z": 0.0 },
        "velocity": { "x": 4.2, "y": 0.0 },
        "onGround": true,
        "health": { "current": 3, "max": 3 },
        "collider": { "min": { "x": 2.2, "y": 2.6 }, "max": { "x": 2.8, "y": 3.6 } }
      },
      {
        "index": 1,
        "type": "platform",
        "position": { "x": 0.0, "y": -0.5, "z": 0.0 },
        "collider": { "min": { "x": -10.0, "y": -1.0 }, "max": { "x": 10.0, "y": 0.0 } }
      }
    ]
  }
}
```

`type` is inferred from components (`player`, `platform`, `enemy_patrol`, etc.). Fields such as `velocity` / `health` appear only when the relevant components exist.

### 1.5 Implementation

| File | Role |
|------|------|
| **`src/core/DebugProtocol.h/cpp`** | Background thread on **Unix** uses `poll()` + `read()` on stdin and pushes complete lines to a **mutex-protected queue** (main thread drains once per frame). **Windows** uses a blocking `getline` loop; shutdown **detaches** the reader (stdin cannot be unblocked portably). |
| **`src/core/DebugProtocolHandler.h/cpp`** | Parses **nlohmann::json**, dispatches commands, writes responses; screenshot completion runs in **`onAfterSceneRender`** after `Renderer::render`. |
| **`src/core/DebugInputBridge.h/cpp`** | Feeds **`input_frame`** / **`set_input`** into `PlayerInputComponent` ahead of SDL (`PlatformerInputSystem` skips keyboard when overridden). |
| **Gate** | `#ifdef WHISKERS_DEBUG_PROTOCOL` — sources and define only when the CMake option is **ON**. |
| **`GameLoop`** | Exposes `runFixedSteps`, `paused`, `timeScale`, `getSimulationFrame()`, hooks `onBeforeFrame`, `onFixedStepStart`, `onAfterSceneRender`. |

### 1.6 Screenshot flow

1. Agent sends `{"cmd":"screenshot","args":{"width":640,"height":480}}`
2. Game sets pending flag; on the **next** frame after `Renderer::render` + editor overlay, **`glReadPixels`** reads RGBA (Y flipped for PNG), **`stbi_write_png_to_mem`** (`include/stb_image_write.h`), then **base64** in JSON
3. Agent decodes base64 to PNG bytes

---

## 2. Unit & Integration Testing

### 2.1 Test framework

**Catch2 v3** via **`FetchContent`** (`v3.6.0`), **`Catch2::Catch2WithMain`**, **`catch_discover_tests`**.

### 2.2 Test categories

| Category | Location | Description |
|----------|----------|-------------|
| **Unit tests** | `tests/unit/` | AABB, physics integration, ECS, level JSON + `Level::load` |
| **Integration tests** | `tests/integration/` | Physics + collision, ECS handles |
| **Fixture tests** | `tests/fixtures/test_debug_protocol.cpp` | **Unix only** (CMake `if(UNIX)`): `fork`/`exec` of `platformer_demo`, pipes for stdin/stdout |

### 2.3 CMake / compile definitions

- **`WHISKERS_BUILD_TESTS`** — default **ON** (non-Emscripten).
- **`WHISKERS_TEST_DATA_DIR`** — absolute path to `tests/fixtures` for `loadLevelFromJson` in tests.
- **`WHISKERS_GAME_EXE`** — optional environment variable for the fixture tests; default is `./platformer_demo` (run **`ctest` from the build directory** or set this to `"$PWD/platformer_demo"`).

### 2.4 Example unit tests (implemented)

The repository contains **`test_aabb.cpp`**, **`test_physics.cpp`**, **`test_level_loading.cpp`**, **`test_ecs.cpp`**. The minimal **`level_test_basic.json`** fixture has **no tilemap** (`layers.size() == 0`); tests assert spawned entities and parsed metadata, not tile dimensions.

### 2.5 Fixture tests (implemented shape)

Fixture tests **do not** use a separate `DebugProtocolTestHarness` class: they embed a small **`Proc`** helper (**`fork`**, pipes, **`execl`** with `platformer_demo`, `--headless`, `--debug-protocol`) and **`nlohmann::json`** parse per line. Cases include **player falls and lands** (`step` + `get_state`) and **screenshot returns large base64 PNG string**.

### 2.6 CMake integration (summary)

```cmake
option(WHISKERS_BUILD_TESTS "Build unit and integration tests" ON)
option(WHISKERS_DEBUG_PROTOCOL "Enable debug stdio protocol sources" ON)

# whiskers_tests links Catch2::Catch2WithMain + whiskers_engine
# tests/fixtures/test_debug_protocol.cpp added only if(UNIX)
# add_dependencies(whiskers_tests platformer_demo)
# target_compile_definitions(whiskers_tests PRIVATE
#     WHISKERS_TEST_DATA_DIR="${CMAKE_SOURCE_DIR}/tests/fixtures")
```

### 2.7 CI workflow

- **Linux:** `xvfb-run -a ctest ...` so OpenGL/SDL have a display; **`WHISKERS_GAME_EXE`** points at the built `platformer_demo`.
- **macOS:** `ctest` without xvfb; same env for subprocess tests.
- **Windows:** Fixture source file is **excluded**; remaining Catch2 tests still run.

Configure with **`-DWHISKERS_BUILD_TESTS=ON -DWHISKERS_DEBUG_PROTOCOL=ON`** in CI.

---

## 3. MCP Server (LLM playtest agent)

### 3.1 `tools/mcp_server.py`

- Spawns **`platformer_demo --headless --debug-protocol`** (subprocess with pipes).
- If **`pip install mcp`** is available, registers **FastMCP** tools (`start_game`, `stop_game`, `get_state`, `step_frames`, `press_keys` → `input_frame`, `screenshot`, `load_level`, `set_gravity`, `teleport_entity`).
- If **`mcp`** is missing, the script exits with a short install hint.

### 3.2 MCP tools (when SDK present)

| Tool | Maps to |
|------|--------|
| `start_game` / `stop_game` | subprocess lifecycle |
| `get_state` | `get_state` |
| `step_frames` | `step` |
| `press_keys` | `input_frame` |
| `screenshot` | `screenshot` |
| `load_level` | `load_level` |
| `set_gravity` | `set_gravity` |
| `teleport_entity` | `teleport` |

---

## 4. Test fixtures (JSON levels)

Files: **`tests/fixtures/level_test_basic.json`**, **`tests/fixtures/level_test_collision.json`**.  
Level **`properties`** in JSON may use **numbers or strings**; the loader normalizes numeric values for `std::stof` parsing in C++.

---

## 5. Implementation status

| Step | Status |
|------|--------|
| Debug protocol + handler + input bridge + `GameLoop` hooks | Done |
| Catch2 + `tests/` + `ctest` | Done |
| Integration + fixture (Unix) tests | Done |
| `tools/mcp_server.py` (optional `mcp` package) | Done |
| GitHub Actions (`ctest`, Linux xvfb) | Done |

---

## 6. Directory structure (current)

```
whiskers-engine/
├── include/
│   └── stb_image_write.h          # Screenshot PNG encoding
├── src/core/
│   ├── DebugProtocol.h/cpp
│   ├── DebugProtocolHandler.h/cpp
│   ├── DebugInputBridge.h/cpp
│   └── GameLoop.h/cpp             # Hooks + runFixedSteps, pause, timeScale
├── tests/
│   ├── unit/
│   │   ├── test_aabb.cpp
│   │   ├── test_physics.cpp
│   │   ├── test_level_loading.cpp
│   │   └── test_ecs.cpp
│   ├── integration/
│   │   ├── test_physics_collision.cpp
│   │   └── test_ecs_lifecycle.cpp
│   └── fixtures/
│       ├── level_test_basic.json
│       ├── level_test_collision.json
│       └── test_debug_protocol.cpp
├── tools/
│   └── mcp_server.py
└── CMakeLists.txt
```

---

## 7. Success criteria

- [x] **`ctest`** runs and passes with **≥ 10** tests (unit, integration, and Unix fixture tests where enabled).
- [x] Debug protocol responds to **`get_state`**, **`set_input`**, **`step`**, **`screenshot`** (with **`--debug-protocol`**).
- [x] Integration / fixture coverage verifies **falling**, landing, and **`onGround`** where applicable.
- [x] **`tools/mcp_server.py`** can be used with **`pip install mcp`** for LLM-driven playtesting.
- [x] **CI** runs **`ctest`** on pull requests (per-platform test steps; Linux uses **xvfb**).
