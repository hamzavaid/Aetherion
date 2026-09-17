#pragma once

#include <vector>

#include "aetherion/math/vec3d.hpp"

namespace aetherion::physics::em {

enum class AnalyticFieldSourceKind { uniform, magnetic_dipole };

/// Analytic external field source. Uniform values use V/m and T; dipole moment uses A m^2.
struct AnalyticFieldSource {
    AnalyticFieldSourceKind kind{AnalyticFieldSourceKind::uniform};
    math::Vec3d position_m;
    math::Vec3d electric_Vpm;
    math::Vec3d magnetic_T;
    math::Vec3d magnetic_dipole_moment_Am2;
    double singularity_radius_m{1.0e-9};
};

struct ElectromagneticSettings {
    bool electrostatics_enabled{};
    bool magnetic_enabled{};
    double minimum_separation_m{1.0e-9};
    double softening_m{};
    std::vector<AnalyticFieldSource> analytic_sources;
};

} // namespace aetherion::physics::em
