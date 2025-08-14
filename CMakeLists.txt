cmake_minimum_required(VERSION 3.10)
project(whiskers_demo_opengl)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# SDL2
find_package(SDL2 REQUIRED)
if (TARGET SDL2::SDL2)
    set(SDL_TARGET SDL2::SDL2)
else()
    set(SDL_TARGET ${SDL2_LIBRARIES})
endif()

# GLM (header-only)
find_path(GLM_INCLUDE_DIRS "glm/glm.hpp" PATHS /opt/homebrew/include)

message(STATUS "Using GLM include dirs: ${GLM_INCLUDE_DIRS}")

add_executable(whiskers_demo
    main.cpp
    glad.c
    EntityManager.cpp
    PhysicsSystem.cpp
    Renderer.cpp
)

target_include_directories(whiskers_demo PRIVATE 
    ${SDL2_INCLUDE_DIRS}
    ${GLM_INCLUDE_DIRS}
    ./include
)

target_link_libraries(whiskers_demo PRIVATE ${SDL_TARGET})

if(APPLE)
    target_link_libraries(whiskers_demo PRIVATE "-framework OpenGL")
else()
    target_link_libraries(whiskers_demo PRIVATE GL)
endif()
