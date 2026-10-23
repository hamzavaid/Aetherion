/** @file
 * Fixed-step time accumulation with bounded catch-up, rejected-step retry, and finite-time checks.
 */
#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <type_traits>

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

    /// Accumulates real seconds scaled by time_scale, accepting only whole physics_dt_s steps.
    /// Void callbacks accept; bool/Status callbacks can reject without consuming elapsed time.
    /// Catch-up is capped by max_substeps, with excess reported as dropped simulation seconds.
    /// Throws before overflowing scaled/accumulated time; accepted earlier steps remain accepted.
    template <typename StepFunction>
    std::size_t advance(double real_delta_s, StepFunction&& step_function) {
        if (!std::isfinite(real_delta_s) || real_delta_s < 0.0) {
            throw std::invalid_argument("real frame delta must be finite and non-negative");
        }
        const double scaled_delta_s = real_delta_s * config_.time_scale;
        const double next_accumulator_s = accumulator_s_ + scaled_delta_s;
        if (!std::isfinite(scaled_delta_s) || !std::isfinite(next_accumulator_s))
            throw std::overflow_error("scaled frame duration exceeds finite simulation time");
        accumulator_s_ = next_accumulator_s;
        std::size_t substeps = 0;
        while (accumulator_s_ >= config_.physics_dt_s && substeps < config_.max_substeps) {
            const double next_time_s = simulation_time_s_ + config_.physics_dt_s;
            if (!std::isfinite(next_time_s) || next_time_s <= simulation_time_s_)
                throw std::overflow_error("fixed timestep cannot advance finite simulation time");
            // Void callbacks are accepted for simple clients; bool/Status callbacks may reject a
            // step. A rejected step retains its elapsed time for a corrected retry.
            if constexpr (std::is_void_v<std::invoke_result_t<StepFunction&, double>>) {
                step_function(config_.physics_dt_s);
            } else if (!static_cast<bool>(step_function(config_.physics_dt_s))) {
                break;
            }
            accumulator_s_ -= config_.physics_dt_s;
            simulation_time_s_ = next_time_s;
            ++substeps;
        }
        if (substeps == config_.max_substeps && accumulator_s_ >= config_.physics_dt_s) {
            dropped_time_s_ =
                std::min(std::numeric_limits<double>::max() - accumulator_s_, dropped_time_s_) +
                accumulator_s_;
            accumulator_s_ = 0.0;
        }
        return substeps;
    }

    [[nodiscard]] double interpolationAlpha() const noexcept {
        return std::min(accumulator_s_ / config_.physics_dt_s, 1.0);
    }
    [[nodiscard]] double simulationTimeSeconds() const noexcept { return simulation_time_s_; }
    [[nodiscard]] double droppedTimeSeconds() const noexcept { return dropped_time_s_; }
    [[nodiscard]] double physicsDtSeconds() const noexcept { return config_.physics_dt_s; }
    void configure(SimulationClockConfig config);
    void reset() noexcept;

  private:
    SimulationClockConfig config_;
    double accumulator_s_{};
    double simulation_time_s_{};
    double dropped_time_s_{};
};

} // namespace aetherion::core
