#pragma once

namespace aetherion::renderer {

/// Input ownership reported by the UI for the current frame.
struct InputCapture {
    bool mouse{};
    bool keyboard{};
};

/// Scene mouse controls are enabled only when no UI widget owns pointer input.
[[nodiscard]] constexpr bool routesMouseToScene(const InputCapture& capture) noexcept {
    return !capture.mouse;
}

/// Scene keyboard shortcuts are enabled only when no UI widget owns keyboard input.
[[nodiscard]] constexpr bool routesKeyboardToScene(const InputCapture& capture) noexcept {
    return !capture.keyboard;
}

} // namespace aetherion::renderer
