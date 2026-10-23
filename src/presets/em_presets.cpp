/** @file
 * Deterministic charge, magnetic, and propagation-delay demonstrations with SI state and display
 * defaults.
 */
#include "aetherion/presets/em_presets.hpp"

#include <numbers>
#include <stdexcept>

#include "aetherion/physics/constants.hpp"

namespace aetherion::presets {
namespace {

ElectromagneticPreset makeChargePair(double first_charge_C, double second_charge_C) {
    ElectromagneticPreset preset;
    preset.runtime.physics_dt_s = 1.0e-5;
    preset.runtime.time_scale = 1.0e-3;
    preset.runtime.gravity_enabled = false;
    preset.runtime.electromagnetism.electrostatics_enabled = true;
    preset.runtime.electromagnetism.minimum_separation_m = 1.0e-3;
    const auto first = preset.scene.createBody({.name = "Charge A",
                                                .mass_kg = 1.0,
                                                .charge_C = first_charge_C,
                                                .radius_m = 0.08,
                                                .state = {.position_m = {-1.0, 0.0, 0.0}}});
    const auto second = preset.scene.createBody({.name = "Charge B",
                                                 .mass_kg = 1.0,
                                                 .charge_C = second_charge_C,
                                                 .radius_m = 0.08,
                                                 .state = {.position_m = {1.0, 0.0, 0.0}}});
    if (!first || !second)
        throw std::runtime_error("built-in charge-pair preset failed validation");
    preset.visualization.mode = renderer::FieldDisplayMode::field_lines;
    preset.visualization.field = renderer::ObservedField::electric;
    preset.visualization.region.half_extent_m = {3.0, 3.0, 3.0};
    preset.visualization.vectors.resolution = 13;
    preset.visualization.vectors.visual_length_m = 0.2;
    preset.visualization.vectors.scaling = renderer::VectorScaling::logarithmic;
    preset.visualization.vectors.reference_magnitude = 1.0e4;
    preset.visualization.lines.automatic_seed_count = 24;
    preset.visualization.lines.step_size_m = 0.04;
    preset.visualization.lines.maximum_length_m = 12.0;
    return preset;
}

ElectromagneticPreset makeUniformFieldParticle(const math::Vec3d& velocity_mps,
                                               const math::Vec3d& electric_Vpm,
                                               const math::Vec3d& magnetic_T) {
    ElectromagneticPreset preset;
    preset.runtime.physics_dt_s = 2.0 * std::numbers::pi / 200.0;
    preset.runtime.gravity_enabled = false;
    preset.runtime.integrator = physics::IntegratorKind::boris;
    preset.runtime.electromagnetism.electrostatics_enabled = electric_Vpm.squaredNorm() > 0.0;
    preset.runtime.electromagnetism.magnetic_enabled = magnetic_T.squaredNorm() > 0.0;
    preset.runtime.electromagnetism.analytic_sources.push_back(
        {.electric_Vpm = electric_Vpm, .magnetic_T = magnetic_T});
    const auto particle = preset.scene.createBody({.name = "Charged particle",
                                                   .mass_kg = 1.0,
                                                   .charge_C = 1.0,
                                                   .radius_m = 0.06,
                                                   .state = {.velocity_mps = velocity_mps}});
    if (!particle)
        throw std::runtime_error("built-in magnetic particle preset failed validation");
    preset.visualization.field = renderer::ObservedField::magnetic;
    preset.visualization.region.half_extent_m = {2.0, 2.0, 2.0};
    preset.visualization.vectors.geometry = renderer::SamplingGeometry::plane_xy;
    preset.visualization.vectors.resolution = 11;
    preset.visualization.vectors.visual_length_m = 0.15;
    preset.visualization.vectors.reference_magnitude = 1.0;
    preset.visualization.lines.automatic_seed_count = 16;
    preset.visualization.lines.step_size_m = 0.05;
    preset.visualization.lines.maximum_length_m = 5.0;
    return preset;
}

} // namespace

ElectromagneticPreset makeLikeChargesPreset() { return makeChargePair(1.0e-6, 1.0e-6); }

ElectromagneticPreset makeOppositeChargesPreset() { return makeChargePair(1.0e-6, -1.0e-6); }

ElectromagneticPreset makeElectricDipolePreset() {
    auto preset = makeOppositeChargesPreset();
    preset.scene.bodies()[0].name = "Dipole +";
    preset.scene.bodies()[1].name = "Dipole -";
    preset.scene.bodies()[0].fixed = true;
    preset.scene.bodies()[1].fixed = true;
    preset.visualization.lines.automatic_seed_count = 32;
    return preset;
}

ElectromagneticPreset makeUniformMagneticGyroPreset() {
    auto preset = makeUniformFieldParticle({1.0, 0.0, 0.0}, {}, {0.0, 0.0, 1.0});
    preset.visualization.mode = renderer::FieldDisplayMode::observed_vectors;
    return preset;
}

ElectromagneticPreset makeHelicalMagneticPreset() {
    auto preset = makeUniformFieldParticle({1.0, 0.0, 0.25}, {}, {0.0, 0.0, 1.0});
    preset.visualization.mode = renderer::FieldDisplayMode::field_lines;
    preset.visualization.region.half_extent_m = {2.0, 2.0, 4.0};
    return preset;
}

ElectromagneticPreset makeCrossedFieldsPreset() {
    auto preset = makeUniformFieldParticle({1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0});
    preset.visualization.mode = renderer::FieldDisplayMode::observed_vectors;
    return preset;
}

ElectromagneticPreset makeMagneticVectorPreset() {
    auto preset = makeUniformMagneticGyroPreset();
    preset.scene.bodies()[0].fixed = true;
    preset.visualization.mode = renderer::FieldDisplayMode::observed_vectors;
    preset.visualization.vectors.geometry = renderer::SamplingGeometry::volume;
    return preset;
}

ElectromagneticPreset makeMagneticFieldLinesPreset() {
    auto preset = makeUniformMagneticGyroPreset();
    preset.scene.bodies()[0].fixed = true;
    preset.visualization.mode = renderer::FieldDisplayMode::field_lines;
    preset.visualization.lines.trace_forward = true;
    preset.visualization.lines.trace_backward = true;
    return preset;
}

ElectromagneticPreset makePropagationDelayPreset() {
    ElectromagneticPreset preset;
    preset.body_radius_scale = 6.0F;
    preset.runtime.gravity_enabled = false;
    preset.runtime.physics_dt_s = 1.0e-9;
    preset.runtime.time_scale = 1.0e-8;
    preset.runtime.electromagnetism.retarded.enabled = true;
    preset.runtime.electromagnetism.retarded.maximum_observable_distance_m = 30.0;
    const auto id = preset.scene.createBody(
        {.name = "Moving charge (0.01c)",
         .mass_kg = 1.0,
         .charge_C = 1.0e-6,
         .radius_m = 0.05,
         .state = {.velocity_mps = {0.0, 0.01 * physics::constants::speed_of_light_mps, 0.0}}});
    if (!id)
        throw std::runtime_error("propagation-delay preset failed validation");
    preset.visualization.mode = renderer::FieldDisplayMode::observed_vectors;
    preset.visualization.field = renderer::ObservedField::electric;
    preset.visualization.region.center_m = {3.0, 0.0, 0.0};
    preset.visualization.region.half_extent_m = {4.0, 4.0, 4.0};
    preset.visualization.vectors.geometry = renderer::SamplingGeometry::plane_xy;
    preset.visualization.vectors.resolution = 9;
    preset.visualization.vectors.visual_length_m = 0.3;
    preset.probes = {
        {.name = "Near probe (3 m)", .position_m = {3.0, 0.0, 0.0}, .show_electric_vector = true},
        {.name = "Far probe (6 m)", .position_m = {6.0, 0.0, 0.0}, .show_electric_vector = true}};
    return preset;
}

} // namespace aetherion::presets
