#include "aetherion/core/simulation_controller.hpp"

#include <gtest/gtest.h>

using aetherion::core::Body;
using aetherion::core::BodyPatch;
using aetherion::core::CreateBodyCommand;
using aetherion::core::Scene;
using aetherion::core::SetTimeScaleCommand;
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

TEST(ControllerWorkflow, ResetRestoresLatestSavedSceneSettingsAndSimulationTime) {
    Scene initial;
    const auto id = initial.createBody(Body{.name = "body",
                                            .mass_kg = 1.0,
                                            .radius_m = 1.0,
                                            .state = {.velocity_mps = {2.0, 0.0, 0.0}}});
    ASSERT_TRUE(id);
    SimulationController controller(std::move(initial), {.physics_dt_s = 0.5});
    ASSERT_TRUE(controller.singleStep());
    controller.commands().enqueue(
        UpdateBodyCommand{.id = id.value(), .patch = BodyPatch{.mass_kg = 4.0}});
    controller.commands().enqueue(SetTimeScaleCommand{3.0});
    const auto saved_id = controller.saveCheckpoint();
    EXPECT_EQ(controller.checkpoints().back().id, saved_id);
    EXPECT_DOUBLE_EQ(controller.scene().find(id.value())->mass_kg, 4.0);
    ASSERT_TRUE(controller.singleStep());
    controller.commands().enqueue(
        UpdateBodyCommand{.id = id.value(), .patch = BodyPatch{.mass_kg = 9.0}});
    controller.commands().enqueue(SetTimeScaleCommand{7.0});
    ASSERT_TRUE(controller.update(0.0));

    controller.reset();

    EXPECT_DOUBLE_EQ(controller.simulationTimeSeconds(), 0.5);
    EXPECT_DOUBLE_EQ(controller.scene().find(id.value())->state.position_m.x, 1.0);
    EXPECT_DOUBLE_EQ(controller.scene().find(id.value())->mass_kg, 4.0);
    EXPECT_DOUBLE_EQ(controller.settings().time_scale, 3.0);
    EXPECT_FALSE(controller.isPlaying());
}

TEST(ControllerWorkflow, SaveHistoryCanRestoreAnEarlierCheckpoint) {
    Scene initial;
    const auto id = initial.createBody(Body{.name = "body",
                                            .mass_kg = 1.0,
                                            .radius_m = 1.0,
                                            .state = {.velocity_mps = {1.0, 0.0, 0.0}}});
    ASSERT_TRUE(id);
    SimulationController controller(std::move(initial), {.physics_dt_s = 1.0});
    const auto first_save = controller.saveCheckpoint();
    ASSERT_TRUE(controller.singleStep());
    const auto second_save = controller.saveCheckpoint();
    ASSERT_NE(first_save, second_save);
    ASSERT_EQ(controller.checkpoints().size(), 2U);
    EXPECT_DOUBLE_EQ(controller.checkpoints()[1].simulation_time_s, 1.0);

    ASSERT_TRUE(controller.restoreCheckpoint(first_save));
    EXPECT_DOUBLE_EQ(controller.simulationTimeSeconds(), 0.0);
    EXPECT_DOUBLE_EQ(controller.scene().find(id.value())->state.position_m.x, 0.0);
    EXPECT_FALSE(controller.restoreCheckpoint(999'999U));
}

TEST(ControllerWorkflow, SaveHistoryRetainsLatestSixtyFourCheckpoints) {
    Scene initial;
    ASSERT_TRUE(initial.createBody(Body{.name = "body", .mass_kg = 1.0, .radius_m = 1.0}));
    SimulationController controller(std::move(initial));
    for (std::size_t index = 0; index < 66U; ++index)
        static_cast<void>(controller.saveCheckpoint());
    ASSERT_EQ(controller.checkpoints().size(), 64U);
    EXPECT_EQ(controller.checkpoints().front().id, 3U);
    EXPECT_EQ(controller.checkpoints().back().id, 66U);
    EXPECT_FALSE(controller.restoreCheckpoint(1U));
}
