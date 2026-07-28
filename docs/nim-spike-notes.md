# Nim exploratory spike notes

**Pinned toolchain:** Nim **2.2.10** via choosenim (`~/.nimble/bin` on PATH)  
**Date:** 2026-07-26  
**Branch:** `explore/nim-port`

## Phase 0 — done

- Package: `whiskers.nimble` + stub library `nim/src/whiskers.nim`
- Demo: `nim/src/hello_gl.nim` — SDL2 window, OpenGL 3.3 core context, pulsing clear color
- Verified on this machine: `GL vendor Apple` / `Apple M1` / `4.1 Metal`

### Build / run

```bash
export PATH="$HOME/.nimble/bin:$PATH"   # if using choosenim
nimble install -d                       # once: sdl2, opengl (+ x11 on non-Windows)
nimble build_gl                         # -> nim/bin/hello_gl
nimble hello                            # build + run
```

Needs system **SDL2** (`brew install sdl2` on macOS).

`config.nims` sets `--dynlibOverride:SDL2` and Homebrew link flags so `libSDL2` resolves without `DYLD_LIBRARY_PATH`.

### Friction logged

1. **`nimble build` + `opengl` package:** the published `opengl` nimble metadata sets `srcDir = "src"` but ships sources at the package root, so declarative nimble resolution fails with `cannot open file: opengl`. Workaround: custom `nimble build` task that passes `--path:$(nimble path opengl)`.
2. **SDL2 dylib load:** default `libSDL2.dylib` search failed until linking with `--dynlibOverride:SDL2` and Homebrew/`pkg-config` `-L`/`-lSDL2` (plus rpath on macOS).
3. **choosenim first install** builds the toolchain from source and takes several minutes.

None of these are blockers for Phase 1.

## Next (Phase 1)

Port the C++ space demo from `main`: entities (ship / asteroid / bullet), simple physics + TTL, minimal GL draw.
