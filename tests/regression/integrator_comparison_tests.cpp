#include "aetherion/physics/integrator_comparison.hpp"
#include "aetherion/presets/mechanics_presets.hpp"

#include <gtest/gtest.h>

using aetherion::physics::IntegratorComparison;
using aetherion::physics::IntegratorKind;

TEST(IntegratorComparison, EarthOrbitDemonstratesExpectedErrorAndStabilityOrdering) {
    const auto initial = aetherion::presets::makeEarthLikeOrbit();
    constexpr std::size_t steps = 365;
    const double dt_s = aetherion::presets::earth_like_orbit_period_s / static_cast<double>(steps);
    const auto report = IntegratorComparison::run(initial, dt_s, steps);
    ASSERT_EQ(report.runs.size(), 3U);
    const auto& euler = report.forIntegrator(IntegratorKind::semi_implicit_euler);
    const auto& verlet = report.forIntegrator(IntegratorKind::velocity_verlet);
    const auto& rk4 = report.forIntegrator(IntegratorKind::rk4);
    EXPECT_LT(verlet.maximum_relative_energy_error, euler.maximum_relative_energy_error * 0.1);
    EXPECT_LT(rk4.maximum_relative_energy_error, verlet.maximum_relative_energy_error * 0.1);
    EXPECT_LT(verlet.relative_orbit_closure_error, euler.relative_orbit_closure_error);
    EXPECT_LT(rk4.relative_orbit_closure_error, verlet.relative_orbit_closure_error);
    EXPECT_LT(rk4.relative_orbit_closure_error, 1.0e-7);
}

TEST(IntegratorComparison, Rk4ShowsFourthOrderConvergenceTrend) {
    const auto initial = aetherion::presets::makeEarthLikeOrbit();
    const double duration_s = aetherion::presets::earth_like_orbit_period_s / 8.0;
    const auto coarse = IntegratorComparison::run(initial, duration_s / 16.0, 16)
                            .forIntegrator(IntegratorKind::rk4)
                            .relative_orbit_reference_error;
    const auto fine = IntegratorComparison::run(initial, duration_s / 32.0, 32)
                          .forIntegrator(IntegratorKind::rk4)
                          .relative_orbit_reference_error;
    EXPECT_GT(coarse / fine, 12.0);
}
