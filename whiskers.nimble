# Package

version       = "0.1.0"
author        = "Whiskers contributors"
description   = "Exploratory Nim port of the Whiskers game engine"
license       = "MIT"
srcDir        = "nim/src"
binDir        = "nim/bin"

# Dependencies

requires "nim >= 2.2.0"
requires "sdl2"
requires "opengl"

import std/strutils

proc pkgPath(name: string): string =
  ## Resolve an installed nimble package path (workaround for opengl's broken srcDir).
  result = gorge("nimble path " & name).strip
  if result.len == 0:
    raise newException(OSError, "nimble path " & name & " returned empty; run: nimble install -d")

task build_gl, "Build hello_gl (Phase 0 spike)":
  mkDir "nim/bin"
  let cmd =
    "nim c -d:release" &
    " --path:" & pkgPath("opengl") &
    " --path:" & pkgPath("sdl2") &
    " -o:nim/bin/hello_gl" &
    " nim/src/hello_gl.nim"
  echo "Running: ", cmd
  exec cmd

task hello, "Build and run the Phase 0 SDL2 + OpenGL clear window":
  exec "nimble build_gl"
  exec "nim/bin/hello_gl"
