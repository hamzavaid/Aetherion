#include "aetherion/core/simulation_controller.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <numbers>

#include "aetherion/presets/em_presets.hpp"

TEST(BorisGyroRegression, ControllerMaintainsSpeedAndBoundedOrbitClosureForOneHundredPeriods) {
    auto preset = aetherion::presets::makeUniformMagneticGyroPreset();
    const auto initial_position = preset.scene.bodies()[0].state.position_m;
    const double initial_speed = preset.scene.bodies()[0].state.velocity_mps.norm();
    aetherion::core::SimulationController controller(std::move(preset.scene), preset.runtime);
    constexpr std::size_t periods = 100;
    constexpr std::size_t steps_per_period = 200;
    for (std::size_t step = 0; step < periods * steps_per_period; ++step)
        ASSERT_TRUE(controller.singleStep());
    const auto& particle = controller.scene().bodies()[0];
    EXPECT_NEAR(particle.state.velocity_mps.norm(), initial_speed, 2.0e-12);
    EXPECT_LT((particle.state.position_m - initial_position).norm(), 0.06);
    ASSERT_FALSE(controller.telemetry().samples().empty());
    EXPECT_LT(controller.telemetry().samples().back().maximum_speed_drift_mps, 2.0e-12);
}

TEST(BorisGyroRegression, BorisSpeedDriftIsLowerThanSemiImplicitEuler) {
    auto boris_preset = aetherion::presets::makeUniformMagneticGyroPreset();
    auto euler_preset = boris_preset;
    euler_preset.runtime.integrator = aetherion::physics::IntegratorKind::semi_implicit_euler;
    aetherion::core::SimulationController boris(std::move(boris_preset.scene),
                                                boris_preset.runtime);
    aetherion::core::SimulationController euler(std::move(euler_preset.scene),
                                                euler_preset.runtime);
    constexpr std::size_t steps = 2'000;
    for (std::size_t step = 0; step < steps; ++step) {
        ASSERT_TRUE(boris.singleStep());
        ASSERT_TRUE(euler.singleStep());
    }
    const double boris_drift = boris.telemetry().samples().back().maximum_speed_drift_mps;
    const double euler_drift = euler.telemetry().samples().back().maximum_speed_drift_mps;
    EXPECT_LT(boris_drift, 1.0e-11);
    EXPECT_GT(euler_drift, 1.0e-2);
    EXPECT_LT(boris_drift, euler_drift * 1.0e-6);
}
