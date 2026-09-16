#include "aetherion/presets/mechanics_presets.hpp"

#include <cmath>
#include <stdexcept>

#include "aetherion/physics/constants.hpp"

namespace aetherion::presets {

core::Scene makeEarthLikeOrbit() {
    constexpr double sun_mass_kg = 1.98847e30;
    constexpr double earth_mass_kg = 5.9722e24;
    constexpr double total_mass_kg = sun_mass_kg + earth_mass_kg;
    const double angular_speed_rad_ps =
        std::sqrt(physics::constants::gravitational_constant * total_mass_kg /
                  (astronomical_unit_m * astronomical_unit_m * astronomical_unit_m));
    const double sun_radius_from_barycenter_m = astronomical_unit_m * earth_mass_kg / total_mass_kg;
    const double earth_radius_from_barycenter_m = astronomical_unit_m * sun_mass_kg / total_mass_kg;

    core::Scene scene;
    const auto sun = scene.createBody(
        core::Body{.name = "Sun",
                   .mass_kg = sun_mass_kg,
                   .radius_m = 6.957e8,
                   .state = {.position_m = {-sun_radius_from_barycenter_m, 0.0, 0.0},
                             .velocity_mps = {
                                 0.0, -angular_speed_rad_ps * sun_radius_from_barycenter_m, 0.0}}});
    const auto earth = scene.createBody(core::Body{
        .name = "Earth",
        .mass_kg = earth_mass_kg,
        .radius_m = 6.371e6,
        .state = {
            .position_m = {earth_radius_from_barycenter_m, 0.0, 0.0},
            .velocity_mps = {0.0, angular_speed_rad_ps * earth_radius_from_barycenter_m, 0.0}}});
    if (!sun || !earth) {
        throw std::runtime_error("built-in Earth-like orbit preset failed validation");
    }
    return scene;
}

} // namespace aetherion::presets
