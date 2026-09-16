#pragma once

#include <memory>
#include <string_view>

#include "aetherion/core/error.hpp"
#include "aetherion/core/scene.hpp"
#include "aetherion/renderer/camera.hpp"
#include "aetherion/renderer/render_data.hpp"

namespace aetherion::renderer {

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
    void pollEvents();
    [[nodiscard]] bool shouldClose() const noexcept;
    void requestClose() noexcept;

  private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace aetherion::renderer
