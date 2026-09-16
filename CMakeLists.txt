cmake_minimum_required(VERSION 3.25)

project(Aetherion VERSION 0.1.0 LANGUAGES C CXX)

option(AETHERION_BUILD_TESTS "Build automated tests" ON)
option(AETHERION_BUILD_RENDERER "Build the GLFW/OpenGL renderer" ON)
option(AETHERION_ENABLE_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)
option(AETHERION_ENABLE_CLANG_TIDY "Run clang-tidy while compiling" OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerWarnings)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(AETHERION_ENABLE_CLANG_TIDY)
  find_program(CLANG_TIDY_EXE NAMES clang-tidy REQUIRED)
  set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
endif()

add_library(aetherion_core
  src/core/log.cpp
  src/core/scene.cpp
  src/core/simulation.cpp
  src/core/simulation_clock.cpp
  src/core/telemetry.cpp
  src/math/vec3d.cpp
  src/physics/gravity/gravity_solver.cpp
  src/physics/integrators/semi_implicit_euler.cpp
  src/presets/mechanics_presets.cpp
)
add_library(Aetherion::core ALIAS aetherion_core)
target_include_directories(aetherion_core PUBLIC include)
aetherion_set_project_warnings(aetherion_core)

add_library(aetherion_renderer
  src/renderer/camera.cpp
  src/renderer/mesh.cpp
  src/renderer/picking.cpp
  src/renderer/render_data.cpp
)
add_library(Aetherion::renderer ALIAS aetherion_renderer)
target_include_directories(aetherion_renderer PUBLIC include)
target_link_libraries(aetherion_renderer PUBLIC Aetherion::core)
aetherion_set_project_warnings(aetherion_renderer)

if(AETHERION_BUILD_RENDERER)
  include(FetchContent)
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
  set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
    GIT_SHALLOW TRUE
  )
  FetchContent_MakeAvailable(glfw)
  FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG v2.0.8
    GIT_SHALLOW TRUE
    SOURCE_SUBDIR cmake
  )
  FetchContent_MakeAvailable(glad)
  glad_add_library(aetherion_glad REPRODUCIBLE API gl:core=4.1)
  find_package(OpenGL REQUIRED)
  target_sources(aetherion_renderer PRIVATE src/renderer/opengl_backend.cpp)
  target_link_libraries(aetherion_renderer PRIVATE glfw aetherion_glad OpenGL::GL)
  target_compile_definitions(aetherion_renderer PUBLIC AETHERION_HAS_OPENGL=1)

  add_executable(aetherion_renderer_smoke tests/renderer_smoke.cpp)
  target_link_libraries(aetherion_renderer_smoke PRIVATE Aetherion::renderer)
  aetherion_set_project_warnings(aetherion_renderer_smoke)
endif()

add_executable(aetherion_app src/app/main.cpp)
target_link_libraries(aetherion_app PRIVATE Aetherion::core)
if(AETHERION_BUILD_RENDERER)
  target_link_libraries(aetherion_app PRIVATE Aetherion::renderer)
endif()
aetherion_set_project_warnings(aetherion_app)

add_executable(aetherion_headless src/app/headless_main.cpp)
target_link_libraries(aetherion_headless PRIVATE Aetherion::core)
aetherion_set_project_warnings(aetherion_headless)

if(AETHERION_BUILD_TESTS)
  enable_testing()
  include(FetchContent)
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
    GIT_SHALLOW TRUE
  )
  FetchContent_MakeAvailable(googletest)
  add_subdirectory(tests)
endif()
