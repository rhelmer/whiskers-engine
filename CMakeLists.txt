cmake_minimum_required(VERSION 3.14)
project(whiskers_engine LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(WHISKERS_DEBUG_PROTOCOL "JSON stdio debug protocol for agents/tests" ON)
option(WHISKERS_BUILD_TESTS "Build Catch2 tests (ctest)" ON)

# __ Whiskers Engine - A 2.5D platformer engine library __

# Emscripten (WASM) detection
if(EMSCRIPTEN)
    set(WHISKERS_WASM_BUILD ON)
    message(STATUS "Building for WebAssembly (Emscripten)")
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
else()
    find_package(SDL2 REQUIRED)
    if(TARGET SDL2::SDL2)
        set(SDL_TARGET SDL2::SDL2)
    else()
        set(SDL_TARGET ${SDL2_LIBRARIES})
    endif()

    if(WIN32)
        if(TARGET SDL2::SDL2main)
            list(APPEND SDL_TARGET SDL2::SDL2main)
        endif()
    endif()

    find_package(OpenGL REQUIRED)
endif()

# GLM (header-only)
if(EMSCRIPTEN)
    include(FetchContent)
    FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG        1.0.1
    )
    FetchContent_MakeAvailable(glm)
    set(GLM_INCLUDE_DIRS "${glm_SOURCE_DIR}")
else()
    find_path(GLM_INCLUDE_DIRS "glm/glm.hpp" PATHS /opt/homebrew/include)
endif()
message(STATUS "Using GLM include dirs: ${GLM_INCLUDE_DIRS}")

# Dear ImGui
include(FetchContent)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.90.5
)
FetchContent_MakeAvailable(imgui)
set(IMGUI_DIR "${imgui_SOURCE_DIR}")

# __ Whiskers Engine Library __

set(WHISKERS_ENGINE_SOURCES
    src/core/GameLoop.cpp
    src/core/EntityManager.h
    src/core/System.h
    src/core/Component.h
    src/physics/PhysicsSystem.cpp
    src/physics/PhysicsSystem.h
    src/rendering/Renderer.cpp
    src/rendering/Renderer.h
    src/rendering/CameraSystem.cpp
    src/rendering/CameraSystem.h
    src/rendering/TilemapRenderer.cpp
    src/rendering/TilemapRenderer.h
    src/level/Level.cpp
    src/level/Level.h
    src/level/Tilemap.h
    src/glad.cpp
)

set(WHISKERS_ENGINE_HEADERS
    src/core/Component.h
    src/core/EntityManager.h
    src/core/GameLoop.h
    src/core/System.h
    src/physics/PhysicsSystem.h
    src/rendering/Renderer.h
    src/rendering/CameraSystem.h
    src/rendering/TilemapRenderer.h
    src/level/Level.h
    src/level/Tilemap.h
)

add_library(whiskers_engine STATIC ${WHISKERS_ENGINE_SOURCES})

# Include directories for the library
target_include_directories(whiskers_engine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# For native: glad.h is in include/glad/
# For WASM: glad_wasm.h is in include/whiskers/
if(NOT EMSCRIPTEN)
    target_include_directories(whiskers_engine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
else()
    target_include_directories(whiskers_engine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/whiskers)
endif()

target_include_directories(whiskers_engine PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:src>
    ${GLM_INCLUDE_DIRS}
)

# Link OpenGL for the library (renderer uses GL functions)
if(NOT EMSCRIPTEN)
    target_link_libraries(whiskers_engine PRIVATE OpenGL::GL)
endif()

# __ Export / Install __

set_target_properties(whiskers_engine PROPERTIES
    PUBLIC_HEADER "${WHISKERS_ENGINE_HEADERS}"
    EXPORT_NAME WhiskersEngine
)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/whiskers-engine-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/whiskers-engine-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/whiskers_engine
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/whiskers-engine-config-version.cmake
    VERSION 1.0.0
    COMPATIBILITY SameMajorVersion
)

install(TARGETS whiskers_engine
    EXPORT whiskers_engineTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/whiskers_engine
)

install(EXPORT whiskers_engineTargets
    FILE whiskers-engine-targets.cmake
    NAMESPACE whiskers::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/whiskers_engine
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/whiskers-engine-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/whiskers-engine-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/whiskers_engine
)

# __ Demo Executables __

option(BUILD_WHISKERS_DEMOS "Build Whiskers Engine demo executables" ON)

if(BUILD_WHISKERS_DEMOS)
    if(NOT EMSCRIPTEN)
        # Legacy demo - uses old non-ECS API, kept for reference
        # Note: This uses old EntityManager.h/Renderer.h at root level
        if(BUILD_LEGACY_DEMO)
            add_executable(whiskers_demo main.cpp EntityManager.cpp PhysicsSystem.cpp Renderer.cpp)
            target_include_directories(whiskers_demo PRIVATE
                ${SDL2_INCLUDE_DIRS}
                ${CMAKE_CURRENT_SOURCE_DIR}/include
            )
            target_link_libraries(whiskers_demo PRIVATE
                ${SDL_TARGET}
                OpenGL::GL
            )
        endif()

        # 2.5D platformer demo (ECS + ImGui editor; games can live in separate repos)
        set(PLATFORMER_DEMO_SOURCES
            src/main_platformer.cpp
            src/game/PixelArtSprite.cpp
            src/game/SpriteAnimator.cpp
            src/editor/Editor.cpp
            src/editor/LevelSerializer.cpp
            src/editor/PixelEditor.cpp
            "${IMGUI_DIR}/imgui.cpp"
            "${IMGUI_DIR}/imgui_draw.cpp"
            "${IMGUI_DIR}/imgui_tables.cpp"
            "${IMGUI_DIR}/imgui_widgets.cpp"
            "${IMGUI_DIR}/imgui_demo.cpp"
            "${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp"
            "${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp"
        )
        if(WHISKERS_DEBUG_PROTOCOL)
            list(APPEND PLATFORMER_DEMO_SOURCES
                src/core/DebugProtocol.cpp
                src/core/DebugProtocolHandler.cpp
                src/core/DebugInputBridge.cpp
            )
        endif()
        add_executable(platformer_demo ${PLATFORMER_DEMO_SOURCES})
        target_include_directories(platformer_demo PRIVATE
            ${SDL2_INCLUDE_DIRS}
            ${GLM_INCLUDE_DIRS}
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${IMGUI_DIR}
            "${IMGUI_DIR}/backends"
        )
        target_link_libraries(platformer_demo PRIVATE
            whiskers_engine
            ${SDL_TARGET}
            OpenGL::GL
        )
        if(WHISKERS_DEBUG_PROTOCOL)
            target_compile_definitions(platformer_demo PRIVATE WHISKERS_DEBUG_PROTOCOL)
            find_package(Threads REQUIRED)
            target_link_libraries(platformer_demo PRIVATE Threads::Threads)
        endif()
    endif()

    # WASM web build
    if(EMSCRIPTEN)
        add_executable(platformer_web
            src/main_platformer.cpp
            src/game/PixelArtSprite.cpp
            src/game/SpriteAnimator.cpp
            src/editor/Editor.cpp
            src/editor/LevelSerializer.cpp
            src/editor/PixelEditor.cpp
            "${IMGUI_DIR}/imgui.cpp"
            "${IMGUI_DIR}/imgui_draw.cpp"
            "${IMGUI_DIR}/imgui_tables.cpp"
            "${IMGUI_DIR}/imgui_widgets.cpp"
            "${IMGUI_DIR}/imgui_demo.cpp"
            "${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp"
            "${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp"
        )
        target_include_directories(platformer_web PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${GLM_INCLUDE_DIRS}
            ${IMGUI_DIR}
            "${IMGUI_DIR}/backends"
            ${CMAKE_CURRENT_SOURCE_DIR}/include/whiskers
        )
        target_link_libraries(platformer_web PRIVATE whiskers_engine)
        target_compile_options(platformer_web PRIVATE "-sUSE_SDL=2")
        target_compile_definitions(platformer_web PRIVATE WHISKERS_WASM_BUILD)
        target_link_options(platformer_web PRIVATE
            "-sUSE_SDL=2"
            "-sMAX_WEBGL_VERSION=2"
            "-sALLOW_MEMORY_GROWTH=1"
            "-sINITIAL_MEMORY=64MB"
            "-sASYNCIFY"
            "--shell-file" "${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html"
            "-O2"
            "-lembind"
        )
    endif()
endif()

# Alias
add_library(Whiskers::Engine ALIAS whiskers_engine)

# __ Tests __

if(WHISKERS_BUILD_TESTS AND NOT EMSCRIPTEN)
  include(FetchContent)
  FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.6.0
  )
  FetchContent_MakeAvailable(Catch2)

  enable_testing()

  set(WHISKERS_TEST_SOURCES
    tests/unit/test_aabb.cpp
    tests/unit/test_physics.cpp
    tests/unit/test_level_loading.cpp
    tests/unit/test_ecs.cpp
    tests/integration/test_physics_collision.cpp
    tests/integration/test_ecs_lifecycle.cpp
  )
  if(UNIX)
    list(APPEND WHISKERS_TEST_SOURCES tests/fixtures/test_debug_protocol.cpp)
  endif()

  add_executable(whiskers_tests ${WHISKERS_TEST_SOURCES})
  target_include_directories(whiskers_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  )
  target_link_libraries(whiskers_tests PRIVATE Catch2::Catch2WithMain whiskers_engine)
  target_compile_definitions(whiskers_tests PRIVATE
    WHISKERS_TEST_DATA_DIR="${CMAKE_SOURCE_DIR}/tests/fixtures"
  )
  if(TARGET platformer_demo)
    add_dependencies(whiskers_tests platformer_demo)
  endif()

  list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
  include(Catch)
  catch_discover_tests(whiskers_tests)
endif()
