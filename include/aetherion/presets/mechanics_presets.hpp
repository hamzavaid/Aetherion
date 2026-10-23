/** @file
 * Barycentric two-body and approximate Sun-Earth-Moon Newtonian demonstrations in SI units.
 */
#pragma once

#include "aetherion/core/command_queue.hpp"

namespace aetherion::presets {

inline constexpr double astronomical_unit_m = 149'597'870'700.0;
// Circular period derived from the constants and masses used by makeEarthLikeOrbit().
inline constexpr double earth_like_orbit_period_s = 31'557'671.4826143;

/// Barycentric circular Sun-Earth analogue in the x-y plane using SI units.
[[nodiscard]] core::Scene makeEarthLikeOrbit();

struct GravityPreset {
    core::Scene scene;
    core::RuntimeSettings runtime;
    double meters_to_render_units{1.0};
    float minimum_apparent_radius{0.01F};
    float body_radius_scale{1.0F};
};

/// Circular barycentric Sun-Earth orbit; rendering is scaled separately from physical SI state.
[[nodiscard]] GravityPreset makeEarthSunGravityPreset();
/// Circular barycentric Earth-Moon orbit in the world XZ plane.
[[nodiscard]] GravityPreset makeEarthMoonGravityPreset();
/// Approximate coplanar three-body initial conditions with a fixed Sun, not an ephemeris solution.
[[nodiscard]] GravityPreset makeSunEarthMoonGravityPreset();

} // namespace aetherion::presets
