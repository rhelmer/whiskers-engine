# Nim port — decision log

**Status:** Paused at Phase 0.
**Date:** 2026-10-25
**Branch:** `explore/nim-port`
**Decision:** **Shelve** (not stop, not continue). Keep the branch and plan intact;
do not invest further Nim time until the triggers below are met.

---

## Where we got to

- Phase 0 of [docs/nim-port-plan.md](nim-port-plan.md) is complete.
- `nimble build_gl` / `nimble hello` produce a working SDL2 + OpenGL 3.3 core window
  on macOS (Apple M1, GL 4.1 Metal). See [docs/nim-spike-notes.md](nim-spike-notes.md).
- Toolchain pinned at Nim **2.2.10** via choosenim.
- Known friction logged: broken `opengl` nimble `srcDir`, SDL2 dylib resolution,
  slow first choosenim bootstrap. All worked around; none blocking.

## Why we are pausing

Phase 0 only exercises FFI smoothness. The hypotheses that actually drive a
go / no-go — ECS ergonomics, GC behavior under a 120 Hz fixed-step loop, and
build/test/CI ergonomics for a real game — are untested. The plan itself
places the decision gate after Phase 3, and we do not currently have appetite
to fund Phases 1–3.

Meanwhile `feature/2.5d-platformer-spec` is the semantic target for Whiskers
and is where new gameplay/engine value lands. Splitting attention between a
C++ platformer and a Nim space-demo spike dilutes both.

## What would make us revisit

Resume Nim exploration when **any** of these is true:

1. The C++ platformer branch hits a wall that Nim's metaprogramming or GC
   model would plausibly fix (e.g. ECS boilerplate becomes the main friction,
   or we want a scripting-like layer and don't want to embed Lua/JS).
2. We decide to invest in tooling (asset pipeline, MCP/debug protocol helpers,
   editor glue) and want to prototype that in Nim first — the "hybrid" branch
   from the plan's §7 decision criteria.
3. A contributor shows up who wants to drive Phase 1 end-to-end.
4. We hit a WASM / portability requirement that C++/Emscripten handles poorly.

## What NOT to do

- Do not merge `explore/nim-port` into `main`.
- Do not delete the branch — the Phase 0 spike + notes are the cheapest
  evidence we have for a future decision.
- Do not re-litigate this doc without new evidence; update it in place when
  a trigger above fires.

## Next action (non-Nim)

Switch to `feature/2.5d-platformer-spec` and triage what's blocking the
platformer vertical slice.
