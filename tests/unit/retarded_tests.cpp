#include "aetherion/physics/em/retarded_fields.hpp"

#include <algorithm>
#include <gtest/gtest.h>
#include <limits>

#include "aetherion/physics/constants.hpp"
#include "aetherion/physics/em/electrostatics.hpp"

using namespace aetherion;

TEST(SourceHistory, RingWrapBracketsAndLinearInterpolationNeverExtrapolate) {
    physics::em::HistoryBuffer history(3);
    for (int index = 0; index < 5; ++index) {
        const double t = static_cast<double>(index);
        ASSERT_TRUE(history.append({t, {2.0 * t, 0.0, 0.0}, {2.0, 0.0, 0.0}, {}}));
    }
    EXPECT_EQ(history.size(), 3U);
    EXPECT_DOUBLE_EQ(history.oldest().time_s, 2.0);
    EXPECT_DOUBLE_EQ(history.newest().time_s, 4.0);
    const auto bracket = history.bracket(2.5);
    ASSERT_TRUE(bracket);
    EXPECT_DOUBLE_EQ(bracket.value().before.time_s, 2.0);
    EXPECT_DOUBLE_EQ(bracket.value().after.time_s, 3.0);
    const auto state = history.interpolate(2.5);
    ASSERT_TRUE(state);
    EXPECT_NEAR(state.value().position_m.x, 5.0, 1.0e-14);
    EXPECT_FALSE(history.interpolate(1.9));
    EXPECT_FALSE(history.interpolate(4.1));
    EXPECT_FALSE(history.append({3.0, {}, {}, {}}));
    ASSERT_TRUE(history.append({4.0, {9.0, 0.0, 0.0}, {}, {}}));
    EXPECT_EQ(history.size(), 3U);
    EXPECT_DOUBLE_EQ(history.newest().position_m.x, 9.0);
}

TEST(RetardedTime, StationaryAndUniformVelocityMatchAnalyticalRoots) {
    constexpr double c = physics::constants::speed_of_light_mps;
    physics::em::HistoryBuffer stationary(3);
    ASSERT_TRUE(stationary.append({0.0, {}, {}, {}}));
    ASSERT_TRUE(stationary.append({2.0, {}, {}, {}}));
    const auto static_result = physics::em::solveRetardedTime(stationary, {c, 0.0, 0.0}, 2.0, {});
    ASSERT_EQ(static_result.status, physics::em::RetardedStatus::converged);
    EXPECT_NEAR(static_result.retarded_time_s, 1.0, 1.0e-11);
    physics::em::HistoryBuffer moving(3);
    const double v = 0.1 * c;
    ASSERT_TRUE(moving.append({0.0, {}, {v, 0.0, 0.0}, {}}));
    ASSERT_TRUE(moving.append({2.0, {2.0 * v, 0.0, 0.0}, {v, 0.0, 0.0}, {}}));
    const auto result = physics::em::solveRetardedTime(moving, {c, 0.0, 0.0}, 2.0, {});
    ASSERT_EQ(result.status, physics::em::RetardedStatus::converged);
    EXPECT_NEAR(result.retarded_time_s, 1.0 / 0.9, 1.0e-10);
    EXPECT_LE(result.residual_s, 1.0e-10);
}

TEST(RetardedTime, FailuresAreTypedAndNeverReturnUndefinedState) {
    physics::em::HistoryBuffer history(3);
    ASSERT_TRUE(history.append({0.0, {}, {}, {}}));
    ASSERT_TRUE(history.append({1.0, {}, {}, {}}));
    const double c = physics::constants::speed_of_light_mps;
    EXPECT_EQ(physics::em::solveRetardedTime(history, {c, 0.0, 0.0}, 0.1, {}).status,
              physics::em::RetardedStatus::history_underrun);
    EXPECT_EQ(physics::em::solveRetardedTime(history, {1.0, 0.0, 0.0}, 2.0, {}).status,
              physics::em::RetardedStatus::history_future);
    EXPECT_EQ(physics::em::solveRetardedTime(history, {}, 0.5, {}).status,
              physics::em::RetardedStatus::singularity);
    physics::em::RetardedSettings settings;
    settings.maximum_iterations = 1;
    ASSERT_TRUE(history.append({1.0, {0.1 * c, 0.0, 0.0}, {0.1 * c, 0.0, 0.0}, {}}));
    EXPECT_EQ(physics::em::solveRetardedTime(history, {c, 0.0, 0.0}, 1.0, settings).status,
              physics::em::RetardedStatus::non_converged);
}

TEST(SourceHistory, CapacityUsesLightTravelHorizonAndRejectsUnboundedWork) {
    physics::em::RetardedSettings settings;
    settings.enabled = true;
    settings.maximum_observable_distance_m = physics::constants::speed_of_light_mps;
    const auto capacity = physics::em::historyCapacity(settings, 0.25);
    ASSERT_TRUE(capacity);
    EXPECT_EQ(capacity.value(), 8U);
    EXPECT_FALSE(physics::em::historyCapacity(settings, 1.0e-20));
    settings.maximum_iterations = 0;
    EXPECT_FALSE(physics::em::validateRetardedSettings(settings));
}

TEST(RetardedField, StaticLimitAndProbeSignalsHaveFiniteArrivalDelay) {
    core::Scene scene;
    const auto id =
        scene.createBody({.name = "source", .mass_kg = 1.0, .charge_C = 1.0e-6, .radius_m = 0.01});
    ASSERT_TRUE(id);
    physics::em::ElectromagneticSettings settings;
    settings.retarded.enabled = true;
    settings.retarded.maximum_observable_distance_m = 30.0;
    physics::em::SourceHistories histories;
    ASSERT_TRUE(histories.configure(settings.retarded, 1.0e-9));
    const double speed = 0.01 * physics::constants::speed_of_light_mps;
    for (int step = -50; step <= 50; ++step) {
        const double t = static_cast<double>(step) * 1.0e-9;
        scene.find(id.value())->state.position_m.y = speed * std::max(t, 0.0);
        scene.find(id.value())->state.velocity_mps.y = t >= 0.0 ? speed : 0.0;
        ASSERT_TRUE(histories.record(scene, t));
    }
    physics::em::ElectromagneticFieldProvider instant(scene, settings);
    physics::em::RetardedFieldProvider delayed(scene, settings, histories, instant);
    const double near_arrival = 3.0 / physics::constants::speed_of_light_mps;
    const double far_arrival = 6.0 / physics::constants::speed_of_light_mps;
    const auto before = delayed.sample({3.0, 0.0, 0.0}, near_arrival - 2.0e-9);
    ASSERT_TRUE(before.valid);
    EXPECT_NEAR(before.electric_Vpm.x, physics::constants::coulomb_constant * 1.0e-6 / 9.0,
                1.0e-10);
    EXPECT_NEAR(before.electric_Vpm.y, 0.0, 1.0e-14);
    const auto near_after = delayed.sample({3.0, 0.0, 0.0}, near_arrival + 2.0e-9);
    const auto far_before = delayed.sample({6.0, 0.0, 0.0}, near_arrival + 2.0e-9);
    const auto far_after = delayed.sample({6.0, 0.0, 0.0}, far_arrival + 2.0e-9);
    ASSERT_TRUE(near_after.valid && far_before.valid && far_after.valid);
    EXPECT_LT(near_after.electric_Vpm.y, -0.1);
    EXPECT_NEAR(far_before.electric_Vpm.y, 0.0, 1.0e-14);
    EXPECT_LT(far_after.electric_Vpm.y, -0.01);
    EXPECT_NEAR(far_before.maximum_delay_s - before.maximum_delay_s,
                3.0 / physics::constants::speed_of_light_mps, 1.0e-15);
}

TEST(RetardedField, MissingHistoryInvalidatesSuperpositionAndCountsFailure) {
    core::Scene scene;
    ASSERT_TRUE(
        scene.createBody({.name = "source", .mass_kg = 1.0, .charge_C = 1.0e-6, .radius_m = 0.01}));
    physics::em::ElectromagneticSettings settings;
    settings.retarded.enabled = true;
    physics::em::SourceHistories histories;
    ASSERT_TRUE(histories.configure(settings.retarded, 1.0e-9));
    ASSERT_TRUE(histories.record(scene, 0.0));
    physics::em::ElectromagneticFieldProvider instant(scene, settings);
    physics::em::RetardedFieldProvider delayed(scene, settings, histories, instant);
    const auto sample = delayed.sample({3.0, 0.0, 0.0}, 0.0);
    EXPECT_FALSE(sample.valid);
    EXPECT_TRUE(sample.retarded);
    EXPECT_EQ(sample.electric_Vpm, math::Vec3d{});
    EXPECT_EQ(delayed.diagnostics().history_failures, 1U);
}

TEST(RetardedTime, SuperluminalSegmentsAndNonfiniteQueriesAreRejected) {
    physics::em::HistoryBuffer history(3);
    const double c = physics::constants::speed_of_light_mps;
    ASSERT_TRUE(history.append({0.0, {}, {c, 0.0, 0.0}, {}}));
    ASSERT_TRUE(history.append({2.0, {}, {c, 0.0, 0.0}, {}}));
    EXPECT_EQ(physics::em::solveRetardedTime(history, {c, 0.0, 0.0}, 2.0, {}).status,
              physics::em::RetardedStatus::superluminal_source);
    EXPECT_EQ(
        physics::em::solveRetardedTime(history, {}, std::numeric_limits<double>::quiet_NaN(), {})
            .status,
        physics::em::RetardedStatus::invalid_input);
    EXPECT_THROW(physics::em::HistoryBuffer(std::numeric_limits<std::size_t>::max()),
                 std::invalid_argument);
}

TEST(SourceHistory, AggregateAllocationAndChargedSourceCountAreBounded) {
    core::Scene scene;
    for (std::size_t index = 0; index <= physics::em::maximum_history_sources; ++index)
        ASSERT_TRUE(scene.createBody(
            {.name = "charged", .mass_kg = 1.0, .charge_C = 1.0e-6, .radius_m = 0.01}));
    physics::em::RetardedSettings settings;
    settings.enabled = true;
    EXPECT_FALSE(physics::em::validateRetardedScene(scene, settings, 1.0e-6));
    ASSERT_TRUE(scene.remove(scene.bodies().back().id));
    settings.maximum_observable_distance_m = physics::constants::speed_of_light_mps;
    ASSERT_TRUE(physics::em::historyCapacity(settings, 1.0 / 65000.0));
    EXPECT_FALSE(physics::em::validateRetardedScene(scene, settings, 1.0 / 65000.0));
}
