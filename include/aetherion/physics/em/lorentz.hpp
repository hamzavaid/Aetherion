#pragma once

#include <cstddef>

#include "aetherion/core/error.hpp"
#include "aetherion/core/scene.hpp"
#include "aetherion/physics/em/electromagnetic_settings.hpp"
#include "aetherion/physics/fields/field_provider.hpp"

namespace aetherion::physics::em {

struct LorentzForceComponents {
    math::Vec3d electric_N;
    math::Vec3d magnetic_N;
    math::Vec3d total_N;
};

struct GyroDiagnostics {
    double radius_m{};
    double angular_frequency_rad_ps{};
    double period_s{};
};

/// Evaluates F=q(E+v cross B) in SI units and the inertial world frame.
[[nodiscard]] core::Result<LorentzForceComponents>
lorentzForce(double charge_C, const math::Vec3d& velocity_mps, const fields::FieldSample& field);

/// Nonrelativistic uniform-B gyro quantities using velocity perpendicular to B.
[[nodiscard]] core::Result<GyroDiagnostics> gyroDiagnostics(double mass_kg, double charge_C,
                                                            const math::Vec3d& velocity_mps,
                                                            const math::Vec3d& magnetic_T);

struct LorentzDiagnostics {
    std::size_t evaluated_bodies{};
    math::Vec3d total_electric_force_N;
    math::Vec3d total_magnetic_force_N;
};

/// Adds external analytic q(E+v cross B)/m without duplicating pairwise Coulomb fields.
class LorentzSolver final {
  public:
    explicit LorentzSolver(ElectromagneticSettings settings = {});
    void setSettings(const ElectromagneticSettings& settings);
    [[nodiscard]] core::Status accumulateAccelerations(core::Scene& scene, double time_s,
                                                       bool include_electric,
                                                       bool include_magnetic);
    [[nodiscard]] const LorentzDiagnostics& diagnostics() const noexcept { return diagnostics_; }

  private:
    ElectromagneticSettings settings_;
    LorentzDiagnostics diagnostics_;
};

} // namespace aetherion::physics::em
