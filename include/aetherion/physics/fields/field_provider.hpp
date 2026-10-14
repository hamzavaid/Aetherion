#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "aetherion/math/vec3d.hpp"

namespace aetherion::physics::fields {

enum class SampleFailure {
    none,
    history_unavailable,
    non_convergence,
    invalid_source,
    singularity,
    outside_history_domain
};

[[nodiscard]] inline std::string_view sampleFailureName(SampleFailure failure) noexcept {
    switch (failure) {
    case SampleFailure::none:
        return "none";
    case SampleFailure::history_unavailable:
        return "history unavailable / warming up";
    case SampleFailure::non_convergence:
        return "retarded iteration limit";
    case SampleFailure::invalid_source:
        return "invalid or non-subluminal source";
    case SampleFailure::singularity:
        return "source guard / singularity";
    case SampleFailure::outside_history_domain:
        return "outside configured history distance";
    }
    return "invalid";
}

/// Electric, magnetic, and gravitational field sample in SI units in the inertial world frame.
struct FieldSample {
    math::Vec3d electric_Vpm;
    math::Vec3d magnetic_T;
    math::Vec3d gravity_mps2;
    bool valid{true};
    /// Gravity can be singular inside a massive source without invalidating an E/B sample.
    bool gravity_valid{true};
    bool retarded{};
    SampleFailure failure{SampleFailure::none};
    double maximum_delay_s{};
    double maximum_residual_s{};
    std::size_t maximum_iterations{};
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
