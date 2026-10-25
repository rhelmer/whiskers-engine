#!/bin/bash
# Build platformer_web (WASM) using Emscripten
# Requires: emsdk installed and activated
#
# Usage:
#   ./web/build_wasm.sh           # Full build
#   ./web/build_wasm.sh --clean   # Clean + build
#
# Output: build-web/platformer_web.html + .wasm + .data + .js

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_DIR/build-web"

if [ "$1" = "--clean" ]; then
    echo "Cleaning web build directory..."
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "Configuring for Emscripten (WASM)..."
emcmake cmake .. \
    -DCMAKE_BUILD_TYPE=Release

echo "Building..."
emmake make -j"$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)" platformer_web

echo ""
echo "=========================================="
echo " WASM build complete!"
echo "=========================================="
echo ""
echo "  Output: $BUILD_DIR/platformer_web.html"
echo ""
echo "To serve locally:"
echo "  cd $BUILD_DIR && python3 -m http.server 8080"
echo "  Then open http://localhost:8080"
echo ""
