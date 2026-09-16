#include "aetherion/physics/stability_analyzer.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <stdexcept>

#include "aetherion/physics/constants.hpp"

namespace aetherion::physics {

std::vector<StabilityWarning> analyzeTimestep(const core::Scene& scene, double dt_s) {
    if (!std::isfinite(dt_s) || dt_s <= 0.0) {
        throw std::invalid_argument("stability analysis requires a positive finite timestep in s");
    }
    double shortest_period_s = std::numeric_limits<double>::infinity();
    double shortest_crossing_s = std::numeric_limits<double>::infinity();
    const auto& bodies = scene.bodies();
    for (std::size_t first = 0; first < bodies.size(); ++first) {
        for (std::size_t second = first + 1U; second < bodies.size(); ++second) {
            const auto relative_position =
                bodies[second].state.position_m - bodies[first].state.position_m;
            const double separation_m = relative_position.norm();
            const double total_mass_kg = bodies[first].mass_kg + bodies[second].mass_kg;
            if (separation_m > 0.0 && total_mass_kg > 0.0) {
                const double period_s =
                    2.0 * std::numbers::pi *
                    std::sqrt(separation_m * separation_m * separation_m /
                              (constants::gravitational_constant * total_mass_kg));
                shortest_period_s = std::min(shortest_period_s, period_s);
            }
            const double relative_speed =
                (bodies[second].state.velocity_mps - bodies[first].state.velocity_mps).norm();
            if (separation_m > 0.0 && relative_speed > 0.0) {
                shortest_crossing_s = std::min(shortest_crossing_s, separation_m / relative_speed);
            }
        }
    }
    std::vector<StabilityWarning> warnings;
    const auto append = [&](double timescale, double caution_ratio, std::string label) {
        if (!std::isfinite(timescale))
            return;
        const double ratio = dt_s / timescale;
        if (ratio > caution_ratio) {
            const auto severity =
                ratio > 0.1 ? StabilitySeverity::unstable : StabilitySeverity::caution;
            warnings.push_back(
                {severity, timescale, ratio, label + " is under-resolved; reduce physics dt"});
        }
    };
    append(shortest_period_s, 0.02, "shortest gravitational orbital period");
    append(shortest_crossing_s, 0.1, "shortest pair crossing timescale");
    return warnings;
}

} // namespace aetherion::physics
