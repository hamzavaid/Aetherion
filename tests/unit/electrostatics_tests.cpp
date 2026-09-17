#include "aetherion/physics/em/electrostatics.hpp"

#include <gtest/gtest.h>

#include <cmath>

#include "aetherion/physics/constants.hpp"

using aetherion::core::Body;
using aetherion::core::Scene;
using aetherion::physics::constants::coulomb_constant;
using aetherion::physics::em::ElectromagneticFieldProvider;
using aetherion::physics::em::ElectromagneticSettings;
using aetherion::physics::em::ElectrostaticSolver;

TEST(Electrostatics, AnalyticalMagnitudeAndLikeChargeRepulsion) {
    Scene scene;
    ASSERT_TRUE(scene.createBody(Body{.name = "left",
                                      .mass_kg = 2.0,
                                      .charge_C = 3.0e-6,
                                      .radius_m = 0.1,
                                      .state = {.position_m = {-1.0, 0.0, 0.0}}}));
    ASSERT_TRUE(scene.createBody(Body{.name = "right",
                                      .mass_kg = 4.0,
                                      .charge_C = 5.0e-6,
                                      .radius_m = 0.1,
                                      .state = {.position_m = {1.0, 0.0, 0.0}}}));
    ElectrostaticSolver solver;
    ASSERT_TRUE(solver.accumulateAccelerations(scene));
    const double expected_force_N = coulomb_constant * 3.0e-6 * 5.0e-6 / 4.0;
    EXPECT_NEAR(scene.bodies()[0].state.acceleration_mps2.x, -expected_force_N / 2.0,
                expected_force_N * 1.0e-13);
    EXPECT_NEAR(scene.bodies()[1].state.acceleration_mps2.x, expected_force_N / 4.0,
                expected_force_N * 1.0e-13);
    EXPECT_EQ(solver.diagnostics().evaluated_pairs, 1U);
}

TEST(Electrostatics, OppositeChargesAttractAndZeroChargeDoesNothing) {
    Scene scene;
    ASSERT_TRUE(scene.createBody(Body{.name = "positive",
                                      .mass_kg = 1.0,
                                      .charge_C = 1.0e-6,
                                      .radius_m = 0.1,
                                      .state = {.position_m = {-1.0, 0.0, 0.0}}}));
    ASSERT_TRUE(scene.createBody(Body{.name = "negative",
                                      .mass_kg = 1.0,
                                      .charge_C = -1.0e-6,
                                      .radius_m = 0.1,
                                      .state = {.position_m = {1.0, 0.0, 0.0}}}));
    ASSERT_TRUE(scene.createBody(Body{.name = "neutral",
                                      .mass_kg = 1.0,
                                      .charge_C = 0.0,
                                      .radius_m = 0.1,
                                      .state = {.position_m = {0.0, 2.0, 0.0}}}));
    ElectrostaticSolver solver;
    ASSERT_TRUE(solver.accumulateAccelerations(scene));
    EXPECT_GT(scene.bodies()[0].state.acceleration_mps2.x, 0.0);
    EXPECT_LT(scene.bodies()[1].state.acceleration_mps2.x, 0.0);
    EXPECT_EQ(scene.bodies()[2].state.acceleration_mps2, aetherion::math::Vec3d{});
}

TEST(ElectricField, AnalyticalSampleAndMultipleChargeSuperposition) {
    Scene scene;
    ASSERT_TRUE(scene.createBody(Body{.name = "q1",
                                      .mass_kg = 1.0,
                                      .charge_C = 2.0e-9,
                                      .radius_m = 0.01,
                                      .state = {.position_m = {-1.0, 0.0, 0.0}}}));
    ASSERT_TRUE(scene.createBody(Body{.name = "q2",
                                      .mass_kg = 1.0,
                                      .charge_C = 2.0e-9,
                                      .radius_m = 0.01,
                                      .state = {.position_m = {1.0, 0.0, 0.0}}}));
    ElectromagneticSettings settings;
    settings.minimum_separation_m = 1.0e-6;
    ElectromagneticFieldProvider provider(scene, settings);
    const auto center = provider.sample({}, 0.0);
    ASSERT_TRUE(center.valid);
    EXPECT_NEAR(center.electric_Vpm.norm(), 0.0, 1.0e-14);
    const auto sample = provider.sample({0.0, 1.0, 0.0}, 0.0);
    ASSERT_TRUE(sample.valid);
    const double expected_y = 2.0 * coulomb_constant * 2.0e-9 / (2.0 * std::sqrt(2.0));
    EXPECT_NEAR(sample.electric_Vpm.x, 0.0, 1.0e-13);
    EXPECT_NEAR(sample.electric_Vpm.y, expected_y, expected_y * 1.0e-12);
}

TEST(ElectricField, GuardedSingularityIsInvalidAndZeroChargeIsFinite) {
    Scene scene;
    ASSERT_TRUE(scene.createBody(
        Body{.name = "charged", .mass_kg = 1.0, .charge_C = 1.0, .radius_m = 0.1}));
    ElectromagneticSettings settings;
    settings.minimum_separation_m = 0.01;
    ElectromagneticFieldProvider provider(scene, settings);
    EXPECT_FALSE(provider.sample({0.005, 0.0, 0.0}, 0.0).valid);
    scene.bodies()[0].charge_C = 0.0;
    const auto zero = provider.sample({}, 0.0);
    EXPECT_TRUE(zero.valid);
    EXPECT_EQ(zero.electric_Vpm, aetherion::math::Vec3d{});
}
