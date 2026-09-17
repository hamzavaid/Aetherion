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

/// Superposed point-charge and analytic external fields at arbitrary world positions.
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

} // namespace aetherion::physics::em
