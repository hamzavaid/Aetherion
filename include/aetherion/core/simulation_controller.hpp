#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "aetherion/core/command_queue.hpp"
#include "aetherion/core/simulation.hpp"
#include "aetherion/core/simulation_clock.hpp"

namespace aetherion::core {

using SimulationControllerConfig = RuntimeSettings;
using CheckpointId = std::uint64_t;

struct SimulationCheckpoint {
    CheckpointId id{};
    Scene scene;
    RuntimeSettings settings;
    double simulation_time_s{};
};

/// UI-facing runtime facade. Scene mutation occurs only when its command queue is drained.
class SimulationController final {
  public:
    explicit SimulationController(Scene initial_scene, SimulationControllerConfig config = {});
    [[nodiscard]] Status update(double real_delta_s);
    [[nodiscard]] Status singleStep();
    /// Saves the current scene and runtime state after applying pending commands.
    [[nodiscard]] CheckpointId saveCheckpoint();
    /// Restores a historical checkpoint and pauses execution.
    [[nodiscard]] Status restoreCheckpoint(CheckpointId id);
    /// Replaces the active/startup scene for an explicit preset or serialized-scene load.
    [[nodiscard]] Status loadState(Scene scene, RuntimeSettings settings);
    void reset();
    void setPlaying(bool playing) noexcept { playing_ = playing; }

    [[nodiscard]] bool isPlaying() const noexcept { return playing_; }
    [[nodiscard]] Scene& scene() noexcept { return scene_; }
    [[nodiscard]] const Scene& scene() const noexcept { return scene_; }
    [[nodiscard]] CommandQueue& commands() noexcept { return commands_; }
    [[nodiscard]] const CommandQueue& commands() const noexcept { return commands_; }
    [[nodiscard]] const RuntimeSettings& settings() const noexcept { return settings_; }
    [[nodiscard]] const std::vector<SimulationCheckpoint>& checkpoints() const noexcept {
        return checkpoints_;
    }
    [[nodiscard]] double simulationTimeSeconds() const noexcept {
        return simulation_.timeSeconds();
    }
    [[nodiscard]] const TelemetryRecorder& telemetry() const noexcept {
        return simulation_.telemetry();
    }
    [[nodiscard]] const physics::fields::IFieldProvider& fieldProvider() const noexcept {
        return simulation_.fieldProvider();
    }

  private:
    void applyCommandsAtBoundary();
    void synchronizePhysicsSettings();
    void synchronizeClockSettings();
    void restoreState(const Scene& scene, const RuntimeSettings& settings, double time_s);

    Scene scene_;
    Scene initial_scene_;
    RuntimeSettings initial_settings_;
    RuntimeSettings settings_;
    Simulation simulation_;
    SimulationClock clock_;
    CommandQueue commands_;
    std::vector<SimulationCheckpoint> checkpoints_;
    CheckpointId next_checkpoint_id_{1};
    bool playing_{};
};

} // namespace aetherion::core
