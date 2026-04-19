// include/whiskers/glad_wasm.h
// WASM-specific OpenGL/GLES stubs — included only when WHISKERS_WASM_BUILD is defined.
// Provides minimal OpenGL function declarations needed for Emscripten/WebGL.
#pragma once

#include <GLES3/gl3.h>

// Stub glad initialization — always succeeds on WASM/WebGL
static inline int gladLoadGLLoader(void*(*)(const char*)) { return 1; }

// Type needed by gladLoadGLLoader signature in main
typedef void (*GLADloadproc)(const char*);
