#pragma once

#include <functional>
#include <vector>

#include "aetherion/core/error.hpp"
#include "aetherion/core/scene.hpp"
#include "aetherion/physics/integrators/integrator.hpp"

namespace aetherion::physics {

using MagneticFieldFunction =
    std::function<core::Result<math::Vec3d>(const math::Vec3d& position_m)>;

/// Nonrelativistic Boris kick-rotate-kick pusher in the inertial world frame.
///
/// `nonmagnetic_acceleration` supplies m/s^2 from gravity, electric fields, and other forces. The
/// magnetic callback supplies B in tesla. The centered Cayley rotation preserves speed for a pure
/// magnetic field to roundoff, making this preferable to the general integrators for gyro motion.
/// Fields are treated as constant over one rotation and relativistic effects are outside scope.
class BorisIntegrator final {
  public:
    [[nodiscard]] core::Status step(core::Scene& scene, double dt_s,
                                    const AccelerationFunction& nonmagnetic_acceleration,
                                    const MagneticFieldFunction& magnetic_field);

  private:
    std::vector<core::BodyState> initial_;
};

} // namespace aetherion::physics
