#pragma once

#include <cstddef>

#include "aetherion/core/command_queue.hpp"
#include "aetherion/core/simulation.hpp"
#include "aetherion/core/simulation_clock.hpp"

namespace aetherion::core {

using SimulationControllerConfig = RuntimeSettings;

/// UI-facing runtime facade. Scene mutation occurs only when its command queue is drained.
class SimulationController final {
  public:
    explicit SimulationController(Scene initial_scene, SimulationControllerConfig config = {});
    [[nodiscard]] Status update(double real_delta_s);
    [[nodiscard]] Status singleStep();
    void reset();
    void setPlaying(bool playing) noexcept { playing_ = playing; }

    [[nodiscard]] bool isPlaying() const noexcept { return playing_; }
    [[nodiscard]] Scene& scene() noexcept { return scene_; }
    [[nodiscard]] const Scene& scene() const noexcept { return scene_; }
    [[nodiscard]] CommandQueue& commands() noexcept { return commands_; }
    [[nodiscard]] const CommandQueue& commands() const noexcept { return commands_; }
    [[nodiscard]] const RuntimeSettings& settings() const noexcept { return settings_; }
    [[nodiscard]] double simulationTimeSeconds() const noexcept {
        return simulation_.timeSeconds();
    }
    [[nodiscard]] const TelemetryRecorder& telemetry() const noexcept {
        return simulation_.telemetry();
    }

  private:
    void applyCommandsAtBoundary();
    void synchronizePhysicsSettings();
    void synchronizeClockSettings();

    Scene scene_;
    Scene initial_scene_;
    RuntimeSettings settings_;
    Simulation simulation_;
    SimulationClock clock_;
    CommandQueue commands_;
    bool playing_{};
};

} // namespace aetherion::core
