#include "aetherion/physics/gravity/gravity_solver.hpp"

#include <cmath>
#include <stdexcept>

#include "aetherion/physics/constants.hpp"

namespace aetherion::physics {

GravitySolver::GravitySolver(GravityConfig config) : config_(config) {
    if (!std::isfinite(config.softening_m) || config.softening_m < 0.0 ||
        !std::isfinite(config.minimum_separation_m) || config.minimum_separation_m < 0.0) {
        throw std::invalid_argument("gravity distance guards must be finite and non-negative in m");
    }
}

core::Status GravitySolver::computeAccelerations(core::Scene& scene) {
    auto& bodies = scene.bodies();
    for (auto& body : bodies) {
        body.state.acceleration_mps2 = {};
    }
    return accumulateAccelerations(scene);
}

core::Status GravitySolver::accumulateAccelerations(core::Scene& scene) {
    diagnostics_ = {};
    auto& bodies = scene.bodies();

    const double softening_squared = config_.softening_m * config_.softening_m;
    const double minimum_squared = config_.minimum_separation_m * config_.minimum_separation_m;
    for (std::size_t first = 0; first < bodies.size(); ++first) {
        for (std::size_t second = first + 1U; second < bodies.size(); ++second) {
            auto& lhs = bodies[first];
            auto& rhs = bodies[second];
            if (!lhs.interactions.contains(core::Interaction::gravity) ||
                !rhs.interactions.contains(core::Interaction::gravity)) {
                continue;
            }
            const auto displacement_m = rhs.state.position_m - lhs.state.position_m;
            const double physical_distance_squared = displacement_m.squaredNorm();
            if (physical_distance_squared <= minimum_squared) {
                ++diagnostics_.coincident_pairs;
                continue;
            }
            const double softened_distance_squared = physical_distance_squared + softening_squared;
            const double inverse_distance_cubed =
                1.0 / (softened_distance_squared * std::sqrt(softened_distance_squared));
            const auto acceleration_factor =
                constants::gravitational_constant * displacement_m * inverse_distance_cubed;
            if (!lhs.fixed) {
                lhs.state.acceleration_mps2 += rhs.mass_kg * acceleration_factor;
            }
            if (!rhs.fixed) {
                rhs.state.acceleration_mps2 -= lhs.mass_kg * acceleration_factor;
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
    return core::success();
}

} // namespace aetherion::physics
