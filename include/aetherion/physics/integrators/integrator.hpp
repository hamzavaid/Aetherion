/** @file
 * Particle-integrator selection and force-evaluation contracts; independent of visualization
 * tracing.
 */
#pragma once

#include <functional>
#include <string_view>

#include "aetherion/core/error.hpp"
#include "aetherion/core/scene.hpp"

namespace aetherion::physics {

enum class IntegratorKind { semi_implicit_euler, velocity_verlet, rk4, boris };

/// Rejects out-of-range enum values supplied by runtime commands or programmatic callers.
[[nodiscard]] constexpr bool isValidIntegrator(IntegratorKind kind) noexcept {
    return kind == IntegratorKind::semi_implicit_euler || kind == IntegratorKind::velocity_verlet ||
           kind == IntegratorKind::rk4 || kind == IntegratorKind::boris;
}

[[nodiscard]] constexpr std::string_view integratorName(IntegratorKind kind) noexcept {
    switch (kind) {
    case IntegratorKind::semi_implicit_euler:
        return "Semi-Implicit Euler";
    case IntegratorKind::velocity_verlet:
        return "Velocity Verlet";
    case IntegratorKind::rk4:
        return "RK4";
    case IntegratorKind::boris:
        return "Boris";
    }
    return "Unknown";
}

/// Recomputes accelerations for the positions and velocities currently stored in a scene.
/// Evaluators must preserve body identities/order/count and modify only kinematics, not topology.
/// Integrators restore entry states if evaluation fails or throws; callbacks must return finite
/// acceleration on success. The current external source models are time independent.
using AccelerationFunction = std::function<core::Status(core::Scene&)>;

} // namespace aetherion::physics
