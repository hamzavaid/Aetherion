#pragma once

#include "aetherion/physics/integrators/integrator.hpp"

namespace aetherion::physics {

/// Second-order symplectic position/velocity update for conservative acceleration fields.
class VelocityVerletIntegrator final {
  public:
    [[nodiscard]] core::Status step(core::Scene& scene, double dt_s,
                                    const AccelerationFunction& evaluate_acceleration) const;
};

} // namespace aetherion::physics
