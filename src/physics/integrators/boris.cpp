#include "aetherion/physics/integrators/boris.hpp"

#include <cmath>

namespace aetherion::physics {

core::Status BorisIntegrator::step(core::Scene& scene, double dt_s,
                                   const AccelerationFunction& nonmagnetic_acceleration,
                                   const MagneticFieldFunction& magnetic_field) {
    if (!std::isfinite(dt_s) || dt_s <= 0.0 || !nonmagnetic_acceleration || !magnetic_field) {
        return core::Error{core::ErrorCode::invalid_argument,
                           "Boris requires positive finite dt and valid field evaluators"};
    }
    initial_.resize(scene.size());
    for (std::size_t index = 0; index < scene.size(); ++index)
        initial_[index] = scene.bodies()[index].state;
    auto restore = [&]() {
        for (std::size_t index = 0; index < scene.size(); ++index)
            scene.bodies()[index].state = initial_[index];
    };
    auto status = nonmagnetic_acceleration(scene);
    if (!status)
        return status;
    const double half_dt = 0.5 * dt_s;
    for (auto& body : scene.bodies()) {
        if (body.fixed)
            continue;
        if (body.charge_C != 0.0 && body.mass_kg <= 0.0) {
            restore();
            return core::Error{core::ErrorCode::invalid_argument,
                               "Boris requires positive mass for charged dynamic bodies"};
        }
        const auto velocity_minus =
            body.state.velocity_mps + body.state.acceleration_mps2 * half_dt;
        math::Vec3d velocity_plus = velocity_minus;
        if (body.charge_C != 0.0 && body.interactions.contains(core::Interaction::magnetic)) {
            const auto sampled_b = magnetic_field(body.state.position_m);
            if (!sampled_b || !sampled_b.value().isFinite()) {
                restore();
                if (!sampled_b)
                    return sampled_b.error();
                return core::Error{core::ErrorCode::numerical_failure,
                                   "Boris sampled non-finite magnetic field"};
            }
            const auto rotation = sampled_b.value() * (body.charge_C * half_dt / body.mass_kg);
            const auto doubled_rotation = 2.0 * rotation / (1.0 + rotation.squaredNorm());
            const auto velocity_prime = velocity_minus + math::cross(velocity_minus, rotation);
            velocity_plus = velocity_minus + math::cross(velocity_prime, doubled_rotation);
        }
        body.state.velocity_mps = velocity_plus;
        body.state.position_m += velocity_plus * dt_s;
        if (!body.state.position_m.isFinite() || !body.state.velocity_mps.isFinite()) {
            restore();
            return core::Error{core::ErrorCode::numerical_failure,
                               "Boris pusher produced non-finite state"};
        }
    }
    status = nonmagnetic_acceleration(scene);
    if (!status) {
        restore();
        return status;
    }
    for (auto& body : scene.bodies()) {
        if (!body.fixed)
            body.state.velocity_mps += body.state.acceleration_mps2 * half_dt;
        if (!body.state.velocity_mps.isFinite()) {
            restore();
            return core::Error{core::ErrorCode::numerical_failure,
                               "Boris electric kick produced non-finite velocity"};
        }
    }
    return core::success();
}

} // namespace aetherion::physics
