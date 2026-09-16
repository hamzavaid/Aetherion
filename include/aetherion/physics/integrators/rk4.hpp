#pragma once

#include <vector>

#include "aetherion/physics/integrators/integrator.hpp"

namespace aetherion::physics {

/// Classical fourth-order Runge-Kutta on the coupled first-order (position, velocity) system.
class Rk4Integrator final {
  public:
    [[nodiscard]] core::Status step(core::Scene& scene, double dt_s,
                                    const AccelerationFunction& evaluate_acceleration);

  private:
    struct Derivative {
        math::Vec3d position_rate_mps;
        math::Vec3d velocity_rate_mps2;
    };
    void resize(std::size_t count);
    void setStage(core::Scene& scene, const std::vector<Derivative>& derivative, double scale_s);
    void captureDerivative(const core::Scene& scene, std::vector<Derivative>& destination);
    void restore(core::Scene& scene) noexcept;

    std::vector<core::BodyState> initial_;
    std::vector<Derivative> k1_;
    std::vector<Derivative> k2_;
    std::vector<Derivative> k3_;
    std::vector<Derivative> k4_;
};

} // namespace aetherion::physics
