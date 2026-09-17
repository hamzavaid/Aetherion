#pragma once

#include <cstdint>

#include "aetherion/math/vec3d.hpp"

namespace aetherion::physics::fields {

/// Electric, magnetic, and gravitational field sample in SI units in the inertial world frame.
struct FieldSample {
    math::Vec3d electric_Vpm;
    math::Vec3d magnetic_T;
    math::Vec3d gravity_mps2;
    bool valid{true};
    /// Gravity can be singular inside a massive source without invalidating an E/B sample.
    bool gravity_valid{true};
};

/// Read-only arbitrary-position field interface shared by physics and visualization clients.
class IFieldProvider {
  public:
    virtual ~IFieldProvider() = default;
    [[nodiscard]] virtual FieldSample sample(const math::Vec3d& position_m,
                                             double time_s) const = 0;
    /// Deterministic signature of source state used to invalidate visualization caches.
    [[nodiscard]] virtual std::uint64_t revision() const noexcept = 0;
};

} // namespace aetherion::physics::fields
