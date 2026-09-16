#include "aetherion/physics/integrators/rk4.hpp"

#include <cmath>

namespace aetherion::physics {

void Rk4Integrator::resize(std::size_t count) {
    initial_.resize(count);
    k1_.resize(count);
    k2_.resize(count);
    k3_.resize(count);
    k4_.resize(count);
}

void Rk4Integrator::restore(core::Scene& scene) noexcept {
    for (std::size_t index = 0; index < initial_.size(); ++index) {
        scene.bodies()[index].state = initial_[index];
    }
}

void Rk4Integrator::setStage(core::Scene& scene, const std::vector<Derivative>& derivative,
                             double scale_s) {
    for (std::size_t index = 0; index < initial_.size(); ++index) {
        auto& body = scene.bodies()[index];
        body.state = initial_[index];
        if (body.fixed)
            continue;
        body.state.position_m += derivative[index].position_rate_mps * scale_s;
        body.state.velocity_mps += derivative[index].velocity_rate_mps2 * scale_s;
    }
}

void Rk4Integrator::captureDerivative(const core::Scene& scene,
                                      std::vector<Derivative>& destination) {
    for (std::size_t index = 0; index < initial_.size(); ++index) {
        const auto& state = scene.bodies()[index].state;
        destination[index] = {state.velocity_mps, state.acceleration_mps2};
    }
}

core::Status Rk4Integrator::step(core::Scene& scene, double dt_s,
                                 const AccelerationFunction& evaluate_acceleration) {
    if (!std::isfinite(dt_s) || dt_s <= 0.0 || !evaluate_acceleration) {
        return core::Error{core::ErrorCode::invalid_argument,
                           "RK4 requires a positive finite dt and acceleration evaluator"};
    }
    resize(scene.size());
    for (std::size_t index = 0; index < scene.size(); ++index) {
        initial_[index] = scene.bodies()[index].state;
    }
    auto status = evaluate_acceleration(scene);
    if (!status) {
        restore(scene);
        return status;
    }
    captureDerivative(scene, k1_);
    setStage(scene, k1_, 0.5 * dt_s);
    status = evaluate_acceleration(scene);
    if (!status) {
        restore(scene);
        return status;
    }
    captureDerivative(scene, k2_);
    setStage(scene, k2_, 0.5 * dt_s);
    status = evaluate_acceleration(scene);
    if (!status) {
        restore(scene);
        return status;
    }
    captureDerivative(scene, k3_);
    setStage(scene, k3_, dt_s);
    status = evaluate_acceleration(scene);
    if (!status) {
        restore(scene);
        return status;
    }
    captureDerivative(scene, k4_);

    for (std::size_t index = 0; index < scene.size(); ++index) {
        auto& body = scene.bodies()[index];
        body.state = initial_[index];
        if (body.fixed)
            continue;
        body.state.position_m +=
            (k1_[index].position_rate_mps + 2.0 * k2_[index].position_rate_mps +
             2.0 * k3_[index].position_rate_mps + k4_[index].position_rate_mps) *
            (dt_s / 6.0);
        body.state.velocity_mps +=
            (k1_[index].velocity_rate_mps2 + 2.0 * k2_[index].velocity_rate_mps2 +
             2.0 * k3_[index].velocity_rate_mps2 + k4_[index].velocity_rate_mps2) *
            (dt_s / 6.0);
        if (!body.state.position_m.isFinite() || !body.state.velocity_mps.isFinite()) {
            restore(scene);
            return core::Error{core::ErrorCode::numerical_failure, "RK4 produced non-finite state"};
        }
    }
    status = evaluate_acceleration(scene);
    if (!status)
        restore(scene);
    return status;
}

} // namespace aetherion::physics
