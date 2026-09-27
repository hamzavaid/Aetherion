#pragma once

#include <cmath>
#include <optional>

#include "aetherion/math/vec3d.hpp"

namespace aetherion::renderer {

/// Input ownership reported by the UI for the current frame.
struct InputCapture {
    bool mouse{};
    bool keyboard{};
};

enum class SceneActionKind {
    move_left,
    move_right,
    move_forward,
    move_backward,
    move_up,
    move_down,
    undo,
    reset_position
};

/// Returns a right-handed, inertial-world displacement in meters for one key press/repeat.
/// X is left/right, Y is vertical, and forward is -Z; invalid steps and non-movement actions
/// have no displacement. This never manipulates the camera or simulation directly.
[[nodiscard]] inline std::optional<math::Vec3d> sceneNudgeDelta(SceneActionKind action,
                                                                double step_m) noexcept {
    if (!std::isfinite(step_m) || step_m <= 0.0)
        return std::nullopt;
    switch (action) {
    case SceneActionKind::move_left:
        return math::Vec3d{-step_m, 0.0, 0.0};
    case SceneActionKind::move_right:
        return math::Vec3d{step_m, 0.0, 0.0};
    case SceneActionKind::move_forward:
        return math::Vec3d{0.0, 0.0, -step_m};
    case SceneActionKind::move_backward:
        return math::Vec3d{0.0, 0.0, step_m};
    case SceneActionKind::move_up:
        return math::Vec3d{0.0, step_m, 0.0};
    case SceneActionKind::move_down:
        return math::Vec3d{0.0, -step_m, 0.0};
    case SceneActionKind::undo:
    case SceneActionKind::reset_position:
        return std::nullopt;
    }
    return std::nullopt;
}

/// Scene mouse controls are enabled only when no UI widget owns pointer input.
[[nodiscard]] constexpr bool routesMouseToScene(const InputCapture& capture) noexcept {
    return !capture.mouse;
}

/// Scene keyboard shortcuts are enabled only when no UI widget owns keyboard input.
[[nodiscard]] constexpr bool routesKeyboardToScene(const InputCapture& capture) noexcept {
    return !capture.keyboard;
}

} // namespace aetherion::renderer
