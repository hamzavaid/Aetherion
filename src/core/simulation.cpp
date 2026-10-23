/** @file
 * Headless composition of instantaneous forces, particle integrators, accepted-state history, and
 * diagnostics.
 */
#include "aetherion/core/simulation.hpp"

#include <cmath>
#include <stdexcept>

#include "aetherion/core/state_transaction.hpp"
#include "aetherion/physics/integrators/semi_implicit_euler.hpp"

namespace aetherion::core {

Simulation::Simulation(Scene& scene, SimulationConfig config)
    : scene_(scene), config_(config), electrostatics_(electromagnetic_settings_),
      lorentz_(electromagnetic_settings_), field_provider_(scene_, electromagnetic_settings_),
      retarded_provider_(scene_, electromagnetic_settings_, source_histories_, field_provider_) {
    if (!std::isfinite(config.physics_dt_s) || config.physics_dt_s <= 0.0) {
        throw std::invalid_argument("simulation timestep must be finite and positive in s");
    }
    setIntegrator(config.integrator);
    recordTelemetry();
}

Status Simulation::step() {
    core::StateTransaction transaction(scene_);
    if (electromagnetic_settings_.retarded.enabled) {
        const auto record_status = source_histories_.record(scene_, time_s_);
        if (!record_status)
            return record_status;
    }
    const physics::AccelerationFunction evaluate_nonmagnetic = [this](Scene& evaluated_scene) {
        for (auto& body : evaluated_scene.bodies())
            body.state.acceleration_mps2 = {};
        if (gravity_enabled_) {
            const auto status = gravity_.accumulateAccelerations(evaluated_scene);
            if (!status)
                return status;
        }
        if (electromagnetic_settings_.electrostatics_enabled) {
            const auto status = electrostatics_.accumulateAccelerations(evaluated_scene);
            if (!status)
                return status;
        }
        const auto lorentz_status = lorentz_.accumulateAccelerations(
            evaluated_scene, time_s_, electromagnetic_settings_.electrostatics_enabled, false);
        if (!lorentz_status)
            return lorentz_status;
        return success();
    };
    const physics::AccelerationFunction evaluate = [this, &evaluate_nonmagnetic](Scene& evaluated) {
        const auto status = evaluate_nonmagnetic(evaluated);
        if (!status)
            return status;
        return lorentz_.accumulateAccelerations(evaluated, time_s_, false,
                                                electromagnetic_settings_.magnetic_enabled);
    };
    Status status = success();
    switch (config_.integrator) {
    case physics::IntegratorKind::semi_implicit_euler:
        status = evaluate(scene_);
        if (status) {
            status = physics::SemiImplicitEuler::integrate(scene_, config_.physics_dt_s);
        }
        break;
    case physics::IntegratorKind::velocity_verlet:
        status = velocity_verlet_.step(scene_, config_.physics_dt_s, evaluate);
        break;
    case physics::IntegratorKind::rk4:
        status = rk4_.step(scene_, config_.physics_dt_s, evaluate);
        break;
    case physics::IntegratorKind::boris: {
        const physics::MagneticFieldFunction magnetic =
            [this](const math::Vec3d& position_m) -> Result<math::Vec3d> {
            if (!electromagnetic_settings_.magnetic_enabled)
                return math::Vec3d{};
            const auto sample =
                physics::em::sampleAnalyticField(electromagnetic_settings_, position_m, time_s_);
            if (!sample.valid)
                return Error{ErrorCode::numerical_failure,
                             "Boris magnetic sample entered a source singularity"};
            return sample.magnetic_T;
        };
        status = boris_.step(scene_, config_.physics_dt_s, evaluate_nonmagnetic, magnetic);
        if (status)
            status = evaluate(scene_);
        break;
    }
    }
    if (!status) {
        return status;
    }
    const double next_time_s = time_s_ + config_.physics_dt_s;
    if (!std::isfinite(next_time_s) || next_time_s <= time_s_) {
        return Error{ErrorCode::numerical_failure,
                     "simulation timestep cannot advance finite time"};
    }
    if (electromagnetic_settings_.retarded.enabled) {
        const auto record_status = source_histories_.record(scene_, next_time_s);
        if (!record_status) {
            return record_status;
        }
    }
    time_s_ = next_time_s;
    recordTelemetry();
    transaction.commit();
    return success();
}

void Simulation::setElectromagneticSettings(const physics::em::ElectromagneticSettings& settings) {
    configureFields(settings, config_.physics_dt_s);
}

void Simulation::configureFields(const physics::em::ElectromagneticSettings& settings,
                                 double dt_s) {
    const auto status = physics::em::validateElectromagneticSettings(settings);
    if (!status)
        throw std::invalid_argument(status.error().message);
    const auto scene_status = physics::em::validateRetardedScene(scene_, settings.retarded, dt_s);
    if (!scene_status)
        throw std::invalid_argument(scene_status.error().message);
    const auto before_revision = source_histories_.revision();
    const auto configured = source_histories_.configure(settings.retarded, dt_s);
    if (!configured)
        throw std::invalid_argument(configured.error().message);
    config_.physics_dt_s = dt_s;
    electromagnetic_settings_ = settings;
    electrostatics_.setSettings(settings);
    lorentz_.setSettings(settings);
    if (source_histories_.revision() != before_revision) {
        retarded_provider_.clearDiagnostics();
        const auto recorded = source_histories_.record(scene_, time_s_);
        if (!recorded)
            throw std::invalid_argument(recorded.error().message);
    }
}

void Simulation::setPhysicsDt(double physics_dt_s) {
    if (!std::isfinite(physics_dt_s) || physics_dt_s <= 0.0) {
        throw std::invalid_argument("simulation timestep must be finite and positive in s");
    }
    configureFields(electromagnetic_settings_, physics_dt_s);
}

void Simulation::invalidateSourceHistory() {
    source_histories_.clear();
    retarded_provider_.clearDiagnostics();
    const auto status = source_histories_.record(scene_, time_s_);
    if (!status)
        throw std::invalid_argument(status.error().message);
}

void Simulation::reset(double time_s) {
    if (!std::isfinite(time_s) || time_s < 0.0) {
        throw std::invalid_argument("simulation reset time must be finite and non-negative in s");
    }
    time_s_ = time_s;
    invalidateSourceHistory();
    telemetry_.clear();
    recordTelemetry();
}

void Simulation::recordTelemetry() {
    TelemetryForceModel model{
        .gravity_enabled = gravity_enabled_,
        .electrostatics_enabled = electromagnetic_settings_.electrostatics_enabled,
        .electric_softening_m = electromagnetic_settings_.softening_m,
        .electric_minimum_separation_m = electromagnetic_settings_.minimum_separation_m};
    for (const auto& source : electromagnetic_settings_.analytic_sources)
        if (source.kind == physics::em::AnalyticFieldSourceKind::uniform)
            model.uniform_electric_Vpm += source.electric_Vpm;
    static_cast<void>(telemetry_.sample(scene_, time_s_, model));
}

} // namespace aetherion::core
