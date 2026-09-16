#pragma once

#include <cmath>
#include <cstddef>
#include <stdexcept>

namespace aetherion::core {

struct SimulationClockConfig {
    double physics_dt_s{1.0 / 120.0};
    double time_scale{1.0};
    std::size_t max_substeps{8};
};

/// Fixed-timestep accumulator decoupled from presentation frame duration.
class SimulationClock final {
  public:
    explicit SimulationClock(SimulationClockConfig config);

    template <typename StepFunction>
    std::size_t advance(double real_delta_s, StepFunction&& step_function) {
        if (!std::isfinite(real_delta_s) || real_delta_s < 0.0) {
            throw std::invalid_argument("real frame delta must be finite and non-negative");
        }
        accumulator_s_ += real_delta_s * config_.time_scale;
        std::size_t substeps = 0;
        while (accumulator_s_ >= config_.physics_dt_s && substeps < config_.max_substeps) {
            step_function(config_.physics_dt_s);
            accumulator_s_ -= config_.physics_dt_s;
            simulation_time_s_ += config_.physics_dt_s;
            ++substeps;
        }
        if (substeps == config_.max_substeps && accumulator_s_ >= config_.physics_dt_s) {
            dropped_time_s_ += accumulator_s_;
            accumulator_s_ = 0.0;
        }
        return substeps;
    }

    [[nodiscard]] double interpolationAlpha() const noexcept {
        return accumulator_s_ / config_.physics_dt_s;
    }
    [[nodiscard]] double simulationTimeSeconds() const noexcept { return simulation_time_s_; }
    [[nodiscard]] double droppedTimeSeconds() const noexcept { return dropped_time_s_; }
    void reset() noexcept;

  private:
    SimulationClockConfig config_;
    double accumulator_s_{};
    double simulation_time_s_{};
    double dropped_time_s_{};
};

} // namespace aetherion::core
