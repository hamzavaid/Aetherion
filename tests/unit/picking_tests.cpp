#include "aetherion/renderer/picking.hpp"

#include <gtest/gtest.h>

using aetherion::core::Body;
using aetherion::core::Scene;
using aetherion::renderer::pickBody;
using aetherion::renderer::Ray;

TEST(Picking, SelectsNearestPositiveRaySphereIntersection) {
    Scene scene;
    const auto near_id = scene.createBody(Body{
        .name = "near", .mass_kg = 1.0, .radius_m = 2.0, .state = {.position_m = {0, 0, -10}}});
    ASSERT_TRUE(near_id);
    ASSERT_TRUE(scene.createBody(Body{
        .name = "far", .mass_kg = 1.0, .radius_m = 3.0, .state = {.position_m = {0, 0, -30}}}));
    const auto hit = pickBody(scene, Ray{.origin = {}, .direction = {0, 0, -1}});
    ASSERT_TRUE(hit.has_value());
    EXPECT_EQ(hit->id, near_id.value());
    EXPECT_NEAR(hit->distance_m, 8.0, 1.0e-12);
}

TEST(Picking, ReturnsNoHitForRayPointingAway) {
    Scene scene;
    ASSERT_TRUE(scene.createBody(Body{
        .name = "body", .mass_kg = 1.0, .radius_m = 1.0, .state = {.position_m = {0, 0, -10}}}));
    EXPECT_FALSE(pickBody(scene, Ray{.origin = {}, .direction = {0, 0, 1}}));
}

TEST(Picking, UsesVisualizationRadiusForDirectSceneSelection) {
    Scene scene;
    const auto id = scene.createBody(Body{
        .name = "visually enlarged",
        .mass_kg = 1.0,
        .radius_m = 0.1,
        .state = {.position_m = {1.5, 0.0, -10.0}},
    });
    ASSERT_TRUE(id);
    const auto ray = Ray{.origin = {}, .direction = {0.0, 0.0, -1.0}};
    EXPECT_FALSE(pickBody(scene, ray));
    const auto hit = pickBody(scene, ray, {.radius_scale = 1.0, .minimum_radius_m = 2.0});
    ASSERT_TRUE(hit);
    EXPECT_EQ(hit->id, id.value());
}

TEST(Picking, SelectsVisibleProbeMarkerAndSkipsHiddenProbes) {
    std::vector<aetherion::core::FieldProbe> probes = {
        {.name = "near", .position_m = {0.0, 0.0, -10.0}},
        {.name = "far", .position_m = {0.0, 0.0, -20.0}}};
    const auto ray = Ray{.origin = {}, .direction = {0.0, 0.0, -1.0}};
    const auto first = aetherion::renderer::pickProbe(probes, ray, 0.5);
    ASSERT_TRUE(first);
    EXPECT_EQ(first->index, 0U);
    probes[0].visible = false;
    const auto second = aetherion::renderer::pickProbe(probes, ray, 0.5);
    ASSERT_TRUE(second);
    EXPECT_EQ(second->index, 1U);
    EXPECT_FALSE(aetherion::renderer::pickProbe(probes, ray, 0.0));
}
