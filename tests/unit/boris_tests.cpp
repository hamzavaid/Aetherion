#include "aetherion/physics/integrators/boris.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <numbers>

using aetherion::core::Body;
using aetherion::core::Scene;
using aetherion::math::Vec3d;
using aetherion::physics::BorisIntegrator;

TEST(Boris, PureMagneticFieldPreservesSpeedAcrossManyGyroPeriods) {
    Scene scene;
    ASSERT_TRUE(scene.createBody(Body{.name = "particle",
                                      .mass_kg = 1.0,
                                      .charge_C = 1.0,
                                      .radius_m = 0.01,
                                      .state = {.velocity_mps = {1.0, 0.0, 0.0}}}));
    BorisIntegrator boris;
    const auto zero_acceleration = [](Scene& evaluated) {
        for (auto& body : evaluated.bodies())
            body.state.acceleration_mps2 = {};
        return aetherion::core::success();
    };
    const auto uniform_b = [](const Vec3d&) -> aetherion::core::Result<Vec3d> {
        return Vec3d{0.0, 0.0, 1.0};
    };
    constexpr std::size_t steps_per_period = 200;
    constexpr std::size_t periods = 100;
    const double dt = 2.0 * std::numbers::pi / static_cast<double>(steps_per_period);
    for (std::size_t step = 0; step < steps_per_period * periods; ++step)
        ASSERT_TRUE(boris.step(scene, dt, zero_acceleration, uniform_b));
    EXPECT_NEAR(scene.bodies()[0].state.velocity_mps.norm(), 1.0, 2.0e-13);
    EXPECT_LT(scene.bodies()[0].state.position_m.norm(), 0.06);
}

TEST(Boris, ElectricKickAndMagneticRotationKeepFixedAndNeutralSemantics) {
    Scene scene;
    ASSERT_TRUE(scene.createBody(Body{.name = "charged",
                                      .mass_kg = 2.0,
                                      .charge_C = 1.0,
                                      .radius_m = 0.1,
                                      .state = {.velocity_mps = {1.0, 0.0, 0.0}}}));
    ASSERT_TRUE(scene.createBody(Body{.name = "fixed",
                                      .mass_kg = 2.0,
                                      .charge_C = 1.0,
                                      .radius_m = 0.1,
                                      .fixed = true,
                                      .state = {.velocity_mps = {1.0, 0.0, 0.0}}}));
    BorisIntegrator boris;
    const auto electric_acceleration = [](Scene& evaluated) {
        for (auto& body : evaluated.bodies())
            body.state.acceleration_mps2 = {2.0, 0.0, 0.0};
        return aetherion::core::success();
    };
    const auto zero_b = [](const Vec3d&) -> aetherion::core::Result<Vec3d> { return Vec3d{}; };
    ASSERT_TRUE(boris.step(scene, 0.5, electric_acceleration, zero_b));
    EXPECT_NEAR(scene.bodies()[0].state.velocity_mps.x, 2.0, 1.0e-14);
    EXPECT_EQ(scene.bodies()[1].state.position_m, Vec3d{});
    EXPECT_EQ(scene.bodies()[1].state.velocity_mps, (Vec3d{1.0, 0.0, 0.0}));
}

TEST(Boris, NeutralParticleDoesNotSampleInvalidMagneticField) {
    Scene scene;
    ASSERT_TRUE(scene.createBody(Body{.name = "neutral",
                                      .mass_kg = 1.0,
                                      .radius_m = 0.1,
                                      .state = {.velocity_mps = {1.0, 0.0, 0.0}}}));
    BorisIntegrator boris;
    const auto zero_acceleration = [](Scene& evaluated) {
        evaluated.bodies()[0].state.acceleration_mps2 = {};
        return aetherion::core::success();
    };
    const auto invalid_field = [](const Vec3d&) -> aetherion::core::Result<Vec3d> {
        return aetherion::core::Error{aetherion::core::ErrorCode::numerical_failure,
                                      "guarded singularity"};
    };
    ASSERT_TRUE(boris.step(scene, 0.25, zero_acceleration, invalid_field));
    EXPECT_EQ(scene.bodies()[0].state.velocity_mps, (Vec3d{1.0, 0.0, 0.0}));
    EXPECT_EQ(scene.bodies()[0].state.position_m, (Vec3d{0.25, 0.0, 0.0}));
}
