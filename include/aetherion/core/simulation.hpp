/** @file
 * Headless composition of instantaneous forces, particle integrators, accepted-state history, and
 * diagnostics.
 */
#pragma once

#include <stdexcept>

#include "aetherion/core/error.hpp"
#include "aetherion/core/scene.hpp"
#include "aetherion/core/telemetry.hpp"
#include "aetherion/physics/em/electrostatics.hpp"
#include "aetherion/physics/em/lorentz.hpp"
#include "aetherion/physics/em/retarded_fields.hpp"
#include "aetherion/physics/gravity/gravity_solver.hpp"
#include "aetherion/physics/integrators/boris.hpp"
#include "aetherion/physics/integrators/integrator.hpp"
#include "aetherion/physics/integrators/rk4.hpp"
#include "aetherion/physics/integrators/velocity_verlet.hpp"

namespace aetherion::core {

struct SimulationConfig {
    double physics_dt_s{1.0 / 120.0};
    physics::IntegratorKind integrator{physics::IntegratorKind::semi_implicit_euler};
};

/// Headless mechanics composition: gravity, integration, simulation time, and telemetry.
class Simulation final {
  public:
    Simulation(Scene& scene, SimulationConfig config = {});
    /// Advances one fixed SI timestep. Numerical failure restores entry body kinematics and time;
    /// only accepted final states are added to source history and invariant telemetry.
    [[nodiscard]] Status step();
    void setPhysicsDt(double physics_dt_s);
    void setGravityEnabled(bool enabled) noexcept { gravity_enabled_ = enabled; }
    void setElectromagneticSettings(const physics::em::ElectromagneticSettings& settings);
    /// Atomically validates timestep and source-history configuration before changing either.
    void configureFields(const physics::em::ElectromagneticSettings& settings, double dt_s);
    void invalidateSourceHistory();
    void setIntegrator(physics::IntegratorKind integrator) {
        if (!physics::isValidIntegrator(integrator))
            throw std::invalid_argument("unknown particle integrator");
        config_.integrator = integrator;
    }
    /// Clears diagnostics and establishes a new baseline at the supplied simulation time in
    /// seconds.
    void reset(double time_s = 0.0);
    [[nodiscard]] double timeSeconds() const noexcept { return time_s_; }
    [[nodiscard]] TelemetryRecorder& telemetry() noexcept { return telemetry_; }
    [[nodiscard]] const TelemetryRecorder& telemetry() const noexcept { return telemetry_; }
    [[nodiscard]] const physics::fields::IFieldProvider& fieldProvider() const noexcept {
        if (electromagnetic_settings_.retarded.enabled)
            return retarded_provider_;
        return field_provider_;
    }
    [[nodiscard]] const physics::em::SourceHistories& sourceHistories() const noexcept {
        return source_histories_;
    }
    [[nodiscard]] const physics::em::RetardedDiagnostics& retardedDiagnostics() const noexcept {
        return retarded_provider_.diagnostics();
    }

  private:
    /// Samples conservative diagnostics with the same enabled forces as the motion solver.
    void recordTelemetry();
    Scene& scene_;
    SimulationConfig config_;
    physics::GravitySolver gravity_;
    physics::em::ElectromagneticSettings electromagnetic_settings_;
    physics::em::ElectrostaticSolver electrostatics_;
    physics::em::LorentzSolver lorentz_;
    physics::em::ElectromagneticFieldProvider field_provider_;
    physics::em::SourceHistories source_histories_;
    physics::em::RetardedFieldProvider retarded_provider_;
    physics::BorisIntegrator boris_;
    physics::VelocityVerletIntegrator velocity_verlet_;
    physics::Rk4Integrator rk4_;
    TelemetryRecorder telemetry_;
    double time_s_{};
    bool gravity_enabled_{true};
};

} // namespace aetherion::core
