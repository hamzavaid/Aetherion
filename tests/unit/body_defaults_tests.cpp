#include "aetherion/core/body_defaults.hpp"

#include <gtest/gtest.h>

#include "aetherion/core/simulation.hpp"

TEST(BodyDefaults, ElectrostaticContextCreatesChargedNonCoincidentDynamicBody) {
    aetherion::core::Scene scene;
    ASSERT_TRUE(scene.createBody({.name = "left",
                                  .mass_kg = 1.0,
                                  .charge_C = 1.0e-6,
                                  .radius_m = 0.08,
                                  .fixed = true,
                                  .state = {.position_m = {-1.0, 0.0, 0.0}}}));
    ASSERT_TRUE(scene.createBody({.name = "right",
                                  .mass_kg = 1.0,
                                  .charge_C = 1.0e-6,
                                  .radius_m = 0.08,
                                  .fixed = true,
                                  .state = {.position_m = {1.0, 0.0, 0.0}}}));
    aetherion::core::RuntimeSettings runtime;
    runtime.gravity_enabled = false;
    runtime.electromagnetism.electrostatics_enabled = true;
    const auto body = aetherion::core::makeInteractiveBody(scene, runtime, "test charge");
    EXPECT_NE(body.charge_C, 0.0);
    EXPECT_GT(body.state.position_m.x, 1.0);
    ASSERT_TRUE(scene.createBody(body));
    aetherion::core::Simulation simulation(scene, {.physics_dt_s = 1.0e-5});
    simulation.setGravityEnabled(false);
    simulation.setElectromagneticSettings(runtime.electromagnetism);
    ASSERT_TRUE(simulation.step());
    EXPECT_GT(scene.bodies().back().state.velocity_mps.norm(), 0.0);
}

TEST(BodyDefaults, MechanicalContextKeepsNewBodyNeutral) {
    const aetherion::core::Scene scene;
    const aetherion::core::RuntimeSettings runtime;
    const auto body = aetherion::core::makeInteractiveBody(scene, runtime, "neutral");
    EXPECT_DOUBLE_EQ(body.charge_C, 0.0);
    EXPECT_EQ(body.name, "neutral");
}
