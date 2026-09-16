#include "aetherion/core/simulation.hpp"
#include "aetherion/presets/mechanics_presets.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <iostream>

int main() {
    auto scene = aetherion::presets::makeEarthLikeOrbit();
    constexpr double dt_s = 3600.0;
    aetherion::core::Simulation simulation(scene, {.physics_dt_s = dt_s});
    const auto initial = simulation.telemetry().sample(scene, 0.0);
    const auto steps = static_cast<std::size_t>(
        std::llround(aetherion::presets::earth_like_orbit_period_s / dt_s));
    double maximum_radius_error = 0.0;
    double maximum_energy_error = 0.0;
    for (std::size_t step = 0; step < steps; ++step) {
        if (!simulation.step()) {
            std::cerr << "Earth-like orbit simulation failed at step " << step << '\n';
            return 1;
        }
        const auto relative_position =
            scene.bodies()[1].state.position_m - scene.bodies()[0].state.position_m;
        maximum_radius_error =
            std::max(maximum_radius_error,
                     std::abs(relative_position.norm() - aetherion::presets::astronomical_unit_m) /
                         aetherion::presets::astronomical_unit_m);
        const auto& sample = simulation.telemetry().samples().back();
        maximum_energy_error = std::max(
            maximum_energy_error,
            std::abs((sample.total_energy_J - initial.total_energy_J) / initial.total_energy_J));
    }
    std::cout << std::setprecision(17) << "steps=" << steps << "\n"
              << "simulated_time_s=" << simulation.timeSeconds() << "\n"
              << "maximum_relative_radius_error=" << maximum_radius_error << "\n"
              << "maximum_relative_energy_error=" << maximum_energy_error << '\n';
    return maximum_radius_error < 2.0e-3 && maximum_energy_error < 2.0e-5 ? 0 : 2;
}
