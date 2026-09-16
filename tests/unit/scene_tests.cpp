#include "aetherion/core/scene.hpp"

#include <gtest/gtest.h>

#include <limits>

using aetherion::core::Body;
using aetherion::core::Scene;

TEST(Scene, AllocatesStableMonotonicIdsAndSupportsLifecycle) {
    Scene scene;
    auto first = scene.createBody(Body{.name = "first", .mass_kg = 1.0, .radius_m = 1.0});
    auto second = scene.createBody(Body{.name = "second", .mass_kg = 2.0, .radius_m = 1.0});
    ASSERT_TRUE(first);
    ASSERT_TRUE(second);
    EXPECT_LT(first.value(), second.value());
    ASSERT_NE(scene.find(first.value()), nullptr);
    EXPECT_TRUE(scene.remove(first.value()));
    EXPECT_EQ(scene.find(first.value()), nullptr);
    EXPECT_EQ(scene.size(), 1U);
}

TEST(Scene, RejectsInvalidPhysicalStateWithoutMutation) {
    Scene scene;
    Body invalid{.name = "invalid", .mass_kg = -1.0, .radius_m = 1.0};
    EXPECT_FALSE(scene.createBody(invalid));
    invalid.mass_kg = 1.0;
    invalid.state.position_m.x = std::numeric_limits<double>::quiet_NaN();
    EXPECT_FALSE(scene.createBody(invalid));
    EXPECT_EQ(scene.size(), 0U);
}
