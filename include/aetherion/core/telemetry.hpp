/** @file
 * Bounded inertial SI diagnostics for kinetic energy, conservative potentials, momentum, and speed
 * drift.
 */
#pragma once

#include <cstddef>
#include <vector>

#include "aetherion/core/scene.hpp"

namespace aetherion::core {

/// Conservative force configuration used only for diagnostics, not force evaluation.
struct TelemetryForceModel {
    bool gravity_enabled{true};
    bool electrostatics_enabled{};
    double electric_softening_m{};
    double electric_minimum_separation_m{};
    math::Vec3d uniform_electric_Vpm;
    bool operator==(const TelemetryForceModel&) const = default;
};

struct TelemetrySample {
    double time_s{};
    double kinetic_energy_J{};
    double gravitational_potential_energy_J{};
    double electric_potential_energy_J{};
    double total_energy_J{};
    math::Vec3d linear_momentum_kg_mps;
    math::Vec3d angular_momentum_kg_m2_ps;
    math::Vec3d center_of_mass_m;
    double relative_energy_error{};
    double momentum_error_kg_mps{};
    double relative_angular_momentum_error{};
    /// Maximum per-body absolute speed change from the first sample, in m/s.
    double maximum_speed_drift_mps{};
};

/// Bounded chronological mechanical diagnostics sampled from an inertial SI scene.
class TelemetryRecorder final {
  public:
    explicit TelemetryRecorder(std::size_t capacity = 4096);
    /// Energy includes enabled Newtonian, softened Coulomb, and static uniform-E potentials.
    /// Zero baseline relative errors are 0 if unchanged, 1 otherwise; overflow saturates.
    [[nodiscard]] TelemetrySample sample(const Scene& scene, double time_s,
                                         const TelemetryForceModel& force_model = {});
    [[nodiscard]] const std::vector<TelemetrySample>& samples() const noexcept { return samples_; }
    void clear() noexcept;

  private:
    std::size_t capacity_;
    std::vector<TelemetrySample> samples_;
    bool has_baseline_{};
    double baseline_energy_J_{};
    math::Vec3d baseline_momentum_kg_mps_;
    math::Vec3d baseline_angular_momentum_kg_m2_ps_;
    std::vector<double> baseline_speeds_mps_;
    std::vector<EntityId> baseline_entity_ids_;
    TelemetryForceModel baseline_force_model_;
};

} // namespace aetherion::core
