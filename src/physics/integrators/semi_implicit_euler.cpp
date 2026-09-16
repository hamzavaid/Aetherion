#include "aetherion/physics/integrators/semi_implicit_euler.hpp"

#include <cmath>

namespace aetherion::physics {

core::Status SemiImplicitEuler::integrate(core::Scene& scene, double dt_s) {
    if (!std::isfinite(dt_s) || dt_s <= 0.0) {
        return core::Error{core::ErrorCode::invalid_argument,
                           "integration timestep must be finite and positive in s"};
    }
    for (auto& body : scene.bodies()) {
        if (body.fixed) {
            continue;
        }
        body.state.velocity_mps += body.state.acceleration_mps2 * dt_s;
        body.state.position_m += body.state.velocity_mps * dt_s;
        if (!body.state.position_m.isFinite() || !body.state.velocity_mps.isFinite()) {
            return core::Error{core::ErrorCode::numerical_failure,
                               "Semi-Implicit Euler produced non-finite state"};
        }
    }
    return core::success();
}

} // namespace aetherion::physics
