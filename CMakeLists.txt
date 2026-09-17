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
  src/core/command_queue.cpp
  src/core/body_defaults.cpp
  src/core/scene.cpp
  src/core/simulation.cpp
  src/core/simulation_clock.cpp
  src/core/simulation_controller.cpp
  src/core/telemetry.cpp
  src/math/vec3d.cpp
  src/physics/gravity/gravity_solver.cpp
  src/physics/em/electrostatics.cpp
  src/physics/em/lorentz.cpp
  src/physics/integrator_comparison.cpp
  src/physics/integrators/rk4.cpp
  src/physics/integrators/boris.cpp
  src/physics/integrators/semi_implicit_euler.cpp
  src/physics/integrators/velocity_verlet.cpp
  src/physics/stability_analyzer.cpp
  src/presets/mechanics_presets.cpp
)
add_library(Aetherion::core ALIAS aetherion_core)
target_include_directories(aetherion_core PUBLIC include)
aetherion_set_project_warnings(aetherion_core)

add_library(aetherion_renderer
  src/renderer/adaptive_grid.cpp
  src/renderer/camera.cpp
  src/renderer/camera_tracking.cpp
  src/renderer/field_visualization.cpp
  src/renderer/mesh.cpp
  src/renderer/picking.cpp
  src/renderer/render_data.cpp
  src/renderer/trail_history.cpp
  src/presets/em_presets.cpp
)
add_library(Aetherion::renderer ALIAS aetherion_renderer)
target_include_directories(aetherion_renderer PUBLIC include)
target_link_libraries(aetherion_renderer PUBLIC Aetherion::core)
aetherion_set_project_warnings(aetherion_renderer)

add_library(aetherion_serialization src/serialization/scene_serialization.cpp)
add_library(Aetherion::serialization ALIAS aetherion_serialization)
target_include_directories(aetherion_serialization PUBLIC include)
target_link_libraries(aetherion_serialization PUBLIC Aetherion::core Aetherion::renderer)
aetherion_set_project_warnings(aetherion_serialization)

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
  FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG d7950e2c42cfd57020c7f9a5a90010d9c702a29f
    GIT_SHALLOW TRUE
  )
  FetchContent_MakeAvailable(imgui)
  find_package(OpenGL REQUIRED)
  add_library(aetherion_imgui
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
  )
  target_include_directories(aetherion_imgui SYSTEM PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
  target_link_libraries(aetherion_imgui PUBLIC glfw OpenGL::GL)
  target_sources(aetherion_renderer PRIVATE src/renderer/opengl_backend.cpp)
  target_link_libraries(aetherion_renderer PRIVATE glfw aetherion_glad OpenGL::GL)
  target_compile_definitions(aetherion_renderer PUBLIC AETHERION_HAS_OPENGL=1)

  add_library(aetherion_ui src/ui/engineering_ui.cpp)
  add_library(Aetherion::ui ALIAS aetherion_ui)
  target_include_directories(aetherion_ui PUBLIC include)
  target_link_libraries(aetherion_ui PUBLIC Aetherion::core Aetherion::renderer
                        Aetherion::serialization PRIVATE aetherion_imgui)
  aetherion_set_project_warnings(aetherion_ui)

  add_executable(aetherion_renderer_smoke tests/renderer_smoke.cpp)
  target_link_libraries(aetherion_renderer_smoke PRIVATE Aetherion::renderer)
  aetherion_set_project_warnings(aetherion_renderer_smoke)

  add_executable(aetherion_ui_smoke tests/ui_smoke.cpp)
  target_link_libraries(aetherion_ui_smoke PRIVATE Aetherion::ui)
  aetherion_set_project_warnings(aetherion_ui_smoke)
endif()

add_executable(aetherion_app src/app/main.cpp)
target_link_libraries(aetherion_app PRIVATE Aetherion::core)
if(AETHERION_BUILD_RENDERER)
  target_link_libraries(aetherion_app PRIVATE Aetherion::renderer Aetherion::ui)
  target_compile_definitions(aetherion_app PRIVATE AETHERION_HAS_UI=1)
endif()
aetherion_set_project_warnings(aetherion_app)

add_executable(aetherion_headless src/app/headless_main.cpp)
target_link_libraries(aetherion_headless PRIVATE Aetherion::core)
aetherion_set_project_warnings(aetherion_headless)

add_executable(aetherion_compare src/app/comparison_main.cpp)
target_link_libraries(aetherion_compare PRIVATE Aetherion::core)
aetherion_set_project_warnings(aetherion_compare)

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
