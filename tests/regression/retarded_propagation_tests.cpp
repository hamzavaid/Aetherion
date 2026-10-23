// Finite probe-signal arrival, deterministic replay, accepted-state recording, and edit/reset
// boundaries.
#include "aetherion/core/simulation_controller.hpp"
#include "aetherion/presets/em_presets.hpp"

#include <gtest/gtest.h>

#include "aetherion/physics/constants.hpp"

using namespace aetherion;

TEST(RetardedPropagation, AcceptedStepsYieldCorrectTwoProbeFlightTimesAndDeterminism) {
    const auto run = [] {
        auto preset = presets::makePropagationDelayPreset();
        core::SimulationController controller(std::move(preset.scene), preset.runtime);
        double near_first_s = -1.0;
        double far_first_s = -1.0;
        for (int step = 0; step < 80; ++step) {
            EXPECT_TRUE(controller.singleStep());
            const auto near = controller.fieldProvider().sample({3.0, 0.0, 0.0},
                                                                controller.simulationTimeSeconds());
            const auto far = controller.fieldProvider().sample({6.0, 0.0, 0.0},
                                                               controller.simulationTimeSeconds());
            if (near.valid && near_first_s < 0.0)
                near_first_s = controller.simulationTimeSeconds();
            if (far.valid && far_first_s < 0.0)
                far_first_s = controller.simulationTimeSeconds();
        }
        EXPECT_NEAR(near_first_s, 3.0 / physics::constants::speed_of_light_mps,
                    preset.runtime.physics_dt_s);
        EXPECT_NEAR(far_first_s - near_first_s, 3.0 / physics::constants::speed_of_light_mps,
                    preset.runtime.physics_dt_s);
        const auto id = controller.scene().bodies().front().id;
        const auto* source = controller.sourceHistories().find(id);
        EXPECT_NE(source, nullptr);
        if (source != nullptr) {
            EXPECT_EQ(source->history.size(), 81U);
            EXPECT_NEAR(source->history.newest().time_s, controller.simulationTimeSeconds(),
                        1.0e-22);
        }
        return controller.fieldProvider()
            .sample({3.0, 0.0, 0.0}, controller.simulationTimeSeconds())
            .electric_Vpm;
    };
    EXPECT_EQ(run(), run());
}

TEST(RetardedPropagation, EditResetAndInvalidSettingsRespectHistoryBoundaries) {
    auto preset = presets::makePropagationDelayPreset();
    core::SimulationController controller(std::move(preset.scene), preset.runtime);
    for (int step = 0; step < 30; ++step)
        ASSERT_TRUE(controller.singleStep());
    const auto id = controller.scene().bodies().front().id;
    controller.commands().enqueue(
        core::UpdateBodyCommand{id, {.position_m = math::Vec3d{0.0, 1.0, 0.0}}});
    ASSERT_TRUE(controller.update(0.0));
    ASSERT_NE(controller.sourceHistories().find(id), nullptr);
    EXPECT_EQ(controller.sourceHistories().find(id)->history.size(), 1U);
    EXPECT_FALSE(controller.fieldProvider()
                     .sample({3.0, 0.0, 0.0}, controller.simulationTimeSeconds())
                     .valid);
    controller.commands().enqueue(core::SetPhysicsDtCommand{1.0e-30});
    ASSERT_TRUE(controller.update(0.0));
    EXPECT_EQ(controller.commands().eventLog().back().accepted, false);
    controller.reset();
    EXPECT_DOUBLE_EQ(controller.simulationTimeSeconds(), 0.0);
    ASSERT_NE(controller.sourceHistories().find(id), nullptr);
    EXPECT_EQ(controller.sourceHistories().find(id)->history.size(), 1U);
}

TEST(RetardedPropagation, FailedPhysicsStepDoesNotRecordOrAdvanceSourceState) {
    auto preset = presets::makePropagationDelayPreset();
    preset.runtime.integrator = physics::IntegratorKind::boris;
    preset.runtime.electromagnetism.magnetic_enabled = true;
    preset.runtime.electromagnetism.analytic_sources.push_back(
        {.kind = physics::em::AnalyticFieldSourceKind::magnetic_dipole,
         .magnetic_dipole_moment_Am2 = {0.0, 0.0, 1.0},
         .singularity_radius_m = 1.0});
    core::SimulationController controller(std::move(preset.scene), preset.runtime);
    const auto id = controller.scene().bodies().front().id;
    const auto before = controller.scene().bodies().front().state;
    EXPECT_FALSE(controller.singleStep());
    EXPECT_DOUBLE_EQ(controller.simulationTimeSeconds(), 0.0);
    EXPECT_EQ(controller.sourceHistories().find(id)->history.size(), 1U);
    EXPECT_EQ(controller.scene().bodies().front().state.position_m, before.position_m);
    EXPECT_EQ(controller.scene().bodies().front().state.velocity_mps, before.velocity_mps);
}

TEST(RetardedPropagation, PlayingTimestepAndSamplingEditsUseOneConsistentStepConfiguration) {
    auto preset = presets::makePropagationDelayPreset();
    core::SimulationController controller(
        std::move(preset.scene),
        {.physics_dt_s = 1.0e-12, .time_scale = 1.0e-8, .gravity_enabled = false});
    controller.setPlaying(true);
    controller.commands().enqueue(core::SetPhysicsDtCommand{preset.runtime.physics_dt_s});
    controller.commands().enqueue(
        core::SetElectromagneticSettingsCommand{preset.runtime.electromagnetism});
    ASSERT_TRUE(controller.update(0.2));
    EXPECT_GE(controller.simulationTimeSeconds(), 1.0e-9);
    EXPECT_EQ(controller.settings().physics_dt_s, preset.runtime.physics_dt_s);
    ASSERT_NE(controller.sourceHistories().find(controller.scene().bodies().front().id), nullptr);
    EXPECT_GE(
        controller.sourceHistories().find(controller.scene().bodies().front().id)->history.size(),
        2U);
}
