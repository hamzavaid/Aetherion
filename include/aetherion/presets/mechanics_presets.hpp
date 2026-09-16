#pragma once

#include "aetherion/core/scene.hpp"

namespace aetherion::presets {

inline constexpr double astronomical_unit_m = 149'597'870'700.0;
inline constexpr double earth_like_orbit_period_s = 365.256363004 * 86'400.0;

/// Barycentric circular Sun-Earth analogue in the x-y plane using SI units.
[[nodiscard]] core::Scene makeEarthLikeOrbit();

} // namespace aetherion::presets
