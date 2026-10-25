# Project-level Nim config for the Whiskers exploratory port.
# Used by `nim c` and by nimble when compiling bins from this directory.

switch("path", "nim/src")

# Prefer linking SDL2 at build time; runtime dylib search is fragile on macOS Homebrew.
switch("dynlibOverride", "SDL2")

when defined(macosx):
  # Homebrew (Apple Silicon / Intel) — override via --passL if needed.
  if dirExists("/opt/homebrew/lib"):
    switch("passL", "-L/opt/homebrew/lib -lSDL2 -Wl,-rpath,/opt/homebrew/lib")
  elif dirExists("/usr/local/lib"):
    switch("passL", "-L/usr/local/lib -lSDL2 -Wl,-rpath,/usr/local/lib")
  else:
    switch("passL", "-lSDL2")
else:
  switch("passL", "-lSDL2")
# begin Nimble config (version 2)
when withDir(thisDir(), system.fileExists("nimble.paths")):
  include "nimble.paths"
# end Nimble config
