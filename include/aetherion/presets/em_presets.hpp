/** @file
 * Deterministic charge, magnetic, and propagation-delay demonstrations with SI state and display
 * defaults.
 */
#pragma once

#include "aetherion/core/command_queue.hpp"
#include "aetherion/core/scene.hpp"
#include "aetherion/core/scientific_analysis.hpp"
#include "aetherion/renderer/field_visualization.hpp"

namespace aetherion::presets {

struct ElectromagneticPreset {
    core::Scene scene;
    core::RuntimeSettings runtime;
    renderer::FieldVisualizationSettings visualization;
    double meters_to_render_units{1.0};
    float minimum_apparent_radius{0.01F};
    float body_radius_scale{1.0F};
    std::vector<core::FieldProbe> probes;
};

/// Two equal +1 microcoulomb mobile sources, initially separated by 2 m.
[[nodiscard]] ElectromagneticPreset makeLikeChargesPreset();
/// A +/-1 microcoulomb mobile pair, initially separated by 2 m.
[[nodiscard]] ElectromagneticPreset makeOppositeChargesPreset();
/// The opposite-charge pair held fixed, useful for stationary field lines and probes.
[[nodiscard]] ElectromagneticPreset makeElectricDipolePreset();
/// Unit-mass/unit-charge particle with 1 m/s transverse velocity in a uniform 1 T z-field.
[[nodiscard]] ElectromagneticPreset makeUniformMagneticGyroPreset();
/// Adds 0.25 m/s velocity parallel to the same uniform magnetic field.
[[nodiscard]] ElectromagneticPreset makeHelicalMagneticPreset();
/// Uniform E=(0,1,0) V/m and B=(0,0,1) T cancel at initial v=(1,0,0) m/s.
[[nodiscard]] ElectromagneticPreset makeCrossedFieldsPreset();
/// Stationary source demonstration with a 3D observed magnetic-vector lattice.
[[nodiscard]] ElectromagneticPreset makeMagneticVectorPreset();
/// Stationary source demonstration with bidirectional uniform magnetic-field traces.
[[nodiscard]] ElectromagneticPreset makeMagneticFieldLinesPreset();
/// Low-speed freely moving source with 3 m / 6 m probes and nanosecond light-flight timing.
[[nodiscard]] ElectromagneticPreset makePropagationDelayPreset();

} // namespace aetherion::presets
