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

} // namespace aetherion::presets
