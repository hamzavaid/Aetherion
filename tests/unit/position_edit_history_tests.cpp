#include "aetherion/core/position_edit_history.hpp"

#include <gtest/gtest.h>

#include <limits>

using namespace aetherion;

TEST(PositionEditHistory, RepeatedNudgesUndoOneStepAtATimeAndResetToSelectionOrigin) {
    core::PositionEditHistory edits(3);
    ASSERT_TRUE(edits.select({core::PositionTargetKind::body, 17}, {10.0, 0.0, 0.0}));
    ASSERT_TRUE(edits.nudge({2.0, 0.0, 0.0}));
    ASSERT_TRUE(edits.nudge({2.0, 0.0, 0.0}));
    EXPECT_EQ(edits.current(), (math::Vec3d{14.0, 0.0, 0.0}));
    ASSERT_TRUE(edits.undo());
    EXPECT_EQ(edits.current(), (math::Vec3d{12.0, 0.0, 0.0}));
    ASSERT_TRUE(edits.reset());
    EXPECT_EQ(edits.current(), (math::Vec3d{10.0, 0.0, 0.0}));
    ASSERT_TRUE(edits.undo());
    EXPECT_EQ(edits.current(), (math::Vec3d{12.0, 0.0, 0.0}));
}

TEST(PositionEditHistory, TargetSwitchAndRebaseDoNotUndoAnotherObject) {
    core::PositionEditHistory edits;
    ASSERT_TRUE(edits.select({core::PositionTargetKind::body, 2}, {1.0, 2.0, 3.0}));
    ASSERT_TRUE(edits.nudge({1.0, 0.0, 0.0}));
    ASSERT_TRUE(edits.select({core::PositionTargetKind::probe, 0}, {5.0, 0.0, 0.0}));
    EXPECT_FALSE(edits.undo());
    ASSERT_TRUE(edits.nudge({0.0, 1.0, 0.0}));
    edits.rebase({6.0, 0.0, 0.0});
    EXPECT_FALSE(edits.undo());
    EXPECT_EQ(edits.current(), (math::Vec3d{6.0, 0.0, 0.0}));
    EXPECT_FALSE(edits.nudge({std::numeric_limits<double>::infinity(), 0.0, 0.0}));
}
