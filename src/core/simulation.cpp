#include "aetherion/core/simulation.hpp"

#include <cmath>
#include <stdexcept>

#include "aetherion/physics/integrators/semi_implicit_euler.hpp"

namespace aetherion::core {

Simulation::Simulation(Scene& scene, SimulationConfig config)
    : scene_(scene), config_(config), electrostatics_(electromagnetic_settings_),
      lorentz_(electromagnetic_settings_), field_provider_(scene_, electromagnetic_settings_) {
    if (!std::isfinite(config.physics_dt_s) || config.physics_dt_s <= 0.0) {
        throw std::invalid_argument("simulation timestep must be finite and positive in s");
    }
    static_cast<void>(telemetry_.sample(scene_, 0.0));
}

Status Simulation::step() {
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
    time_s_ += config_.physics_dt_s;
    static_cast<void>(telemetry_.sample(scene_, time_s_));
    return success();
}

void Simulation::setElectromagneticSettings(const physics::em::ElectromagneticSettings& settings) {
    const auto status = physics::em::validateElectromagneticSettings(settings);
    if (!status)
        throw std::invalid_argument(status.error().message);
    electromagnetic_settings_ = settings;
    electrostatics_.setSettings(settings);
    lorentz_.setSettings(settings);
}

void Simulation::setPhysicsDt(double physics_dt_s) {
    if (!std::isfinite(physics_dt_s) || physics_dt_s <= 0.0) {
        throw std::invalid_argument("simulation timestep must be finite and positive in s");
    }
    config_.physics_dt_s = physics_dt_s;
}

void Simulation::reset(double time_s) {
    if (!std::isfinite(time_s) || time_s < 0.0) {
        throw std::invalid_argument("simulation reset time must be finite and non-negative in s");
    }
    time_s_ = time_s;
    telemetry_.clear();
    static_cast<void>(telemetry_.sample(scene_, time_s_));
}

} // namespace aetherion::core
