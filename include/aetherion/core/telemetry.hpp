#pragma once

#include <cstddef>
#include <vector>

#include "aetherion/core/scene.hpp"

namespace aetherion::core {

struct TelemetrySample {
    double time_s{};
    double kinetic_energy_J{};
    double gravitational_potential_energy_J{};
    double total_energy_J{};
    math::Vec3d linear_momentum_kg_mps;
    math::Vec3d angular_momentum_kg_m2_ps;
    math::Vec3d center_of_mass_m;
    double relative_energy_error{};
    double momentum_error_kg_mps{};
    double relative_angular_momentum_error{};
};

/// Bounded chronological mechanical diagnostics sampled from an inertial SI scene.
class TelemetryRecorder final {
  public:
    explicit TelemetryRecorder(std::size_t capacity = 4096);
    [[nodiscard]] TelemetrySample sample(const Scene& scene, double time_s);
    [[nodiscard]] const std::vector<TelemetrySample>& samples() const noexcept { return samples_; }
    void clear() noexcept;

  private:
    std::size_t capacity_;
    std::vector<TelemetrySample> samples_;
    bool has_baseline_{};
    double baseline_energy_J_{};
    math::Vec3d baseline_momentum_kg_mps_;
    math::Vec3d baseline_angular_momentum_kg_m2_ps_;
};

} // namespace aetherion::core
