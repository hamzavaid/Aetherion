add_executable(aetherion_tests
  unit/error_tests.cpp
  unit/log_tests.cpp
  unit/vec3d_tests.cpp
)
target_link_libraries(aetherion_tests PRIVATE Aetherion::core GTest::gtest_main)
aetherion_set_project_warnings(aetherion_tests)

include(GoogleTest)
gtest_discover_tests(aetherion_tests)
