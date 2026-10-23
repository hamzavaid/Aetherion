/** @file
 * Bounded inertial SI diagnostics for kinetic energy, conservative potentials, momentum, and speed
 * drift.
 */
#include "aetherion/core/telemetry.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>

#include "aetherion/core/scientific_analysis.hpp"
#include "aetherion/physics/constants.hpp"

namespace aetherion::core {
namespace {
// Relative error has no conventional denominator for a zero invariant. Use the absolute
// change as its own scale in that case (0 unchanged, 1 changed); saturate unrepresentable ratios.
double boundedRelativeError(double difference, double baseline_scale) {
    if (difference == 0.0)
        return 0.0;
    if (baseline_scale == 0.0)
        return 1.0;
    return std::min(difference / baseline_scale, std::numeric_limits<double>::max());
}
} // namespace

TelemetryRecorder::TelemetryRecorder(std::size_t capacity) : capacity_(capacity) {
    if (capacity == 0U) {
        throw std::invalid_argument("telemetry capacity must be non-zero");
    }
    samples_.reserve(capacity);
}

TelemetrySample TelemetryRecorder::sample(const Scene& scene, double time_s,
                                          const TelemetryForceModel& force_model) {
    if (!std::isfinite(time_s)) {
        throw std::invalid_argument("telemetry time must be finite in s");
    }
    TelemetrySample output{.time_s = time_s};
    const auto& bodies = scene.bodies();
    for (const auto& body : bodies) {
        const double speed_mps = body.state.velocity_mps.norm();
        output.kinetic_energy_J += 0.5 * body.mass_kg * speed_mps * speed_mps;
        output.linear_momentum_kg_mps += body.mass_kg * body.state.velocity_mps;
        output.angular_momentum_kg_m2_ps +=
            body.mass_kg * math::cross(body.state.position_m, body.state.velocity_mps);
        if (force_model.electrostatics_enabled &&
            body.interactions.contains(Interaction::electrostatic))
            output.electric_potential_energy_J -=
                body.charge_C * math::dot(force_model.uniform_electric_Vpm, body.state.position_m);
    }
    if (const auto center = centerOfMass(scene))
        output.center_of_mass_m = center->position_m;
    for (std::size_t first = 0; first < bodies.size(); ++first) {
        for (std::size_t second = first + 1U; second < bodies.size(); ++second) {
            const double separation_m =
                (bodies[second].state.position_m - bodies[first].state.position_m).norm();
            if (force_model.gravity_enabled && separation_m > 0.0 &&
                bodies[first].interactions.contains(Interaction::gravity) &&
                bodies[second].interactions.contains(Interaction::gravity)) {
                output.gravitational_potential_energy_J -=
                    physics::constants::gravitational_constant * bodies[first].mass_kg *
                    bodies[second].mass_kg / separation_m;
            }
            if (force_model.electrostatics_enabled &&
                separation_m > force_model.electric_minimum_separation_m &&
                bodies[first].interactions.contains(Interaction::electrostatic) &&
                bodies[second].interactions.contains(Interaction::electrostatic)) {
                const double softened_distance_m =
                    std::hypot(separation_m, force_model.electric_softening_m);
                output.electric_potential_energy_J += physics::constants::coulomb_constant *
                                                      bodies[first].charge_C *
                                                      bodies[second].charge_C / softened_distance_m;
            }
        }
    }
    output.total_energy_J = output.kinetic_energy_J + output.gravitational_potential_energy_J +
                            output.electric_potential_energy_J;
    bool same_entities = baseline_entity_ids_.size() == bodies.size();
    for (std::size_t index = 0; same_entities && index < bodies.size(); ++index)
        same_entities = baseline_entity_ids_[index] == bodies[index].id;
    if (!has_baseline_ || !same_entities || baseline_force_model_ != force_model) {
        baseline_force_model_ = force_model;
        has_baseline_ = true;
        baseline_energy_J_ = output.total_energy_J;
        baseline_momentum_kg_mps_ = output.linear_momentum_kg_mps;
        baseline_angular_momentum_kg_m2_ps_ = output.angular_momentum_kg_m2_ps;
        baseline_speeds_mps_.clear();
        baseline_entity_ids_.clear();
        baseline_speeds_mps_.reserve(bodies.size());
        for (const auto& body : bodies) {
            baseline_speeds_mps_.push_back(body.state.velocity_mps.norm());
            baseline_entity_ids_.push_back(body.id);
        }
    }
    output.relative_energy_error = boundedRelativeError(
        std::abs(output.total_energy_J - baseline_energy_J_), std::abs(baseline_energy_J_));
    output.momentum_error_kg_mps =
        (output.linear_momentum_kg_mps - baseline_momentum_kg_mps_).norm();
    output.relative_angular_momentum_error = boundedRelativeError(
        (output.angular_momentum_kg_m2_ps - baseline_angular_momentum_kg_m2_ps_).norm(),
        baseline_angular_momentum_kg_m2_ps_.norm());
    for (std::size_t index = 0; index < bodies.size(); ++index) {
        output.maximum_speed_drift_mps = std::max(
            output.maximum_speed_drift_mps,
            std::abs(bodies[index].state.velocity_mps.norm() - baseline_speeds_mps_[index]));
    }
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
    baseline_speeds_mps_.clear();
    baseline_entity_ids_.clear();
}

} // namespace aetherion::core
