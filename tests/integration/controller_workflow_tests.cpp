#include "aetherion/core/simulation_controller.hpp"

#include <gtest/gtest.h>

using aetherion::core::Body;
using aetherion::core::BodyPatch;
using aetherion::core::CreateBodyCommand;
using aetherion::core::Scene;
using aetherion::core::SimulationController;
using aetherion::core::UpdateBodyCommand;

TEST(ControllerWorkflow, CreatesEditsAndRunsBodyWithoutRestarting) {
    Scene initial;
    ASSERT_TRUE(initial.createBody(
        Body{.name = "source", .mass_kg = 5.0e14, .radius_m = 1.0, .fixed = true}));
    SimulationController controller(std::move(initial), {.physics_dt_s = 0.01, .max_substeps = 16});
    controller.commands().enqueue(CreateBodyCommand{
        .body = Body{.name = "orbiter",
                     .mass_kg = 1.0,
                     .radius_m = 0.1,
                     .state = {.position_m = {10.0, 0.0, 0.0}, .velocity_mps = {0.0, 1.0, 0.0}}}});
    ASSERT_TRUE(controller.update(0.0));
    ASSERT_EQ(controller.scene().size(), 2U);
    const auto orbiter_id = controller.scene().bodies()[1].id;
    controller.commands().enqueue(
        UpdateBodyCommand{.id = orbiter_id, .patch = BodyPatch{.velocity_mps = {{0.0, 2.0, 0.0}}}});
    controller.setPlaying(true);
    ASSERT_TRUE(controller.update(0.1));
    EXPECT_GT(controller.scene().find(orbiter_id)->state.position_m.y, 0.0);
    EXPECT_NEAR(controller.simulationTimeSeconds(), 0.1, 1.0e-14);
    EXPECT_GE(controller.commands().eventLog().size(), 2U);
}

TEST(ControllerWorkflow, PauseSingleStepAndResetHaveExplicitSemantics) {
    Scene initial;
    const auto id = initial.createBody(Body{.name = "body",
                                            .mass_kg = 1.0,
                                            .radius_m = 1.0,
                                            .state = {.velocity_mps = {1.0, 0.0, 0.0}}});
    ASSERT_TRUE(id);
    SimulationController controller(std::move(initial), {.physics_dt_s = 0.5});
    ASSERT_TRUE(controller.singleStep());
    EXPECT_DOUBLE_EQ(controller.simulationTimeSeconds(), 0.5);
    EXPECT_DOUBLE_EQ(controller.scene().find(id.value())->state.position_m.x, 0.5);
    controller.reset();
    EXPECT_DOUBLE_EQ(controller.simulationTimeSeconds(), 0.0);
    EXPECT_DOUBLE_EQ(controller.scene().find(id.value())->state.position_m.x, 0.0);
    EXPECT_FALSE(controller.isPlaying());
}
