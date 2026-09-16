#include "aetherion/core/simulation.hpp"
#include "aetherion/presets/mechanics_presets.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>

namespace {

struct OrbitResult {
    aetherion::math::Vec3d relative_position_m;
    aetherion::math::Vec3d relative_velocity_mps;
    double maximum_relative_radius_error{};
    double maximum_relative_energy_error{};
};

OrbitResult runOneYear() {
    auto scene = aetherion::presets::makeEarthLikeOrbit();
    aetherion::core::Simulation simulation(scene, {.physics_dt_s = 3600.0});
    const auto initial = simulation.telemetry().sample(scene, 0.0);
    const double year_s = aetherion::presets::earth_like_orbit_period_s;
    const auto step_count = static_cast<std::size_t>(std::llround(year_s / 3600.0));
    double maximum_radius_error = 0.0;
    double maximum_energy_error = 0.0;
    for (std::size_t step = 0; step < step_count; ++step) {
        EXPECT_TRUE(simulation.step());
        const auto relative_position =
            scene.bodies()[1].state.position_m - scene.bodies()[0].state.position_m;
        maximum_radius_error =
            std::max(maximum_radius_error,
                     std::abs(relative_position.norm() - aetherion::presets::astronomical_unit_m) /
                         aetherion::presets::astronomical_unit_m);
        const auto& sample = simulation.telemetry().samples().back();
        maximum_energy_error = std::max(
            maximum_energy_error,
            std::abs((sample.total_energy_J - initial.total_energy_J) / initial.total_energy_J));
    }
    const auto relative_position =
        scene.bodies()[1].state.position_m - scene.bodies()[0].state.position_m;
    const auto relative_velocity =
        scene.bodies()[1].state.velocity_mps - scene.bodies()[0].state.velocity_mps;
    return {.relative_position_m = relative_position,
            .relative_velocity_mps = relative_velocity,
            .maximum_relative_radius_error = maximum_radius_error,
            .maximum_relative_energy_error = maximum_energy_error};
}

} // namespace

TEST(EarthOrbitRegression, SemiImplicitEulerIsDeterministicAndBoundedForOneYear) {
    const OrbitResult first = runOneYear();
    const OrbitResult second = runOneYear();
    EXPECT_EQ(first.relative_position_m, second.relative_position_m);
    EXPECT_EQ(first.relative_velocity_mps, second.relative_velocity_mps);
    EXPECT_LT(first.maximum_relative_radius_error, 2.0e-3);
    EXPECT_LT(first.maximum_relative_energy_error, 2.0e-5);
}
