#pragma once

#include <optional>

#include "aetherion/core/error.hpp"
#include "aetherion/core/simulation_controller.hpp"
#include "aetherion/renderer/camera.hpp"
#include "aetherion/renderer/render_data.hpp"

namespace aetherion::ui {

/// Docking engineering workspace. It emits commands and never mutates scene state directly.
class EngineeringUi final {
  public:
    EngineeringUi() = default;
    ~EngineeringUi();
    EngineeringUi(const EngineeringUi&) = delete;
    EngineeringUi& operator=(const EngineeringUi&) = delete;

    [[nodiscard]] core::Status initialize(void* glfw_window);
    [[nodiscard]] core::Status draw(core::SimulationController& controller,
                                    renderer::Camera& camera,
                                    renderer::RenderSettings& render_settings);
    [[nodiscard]] std::optional<core::EntityId> selectedEntity() const noexcept {
        return selected_;
    }

  private:
    void drawDockSpace();
    void drawHierarchy(core::SimulationController& controller);
    void drawInspector(core::SimulationController& controller, renderer::Camera& camera);
    void drawSimulationControls(core::SimulationController& controller,
                                renderer::RenderSettings& render_settings);
    void drawDiagnostics(const core::SimulationController& controller);
    void shutdown() noexcept;

    std::optional<core::EntityId> selected_;
    bool initialized_{};
    bool dock_layout_initialized_{};
    std::size_t new_body_counter_{1};
};

} // namespace aetherion::ui
