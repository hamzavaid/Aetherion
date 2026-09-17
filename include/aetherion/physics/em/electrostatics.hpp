#pragma once

#include <cstddef>

#include "aetherion/core/error.hpp"
#include "aetherion/core/scene.hpp"
#include "aetherion/physics/em/electromagnetic_settings.hpp"
#include "aetherion/physics/fields/field_provider.hpp"

namespace aetherion::physics::em {

struct ElectrostaticDiagnostics {
    std::size_t evaluated_pairs{};
    std::size_t guarded_pairs{};
};

/// Direct O(N^2) Coulomb reference solver. It accumulates acceleration in m/s^2.
class ElectrostaticSolver final {
  public:
    explicit ElectrostaticSolver(ElectromagneticSettings settings = {});
    [[nodiscard]] core::Status accumulateAccelerations(core::Scene& scene);
    [[nodiscard]] const ElectrostaticDiagnostics& diagnostics() const noexcept {
        return diagnostics_;
    }
    void setSettings(const ElectromagneticSettings& settings);

  private:
    ElectromagneticSettings settings_;
    ElectrostaticDiagnostics diagnostics_;
};

/// Superposed scene gravity, point-charge, and analytic EM fields at arbitrary world positions.
class ElectromagneticFieldProvider final : public fields::IFieldProvider {
  public:
    ElectromagneticFieldProvider(const core::Scene& scene,
                                 const ElectromagneticSettings& settings) noexcept;
    [[nodiscard]] fields::FieldSample sample(const math::Vec3d& position_m,
                                             double time_s) const override;
    [[nodiscard]] std::uint64_t revision() const noexcept override;

  private:
    const core::Scene& scene_;
    const ElectromagneticSettings& settings_;
};

[[nodiscard]] core::Status validateElectromagneticSettings(const ElectromagneticSettings& settings);
/// Samples configured static analytic sources without particle-generated Coulomb fields.
///
/// Uniform source components are SI V/m and T. A magnetic dipole uses
/// B=mu0/(4*pi*r^3)*(3(m dot r_hat)r_hat-m), with moment in A m^2. Samples inside a source's
/// singularity radius are marked invalid; time is accepted for the common provider contract but
/// the Phase 6 sources are time independent.
[[nodiscard]] fields::FieldSample sampleAnalyticField(const ElectromagneticSettings& settings,
                                                      const math::Vec3d& position_m, double time_s);

} // namespace aetherion::physics::em
