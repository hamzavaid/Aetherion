#include "aetherion/renderer/camera.hpp"
#include "aetherion/renderer/camera_tracking.hpp"

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

TEST(Camera, DefaultViewStartsAboveGridAndLooksDownTowardTarget) {
    Camera camera;
    EXPECT_GT(camera.positionWorld().y, camera.targetWorld().y);
    const auto ray = camera.rayFromNdc(0.0, 0.0, 1.0);
    EXPECT_LT(ray.direction.y, 0.0);
}

TEST(Camera, ProjectionRetainsScaleInvarianceForMeterAndAstronomicalRenderUnits) {
    Camera camera;
    camera.focus({}, 0.08);
    const auto meter_scale = camera.viewProjection(16.0 / 9.0, 1.0);
    const auto astronomical_scale = camera.viewProjection(16.0 / 9.0, 1.0e-9);
    EXPECT_NEAR(astronomical_scale[10], meter_scale[10], 1.0e-6F);
    EXPECT_NEAR(astronomical_scale[14], meter_scale[14] * 1.0e-9F, 1.0e-15F);
}

TEST(Camera, TwoDimensionalModeUsesLockedTopDownOrthographicProjection) {
    Camera camera;
    camera.focus({2.0, 0.0, -3.0}, 4.0);
    camera.setTwoDimensional(true);
    ASSERT_TRUE(camera.isTwoDimensional());
    const auto before_orbit = camera.positionWorld();
    camera.orbit(1.0, -0.5);
    EXPECT_EQ(camera.positionWorld(), before_orbit);
    const auto ray = camera.rayFromNdc(0.0, 0.0, 1.0);
    EXPECT_NEAR(ray.direction.x, 0.0, 1.0e-12);
    EXPECT_LT(ray.direction.y, -0.999999);
    const auto projection = camera.viewProjection(1.0, 1.0);
    EXPECT_FLOAT_EQ(projection[11], 0.0F);
    EXPECT_FLOAT_EQ(projection[15], 1.0F);
    camera.setTwoDimensional(false);
    EXPECT_FALSE(camera.isTwoDimensional());
}

TEST(Camera, UpdatingTargetPreservesZoomAndOrbitOffset) {
    Camera camera;
    camera.focus({10.0, 20.0, 30.0}, 4.0);
    camera.orbit(0.4, -0.2);
    camera.zoom(3.0);
    const double distance = camera.distanceMeters();
    const auto offset = camera.positionWorld() - camera.targetWorld();

    camera.setTargetWorld({100.0, -50.0, 7.0});

    EXPECT_EQ(camera.targetWorld(), (Vec3d{100.0, -50.0, 7.0}));
    EXPECT_DOUBLE_EQ(camera.distanceMeters(), distance);
    const auto tracked_offset = camera.positionWorld() - camera.targetWorld();
    EXPECT_NEAR(tracked_offset.x, offset.x, 1.0e-12);
    EXPECT_NEAR(tracked_offset.y, offset.y, 1.0e-12);
    EXPECT_NEAR(tracked_offset.z, offset.z, 1.0e-12);
}

TEST(CameraTracker, FollowsEntityMotionWithoutChangingZoomAndCanDetach) {
    aetherion::core::Scene scene;
    const auto id = scene.createBody({.name = "tracked",
                                      .mass_kg = 1.0,
                                      .radius_m = 1.0,
                                      .state = {.position_m = {1.0, 2.0, 3.0}}});
    ASSERT_TRUE(id);
    Camera camera;
    camera.focus({}, 5.0);
    camera.zoom(2.0);
    const double distance = camera.distanceMeters();
    aetherion::renderer::CameraTracker tracker;
    tracker.follow(id.value());

    EXPECT_TRUE(tracker.update(scene, camera));
    EXPECT_EQ(camera.targetWorld(), (Vec3d{1.0, 2.0, 3.0}));
    EXPECT_DOUBLE_EQ(camera.distanceMeters(), distance);
    scene.bodies().front().state.position_m = {9.0, 8.0, 7.0};
    EXPECT_TRUE(tracker.update(scene, camera));
    EXPECT_EQ(camera.targetWorld(), (Vec3d{9.0, 8.0, 7.0}));
    EXPECT_DOUBLE_EQ(camera.distanceMeters(), distance);

    tracker.stop();
    scene.bodies().front().state.position_m = {20.0, 0.0, 0.0};
    EXPECT_FALSE(tracker.update(scene, camera));
    EXPECT_EQ(camera.targetWorld(), (Vec3d{9.0, 8.0, 7.0}));
}

TEST(CameraTracker, ClearsReferenceWhenTrackedEntityIsRemoved) {
    aetherion::core::Scene scene;
    const auto id = scene.createBody({.name = "tracked", .mass_kg = 1.0, .radius_m = 1.0});
    ASSERT_TRUE(id);
    Camera camera;
    aetherion::renderer::CameraTracker tracker;
    tracker.follow(id.value());
    ASSERT_TRUE(scene.remove(id.value()));
    EXPECT_FALSE(tracker.update(scene, camera));
    EXPECT_FALSE(tracker.followedEntity());
}
