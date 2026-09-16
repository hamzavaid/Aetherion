#pragma once

#include "aetherion/core/error.hpp"
#include "aetherion/core/scene.hpp"
#include "aetherion/core/telemetry.hpp"
#include "aetherion/physics/gravity/gravity_solver.hpp"

namespace aetherion::core {

struct SimulationConfig {
    double physics_dt_s{1.0 / 120.0};
};

/// Headless mechanics composition: gravity, integration, simulation time, and telemetry.
class Simulation final {
  public:
    Simulation(Scene& scene, SimulationConfig config = {});
    [[nodiscard]] Status step();
    [[nodiscard]] double timeSeconds() const noexcept { return time_s_; }
    [[nodiscard]] TelemetryRecorder& telemetry() noexcept { return telemetry_; }

  private:
    Scene& scene_;
    SimulationConfig config_;
    physics::GravitySolver gravity_;
    TelemetryRecorder telemetry_;
    double time_s_{};
};

} // namespace aetherion::core
