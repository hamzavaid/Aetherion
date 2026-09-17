#pragma once

#include <cstddef>

#include "aetherion/core/error.hpp"
#include "aetherion/core/scene.hpp"

namespace aetherion::physics {

struct GravityConfig {
    double softening_m{};
    double minimum_separation_m{};
};

struct GravityDiagnostics {
    std::size_t evaluated_pairs{};
    std::size_t coincident_pairs{};
};

/// Direct O(N^2) Newtonian gravity reference solver in SI units.
class GravitySolver final {
  public:
    explicit GravitySolver(GravityConfig config = {});
    [[nodiscard]] core::Status computeAccelerations(core::Scene& scene);
    /// Adds Newtonian acceleration to the existing body acceleration accumulators.
    [[nodiscard]] core::Status accumulateAccelerations(core::Scene& scene);
    [[nodiscard]] const GravityDiagnostics& diagnostics() const noexcept { return diagnostics_; }

  private:
    GravityConfig config_;
    GravityDiagnostics diagnostics_;
};

} // namespace aetherion::physics
