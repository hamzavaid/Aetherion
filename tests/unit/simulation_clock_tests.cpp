#include "aetherion/core/simulation_clock.hpp"

#include <gtest/gtest.h>

using aetherion::core::SimulationClock;

TEST(SimulationClock, AdvancesOnlyWholeFixedStepsAndRetainsRemainder) {
    SimulationClock clock({.physics_dt_s = 0.1, .time_scale = 2.0, .max_substeps = 8});
    int steps = 0;
    EXPECT_EQ(clock.advance(0.12,
                            [&](double dt) {
                                EXPECT_DOUBLE_EQ(dt, 0.1);
                                ++steps;
                            }),
              2U);
    EXPECT_EQ(steps, 2);
    EXPECT_NEAR(clock.interpolationAlpha(), 0.4, 1.0e-14);
    EXPECT_DOUBLE_EQ(clock.simulationTimeSeconds(), 0.2);
}

TEST(SimulationClock, CapsCatchupAndReportsDroppedSimulationTime) {
    SimulationClock clock({.physics_dt_s = 0.01, .time_scale = 1.0, .max_substeps = 3});
    EXPECT_EQ(clock.advance(1.0, [](double) {}), 3U);
    EXPECT_GT(clock.droppedTimeSeconds(), 0.96);
    EXPECT_LT(clock.interpolationAlpha(), 1.0);
}
