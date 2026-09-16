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
    if (accumulator_s_ >= config_.physics_dt_s) {
        dropped_time_s_ += accumulator_s_;
        accumulator_s_ = 0.0;
    }
}

void SimulationClock::reset() noexcept {
    accumulator_s_ = 0.0;
    simulation_time_s_ = 0.0;
    dropped_time_s_ = 0.0;
}

} // namespace aetherion::core
