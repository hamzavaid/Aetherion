/** @file
 * Kick-drift-kick Velocity Verlet integration for position-dependent conservative accelerations.
 */
#pragma once

#include "aetherion/physics/integrators/integrator.hpp"

namespace aetherion::physics {

/// Second-order symplectic position/velocity update for conservative acceleration fields.
/// Assumes acceleration depends on position, not velocity; use Boris or RK4 for Lorentz motion.
class VelocityVerletIntegrator final {
  public:
    [[nodiscard]] core::Status step(core::Scene& scene, double dt_s,
                                    const AccelerationFunction& evaluate_acceleration) const;
};

} // namespace aetherion::physics
