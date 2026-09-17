#pragma once

#include <functional>
#include <string_view>

#include "aetherion/core/error.hpp"
#include "aetherion/core/scene.hpp"

namespace aetherion::physics {

enum class IntegratorKind { semi_implicit_euler, velocity_verlet, rk4, boris };

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
using AccelerationFunction = std::function<core::Status(core::Scene&)>;

} // namespace aetherion::physics
