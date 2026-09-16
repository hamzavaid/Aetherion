add_executable(aetherion_tests
  unit/error_tests.cpp
  unit/log_tests.cpp
  unit/scene_tests.cpp
  unit/simulation_clock_tests.cpp
  unit/gravity_tests.cpp
  unit/integrator_tests.cpp
  unit/input_routing_tests.cpp
  regression/integrator_comparison_tests.cpp
  unit/camera_tests.cpp
  unit/adaptive_grid_tests.cpp
  unit/command_queue_tests.cpp
  integration/controller_workflow_tests.cpp
  unit/mesh_tests.cpp
  unit/picking_tests.cpp
  unit/render_data_tests.cpp
  unit/trail_history_tests.cpp
  unit/semi_implicit_euler_tests.cpp
  regression/earth_orbit_tests.cpp
  unit/telemetry_tests.cpp
  unit/stability_tests.cpp
  unit/vec3d_tests.cpp
)
target_link_libraries(aetherion_tests PRIVATE Aetherion::core Aetherion::renderer GTest::gtest_main)
aetherion_set_project_warnings(aetherion_tests)

include(GoogleTest)
gtest_discover_tests(aetherion_tests)
