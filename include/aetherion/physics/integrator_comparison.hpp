#pragma once

#include <cstddef>
#include <vector>

#include "aetherion/core/scene.hpp"
#include "aetherion/physics/integrators/integrator.hpp"

namespace aetherion::physics {

struct NumericalErrorPoint {
    double time_s{};
    double relative_energy_error{};
    double momentum_error_kg_mps{};
};

struct IntegratorRun {
    IntegratorKind integrator{};
    core::Scene final_scene;
    std::vector<NumericalErrorPoint> error_series;
    double maximum_relative_energy_error{};
    double maximum_momentum_error_kg_mps{};
    double relative_orbit_closure_error{};
    double relative_orbit_reference_error{};
};

struct IntegratorComparisonReport {
    double dt_s{};
    std::size_t steps{};
    std::vector<IntegratorRun> runs;
    [[nodiscard]] const IntegratorRun& forIntegrator(IntegratorKind integrator) const;
};

/// Runs identical initial state through all Phase 4 integrators and records invariant/orbit errors.
class IntegratorComparison final {
  public:
    [[nodiscard]] static IntegratorComparisonReport run(const core::Scene& initial_scene,
                                                        double dt_s, std::size_t steps);
};

} // namespace aetherion::physics
