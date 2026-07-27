# Exploratory plan: porting Whiskers to Nim

**Branch:** `explore/nim-port` (off `main`)  
**Status:** exploration only — no production commitment yet  
**Goal:** learn whether a Nim reimplementation of Whiskers is viable, pleasant, and maintainable relative to the current C++ tree.

---

## 1. Context

`main` today is the older **space-game demo**: flat sources (`Entity`, `EntityManager`, `PhysicsSystem`, `Renderer`), SDL2 + OpenGL 3.3 + GLM, single executable.

The richer **2.5D platformer engine** (library + ECS components, level JSON, ImGui editor, Catch2 tests, debug protocol, WASM path) lives on `feature/2.5d-platformer-spec`. That branch is the **semantic source of truth** for what “Whiskers” should become.

This Nim branch deliberately starts from `main` so exploration stays isolated. When evaluating architecture, prefer the platformer design; when validating a first spike, prefer the smaller space-game surface area.

---

## 2. Why Nim (hypotheses to test)

| Hypothesis | How we falsify it |
|------------|-------------------|
| Nim’s compile-time + metaprogramming simplify ECS boilerplate vs C++ headers | Compare LOC / ergonomics of entity queries after a spike |
| C interop lets us keep SDL2 / OpenGL / (later) ImGui without rewriting the world | Bindings work on macOS + Linux with acceptable friction |
| Memory / GC model is fine for a 120 Hz platformer with modest entity counts | No hitching under fixed-step physics + render in a stress demo |
| Tooling (nimble, `testament`, packaging) is good enough for games + CI | Can build, test, and run a demo in GitHub Actions |
| WASM story is acceptable | Either Nim→JS/WASM or keep a C++/Emscripten path — decide after spike |

If several hypotheses fail, we stop or keep Nim only for tools (editor helpers, MCP, asset pipelines).

---

## 3. Non-goals (this branch)

- Replacing C++ on `main` / merging a full port
- Feature parity with the platformer demo in one pass
- Perfect ECS (archetypes, sparse sets) on day one
- Shipping WASM, ImGui editor, or debug protocol in the first milestones
- Rewriting assets or inventing a new gameplay design

---

## 4. Target architecture (Nim sketch)

Mirror the platformer engine’s module split, even if the first spike is simpler:

```
whiskers/                 # nimble package (library)
  core/                   # game loop, ECS handles, systems
  physics/
  rendering/              # OpenGL helpers, camera, sprites
  level/                  # JSON level load (later)
demos/
  space/                  # port of main’s space demo (spike)
  platformer/             # later: thin game on top of library
tests/
docs/                     # this plan + spike notes
```

**ECS:** keep the known-good pattern from the platformer branch:

- Stable `EntityHandle` (`index` + `generation`)
- Plain component objects + “hasX” flags (or `Option[T]` / discriminant unions)
- Systems as procs over `EntityManager`, phased like C++ (`earlyUpdate` / `update` / `lateUpdate` / `render`)
- Fixed timestep (~120 Hz) with variable render rate

Nim-native refinements to explore (not required for spike):

- `macro` / `template` for `queryWith(Physics, Collider)`
- `{.packed.}` / distinct types for handles
- Avoid premature archetype storage

**Math:** start with a thin `Vec2`/`Vec3`/`Mat4` module or a mature Nim math lib; do **not** pull GLM via C++.

**Platform:** SDL2 for window/input/GL context; OpenGL 3.3 core (same as today). Prefer existing Nim SDL2/OpenGL packages before hand-rolled FFI.

---

## 5. Dependency map (C++ → Nim)

| C++ today | Nim candidate | Notes |
|-----------|---------------|--------|
| SDL2 | `sdl2` nimble / wrappers | Validate GL context + events first |
| OpenGL + glad | `opengl` + loader, or generate bindings | Match 3.3 core profile |
| GLM | Nim vec/mat types | Keep API close to current usage |
| nlohmann/json | `json` stdlib | Level format later |
| Dear ImGui | C ImGui + Nim wrapper, or defer | Phase 3+ |
| Catch2 | `testament` / `unittest` | Mirror unit tests from platformer |
| Emscripten WASM | TBD | Explicit open question |
| Debug protocol (JSON stdio) | stdlib JSON + threads/async IO | After core loop works |

---

## 6. Phased milestones

### Phase 0 — Tooling spike (1–2 days)

- Install Nim + nimble; pin a compiler version in the plan notes
- Empty nimble package builds on macOS (and ideally Linux CI)
- Document how to open a window and clear a GL color (hello triangle optional)

**Exit:** `nimble build` works; README fragment for Nim exists under `docs/`.

### Phase 1 — Space demo port (spike)

Port `main`’s loop:

1. Window + GL context via SDL2  
2. Entity list (ship / asteroid / bullet)  
3. Simple physics + TTL  
4. Minimal renderer (enough to see the ship and bullets)

Keep behavior close to C++; do not invent features.

**Exit:** playable space demo in Nim; short write-up of pain points (bindings, GC, build).

### Phase 2 — Core library extraction

- Lift ECS + fixed game loop into `whiskers` package
- Add AABB overlap + gravity-style physics tests against C++ fixtures where useful
- Optionally cherry-pick / reimplement platformer `Component` set as plain Nim objects

**Exit:** demo depends on library; a few unit tests pass.

### Phase 3 — Platformer vertical slice

Using `feature/2.5d-platformer-spec` as reference:

- Player move / jump / coyote-time basics  
- Tile or box platforms  
- One camera follow  
- Load one simple level (JSON or hard-coded)

**Exit:** side-scroller slice runs; decide go / no-go on continuing the port.

### Phase 4 — Optional parity tracks (only if Phase 3 succeeds)

- Pixel-art sprites / animator  
- Combat, pickups, delivery flow  
- ImGui editor bindings  
- Debug protocol for agent/MCP playtests  
- WASM / web strategy decision

---

## 7. Decision criteria (go / no-go)

After Phase 1–3, write **continue**, **hybrid** (Nim tools + C++ engine), or **stop**.

Favor **continue** if:

- Day-to-day iteration feels faster than C++ for engine code  
- Bindings are stable enough that GL/SDL are not the main time sink  
- Fixed-step gameplay feels solid (no GC stalls in the spike)  
- Tests and CI are straightforward  

Favor **stop / hybrid** if:

- FFI and packaging dominate the schedule  
- GC or memory model fights the 120 Hz loop  
- Editor / ImGui / WASM path looks worse than keeping C++  

Record the decision in `docs/nim-port-decision.md` (to be written later).

---

## 8. Working agreements for this branch

- Prefer small, reversible spikes over large rewrites  
- Leave C++ on `main` untouched unless a tiny shared asset/doc change helps both  
- When unsure about behavior, check `feature/2.5d-platformer-spec` before inventing  
- Keep spike notes in `docs/` (dated short entries are fine)  
- Do not merge to `main` without an explicit go decision  

---

## 9. Immediate next steps

1. Pin Nim version and create a skeleton `whiskers.nimble` + hello window  
2. Implement Phase 1 space demo  
3. Write a short spike retrospective (`docs/nim-spike-notes.md`)  
4. Reassess before starting Phase 2  

---

## 10. Open questions

- Nim 2.x version pin and GC (`orc` vs others) for games  
- SDL2 package quality on Windows (vcpkg parity)  
- Keep glad-style loaders or use a Nim OpenGL loader  
- Whether to vendor C++ as reference submodule vs relying on the other branch  
- Long-term: single repo with dual backends, or Nim fork / sibling repo  
