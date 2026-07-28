## Phase 0 spike: SDL2 window + OpenGL 3.3 clear loop.
##
## Esc or close the window to quit. Clear color pulses so a live GL context
## is obvious even without geometry.

import std/[math, times]
import sdl2
import opengl
import whiskers

proc die(msg: string) =
  quit("hello_gl: " & msg, QuitFailure)

proc main() =
  echo "whiskers ", WhiskersVersion, " / hello_gl (Nim ", NimVersion, ")"

  if sdl2.init(INIT_VIDEO) != SdlSuccess:
    die("SDL_Init failed: " & $getError())
  defer: sdl2.quit()

  discard glSetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3)
  discard glSetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3)
  discard glSetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE)
  discard glSetAttribute(SDL_GL_DOUBLEBUFFER, 1)

  let window = createWindow(
    "Whiskers Nim — hello_gl",
    SDL_WINDOWPOS_CENTERED,
    SDL_WINDOWPOS_CENTERED,
    800,
    600,
    SDL_WINDOW_OPENGL or SDL_WINDOW_RESIZABLE or SDL_WINDOW_SHOWN,
  )
  if window.isNil:
    die("SDL_CreateWindow failed: " & $getError())
  defer: destroy window

  let glContext = glCreateContext(window)
  if glContext.isNil:
    die("SDL_GL_CreateContext failed: " & $getError())
  defer: glDeleteContext(glContext)

  loadExtensions()
  discard glSetSwapInterval(1)

  echo "GL vendor:   ", $cast[cstring](glGetString(GL_VENDOR))
  echo "GL renderer: ", $cast[cstring](glGetString(GL_RENDERER))
  echo "GL version:  ", $cast[cstring](glGetString(GL_VERSION))

  let started = epochTime()
  var running = true
  var event = defaultEvent

  while running:
    while pollEvent(event):
      case event.kind
      of QuitEvent:
        running = false
      of KeyDown:
        if event.key.keysym.sym == K_ESCAPE:
          running = false
      of WindowEvent:
        if event.window.event == WindowEvent_Resized:
          glViewport(0, 0, event.window.data1, event.window.data2)
      else:
        discard

    let t = epochTime() - started
    let r = 0.12f + 0.08f * sin(t).float32
    let g = 0.18f + 0.10f * sin(t * 0.7).float32
    let b = 0.28f + 0.12f * sin(t * 1.3).float32
    glClearColor(r, g, b, 1.0f)
    glClear(GL_COLOR_BUFFER_BIT)
    window.glSwapWindow()

  echo "bye"

when isMainModule:
  main()
