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
