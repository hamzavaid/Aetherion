#pragma once

#include <memory>
#include <optional>
#include <string_view>

#include "aetherion/core/error.hpp"
#include "aetherion/core/scene.hpp"
#include "aetherion/renderer/camera.hpp"
#include "aetherion/renderer/input_routing.hpp"
#include "aetherion/renderer/render_data.hpp"

namespace aetherion::renderer {

struct ViewportClick {
    double x_ndc{};
    double y_ndc{};
    double aspect_ratio{1.0};
};

/// GLFW/OpenGL 4.1 renderer with instanced spheres, an engineering grid, and camera controls.
class OpenGlRenderer final {
  public:
    OpenGlRenderer();
    ~OpenGlRenderer();
    OpenGlRenderer(OpenGlRenderer&&) noexcept;
    OpenGlRenderer& operator=(OpenGlRenderer&&) noexcept;
    OpenGlRenderer(const OpenGlRenderer&) = delete;
    OpenGlRenderer& operator=(const OpenGlRenderer&) = delete;

    [[nodiscard]] core::Status initialize(int width, int height, std::string_view title,
                                          bool visible = true);
    [[nodiscard]] core::Status render(const core::Scene& scene, Camera& camera,
                                      const RenderSettings& settings);
    void present();
    void pollEvents();
    /// Sets UI ownership before event polling; captured input cannot manipulate the scene camera.
    void setInputCapture(const InputCapture& capture) noexcept;
    /// Consumes a left-click made in the unobstructed scene viewport, if one is pending.
    [[nodiscard]] std::optional<ViewportClick> takeViewportClick() noexcept;
    [[nodiscard]] bool shouldClose() const noexcept;
    void requestClose() noexcept;
    [[nodiscard]] void* nativeWindowHandle() noexcept;

  private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace aetherion::renderer
