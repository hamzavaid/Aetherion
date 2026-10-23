/** @file
 * Kick-drift-kick Velocity Verlet integration for position-dependent conservative accelerations.
 */
#include "aetherion/physics/integrators/velocity_verlet.hpp"

#include "aetherion/core/state_transaction.hpp"
#include <cmath>

namespace aetherion::physics {

core::Status
VelocityVerletIntegrator::step(core::Scene& scene, double dt_s,
                               const AccelerationFunction& evaluate_acceleration) const {
    if (!std::isfinite(dt_s) || dt_s <= 0.0 || !evaluate_acceleration) {
        return core::Error{
            core::ErrorCode::invalid_argument,
            "Velocity Verlet requires a positive finite dt and acceleration evaluator"};
    }
    core::StateTransaction transaction(scene);
    auto status = evaluate_acceleration(scene);
    if (!status)
        return status;
    const double half_dt = 0.5 * dt_s;
    for (auto& body : scene.bodies()) {
        if (body.fixed)
            continue;
        body.state.velocity_mps += body.state.acceleration_mps2 * half_dt;
        body.state.position_m += body.state.velocity_mps * dt_s;
    }
    status = evaluate_acceleration(scene);
    if (!status)
        return status;
    for (auto& body : scene.bodies()) {
        if (body.fixed)
            continue;
        body.state.velocity_mps += body.state.acceleration_mps2 * half_dt;
        if (!body.state.position_m.isFinite() || !body.state.velocity_mps.isFinite()) {
            return core::Error{core::ErrorCode::numerical_failure,
                               "Velocity Verlet produced non-finite state"};
        }
    }
    transaction.commit();
    return core::success();
}

} // namespace aetherion::physics
