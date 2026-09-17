#pragma once

#include "aetherion/core/command_queue.hpp"
#include "aetherion/core/scene.hpp"
#include "aetherion/renderer/field_visualization.hpp"

namespace aetherion::presets {

struct ElectromagneticPreset {
    core::Scene scene;
    core::RuntimeSettings runtime;
    renderer::FieldVisualizationSettings visualization;
    double meters_to_render_units{1.0};
    float minimum_apparent_radius{0.01F};
    float body_radius_scale{1.0F};
};

[[nodiscard]] ElectromagneticPreset makeLikeChargesPreset();
[[nodiscard]] ElectromagneticPreset makeOppositeChargesPreset();
[[nodiscard]] ElectromagneticPreset makeElectricDipolePreset();
[[nodiscard]] ElectromagneticPreset makeUniformMagneticGyroPreset();
[[nodiscard]] ElectromagneticPreset makeHelicalMagneticPreset();
[[nodiscard]] ElectromagneticPreset makeCrossedFieldsPreset();
[[nodiscard]] ElectromagneticPreset makeMagneticVectorPreset();
[[nodiscard]] ElectromagneticPreset makeMagneticFieldLinesPreset();

} // namespace aetherion::presets
