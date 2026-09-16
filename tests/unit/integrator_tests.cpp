#include "aetherion/physics/integrators/rk4.hpp"
#include "aetherion/physics/integrators/velocity_verlet.hpp"

#include <gtest/gtest.h>

using aetherion::core::Body;
using aetherion::core::Scene;
using aetherion::physics::Rk4Integrator;
using aetherion::physics::VelocityVerletIntegrator;

namespace {

aetherion::core::Status constantAcceleration(Scene& scene) {
    for (auto& body : scene.bodies())
        body.state.acceleration_mps2 = {2.0, -4.0, 1.0};
    return aetherion::core::success();
}

Scene constantAccelerationScene() {
    Scene scene;
    const auto result = scene.createBody(
        Body{.name = "body",
             .mass_kg = 1.0,
             .radius_m = 1.0,
             .state = {.position_m = {1.0, 2.0, 3.0}, .velocity_mps = {4.0, 5.0, 6.0}}});
    EXPECT_TRUE(result);
    return scene;
}

} // namespace

TEST(VelocityVerlet, IsExactForConstantAcceleration) {
    auto scene = constantAccelerationScene();
    VelocityVerletIntegrator integrator;
    ASSERT_TRUE(integrator.step(scene, 0.5, constantAcceleration));
    EXPECT_EQ(scene.bodies()[0].state.position_m, (aetherion::math::Vec3d{3.25, 4.0, 6.125}));
    EXPECT_EQ(scene.bodies()[0].state.velocity_mps, (aetherion::math::Vec3d{5.0, 3.0, 6.5}));
}

TEST(Rk4, IsExactForConstantAcceleration) {
    auto scene = constantAccelerationScene();
    Rk4Integrator integrator;
    ASSERT_TRUE(integrator.step(scene, 0.5, constantAcceleration));
    EXPECT_NEAR(scene.bodies()[0].state.position_m.x, 3.25, 1.0e-14);
    EXPECT_NEAR(scene.bodies()[0].state.position_m.y, 4.0, 1.0e-14);
    EXPECT_NEAR(scene.bodies()[0].state.velocity_mps.z, 6.5, 1.0e-14);
}
