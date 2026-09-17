#pragma once

#include "aetherion/core/command_queue.hpp"
#include "aetherion/core/scene.hpp"
#include "aetherion/renderer/field_visualization.hpp"

namespace aetherion::presets {

struct ElectromagneticPreset {
    core::Scene scene;
    core::RuntimeSettings runtime;
    renderer::FieldVisualizationSettings visualization;
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
