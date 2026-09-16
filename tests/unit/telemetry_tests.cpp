#include "aetherion/core/telemetry.hpp"

#include <gtest/gtest.h>

using aetherion::core::Body;
using aetherion::core::Scene;
using aetherion::core::TelemetryRecorder;

TEST(Telemetry, ReportsMechanicalInvariantsInSiUnits) {
    Scene scene;
    ASSERT_TRUE(scene.createBody(
        Body{.name = "a",
             .mass_kg = 2.0,
             .radius_m = 1.0,
             .state = {.position_m = {-1.0, 0.0, 0.0}, .velocity_mps = {0.0, 3.0, 0.0}}}));
    ASSERT_TRUE(scene.createBody(
        Body{.name = "b",
             .mass_kg = 2.0,
             .radius_m = 1.0,
             .state = {.position_m = {1.0, 0.0, 0.0}, .velocity_mps = {0.0, -3.0, 0.0}}}));
    TelemetryRecorder recorder(4);
    const auto sample = recorder.sample(scene, 2.5);
    EXPECT_DOUBLE_EQ(sample.time_s, 2.5);
    EXPECT_DOUBLE_EQ(sample.kinetic_energy_J, 18.0);
    EXPECT_NEAR(sample.linear_momentum_kg_mps.norm(), 0.0, 1.0e-15);
    EXPECT_EQ(sample.center_of_mass_m, (aetherion::math::Vec3d{}));
    EXPECT_EQ(recorder.samples().size(), 1U);
}
