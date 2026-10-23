/** @file
 * Dockable engineering controls; physical edits use commands and presentation edits are validated
 * locally.
 */
#pragma once

#include <optional>
#include <string>
#include <utility>

#include "aetherion/core/error.hpp"
#include "aetherion/core/position_edit_history.hpp"
#include "aetherion/core/scientific_analysis.hpp"
#include "aetherion/core/simulation_controller.hpp"
#include "aetherion/physics/integrator_comparison.hpp"
#include "aetherion/renderer/camera.hpp"
#include "aetherion/renderer/camera_tracking.hpp"
#include "aetherion/renderer/input_routing.hpp"
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
    [[nodiscard]] std::optional<std::size_t> selectedProbe() const noexcept {
        return selected_probe_;
    }
    void selectEntity(std::optional<core::EntityId> entity) noexcept {
        if (selected_ != entity)
            position_edits_.clear();
        selected_ = entity;
        if (entity)
            selected_probe_.reset();
    }
    void selectProbe(std::optional<std::size_t> index) noexcept {
        if (selected_probe_ != index)
            position_edits_.clear();
        selected_probe_ = index;
        if (index) {
            selected_.reset();
            camera_tracker_.stop();
        }
    }
    /// Applies one viewport shortcut. Body positions are queued; probes are display state.
    void handleSceneAction(core::SimulationController& controller,
                           renderer::RenderSettings& settings, renderer::Camera& camera,
                           renderer::SceneActionKind action);
    /// Advances a checked body-reference camera after simulation updates and before rendering.
    void updateCameraTracking(const core::Scene& scene, renderer::Camera& camera) {
        static_cast<void>(camera_tracker_.update(scene, camera));
    }
    [[nodiscard]] renderer::InputCapture inputCapture() const noexcept { return input_capture_; }
    /// Displays recoverable simulation/render failures while keeping controls available to repair
    /// them.
    void reportRuntimeError(std::string message) { runtime_error_ = std::move(message); }

  private:
    /// Builds the initial dock arrangement once; later frames preserve the user's layout.
    void drawDockSpace();
    /// Selects stable body IDs or probe indices without mutating physical body properties.
    void drawHierarchy(core::SimulationController& controller,
                       const renderer::RenderSettings& settings, renderer::Camera& camera);
    /// Emits validated body patches and applies display-only camera focus/follow controls.
    void drawInspector(core::SimulationController& controller, renderer::Camera& camera,
                       const renderer::RenderSettings& render_settings);
    /// Queues solver edits; validates presentation edits before committing them for the next frame.
    void drawSimulationControls(core::SimulationController& controller, renderer::Camera& camera,
                                renderer::RenderSettings& render_settings);
    /// Restores retained physical checkpoints; transient plot/movement histories are cleared.
    void drawSaveHistory(core::SimulationController& controller);
    /// Displays accepted-state invariants, field failures, gyro resolution, and command rejection.
    void drawDiagnostics(const core::SimulationController& controller);
    /// Converts finite SI observations to float plots only; no integration or field calculation.
    void drawPlots(const core::SimulationController& controller);
    /// Edits fixed world-space probe definitions and samples the shared field provider read-only.
    void drawProbes(core::SimulationController& controller, renderer::RenderSettings& settings,
                    renderer::Camera& camera);
    /// Bounded per-selection nudge/undo controls; physical movement is routed through commands.
    void drawMovementControls(core::SimulationController& controller,
                              renderer::RenderSettings& settings, renderer::Camera& camera);
    /// Releases ImGui backends/context while the renderer's GLFW/OpenGL context still exists.
    void shutdown() noexcept;

    std::optional<core::EntityId> selected_;
    std::optional<std::size_t> selected_probe_;
    renderer::CameraTracker camera_tracker_;
    core::PositionEditHistory position_edits_;
    double move_step_m_{1.0};
    bool initialized_{};
    bool dock_layout_initialized_{};
    std::size_t new_body_counter_{1};
    int plot_metric_{};
    int plot_probe_{};
    core::ScientificHistory scientific_history_;
    std::optional<physics::IntegratorComparisonReport> comparison_;
    renderer::InputCapture input_capture_;
    std::string scene_file_status_;
    std::string runtime_error_;
};

} // namespace aetherion::ui
