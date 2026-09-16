#include "aetherion/renderer/render_data.hpp"

#include <gtest/gtest.h>

using aetherion::core::Body;
using aetherion::core::Scene;
using aetherion::renderer::buildBodyInstances;
using aetherion::renderer::RenderSettings;
using aetherion::renderer::toCameraRelative;

TEST(RenderData, CameraRelativeConversionPreservesLocalDetailAtAstronomicalOrigin) {
    const auto relative =
        toCameraRelative({149'597'870'700.25, -40.5, 3.0}, {149'597'870'700.0, -40.0, 1.0}, 2.0);
    EXPECT_FLOAT_EQ(relative.x, 0.5F);
    EXPECT_FLOAT_EQ(relative.y, -1.0F);
    EXPECT_FLOAT_EQ(relative.z, 4.0F);
}

TEST(RenderData, InstancesSupportPhysicalScaleAndMinimumApparentRadius) {
    Scene scene;
    ASSERT_TRUE(scene.createBody(Body{.name = "astronomical",
                                      .mass_kg = 1.0,
                                      .radius_m = 1.0,
                                      .state = {.position_m = {1.5e11 + 1000.0, 0.0, 0.0}}}));
    const auto instances = buildBodyInstances(
        scene, {1.5e11, 0.0, 0.0},
        RenderSettings{.meters_to_render_units = 1.0e-3, .minimum_apparent_radius = 2.0F});
    ASSERT_EQ(instances.size(), 1U);
    EXPECT_FLOAT_EQ(instances[0].position.x, 1.0F);
    EXPECT_FLOAT_EQ(instances[0].radius, 2.0F);
}
