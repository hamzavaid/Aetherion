#include "aetherion/renderer/input_routing.hpp"

#include <gtest/gtest.h>

using aetherion::renderer::InputCapture;
using aetherion::renderer::routesKeyboardToScene;
using aetherion::renderer::routesMouseToScene;

TEST(InputRouting, UiMouseCapturePreventsSceneCameraInput) {
    EXPECT_TRUE(routesMouseToScene({}));
    EXPECT_FALSE(routesMouseToScene(InputCapture{.mouse = true}));
}

TEST(InputRouting, MouseAndKeyboardCaptureAreIndependent) {
    const InputCapture capture{.mouse = true, .keyboard = false};
    EXPECT_FALSE(routesMouseToScene(capture));
    EXPECT_TRUE(routesKeyboardToScene(capture));
    EXPECT_FALSE(routesKeyboardToScene(InputCapture{.keyboard = true}));
}

TEST(InputRouting, SceneNudgeActionsMapToWorldAxesAndRejectInvalidStep) {
    using aetherion::renderer::SceneActionKind;
    using aetherion::renderer::sceneNudgeDelta;
    EXPECT_EQ(sceneNudgeDelta(SceneActionKind::move_right, 2.0),
              (aetherion::math::Vec3d{2.0, 0.0, 0.0}));
    EXPECT_EQ(sceneNudgeDelta(SceneActionKind::move_up, 2.0),
              (aetherion::math::Vec3d{0.0, 2.0, 0.0}));
    EXPECT_EQ(sceneNudgeDelta(SceneActionKind::move_forward, 2.0),
              (aetherion::math::Vec3d{0.0, 0.0, -2.0}));
    EXPECT_FALSE(sceneNudgeDelta(SceneActionKind::undo, 2.0));
    EXPECT_FALSE(sceneNudgeDelta(SceneActionKind::move_right, 0.0));
}
