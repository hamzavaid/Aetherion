#include "aetherion/renderer/camera.hpp"

#include <gtest/gtest.h>

using aetherion::math::Vec3d;
using aetherion::renderer::Camera;

TEST(Camera, OrbitPanZoomFocusAndResetMaintainFiniteGeometry) {
    Camera camera;
    const auto initial_position = camera.positionWorld();
    camera.orbit(0.5, 0.25);
    EXPECT_NE(camera.positionWorld(), initial_position);
    camera.pan(12.0, -4.0);
    camera.zoom(-1000.0);
    EXPECT_GT(camera.distanceMeters(), 0.0);
    EXPECT_TRUE(camera.positionWorld().isFinite());
    camera.focus({1.5e11, -2.0e10, 3.0e9}, 7.0e8);
    EXPECT_EQ(camera.targetWorld(), (Vec3d{1.5e11, -2.0e10, 3.0e9}));
    camera.reset();
    EXPECT_EQ(camera.positionWorld(), initial_position);
}

TEST(Camera, CenterViewportRayPointsAtOrbitTarget) {
    Camera camera;
    camera.focus({10.0, 20.0, -30.0}, 4.0);
    const auto ray = camera.rayFromNdc(0.0, 0.0, 16.0 / 9.0);
    const auto expected = (camera.targetWorld() - camera.positionWorld()).normalized();
    EXPECT_NEAR(aetherion::math::dot(ray.direction, expected), 1.0, 1.0e-12);
    EXPECT_EQ(ray.origin, camera.positionWorld());
}
