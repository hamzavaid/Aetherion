/** @file
 * Fixed-step time accumulation with bounded catch-up, rejected-step retry, and finite-time checks.
 */
#include "aetherion/core/simulation_clock.hpp"

#include <cmath>
#include <stdexcept>

namespace aetherion::core {

SimulationClock::SimulationClock(SimulationClockConfig config) : config_(config) {
    configure(config);
}

void SimulationClock::configure(SimulationClockConfig config) {
    if (!std::isfinite(config.physics_dt_s) || config.physics_dt_s <= 0.0 ||
        !std::isfinite(config.time_scale) || config.time_scale < 0.0 || config.max_substeps == 0U) {
        throw std::invalid_argument(
            "simulation clock requires positive dt/substeps and finite time scale");
    }
    config_ = config;
    // The accumulator is already in simulation seconds. Retain it across timestep/time-scale
    // edits; advance() applies the new step size and its usual catch-up limit.
}

void SimulationClock::reset() noexcept {
    accumulator_s_ = 0.0;
    simulation_time_s_ = 0.0;
    dropped_time_s_ = 0.0;
}

} // namespace aetherion::core
