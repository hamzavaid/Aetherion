#pragma once

#include "aetherion/core/error.hpp"
#include "aetherion/core/scene.hpp"

namespace aetherion::physics {

/// First-order symplectic Euler update: v[n+1]=v[n]+a[n]dt, x[n+1]=x[n]+v[n+1]dt.
class SemiImplicitEuler final {
  public:
    [[nodiscard]] static core::Status integrate(core::Scene& scene, double dt_s);
};

} // namespace aetherion::physics
