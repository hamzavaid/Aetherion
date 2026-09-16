#include "aetherion/renderer/trail_history.hpp"

#include <gtest/gtest.h>

using aetherion::core::Body;
using aetherion::core::Scene;
using aetherion::renderer::TrailHistory;

TEST(TrailHistory, RetainsOnlyRequestedPhysicalTimeWindow) {
    Scene scene;
    const auto id = scene.createBody(Body{.name = "body", .mass_kg = 1.0, .radius_m = 1.0});
    ASSERT_TRUE(id);
    TrailHistory history(16);
    history.sample(scene, 0.0, 10.0);
    scene.bodies()[0].state.position_m.x = 5.0;
    history.sample(scene, 5.0, 10.0);
    scene.bodies()[0].state.position_m.x = 12.0;
    history.sample(scene, 12.0, 10.0);
    const auto* trail = history.find(id.value());
    ASSERT_NE(trail, nullptr);
    ASSERT_EQ(trail->points.size(), 2U);
    EXPECT_DOUBLE_EQ(trail->points.front().time_s, 5.0);
    EXPECT_DOUBLE_EQ(trail->points.back().position_world_m.x, 12.0);
}

TEST(TrailHistory, ReplacesSameTimeSampleAndClearsOnTimeRewind) {
    Scene scene;
    const auto id = scene.createBody(Body{.name = "body", .mass_kg = 1.0, .radius_m = 1.0});
    ASSERT_TRUE(id);
    TrailHistory history(16);
    history.sample(scene, 3.0, 10.0);
    scene.bodies()[0].state.position_m.x = 2.0;
    history.sample(scene, 3.0, 10.0);
    ASSERT_EQ(history.find(id.value())->points.size(), 1U);
    EXPECT_DOUBLE_EQ(history.find(id.value())->points[0].position_world_m.x, 2.0);
    history.sample(scene, 1.0, 10.0);
    ASSERT_EQ(history.find(id.value())->points.size(), 1U);
    EXPECT_DOUBLE_EQ(history.find(id.value())->points[0].time_s, 1.0);
}
