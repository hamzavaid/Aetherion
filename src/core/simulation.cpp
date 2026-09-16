#include "aetherion/core/simulation.hpp"

#include <cmath>
#include <stdexcept>

#include "aetherion/physics/integrators/semi_implicit_euler.hpp"

namespace aetherion::core {

Simulation::Simulation(Scene& scene, SimulationConfig config) : scene_(scene), config_(config) {
    if (!std::isfinite(config.physics_dt_s) || config.physics_dt_s <= 0.0) {
        throw std::invalid_argument("simulation timestep must be finite and positive in s");
    }
    static_cast<void>(telemetry_.sample(scene_, 0.0));
}

Status Simulation::step() {
    const physics::AccelerationFunction evaluate = [this](Scene& evaluated_scene) {
        if (gravity_enabled_)
            return gravity_.computeAccelerations(evaluated_scene);
        for (auto& body : evaluated_scene.bodies())
            body.state.acceleration_mps2 = {};
        return success();
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
    }
    if (!status) {
        return status;
    }
    time_s_ += config_.physics_dt_s;
    static_cast<void>(telemetry_.sample(scene_, time_s_));
    return success();
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
