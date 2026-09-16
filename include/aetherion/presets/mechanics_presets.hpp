#pragma once

#include "aetherion/core/scene.hpp"

namespace aetherion::presets {

inline constexpr double astronomical_unit_m = 149'597'870'700.0;
// Circular period derived from the constants and masses used by makeEarthLikeOrbit().
inline constexpr double earth_like_orbit_period_s = 31'557'671.4826143;

/// Barycentric circular Sun-Earth analogue in the x-y plane using SI units.
[[nodiscard]] core::Scene makeEarthLikeOrbit();

} // namespace aetherion::presets
