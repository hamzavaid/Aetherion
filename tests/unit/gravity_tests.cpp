#include "aetherion/physics/gravity/gravity_solver.hpp"

#include <gtest/gtest.h>

#include <numbers>

#include "aetherion/physics/constants.hpp"
#include "aetherion/physics/em/electrostatics.hpp"

using aetherion::core::Body;
using aetherion::core::Scene;
using aetherion::physics::GravitySolver;

TEST(GravitySolver, ComputesAnalyticalMagnitudeDirectionAndForceSymmetry) {
    Scene scene;
    ASSERT_TRUE(scene.createBody(
        Body{.name = "a", .mass_kg = 2.0, .radius_m = 0.1, .state = {.position_m = {0, 0, 0}}}));
    ASSERT_TRUE(scene.createBody(
        Body{.name = "b", .mass_kg = 3.0, .radius_m = 0.1, .state = {.position_m = {2, 0, 0}}}));

    GravitySolver solver;
    const auto result = solver.computeAccelerations(scene);
    ASSERT_TRUE(result);
    const auto& bodies = scene.bodies();
    const double expected_a = aetherion::physics::constants::gravitational_constant * 3.0 / 4.0;
    EXPECT_NEAR(bodies[0].state.acceleration_mps2.x, expected_a, expected_a * 1.0e-14);
    EXPECT_NEAR(bodies[1].state.acceleration_mps2.x,
                -aetherion::physics::constants::gravitational_constant * 2.0 / 4.0,
                expected_a * 1.0e-14);
    const auto total_force = bodies[0].mass_kg * bodies[0].state.acceleration_mps2 +
                             bodies[1].mass_kg * bodies[1].state.acceleration_mps2;
    EXPECT_NEAR(total_force.norm(), 0.0, 1.0e-25);
}

TEST(GravitySolver, CoincidentBodiesAreSkippedAndDiagnosed) {
    Scene scene;
    ASSERT_TRUE(scene.createBody(Body{.name = "a", .mass_kg = 1.0, .radius_m = 1.0}));
    ASSERT_TRUE(scene.createBody(Body{.name = "b", .mass_kg = 1.0, .radius_m = 1.0}));
    GravitySolver solver;
    ASSERT_TRUE(solver.computeAccelerations(scene));
    EXPECT_EQ(solver.diagnostics().coincident_pairs, 1U);
    EXPECT_EQ(scene.bodies()[0].state.acceleration_mps2, (aetherion::math::Vec3d{}));
}

TEST(GravityField, SamplesAnalyticalAccelerationAndGuardsBodyInterior) {
    Scene scene;
    ASSERT_TRUE(scene.createBody(Body{.name = "source",
                                      .mass_kg = 5.0e10,
                                      .radius_m = 2.0,
                                      .fixed = true,
                                      .state = {.position_m = {1.0, 0.0, 0.0}}}));
    const aetherion::physics::em::ElectromagneticSettings em;
    const aetherion::physics::em::ElectromagneticFieldProvider provider(scene, em);

    const auto sample = provider.sample({4.0, 0.0, 0.0}, 0.0);
    ASSERT_TRUE(sample.valid);
    const double expected = aetherion::physics::constants::gravitational_constant * 5.0e10 / 9.0;
    EXPECT_NEAR(sample.gravity_mps2.x, -expected, expected * 1.0e-14);
    EXPECT_NEAR(sample.gravity_mps2.y, 0.0, expected * 1.0e-14);
    EXPECT_NEAR(sample.gravity_mps2.z, 0.0, expected * 1.0e-14);

    EXPECT_FALSE(provider.sample({2.0, 0.0, 0.0}, 0.0).gravity_valid);
}
