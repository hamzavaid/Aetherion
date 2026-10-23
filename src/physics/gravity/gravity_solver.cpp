/** @file
 * Deterministic direct Newtonian gravity with optional Plummer softening and separation guards.
 */
#include "aetherion/physics/gravity/gravity_solver.hpp"

#include <cmath>
#include <stdexcept>

#include "aetherion/core/state_transaction.hpp"
#include "aetherion/physics/constants.hpp"

namespace aetherion::physics {

GravitySolver::GravitySolver(GravityConfig config) : config_(config) {
    if (!std::isfinite(config.softening_m) || config.softening_m < 0.0 ||
        !std::isfinite(config.minimum_separation_m) || config.minimum_separation_m < 0.0) {
        throw std::invalid_argument("gravity distance guards must be finite and non-negative in m");
    }
}

core::Status GravitySolver::computeAccelerations(core::Scene& scene) {
    core::StateTransaction transaction(scene);
    auto& bodies = scene.bodies();
    for (auto& body : bodies) {
        body.state.acceleration_mps2 = {};
    }
    const auto status = accumulateAccelerations(scene);
    if (status)
        transaction.commit();
    return status;
}

core::Status GravitySolver::accumulateAccelerations(core::Scene& scene) {
    core::StateTransaction transaction(scene);
    diagnostics_ = {};
    auto& bodies = scene.bodies();

    for (std::size_t first = 0; first < bodies.size(); ++first) {
        for (std::size_t second = first + 1U; second < bodies.size(); ++second) {
            auto& lhs = bodies[first];
            auto& rhs = bodies[second];
            if (!lhs.interactions.contains(core::Interaction::gravity) ||
                !rhs.interactions.contains(core::Interaction::gravity)) {
                continue;
            }
            const auto displacement_m = rhs.state.position_m - lhs.state.position_m;
            const double physical_distance_m = displacement_m.norm();
            if (physical_distance_m <= config_.minimum_separation_m) {
                ++diagnostics_.coincident_pairs;
                continue;
            }
            const double softened_distance_m = std::hypot(physical_distance_m, config_.softening_m);
            if (!std::isfinite(softened_distance_m) || softened_distance_m <= 0.0)
                return core::Error{core::ErrorCode::numerical_failure,
                                   "gravity distance exceeds numerical range"};
            // Factor r/r_soft and G*m/r_soft² separately; r³ need not be representable even
            // when the final acceleration is finite. This retains Plummer softening exactly.
            const auto softened_direction = displacement_m / softened_distance_m;
            if (!lhs.fixed) {
                lhs.state.acceleration_mps2 +=
                    softened_direction *
                    ((constants::gravitational_constant * rhs.mass_kg / softened_distance_m) /
                     softened_distance_m);
            }
            if (!rhs.fixed) {
                rhs.state.acceleration_mps2 -=
                    softened_direction *
                    ((constants::gravitational_constant * lhs.mass_kg / softened_distance_m) /
                     softened_distance_m);
            }
            ++diagnostics_.evaluated_pairs;
        }
    }

    for (const auto& body : bodies) {
        if (!body.state.acceleration_mps2.isFinite()) {
            return core::Error{core::ErrorCode::numerical_failure,
                               "gravity generated a non-finite acceleration"};
        }
    }
    transaction.commit();
    return core::success();
}

} // namespace aetherion::physics
