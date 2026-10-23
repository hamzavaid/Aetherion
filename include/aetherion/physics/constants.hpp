/** @file
 * Shared SI physical constants used by force solvers, field samplers, and analytical fixtures.
 */
#pragma once

namespace aetherion::physics::constants {

inline constexpr double gravitational_constant = 6.67430e-11;   // m^3 kg^-1 s^-2
inline constexpr double coulomb_constant = 8.9875517923e9;      // N m^2 C^-2
inline constexpr double vacuum_permeability = 1.25663706212e-6; // N A^-2
inline constexpr double speed_of_light_mps = 299792458.0;       // exact SI m/s

} // namespace aetherion::physics::constants
