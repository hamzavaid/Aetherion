#include "aetherion/core/telemetry.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>

#include "aetherion/physics/constants.hpp"

namespace aetherion::core {

TelemetryRecorder::TelemetryRecorder(std::size_t capacity) : capacity_(capacity) {
    if (capacity == 0U) {
        throw std::invalid_argument("telemetry capacity must be non-zero");
    }
    samples_.reserve(capacity);
}

TelemetrySample TelemetryRecorder::sample(const Scene& scene, double time_s) {
    if (!std::isfinite(time_s)) {
        throw std::invalid_argument("telemetry time must be finite in s");
    }
    TelemetrySample output{.time_s = time_s};
    double total_mass_kg = 0.0;
    const auto& bodies = scene.bodies();
    for (const auto& body : bodies) {
        output.kinetic_energy_J += 0.5 * body.mass_kg * body.state.velocity_mps.squaredNorm();
        output.linear_momentum_kg_mps += body.mass_kg * body.state.velocity_mps;
        output.angular_momentum_kg_m2_ps +=
            body.mass_kg * math::cross(body.state.position_m, body.state.velocity_mps);
        output.center_of_mass_m += body.mass_kg * body.state.position_m;
        total_mass_kg += body.mass_kg;
    }
    if (total_mass_kg > 0.0) {
        output.center_of_mass_m /= total_mass_kg;
    }
    for (std::size_t first = 0; first < bodies.size(); ++first) {
        for (std::size_t second = first + 1U; second < bodies.size(); ++second) {
            if (!bodies[first].interactions.contains(Interaction::gravity) ||
                !bodies[second].interactions.contains(Interaction::gravity)) {
                continue;
            }
            const double separation_m =
                (bodies[second].state.position_m - bodies[first].state.position_m).norm();
            if (separation_m > 0.0) {
                output.gravitational_potential_energy_J -=
                    physics::constants::gravitational_constant * bodies[first].mass_kg *
                    bodies[second].mass_kg / separation_m;
            }
        }
    }
    output.total_energy_J = output.kinetic_energy_J + output.gravitational_potential_energy_J;
    if (!has_baseline_) {
        has_baseline_ = true;
        baseline_energy_J_ = output.total_energy_J;
        baseline_momentum_kg_mps_ = output.linear_momentum_kg_mps;
        baseline_angular_momentum_kg_m2_ps_ = output.angular_momentum_kg_m2_ps;
    }
    const double energy_scale =
        std::max(std::abs(baseline_energy_J_), std::numeric_limits<double>::min());
    output.relative_energy_error =
        std::abs(output.total_energy_J - baseline_energy_J_) / energy_scale;
    output.momentum_error_kg_mps =
        (output.linear_momentum_kg_mps - baseline_momentum_kg_mps_).norm();
    const double angular_scale =
        std::max(baseline_angular_momentum_kg_m2_ps_.norm(), std::numeric_limits<double>::min());
    output.relative_angular_momentum_error =
        (output.angular_momentum_kg_m2_ps - baseline_angular_momentum_kg_m2_ps_).norm() /
        angular_scale;
    if (samples_.size() == capacity_) {
        samples_.erase(samples_.begin());
    }
    samples_.push_back(output);
    return output;
}

void TelemetryRecorder::clear() noexcept {
    samples_.clear();
    has_baseline_ = false;
    baseline_energy_J_ = 0.0;
    baseline_momentum_kg_mps_ = {};
    baseline_angular_momentum_kg_m2_ps_ = {};
}

} // namespace aetherion::core
