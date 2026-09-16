#include "aetherion/physics/integrators/semi_implicit_euler.hpp"

#include <gtest/gtest.h>

using aetherion::core::Body;
using aetherion::core::Scene;
using aetherion::physics::SemiImplicitEuler;

TEST(SemiImplicitEuler, AppliesVelocityBeforePositionForConstantAcceleration) {
    Scene scene;
    ASSERT_TRUE(scene.createBody(
        Body{.name = "body",
             .mass_kg = 1.0,
             .radius_m = 1.0,
             .state = {.velocity_mps = {1.0, 0.0, 0.0}, .acceleration_mps2 = {2.0, 0.0, 0.0}}}));
    ASSERT_TRUE(SemiImplicitEuler::integrate(scene, 0.5));
    EXPECT_EQ(scene.bodies()[0].state.velocity_mps, (aetherion::math::Vec3d{2.0, 0.0, 0.0}));
    EXPECT_EQ(scene.bodies()[0].state.position_m, (aetherion::math::Vec3d{1.0, 0.0, 0.0}));
}

TEST(SemiImplicitEuler, NeverIntegratesFixedBodies) {
    Scene scene;
    ASSERT_TRUE(scene.createBody(
        Body{.name = "fixed",
             .mass_kg = 1.0,
             .radius_m = 1.0,
             .fixed = true,
             .state = {.velocity_mps = {4.0, 0.0, 0.0}, .acceleration_mps2 = {2.0, 0.0, 0.0}}}));
    const auto initial = scene.bodies()[0].state;
    ASSERT_TRUE(SemiImplicitEuler::integrate(scene, 1.0));
    EXPECT_EQ(scene.bodies()[0].state, initial);
}
