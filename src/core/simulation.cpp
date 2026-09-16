#include "aetherion/core/simulation.hpp"

#include <cmath>
#include <stdexcept>

#include "aetherion/physics/integrators/semi_implicit_euler.hpp"

namespace aetherion::core {

Simulation::Simulation(Scene& scene, SimulationConfig config) : scene_(scene), config_(config) {
    if (!std::isfinite(config.physics_dt_s) || config.physics_dt_s <= 0.0) {
        throw std::invalid_argument("simulation timestep must be finite and positive in s");
    }
}

Status Simulation::step() {
    Status status = success();
    if (gravity_enabled_) {
        status = gravity_.computeAccelerations(scene_);
        if (!status) {
            return status;
        }
    } else {
        for (auto& body : scene_.bodies()) {
            body.state.acceleration_mps2 = {};
        }
    }
    status = physics::SemiImplicitEuler::integrate(scene_, config_.physics_dt_s);
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

void Simulation::reset() noexcept {
    time_s_ = 0.0;
    telemetry_.clear();
}

} // namespace aetherion::core
