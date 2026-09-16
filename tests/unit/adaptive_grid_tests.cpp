#include "aetherion/renderer/adaptive_grid.hpp"

#include <gtest/gtest.h>

TEST(AdaptiveGrid, ExpandsAndUsesCoarserSpacingAsCameraZoomsOut) {
    const auto near_grid = aetherion::renderer::calculateAdaptiveGrid(10.0, 1.0);
    const auto far_grid = aetherion::renderer::calculateAdaptiveGrid(10'000.0, 1.0);
    EXPECT_GT(far_grid.spacing_render_units, near_grid.spacing_render_units);
    EXPECT_GT(far_grid.half_extent_render_units, near_grid.half_extent_render_units);
    EXPECT_GE(near_grid.half_extent_render_units, 30.0);
    EXPECT_GE(far_grid.half_extent_render_units, 30'000.0);
}

TEST(AdaptiveGrid, UsesOneTwoFiveEngineeringSpacingAndBoundedLineCount) {
    const auto grid = aetherion::renderer::calculateAdaptiveGrid(12'300.0, 0.01);
    EXPECT_DOUBLE_EQ(grid.spacing_render_units, 20.0);
    EXPECT_LE(grid.subdivisions_each_direction, 64U);
    EXPECT_GE(grid.subdivisions_each_direction, 16U);
}
