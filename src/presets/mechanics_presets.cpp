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

GravityPreset makeEarthSunGravityPreset() {
    GravityPreset preset;
    preset.scene = makeEarthLikeOrbit();
    preset.runtime.physics_dt_s = 3'600.0;
    preset.runtime.time_scale = 86'400.0;
    preset.runtime.max_substeps = 16;
    preset.runtime.gravity_enabled = true;
    preset.runtime.integrator = physics::IntegratorKind::velocity_verlet;
    preset.meters_to_render_units = 1.0e-9;
    preset.minimum_apparent_radius = 0.02F;
    return preset;
}

GravityPreset makeEarthMoonGravityPreset() {
    constexpr double earth_mass_kg = 5.9722e24;
    constexpr double moon_mass_kg = 7.342e22;
    constexpr double separation_m = 384.4e6;
    constexpr double total_mass_kg = earth_mass_kg + moon_mass_kg;
    const double omega = std::sqrt(physics::constants::gravitational_constant * total_mass_kg /
                                   (separation_m * separation_m * separation_m));
    const double earth_offset_m = separation_m * moon_mass_kg / total_mass_kg;
    const double moon_offset_m = separation_m * earth_mass_kg / total_mass_kg;
    GravityPreset preset;
    const auto earth =
        preset.scene.createBody({.name = "Earth",
                                 .mass_kg = earth_mass_kg,
                                 .radius_m = 6.371e6,
                                 .state = {.position_m = {-earth_offset_m, 0.0, 0.0},
                                           .velocity_mps = {0.0, 0.0, -omega * earth_offset_m}}});
    const auto moon =
        preset.scene.createBody({.name = "Moon",
                                 .mass_kg = moon_mass_kg,
                                 .radius_m = 1.7374e6,
                                 .state = {.position_m = {moon_offset_m, 0.0, 0.0},
                                           .velocity_mps = {0.0, 0.0, omega * moon_offset_m}}});
    if (!earth || !moon)
        throw std::runtime_error("built-in Earth-Moon preset failed validation");
    preset.runtime.physics_dt_s = 300.0;
    preset.runtime.time_scale = 3'600.0;
    preset.runtime.max_substeps = 16;
    preset.runtime.gravity_enabled = true;
    preset.runtime.integrator = physics::IntegratorKind::velocity_verlet;
    preset.meters_to_render_units = 1.0e-7;
    preset.minimum_apparent_radius = 0.03F;
    return preset;
}

GravityPreset makeSunEarthMoonGravityPreset() {
    constexpr double sun_mass_kg = 1.98847e30;
    constexpr double earth_mass_kg = 5.9722e24;
    constexpr double moon_mass_kg = 7.342e22;
    constexpr double moon_separation_m = 384.4e6;
    GravityPreset preset;
    const double earth_speed_mps =
        std::sqrt(physics::constants::gravitational_constant * sun_mass_kg / astronomical_unit_m);
    const double moon_relative_speed_mps =
        std::sqrt(physics::constants::gravitational_constant * earth_mass_kg / moon_separation_m);
    const auto sun = preset.scene.createBody(
        {.name = "Sun", .mass_kg = sun_mass_kg, .radius_m = 6.957e8, .fixed = true});
    const auto earth =
        preset.scene.createBody({.name = "Earth",
                                 .mass_kg = earth_mass_kg,
                                 .radius_m = 6.371e6,
                                 .state = {.position_m = {astronomical_unit_m, 0.0, 0.0},
                                           .velocity_mps = {0.0, 0.0, earth_speed_mps}}});
    const auto moon = preset.scene.createBody(
        {.name = "Moon",
         .mass_kg = moon_mass_kg,
         .radius_m = 1.7374e6,
         .state = {.position_m = {astronomical_unit_m + moon_separation_m, 0.0, 0.0},
                   .velocity_mps = {0.0, 0.0, earth_speed_mps + moon_relative_speed_mps}}});
    if (!sun || !earth || !moon)
        throw std::runtime_error("built-in Sun-Earth-Moon preset failed validation");
    preset.runtime.physics_dt_s = 300.0;
    preset.runtime.time_scale = 3'600.0;
    preset.runtime.max_substeps = 16;
    preset.runtime.gravity_enabled = true;
    preset.runtime.integrator = physics::IntegratorKind::velocity_verlet;
    preset.meters_to_render_units = 1.0e-9;
    preset.minimum_apparent_radius = 0.01F;
    return preset;
}

} // namespace aetherion::presets
